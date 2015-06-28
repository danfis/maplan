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
#include <plan/pddl.h>

#include "pddl_err.h"


#define PLAN_PDDL_COND_NONE     0
#define PLAN_PDDL_COND_AND      1
#define PLAN_PDDL_COND_OR       2
#define PLAN_PDDL_COND_NOT      3
#define PLAN_PDDL_COND_IMPLY    4
#define PLAN_PDDL_COND_EXISTS   5
#define PLAN_PDDL_COND_FORALL   6
#define PLAN_PDDL_COND_WHEN     7
#define PLAN_PDDL_COND_INCREASE 50
#define PLAN_PDDL_COND_FUNC     51
#define PLAN_PDDL_COND_PRED     100
#define PLAN_PDDL_COND_CONST    101
#define PLAN_PDDL_COND_VAR      102
#define PLAN_PDDL_COND_INT      103

/**
 * Condition (sub-)tree.
 */
typedef struct _plan_pddl_cond_t plan_pddl_cond_t;
struct _plan_pddl_cond_t {
    int type;               /*!< Type of the root node */
    int val;                /*!< Value of the root node if any */
    plan_pddl_cond_t *arg;  /*!< List of arguments */
    int arg_size;           /*!< Number of arguments */
};

/**
 * Mapping between keyword on type of condition node.
 */
struct _cond_type_t {
    int kw;
    int cond;
};
typedef struct _cond_type_t cond_type_t;

static cond_type_t cond_type[] = {
    { PLAN_PDDL_KW_INCREASE, PLAN_PDDL_COND_INCREASE },

    { PLAN_PDDL_KW_AND, PLAN_PDDL_COND_AND },
    { PLAN_PDDL_KW_OR, PLAN_PDDL_COND_OR },
    { PLAN_PDDL_KW_NOT, PLAN_PDDL_COND_NOT },
    { PLAN_PDDL_KW_IMPLY, PLAN_PDDL_COND_IMPLY },
    { PLAN_PDDL_KW_EXISTS, PLAN_PDDL_COND_EXISTS },
    { PLAN_PDDL_KW_FORALL, PLAN_PDDL_COND_FORALL },
    { PLAN_PDDL_KW_WHEN, PLAN_PDDL_COND_WHEN },
};
static int cond_type_size = sizeof(cond_type) / sizeof(cond_type_t);

/** Returns PLAN_PDDL_COND_* type based on keyword. */
static int condType(int kw);
/** Initialize and free condition tree */
static void condInit(plan_pddl_cond_t *c);
static void condFree(plan_pddl_cond_t *c);
/** Flatten condition tree so there are not nested conjunctions */
static void condFlattenAnd(plan_pddl_cond_t *cond);
/** Parse pddl node tree into condition tree. */
static int condParse(plan_pddl_t *pddl, const plan_pddl_lisp_node_t *root,
                     int action_id, plan_pddl_cond_t *cond);
static void condPrint(const plan_pddl_t *pddl, const plan_pddl_action_t *a,
                      const plan_pddl_cond_t *cond, FILE *fout);

static void predicateInit(plan_pddl_predicate_t *p)
{
    bzero(p, sizeof(*p));
}

static void predicateFree(plan_pddl_predicate_t *p)
{
    if (p->param != NULL)
        BOR_FREE(p->param);
}

static void predicateFree2(plan_pddl_predicate_t *p, int size)
{
    int i;
    for (i = 0; i < size; ++i)
        predicateFree(p + i);
}

static void predicateDel2(plan_pddl_predicate_t *p, int size)
{
    if (p == NULL)
        return;
    predicateFree2(p, size);
    BOR_FREE(p);
}


static void factInit(plan_pddl_fact_t *f)
{
    bzero(f, sizeof(*f));
}

static void factFree(plan_pddl_fact_t *f)
{
    if (f->arg != NULL)
        BOR_FREE(f->arg);
}

static void factFree2(plan_pddl_fact_t *f, int size)
{
    int i;
    for (i = 0; i < size; ++i)
        factFree(f + i);
}

static void factDel2(plan_pddl_fact_t *f, int size)
{
    if (f == NULL)
        return;
    factFree2(f, size);
    BOR_FREE(f);
}

static plan_pddl_fact_t *factNewArr(int size)
{
    return BOR_CALLOC_ARR(plan_pddl_fact_t, size);
}

static void factPrint(const plan_pddl_t *pddl, const plan_pddl_action_t *a,
                      const plan_pddl_fact_t *f, FILE *fout)
{
    int i, id;

    if (f->neg)
        fprintf(fout, "N:");
    fprintf(fout, "%s:", pddl->predicate[f->pred].name);
    for (i = 0; i < f->arg_size; ++i){
        if (a != NULL){
            if (f->arg[i] >= 0){
                fprintf(fout, " %s", a->param[f->arg[i]].name);
            }else{
                id = f->arg[i] + pddl->obj.size;
                fprintf(fout, " %s", pddl->obj.obj[id].name);
            }
        }else{
            if (f->arg[i] >= 0){
                fprintf(fout, " %s", pddl->obj.obj[f->arg[i]].name);
            }else{
                fprintf(fout, " ???");
            }
        }
    }
}

static void factPrintArr(const plan_pddl_t *pddl,
                         const plan_pddl_action_t *a,
                         const plan_pddl_fact_t *f, int size,
                         FILE *fout, const char *prefix)
{
    int i;

    for (i = 0; i < size; ++i){
        fprintf(fout, "%s", prefix);
        factPrint(pddl, a, f + i, fout);
        fprintf(fout, "\n");
    }
}

static void actionInit(plan_pddl_action_t *a)
{
    bzero(a, sizeof(*a));
}

static void actionDel2(plan_pddl_action_t *a, int size);
static void actionFree(plan_pddl_action_t *a)
{
    int i;

    if (a->param != NULL)
        BOR_FREE(a->param);
    factDel2(a->pre, a->pre_size);
    factDel2(a->eff, a->eff_size);
    actionDel2(a->cond_eff, a->cond_eff_size);
    for (i = 0; i < a->cost_size; ++i){
        if (a->cost[i].arg != NULL)
            BOR_FREE(a->cost[i].arg);
    }
    if (a->cost != NULL)
        BOR_FREE(a->cost);
}

static void actionFree2(plan_pddl_action_t *a, int size)
{
    int i;
    for (i = 0; i < size; ++i)
        actionFree(a + i);
}

static void actionDel2(plan_pddl_action_t *a, int size)
{
    if (a == NULL)
        return;
    actionFree2(a, size);
    BOR_FREE(a);
}

