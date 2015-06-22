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

#include "pddl.h"

struct _require_mask_t {
    const char *name;
    unsigned mask;
};
typedef struct _require_mask_t require_mask_t;

static require_mask_t require_mask[] = {
    { ":strips", PLAN_PDDL_REQUIRE_STRIPS },
    { ":typing", PLAN_PDDL_REQUIRE_TYPING },
    { ":negative-preconditions", PLAN_PDDL_REQUIRE_NEGATIVE_PRE },
    { ":disjunctive-preconditions", PLAN_PDDL_REQUIRE_DISJUNCTIVE_PRE },
    { ":equality", PLAN_PDDL_REQUIRE_EQUALITY },
    { ":existential-preconditions", PLAN_PDDL_REQUIRE_EXISTENTIAL_PRE },
    { ":universal-preconditions", PLAN_PDDL_REQUIRE_UNIVERSAL_PRE },
    { ":conditional-effects", PLAN_PDDL_REQUIRE_CONDITIONAL_EFF },
    { ":numeric-fluents", PLAN_PDDL_REQUIRE_NUMERIC_FLUENT },
    { ":numeric-fluents", PLAN_PDDL_REQUIRE_OBJECT_FLUENT },
    { ":durative-actions", PLAN_PDDL_REQUIRE_DURATIVE_ACTION },
    { ":duration-inequalities", PLAN_PDDL_REQUIRE_DURATION_INEQUALITY },
    { ":continuous-effects", PLAN_PDDL_REQUIRE_CONTINUOUS_EFF },
    { ":derived-predicates", PLAN_PDDL_REQUIRE_DERIVED_PRED },
    { ":timed-initial-literals", PLAN_PDDL_REQUIRE_TIMED_INITIAL_LITERAL },
    { ":durative-actions", PLAN_PDDL_REQUIRE_DURATIVE_ACTION },
    { ":preferences", PLAN_PDDL_REQUIRE_PREFERENCE },
    { ":constraints", PLAN_PDDL_REQUIRE_CONSTRAINT },
    { ":action-costs", PLAN_PDDL_REQUIRE_ACTION_COST },
    { ":multi-agent", PLAN_PDDL_REQUIRE_MULTI_AGENT },
    { ":unfactored-privacy", PLAN_PDDL_REQUIRE_UNFACTORED_PRIVACY },
    { ":factored_privacy", PLAN_PDDL_REQUIRE_FACTORED_PRIVACY },

    { ":quantified-preconditions", PLAN_PDDL_REQUIRE_EXISTENTIAL_PRE |
                                   PLAN_PDDL_REQUIRE_UNIVERSAL_PRE },
    { ":fluents", PLAN_PDDL_REQUIRE_NUMERIC_FLUENT |
                  PLAN_PDDL_REQUIRE_OBJECT_FLUENT },
    { ":adl", PLAN_PDDL_REQUIRE_STRIPS |
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

unsigned requireMask(const char *require)
{
    int i;

    for (i = 0; i < require_mask_size; ++i){
        if (strcmp(require, require_mask[i].name) == 0)
            return require_mask[i].mask;
    }
    return 0u;
}

static plan_pddl_lisp_node_t *findExp(plan_pddl_lisp_node_t *node, int kw)
{
    bor_list_t *item, *it;
    plan_pddl_lisp_node_t *n, *m;

    BOR_LIST_FOR_EACH(&node->children, item){
        n = BOR_LIST_ENTRY(item, plan_pddl_lisp_node_t, list);
        if (n->value == NULL){
            it = borListNext(&n->children);
            m = BOR_LIST_ENTRY(it, plan_pddl_lisp_node_t, list);
            if (m->kw == kw)
                return n;
        }
    }
    return NULL;
}

typedef int (*parse_typed_list_add_fn)(plan_pddl_domain_t *dom, const char *name);
typedef int (*parse_typed_list_set_type_fn)(plan_pddl_domain_t *dom,
                                            int from, int to, const char *type);
static int parseTypedList(plan_pddl_domain_t *dom,
                          bor_list_t *list, bor_list_t *from,
                          parse_typed_list_add_fn add_fn,
                          parse_typed_list_set_type_fn set_type_fn)
{
    plan_pddl_lisp_node_t *n;
    bor_list_t *item = from;
    int itfrom, it, found_type;

    itfrom = 0;
    found_type = 0;
    for (it = 0; item != list; item = borListNext(item)){
        n = BOR_LIST_ENTRY(item, plan_pddl_lisp_node_t, list);
        if (n->value == NULL){
            fprintf(stderr, "Error PDDL: Invalid definition of typed-list.\n");
            return -1;
        }

        if (found_type){
            if (set_type_fn(dom, itfrom, it, n->value) != 0)
                return -1;
            found_type = 0;
            itfrom = it;

        }else if (strcmp(n->value, "-") == 0){
            found_type = 1;

        }else{
            if (add_fn(dom, n->value) != 0)
                return -1;
            ++it;
        }
    }

    return 0;
}

static const char *parseName(plan_pddl_lisp_node_t *root)
{
    plan_pddl_lisp_node_t *n;
    bor_list_t *l;

    n = findExp(root, PLAN_PDDL_KW_DOMAIN);
    if (n == NULL)
        return NULL;

    l = borListNext(&n->children);
    l = borListNext(l);
    if (l == &n->children)
        return NULL;

    n = BOR_LIST_ENTRY(l, plan_pddl_lisp_node_t, list);
    return n->value;
}

static unsigned parseRequire(plan_pddl_lisp_node_t *root)
{
    plan_pddl_lisp_node_t *rnode, *n;
    bor_list_t *item;
    unsigned mask = 0u, m;

    rnode = findExp(root, PLAN_PDDL_KW_REQUIREMENTS);
    // No :requirements implies :strips
    if (rnode == NULL)
        return PLAN_PDDL_REQUIRE_STRIPS;

    item = borListNext(&rnode->children);
    item = borListNext(item);
    while (item != &rnode->children){
        n = BOR_LIST_ENTRY(item, plan_pddl_lisp_node_t, list);

        if (n->value == NULL){
            fprintf(stderr, "Error PDDL: Invalid :requirements definition.\n");
            return 0u;
        }
        if ((m = requireMask(n->value)) == 0u){
            fprintf(stderr, "Error PDDL: Invalid :requirements definition:"
                            " Unknown keyword `%s'\n", n->value);
            return 0u;
        }

        mask |= m;
        item = borListNext(item);
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

static int parseTypeAdd(plan_pddl_domain_t *dom, const char *name)
{
    addType(dom, name);
    return 0;
}

static int parseTypeSetType(plan_pddl_domain_t *dom, int from, int to,
                            const char *name)
{
    int i, pid;

    pid = addType(dom, name);
    from++;
    to++;
    for (i = from; i < to; ++i)
        dom->type[i].parent = pid;
    return 0;
}

static int parseType(plan_pddl_domain_t *domain, plan_pddl_lisp_node_t *root)
{
    bor_list_t *it;

    root = findExp(root, PLAN_PDDL_KW_TYPES);
    if (root == NULL)
        return -1;

    it = borListNext(&root->children);
    it = borListNext(it);
    // TODO: Check circular dependency on types
    if (parseTypedList(domain, &root->children, it,
                       parseTypeAdd, parseTypeSetType) != 0){
        fprintf(stderr, "Error PDDL: Invalid definition of :types\n");
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

static int parseConstantAdd(plan_pddl_domain_t *dom, const char *name)
{
    addConstant(dom, name, 0);
    return 0;
}

static int parseConstantSet(plan_pddl_domain_t *dom, int from, int to,
                            const char *name)
{
    int i, tid;

    tid = getType(dom, name);
    if (tid < 0){
        fprintf(stderr, "Error PDDL: Invalid type `%s'\n", name);
        return -1;
    }

    for (i = from; i < to; ++i)
        dom->constant[i].type = tid;

    return 0;
}

static int parseConstant(plan_pddl_domain_t *domain, plan_pddl_lisp_node_t *root)
{
    bor_list_t *it;

    root = findExp(root, PLAN_PDDL_KW_CONSTANTS);
    if (root == NULL)
        return 0;

    it = borListNext(&root->children);
    it = borListNext(it);
    if (parseTypedList(domain, &root->children, it,
                       parseConstantAdd, parseConstantSet) != 0){
        fprintf(stderr, "Error PDDL: Invalid definition of :constants\n");
        return -1;
    }
    return 0;
}

static int addPredicate(plan_pddl_domain_t *domain, const char *name)
{
    int i, id;

    for (i = 0; i < domain->predicate_size; ++i){
        if (strcmp(domain->predicate[i].name, name) == 0){
            fprintf(stderr, "Error PDDL: Duplicate predicate `%s'\n", name);
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

static int parsePredicateSet(plan_pddl_domain_t *dom, int from, int to,
                             const char *name)
{
    plan_pddl_predicate_t *pred;
    int i, tid;

    tid = getType(dom, name);
    if (tid < 0){
        fprintf(stderr, "Error PDDL: Invalid type `%s'\n", name);
        return -1;
    }

    pred = dom->predicate + dom->predicate_size - 1;
    for (i = from; i < to; ++i)
        pred->param[i] = tid;
    return 0;
}

static int parsePredicate1(plan_pddl_domain_t *domain, plan_pddl_lisp_node_t *root)
{
    plan_pddl_lisp_node_t *n;
    bor_list_t *it;
    int id;

    it = borListNext(&root->children);
    if (it == &root->children){
        fprintf(stderr, "Error PDDL: Invalid predicate definition.\n");
        return -1;
    }

    n = BOR_LIST_ENTRY(it, plan_pddl_lisp_node_t, list);
    if (n->value == NULL){
        fprintf(stderr, "Error PDDL: Invalid predicate definition.\n");
        return -1;
    }

    id = addPredicate(domain, n->value);
    if (id < 0)
        return -1;

    it = borListNext(it);
    if (parseTypedList(domain, &root->children, it,
                       parsePredicateAdd, parsePredicateSet) != 0){
        fprintf(stderr, "Error PDDL: Invalid definition of predicate `%s'\n",
                        domain->predicate[id].name);
        return -1;
    }
    return 0;
}

static int parsePredicate(plan_pddl_domain_t *domain, plan_pddl_lisp_node_t *root)
{
    plan_pddl_lisp_node_t *n;
    bor_list_t *it;

    root = findExp(root, PLAN_PDDL_KW_PREDICATES);
    if (root == NULL)
        return 0;

    it = borListNext(&root->children);
    it = borListNext(it);
    for (; it != &root->children; it = borListNext(it)){
        n = BOR_LIST_ENTRY(it, plan_pddl_lisp_node_t, list);
        if (parsePredicate1(domain, n) != 0)
            return -1;
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
    domain->name = parseName(lisp->root);
    if (domain->name == NULL){
        fprintf(stderr, "Error PDDL: Could not find name of the domain.\n");
        planPDDLDomainDel(domain);
        return NULL;
    }

    domain->require = parseRequire(lisp->root);
    if (domain->require == 0u){
        planPDDLDomainDel(domain);
        return NULL;
    }

    domain->type_size = 1;
    domain->type = BOR_ALLOC_ARR(plan_pddl_type_t, 1);
    domain->type[0].name = _object_type_name;
    domain->type[0].parent = -1;
    if (domain->require & PLAN_PDDL_REQUIRE_TYPING){
        if (parseType(domain, lisp->root) != 0){
            planPDDLDomainDel(domain);
            return NULL;
        }
    }

    if (parseConstant(domain, lisp->root) != 0){
        planPDDLDomainDel(domain);
        return NULL;
    }

    if (parsePredicate(domain, lisp->root) != 0){
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
}
