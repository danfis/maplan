/***
 * maplan
 * -------
 * Copyright (c)2015 Daniel Fiser <danfis@danfis.cz>,
 * Agent Technology Center, Department of Computer Science,
 * Faculty of Electrical Engineering, Czech Technical University in Prague.
 * All rights reserved.
 *
 * This file is part of maplan.
 *
 * Distributed under the OSI-approved BSD License (the "License");
 * see accompanying file BDS-LICENSE for details or see
 * <http://www.opensource.org/licenses/bsd-license.php>.
 *
 * This software is distributed WITHOUT ANY WARRANTY; without even the
 * implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the License for more information.
 */

#include <boruvka/alloc.h>
#include "plan/pddl_type.h"
#include "pddl_err.h"

static const char *object_name = "object";

int planPDDLTypesGet(const plan_pddl_types_t *t, const char *name)
{
    int i;

    for (i = 0; i < t->size; ++i){
        if (strcmp(t->type[i].name, name) == 0)
            return i;
    }

    return -1;
}


static int add(plan_pddl_types_t *t, const char *name)
{
    int id;

    if ((id = planPDDLTypesGet(t, name)) != -1)
        return id;

    ++t->size;
    t->type = BOR_REALLOC_ARR(t->type, plan_pddl_type_t, t->size);
    t->type[t->size - 1].name = name;
    t->type[t->size - 1].parent = 0;
    return t->size - 1;
}

static int setCB(const plan_pddl_lisp_node_t *root,
                 int child_from, int child_to, int child_type, void *ud)
{
    plan_pddl_types_t *t = ud;
    int i, tid, pid;

    pid = 0;
    if (child_type >= 0){
        if (root->child[child_type].value == NULL){
            ERRN2(root->child + child_type, "Invalid type definition.");
            return -1;
        }
        pid = add(t, root->child[child_type].value);
    }

    for (i = child_from; i < child_to; ++i){
        if (root->child[i].value == NULL){
            ERRN2(root->child + i, "Invalid type definition.");
            return -1;
        }

        tid = add(t, root->child[i].value);
        t->type[tid].parent = pid;
    }

    return 0;
}

int planPDDLTypesParse(const plan_pddl_lisp_t *domain,
                       plan_pddl_types_t *types)
{
    const plan_pddl_lisp_node_t *n;

    // Create a default "object" type
    types->size = 1;
    types->type = BOR_ALLOC(plan_pddl_type_t);
    types->type[0].name = object_name;
    types->type[0].parent = -1;

    n = planPDDLLispFindNode(&domain->root, PLAN_PDDL_KW_TYPES);
    if (n == NULL)
        return 0;

    if (planPDDLLispParseTypedList(n, 1, n->child_size, setCB, types) != 0){
        ERRN2(n, "Invalid definition of :types");
        return -1;
    }

    // TODO: Check circular dependency on types
    return 0;
}

void planPDDLTypesFree(plan_pddl_types_t *types)
{
    if (types->type != NULL)
        BOR_FREE(types->type);
}

void planPDDLTypesPrint(const plan_pddl_types_t *t, FILE *fout)
{
    int i;

    fprintf(fout, "Type[%d]:\n", t->size);
    for (i = 0; i < t->size; ++i){
        fprintf(fout, "    [%d]: %s, parent: %d\n", i,
                t->type[i].name, t->type[i].parent);
    }
}