static void actionPrint(const plan_pddl_t *pddl,
                        const plan_pddl_action_t *a, FILE *fout)
{
    int i, j, id;

    fprintf(fout, "    %s:", a->name);
    for (i = 0; i < a->param_size; ++i)
        fprintf(fout, " %s:%d", a->param[i].name, a->param[i].type);
    fprintf(fout, "\n");

    fprintf(fout, "        pre[%d]:\n", a->pre_size);
    factPrintArr(pddl, a, a->pre, a->pre_size, fout, "            ");

    fprintf(fout, "        eff[%d]:\n", a->eff_size);
    factPrintArr(pddl, a, a->eff, a->eff_size, fout, "            ");

    fprintf(fout, "        cond-eff[%d]:\n", a->cond_eff_size);
    for (i = 0; i < a->cond_eff_size; ++i){
        fprintf(fout, "            pre[%d]:\n", a->cond_eff[i].pre_size);
        factPrintArr(pddl, a, a->cond_eff[i].pre, a->cond_eff[i].pre_size,
                     fout, "                ");
        fprintf(fout, "            eff[%d]:\n", a->cond_eff[i].eff_size);
        factPrintArr(pddl, a, a->cond_eff[i].eff, a->cond_eff[i].eff_size,
                     fout, "                ");
    }

    for (i = 0; i < a->cost_size; ++i){
        fprintf(fout, "        cost: ");
        if (a->cost[i].func == -1){
            fprintf(fout, "%d", a->cost[i].val);
        }else{
            fprintf(fout, "%s", pddl->function[a->cost[i].func].name);
            for (j = 0; j < a->cost[i].arg_size; ++j){
                if (a->cost[i].arg[j] >= 0){
                    fprintf(fout, " %s", a->param[a->cost[i].arg[j]].name);
                }else{
                    id = a->cost[i].arg[j] + pddl->obj.size;
                    fprintf(fout, " %s", pddl->obj.obj[id].name);
                }
            }
        }
        fprintf(fout, "\n");
    }
}

static void actionPrintArr(const plan_pddl_t *pddl,
                           const plan_pddl_action_t *a,int size,
                           FILE *fout)
{
    int i;
    for (i = 0; i < size; ++i)
        actionPrint(pddl, a + i, fout);
}



typedef int (*parse_typed_list_set_fn)(plan_pddl_t *pddl,
                                       const plan_pddl_lisp_node_t *root,
                                       int child_from, int child_to,
                                       int child_type);
static int parseTypedList(plan_pddl_t *pddl,
                          const plan_pddl_lisp_node_t *root,
                          int from, int to,
                          parse_typed_list_set_fn set_fn)
{
    plan_pddl_lisp_node_t *n;
    int i, itfrom, itto, ittype;

    ittype = -1;
    itfrom = from;
    itto = from;
    for (i = from; i < to; ++i){
        n = root->child + i;

        if (strcmp(n->value, "-") == 0){
            itto = i;
            ittype = ++i;
            if (ittype >= to){
                ERRN2(root, "Invalid typed list.");
                return -1;
            }
            if (set_fn(pddl, root, itfrom, itto, ittype) != 0)
                return -1;
            itfrom = i + 1;
            itto = i + 1;
            ittype = -1;

        }else{
            ++itto;
        }
    }

    if (itfrom < itto){
        if (set_fn(pddl, root, itfrom, itto, -1) != 0)
            return -1;
    }

    return 0;
}

static const char *parseName(plan_pddl_lisp_node_t *root, int kw,
                             const char *err_name)
{
    const plan_pddl_lisp_node_t *n;

    n = planPDDLLispFindNode(root, kw);
    if (n == NULL){
        ERR("Could not find %s name definition.", err_name);
        return NULL;
    }

    if (n->child_size != 2 || n->child[1].value == NULL){
        ERRN(n, "Invalid %s name definition", err_name);
        return NULL;
    }

    return n->child[1].value;
}

static const char *parseDomainName(plan_pddl_lisp_node_t *root)
{
    return parseName(root, PLAN_PDDL_KW_DOMAIN, "domain");
}

static const char *parseProblemName(plan_pddl_lisp_node_t *root)
{
    return parseName(root, PLAN_PDDL_KW_PROBLEM, "problem");
}

static int checkDomainName(plan_pddl_t *pddl)
{
    const char *problem_domain_name;

    problem_domain_name = parseName(&pddl->problem_lisp->root,
                                    PLAN_PDDL_KW_DOMAIN, ":domain");
    if (problem_domain_name == NULL
            || strcmp(problem_domain_name, pddl->domain_name) != 0){
        WARN("Domain names does not match: `%s' x `%s'",
             pddl->domain_name, problem_domain_name);
        return 0;
    }
    return 0;
}


static int addPredicate(plan_pddl_t *pddl, const char *name)
{
    int i, id;

    for (i = 0; i < pddl->predicate_size; ++i){
        if (strcmp(pddl->predicate[i].name, name) == 0){
            ERR("Duplicate predicate `%s'", name);
            return -1;
        }
    }

    ++pddl->predicate_size;
    pddl->predicate = BOR_REALLOC_ARR(pddl->predicate,
                                      plan_pddl_predicate_t,
                                      pddl->predicate_size);
    id = pddl->predicate_size - 1;
    predicateInit(pddl->predicate + id);
    pddl->predicate[id].name = name;
    return id;
}

static int getPredicate(plan_pddl_t *pddl, const char *name)
{
    int i;

    for (i = 0; i < pddl->predicate_size; ++i){
        if (strcmp(name, pddl->predicate[i].name) == 0)
            return i;
    }

    return -1;
}

static void addEqPredicate(plan_pddl_t *pddl)
{
    int id;

    id = addPredicate(pddl, "=");
    pddl->predicate[id].param_size = 2;
    pddl->predicate[id].param = BOR_CALLOC_ARR(int, 2);
    pddl->eq_pred_id = id;
}

static int parsePredicateSet(plan_pddl_t *pddl,
                             const plan_pddl_lisp_node_t *root,
                             int child_from, int child_to, int child_type)
{
    plan_pddl_predicate_t *pred;
    int i, j, tid;

    tid = 0;
    if (child_type >= 0){
        tid = planPDDLTypesGet(&pddl->type, root->child[child_type].value);
        if (tid < 0){
            ERRN(root->child + child_type, "Invalid type `%s'",
                 root->child[child_type].value);
            return -1;
        }
    }

    pred = pddl->predicate + pddl->predicate_size - 1;
    j = pred->param_size;
    pred->param_size += child_to - child_from;
    pred->param = BOR_REALLOC_ARR(pred->param, int, pred->param_size);
    for (i = child_from; i < child_to; ++i, ++j)
        pred->param[j] = tid;
    return 0;
}

