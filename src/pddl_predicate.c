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
#include "plan/pddl_predicate.h"
#include "pddl_err.h"

struct _set_t {
    plan_pddl_predicate_t *pred;
    const plan_pddl_types_t *types;
};
typedef struct _set_t set_t;

static const char *eq_name = "=";

static int setCB(const plan_pddl_lisp_node_t *root,
                 int child_from, int child_to, int child_type, void *ud)
{
    plan_pddl_predicate_t *pred = ((set_t *)ud)->pred;
    const plan_pddl_types_t *types = ((set_t *)ud)->types;
    int i, j, tid;

    tid = 0;
    if (child_type >= 0){
        tid = planPDDLTypesGet(types, root->child[child_type].value);
        if (tid < 0){
            ERRN(root->child + child_type, "Invalid type `%s'",
                 root->child[child_type].value);
            return -1;
        }
    }

    j = pred->param_size;
    pred->param_size += child_to - child_from;
    pred->param = BOR_REALLOC_ARR(pred->param, int, pred->param_size);
    for (i = child_from; i < child_to; ++i, ++j)
        pred->param[j] = tid;
    return 0;
}

static int checkDuplicate(const plan_pddl_predicates_t *ps, const char *name)
{
    int i;

    for (i = 0; i < ps->size; ++i){
        if (strcmp(ps->pred[i].name, name) == 0)
            return 1;
    }
    return 0;
}

static int parsePredicate(const plan_pddl_types_t *types,
                          const plan_pddl_lisp_node_t *n,
                          const plan_pddl_predicates_t *ps,
                          const char *errname,
                          plan_pddl_predicate_t *p)
{
    set_t set;

    if (n->child_size < 1 || n->child[0].value == NULL){
        ERRN(n, "Invalid definition of %s.", errname);
        return -1;
    }

    if (checkDuplicate(ps, n->child[0].value)){
        ERRN(n, "Duplicate %s `%s'", errname, n->child[0].value);
        return -1;
    }

    set.pred = p;
    set.types = types;
    if (planPDDLLispParseTypedList(n, 1, n->child_size, setCB, &set) != 0)
        return -1;
    p->name = n->child[0].value;
    return 0;
}

static void addEqPredicate(plan_pddl_predicates_t *ps)
{
    int id;

    id = ps->size++;
    ps->pred[id].name = eq_name;
    ps->pred[id].param_size = 2;
    ps->pred[id].param = BOR_CALLOC_ARR(int, 2);
    ps->eq_pred = id;
}

int planPDDLPredicatesParse(const plan_pddl_lisp_t *domain,
                            unsigned require,
                            const plan_pddl_types_t *types,
                            plan_pddl_predicates_t *ps)
{
    const plan_pddl_lisp_node_t *n;
    int i;

    n = planPDDLLispFindNode(&domain->root, PLAN_PDDL_KW_PREDICATES);
    if (n == NULL)
        return 0;

    ps->pred = BOR_CALLOC_ARR(plan_pddl_predicate_t, n->child_size);
    if (require & PLAN_PDDL_REQUIRE_EQUALITY)
        addEqPredicate(ps);
    for (i = 1; i < n->child_size; ++i){
        if (parsePredicate(types, n->child + i, ps,
                           "predicate", ps->pred + ps->size) != 0)
            return -1;
        ++ps->size;
    }

    if (ps->size != n->child_size)
        ps->pred = BOR_REALLOC_ARR(ps->pred, plan_pddl_predicate_t, ps->size);

    return 0;
}

int planPDDLFunctionsParse(const plan_pddl_lisp_t *domain,
                           const plan_pddl_types_t *types,
                           plan_pddl_predicates_t *ps)
{
    const plan_pddl_lisp_node_t *n;
    int i;

    n = planPDDLLispFindNode(&domain->root, PLAN_PDDL_KW_FUNCTIONS);
    if (n == NULL)
        return 0;

    ps->pred = BOR_CALLOC_ARR(plan_pddl_predicate_t, n->child_size);
    for (i = 1; i < n->child_size; ++i){
        if (parsePredicate(types, n->child + i, ps,
                           "function", ps->pred + ps->size) != 0)
            return -1;
        ++ps->size;

        if (i + 2 < n->child_size
                && n->child[i + 1].value != NULL
                && strcmp(n->child[i + 1].value, "-") == 0){
            if (n->child[i + 2].value == NULL
                    || strcmp(n->child[i + 2].value, "number") != 0){
                ERRN2(n->child + i + 2, "Only number functions are supported.");
                return -1;
            }
            i += 2;
        }
    }

    if (ps->size != n->child_size)
        ps->pred = BOR_REALLOC_ARR(ps->pred, plan_pddl_predicate_t, ps->size);

    return 0;
}

void planPDDLPredicatesFree(plan_pddl_predicates_t *ps)
{
    int i;

    for (i = 0; i < ps->size; ++i){
        if (ps->pred[i].param != NULL)
            BOR_FREE(ps->pred[i].param);
    }
    if (ps->pred != NULL)
        BOR_FREE(ps->pred);
}

int planPDDLPredicatesGet(const plan_pddl_predicates_t *ps, const char *name)
{
    int i;

    for (i = 0; i < ps->size; ++i){
        if (strcmp(ps->pred[i].name, name) == 0)
            return i;
    }
    return -1;
}

void planPDDLPredicatesPrint(const plan_pddl_predicates_t *ps,
                             const char *title, FILE *fout)
{
    int i, j;

    fprintf(fout, "%s[%d]:\n", title, ps->size);
    for (i = 0; i < ps->size; ++i){
        fprintf(fout, "    %s:", ps->pred[i].name);
        for (j = 0; j < ps->pred[i].param_size; ++j){
            fprintf(fout, " %d", ps->pred[i].param[j]);
        }
        fprintf(fout, "\n");
    }
}
