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

plan_state_packer_t *planStatePackerNew(const plan_var_t *var,
                                        int var_size)
{
    int i, is_set, bitlen, wordpos;
    plan_state_packer_t *p;
    plan_state_packer_var_t **pvars;


    p = BOR_ALLOC(plan_state_packer_t);
    p->num_vars = var_size;
    p->vars = BOR_ALLOC_ARR(plan_state_packer_var_t, p->num_vars);

    pvars = BOR_ALLOC_ARR(plan_state_packer_var_t *, p->num_vars);

    for (i = 0; i < var_size; ++i){
        p->vars[i].bitlen = packerBitsNeeded(var[i].range);
        p->vars[i].pos = -1;
        pvars[i] = p->vars + i;
    }
    qsort(pvars, var_size, sizeof(plan_state_packer_var_t *), sortCmpVar);

    is_set = 0;
    wordpos = -1;
    while (is_set < var_size){
        ++wordpos;

        bitlen = 0;
        for (i = 0; i < var_size; ++i){
            if (pvars[i]->pos == -1
                    && pvars[i]->bitlen + bitlen <= PLAN_PACKER_WORD_BITS){
                pvars[i]->pos = wordpos;
                bitlen += pvars[i]->bitlen;
                pvars[i]->shift = PLAN_PACKER_WORD_BITS - bitlen;
                pvars[i]->mask = packerVarMask(pvars[i]->bitlen,
                                               pvars[i]->shift);
                pvars[i]->clear_mask = ~pvars[i]->mask;
                ++is_set;
            }
        }
    }

    p->bufsize = sizeof(plan_packer_word_t) * (wordpos + 1);

    // free temporary array
    BOR_FREE(pvars);

    /*
    for (i = 0; i < var_size; ++i){
        fprintf(stderr, "%d %d %d %d\n", (int)PLAN_PACKER_WORD_BITS,
                p->vars[i].bitlen, p->vars[i].shift,
                p->vars[i].pos);
    }
    fprintf(stderr, "%lu\n", p->bufsize);
    */
    return p;
}

void planStatePackerDel(plan_state_packer_t *p)
{
    if (p->vars)
        BOR_FREE(p->vars);
    BOR_FREE(p);
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
    part_state->valbuf  = BOR_ALLOC_ARR(char, p->bufsize);
    part_state->maskbuf = BOR_ALLOC_ARR(char, p->bufsize);
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