static int parsePredicate1(plan_pddl_t *pddl, const plan_pddl_lisp_node_t *root)
{
    if (root->child_size < 1 || root->child[0].value == NULL){
        ERRN2(root, "Invalid definition of predicate.");
        return -1;
    }

    if (addPredicate(pddl, root->child[0].value) < 0)
        return -1;

    if (parseTypedList(pddl, root, 1, root->child_size, parsePredicateSet) != 0)
        return -1;
    return 0;
}

static int parsePredicate(plan_pddl_t *pddl, const plan_pddl_lisp_node_t *root)
{
    const plan_pddl_lisp_node_t *n;
    int i;

    n = planPDDLLispFindNode(root, PLAN_PDDL_KW_PREDICATES);
    if (n == NULL)
        return 0;

    for (i = 1; i < n->child_size; ++i){
        if (parsePredicate1(pddl, n->child + i) != 0)
            return -1;
    }
    return 0;
}

static int addFunction(plan_pddl_t *pddl, const char *name)
{
    int i, id;

    for (i = 0; i < pddl->function_size; ++i){
        if (strcmp(pddl->function[i].name, name) == 0){
            ERR("Duplicate function `%s'", name);
            return -1;
        }
    }

    ++pddl->function_size;
    pddl->function = BOR_REALLOC_ARR(pddl->function,
                                     plan_pddl_predicate_t,
                                     pddl->function_size);
    id = pddl->function_size - 1;
    pddl->function[id].name = name;
    pddl->function[id].param = NULL;
    pddl->function[id].param_size = 0;
    return id;
}

static int getFunction(plan_pddl_t *pddl, const char *name)
{
    int i;

    for (i = 0; i < pddl->function_size; ++i){
        if (strcmp(name, pddl->function[i].name) == 0)
            return i;
    }

    return -1;
}

static int parseFunctionSet(plan_pddl_t *pddl,
                            const plan_pddl_lisp_node_t *root,
                            int child_from, int child_to, int child_type)
{
    plan_pddl_predicate_t *pred;
    int i, j, tid;

    tid = 0;
    if (child_type >= 0){
        tid = planPDDLTypesGet(&pddl->type, root->child[child_type].value);
        if (tid < 0){
            ERRN(root->child + child_type, "Invalid type `%s'",
                 root->child[child_type].value);
            return -1;
        }
    }

    pred = pddl->function + pddl->function_size - 1;
    j = pred->param_size;
    pred->param_size += child_to - child_from;
    pred->param = BOR_REALLOC_ARR(pred->param, int, pred->param_size);
    for (i = child_from; i < child_to; ++i, ++j)
        pred->param[j] = tid;
    return 0;
}

static int parseFunction1(plan_pddl_t *pddl, const plan_pddl_lisp_node_t *root)
{
    if (root->child_size < 1 || root->child[0].value == NULL){
        ERRN2(root, "Invalid definition of predicate.");
        return -1;
    }

    if (addFunction(pddl, root->child[0].value) < 0)
        return -1;

    if (parseTypedList(pddl, root, 1, root->child_size, parseFunctionSet) != 0)
        return -1;
    return 0;
}

