#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

#include <boruvka/alloc.h>

#include "plan/pddl_lisp.h"
#include "pddl_err.h"

#define IS_WS(c) ((c) == ' ' || (c) == '\n' || (c) == '\r' || (c) == '\t')
#define IS_ALPHA(c) (!IS_WS(c) && (c) != ')' && (c) != '(' && (c) != ';')

struct _kw_t {
    const char *text;
    int kw;
};
typedef struct _kw_t kw_t;

static kw_t kw[] = {
    { "define", PLAN_PDDL_KW_DEFINE },
    { "domain", PLAN_PDDL_KW_DOMAIN },
    { ":domain", PLAN_PDDL_KW_DOMAIN },
    { ":requirements", PLAN_PDDL_KW_REQUIREMENTS },
    { ":types", PLAN_PDDL_KW_TYPES },
    { ":predicates", PLAN_PDDL_KW_PREDICATES },
    { ":constants", PLAN_PDDL_KW_CONSTANTS },
    { ":action", PLAN_PDDL_KW_ACTION },
    { ":parameters", PLAN_PDDL_KW_PARAMETERS },
    { ":precondition", PLAN_PDDL_KW_PRE },
    { ":effect", PLAN_PDDL_KW_EFF },

    { ":strips", PLAN_PDDL_KW_STRIPS },
    { ":typing", PLAN_PDDL_KW_TYPING },
    { ":negative-preconditions", PLAN_PDDL_KW_NEGATIVE_PRE },
    { ":disjunctive-preconditions", PLAN_PDDL_KW_DISJUNCTIVE_PRE },
    { ":equality", PLAN_PDDL_KW_EQUALITY },
    { ":existential-preconditions", PLAN_PDDL_KW_EXISTENTIAL_PRE },
    { ":universal-preconditions", PLAN_PDDL_KW_UNIVERSAL_PRE },
    { ":conditional-effects", PLAN_PDDL_KW_CONDITIONAL_EFF },
    { ":numeric-fluents", PLAN_PDDL_KW_NUMERIC_FLUENT },
    { ":numeric-fluents", PLAN_PDDL_KW_OBJECT_FLUENT },
    { ":durative-actions", PLAN_PDDL_KW_DURATIVE_ACTION },
    { ":duration-inequalities", PLAN_PDDL_KW_DURATION_INEQUALITY },
    { ":continuous-effects", PLAN_PDDL_KW_CONTINUOUS_EFF },
    { ":derived-predicates", PLAN_PDDL_KW_DERIVED_PRED },
    { ":timed-initial-literals", PLAN_PDDL_KW_TIMED_INITIAL_LITERAL },
    { ":durative-actions", PLAN_PDDL_KW_DURATIVE_ACTION },
    { ":preferences", PLAN_PDDL_KW_PREFERENCE },
    { ":constraints", PLAN_PDDL_KW_CONSTRAINT },
    { ":action-costs", PLAN_PDDL_KW_ACTION_COST },
    { ":multi-agent", PLAN_PDDL_KW_MULTI_AGENT },
    { ":unfactored-privacy", PLAN_PDDL_KW_UNFACTORED_PRIVACY },
    { ":factored_privacy", PLAN_PDDL_KW_FACTORED_PRIVACY },
    { ":quantified-preconditions", PLAN_PDDL_KW_QUANTIFIED_PRE },
    { ":fluents", PLAN_PDDL_KW_FLUENTS },
    { ":adl", PLAN_PDDL_KW_ADL },
    { ":multi-agent", PLAN_PDDL_KW_MULTI_AGENT },
    { ":unfactored-privacy", PLAN_PDDL_KW_UNFACTORED_PRIVACY },
    { ":factored-privacy", PLAN_PDDL_KW_FACTORED_PRIVACY },

    { ":functions", PLAN_PDDL_KW_FUNCTIONS },
    { "number", PLAN_PDDL_KW_NUMBER },
    { "problem", PLAN_PDDL_KW_PROBLEM },
    { ":objects", PLAN_PDDL_KW_OBJECTS },
    { ":init", PLAN_PDDL_KW_INIT },
    { ":goal", PLAN_PDDL_KW_GOAL },
    { ":metric", PLAN_PDDL_KW_METRIC },
    { "minimize", PLAN_PDDL_KW_MINIMIZE },
    { "maximize", PLAN_PDDL_KW_MAXIMIZE },
    { "increase", PLAN_PDDL_KW_INCREASE },

    { "and", PLAN_PDDL_KW_AND },
    { "or", PLAN_PDDL_KW_OR },
    { "not", PLAN_PDDL_KW_NOT },
    { "imply", PLAN_PDDL_KW_IMPLY },
    { "exists", PLAN_PDDL_KW_EXISTS },
    { "forall", PLAN_PDDL_KW_FORALL },
    { "when", PLAN_PDDL_KW_WHEN },

    { ":private", PLAN_PDDL_KW_PRIVATE },
    { ":agent", PLAN_PDDL_KW_AGENT },
};
static int kw_size = sizeof(kw) / sizeof(kw_t);

