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

static int factSetPrivate(plan_pddl_fact_t *fact,
                          const plan_pddl_predicates_t *pred,
                          const plan_pddl_objs_t *objs)
{
    const plan_pddl_predicate_t *pr;
    const plan_pddl_obj_t *obj;
    int i;

    pr = pred->pred + fact->pred;
    if (pr->is_private){
        fact->is_private = 1;
        fact->owner = fact->arg[pr->owner_param];
    }

    for (i = 0; i < fact->arg_size; ++i){
        obj = objs->obj + fact->arg[i];
        if (obj->is_private){
            if (fact->is_private){
                if (fact->owner != obj->owner){
                    fprintf(stderr, "Error PDDL: Invalid definition of fact ");
                    planPDDLFactPrint(pred, objs, fact, stderr);
                    fprintf(stderr, ".\n");
                    ERR2("The fact is defined so it should be private for"
                         " two different agents.");
                    return -1;
                }
            }else{
                fact->is_private = 1;
                fact->owner = obj->owner;
            }
        }
    }

    return 0;
}

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

    func = planPDDLFactsAdd(fs);
    func->func_val = atoi(nval->value);
    func->pred = planPDDLPredicatesGet(functions, nfunc->child[0].value);
    if (func->pred < 0){
        ERRN(nfunc, "Unknown function `%s'", nfunc->child[0].value);
        return -1;
    }

    if (parseObjsIntoArr(nfunc, objs, 1, nfunc->child_size,
                         &func->arg, &func->arg_size) != 0)
        return -1;

    if (factSetPrivate(func, functions, objs) != 0)
        return -1;

    return 0;
}

static int parseFact(const plan_pddl_lisp_node_t *n,
                     const plan_pddl_predicates_t *predicates,
                     const plan_pddl_objs_t *objs,
                     const char *head, plan_pddl_facts_t *fs)
{
    plan_pddl_fact_t *fact;

    fact = planPDDLFactsAdd(fs);
    fact->pred = planPDDLPredicatesGet(predicates, head);
    if (fact->pred < 0){
        ERRN(n, "Unkwnown predicate `%s'.", head);
        return -1;
    }

    if (parseObjsIntoArr(n, objs, 1, n->child_size,
                         &fact->arg, &fact->arg_size) != 0)
        return -1;

    if (factSetPrivate(fact, predicates, objs) != 0)
        return -1;

    return 0;
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

    for (i = 1; i < ninit->child_size; ++i){
        n = ninit->child + i;
        if (parseFactFunc(n, predicates, functions, objs,
                          init_fact, init_func) != 0)
            return -1;
    }

    return 0;
}

static plan_pddl_fact_t *parseGoalFact(const plan_pddl_lisp_node_t *root,
                                       const plan_pddl_predicates_t *predicates,
                                       const plan_pddl_objs_t *objs,
                                       plan_pddl_facts_t *goal)
{
    const char *name;

    // Get predicate name
    name = planPDDLLispNodeHead(root);
    if (name == NULL){
        ERRN2(root, "Invalid definition of :goal, missing predicate name.");
        return NULL;
    }

    if (parseFact(root, predicates, objs, name, goal) != 0)
        return NULL;
    return goal->fact + goal->size - 1;
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

    if (fs->size >= fs->alloc_size){
        if (fs->alloc_size == 0){
            fs->alloc_size = 2;
        }else{
            fs->alloc_size *= 2;
        }
        fs->fact = BOR_REALLOC_ARR(fs->fact, plan_pddl_fact_t, fs->alloc_size);
    }

    f = fs->fact + fs->size++;
    bzero(f, sizeof(*f));
    f->owner = -1;
    return f;
}

void planPDDLFactsSqueeze(plan_pddl_facts_t *fs)
{
    fs->alloc_size = fs->size;
    fs->fact = BOR_REALLOC_ARR(fs->fact, plan_pddl_fact_t, fs->alloc_size);
}

void planPDDLFactsReserve(plan_pddl_facts_t *fs, int alloc_size)
{
    if (fs->alloc_size >= alloc_size)
        return;
    fs->alloc_size = alloc_size;
    fs->fact = BOR_REALLOC_ARR(fs->fact, plan_pddl_fact_t, fs->alloc_size);
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
    if (f->is_private){
        fprintf(fout, "P");
        if (f->owner >= 0)
            fprintf(fout, "[%d]", f->owner);
        fprintf(fout, ":");
    }
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