static int parseFunction(plan_pddl_t *pddl, const plan_pddl_lisp_node_t *root)
{
    const plan_pddl_lisp_node_t *n;
    int i;

    n = planPDDLLispFindNode(root, PLAN_PDDL_KW_FUNCTIONS);
    if (n == NULL)
        return 0;

    for (i = 1; i < n->child_size; ++i){
        if (parseFunction1(pddl, n->child + i) != 0)
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

static int addAction(plan_pddl_t *pddl, const char *name)
{
    int id;

    id = pddl->action_size;
    ++pddl->action_size;
    pddl->action = BOR_REALLOC_ARR(pddl->action, plan_pddl_action_t,
                                   pddl->action_size);
    actionInit(pddl->action + id);
    pddl->action[id].name = name;

    return id;
}

static int getActionParam(const plan_pddl_t *pddl, int action_id,
                          const char *param_name)
{
    plan_pddl_action_t *a = pddl->action + action_id;
    int i;

    for (i = 0; i < a->param_size; ++i){
        if (strcmp(a->param[i].name, param_name) == 0)
            return i;
    }
    return -1;
}

static int parseActionParamSet(plan_pddl_t *pddl,
                               const plan_pddl_lisp_node_t *root,
                               int child_from, int child_to, int child_type)
{
    int id = pddl->action_size - 1;
    plan_pddl_action_t *a = pddl->action + id;
    int i, j, tid;

    tid = 0;
    if (child_type >= 0){
        tid = planPDDLTypesGet(&pddl->type, root->child[child_type].value);
        if (tid < 0){
            ERRN(root->child + child_type, "Unkown type `%s'",
                 root->child[child_type].value);
            return -1;
        }
    }

    j = a->param_size;
    a->param_size += child_to - child_from;
    a->param = BOR_REALLOC_ARR(a->param, plan_pddl_obj_t, a->param_size);
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

        a->param[j].name = root->child[i].value;
        a->param[j].type = tid;
    }

    return 0;
}

static int condVarConstArg(const plan_pddl_t *pddl,
                           const plan_pddl_cond_t *c, int i)
{
    if (c->arg[i].type == PLAN_PDDL_COND_VAR){
        return c->arg[i].val;
    }else if (c->arg[i].type == PLAN_PDDL_COND_CONST){
        return c->arg[i].val - pddl->obj.size;
    }
    return -1;
}

static void condToActionFact(plan_pddl_t *pddl,
                             const plan_pddl_cond_t *c,
                             int neg, plan_pddl_fact_t *f)
{
    int i;

    f->pred = c->val;
    f->neg = neg;
    f->arg_size = c->arg_size;
    f->arg = BOR_CALLOC_ARR(int, c->arg_size);
    for (i = 0; i < f->arg_size; ++i){
        f->arg[i] = condVarConstArg(pddl, c, i);
    }
}

static int parseActionPre(plan_pddl_t *pddl,
                          plan_pddl_action_t *a,
                          plan_pddl_cond_t *cond)
{
    plan_pddl_cond_t *c;
    int i, neg;

    a->pre_size = cond->arg_size;
    a->pre = factNewArr(a->pre_size);
    for (i = 0; i < a->pre_size; ++i){
        c = cond->arg + i;
        neg = 0;
        if (c->type == PLAN_PDDL_COND_NOT){
            neg = 1;
            c = c->arg;
        }

        if (c->type != PLAN_PDDL_COND_PRED){
            ERR("Unsupported precondition of action `%s'", a->name);
            return -1;
        }

        condToActionFact(pddl, c, neg, a->pre + i);
    }

    return 0;
}

static int parseActionEff(plan_pddl_t *pddl,
                          plan_pddl_action_t *a,
                          plan_pddl_cond_t *cond);
static int addActionCondEff(plan_pddl_t *pddl, plan_pddl_action_t *a,
                            plan_pddl_cond_t *cond)
{
    plan_pddl_action_t *ca;

    ++a->cond_eff_size;
    a->cond_eff = BOR_REALLOC_ARR(a->cond_eff, plan_pddl_action_t,
                                  a->cond_eff_size);
    ca = a->cond_eff + a->cond_eff_size - 1;
    actionInit(ca);
    if (parseActionPre(pddl, ca, cond->arg) != 0)
        return -1;
    if (parseActionEff(pddl, ca, cond->arg + 1) != 0)
        return -1;

    return 0;
}

static int addActionCost(plan_pddl_t *pddl, plan_pddl_action_t *a,
                         plan_pddl_cond_t *cond)
{
    plan_pddl_inst_func_t *f;
    plan_pddl_cond_t *ccost;
    int i;

    ++a->cost_size;
    a->cost = BOR_REALLOC_ARR(a->cost, plan_pddl_inst_func_t, a->cost_size);
    f = a->cost + a->cost_size - 1;
    bzero(f, sizeof(*f));
    ccost = cond->arg;
    if (ccost->type == PLAN_PDDL_COND_INT){
        f->func = -1;
        f->val = ccost->val;
    }else{
        f->func = ccost->val;
        f->arg_size = ccost->arg_size;
        f->arg = BOR_ALLOC_ARR(int, f->arg_size);
        for (i = 0; i < f->arg_size; ++i)
            f->arg[i] = condVarConstArg(pddl, ccost, i);
    }
    return 0;
}

static int parseActionEff(plan_pddl_t *pddl,
                          plan_pddl_action_t *a,
                          plan_pddl_cond_t *cond)
{
    plan_pddl_cond_t *c;
    int i, neg;

    a->eff_size = 0;
    a->eff = factNewArr(cond->arg_size);
    for (i = 0; i < cond->arg_size; ++i){
        c = cond->arg + i;
        neg = 0;
        if (c->type == PLAN_PDDL_COND_NOT){
            neg = 1;
            c = c->arg;
        }

        if (c->type == PLAN_PDDL_COND_PRED){
            condToActionFact(pddl, c, neg, a->eff + a->eff_size++);

        }else if (c->type == PLAN_PDDL_COND_WHEN){
            if (addActionCondEff(pddl, a, c) != 0)
                return -1;

        }else if (c->type == PLAN_PDDL_COND_INCREASE){
            if (addActionCost(pddl, a, c) != 0)
                return -1;
        }else{
            ERR("Unsupported effect of action `%s'", a->name);
            return -1;
        }

    }

    return 0;
}

static int parseAction1(plan_pddl_t *pddl, const plan_pddl_lisp_node_t *root)
{
    const plan_pddl_lisp_node_t *n;
    plan_pddl_cond_t cond;
    plan_pddl_action_t *a;
    int action_id, i, ret;

    if (root->child_size < 4
            || root->child_size / 2 == 1
            || root->child[1].value == NULL){
        ERRN2(root, "Invalid definition of :action");
        return -1;
    }

    action_id = addAction(pddl, root->child[1].value);
    a = pddl->action + action_id;
    for (i = 2; i < root->child_size; i += 2){
        n = root->child + i + 1;

        if (root->child[i].kw == PLAN_PDDL_KW_PARAMETERS){
            if (parseTypedList(pddl, n, 0, n->child_size,
                               parseActionParamSet) != 0)
                return -1;

        }else if (root->child[i].kw == PLAN_PDDL_KW_PRE
                    || root->child[i].kw == PLAN_PDDL_KW_EFF){
            condInit(&cond);
            ret = condParse(pddl, n, action_id, &cond);
            if (ret == 0){
                condFlattenAnd(&cond);
                if (root->child[i].kw == PLAN_PDDL_KW_PRE){
                    ret = parseActionPre(pddl, a, &cond);
                }else{
                    ret = parseActionEff(pddl, a, &cond);
                }
            }
            condFree(&cond);
            if (ret != 0)
                return ret;

        }else{
            ERRN(root->child + i, "Invalid definition of :action."
                                  " Unexpected token: %s",
                                  root->child[i].value);
            return -1;
        }
    }

    return 0;
}

static int parseAction(plan_pddl_t *pddl, plan_pddl_lisp_node_t *root)
{
    int i;

    for (i = 0; i < root->child_size; ++i){
        if (planPDDLLispNodeHeadKw(root->child + i) == PLAN_PDDL_KW_ACTION){
            if (parseAction1(pddl, root->child + i) != 0)
                return -1;
        }
    }
    return 0;
}

static int parseGoal(plan_pddl_t *pddl, const plan_pddl_lisp_node_t *root)
{
    const plan_pddl_lisp_node_t *n;
    plan_pddl_cond_t cond;
    int i, j;

    n = planPDDLLispFindNode(root, PLAN_PDDL_KW_GOAL);
    if (n == NULL){
        ERR2("Could not find :goal.");
        return -1;
    }
    if (n->child_size != 2
            || n->child[1].value != NULL){
        ERRN2(n, "Invalid definition of :goal.");
        return -1;
    }

    condInit(&cond);
    if (condParse(pddl, n->child + 1, -1, &cond) != 0){
        condFree(&cond);
        return -1;
    }
    condFlattenAnd(&cond);

    pddl->goal = factNewArr(cond.arg_size);
    pddl->goal_size = 0;
    for (i = 0; i < cond.arg_size; ++i){
        if (cond.arg[i].type != PLAN_PDDL_COND_PRED){
            ERRN2(root, "Unsupported goal definition. Only conjunctive"
                        " form of non-negated facts is allowed.");
            return -1;
        }
        pddl->goal[i].pred = cond.arg[i].val;
        pddl->goal[i].arg = BOR_ALLOC_ARR(int, cond.arg[i].arg_size);
        pddl->goal[i].arg_size = cond.arg[i].arg_size;
        for (j = 0; j < cond.arg[i].arg_size; ++j){
            if (cond.arg[i].arg[j].type != PLAN_PDDL_COND_CONST){
                ERRN2(root, "Facts in goal must be grounded.");
                return -1;
            }
            pddl->goal[i].arg[j] = cond.arg[i].arg[j].val;
        }
        ++pddl->goal_size;
    }

    condFree(&cond);
    return 0;
}

static int parseObjsIntoArr(plan_pddl_t *pddl, const plan_pddl_lisp_node_t *n,
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

        obj[i] = planPDDLObjsGet(&pddl->obj, c->value);
        if (obj[i] < 0){
            ERRN(c, "Unknown object `%s'.", c->value);
            return -1;
        }
    }

    return 0;
}

