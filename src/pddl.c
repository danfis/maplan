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

#define ERR(format, ...) do { \
    fprintf(stderr, "Error PDDL: " format "\n", __VA_ARGS__); \
    fflush(stderr); \
    } while (0)
#define ERR2(text) do { \
    fprintf(stderr, "Error PDDL: " text "\n"); \
    fflush(stderr); \
    } while (0)

#define ERRN(NODE, format, ...) do { \
    fprintf(stderr, "Error PDDL: Line %d: " format "\n", \
            (NODE)->lineno, __VA_ARGS__); \
    fflush(stderr); \
    } while (0)
#define ERRN2(NODE, text) do { \
    fprintf(stderr, "Error PDDL: Line %d: " text "\n", \
            (NODE)->lineno); \
    fflush(stderr); \
    } while (0)

struct _require_mask_t {
    int kw;
    unsigned mask;
};
typedef struct _require_mask_t require_mask_t;

static require_mask_t require_mask[] = {
    { PLAN_PDDL_KW_STRIPS, PLAN_PDDL_REQUIRE_STRIPS },
    { PLAN_PDDL_KW_TYPING, PLAN_PDDL_REQUIRE_TYPING },
    { PLAN_PDDL_KW_NEGATIVE_PRE, PLAN_PDDL_REQUIRE_NEGATIVE_PRE },
    { PLAN_PDDL_KW_DISJUNCTIVE_PRE, PLAN_PDDL_REQUIRE_DISJUNCTIVE_PRE },
    { PLAN_PDDL_KW_EQUALITY, PLAN_PDDL_REQUIRE_EQUALITY },
    { PLAN_PDDL_KW_EXISTENTIAL_PRE, PLAN_PDDL_REQUIRE_EXISTENTIAL_PRE },
    { PLAN_PDDL_KW_UNIVERSAL_PRE, PLAN_PDDL_REQUIRE_UNIVERSAL_PRE },
    { PLAN_PDDL_KW_CONDITIONAL_EFF, PLAN_PDDL_REQUIRE_CONDITIONAL_EFF },
    { PLAN_PDDL_KW_NUMERIC_FLUENT, PLAN_PDDL_REQUIRE_NUMERIC_FLUENT },
    { PLAN_PDDL_KW_OBJECT_FLUENT, PLAN_PDDL_REQUIRE_OBJECT_FLUENT },
    { PLAN_PDDL_KW_DURATIVE_ACTION, PLAN_PDDL_REQUIRE_DURATIVE_ACTION },
    { PLAN_PDDL_KW_DURATION_INEQUALITY, PLAN_PDDL_REQUIRE_DURATION_INEQUALITY },
    { PLAN_PDDL_KW_CONTINUOUS_EFF, PLAN_PDDL_REQUIRE_CONTINUOUS_EFF },
    { PLAN_PDDL_KW_DERIVED_PRED, PLAN_PDDL_REQUIRE_DERIVED_PRED },
    { PLAN_PDDL_KW_TIMED_INITIAL_LITERAL, PLAN_PDDL_REQUIRE_TIMED_INITIAL_LITERAL },
    { PLAN_PDDL_KW_DURATIVE_ACTION, PLAN_PDDL_REQUIRE_DURATIVE_ACTION },
    { PLAN_PDDL_KW_PREFERENCE, PLAN_PDDL_REQUIRE_PREFERENCE },
    { PLAN_PDDL_KW_CONSTRAINT, PLAN_PDDL_REQUIRE_CONSTRAINT },
    { PLAN_PDDL_KW_ACTION_COST, PLAN_PDDL_REQUIRE_ACTION_COST },
    { PLAN_PDDL_KW_MULTI_AGENT, PLAN_PDDL_REQUIRE_MULTI_AGENT },
    { PLAN_PDDL_KW_UNFACTORED_PRIVACY, PLAN_PDDL_REQUIRE_UNFACTORED_PRIVACY },
    { PLAN_PDDL_KW_FACTORED_PRIVACY, PLAN_PDDL_REQUIRE_FACTORED_PRIVACY },

    { PLAN_PDDL_KW_QUANTIFIED_PRE, PLAN_PDDL_REQUIRE_EXISTENTIAL_PRE |
                                   PLAN_PDDL_REQUIRE_UNIVERSAL_PRE },
    { PLAN_PDDL_KW_FLUENTS, PLAN_PDDL_REQUIRE_NUMERIC_FLUENT |
                            PLAN_PDDL_REQUIRE_OBJECT_FLUENT },
    { PLAN_PDDL_KW_ADL, PLAN_PDDL_REQUIRE_STRIPS |
                        PLAN_PDDL_REQUIRE_TYPING |
                        PLAN_PDDL_REQUIRE_NEGATIVE_PRE |
                        PLAN_PDDL_REQUIRE_DISJUNCTIVE_PRE |
                        PLAN_PDDL_REQUIRE_EQUALITY |
                        PLAN_PDDL_REQUIRE_EXISTENTIAL_PRE |
                        PLAN_PDDL_REQUIRE_UNIVERSAL_PRE |
                        PLAN_PDDL_REQUIRE_CONDITIONAL_EFF },
};
static int require_mask_size = sizeof(require_mask) / sizeof(require_mask_t);

