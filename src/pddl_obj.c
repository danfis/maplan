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
#include "plan/pddl_obj.h"
#include "plan/pddl_require.h"
#include "pddl_err.h"

struct _set_t {
    plan_pddl_objs_t *objs;
    const plan_pddl_types_t *types;
    int is_const;
};
typedef struct _set_t set_t;

static int setCB(const plan_pddl_lisp_node_t *root,
                 int child_from, int child_to, int child_type, void *ud)
{
    plan_pddl_objs_t *objs = ((set_t *)ud)->objs;
    plan_pddl_obj_t *o;
    const plan_pddl_types_t *types = ((set_t *)ud)->types;
    int is_const = ((set_t *)ud)->is_const;
    int i, tid;

    tid = 0;
    if (child_type >= 0){
        tid = planPDDLTypesGet(types, root->child[child_type].value);
        if (tid < 0){
            ERRN(root->child + child_type, "Invalid type `%s'",
                 root->child[child_type].value);
            return -1;
        }
    }


    for (i = child_from; i < child_to; ++i){
        o = planPDDLObjsAdd(objs);
        o->name = root->child[i].value;
        o->type = tid;
        o->is_constant = is_const;
        o->is_private = 0;
        o->owner = -1;
        o->is_agent = 0;
    }

    return 0;
}

static int parse(const plan_pddl_lisp_t *lisp, int kw, int is_const,
                 const plan_pddl_types_t *types,
                 plan_pddl_objs_t *objs)
{
    const plan_pddl_lisp_node_t *n;
    int to;
    set_t set;

    n = planPDDLLispFindNode(&lisp->root, kw);
    if (n == NULL)
        return 0;

    // Find out if there are :private objects -- according to BNF the
    // :private definitions must be at the end of the :constants or
    // :objects expression, so we just need to determine the first :private
    // child.
    for (to = 1; to < n->child_size; ++to){
        if (n->child[to].child_size > 0
                && n->child[to].child[0].kw == PLAN_PDDL_KW_PRIVATE)
            break;
    }

    set.objs = objs;
    set.types = types;
    set.is_const = is_const;
    if (planPDDLLispParseTypedList(n, 1, to, setCB, &set) != 0){
        if (is_const){
            ERRN2(n, "Invalid definition of :constants.");
        }else{
            ERRN2(n, "Invalid definition of :objects.");
        }
        return -1;
    }

    return 0;
}

static int parsePrivate(const plan_pddl_lisp_t *lisp,
                        const plan_pddl_types_t *types,
                        plan_pddl_objs_t *objs)
{
    const plan_pddl_lisp_node_t *n, *p;
    int i, pi, owner;
    set_t set;

    n = planPDDLLispFindNode(&lisp->root, PLAN_PDDL_KW_OBJECTS);
    if (n == NULL)
        return 0;

    set.objs = objs;
    set.types = types;
    set.is_const = 0;
    for (i = 1; i < n->child_size; ++i){
        p = n->child + i;
        if (p->child_size == 0 || p->child[0].kw != PLAN_PDDL_KW_PRIVATE)
            continue;

        pi = objs->size;
        if (planPDDLLispParseTypedList(p, 2, p->child_size, setCB, &set) != 0){
            ERRN2(n->child + i, "Invalid definition of :private :objects.");
            return -1;
        }

        owner = planPDDLObjsGet(objs, p->child[1].value);
        if (owner < 0){
            ERRN(n->child + i, "Invalid definition of private objects."
                               " Unkown owner `%s'.\n", p->child[1].value);
            return -1;
        }

        for (; pi < objs->size; ++pi){
            objs->obj[pi].is_private = 1;
            objs->obj[pi].owner = owner;
        }
    }

    return 0;
}

int planPDDLObjsParse(const plan_pddl_lisp_t *domain,
                      const plan_pddl_lisp_t *problem,
                      const plan_pddl_types_t *types,
                      unsigned require,
                      plan_pddl_objs_t *objs)
{
    bzero(objs, sizeof(*objs));
    if (parse(domain, PLAN_PDDL_KW_CONSTANTS, 1, types, objs) != 0
            || parse(problem, PLAN_PDDL_KW_OBJECTS, 0, types, objs) != 0)
        return -1;

    if ((require & PLAN_PDDL_REQUIRE_MULTI_AGENT)
            && (require & PLAN_PDDL_REQUIRE_UNFACTORED_PRIVACY)){
        if (parsePrivate(problem, types, objs) != 0)
            return -1;
    }
    return 0;
}