static int parseInstFunc(plan_pddl_t *pddl, const plan_pddl_lisp_node_t *n)
{
    const plan_pddl_lisp_node_t *nfunc, *nval;
    plan_pddl_inst_func_t *func;

    nfunc = n->child + 1;
    nval = n->child + 2;

    if (nfunc->child_size < 1
            || nfunc->child[0].value == NULL
            || nval->value == NULL){
        ERRN2(n, "Invalid function assignement.");
        return -1;
    }

    func = pddl->init_func + pddl->init_func_size++;
    func->val = atoi(nval->value);
    func->func = getFunction(pddl, nfunc->child[0].value);
    if (func->func < 0){
        ERRN(nfunc, "Unknown function `%s'", nfunc->child[0].value);
        return -1;
    }

    return parseObjsIntoArr(pddl, nfunc, 1, nfunc->child_size,
                            &func->arg, &func->arg_size);
}

static int parseFact(plan_pddl_t *pddl, const char *head,
                     const plan_pddl_lisp_node_t *n)
{
    plan_pddl_fact_t *fact = pddl->init_fact + pddl->init_fact_size++;

    fact->pred = getPredicate(pddl, head);
    if (fact->pred < 0){
        ERRN(n, "Unkwnown predicate `%s'.", head);
        return -1;
    }

    return parseObjsIntoArr(pddl, n, 1, n->child_size,
                            &fact->arg, &fact->arg_size);
}

static int parseFactFunc(plan_pddl_t *pddl, const plan_pddl_lisp_node_t *n)
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
        return parseInstFunc(pddl, n);
    }else{
        return parseFact(pddl, head, n);
    }
}

static int parseInit(plan_pddl_t *pddl, const plan_pddl_lisp_node_t *root)
{
    const plan_pddl_lisp_node_t *I, *n;
    int i;

    I = planPDDLLispFindNode(root, PLAN_PDDL_KW_INIT);
    if (I == NULL){
        ERR2("Missing :init.");
        return -1;
    }

    // Pre-allocate facts and functions
    pddl->init_fact = factNewArr(I->child_size - 1);
    pddl->init_func = BOR_CALLOC_ARR(plan_pddl_inst_func_t, I->child_size - 1);

    for (i = 1; i < I->child_size; ++i){
        n = I->child + i;
        if (parseFactFunc(pddl, n) != 0)
            return -1;
    }

    return 0;
}

static int parseMetric(plan_pddl_t *pddl, const plan_pddl_lisp_node_t *root)
{
    const plan_pddl_lisp_node_t *n;

    n = planPDDLLispFindNode(root, PLAN_PDDL_KW_METRIC);
    if (n == NULL)
        return 0;

    if (n->child_size != 3
            || n->child[1].value == NULL
            || n->child[1].kw != PLAN_PDDL_KW_MINIMIZE
            || n->child[2].value != NULL
            || n->child[2].child_size != 1
            || strcmp(n->child[2].child[0].value, "total-cost") != 0){
        ERRN2(n, "Only (:metric minimize (total-cost)) is supported.");
        return -1;
    }

    pddl->metric = 1;
    return 0;
}

plan_pddl_t *planPDDLNew(const char *domain_fn, const char *problem_fn)
{
    plan_pddl_t *pddl;
    plan_pddl_lisp_t *domain_lisp, *problem_lisp;

    domain_lisp = planPDDLLispParse(domain_fn);
    problem_lisp = planPDDLLispParse(problem_fn);
    if (domain_lisp == NULL || problem_lisp == NULL){
        if (domain_lisp)
            planPDDLLispDel(domain_lisp);
        if (problem_lisp)
            planPDDLLispDel(problem_lisp);
        return NULL;
    }

    pddl = BOR_ALLOC(plan_pddl_t);
    bzero(pddl, sizeof(*pddl));
    pddl->eq_pred_id = -1;
    pddl->domain_lisp = domain_lisp;
    pddl->problem_lisp = problem_lisp;
    pddl->domain_name = parseDomainName(&domain_lisp->root);
    if (pddl->domain_name == NULL)
        goto pddl_fail;

    pddl->problem_name = parseProblemName(&problem_lisp->root);
    if (pddl->domain_name == NULL)
        goto pddl_fail;

    if (checkDomainName(pddl) != 0)
        goto pddl_fail;

    if (planPDDLRequireParse(domain_lisp, &pddl->require) != 0
            || planPDDLTypesParse(domain_lisp, &pddl->type) != 0)
        goto pddl_fail;

    // Add (= ?x ?y - object) predicate if set in requirements
    if (pddl->require & PLAN_PDDL_REQUIRE_EQUALITY)
        addEqPredicate(pddl);

    if (planPDDLObjsParse(domain_lisp, problem_lisp, &pddl->type, &pddl->obj) != 0
            || planPDDLTypeObjInit(&pddl->type_obj, &pddl->type, &pddl->obj) != 0
            || parsePredicate(pddl, &domain_lisp->root) != 0
            || parseFunction(pddl, &domain_lisp->root) != 0
            || parseAction(pddl, &domain_lisp->root) != 0
            || parseGoal(pddl, &problem_lisp->root) != 0
            || parseInit(pddl, &problem_lisp->root) != 0
            || parseMetric(pddl, &problem_lisp->root) != 0){
        goto pddl_fail;
    }

    return pddl;

pddl_fail:
    if (pddl != NULL)
        planPDDLDel(pddl);
    return NULL;
}

