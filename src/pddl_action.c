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
#include "plan/pddl_action.h"
#include "pddl_err.h"

struct _set_param_t {
    plan_pddl_action_t *a;
    const plan_pddl_types_t *types;
};
typedef struct _set_param_t set_param_t;

static int parsePreEff(const plan_pddl_objs_t *objs,
                       const plan_pddl_predicates_t *predicates,
                       const plan_pddl_predicates_t *functions,
                       const plan_pddl_lisp_node_t *root,
                       const char *errname,
                       plan_pddl_action_t *a,
                       plan_pddl_facts_t *facts,
                       int parse_cost, int parse_cond_eff);

static plan_pddl_action_t *addAction(plan_pddl_actions_t *as, const char *name)
{
    plan_pddl_action_t *a;

    ++as->size;
    as->action = BOR_REALLOC_ARR(as->action, plan_pddl_action_t, as->size);
    a = as->action + as->size - 1;
    bzero(a, sizeof(*a));
    a->name = name;
    return a;
}

static plan_pddl_cond_eff_t *addCondEff(plan_pddl_action_t *a)
{
    plan_pddl_cond_eff_t *ce;

    ++a->cond_eff.size;
    a->cond_eff.cond_eff = BOR_REALLOC_ARR(a->cond_eff.cond_eff,
                                           plan_pddl_cond_eff_t,
                                           a->cond_eff.size);
    ce = a->cond_eff.cond_eff + a->cond_eff.size - 1;
    bzero(ce, sizeof(*ce));
    return ce;
}

static int getParam(const plan_pddl_action_t *a, const char *var_name)
{
    int i;

    for (i = 0; i < a->param.size; ++i){
        if (strcmp(a->param.obj[i].name, var_name) == 0)
            return i;
    }
    return -1;
}

static int setParams(const plan_pddl_lisp_node_t *root,
                     int child_from, int child_to, int child_type, void *ud)
{
    plan_pddl_action_t *a = ((set_param_t *)ud)->a;
    const plan_pddl_types_t *types = ((set_param_t *)ud)->types;
    int i, j, tid;

    tid = 0;
    if (child_type >= 0){
        tid = planPDDLTypesGet(types, root->child[child_type].value);
        if (tid < 0){
            ERRN(root->child + child_type, "Unkown type `%s'",
                 root->child[child_type].value);
            return -1;
        }
    }

    j = a->param.size;
    a->param.size += child_to - child_from;
    a->param.obj = BOR_REALLOC_ARR(a->param.obj, plan_pddl_obj_t,
                                   a->param.size);
    for (i = child_from; i < child_to; ++i, ++j){
        if (root->child[i].value == NULL){
            ERRN2(root->child + i, "Invalid parameter definition:"
                                   " Unexpected expression.");
            return -1;
        }

        if (root->child[i].value[0] != '?'){
            ERRN(root->child + i, "Invalid parameter definition:"
                                  " Expected variable, got %s.",
                 root->child[i].value);
            return -1;
        }

        a->param.obj[j].name = root->child[i].value;
        a->param.obj[j].type = tid;
        a->param.obj[j].is_constant = 0;
    }

    return 0;
}

static int parseParams(const plan_pddl_types_t *types,
                       const plan_pddl_lisp_node_t *n,
                       plan_pddl_action_t *a)
{
    set_param_t set_param;
    set_param.a = a;
    set_param.types = types;
    if (planPDDLLispParseTypedList(n, 0, n->child_size,
                                   setParams, &set_param) != 0)
        return -1;
    return 0;
}

static int parseVarConst(const plan_pddl_lisp_node_t *root,
                         const plan_pddl_objs_t *objs,
                         const plan_pddl_action_t *a,
                         int *dst)
{
    if (root->value[0] == '?'){
        *dst = getParam(a, root->value);
        if (*dst < 0){
            ERRN(root, "Invalid paramenter name `%s'", root->value);
            return -1;
        }

    }else{
        *dst = planPDDLObjsGet(objs, root->value);
        if (*dst < 0){
            ERRN(root, "Unkown constant `%s'", root->value);
            return -1;
        }
        *dst -= objs->size;
    }

    return 0;
}