void planPDDLObjsFree(plan_pddl_objs_t *objs)
{
    if (objs->obj != NULL)
        BOR_FREE(objs->obj);
}

int planPDDLObjsGet(const plan_pddl_objs_t *objs, const char *name)
{
    int i;

    for (i = 0; i < objs->size; ++i){
        if (strcmp(objs->obj[i].name, name) == 0)
            return i;
    }
    return -1;
}

plan_pddl_obj_t *planPDDLObjsAdd(plan_pddl_objs_t *objs)
{
    plan_pddl_obj_t *o;

    if (objs->size >= objs->alloc_size){
        if (objs->alloc_size == 0){
            objs->alloc_size = 2;
        }else{
            objs->alloc_size *= 2;
        }
        objs->obj = BOR_REALLOC_ARR(objs->obj, plan_pddl_obj_t,
                                    objs->alloc_size);
    }

    o = objs->obj + objs->size++;
    bzero(o, sizeof(*o));
    o->owner = -1;
    return o;
}

void planPDDLObjsPrint(const plan_pddl_objs_t *objs, FILE *fout)
{
    int i;

    fprintf(fout, "Obj[%d]:\n", objs->size);
    for (i = 0; i < objs->size; ++i){
        fprintf(fout, "    [%d]: %s, type: %d, is-constant: %d,"
                      " is-private: %d, owner: %d, is-agent: %d\n", i,
                objs->obj[i].name, objs->obj[i].type,
                objs->obj[i].is_constant, objs->obj[i].is_private,
                objs->obj[i].owner, objs->obj[i].is_agent);
    }
}



static void typeObjMapRec(int *m, int type_id,
                          const plan_pddl_types_t *types,
                          const plan_pddl_objs_t *objs)
{
    int i;

    for (i = 0; i < objs->size; ++i){
        if (objs->obj[i].type == type_id)
            m[i] = 1;
    }

    for (i = 0; i < types->size; ++i){
        if (types->type[i].parent == type_id)
            typeObjMapRec(m, i, types, objs);
    }
}

static void typeObjMap(plan_pddl_type_obj_t *to, int type,
                       const plan_pddl_types_t *types,
                       const plan_pddl_objs_t *objs)
{
    int i, ins, *m;

    m = to->map[type] = BOR_CALLOC_ARR(int, objs->size);
    typeObjMapRec(m, type, types, objs);

    for (ins = 0, i = 0; i < objs->size; ++i){
        if (m[i])
            m[ins++] = i;
    }
    to->map_size[type] = ins;
    if (ins != objs->size)
        to->map[type] = BOR_REALLOC_ARR(to->map[type], int, ins);
}

int planPDDLTypeObjInit(plan_pddl_type_obj_t *to,
                        const plan_pddl_types_t *types,
                        const plan_pddl_objs_t *objs)
{
    int i;

    to->size = types->size;
    to->map = BOR_CALLOC_ARR(int *, types->size);
    to->map_size = BOR_CALLOC_ARR(int, types->size);
    for (i = 0; i < to->size; ++i)
        typeObjMap(to, i, types, objs);
    return 0;
}

void planPDDLTypeObjFree(plan_pddl_type_obj_t *to)
{
    int i;

    for (i = 0; i < to->size; ++i){
        if (to->map[i] != NULL)
            BOR_FREE(to->map[i]);
    }
    if (to->map_size != NULL)
        BOR_FREE(to->map_size);
    if (to->map != NULL)
        BOR_FREE(to->map);
}

const int *planPDDLTypeObjGet(const plan_pddl_type_obj_t *to, int type_id,
                              int *size)
{
    if (size != NULL)
        *size = to->map_size[type_id];
    return to->map[type_id];
}

void planPDDLTypeObjPrint(const plan_pddl_type_obj_t *to, FILE *fout)
{
    int i, j;

    fprintf(fout, "Type-Obj:\n");
    for (i = 0; i < to->size; ++i){
        fprintf(fout, "    [%d]:", i);
        for (j = 0; j < to->map_size[i]; ++j)
            fprintf(fout, " %d", to->map[i][j]);
        fprintf(fout, "\n");
    }
}
