#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

#include <boruvka/alloc.h>

#include "pddl_lisp.h"

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

static plan_pddl_lisp_node_t *lispNodeNew(void)
{
    plan_pddl_lisp_node_t *n;

    n = BOR_ALLOC(plan_pddl_lisp_node_t);
    n->value = NULL;
    n->kw = -1;
    borListInit(&n->children);
    borListInit(&n->list);
    return n;
}

static void lispNodeDel(plan_pddl_lisp_node_t *n)
{
    bor_list_t *item, *tmp;
    plan_pddl_lisp_node_t *m;

    BOR_LIST_FOR_EACH_SAFE(&n->children, item, tmp){
        m = BOR_LIST_ENTRY(item, plan_pddl_lisp_node_t, list);
        lispNodeDel(m);
    }

    BOR_FREE(n);
}

static plan_pddl_lisp_node_t *parseExp(char *data, int from, int size, int *cont)
{
    plan_pddl_lisp_node_t *exp, *sub;
    int i = from;
    char c;

    if (i >= size)
        return NULL;

    exp = lispNodeNew();
    c = data[i];
    while (i < size){
        if (IS_WS(c)){
            // Skip whitespace
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
            sub = parseExp(data, i + 1, size, &i);
            if (sub == NULL){
                lispNodeDel(exp);
                return NULL;
            }

            borListAppend(&exp->children, &sub->list);
            c = data[i];
            continue;

        }else if (c == ')'){
            // Finalize expression
            if (cont != NULL)
                *cont = i + 1;
            return exp;

        }else{
            sub = lispNodeNew();
            borListAppend(&exp->children, &sub->list);
            sub->value = data + i;
            for (; i < size && IS_ALPHA(data[i]); ++i);
            if (i == size){
                lispNodeDel(exp);
                return NULL;
            }

            c = data[i];
            data[i] = 0x0;
            sub->kw = recongnizeKeyword(sub->value);
        }
    }

    lispNodeDel(exp);
    return NULL;
}

plan_pddl_lisp_t *planPDDLLispParse(const char *fn)
{
    int fd, i;
    struct stat st;
    char *data;
    plan_pddl_lisp_t *lisp;
    plan_pddl_lisp_node_t *root = NULL;

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

    for (i = 0; i < (int)st.st_size && data[i] != '('; ++i);
    root = parseExp(data, i + 1, st.st_size, NULL);
    if (root == NULL){
        fprintf(stderr, "Error: Could not parse file `%s'.\n", fn);
        fflush(stderr);
        munmap((void *)data, st.st_size);
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
    if (lisp->root)
        lispNodeDel(lisp->root);
    BOR_FREE(lisp);
}

static void nodeDump(const plan_pddl_lisp_node_t *node, FILE *fout, int prefix)
{
    bor_list_t *item;
    const plan_pddl_lisp_node_t *m;
    int i;

    if (node->value != NULL){
        for (i = 0; i < prefix; ++i)
            fprintf(fout, " ");
        fprintf(fout, "%s [%d]\n", node->value, node->kw);
    }else{
        for (i = 0; i < prefix; ++i)
            fprintf(fout, " ");
        fprintf(fout, "(\n");

        BOR_LIST_FOR_EACH(&node->children, item){
            m = BOR_LIST_ENTRY(item, plan_pddl_lisp_node_t, list);
            nodeDump(m, fout, prefix + 4);
        }

        for (i = 0; i < prefix; ++i)
            fprintf(fout, " ");
        fprintf(fout, ")\n");
    }
}

void planPDDLLispDump(const plan_pddl_lisp_t *lisp, FILE *fout)
{
    nodeDump(lisp->root, fout, 0);
}
