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

static plan_pddl_fact_t *addGoal(plan_pddl_facts_t *goal)
{
    plan_pddl_fact_t *f;

    ++goal->size;
    goal->fact = BOR_REALLOC_ARR(goal->fact, plan_pddl_fact_t, goal->size);
    f = goal->fact + goal->size - 1;
    bzero(f, sizeof(*f));
    return f;
}

static plan_pddl_fact_t *parseGoalFact(const plan_pddl_lisp_node_t *root,
                                       const plan_pddl_predicates_t *predicates,
                                       const plan_pddl_objs_t *objs,
                                       plan_pddl_facts_t *goal)
{
    plan_pddl_fact_t *f;
    const char *name;
    int pred, i;

    // Get predicate name
    name = planPDDLLispNodeHead(root);
    if (name == NULL){
        ERRN2(root, "Invalid definition of :goal, missing predicate name.");
        return NULL;
    }

    // And resolve it agains known predicates
    pred = planPDDLPredicatesGet(predicates, name);
    if (pred == -1){
        ERRN(root, "Unkown predicate `%s'", name);
        return NULL;
    }

    // Check that all children are terminals
    for (i = 1; i < root->child_size; ++i){
        if (root->child[i].value == NULL){
            ERRN(root, "Invalid instantiation of predicate `%s'", name);
            return NULL;
        }
    }

    // Add fact to preconditions
    f = addGoal(goal);
    f->pred = pred;
    f->arg_size = root->child_size - 1;
    f->arg = BOR_ALLOC_ARR(int, f->arg_size);
    for (i = 0; i < f->arg_size; ++i){
        f->arg[i] = planPDDLObjsGet(objs, root->child[i + 1].value);
        if (f->arg[i] < 0){
            ERRN(root->child + i + 1, "Invalid object in :goal predicate %s.",
                 name);
            return NULL;
        }
    }

    return f;
}

int parseGoal(const plan_pddl_lisp_node_t *root,
              const plan_pddl_predicates_t *predicates,
              const plan_pddl_objs_t *objs,
              plan_pddl_facts_t *goal)
{
    plan_pddl_fact_t *f;
    int i, kw;

    kw = planPDDLLispNodeHeadKw(root);
    if (kw == PLAN_PDDL_KW_AND){
        for (i = 1; i < root->child_size; ++i){
            if (parseGoal(root->child + i, predicates, objs, goal) != 0)
                return -1;
        }

    }else if (kw == PLAN_PDDL_KW_NOT){
        if (root->child_size != 2
                || root->child[1].kw != -1){
            ERRN2(root, "Only simple (not (predicate ...)) construct is"
                        " allowed :goal");
            return -1;
        }

        f = parseGoalFact(root->child + 1, predicates, objs, goal);
        if (f == NULL)
            return -1;
        f->neg = 1;

    }else if (kw == PLAN_PDDL_KW_FORALL
                || kw == PLAN_PDDL_KW_WHEN
                || kw == PLAN_PDDL_KW_OR
                || kw == PLAN_PDDL_KW_IMPLY
                || kw == PLAN_PDDL_KW_EXISTS
                || kw == PLAN_PDDL_KW_INCREASE){
        ERRN(root, "(%s ...) is not supported in :goal.",
             planPDDLLispNodeHead(root));
        return 0;

    }else if (kw == -1){
        if (parseGoalFact(root, predicates, objs, goal) == NULL)
            return -1;

    }else{
        if (root->child_size >= 1
                && root->child[0].value != NULL){
            ERRN(root, "Unexpected token while parsing :goal `%s'",
                 root->child[0].value);
        }else{
            ERRN2(root, "Unexpected token while parsing :goal.");
        }
        return -1;
    }
    return 0;
}

