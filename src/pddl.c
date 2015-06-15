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

static plan_lisp_node_t *findExp(plan_lisp_node_t *node, const char *cdr)
{
    bor_list_t *item, *it;
    plan_lisp_node_t *n, *m;

    BOR_LIST_FOR_EACH(&node->children, item){
        n = BOR_LIST_ENTRY(item, plan_lisp_node_t, list);
        if (n->value == NULL){
            it = borListNext(&n->children);
            m = BOR_LIST_ENTRY(it, plan_lisp_node_t, list);
            if (m->value != NULL && strcmp(m->value, cdr) == 0)
                return n;
        }
    }
    return NULL;
}

static int findTypedList(bor_list_t *from, bor_list_t *end,
                         bor_list_t **it_end, bor_list_t **next,
                         const char **type_name)
{
    plan_lisp_node_t *n;
    bor_list_t *item = from;
    int found = 0;

    *it_end = end;
    *type_name = NULL;

    while (item != end){
        n = BOR_LIST_ENTRY(item, plan_lisp_node_t, list);
        if (n->value == NULL){
            fprintf(stderr, "Error PDDL: Invalid definition of typed-list.\n");
            return -1;
        }

        if (found){
            *type_name = n->value;
            *next = borListNext(item);
            return 0;
        }

        if (strcmp(n->value, "-") == 0){
            *it_end = item;
            found = 1;
        }

        item = borListNext(item);
    }

    *next = *it_end;
    return 0;
}

static const char *parseName(plan_lisp_node_t *root)
{
    plan_lisp_node_t *n;
    bor_list_t *l;

    n = findExp(root, "domain");
    if (n == NULL)
        return NULL;

    l = borListNext(&n->children);
    l = borListNext(l);
    if (l == &n->children)
        return NULL;

    n = BOR_LIST_ENTRY(l, plan_lisp_node_t, list);
    return n->value;
}

static unsigned parseRequire(plan_lisp_node_t *root)
{
    plan_lisp_node_t *rnode, *n;
    bor_list_t *item;
    unsigned mask = 0u, m;

    rnode = findExp(root, ":requirements");
    // No :requirements implies :strips
    if (rnode == NULL)
        return PLAN_PDDL_REQUIRE_STRIPS;

    item = borListNext(&rnode->children);
    item = borListNext(item);
    while (item != &rnode->children){
        n = BOR_LIST_ENTRY(item, plan_lisp_node_t, list);

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

static int parseType(plan_pddl_domain_t *domain, plan_lisp_node_t *root)
{
    plan_lisp_node_t *n;
    bor_list_t *it, *it_end, *next;
    const char *name;
    int tid, pid;

    root = findExp(root, ":types");
    if (root == NULL)
        return -1;

    it = borListNext(&root->children);
    it = borListNext(it);
    while (1){
        if (findTypedList(it, &root->children, &it_end, &next, &name) != 0){
            fprintf(stderr, "Error PDDL: Invalid definition of :types\n");
            return -1;
        }

        if (it == it_end)
            break;

        pid = 0;
        if (name != NULL)
            pid = addType(domain, name);

        while (it != it_end){
            n = BOR_LIST_ENTRY(it, plan_lisp_node_t, list);
            if (n->value == NULL){
                fprintf(stderr, "Error PDDL: Invalid definition of :types\n");
                return -1;
            }

            tid = addType(domain, n->value);
            domain->type[tid].parent = pid;
            it = borListNext(it);
        }

        it = next;
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

static int parseConstant(plan_pddl_domain_t *domain, plan_lisp_node_t *root)
{
    plan_lisp_node_t *n;
    bor_list_t *it, *it_end, *next;
    const char *name;
    int pid;

    root = findExp(root, ":constants");
    if (root == NULL)
        return 0;

    it = borListNext(&root->children);
    it = borListNext(it);
    while (1){
        if (findTypedList(it, &root->children, &it_end, &next, &name) != 0){
            fprintf(stderr, "Error PDDL: Invalid definition of :constants\n");
            return -1;
        }

        if (it == &root->children)
            break;

        pid = 0;
        if (name != NULL)
            pid = getType(domain, name);

        while (it != it_end){
            n = BOR_LIST_ENTRY(it, plan_lisp_node_t, list);
            if (n->value == NULL){
                fprintf(stderr, "Error PDDL: Invalid definition of :constants\n");
                return -1;
            }

            addConstant(domain, n->value, pid);
            it = borListNext(it);
        }

        it = next;
    }

    return 0;
}

plan_pddl_domain_t *planPDDLDomainNew(const char *fn)
{
    plan_pddl_domain_t *domain;
    plan_lisp_file_t *lisp;

    lisp = planLispFileParse(fn);
    if (lisp == NULL){
        planLispFileDel(lisp);
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

    return domain;
}

void planPDDLDomainDel(plan_pddl_domain_t *domain)
{
    if (domain->lisp)
        planLispFileDel(domain->lisp);
    if (domain->type)
        BOR_FREE(domain->type);
    if (domain->constant)
        BOR_FREE(domain->constant);
    BOR_FREE(domain);
}

void planPDDLDomainDump(const plan_pddl_domain_t *domain, FILE *fout)
{
    int i;

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
}
