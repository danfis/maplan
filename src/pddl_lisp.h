#ifndef __PLAN_PDDL_LISP_H__
#define __PLAN_PDDL_LISP_H__

#include <stdio.h>
#include <boruvka/list.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct _plan_lisp_node_t {
    const char *value;
    bor_list_t children;
    bor_list_t list;
};
typedef struct _plan_lisp_node_t plan_lisp_node_t;

struct _plan_pddl_lisp_t {
    plan_lisp_node_t *root;
    int fd;
    char *data;
    size_t size;
};
typedef struct _plan_pddl_lisp_t plan_pddl_lisp_t;

plan_pddl_lisp_t *planPDDLLispParse(const char *fn);
void planPDDLLispDel(plan_pddl_lisp_t *lisp);
void planPDDLLispDump(const plan_pddl_lisp_t *lisp, FILE *fout);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __PLAN_PDDL_LISP_H__ */