static int recongnizeKeyword(const char *text)
{
    int i;

    for (i = 0; i < kw_size; ++i){
        if (strcmp(text, kw[i].text) == 0)
            return kw[i].kw;
    }

    return -1;
}

static plan_pddl_lisp_node_t *lispNodeInit(plan_pddl_lisp_node_t *n)
{
    n->value = NULL;
    n->kw = -1;
    n->lineno = -1;
    n->child = NULL;
    n->child_size = 0;
    return n;
}

static void lispNodeFree(plan_pddl_lisp_node_t *n)
{
    int i;

    for (i = 0; i < n->child_size; ++i)
        lispNodeFree(n->child + i);
    if (n->child != NULL)
        BOR_FREE(n->child);
}

static plan_pddl_lisp_node_t *lispNodeAddChild(plan_pddl_lisp_node_t *r)
{
    plan_pddl_lisp_node_t *n;

    ++r->child_size;
    r->child = BOR_REALLOC_ARR(r->child, plan_pddl_lisp_node_t,
                               r->child_size);
    n = r->child + r->child_size - 1;
    lispNodeInit(n);
    return n;
}

static int parseExp(plan_pddl_lisp_node_t *root, int *lineno,
                    char *data, int from, int size, int *cont)
{
    plan_pddl_lisp_node_t *sub;
    int i = from;
    char c;

    if (i >= size){
        fprintf(stderr, "Error PDDL: Error on line %d\n", *lineno);
        return -1;
    }

    c = data[i];
    while (i < size){
        if (IS_WS(c)){
            // Skip whitespace
            if (c == '\n')
                *lineno += 1;
            c = data[++i];
            continue;

        }else if (c == ';'){
            // Skip comments
            for (++i; i < size && data[i] != '\n'; ++i);
            if (i < size)
                c = data[i];
            continue;

        }else if (c == '('){
            // Parse subexpression
            sub = lispNodeAddChild(root);
            sub->lineno = *lineno;
            if (parseExp(sub, lineno, data, i + 1, size, &i) != 0){
                fprintf(stderr, "Error PDDL: Error on line %d\n", *lineno);
                return -1;
            }

            c = data[i];
            continue;

        }else if (c == ')'){
            // Finalize expression
            if (cont != NULL)
                *cont = i + 1;
            return 0;

        }else{
            sub = lispNodeAddChild(root);
            sub->value = data + i;
            sub->lineno = *lineno;
            for (; i < size && IS_ALPHA(data[i]); ++i){
                if (data[i] >= 'A' && data[i] <= 'Z')
                    data[i] = data[i] - 'A' + 'a';
            }
            if (i == size){
                fprintf(stderr, "Error PDDL: Error on line %d\n", *lineno);
                return -1;
            }

            c = data[i];
            data[i] = 0x0;
            sub->kw = recongnizeKeyword(sub->value);
        }
    }

    return 0;
}

