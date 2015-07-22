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
    pddl->domain_lisp = domain_lisp;
    pddl->problem_lisp = problem_lisp;
    pddl->domain_name = parseDomainName(&domain_lisp->root);
    if (pddl->domain_name == NULL)
        goto pddl_fail;

    pddl->problem_name = parseProblemName(&problem_lisp->root);
    if (pddl->domain_name == NULL)
        goto pddl_fail;


    if (checkDomainName(pddl) != 0
            || planPDDLRequireParse(domain_lisp, &pddl->require) != 0
            || planPDDLTypesParse(domain_lisp, &pddl->type) != 0
            || planPDDLObjsParse(domain_lisp, problem_lisp,
                                 &pddl->type, pddl->require, &pddl->obj) != 0
            || planPDDLTypeObjInit(&pddl->type_obj, &pddl->type, &pddl->obj) != 0
            || planPDDLPredicatesParse(domain_lisp, pddl->require,
                                       &pddl->type, &pddl->predicate) != 0
            || planPDDLFunctionsParse(domain_lisp, &pddl->type,
                                      &pddl->function) != 0
            || planPDDLFactsParseInit(problem_lisp, &pddl->predicate,
                                      &pddl->function, &pddl->obj,
                                      &pddl->init_fact, &pddl->init_func) != 0
            || planPDDLFactsParseGoal(problem_lisp, &pddl->predicate,
                                      &pddl->obj, &pddl->goal) != 0
            || planPDDLActionsParse(domain_lisp, &pddl->type, &pddl->obj,
                                    &pddl->type_obj, &pddl->predicate,
                                    &pddl->function, pddl->require,
                                    &pddl->action) != 0
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
    if (pddl->domain_lisp)
        planPDDLLispDel(pddl->domain_lisp);
    if (pddl->problem_lisp)
        planPDDLLispDel(pddl->problem_lisp);
    planPDDLTypesFree(&pddl->type);
    planPDDLObjsFree(&pddl->obj);
    planPDDLTypeObjFree(&pddl->type_obj);
    planPDDLPredicatesFree(&pddl->predicate);
    planPDDLPredicatesFree(&pddl->function);
    planPDDLFactsFree(&pddl->init_fact);
    planPDDLFactsFree(&pddl->init_func);
    planPDDLFactsFree(&pddl->goal);
    planPDDLActionsFree(&pddl->action);

    BOR_FREE(pddl);
}


void planPDDLDump(const plan_pddl_t *pddl, FILE *fout)
{
    fprintf(fout, "Domain: %s\n", pddl->domain_name);
    fprintf(fout, "Problem: %s\n", pddl->problem_name);
    fprintf(fout, "Require: %x\n", pddl->require);
    planPDDLTypesPrint(&pddl->type, fout);
    planPDDLObjsPrint(&pddl->obj, fout);
    planPDDLTypeObjPrint(&pddl->type_obj, fout);
    planPDDLPredicatesPrint(&pddl->predicate, "Predicate", fout);
    planPDDLPredicatesPrint(&pddl->function, "Function", fout);
    planPDDLActionsPrint(&pddl->action, &pddl->obj, &pddl->predicate,
                         &pddl->function, fout);

    planPDDLFactsPrintGoal(&pddl->predicate, &pddl->obj, &pddl->goal, fout);
    planPDDLFactsPrintInit(&pddl->predicate, &pddl->obj, &pddl->init_fact, fout);
    planPDDLFactsPrintInitFunc(&pddl->function, &pddl->obj, &pddl->init_func, fout);
    fprintf(fout, "Metric: %d\n", pddl->metric);
}
