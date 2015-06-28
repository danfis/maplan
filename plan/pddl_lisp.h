#ifndef __PLAN_PDDL_LISP_H__
#define __PLAN_PDDL_LISP_H__

#include <stdio.h>
#include <boruvka/compiler.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum {
    PLAN_PDDL_KW_DEFINE = 1,
    PLAN_PDDL_KW_DOMAIN,
    PLAN_PDDL_KW_REQUIREMENTS,
    PLAN_PDDL_KW_TYPES,
    PLAN_PDDL_KW_CONSTANTS,
    PLAN_PDDL_KW_PREDICATES,
    PLAN_PDDL_KW_ACTION,
    PLAN_PDDL_KW_PARAMETERS,
    PLAN_PDDL_KW_PRE,
    PLAN_PDDL_KW_EFF,

    PLAN_PDDL_KW_STRIPS,
    PLAN_PDDL_KW_TYPING,
    PLAN_PDDL_KW_NEGATIVE_PRE,
    PLAN_PDDL_KW_DISJUNCTIVE_PRE,
    PLAN_PDDL_KW_EQUALITY,
    PLAN_PDDL_KW_EXISTENTIAL_PRE,
    PLAN_PDDL_KW_UNIVERSAL_PRE,
    PLAN_PDDL_KW_CONDITIONAL_EFF,
    PLAN_PDDL_KW_NUMERIC_FLUENT,
    PLAN_PDDL_KW_OBJECT_FLUENT,
    PLAN_PDDL_KW_DURATIVE_ACTION,
    PLAN_PDDL_KW_DURATION_INEQUALITY,
    PLAN_PDDL_KW_CONTINUOUS_EFF,
    PLAN_PDDL_KW_DERIVED_PRED,
    PLAN_PDDL_KW_TIMED_INITIAL_LITERAL,
    PLAN_PDDL_KW_PREFERENCE,
    PLAN_PDDL_KW_CONSTRAINT,
    PLAN_PDDL_KW_ACTION_COST,
    PLAN_PDDL_KW_MULTI_AGENT,
    PLAN_PDDL_KW_UNFACTORED_PRIVACY,
    PLAN_PDDL_KW_FACTORED_PRIVACY,
    PLAN_PDDL_KW_QUANTIFIED_PRE,
    PLAN_PDDL_KW_FLUENTS,
    PLAN_PDDL_KW_ADL,

    PLAN_PDDL_KW_FUNCTIONS,
    PLAN_PDDL_KW_NUMBER,
    PLAN_PDDL_KW_PROBLEM,
    PLAN_PDDL_KW_OBJECTS,
    PLAN_PDDL_KW_INIT,
    PLAN_PDDL_KW_GOAL,
    PLAN_PDDL_KW_METRIC,
    PLAN_PDDL_KW_MINIMIZE,
    PLAN_PDDL_KW_MAXIMIZE,
    PLAN_PDDL_KW_INCREASE,

    PLAN_PDDL_KW_AND,
    PLAN_PDDL_KW_OR,
    PLAN_PDDL_KW_NOT,
    PLAN_PDDL_KW_IMPLY,
    PLAN_PDDL_KW_EXISTS,
    PLAN_PDDL_KW_FORALL,
    PLAN_PDDL_KW_WHEN,
} plan_pddl_kw_t;

typedef struct _plan_pddl_lisp_node_t plan_pddl_lisp_node_t;
struct _plan_pddl_lisp_node_t {
    const char *value;
    int kw;
    int lineno;
    plan_pddl_lisp_node_t *child;
    int child_size;
};

struct _plan_pddl_lisp_t {
    plan_pddl_lisp_node_t root;
    int fd;
    char *data;
    size_t size;
};
typedef struct _plan_pddl_lisp_t plan_pddl_lisp_t;

/**
 * Parses the input file and returns the parsed pddl-lisp object.
 */
plan_pddl_lisp_t *planPDDLLispParse(const char *fn);

/**
 * Deletes pddl-lisp object.
 */
void planPDDLLispDel(plan_pddl_lisp_t *lisp);

/**
 * Prints pddl-lisp object to the specified output.
 */
void planPDDLLispDump(const plan_pddl_lisp_t *lisp, FILE *fout);

/**
 * Returns root's child with the specified head keyword.
 */
const plan_pddl_lisp_node_t *planPDDLLispFindNode(
            const plan_pddl_lisp_node_t *root, int kw);

/**
 * Copy pddl-lisp-node from src to dst.
 */
void planPDDLLispNodeCopy(plan_pddl_lisp_node_t *dst,
                          const plan_pddl_lisp_node_t *src);

/**
 * Frees pddl-lisp-node -- use it as pair function to *Copy().
 */
void planPDDLLispNodeFree(plan_pddl_lisp_node_t *node);

/**
 * Returns the value of the first sub-element of the specified node.
 */
_bor_inline const char *planPDDLLispNodeHead(const plan_pddl_lisp_node_t *n);

/**
 * Simliar to planPDDLLispNodeHead() but returns the keyword corresponding
 * to the head value.
 */
_bor_inline int planPDDLLispNodeHeadKw(const plan_pddl_lisp_node_t *n);

/**** INLINES: ****/
_bor_inline const char *planPDDLLispNodeHead(const plan_pddl_lisp_node_t *n)
{
    if (n->child_size == 0)
        return NULL;
    return n->child[0].value;
}

_bor_inline int planPDDLLispNodeHeadKw(const plan_pddl_lisp_node_t *n)
{
    if (n->child_size == 0)
        return -1;
    return n->child[0].kw;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __PLAN_PDDL_LISP_H__ */