plan_pddl_lisp_t *planPDDLLispParse(const char *fn)
{
    int fd, i, lineno;
    struct stat st;
    char *data;
    plan_pddl_lisp_t *lisp;
    plan_pddl_lisp_node_t root;

    fd = open(fn, O_RDONLY);
    if (fd == -1){
        fprintf(stderr, "Error: Could not open `%s'.\n", fn);
        fflush(stderr);
        return NULL;
    }

    if (fstat(fd, &st) != 0){
        fprintf(stderr, "Error: Could fstat `%s'.\n", fn);
        fflush(stderr);
        close(fd);
        return NULL;
    }

    data = mmap(NULL, st.st_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    if (data == MAP_FAILED){
        fprintf(stderr, "Error: Could not mmap `%s' to memory.\n", fn);
        fflush(stderr);
        close(fd);
        return NULL;
    }

    lineno = 1;
    for (i = 0; i < (int)st.st_size && data[i] != '('; ++i){
        if (data[i] == '\n')
            ++lineno;
    }
    lispNodeInit(&root);
    root.lineno = lineno;
    if (parseExp(&root, &lineno, data, i + 1, st.st_size, NULL) != 0){
        fprintf(stderr, "Error: Could not parse file `%s'.\n", fn);
        fflush(stderr);
        munmap((void *)data, st.st_size);
        lispNodeFree(&root);
        close(fd);
        return NULL;
    }

    lisp = BOR_ALLOC(plan_pddl_lisp_t);
    lisp->fd = fd;
    lisp->data = data;
    lisp->size = st.st_size;
    lisp->root = root;

    return lisp;
}

void planPDDLLispDel(plan_pddl_lisp_t *lisp)
{
    if (lisp->data)
        munmap((void *)lisp->data, lisp->size);
    if (lisp->fd >= 0)
        close(lisp->fd);
    lispNodeFree(&lisp->root);
    BOR_FREE(lisp);
}

static void nodeDump(const plan_pddl_lisp_node_t *node, FILE *fout, int prefix)
{
    int i;

    for (i = 0; i < prefix; ++i)
        fprintf(fout, " ");

    if (node->value != NULL){
        fprintf(fout, "%s [%d] :: %d\n", node->value, node->kw,
                node->lineno);
    }else{
        fprintf(fout, "( :: %d\n", node->lineno);

        for (i = 0; i < node->child_size; ++i){
            nodeDump(node->child + i, fout, prefix + 4);
        }

        for (i = 0; i < prefix; ++i)
            fprintf(fout, " ");
        fprintf(fout, ")\n");
    }
}

void planPDDLLispDump(const plan_pddl_lisp_t *lisp, FILE *fout)
{
    nodeDump(&lisp->root, fout, 0);
}

void planPDDLLispNodeCopy(plan_pddl_lisp_node_t *dst,
                          const plan_pddl_lisp_node_t *src)
{
    int i;

    lispNodeInit(dst);
    dst->value = src->value;
    dst->kw = src->kw;
    dst->lineno = src->lineno;

    dst->child_size = src->child_size;
    dst->child = BOR_ALLOC_ARR(plan_pddl_lisp_node_t, dst->child_size);
    for (i = 0; i < dst->child_size; ++i){
        lispNodeInit(dst->child + i);
        planPDDLLispNodeCopy(dst->child + i, src->child + i);
    }
}

void planPDDLLispNodeFree(plan_pddl_lisp_node_t *node)
{
    lispNodeFree(node);
}

const plan_pddl_lisp_node_t *planPDDLLispFindNode(
            const plan_pddl_lisp_node_t *root, int kw)
{
    int i;

    for (i = 0; i < root->child_size; ++i){
        if (planPDDLLispNodeHeadKw(root->child + i) == kw)
            return root->child + i;
    }
    return NULL;
}

int planPDDLLispParseTypedList(const plan_pddl_lisp_node_t *root,
                               int from, int to,
                               plan_pddl_lisp_parse_typed_list_fn cb,
                               void *ud)
{
    plan_pddl_lisp_node_t *n;
    int i, itfrom, itto, ittype;

    ittype = -1;
    itfrom = from;
    itto = from;
    for (i = from; i < to; ++i){
        n = root->child + i;

        if (n->value == NULL){
            ERRN2(n, "Unexpected token.");
            return -1;
        }

        if (strcmp(n->value, "-") == 0){
            itto = i;
            ittype = ++i;
            if (ittype >= to){
                ERRN2(root, "Invalid typed list.");
                return -1;
            }
            if (cb(root, itfrom, itto, ittype, ud) != 0)
                return -1;
            itfrom = i + 1;
            itto = i + 1;
            ittype = -1;

        }else{
            ++itto;
        }
    }

    if (itfrom < itto){
        if (cb(root, itfrom, itto, -1, ud) != 0)
            return -1;
    }

    return 0;
}