void planPDDLDel(plan_pddl_t *pddl)
{
    int i;

    if (pddl->domain_lisp)
        planPDDLLispDel(pddl->domain_lisp);
    if (pddl->problem_lisp)
        planPDDLLispDel(pddl->problem_lisp);
    planPDDLTypesFree(&pddl->type);
    planPDDLObjsFree(&pddl->obj);
    planPDDLTypeObjFree(&pddl->type_obj);

    predicateDel2(pddl->predicate, pddl->predicate_size);
    predicateDel2(pddl->function, pddl->function_size);
    actionDel2(pddl->action, pddl->action_size);
    factDel2(pddl->goal, pddl->goal_size);
    factDel2(pddl->init_fact, pddl->init_fact_size);

    for (i = 0; i < pddl->init_func_size; ++i){
        if (pddl->init_func[i].arg != NULL)
            BOR_FREE(pddl->init_func[i].arg);
    }
    if (pddl->init_func)
        BOR_FREE(pddl->init_func);

    BOR_FREE(pddl);
}


void planPDDLDump(const plan_pddl_t *pddl, FILE *fout)
{
    int i, j;

    fprintf(fout, "Domain: %s\n", pddl->domain_name);
    fprintf(fout, "Problem: %s\n", pddl->problem_name);
    fprintf(fout, "Require: %x\n", pddl->require);
    planPDDLTypesPrint(&pddl->type, fout);
    planPDDLObjsPrint(&pddl->obj, fout);
    planPDDLTypeObjPrint(&pddl->type_obj, fout);

    fprintf(fout, "Predicate[%d]:\n", pddl->predicate_size);
    for (i = 0; i < pddl->predicate_size; ++i){
        fprintf(fout, "    %s:", pddl->predicate[i].name);
        for (j = 0; j < pddl->predicate[i].param_size; ++j){
            fprintf(fout, " %d", pddl->predicate[i].param[j]);
        }
        fprintf(fout, "\n");
    }

    fprintf(fout, "Function[%d]:\n", pddl->function_size);
    for (i = 0; i < pddl->function_size; ++i){
        fprintf(fout, "    %s:", pddl->function[i].name);
        for (j = 0; j < pddl->function[i].param_size; ++j){
            fprintf(fout, " %d", pddl->function[i].param[j]);
        }
        fprintf(fout, "\n");
    }

    fprintf(fout, "Action[%d]:\n", pddl->action_size);
    actionPrintArr(pddl, pddl->action, pddl->action_size, fout);
    fprintf(fout, "Goal[%d]:\n", pddl->goal_size);
    factPrintArr(pddl, NULL, pddl->goal, pddl->goal_size, fout, "    ");
    fprintf(fout, "Init[%d]:\n", pddl->init_fact_size);
    factPrintArr(pddl, NULL, pddl->init_fact, pddl->init_fact_size, fout, "    ");

    fprintf(fout, "Init Func[%d]:\n", pddl->init_func_size);
    for (i = 0; i < pddl->init_func_size; ++i){
        fprintf(fout, "    %s:",
                pddl->function[pddl->init_func[i].func].name);
        for (j = 0; j < pddl->init_func[i].arg_size; ++j)
            fprintf(fout, " %s",
                    pddl->obj.obj[pddl->init_func[i].arg[j]].name);
        fprintf(fout, " --> %d\n", pddl->init_func[i].val);
    }

    fprintf(fout, "Metric: %d\n", pddl->metric);
}


/**** CONDITION ****/
static int condType(int kw)
{
    int i;

    for (i = 0; i < cond_type_size; ++i){
        if (cond_type[i].kw == kw)
            return cond_type[i].cond;
    }
    return -1;
}


static void condInit(plan_pddl_cond_t *c)
{
    bzero(c, sizeof(*c));
}

static void condFree(plan_pddl_cond_t *c)
{
    int i;

    for (i = 0; i < c->arg_size; ++i)
        condFree(c->arg + i);
    if (c->arg != NULL)
        BOR_FREE(c->arg);
}

static void condFlattenAnd(plan_pddl_cond_t *cond)
{
    plan_pddl_cond_t cond_tmp;
    int i, j, size;

    if (cond->type == PLAN_PDDL_COND_PRED){
        // Replace a single precondition with (and (precond ...))
        cond_tmp = *cond;
        cond->type = PLAN_PDDL_COND_AND;
        cond->val = 0;
        cond->arg = BOR_ALLOC(plan_pddl_cond_t);
        cond->arg_size = 1;
        cond->arg[0] = cond_tmp;

    }else if (cond->type == PLAN_PDDL_COND_AND){
        for (i = 0; i < cond->arg_size; ++i){
            if (cond->arg[i].type == PLAN_PDDL_COND_AND){
                cond_tmp = cond->arg[i];
                condFlattenAnd(&cond_tmp);
                size = cond->arg_size + cond_tmp.arg_size - 1;
                cond->arg = BOR_REALLOC_ARR(cond->arg, plan_pddl_cond_t, size);

                cond->arg[i] = cond_tmp.arg[0];
                for (j = 1; j < cond_tmp.arg_size; j++){
                    cond->arg[cond->arg_size++] = cond_tmp.arg[j];
                }
                bzero(cond_tmp.arg, sizeof(plan_pddl_cond_t) * cond_tmp.arg_size);
                condFree(&cond_tmp);

            }else if (cond->arg[i].type == PLAN_PDDL_COND_WHEN){
                condFlattenAnd(cond->arg + i);
            }
        }

    }else if (cond->type == PLAN_PDDL_COND_WHEN){
        // Flatten both pre and eff
        condFlattenAnd(cond->arg);
        condFlattenAnd(cond->arg + 1);
    }
}

static int condParseVarConst(plan_pddl_t *pddl, const plan_pddl_lisp_node_t *root,
                             int action_id, plan_pddl_cond_t *cond)
{
    if (root->value[0] == '?'){
        if (action_id < 0){
            ERRN(root, "Unexpected variable: `%s'", root->value);
            return -1;
        }

        cond->type = PLAN_PDDL_COND_VAR;
        cond->val = getActionParam(pddl, action_id, root->value);
        if (cond->val == -1){
            ERRN(root, "Invalid paramenter name `%s'", root->value);
            return -1;
        }

    }else{
        cond->type = PLAN_PDDL_COND_CONST;
        cond->val = planPDDLObjsGet(&pddl->obj, root->value);
        if (cond->val == -1){
            ERRN(root, "Invalid object `%s'", root->value);
            return -1;
        }
    }

    return 0;
}