static plan_pddl_fact_t *parseFact(const plan_pddl_objs_t *objs,
                                   const plan_pddl_predicates_t *predicates,
                                   const plan_pddl_lisp_node_t *root,
                                   plan_pddl_action_t *a,
                                   plan_pddl_facts_t *fs)
{
    plan_pddl_fact_t *f;
    const char *name;
    int pred, i;

    // Get predicate name
    name = planPDDLLispNodeHead(root);
    if (name == NULL){
        ERRN2(root, "Invalid definition of conditional, missing head of"
                    " expression.");
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
    f = planPDDLFactsAdd(fs);
    f->pred = pred;
    f->arg_size = root->child_size - 1;
    f->arg = BOR_ALLOC_ARR(int, f->arg_size);
    for (i = 0; i < f->arg_size; ++i){
        if (parseVarConst(root->child + i + 1, objs, a, f->arg + i) != 0)
            return NULL;
    }

    return f;
}

static int parseCost(const plan_pddl_objs_t *objs,
                     const plan_pddl_predicates_t *functions,
                     const plan_pddl_lisp_node_t *root,
                     plan_pddl_action_t *a)
{
    plan_pddl_fact_t *f;

    /* TODO
    if (!(pddl->require & PLAN_PDDL_REQUIRE_ACTION_COST)){
        ERRN2(root, "(increase ...) is supported only under"
                " :action-costs requirement.");
        return -1;
    }
    */

    if (root->child_size != 3){
        ERRN2(root, "Invalid (increase ...) specification.");
        return -1;
    }

    if (root->child[1].value != NULL
            || root->child[1].child_size != 1
            || root->child[1].child[0].value == NULL
            || strcmp(root->child[1].child[0].value, "total-cost") != 0){
        ERRN2(root, "Invalid (increase ...) specification -- the first"
                    " argument must be (total-cost).");
        return -1;
    }

    if (root->child[2].value != NULL){
        f = planPDDLFactsAdd(&a->cost);
        f->pred = -1;
        f->func_val = atoi(root->child[2].value);
    }else{
        f = parseFact(objs, functions, root->child + 2, a, &a->cost);
        if (f == NULL)
            return -1;
    }

    return 0;
}

static int parseCondEff(const plan_pddl_objs_t *objs,
                        const plan_pddl_predicates_t *predicates,
                        const plan_pddl_lisp_node_t *root,
                        plan_pddl_action_t *a)
{
    plan_pddl_cond_eff_t *ce;

    if (root->child_size != 3
            || root->child[1].value != NULL
            || root->child[2].value != NULL){
        ERRN2(root, "Invalid (when ...) condition.");
        return -1;
    }

    ce = addCondEff(a);
    if (parsePreEff(objs, predicates, NULL, root->child + 1,
                    "cond-eff predicate", a, &ce->pre, 0, 0) != 0)
        return -1;
    if (parsePreEff(objs, predicates, NULL, root->child + 2,
                    "cond-eff effect", a, &ce->eff, 0, 0) != 0)
        return -1;
    return 0;
}

static int parsePreEff(const plan_pddl_objs_t *objs,
                       const plan_pddl_predicates_t *predicates,
                       const plan_pddl_predicates_t *functions,
                       const plan_pddl_lisp_node_t *root,
                       const char *errname,
                       plan_pddl_action_t *a,
                       plan_pddl_facts_t *facts,
                       int parse_cost, int parse_cond_eff)
{
    plan_pddl_fact_t *f;
    int i, kw;

    kw = planPDDLLispNodeHeadKw(root);
    if (kw == PLAN_PDDL_KW_AND){
        for (i = 1; i < root->child_size; ++i){
            if (parsePreEff(objs, predicates, functions, root->child + i,
                            errname, a, facts, parse_cost, parse_cond_eff) != 0)
                return -1;
        }

    }else if (kw == PLAN_PDDL_KW_NOT){
        if (root->child_size != 2
                || root->child[1].kw != -1){
            ERRN(root, "Only simple (not (predicate ...)) construct is"
                       " allowed in action %s.", errname);
            return -1;
        }

        if ((f = parseFact(objs, predicates, root->child + 1, a, facts)) == NULL)
            return -1;
        f->neg = 1;

    }else if (kw == PLAN_PDDL_KW_FORALL){
        ERRN(root, "(forall ...) is not supported in %s yet.", errname);
        return 0;

    }else if (kw == PLAN_PDDL_KW_WHEN){
        if (!parse_cond_eff){
            ERRN(root, "(when ...) is not supported in %s.", errname);
            return 0;
        }

        if (parseCondEff(objs, predicates, root, a) != 0)
            return -1;

    }else if (kw == PLAN_PDDL_KW_OR){
        ERRN(root, "(or ...) is not supported in %s yet.", errname);
        return -1;

    }else if (kw == PLAN_PDDL_KW_IMPLY){
        ERRN(root, "(imply ...) is not supported in %s yet.", errname);
        return -1;

    }else if (kw == PLAN_PDDL_KW_EXISTS){
        ERRN(root, "(exists ...) is not supported in %s yet.", errname);
        return -1;

    }else if (kw == PLAN_PDDL_KW_INCREASE){
        if (!parse_cost){
            ERRN(root, "(increase ...) is not supported in %s.", errname);
            return -1;
        }

        if (parseCost(objs, functions, root, a) != 0)
            return -1;

    }else if (kw == -1){
        if (parseFact(objs, predicates, root, a, facts) == NULL)
            return -1;

    }else{
        if (root->child_size >= 1
                && root->child[0].value != NULL){
            ERRN(root, "Unexpected token while parsing action"
                       " %s: `%s'", errname, root->child[0].value);
        }else{
            ERRN(root, "Unexpected token while parsing action %s.", errname);
        }
        return -1;
    }
    return 0;
}

static int parseAction(const plan_pddl_types_t *types,
                       const plan_pddl_objs_t *objs,
                       const plan_pddl_predicates_t *predicates,
                       const plan_pddl_predicates_t *functions,
                       const plan_pddl_lisp_node_t *root,
                       plan_pddl_actions_t *actions)
{
    const plan_pddl_lisp_node_t *n;
    plan_pddl_action_t *a;
    int i;

    if (root->child_size < 4
            || root->child_size / 2 == 1
            || root->child[1].value == NULL){
        ERRN2(root, "Invalid definition of :action");
        return -1;
    }

    a = addAction(actions, root->child[1].value);
    for (i = 2; i < root->child_size; i += 2){
        n = root->child + i + 1;
        if (root->child[i].kw == PLAN_PDDL_KW_PARAMETERS){
            if (parseParams(types, n, a) != 0)
                return -1;

        }else if (root->child[i].kw == PLAN_PDDL_KW_PRE){
            if (parsePreEff(objs, predicates, functions, n,
                            "precondition", a, &a->pre, 0, 0) != 0)
                return -1;

        }else if (root->child[i].kw == PLAN_PDDL_KW_EFF){
            if (parsePreEff(objs, predicates, functions, n,
                            "effect", a, &a->eff, 1, 1) != 0)
                return -1;

        }else{
            ERRN(root->child + i, "Invalid definition of :action."
                                  " Unexpected token: %s",
                                  root->child[i].value);
            return -1;
        }
    }

    return 0;
}

int planPDDLActionsParse(const plan_pddl_lisp_t *domain,
                         const plan_pddl_types_t *types,
                         const plan_pddl_objs_t *objs,
                         const plan_pddl_predicates_t *predicates,
                         const plan_pddl_predicates_t *functions,
                         plan_pddl_actions_t *actions)
{
    const plan_pddl_lisp_node_t *root = &domain->root;
    const plan_pddl_lisp_node_t *n;
    int i;

    for (i = 0; i < root->child_size; ++i){
        n = root->child + i;
        if (planPDDLLispNodeHeadKw(n) == PLAN_PDDL_KW_ACTION){
            if (parseAction(types, objs, predicates, functions, n, actions) != 0)
                return -1;
        }
    }
    return 0;
}

void planPDDLActionsFree(plan_pddl_actions_t *actions)
{
    int i, j;

    for (i = 0; i < actions->size; ++i){
        planPDDLObjsFree(&actions->action[i].param);
        planPDDLFactsFree(&actions->action[i].pre);
        planPDDLFactsFree(&actions->action[i].eff);
        planPDDLFactsFree(&actions->action[i].cost);
        for (j = 0; j < actions->action[i].cond_eff.size; ++j){
            planPDDLFactsFree(&actions->action[i].cond_eff.cond_eff[j].pre);
            planPDDLFactsFree(&actions->action[i].cond_eff.cond_eff[j].eff);
        }
        if (actions->action[i].cond_eff.cond_eff != NULL)
            BOR_FREE(actions->action[i].cond_eff.cond_eff);
    }
    if (actions->action != NULL)
        BOR_FREE(actions->action);
}

static void printFacts(const plan_pddl_action_t *a,
                       const plan_pddl_objs_t *objs,
                       const plan_pddl_predicates_t *predicates,
                       const plan_pddl_facts_t *fs,
                       FILE *fout)
{
    const plan_pddl_fact_t *f;
    int i, j, id;

    for (i = 0; i < fs->size; ++i){
        f = fs->fact + i;
        fprintf(fout, "            ");
        if (f->neg)
            fprintf(fout, "N:");
        fprintf(fout, "%s:", predicates->pred[f->pred].name);
        for (j = 0; j < f->arg_size; ++j){
            if (f->arg[j] < 0){
                id = f->arg[j] + objs->size;
                fprintf(fout, " %s", objs->obj[id].name);
            }else{
                fprintf(fout, " %s", a->param.obj[f->arg[j]].name);
            }
        }
        fprintf(fout, "\n");
    }
}

static void planPDDLActionPrint(const plan_pddl_action_t *a,
                                const plan_pddl_objs_t *objs,
                                const plan_pddl_predicates_t *predicates,
                                const plan_pddl_predicates_t *functions,
                                FILE *fout)
{
    int i, j, id;

    fprintf(fout, "    %s:", a->name);
    for (i = 0; i < a->param.size; ++i)
        fprintf(fout, " %s:%d", a->param.obj[i].name, a->param.obj[i].type);
    fprintf(fout, "\n");

    fprintf(fout, "        pre[%d]:\n", a->pre.size);
    printFacts(a, objs, predicates, &a->pre, fout);

    fprintf(fout, "        eff[%d]:\n", a->eff.size);
    printFacts(a, objs, predicates, &a->eff, fout);

    fprintf(fout, "        cond-eff[%d]:\n", a->cond_eff.size);
    for (i = 0; i < a->cond_eff.size; ++i){
        fprintf(fout, "          pre[%d]:\n",
                a->cond_eff.cond_eff[i].pre.size);
        printFacts(a, objs, predicates, &a->cond_eff.cond_eff[i].pre, fout);
        fprintf(fout, "          eff[%d]:\n",
                a->cond_eff.cond_eff[i].eff.size);
        printFacts(a, objs, predicates, &a->cond_eff.cond_eff[i].eff, fout);
        //fprintf(fout, "            eff[%d]:\n", a->cond_eff[i].eff_size);
        //factPrintArr(pddl, a, a->cond_eff[i].eff, a->cond_eff[i].eff_size,
        //             fout, "                ");
    }

    for (i = 0; i < a->cost.size; ++i){
        fprintf(fout, "        cost: ");
        if (a->cost.fact[i].pred == -1){
            fprintf(fout, "%d", a->cost.fact[i].func_val);
        }else{
            fprintf(fout, "%s", functions->pred[a->cost.fact[i].pred].name);
            for (j = 0; j < a->cost.fact[i].arg_size; ++j){
                if (a->cost.fact[i].arg[j] >= 0){
                    fprintf(fout, " %s", a->param.obj[a->cost.fact[i].arg[j]].name);
                }else{
                    id = a->cost.fact[i].arg[j] + objs->size;
                    fprintf(fout, " %s", objs->obj[id].name);
                }
            }
        }
        fprintf(fout, "\n");
    }
}

void planPDDLActionsPrint(const plan_pddl_actions_t *actions,
                          const plan_pddl_objs_t *objs,
                          const plan_pddl_predicates_t *predicates,
                          const plan_pddl_predicates_t *functions,
                          FILE *fout)
{
    int i;

    fprintf(fout, "Action[%d]:\n", actions->size);
    for (i = 0; i < actions->size; ++i)
        planPDDLActionPrint(actions->action + i, objs,
                            predicates, functions, fout);
}
