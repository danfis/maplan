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

#include "plan/pddl_require.h"

#include "pddl_err.h"

/**
 * Mapping between keywords and require flags.
 */
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

static unsigned requireMask(int kw)
{
    int i;

    for (i = 0; i < require_mask_size; ++i){
        if (require_mask[i].kw == kw)
            return require_mask[i].mask;
    }
    return 0u;
}

int planPDDLRequireParse(const plan_pddl_lisp_t *domain, unsigned *req)
{
    const plan_pddl_lisp_node_t *req_node, *n;
    unsigned m;
    int i;

    *req = 0u;
    req_node = planPDDLLispFindNode(&domain->root, PLAN_PDDL_KW_REQUIREMENTS);
    // No :requirements implies :strips
    if (req_node == NULL){
        *req = PLAN_PDDL_REQUIRE_STRIPS;
        return 0;
    }

    for (i = 1; i < req_node->child_size; ++i){
        n = req_node->child + i;
        if (n->value == NULL){
            ERRN2(n, "Invalid :requirements definition.");
            return -1;
        }
        if ((m = requireMask(n->kw)) == 0u){
            ERRN(n, "Invalid :requirements definition: Unkown keyword `%s'.",
                 n->value);
            return -1;
        }

        *req |= m;
    }

    return 0;
}
