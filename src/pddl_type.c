#include <boruvka/alloc.h>
#include "pddl_type.h"

void planPDDLTypesInit(plan_pddl_types_t *types)
{
    bzero(types, sizeof(*types));
}

void planPDDLTypesFree(plan_pddl_types_t *types)
{
    int i;

    for (i = 0; i < types->size; ++i){
        if (types->name[i])
            BOR_FREE(types->name[i]);
    }
    if (types->name)
        BOR_FREE(types->name);
    if (types->parent)
        BOR_FREE(types->parent);
}

int planPDDLTypesFind(const plan_pddl_types_t *types, const char *name)
{
    int i;

    for (i = 0; i < types->size; ++i){
        if (strcmp(name, types->name[i]) == 0)
            return i;
    }
    return -1;
}

int planPDDLTypesAdd(plan_pddl_types_t *types, const char *name)
{
    int id;

    if ((id = planPDDLTypesFind(types, name)) >= 0)
        return id;

    ++types->size;
    types->name = BOR_REALLOC_ARR(types->name, char *, types->size);
    types->parent = BOR_REALLOC_ARR(types->parent, int, types->size);
    types->name[types->size - 1] = BOR_STRDUP(name);
    types->parent[types->size - 1] = -1;

    return types->size - 1;
}

void planPDDLTypesAddParent(plan_pddl_types_t *types, int id, int parent_id)
{
    types->parent[id] = parent_id;
}