static char _object_type_name[] = "object";

unsigned requireMask(int kw)
{
    int i;

    for (i = 0; i < require_mask_size; ++i){
        if (require_mask[i].kw == kw)
            return require_mask[i].mask;
    }
    return 0u;
}

static int nodeHeadKw(const plan_pddl_lisp_node_t *n)
{
    if (n->child_size == 0)
        return -1;
    return n->child[0].kw;
}

static plan_pddl_lisp_node_t *findNode(plan_pddl_lisp_node_t *node, int kw)
{
    int i;

    for (i = 0; i < node->child_size; ++i){
        if (nodeHeadKw(node->child + i) == kw)
            return node->child + i;
    }
    return NULL;
}

typedef int (*parse_typed_list_set_fn)(plan_pddl_domain_t *dom,
                                       plan_pddl_lisp_node_t *root,
                                       int child_from, int child_to,
                                       int child_type);
static int parseTypedList(plan_pddl_domain_t *dom,
                          plan_pddl_lisp_node_t *root,
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
            if (set_fn(dom, root, itfrom, itto, ittype) != 0)
                return -1;
            itfrom = i + 1;
            itto = i + 1;
            ittype = -1;

        }else{
            ++itto;
        }
    }

    if (itfrom < itto){
        if (set_fn(dom, root, itfrom, itto, -1) != 0)
            return -1;
    }

    return 0;
}

static const char *parseName(plan_pddl_lisp_node_t *root)
{
    plan_pddl_lisp_node_t *n;

    n = findNode(root, PLAN_PDDL_KW_DOMAIN);
    if (n == NULL){
        ERR2("Could not find domain name definition");
        return NULL;
    }

    if (n->child_size != 2 || n->child[1].value == NULL){
        ERRN2(n, "Invalid domain name definition");
        return NULL;
    }

    return n->child[1].value;
}

static unsigned parseRequire(plan_pddl_lisp_node_t *root)
{
    plan_pddl_lisp_node_t *rnode, *n;
    int i;
    unsigned mask = 0u, m;

    rnode = findNode(root, PLAN_PDDL_KW_REQUIREMENTS);
    // No :requirements implies :strips
    if (rnode == NULL)
        return PLAN_PDDL_REQUIRE_STRIPS;

    for (i = 1; i < rnode->child_size; ++i){
        n = rnode->child + i;
        if (n->value == NULL){
            ERRN2(n, "Invalid :requirements definition.");
            return 0u;
        }
        if ((m = requireMask(n->kw)) == 0u){
            ERRN(n, "Invalid :requirements definition: Unkown keyword `%s'.",
                 n->value);
            return 0u;
        }

        mask |= m;
    }

    return mask;
}

