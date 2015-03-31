#include <string.h>
#include <boruvka/alloc.h>
#include "plan/state_packer.h"

struct _plan_state_packer_var_t {
    int bitlen;                    /*!< Number of bits required to store a value */
    plan_packer_word_t shift;      /*!< Left shift size during packing */
    plan_packer_word_t mask;       /*!< Mask for packed values */
    plan_packer_word_t clear_mask; /*!< Clear mask for packed values (i.e., ~mask) */
    int pos;                       /*!< Position of a word in buffer where
                                        values are stored. */
};
typedef struct _plan_state_packer_var_t plan_state_packer_var_t;

/** Returns number of bits needed for storing all values in interval
 * [0,range). */
static int packerBitsNeeded(plan_val_t range);
/** Comparator for sorting variables by their bit length */
static int sortCmpVar(const void *a, const void *b);
/** Constructs mask for variable */
static plan_packer_word_t packerVarMask(int bitlen, int shift);
/** Set variable into packed buffer */
static void packerSetVar(const plan_state_packer_var_t *var,
                         plan_packer_word_t val,
                         void *buffer);
/** Retrieves a value of the variable from packed buffer */
static plan_val_t packerGetVar(const plan_state_packer_var_t *var,
                               const void *buffer);

struct _sorted_vars_t {
    plan_state_packer_var_t **pub;
    int pub_size;
    int pub_left;
    plan_state_packer_var_t **private;
    int private_size;
    int private_left;
    plan_state_packer_var_t **ma_privacy;
    int ma_privacy_size;
    int ma_privacy_left;
};
typedef struct _sorted_vars_t sorted_vars_t;

static void sortedVarsInit(sorted_vars_t *sv, const plan_var_t *var,
                           int var_size, plan_state_packer_var_t *pvar);
static void sortedVarsFree(sorted_vars_t *sv);
static plan_state_packer_var_t *sortedVarsNext(sorted_vars_t *sv,
                                               int filled_bits);


plan_state_packer_t *planStatePackerNew(const plan_var_t *var,
                                        int var_size)
{
    int i, is_set, bitlen, wordpos;
    plan_state_packer_t *p;
    sorted_vars_t sorted_vars;
    plan_state_packer_var_t *pvar;

    p = BOR_ALLOC(plan_state_packer_t);
    p->num_vars = var_size;
    p->vars = BOR_ALLOC_ARR(plan_state_packer_var_t, p->num_vars);
    for (i = 0; i < var_size; ++i){
        p->vars[i].bitlen = packerBitsNeeded(var[i].range);
        p->vars[i].pos = -1;
    }

    sortedVarsInit(&sorted_vars, var, var_size, p->vars);

    is_set = 0;
    wordpos = -1;
    while (is_set < var_size){
        ++wordpos;

        bitlen = 0;
        while ((pvar = sortedVarsNext(&sorted_vars, bitlen)) != NULL){
            pvar->pos = wordpos;
            bitlen += pvar->bitlen;
            pvar->shift = PLAN_PACKER_WORD_BITS - bitlen;
            pvar->mask = packerVarMask(pvar->bitlen, pvar->shift);
            pvar->clear_mask = ~pvar->mask;
            ++is_set;
        }
    }

    p->bufsize = sizeof(plan_packer_word_t) * (wordpos + 1);

    sortedVarsFree(&sorted_vars);
    /*
    for (i = 0; i < var_size; ++i){
        fprintf(stdout, "[%d] bitlen: %d, shift: %d, pos: %d\n",
                i, p->vars[i].bitlen, p->vars[i].shift,
                p->vars[i].pos);
    }
    fprintf(stdout, "%d\n", p->bufsize);
    */
    return p;
}

void planStatePackerDel(plan_state_packer_t *p)
{
    if (p->vars)
        BOR_FREE(p->vars);
    BOR_FREE(p);
}

plan_state_packer_t *planStatePackerClone(const plan_state_packer_t *p)
{
    plan_state_packer_t *packer;

    packer = BOR_ALLOC(plan_state_packer_t);
    packer->num_vars = p->num_vars;
    packer->bufsize = p->bufsize;
    packer->vars = BOR_ALLOC_ARR(plan_state_packer_var_t, p->num_vars);
    memcpy(packer->vars, p->vars,
           sizeof(plan_state_packer_var_t) * p->num_vars);
    return packer;
}

void planStatePackerPack(const plan_state_packer_t *p,
                         const plan_state_t *state,
                         void *buffer)
{
    int i;
    for (i = 0; i < p->num_vars; ++i){
        packerSetVar(p->vars + i, planStateGet(state, i), buffer);
    }
}

void planStatePackerUnpack(const plan_state_packer_t *p,
                           const void *buffer,
                           plan_state_t *state)
{
    int i;
    for (i = 0; i < p->num_vars; ++i){
        planStateSet(state, i, packerGetVar(p->vars + i, buffer));
    }
}

