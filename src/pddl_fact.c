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
#include "plan/pddl_fact.h"
#include "pddl_err.h"

static int parseObjsIntoArr(const plan_pddl_lisp_node_t *n,
                            const plan_pddl_objs_t *objs,
                            int from, int to, int **out, int *out_size)
{
    const plan_pddl_lisp_node_t *c;
    int size, *obj, i;

    *out_size = size = to - from;
    *out = obj = BOR_CALLOC_ARR(int, size);
    for (i = 0; i < size; ++i){
        c = n->child + i + from;
        if (c->value == NULL){
            ERRN2(c, "Expecting object, got something else.");
            return -1;
        }

        obj[i] = planPDDLObjsGet(objs, c->value);
        if (obj[i] < 0){
            ERRN(c, "Unknown object `%s'.", c->value);
            return -1;
        }
    }

    return 0;
}

static int parseFunc(const plan_pddl_lisp_node_t *n,
                     const plan_pddl_predicates_t *functions,
                     const plan_pddl_objs_t *objs,
                     plan_pddl_facts_t *fs)
{
    const plan_pddl_lisp_node_t *nfunc, *nval;
    plan_pddl_fact_t *func;

    nfunc = n->child + 1;
    nval = n->child + 2;

    if (nfunc->child_size < 1
            || nfunc->child[0].value == NULL
            || nval->value == NULL){
        ERRN2(n, "Invalid function assignement.");
        return -1;
    }

    func = fs->fact + fs->size++;
    func->func_val = atoi(nval->value);
    func->pred = planPDDLPredicatesGet(functions, nfunc->child[0].value);
    if (func->pred < 0){
        ERRN(nfunc, "Unknown function `%s'", nfunc->child[0].value);
        return -1;
    }

    return parseObjsIntoArr(nfunc, objs, 1, nfunc->child_size,
                            &func->arg, &func->arg_size);
}

static int parseFact(const plan_pddl_lisp_node_t *n,
                     const plan_pddl_predicates_t *predicates,
                     const plan_pddl_objs_t *objs,
                     const char *head, plan_pddl_facts_t *fs)
{
    plan_pddl_fact_t *fact = fs->fact + fs->size++;

    fact->pred = planPDDLPredicatesGet(predicates, head);
    if (fact->pred < 0){
        ERRN(n, "Unkwnown predicate `%s'.", head);
        return -1;
    }

    return parseObjsIntoArr(n, objs, 1, n->child_size,
                            &fact->arg, &fact->arg_size);
}

static int parseFactFunc(const plan_pddl_lisp_node_t *n,
                         const plan_pddl_predicates_t *predicates,
                         const plan_pddl_predicates_t *functions,
                         const plan_pddl_objs_t *objs,
                         plan_pddl_facts_t *init_fact,
                         plan_pddl_facts_t *init_func)
{
    const char *head;

    if (n->child_size < 1){
        ERRN2(n, "Invalid fact in :init.");
        return -1;
    }

    head = planPDDLLispNodeHead(n);
    if (strcmp(head, "=") == 0
            && n->child_size == 3
            && n->child[1].value == NULL){
        return parseFunc(n, functions, objs, init_func);
    }else{
        return parseFact(n, predicates, objs, head, init_fact);
    }
}

int planPDDLFactsParseInit(const plan_pddl_lisp_t *problem,
                           const plan_pddl_predicates_t *predicates,
                           const plan_pddl_predicates_t *functions,
                           const plan_pddl_objs_t *objs,
                           plan_pddl_facts_t *init_fact,
                           plan_pddl_facts_t *init_func)
{
    const plan_pddl_lisp_node_t *ninit, *n;
    int i;

    ninit = planPDDLLispFindNode(&problem->root, PLAN_PDDL_KW_INIT);
    if (ninit == NULL){
        ERR2("Missing :init.");
        return -1;
    }

    // Pre-allocate facts and functions
    init_fact->fact = BOR_CALLOC_ARR(plan_pddl_fact_t, ninit->child_size - 1);
    init_func->fact = BOR_CALLOC_ARR(plan_pddl_fact_t, ninit->child_size - 1);

    for (i = 1; i < ninit->child_size; ++i){
        n = ninit->child + i;
        if (parseFactFunc(n, predicates, functions, objs,
                          init_fact, init_func) != 0)
            return -1;
    }

    if (init_fact->size < ninit->child_size - 1)
        init_fact->fact = BOR_REALLOC_ARR(init_fact->fact, plan_pddl_fact_t,
                                          init_fact->size);

    if (init_func->size < ninit->child_size - 1)
        init_func->fact = BOR_REALLOC_ARR(init_func->fact, plan_pddl_fact_t,
                                          init_func->size);
    return 0;
}

void planPDDLFactsFree(plan_pddl_facts_t *fs)
{
    int i;

    for (i = 0; i < fs->size; ++i){
        if (fs->fact[i].arg != NULL)
            BOR_FREE(fs->fact[i].arg);
    }
    if (fs->fact != NULL)
        BOR_FREE(fs->fact);
}

plan_pddl_fact_t *planPDDLFactsAdd(plan_pddl_facts_t *fs)
{
    plan_pddl_fact_t *f;

    ++fs->size;
    fs->fact = BOR_REALLOC_ARR(fs->fact, plan_pddl_fact_t, fs->size);
    f = fs->fact + fs->size - 1;
    bzero(f, sizeof(*f));
    return f;
}