static int getType(plan_pddl_domain_t *domain, const char *name)
{
    int i;

    for (i = 0; i < domain->type_size; ++i){
        if (strcmp(domain->type[i].name, name) == 0)
            return i;
    }

    return -1;
}

static int addType(plan_pddl_domain_t *domain, const char *name)
{
    int id;

    if ((id = getType(domain, name)) != -1)
        return id;

    ++domain->type_size;
    domain->type = BOR_REALLOC_ARR(domain->type, plan_pddl_type_t,
                                   domain->type_size);
    domain->type[domain->type_size - 1].name = name;
    domain->type[domain->type_size - 1].parent = 0;
    return domain->type_size - 1;
}

static int parseTypeSet(plan_pddl_domain_t *dom,
                        plan_pddl_lisp_node_t *root,
                        int child_from, int child_to, int child_type)
{
    int i, tid, pid;

    pid = 0;
    if (child_type >= 0){
        if (root->child[child_type].value == NULL){
            ERRN2(root->child + child_type, "Invalid type definition.");
            return -1;
        }
        pid = addType(dom, root->child[child_type].value);
    }

    for (i = child_from; i < child_to; ++i){
        if (root->child[i].value == NULL){
            ERRN2(root->child + i, "Invalid type definition.");
            return -1;
        }

        tid = addType(dom, root->child[i].value);
        dom->type[tid].parent = pid;
    }

    return 0;
}

static int parseType(plan_pddl_domain_t *dom, plan_pddl_lisp_node_t *root)
{
    plan_pddl_lisp_node_t *types;

    types = findNode(root, PLAN_PDDL_KW_TYPES);
    if (types == NULL)
        return 0;

    // TODO: Check circular dependency on types
    if (parseTypedList(dom, types, 1, types->child_size, parseTypeSet) != 0){
        ERRN2(root, "Invalid definition of :types");
        return -1;
    }
    return 0;
}

static void addConstant(plan_pddl_domain_t *dom, const char *name, int type)
{
    plan_pddl_obj_t *c;

    ++dom->constant_size;
    dom->constant = BOR_REALLOC_ARR(dom->constant, plan_pddl_obj_t,
                                    dom->constant_size);
    c = dom->constant + dom->constant_size - 1;
    c->name = name;
    c->type = type;
}

static int parseConstantSet(plan_pddl_domain_t *dom,
                            plan_pddl_lisp_node_t *root,
                            int child_from, int child_to, int child_type)
{
    int i, tid;

    tid = 0;
    if (child_type >= 0){
        tid = getType(dom, root->child[child_type].value);
        if (tid < 0){
            ERRN(root->child + child_type, "Invalid type `%s'",
                 root->child[child_type].value);
            return -1;
        }
    }

    for (i = child_from; i < child_to; ++i)
        addConstant(dom, root->child[i].value, tid);

    return 0;
}

static int parseConstant(plan_pddl_domain_t *domain, plan_pddl_lisp_node_t *root)
{
    plan_pddl_lisp_node_t *n;

    n = findNode(root, PLAN_PDDL_KW_CONSTANTS);
    if (n == NULL)
        return 0;

    if (parseTypedList(domain, n, 1, n->child_size, parseConstantSet) != 0){
        ERRN2(n, "Invalid definition of :constants.");
        return -1;
    }

    return 0;
}

static int addPredicate(plan_pddl_domain_t *domain, const char *name)
{
    int i, id;

    for (i = 0; i < domain->predicate_size; ++i){
        if (strcmp(domain->predicate[i].name, name) == 0){
            ERR("Duplicate predicate `%s'", name);
            return -1;
        }
    }

    ++domain->predicate_size;
    domain->predicate = BOR_REALLOC_ARR(domain->predicate,
                                        plan_pddl_predicate_t,
                                        domain->predicate_size);
    id = domain->predicate_size - 1;
    domain->predicate[id].name = name;
    domain->predicate[id].param = NULL;
    domain->predicate[id].param_size = 0;
    return id;
}