void planStatePackerPackPartState(const plan_state_packer_t *p,
                                  plan_part_state_t *part_state)
{
    plan_var_id_t var;
    plan_val_t val;
    plan_packer_word_t *wbuf;
    int i;

    // First allocate buffers
    if (part_state->valbuf)
        BOR_FREE(part_state->valbuf);
    if (part_state->maskbuf)
        BOR_FREE(part_state->maskbuf);

    part_state->bufsize = p->bufsize;
    part_state->valbuf  = BOR_CALLOC_ARR(char, p->bufsize);
    part_state->maskbuf = BOR_CALLOC_ARR(char, p->bufsize);
    wbuf = part_state->maskbuf;

    PLAN_PART_STATE_FOR_EACH(part_state, i, var, val){
        packerSetVar(p->vars + var, val, part_state->valbuf);
        wbuf[p->vars[var].pos] |= p->vars[var].mask;
    }
}

static int packerBitsNeeded(plan_val_t range)
{
    plan_packer_word_t max_val = range - 1;
    int num = PLAN_PACKER_WORD_BITS;

    for (; !(max_val & PLAN_PACKER_WORD_SET_HI_BIT); --num, max_val <<= 1);
    return num;
}

static int sortCmpVar(const void *a, const void *b)
{
    const plan_state_packer_var_t *va = *(const plan_state_packer_var_t **)a;
    const plan_state_packer_var_t *vb = *(const plan_state_packer_var_t **)b;
    return va->bitlen - vb->bitlen;
}

static plan_packer_word_t packerVarMask(int bitlen, int shift)
{
    plan_packer_word_t mask1, mask2;

    mask1 = PLAN_PACKER_WORD_SET_ALL_BITS << shift;
    mask2 = PLAN_PACKER_WORD_SET_ALL_BITS
                >> (PLAN_PACKER_WORD_BITS - shift - bitlen);
    return mask1 & mask2;
}

static void packerSetVar(const plan_state_packer_var_t *var,
                         plan_packer_word_t val,
                         void *buffer)
{
    plan_packer_word_t *buf;
    val = (val << var->shift) & var->mask;
    buf = ((plan_packer_word_t *)buffer) + var->pos;
    *buf = (*buf & var->clear_mask) | val;
}

static plan_val_t packerGetVar(const plan_state_packer_var_t *var,
                               const void *buffer)
{
    plan_packer_word_t val;
    plan_packer_word_t *buf;
    buf = ((plan_packer_word_t *)buffer) + var->pos;
    val = *buf;
    val = (val & var->mask) >> var->shift;
    return val;
}


static void sortedVarsInit(sorted_vars_t *sv, const plan_var_t *var,
                           int var_size, plan_state_packer_var_t *pvar)
{
    int i;
    size_t elsize = sizeof(plan_state_packer_var_t *);


    // Allocate arrays
    sv->pub = BOR_ALLOC_ARR(plan_state_packer_var_t *, var_size);
    sv->private = BOR_ALLOC_ARR(plan_state_packer_var_t *, var_size);
    sv->ma_privacy = BOR_ALLOC_ARR(plan_state_packer_var_t *, var_size);

    // Find out size of each category and fill arrays
    sv->pub_size = sv->private_size = sv->ma_privacy_size = 0;
    for (i = 0; i < var_size; ++i){
        if (var[i].ma_privacy){
            sv->ma_privacy[sv->ma_privacy_size++] = pvar + i;
        }else if (var[i].is_private){
            sv->private[sv->private_size++] = pvar + i;
        }else{
            sv->pub[sv->pub_size++] = pvar + i;
        }
    }

    // Sort arrays
    if (sv->pub_size > 1)
        qsort(sv->pub, sv->pub_size, elsize, sortCmpVar);
    if (sv->private_size > 1)
        qsort(sv->private, sv->private_size, elsize, sortCmpVar);
    if (sv->pub_size > 1)
        qsort(sv->ma_privacy, sv->ma_privacy_size, elsize, sortCmpVar);

    sv->pub_left = sv->pub_size;
    sv->private_left = sv->private_size;
    sv->ma_privacy_left = sv->ma_privacy_size;
}

static void sortedVarsFree(sorted_vars_t *sv)
{
    if (sv->pub)
        BOR_FREE(sv->pub);
    if (sv->private)
        BOR_FREE(sv->private);
    if (sv->ma_privacy)
        BOR_FREE(sv->ma_privacy);
}

#define SORTED_VARS_NEXT(sv, type, i, filled) \
    if ((sv)->type[(i)]->pos == -1 \
            && (sv)->type[(i)]->bitlen + (filled) <= PLAN_PACKER_WORD_BITS){ \
        (sv)->type##_left--; \
        return (sv)->type[(i)]; \
    }

static plan_state_packer_var_t *sortedVarsNext(sorted_vars_t *sv,
                                               int filled_bits)
{
    int i;

    for (i = 0; sv->pub_left > 0 && i < sv->pub_size; ++i){
        SORTED_VARS_NEXT(sv, pub, i, filled_bits)
    }

    for (i = 0; sv->private_left > 0
                    && sv->pub_left == 0
                    && i < sv->private_size; ++i){
        SORTED_VARS_NEXT(sv, private, i, filled_bits)
    }

    for (i = 0; sv->ma_privacy_left > 0
                    && sv->pub_left == 0
                    && sv->private_left == 0
                    && i < sv->ma_privacy_size; ++i){
        SORTED_VARS_NEXT(sv, ma_privacy, i, filled_bits)
    }

    return NULL;
}

