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

#ifndef __PLAN_PDDL_H__
#define __PLAN_PDDL_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * Requirements
 */
#define PLAN_PDDL_REQUIRE_STRIPS                0x000001u
#define PLAN_PDDL_REQUIRE_TYPING                0x000002u
#define PLAN_PDDL_REQUIRE_NEGATIVE_PRE          0x000004u
#define PLAN_PDDL_REQUIRE_DISJUNCTIVE_PRE       0x000008u
#define PLAN_PDDL_REQUIRE_EQUALITY              0x000010u
#define PLAN_PDDL_REQUIRE_EXISTENTIAL_PRE       0x000020u
#define PLAN_PDDL_REQUIRE_UNIVERSAL_PRE         0x000040u
#define PLAN_PDDL_REQUIRE_CONDITIONAL_EFF       0x000080u
#define PLAN_PDDL_REQUIRE_NUMERIC_FLUENT        0x000100u
#define PLAN_PDDL_REQUIRE_OBJECT_FLUENT         0x000200u
#define PLAN_PDDL_REQUIRE_DURATIVE_ACTION       0x000400u
#define PLAN_PDDL_REQUIRE_DURATION_INEQUALITY   0x000800u
#define PLAN_PDDL_REQUIRE_CONTINUOUS_EFF        0x001000u
#define PLAN_PDDL_REQUIRE_DERIVED_PRED          0x002000u
#define PLAN_PDDL_REQUIRE_TIMED_INITIAL_LITERAL 0x004000u
#define PLAN_PDDL_REQUIRE_PREFERENCE            0x008000u
#define PLAN_PDDL_REQUIRE_CONSTRAINT            0x010000u
#define PLAN_PDDL_REQUIRE_ACTION_COST           0x020000u
#define PLAN_PDDL_REQUIRE_MULTI_AGENT           0x040000u
#define PLAN_PDDL_REQUIRE_UNFACTORED_PRIVACY    0x080000u
#define PLAN_PDDL_REQUIRE_FACTORED_PRIVACY      0x100000u

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* __PLAN_PDDL_H__ */