static int condParsePred(plan_pddl_t *pddl, const plan_pddl_lisp_node_t *root,
                         int action_id, plan_pddl_cond_t *cond)
{
    const char *name;
    int i;

    name = planPDDLLispNodeHead(root);
    if (name == NULL){
        ERRN2(root, "Invalid definition of conditional, missing head of"
                    " expression.");
        return -1;
    }

    cond->type = PLAN_PDDL_COND_PRED;
    cond->val = getPredicate(pddl, name);
    if (cond->val == -1){
        ERRN(root, "Unkown predicate `%s'", name);
        return -1;
    }

    // Check that all children are terminals
    for (i = 1; i < root->child_size; ++i){
        if (root->child[i].value == NULL){
            ERRN(root, "Invalid instantiation of predicate `%s'", name);
            return -1;
        }
    }

    cond->arg_size = root->child_size - 1;
    cond->arg = BOR_ALLOC_ARR(plan_pddl_cond_t, cond->arg_size);
    for (i = 0; i < cond->arg_size; ++i)
        condInit(cond->arg + i);
    for (i = 0; i < cond->arg_size; ++i){
        if (condParseVarConst(pddl, root->child + i + 1, action_id,
                              cond->arg + i) != 0)
            return -1;
    }

    return 0;
}

static void condParseForallReplace(plan_pddl_t *pddl, plan_pddl_lisp_node_t *n)
{
    int i;

    if (n->kw == -1 && n->value != NULL){
        for (i = 0; i < pddl->forall.param_size; ++i){
            if (strcmp(n->value, pddl->forall.param[i].name) == 0){
                n->value = pddl->obj.obj[pddl->forall.param_bind[i]].name;
            }
        }
    }

    for (i = 0; i < n->child_size; ++i)
        condParseForallReplace(pddl, n->child + i);
}

static int condParseForallEval(plan_pddl_t *pddl, const plan_pddl_lisp_node_t *root,
                               int action_id, plan_pddl_cond_t *cond)
{
    plan_pddl_lisp_node_t arg;
    int id, ret;

    planPDDLLispNodeCopy(&arg, root->child + 2);
    condParseForallReplace(pddl, &arg);

    cond->arg_size += 1;
    cond->arg = BOR_REALLOC_ARR(cond->arg, plan_pddl_cond_t, cond->arg_size);
    id = cond->arg_size - 1;
    condInit(cond->arg + id);
    ret = condParse(pddl, &arg, action_id, cond->arg + id);
    planPDDLLispNodeFree(&arg);
    return ret;
}

static int condParseForallRec(plan_pddl_t *pddl, const plan_pddl_lisp_node_t *root,
                              int action_id, plan_pddl_cond_t *cond, int param)
{
    const int *obj;
    int obj_size, i;

    obj = planPDDLTypeObjGet(&pddl->type_obj, pddl->forall.param[param].type,
                             &obj_size);
    for (i = 0; i < obj_size; ++i){
        pddl->forall.param_bind[param] = obj[i];
        if (param == pddl->forall.param_size - 1){
            if (condParseForallEval(pddl, root, action_id, cond) != 0)
                return -1;
        }else{
            if (condParseForallRec(pddl, root, action_id, cond, param + 1) != 0)
                return -1;
        }
    }

    return 0;
}

static int condParseForallSet(plan_pddl_t *pddl,
                              const plan_pddl_lisp_node_t *root,
                              int child_from, int child_to,
                              int child_type)
{
    int i, j, tid;

    tid = 0;
    if (child_type >= 0){
        tid = planPDDLTypesGet(&pddl->type, root->child[child_type].value);
        if (tid < 0){
            ERRN(root->child + child_type, "Invalid type `%s'",
                 root->child[child_type].value);
            return -1;
        }
    }

    j = pddl->forall.param_size;
    pddl->forall.param_size += child_to - child_from;
    pddl->forall.param = BOR_REALLOC_ARR(pddl->forall.param,
                                         plan_pddl_obj_t,
                                         pddl->forall.param_size);
    for (i = child_from; i < child_to; ++i, ++j){
        pddl->forall.param[j].name = root->child[i].value;
        pddl->forall.param[j].type = tid;
    }

    return 0;
}

static int condParseForall(plan_pddl_t *pddl, const plan_pddl_lisp_node_t *root,
                           int action_id, plan_pddl_cond_t *cond)
{
    if (root->child_size != 3
            || root->child[1].value != NULL
            || root->child[2].value != NULL){
        ERRN2(root, "Invalid (forall ...) condition.");
        return -1;
    }

    cond->type = PLAN_PDDL_COND_AND;

    pddl->forall.param = NULL;
    pddl->forall.param_bind = NULL;
    pddl->forall.param_size = 0;
    if (parseTypedList(pddl, root->child + 1, 0, root->child[1].child_size,
                       condParseForallSet) != 0){
        if (pddl->forall.param != NULL)
            BOR_FREE(pddl->forall.param);
        return -1;
    }

    pddl->forall.param_bind = BOR_CALLOC_ARR(int, pddl->forall.param_size);
    if (condParseForallRec(pddl, root, action_id, cond, 0) != 0){
        if (pddl->forall.param != NULL)
            BOR_FREE(pddl->forall.param);
        if (pddl->forall.param_bind != NULL)
            BOR_FREE(pddl->forall.param_bind);
        return -1;
    }

    if (pddl->forall.param != NULL)
        BOR_FREE(pddl->forall.param);
    if (pddl->forall.param_bind != NULL)
        BOR_FREE(pddl->forall.param_bind);
    return 0;
}

static int condParseFunction(plan_pddl_t *pddl, const plan_pddl_lisp_node_t *root,
                             int action_id, plan_pddl_cond_t *cond)
{
    int i;

    if (root->child_size < 1
            || root->child[0].value == NULL){
        ERRN2(root, "Invalid function definition.");
        return -1;
    }

    for (i = 1; i < root->child_size; ++i){
        if (root->child[i].value == NULL){
            ERRN(root->child + i, "Invalid %d'th argument in function"
                 " definition.", i);
            return -1;
        }
    }

    cond->type = PLAN_PDDL_COND_FUNC;
    cond->val = getFunction(pddl, root->child[0].value);
    if (cond->val < 0)
        return -1;

    cond->arg_size = root->child_size - 1;
    cond->arg = BOR_ALLOC_ARR(plan_pddl_cond_t, cond->arg_size);
    for (i = 0; i < cond->arg_size; ++i)
        condInit(cond->arg + i);

    for (i = 0; i < cond->arg_size; ++i){
        if (condParseVarConst(pddl, root->child + i + 1, action_id,
                              cond->arg + i) != 0)
            return -1;
    }

    return 0;
}

