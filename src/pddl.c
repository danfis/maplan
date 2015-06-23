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

#define WARN(format, ...) do { \
    fprintf(stderr, "Warning PDDL: " format "\n", __VA_ARGS__); \
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


static char _object_type_name[] = "object";

static unsigned requireMask(int kw)
{
    int i;

    for (i = 0; i < require_mask_size; ++i){
        if (require_mask[i].kw == kw)
            return require_mask[i].mask;
    }
    return 0u;
}

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

static int nodeHeadKw(const plan_pddl_lisp_node_t *n)
{
    if (n->child_size == 0)
        return -1;
    return n->child[0].kw;
}

static const char *nodeHead(const plan_pddl_lisp_node_t *n)
{
    if (n->child_size == 0)
        return NULL;
    return n->child[0].value;
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

typedef int (*parse_typed_list_set_fn)(plan_pddl_t *pddl,
                                       plan_pddl_lisp_node_t *root,
                                       int child_from, int child_to,
                                       int child_type);
static int parseTypedList(plan_pddl_t *pddl,
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
    plan_pddl_lisp_node_t *n;

    n = findNode(root, kw);
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

static int getType(plan_pddl_t *pddl, const char *name)
{
    int i;

    for (i = 0; i < pddl->type_size; ++i){
        if (strcmp(pddl->type[i].name, name) == 0)
            return i;
    }

    return -1;
}

static int addType(plan_pddl_t *pddl, const char *name)
{
    int id;

    if ((id = getType(pddl, name)) != -1)
        return id;

    ++pddl->type_size;
    pddl->type = BOR_REALLOC_ARR(pddl->type, plan_pddl_type_t,
                                 pddl->type_size);
    pddl->type[pddl->type_size - 1].name = name;
    pddl->type[pddl->type_size - 1].parent = 0;
    return pddl->type_size - 1;
}

static int parseTypeSet(plan_pddl_t *pddl,
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
        pid = addType(pddl, root->child[child_type].value);
    }

    for (i = child_from; i < child_to; ++i){
        if (root->child[i].value == NULL){
            ERRN2(root->child + i, "Invalid type definition.");
            return -1;
        }

        tid = addType(pddl, root->child[i].value);
        pddl->type[tid].parent = pid;
    }

    return 0;
}

static int parseType(plan_pddl_t *pddl, plan_pddl_lisp_node_t *root)
{
    plan_pddl_lisp_node_t *types;

    types = findNode(root, PLAN_PDDL_KW_TYPES);
    if (types == NULL)
        return 0;

    // TODO: Check circular dependency on types
    if (parseTypedList(pddl, types, 1, types->child_size, parseTypeSet) != 0){
        ERRN2(root, "Invalid definition of :types");
        return -1;
    }
    return 0;
}

static plan_pddl_obj_t *addObj(plan_pddl_t *pddl, const char *name, int type)
{
    plan_pddl_obj_t *o;

    ++pddl->obj_size;
    pddl->obj = BOR_REALLOC_ARR(pddl->obj, plan_pddl_obj_t, pddl->obj_size);
    o = pddl->obj + pddl->obj_size - 1;
    o->name = name;
    o->type = type;
    o->is_constant = 0;
    return o;
}

static int getConst(const plan_pddl_t *pddl, const char *name)
{
    int i;

    for (i = 0; i < pddl->obj_size; ++i){
        if (pddl->obj[i].is_constant
                && strcmp(pddl->obj[i].name, name) == 0)
            return i;
    }
    return -1;
}

static int getObj(const plan_pddl_t *pddl, const char *name)
{
    int i;

    for (i = 0; i < pddl->obj_size; ++i){
        if (strcmp(pddl->obj[i].name, name) == 0)
            return i;
    }
    return -1;
}

static int parseConstantSet(plan_pddl_t *pddl,
                            plan_pddl_lisp_node_t *root,
                            int child_from, int child_to, int child_type)
{
    plan_pddl_obj_t *o;
    int i, tid;

    tid = 0;
    if (child_type >= 0){
        tid = getType(pddl, root->child[child_type].value);
        if (tid < 0){
            ERRN(root->child + child_type, "Invalid type `%s'",
                 root->child[child_type].value);
            return -1;
        }
    }

    for (i = child_from; i < child_to; ++i){
        o = addObj(pddl, root->child[i].value, tid);
        o->is_constant = 1;
    }

    return 0;
}

static int parseConstant(plan_pddl_t *pddl, plan_pddl_lisp_node_t *root)
{
    plan_pddl_lisp_node_t *n;

    n = findNode(root, PLAN_PDDL_KW_CONSTANTS);
    if (n == NULL)
        return 0;

    if (parseTypedList(pddl, n, 1, n->child_size, parseConstantSet) != 0){
        ERRN2(n, "Invalid definition of :constants.");
        return -1;
    }

    return 0;
}

static int parseObjSet(plan_pddl_t *pddl,
                       plan_pddl_lisp_node_t *root,
                       int child_from, int child_to, int child_type)
{
    int i, tid;

    tid = 0;
    if (child_type >= 0){
        tid = getType(pddl, root->child[child_type].value);
        if (tid < 0){
            ERRN(root->child + child_type, "Invalid type `%s'",
                 root->child[child_type].value);
            return -1;
        }
    }

    for (i = child_from; i < child_to; ++i)
        addObj(pddl, root->child[i].value, tid);

    return 0;
}

static int parseObj(plan_pddl_t *pddl, plan_pddl_lisp_node_t *root)
{
    plan_pddl_lisp_node_t *n;

    n = findNode(root, PLAN_PDDL_KW_OBJECTS);
    if (n == NULL)
        return 0;

    if (parseTypedList(pddl, n, 1, n->child_size, parseObjSet) != 0){
        ERRN2(n, "Invalid definition of :objects.");
        return -1;
    }

    return 0;
}

static void _makeTypeObjMapRec(plan_pddl_t *pddl, int arr_id, int type_id)
{
    plan_pddl_obj_arr_t *m = pddl->type_obj_map + arr_id;
    int i;

    for (i = 0; i < pddl->obj_size; ++i){
        if (pddl->obj[i].type == type_id)
            m->obj[i] = 1;
    }

    for (i = 0; i < pddl->type_size; ++i){
        if (pddl->type[i].parent == type_id)
            _makeTypeObjMapRec(pddl, arr_id, i);
    }
}

static void _makeTypeObjMap(plan_pddl_t *pddl, int type_id)
{
    plan_pddl_obj_arr_t *m = pddl->type_obj_map + type_id;
    int i, ins;

    m->size = 0;
    m->obj = BOR_CALLOC_ARR(int, pddl->obj_size);
    _makeTypeObjMapRec(pddl, type_id, type_id);

    for (ins = 0, i = 0; i < pddl->obj_size; ++i){
        if (m->obj[i])
            m->obj[ins++] = i;
    }
    m->size = ins;
    if (m->size != pddl->obj_size)
        m->obj = BOR_REALLOC_ARR(m->obj, int, m->size);
}

static int makeTypeObjMap(plan_pddl_t *pddl)
{
    int i;

    pddl->type_obj_map = BOR_CALLOC_ARR(plan_pddl_obj_arr_t,
                                        pddl->type_size);
    for (i = 0; i < pddl->type_size; ++i){
        _makeTypeObjMap(pddl, i);
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
    pddl->predicate[id].name = name;
    pddl->predicate[id].param = NULL;
    pddl->predicate[id].param_size = 0;
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
}

static int parsePredicateAdd(plan_pddl_t *pddl, const char *name)
{
    plan_pddl_predicate_t *pred;
    pred = pddl->predicate + pddl->predicate_size - 1;
    ++pred->param_size;
    pred->param = BOR_REALLOC_ARR(pred->param, int, pred->param_size);
    pred->param[pred->param_size - 1] = 0;
    return 0;
}

static int parsePredicateSet(plan_pddl_t *pddl,
                             plan_pddl_lisp_node_t *root,
                             int child_from, int child_to, int child_type)
{
    plan_pddl_predicate_t *pred;
    int i, j, tid;

    tid = 0;
    if (child_type >= 0){
        tid = getType(pddl, root->child[child_type].value);
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

static int parsePredicate1(plan_pddl_t *pddl, plan_pddl_lisp_node_t *root)
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

static int parsePredicate(plan_pddl_t *pddl, plan_pddl_lisp_node_t *root)
{
    plan_pddl_lisp_node_t *n;
    int i;

    n = findNode(root, PLAN_PDDL_KW_PREDICATES);
    if (n == NULL)
        return 0;

    for (i = 1; i < n->child_size; ++i){
        if (parsePredicate1(pddl, n->child + i) != 0)
            return -1;
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
    pddl->action[id].name = name;
    pddl->action[id].param = NULL;
    pddl->action[id].param_size = 0;
    condInit(&pddl->action[id].pre);
    condInit(&pddl->action[id].eff);

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
                               plan_pddl_lisp_node_t *root,
                               int child_from, int child_to, int child_type)
{
    int id = pddl->action_size - 1;
    plan_pddl_action_t *a = pddl->action + id;
    int i, j, tid;

    tid = 0;
    if (child_type >= 0){
        tid = getType(pddl, root->child[child_type].value);
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

static int _parseVarConst(plan_pddl_t *pddl, plan_pddl_lisp_node_t *root,
                           int action_id, plan_pddl_cond_t *cond)
{
    if (root->value[0] == '?'){
        cond->type = PLAN_PDDL_COND_VAR;
        cond->val = getActionParam(pddl, action_id, root->value);
        if (cond->val == -1){
            ERRN(root, "Invalid paramenter name `%s'", root->value);
            return -1;
        }

    }else{
        cond->type = PLAN_PDDL_COND_CONST;
        cond->val = getObj(pddl, root->value);
        if (cond->val == -1){
            ERRN(root, "Invalid object `%s'", root->value);
            return -1;
        }
    }

    return 0;
}

static int _parseCondPred(plan_pddl_t *pddl, plan_pddl_lisp_node_t *root,
                          int action_id, plan_pddl_cond_t *cond)
{
    const char *name;
    int i;

    name = nodeHead(root);
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
        if (_parseVarConst(pddl, root->child + i + 1, action_id,
                           cond->arg + i) != 0)
            return -1;
    }

    return 0;
}

static void _parseForallReplace(plan_pddl_t *pddl, plan_pddl_lisp_node_t *n)
{
    int i;

    if (n->kw == -1 && n->value != NULL){
        for (i = 0; i < pddl->forall.param_size; ++i){
            if (strcmp(n->value, pddl->forall.param[i].name) == 0){
                fprintf(stderr, "XXX\n");
                n->value = pddl->obj[pddl->forall.param_bind[i]].name;
            }
        }
    }

    for (i = 0; i < n->child_size; ++i){
        _parseForallReplace(pddl, n->child + i);
    }
}

static int parseActionCond(plan_pddl_t *pddl, plan_pddl_lisp_node_t *root,
                           int action_id, plan_pddl_cond_t *cond);

static int _parseForallEval(plan_pddl_t *pddl, plan_pddl_lisp_node_t *root,
                            int action_id, plan_pddl_cond_t *cond)
{
    plan_pddl_lisp_node_t arg;
    int i, id, ret;

    for (i = 0; i < pddl->forall.param_size; ++i){
        fprintf(stderr, "forall[%d]: %s %d | b: %d\n",
                i,
                pddl->forall.param[i].name,
                pddl->forall.param[i].type,
                pddl->forall.param_bind[i]);
    }

    planPDDLLispNodeCopy(&arg, root->child + 2);
    _parseForallReplace(pddl, &arg);

    cond->arg_size += 1;
    cond->arg = BOR_REALLOC_ARR(cond->arg, plan_pddl_cond_t, cond->arg_size);
    id = cond->arg_size - 1;
    condInit(cond->arg + id);
    ret = parseActionCond(pddl, &arg, action_id, cond->arg + id);
    planPDDLLispNodeFree(&arg);
    return ret;
}

static int _parseForallRec(plan_pddl_t *pddl, plan_pddl_lisp_node_t *root,
                           int action_id, plan_pddl_cond_t *cond, int param)
{
    plan_pddl_obj_arr_t *m;
    int i;

    m = pddl->type_obj_map + pddl->forall.param[param].type;
    for (i = 0; i < m->size; ++i){
        pddl->forall.param_bind[param] = m->obj[i];
        if (param == pddl->forall.param_size - 1){
            if (_parseForallEval(pddl, root, action_id, cond) != 0)
                return -1;
        }else{
            if (_parseForallRec(pddl, root, action_id, cond, param + 1) != 0)
                return -1;
        }
    }

    return 0;
}

static int _parseForallSet(plan_pddl_t *pddl,
                           plan_pddl_lisp_node_t *root,
                           int child_from, int child_to,
                           int child_type)
{
    int i, j, tid;

    tid = 0;
    if (child_type >= 0){
        tid = getType(pddl, root->child[child_type].value);
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

static int _parseForall(plan_pddl_t *pddl, plan_pddl_lisp_node_t *root,
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
                       _parseForallSet) != 0){
        if (pddl->forall.param != NULL)
            BOR_FREE(pddl->forall.param);
        return -1;
    }

    pddl->forall.param_bind = BOR_CALLOC_ARR(int, pddl->forall.param_size);
    if (_parseForallRec(pddl, root, action_id, cond, 0) != 0){
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

static int parseActionCond(plan_pddl_t *pddl, plan_pddl_lisp_node_t *root,
                           int action_id, plan_pddl_cond_t *cond)
{
    int i;

    if (root->value != NULL)
        return _parseVarConst(pddl, root, action_id, cond);

    cond->type = condType(nodeHeadKw(root));
    if (cond->type == -1)
        return _parseCondPred(pddl, root, action_id, cond);


    if (cond->type == PLAN_PDDL_COND_NOT){
        if (root->child_size != 2
                || root->child[1].value != NULL){
            ERRN2(root, "Invalid (not ...) condition.");
            return -1;
        }

        cond->arg_size = 1;
        cond->arg = BOR_ALLOC(plan_pddl_cond_t);
        condInit(cond->arg);
        return parseActionCond(pddl, root->child + 1, action_id, cond->arg);

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
            if (parseActionCond(pddl, root->child + i + 1, action_id,
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
        if (_parseForall(pddl, root, action_id, cond) != 0)
            return -1;

    }else if (cond->type == PLAN_PDDL_COND_WHEN){
        // TODO

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

        cond->arg_size = 1;
        cond->arg = BOR_ALLOC(plan_pddl_cond_t);
        condInit(cond->arg);

        if (root->child[2].value != NULL){
            cond->arg[0].type = PLAN_PDDL_COND_INT;
            cond->arg[0].val = atoi(root->child[2].value);
        }else{
            ERRN2(root, "(increase ...) with functions not supported yet.");
            return -1;
        }
    }
    return 0;
}

static int parseAction1(plan_pddl_t *pddl, plan_pddl_lisp_node_t *root)
{
    int action_id;

    if (root->child_size < 4
            || root->child[1].value == NULL
            || root->child[2].kw != PLAN_PDDL_KW_PARAMETERS
            || root->child[3].value != NULL){
        ERRN2(root, "Invalid definition of :action");
        return -1;
    }

    action_id = addAction(pddl, root->child[1].value);
    if (parseTypedList(pddl, root->child + 3, 0,
                       root->child[3].child_size, parseActionParamSet) != 0)
        return -1;

    if (root->child_size >= 6){
        if (root->child[4].kw == PLAN_PDDL_KW_PRE){
            if (parseActionCond(pddl, root->child + 5, action_id,
                                &pddl->action[action_id].pre) != 0)
                return -1;

        }else if (root->child[4].kw == PLAN_PDDL_KW_EFF){
            if (parseActionCond(pddl, root->child + 5, action_id,
                                &pddl->action[action_id].eff) != 0)
                return -1;
        }else{
            ERRN2(root->child + 4, "Invalid definition of :action");
            return -1;
        }
    }

    if (root->child_size == 8){
        if (root->child[6].kw != PLAN_PDDL_KW_EFF){
            ERRN2(root->child + 4, "Invalid definition of :action");
            return -1;
        }
        if (parseActionCond(pddl, root->child + 7, action_id,
                            &pddl->action[action_id].eff) != 0)
            return -1;
    }

    return 0;
}

static int parseAction(plan_pddl_t *pddl, plan_pddl_lisp_node_t *root)
{
    int i;

    for (i = 0; i < root->child_size; ++i){
        if (nodeHeadKw(root->child + i) == PLAN_PDDL_KW_ACTION){
            if (parseAction1(pddl, root->child + i) != 0)
                return -1;
        }
    }
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

    if (checkDomainName(pddl) != 0)
        goto pddl_fail;

    pddl->require = parseRequire(&domain_lisp->root);
    if (pddl->require == 0u)
        goto pddl_fail;

    // Add 'object' type
    pddl->type_size = 1;
    pddl->type = BOR_ALLOC_ARR(plan_pddl_type_t, 1);
    pddl->type[0].name = _object_type_name;
    pddl->type[0].parent = -1;

    // Add (= ?x ?y - object) predicate if set in requirements
    if (pddl->require & PLAN_PDDL_REQUIRE_EQUALITY)
        addEqPredicate(pddl);

    if (parseType(pddl, &domain_lisp->root) != 0
            || parseConstant(pddl, &domain_lisp->root) != 0
            || parseObj(pddl, &problem_lisp->root) != 0
            || makeTypeObjMap(pddl) != 0
            || parsePredicate(pddl, &domain_lisp->root) != 0
            || parseAction(pddl, &domain_lisp->root) != 0)
        goto pddl_fail;

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
    if (pddl->type)
        BOR_FREE(pddl->type);
    if (pddl->obj)
        BOR_FREE(pddl->obj);

    for (i = 0; i < pddl->predicate_size; ++i){
        if (pddl->predicate[i].param != NULL)
            BOR_FREE(pddl->predicate[i].param);
    }
    if (pddl->predicate)
        BOR_FREE(pddl->predicate);

    for (i = 0; i < pddl->action_size; ++i){
        if (pddl->action[i].param != NULL)
            BOR_FREE(pddl->action[i].param);
        condFree(&pddl->action[i].pre);
        condFree(&pddl->action[i].eff);
    }
    if (pddl->action)
        BOR_FREE(pddl->action);

    if (pddl->type_obj_map != NULL){
        for (i = 0; i < pddl->type_size; ++i){
            if (pddl->type_obj_map[i].obj != NULL)
                BOR_FREE(pddl->type_obj_map[i].obj);
        }
        BOR_FREE(pddl->type_obj_map);
    }
    BOR_FREE(pddl);
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

static void dumpCond(const plan_pddl_t *pddl, const plan_pddl_action_t *a,
                     const plan_pddl_cond_t *cond, FILE *fout)
{
    const plan_pddl_predicate_t *pred;
    int i;

    if (cond->type == PLAN_PDDL_COND_AND
            || cond->type == PLAN_PDDL_COND_OR
            || cond->type == PLAN_PDDL_COND_NOT
            || cond->type == PLAN_PDDL_COND_INCREASE){
        fprintf(fout, "(%s", _condToStr(cond->type));
        for (i = 0; i < cond->arg_size; ++i){
            fprintf(fout, " ");
            dumpCond(pddl, a, cond->arg + i, fout);
        }
        fprintf(fout, ")");

    }else if (cond->type == PLAN_PDDL_COND_PRED){
        pred = pddl->predicate + cond->val;
        fprintf(fout, "(%s", pred->name);
        for (i = 0; i < cond->arg_size; ++i){
            fprintf(fout, " ");
            dumpCond(pddl, a, cond->arg + i, fout);
        }
        fprintf(fout, ")");

    }else if (cond->type == PLAN_PDDL_COND_VAR){
        fprintf(fout, "%s", a->param[cond->val].name);

    }else if (cond->type == PLAN_PDDL_COND_CONST){
        fprintf(fout, "%s", pddl->obj[cond->val].name);

    }else if (cond->type == PLAN_PDDL_COND_INT){
        fprintf(fout, "%d", cond->val);
    }
}

void planPDDLDump(const plan_pddl_t *pddl, FILE *fout)
{
    int i, j;

    fprintf(fout, "Domain: %s\n", pddl->domain_name);
    fprintf(fout, "Problem: %s\n", pddl->problem_name);
    fprintf(fout, "Require: %x\n", pddl->require);
    fprintf(fout, "Type[%d]:\n", pddl->type_size);
    for (i = 0; i < pddl->type_size; ++i){
        fprintf(fout, "    [%d]: %s, parent: %d\n", i,
                pddl->type[i].name, pddl->type[i].parent);
    }

    fprintf(fout, "Obj[%d]:\n", pddl->obj_size);
    for (i = 0; i < pddl->obj_size; ++i){
        fprintf(fout, "    [%d]: %s, type: %d, is-constant: %d\n", i,
                pddl->obj[i].name, pddl->obj[i].type,
                pddl->obj[i].is_constant);
    }

    fprintf(fout, "Type-Obj:\n");
    for (i = 0; i < pddl->type_size; ++i){
        fprintf(fout, "    [%d]:", i);
        for (j = 0; j < pddl->type_obj_map[i].size; ++j)
            fprintf(fout, " %d", pddl->type_obj_map[i].obj[j]);
        fprintf(fout, "\n");
    }

    fprintf(fout, "Predicate[%d]:\n", pddl->predicate_size);
    for (i = 0; i < pddl->predicate_size; ++i){
        fprintf(fout, "    %s:", pddl->predicate[i].name);
        for (j = 0; j < pddl->predicate[i].param_size; ++j){
            fprintf(fout, " %d", pddl->predicate[i].param[j]);
        }
        fprintf(fout, "\n");
    }

    fprintf(fout, "Action[%d]:\n", pddl->action_size);
    for (i = 0; i < pddl->action_size; ++i){
        fprintf(fout, "    %s:", pddl->action[i].name);
        for (j = 0; j < pddl->action[i].param_size; ++j){
            fprintf(fout, " %s:%d", pddl->action[i].param[j].name,
                    pddl->action[i].param[j].type);
        }
        fprintf(fout, "\n");

        fprintf(fout, "        pre: ");
        dumpCond(pddl, pddl->action + i, &pddl->action[i].pre, fout);
        fprintf(fout, "\n");

        fprintf(fout, "        eff: ");
        dumpCond(pddl, pddl->action + i, &pddl->action[i].eff, fout);
        fprintf(fout, "\n");
    }
}