static int parsePredicateAdd(plan_pddl_domain_t *dom, const char *name)
{
    plan_pddl_predicate_t *pred;
    pred = dom->predicate + dom->predicate_size - 1;
    ++pred->param_size;
    pred->param = BOR_REALLOC_ARR(pred->param, int, pred->param_size);
    pred->param[pred->param_size - 1] = 0;
    return 0;
}

static int parsePredicateSet(plan_pddl_domain_t *dom,
                             plan_pddl_lisp_node_t *root,
                             int child_from, int child_to, int child_type)
{
    plan_pddl_predicate_t *pred;
    int i, j, tid;

    tid = 0;
    if (child_type >= 0){
        tid = getType(dom, root->child[child_type].value);
        if (tid < 0){
            ERRN(root->child + child_type, "Invalid type `%s'",
                 root->child[child_type].value);
            return -1;
        }
    }

    pred = dom->predicate + dom->predicate_size - 1;
    j = pred->param_size;
    pred->param_size += child_to - child_from;
    pred->param = BOR_REALLOC_ARR(pred->param, int, pred->param_size);
    for (i = child_from; i < child_to; ++i, ++j)
        pred->param[j] = tid;
    return 0;
}

static int parsePredicate1(plan_pddl_domain_t *domain, plan_pddl_lisp_node_t *root)
{
    if (root->child_size < 1 || root->child[0].value == NULL){
        ERRN2(root, "Invalid definition of predicate.");
        return -1;
    }

    if (addPredicate(domain, root->child[0].value) < 0)
        return -1;

    if (parseTypedList(domain, root, 1, root->child_size, parsePredicateSet) != 0)
        return -1;
    return 0;
}

static int parsePredicate(plan_pddl_domain_t *domain, plan_pddl_lisp_node_t *root)
{
    plan_pddl_lisp_node_t *n;
    int i;

    n = findNode(root, PLAN_PDDL_KW_PREDICATES);
    if (n == NULL)
        return 0;

    for (i = 1; i < n->child_size; ++i){
        if (parsePredicate1(domain, n->child + i) != 0)
            return -1;
    }
    return 0;
}

static int addAction(plan_pddl_domain_t *dom, const char *name)
{
    int id;

    id = dom->action_size;
    ++dom->action_size;
    dom->action = BOR_REALLOC_ARR(dom->action, plan_pddl_action_t,
                                  dom->action_size);
    dom->action[id].name = name;
    dom->action[id].param = NULL;
    dom->action[id].param_size = 0;

    return id;
}

