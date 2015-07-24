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
    const char *owner_var;
};
typedef struct _set_t set_t;

static const char *eq_name = "=";

static int setCB(const plan_pddl_lisp_node_t *root,
                 int child_from, int child_to, int child_type, void *ud)
{
    plan_pddl_predicate_t *pred = ((set_t *)ud)->pred;
    const plan_pddl_types_t *types = ((set_t *)ud)->types;
    const char *owner_var = ((set_t *)ud)->owner_var;
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
    for (i = child_from; i < child_to; ++i, ++j){
        pred->param[j] = tid;
        if (owner_var != NULL && strcmp(owner_var, root->child[i].value) == 0){
            pred->owner_param = j;
        }
    }
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
                          const char *owner_var,
                          const char *errname,
                          plan_pddl_predicates_t *ps)
{
    plan_pddl_predicate_t *p;
    set_t set;

    if (n->child_size < 1 || n->child[0].value == NULL){
        ERRN(n, "Invalid definition of %s.", errname);
        return -1;
    }

    if (checkDuplicate(ps, n->child[0].value)){
        ERRN(n, "Duplicate %s `%s'", errname, n->child[0].value);
        return -1;
    }

    p = planPDDLPredicatesAdd(ps);
    set.pred = p;
    set.types = types;
    set.owner_var = owner_var;
    if (planPDDLLispParseTypedList(n, 1, n->child_size, setCB, &set) != 0){
        planPDDLPredicatesRemoveLast(ps);
        return -1;
    }

    p->name = n->child[0].value;
    return 0;
}

static int parsePrivatePredicates(const plan_pddl_types_t *types,
                                  const plan_pddl_lisp_node_t *n,
                                  plan_pddl_predicates_t *ps)
{
    const char *owner_var;
    int i, from;

    if (n->child_size < 3
            || n->child[0].kw != PLAN_PDDL_KW_PRIVATE
            || n->child[1].value == NULL
            || n->child[1].value[0] != '?'
            || (n->child[2].value != NULL && n->child_size < 5)){
        ERRN2(n, "Invalid definition of :private predicate.");
        return -1;
    }

    owner_var = n->child[1].value;

    if (n->child[2].value == NULL){
        from = 2;
    }else{
        from = 4;
    }
    for (i = from; i < n->child_size; ++i){
        if (parsePredicate(types, n->child + i, owner_var,
                           "private predicate", ps) != 0)
            return -1;

        ps->pred[ps->size - 1].is_private = 1;
    }

    return 0;
}

static void addEqPredicate(plan_pddl_predicates_t *ps)
{
    plan_pddl_predicate_t *p;

    p = planPDDLPredicatesAdd(ps);
    p->name = eq_name;
    p->param_size = 2;
    p->param = BOR_CALLOC_ARR(int, 2);
    ps->eq_pred = ps->size - 1;
}

int planPDDLPredicatesParse(const plan_pddl_lisp_t *domain,
                            unsigned require,
                            const plan_pddl_types_t *types,
                            plan_pddl_predicates_t *ps)
{
    const plan_pddl_lisp_node_t *n;
    int i, to, private;

    n = planPDDLLispFindNode(&domain->root, PLAN_PDDL_KW_PREDICATES);
    if (n == NULL)
        return 0;

    ps->eq_pred = -1;
    if (require & PLAN_PDDL_REQUIRE_EQUALITY)
        addEqPredicate(ps);

    // Determine if we can expect :private definitions
    private = (require & PLAN_PDDL_REQUIRE_UNFACTORED_PRIVACY)
                || (require & PLAN_PDDL_REQUIRE_FACTORED_PRIVACY);

    if (private){
        // Find out first :private definition
        for (to = 1; to < n->child_size; ++to){
            if (n->child[to].child_size > 0
                    && n->child[to].child[0].kw == PLAN_PDDL_KW_PRIVATE)
                break;
        }
    }else{
        to = n->child_size;
    }

    // Parse non :private predicates
    for (i = 1; i < to; ++i){
        if (parsePredicate(types, n->child + i, NULL, "predicate", ps) != 0)
            return -1;
    }

    if (private){
        // Parse :private predicates
        for (i = to; i < n->child_size; ++i){
            if (parsePrivatePredicates(types, n->child + i, ps) != 0)
                return -1;
        }
    }

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

    for (i = 1; i < n->child_size; ++i){
        if (parsePredicate(types, n->child + i, NULL, "function", ps) != 0)
            return -1;

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

plan_pddl_predicate_t *planPDDLPredicatesAdd(plan_pddl_predicates_t *ps)
{
    plan_pddl_predicate_t *p;

    if (ps->size >= ps->alloc_size){
        if (ps->alloc_size == 0){
            ps->alloc_size = 2;
        }else{
            ps->alloc_size *= 2;
        }
        ps->pred = BOR_REALLOC_ARR(ps->pred, plan_pddl_predicate_t,
                                   ps->alloc_size);
    }

    p = ps->pred + ps->size++;
    bzero(p, sizeof(*p));
    p->owner_param = -1;
    return p;
}

void planPDDLPredicatesRemoveLast(plan_pddl_predicates_t *ps)
{
    plan_pddl_predicate_t *p;

    p = ps->pred + --ps->size;
    if (p->param != NULL)
        BOR_FREE(p->param);
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
        fprintf(fout, " :: is-private: %d, owner-param: %d",
                ps->pred[i].is_private, ps->pred[i].owner_param);
        fprintf(fout, "\n");
    }
}
