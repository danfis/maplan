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