int planPDDLFactsParseGoal(const plan_pddl_lisp_t *problem,
                           const plan_pddl_predicates_t *predicates,
                           const plan_pddl_objs_t *objs,
                           plan_pddl_facts_t *goal)
{
    const plan_pddl_lisp_node_t *ngoal;

    ngoal = planPDDLLispFindNode(&problem->root, PLAN_PDDL_KW_GOAL);
    if (ngoal == NULL){
        ERR2("Missing :goal.");
        return -1;
    }

    if (ngoal->child_size != 2
            || ngoal->child[1].value != NULL){
        ERRN2(ngoal, "Invalid definition of :goal.");
        return -1;
    }

    return parseGoal(ngoal->child + 1, predicates, objs, goal);
}

void planPDDLFactFree(plan_pddl_fact_t *f)
{
    if (f->arg != NULL)
        BOR_FREE(f->arg);
}

void planPDDLFactsFree(plan_pddl_facts_t *fs)
{
    int i;

    for (i = 0; i < fs->size; ++i)
        planPDDLFactFree(fs->fact + i);
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

void planPDDLFactCopy(plan_pddl_fact_t *dst, const plan_pddl_fact_t *src)
{
    *dst = *src;
    dst->arg = BOR_ALLOC_ARR(int, dst->arg_size);
    memcpy(dst->arg, src->arg, sizeof(int) * dst->arg_size);
}

void planPDDLFactsCopy(plan_pddl_facts_t *dst, const plan_pddl_facts_t *src)
{
    int i;

    *dst = *src;
    if (src->fact != NULL)
        dst->fact = BOR_ALLOC_ARR(plan_pddl_fact_t, src->size);
    for (i = 0; i < dst->size; ++i)
        planPDDLFactCopy(dst->fact + i, src->fact + i);
}

void planPDDLFactPrint(const plan_pddl_predicates_t *predicates,
                       const plan_pddl_objs_t *objs,
                       const plan_pddl_fact_t *f,
                       FILE *fout)
{
    int i;

    if (f->neg)
        fprintf(fout, "N:");
    if (f->stat)
        fprintf(fout, "S:");
    fprintf(fout, "%s:", predicates->pred[f->pred].name);
    for (i = 0; i < f->arg_size; ++i){
        fprintf(fout, " %s", objs->obj[f->arg[i]].name);
    }
}

static void printFact(const plan_pddl_predicates_t *predicates,
                      const plan_pddl_objs_t *objs,
                      const plan_pddl_fact_t *f,
                      int func_val, FILE *fout)
{
    fprintf(fout, "    ");
    planPDDLFactPrint(predicates, objs, f, fout);
    if (func_val != 0)
        fprintf(fout, " --> %d", f->func_val);
    fprintf(fout, "\n");
}

static void planPDDLFactsPrint(const plan_pddl_predicates_t *predicates,
                               const plan_pddl_objs_t *objs,
                               const plan_pddl_facts_t *in,
                               const char *header,
                               int func_val,
                               FILE *fout)
{
    int i;

    fprintf(fout, "%s[%d]:\n", header, in->size);
    for (i = 0; i < in->size; ++i)
        printFact(predicates, objs, in->fact + i, func_val, fout);
}

void planPDDLFactsPrintInit(const plan_pddl_predicates_t *predicates,
                            const plan_pddl_objs_t *objs,
                            const plan_pddl_facts_t *in,
                            FILE *fout)
{
    planPDDLFactsPrint(predicates, objs, in, "Init", 0, fout);
}

void planPDDLFactsPrintInitFunc(const plan_pddl_predicates_t *predicates,
                                const plan_pddl_objs_t *objs,
                                const plan_pddl_facts_t *in,
                                FILE *fout)
{
    planPDDLFactsPrint(predicates, objs, in, "Init Func", 1, fout);
}

void planPDDLFactsPrintGoal(const plan_pddl_predicates_t *predicates,
                            const plan_pddl_objs_t *objs,
                            const plan_pddl_facts_t *in,
                            FILE *fout)
{
    planPDDLFactsPrint(predicates, objs, in, "Goal", 0, fout);
}
