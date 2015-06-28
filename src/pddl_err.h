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

#ifndef __PLAN_PDDL_ERR_H__
#define __PLAN_PDDL_ERR_H__

#include <plan/pddl_lisp.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

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


#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* __PLAN_PDDL_ERR_H__ */