static int condParse(plan_pddl_t *pddl, const plan_pddl_lisp_node_t *root,
                     int action_id, plan_pddl_cond_t *cond)
{
    int i;

    cond->type = condType(planPDDLLispNodeHeadKw(root));
    if (cond->type == -1)
        return condParsePred(pddl, root, action_id, cond);


    if (cond->type == PLAN_PDDL_COND_NOT){
        if (root->child_size != 2
                || root->child[1].value != NULL){
            ERRN2(root, "Invalid (not ...) condition.");
            return -1;
        }

        cond->arg_size = 1;
        cond->arg = BOR_ALLOC(plan_pddl_cond_t);
        condInit(cond->arg);
        return condParse(pddl, root->child + 1, action_id, cond->arg);

    }else if (cond->type == PLAN_PDDL_COND_AND){
        if (root->child_size < 2){
            ERRN2(root, "Invalid (and/or ...) condition.");
            return -1;
        }
        for (i = 1; i < root->child_size; ++i){
            if (root->child[i].value != NULL){
                ERRN(root, "Invalid (and/or ...) condition -- %d'th argument.", i);
                return -1;
            }
        }

        cond->arg_size = root->child_size - 1;
        cond->arg = BOR_ALLOC_ARR(plan_pddl_cond_t, cond->arg_size);
        for (i = 0; i < cond->arg_size; ++i)
            condInit(cond->arg + i);
        for (i = 0; i < cond->arg_size; ++i){
            if (condParse(pddl, root->child + i + 1, action_id,
                          cond->arg + i) != 0)
                return -1;
        }

    }else if (cond->type == PLAN_PDDL_COND_OR){
        ERRN2(root, "(or ...) is not supported yet.");
        return -1;

    }else if (cond->type == PLAN_PDDL_COND_IMPLY){
        ERRN2(root, "(imply ...) is not supported yet.");
        return -1;

    }else if (cond->type == PLAN_PDDL_COND_EXISTS){
        ERRN2(root, "(exists ...) is not supported yet.");
        return -1;

    }else if (cond->type == PLAN_PDDL_COND_FORALL){
        if (condParseForall(pddl, root, action_id, cond) != 0)
            return -1;

    }else if (cond->type == PLAN_PDDL_COND_WHEN){
        if (root->child_size != 3
                || root->child[1].value != NULL
                || root->child[2].value != NULL){
            ERRN2(root, "Invalid (when ...) condition.");
            return -1;
        }

        cond->arg_size = 2;
        cond->arg = BOR_ALLOC_ARR(plan_pddl_cond_t, 2);
        condInit(cond->arg + 0);
        condInit(cond->arg + 1);
        for (i = 0; i < 2; ++i){
            if (condParse(pddl, root->child + i + 1, action_id,
                          cond->arg + i) != 0)
                return -1;
        }

    }else if (cond->type == PLAN_PDDL_COND_INCREASE){
        if (!(pddl->require & PLAN_PDDL_REQUIRE_ACTION_COST)){
            ERRN2(root, "(increase ...) is supported only under"
                        " :action-costs requirement.");
            return -1;
        }

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

        if (getFunction(pddl, "total-cost") < 0){
            ERRN2(root, "(total-cost) function must be defined in"
                        " :functions section if used in effect.");
            return -1;
        }

        cond->arg_size = 1;
        cond->arg = BOR_ALLOC(plan_pddl_cond_t);
        condInit(cond->arg);

        if (root->child[2].value != NULL){
            cond->arg[0].type = PLAN_PDDL_COND_INT;
            cond->arg[0].val = atoi(root->child[2].value);

        }else{
            if (condParseFunction(pddl, root->child + 2, action_id, cond->arg) != 0)
                return -1;
        }
    }
    return 0;
}

static const char *_condToStr(int cond_type)
{
    switch (cond_type){
        case PLAN_PDDL_COND_NONE:
            return "none";
        case PLAN_PDDL_COND_AND:
            return "and";
        case PLAN_PDDL_COND_OR:
            return "or";
        case PLAN_PDDL_COND_NOT:
            return "not";
        case PLAN_PDDL_COND_IMPLY:
            return "imply";
        case PLAN_PDDL_COND_EXISTS:
            return "exists";
        case PLAN_PDDL_COND_FORALL:
            return "forall";
        case PLAN_PDDL_COND_WHEN:
            return "when";
        case PLAN_PDDL_COND_INCREASE:
            return "COST";
        case PLAN_PDDL_COND_PRED:
            return "pred";
        case PLAN_PDDL_COND_VAR:
            return "var";
        case PLAN_PDDL_COND_CONST:
            return "const";
    }
    return "";
}

static void condPrint(const plan_pddl_t *pddl, const plan_pddl_action_t *a,
                      const plan_pddl_cond_t *cond, FILE *fout)
{
    const plan_pddl_predicate_t *pred;
    int i;

    if (cond->type == PLAN_PDDL_COND_AND
            || cond->type == PLAN_PDDL_COND_OR
            || cond->type == PLAN_PDDL_COND_NOT
            || cond->type == PLAN_PDDL_COND_WHEN
            || cond->type == PLAN_PDDL_COND_INCREASE){
        fprintf(fout, "(%s", _condToStr(cond->type));
        for (i = 0; i < cond->arg_size; ++i){
            fprintf(fout, " ");
            condPrint(pddl, a, cond->arg + i, fout);
        }
        fprintf(fout, ")");

    }else if (cond->type == PLAN_PDDL_COND_PRED){
        pred = pddl->predicate + cond->val;
        fprintf(fout, "(%s", pred->name);
        for (i = 0; i < cond->arg_size; ++i){
            fprintf(fout, " ");
            condPrint(pddl, a, cond->arg + i, fout);
        }
        fprintf(fout, ")");

    }else if (cond->type == PLAN_PDDL_COND_FUNC){
        pred = pddl->function + cond->val;
        fprintf(fout, "(%s", pred->name);
        for (i = 0; i < cond->arg_size; ++i){
            fprintf(fout, " ");
            condPrint(pddl, a, cond->arg + i, fout);
        }
        fprintf(fout, ")");

    }else if (cond->type == PLAN_PDDL_COND_VAR){
        if (a != NULL){
            fprintf(fout, "%s", a->param[cond->val].name);
        }else{
            fprintf(fout, "???var");
        }

    }else if (cond->type == PLAN_PDDL_COND_CONST){
        fprintf(fout, "%s", pddl->obj.obj[cond->val].name);

    }else if (cond->type == PLAN_PDDL_COND_INT){
        fprintf(fout, "%d", cond->val);
    }
}
/**** CONDITION END ****/