static int parseActionParamSet(plan_pddl_domain_t *dom,
                               plan_pddl_lisp_node_t *root,
                               int child_from, int child_to, int child_type)
{
    int id = dom->action_size - 1;
    plan_pddl_action_t *a = dom->action + id;
    int i, j, tid;

    tid = 0;
    if (child_type >= 0){
        tid = getType(dom, root->child[child_type].value);
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

static int parseAction1(plan_pddl_domain_t *domain, plan_pddl_lisp_node_t *root)
{
    if (root->child_size < 4
            || root->child[1].value == NULL
            || root->child[2].kw != PLAN_PDDL_KW_PARAMETERS
            || root->child[3].value != NULL){
        ERRN2(root, "Invalid definition of :action");
        return -1;
    }

    addAction(domain, root->child[1].value);
    if (parseTypedList(domain, root->child + 3, 0,
                       root->child[3].child_size, parseActionParamSet) != 0)
        return -1;
    return 0;
}

static int parseAction(plan_pddl_domain_t *domain, plan_pddl_lisp_node_t *root)
{
    int i;

    for (i = 0; i < root->child_size; ++i){
        if (nodeHeadKw(root->child + i) == PLAN_PDDL_KW_ACTION){
            if (parseAction1(domain, root->child + i) != 0)
                return -1;
        }
    }
    return 0;
}

plan_pddl_domain_t *planPDDLDomainNew(const char *fn)
{
    plan_pddl_domain_t *domain;
    plan_pddl_lisp_t *lisp;

    lisp = planPDDLLispParse(fn);
    if (lisp == NULL){
        planPDDLLispDel(lisp);
        return NULL;
    }

    domain = BOR_ALLOC(plan_pddl_domain_t);
    bzero(domain, sizeof(*domain));
    domain->lisp = lisp;
    domain->name = parseName(&lisp->root);
    if (domain->name == NULL){
        fprintf(stderr, "Error PDDL: Could not find name of the domain.\n");
        planPDDLDomainDel(domain);
        return NULL;
    }

    domain->require = parseRequire(&lisp->root);
    if (domain->require == 0u){
        planPDDLDomainDel(domain);
        return NULL;
    }

    domain->type_size = 1;
    domain->type = BOR_ALLOC_ARR(plan_pddl_type_t, 1);
    domain->type[0].name = _object_type_name;
    domain->type[0].parent = -1;
    if (parseType(domain, &lisp->root) != 0){
        planPDDLDomainDel(domain);
        return NULL;
    }

    if (parseConstant(domain, &lisp->root) != 0){
        planPDDLDomainDel(domain);
        return NULL;
    }

    if (parsePredicate(domain, &lisp->root) != 0){
        planPDDLDomainDel(domain);
        return NULL;
    }

    if (parseAction(domain, &lisp->root) != 0){
        planPDDLDomainDel(domain);
        return NULL;
    }

    return domain;
}

void planPDDLDomainDel(plan_pddl_domain_t *domain)
{
    int i;

    if (domain->lisp)
        planPDDLLispDel(domain->lisp);
    if (domain->type)
        BOR_FREE(domain->type);
    if (domain->constant)
        BOR_FREE(domain->constant);

    for (i = 0; i < domain->predicate_size; ++i){
        if (domain->predicate[i].param != NULL)
            BOR_FREE(domain->predicate[i].param);
    }
    if (domain->predicate)
        BOR_FREE(domain->predicate);

    for (i = 0; i < domain->action_size; ++i){
        if (domain->action[i].param != NULL)
            BOR_FREE(domain->action[i].param);
    }
    if (domain->action)
        BOR_FREE(domain->action);

    BOR_FREE(domain);
}

void planPDDLDomainDump(const plan_pddl_domain_t *domain, FILE *fout)
{
    int i, j;

    fprintf(fout, "Domain: %s\n", domain->name);
    fprintf(fout, "Require: %x\n", domain->require);
    fprintf(fout, "Type[%d]:\n", domain->type_size);
    for (i = 0; i < domain->type_size; ++i){
        fprintf(fout, "    [%d]: %s, parent: %d\n", i,
                domain->type[i].name, domain->type[i].parent);
    }
    fprintf(fout, "Constant[%d]:\n", domain->constant_size);
    for (i = 0; i < domain->constant_size; ++i){
        fprintf(fout, "    [%d]: %s, type: %d\n", i,
                domain->constant[i].name, domain->constant[i].type);
    }

    fprintf(fout, "Predicate[%d]:\n", domain->predicate_size);
    for (i = 0; i < domain->predicate_size; ++i){
        fprintf(fout, "    %s:", domain->predicate[i].name);
        for (j = 0; j < domain->predicate[i].param_size; ++j){
            fprintf(fout, " %d", domain->predicate[i].param[j]);
        }
        fprintf(fout, "\n");
    }

    fprintf(fout, "Action[%d]:\n", domain->action_size);
    for (i = 0; i < domain->action_size; ++i){
        fprintf(fout, "    %s:", domain->action[i].name);
        for (j = 0; j < domain->action[i].param_size; ++j){
            fprintf(fout, " %s:%d", domain->action[i].param[j].name,
                    domain->action[i].param[j].type);
        }
        fprintf(fout, "\n");
    }
}
