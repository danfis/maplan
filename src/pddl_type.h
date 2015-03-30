#ifndef __PLAN_PDDL_TYPE_H__
#define __PLAN_PDDL_TYPE_H__

#include <stdio.h>
#include <boruvka/list.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct _plan_pddl_types_t {
    int size;
    char **name;
    int *parent;
};
typedef struct _plan_pddl_types_t plan_pddl_types_t;

void planPDDLTypesInit(plan_pddl_types_t *types);
void planPDDLTypesFree(plan_pddl_types_t *types);
int planPDDLTypesFind(const plan_pddl_types_t *types, const char *name);
int planPDDLTypesAdd(plan_pddl_types_t *types, const char *name);
void planPDDLTypesAddParent(plan_pddl_types_t *types, int id, int parent_id);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __PLAN_PDDL_TYPE_H__ */
