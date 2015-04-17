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

#ifndef __PLAN_MA_SEARCH_H__
#define __PLAN_MA_SEARCH_H__

#include <plan/search.h>
#include <plan/ma_comm.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** Forward declaration */
typedef struct _plan_ma_search_t plan_ma_search_t;

/**
 * Common parameters for all search algorithms.
 */
struct _plan_ma_search_params_t {
    plan_search_t *search; /*!< Main search algorithm */
    plan_ma_comm_t *comm;  /*!< Communication channel between agents */
    int verify_solution;   /*!< Set to true if a solution should be
                                verified by all agents. Default: 0 */
};
typedef struct _plan_ma_search_params_t plan_ma_search_params_t;

/**
 * Initialize parameters.
 */
void planMASearchParamsInit(plan_ma_search_params_t *params);


/**
 * Creates a multi-agent search object.
 */
plan_ma_search_t *planMASearchNew(plan_ma_search_params_t *params);

/**
 * Deletes multi-agent search object.
 */
void planMASearchDel(plan_ma_search_t *ma_search);

/**
 * Runs underlying search algorithm in a multi-agent mode.
 * Returns the same values as the underlying search algorithm.
 */
int planMASearchRun(plan_ma_search_t *search, plan_path_t *path);

/**
 * Aborts ma-search process from outside (can be called from other thread).
 */
void planMASearchAbort(plan_ma_search_t *search);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __PLAN_MA_SEARCH_H__ */
