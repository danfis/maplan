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

#ifndef __PLAN_HEUR_DTG_H__
#define __PLAN_HEUR_DTG_H__

#include <plan/dtg.h>

/**
 * Predecessor on the path.
 */
struct _plan_heur_dtg_path_pre_t {
    plan_val_t val; /*!< Predecessor value */
    int len;        /*!< Length of path from init value */
};
typedef struct _plan_heur_dtg_path_pre_t plan_heur_dtg_path_pre_t;

struct _plan_heur_dtg_path_t {
    plan_heur_dtg_path_pre_t *pre; /*!< Array of predecessor where is
                                        stored path from init to all values */
};
typedef struct _plan_heur_dtg_path_t plan_heur_dtg_path_t;

/**
 * Cached paths found in DTG
 */
struct _plan_heur_dtg_path_cache_t {
    int var_size;                /*!< Number of variables */
    plan_heur_dtg_path_t **path; /*!< Array of arrays for each variable and
                                      value */
    int *range;                  /*!< Range of values for each variable */
};
typedef struct _plan_heur_dtg_path_cache_t plan_heur_dtg_path_cache_t;

/**
 * Flag arrays for all values
 */
struct _plan_heur_dtg_values_t {
    int *val_arr;          /*!< Array for all values from all variables */
    int val_arr_byte_size; /*!< Size of .value_arr in bytes */
    int **val;             /*!< Values indexed by variable ID */
    int *val_range;        /*!< Range of values for each variable */
};
typedef struct _plan_heur_dtg_values_t plan_heur_dtg_values_t;

/**
 * Structure representing an open goal.
 */
struct _plan_heur_dtg_open_goal_t {
    plan_var_id_t var;  /*!< Variable ID */
    plan_val_t val;     /*!< Value of the goal */
    plan_val_t min_val; /*!< Value from which leads shortest path to this
                             value */
    int min_dist;       /*!< Length of the shortes path */
    const plan_heur_dtg_path_t *path;
};
typedef struct _plan_heur_dtg_open_goal_t plan_heur_dtg_open_goal_t;

/**
 * List of open goals.
 */
struct _plan_heur_dtg_open_goals_t {
    plan_heur_dtg_values_t values;    /*!< Flags to prevent re-inserting
                                           already processed open-goals */
    plan_heur_dtg_open_goal_t *goals; /*!< Array of open goals */
    int goals_size;                   /*!< Number of open goals */
    int goals_alloc;                  /*!< Allocated elements in .goals[] */
};
typedef struct _plan_heur_dtg_open_goals_t plan_heur_dtg_open_goals_t;

/**
 * Base data structure for computing DTG heuristic.
 */
struct _plan_heur_dtg_data_t {
    plan_dtg_t dtg;
    plan_heur_dtg_path_cache_t dtg_path;
};
typedef struct _plan_heur_dtg_data_t plan_heur_dtg_data_t;

/**
 * Contex struct for computing actual heuristic value.
 */
struct _plan_heur_dtg_ctx_t {
    plan_heur_dtg_values_t values;
    plan_heur_dtg_open_goals_t open_goals;
    plan_cost_t heur;
    plan_heur_dtg_open_goal_t cur_open_goal;
};
typedef struct _plan_heur_dtg_ctx_t plan_heur_dtg_ctx_t;


/**
 * Initialize *data* structure.
 */
void planHeurDTGDataInit(plan_heur_dtg_data_t *dtg_data,
                         const plan_var_t *var, int var_size,
                         const plan_op_t *op, int op_size);

/**
 * Init with "unknown" value for each variable.
 */
void planHeurDTGDataInitUnknown(plan_heur_dtg_data_t *dtg_data,
                                const plan_var_t *var, int var_size,
                                const plan_op_t *op, int op_size);

/**
 * Fress allocated resources.
 */
void planHeurDTGDataFree(plan_heur_dtg_data_t *dtg_data);

/**
 * Returns path corresponding to the var-val pair.
 */
plan_heur_dtg_path_t *planHeurDTGDataPath(plan_heur_dtg_data_t *dtg_data,
                                          plan_var_id_t var,
                                          plan_val_t val);

/**
 * Initializes context structure.
 */
void planHeurDTGCtxInit(plan_heur_dtg_ctx_t *dtg_ctx,
                        const plan_heur_dtg_data_t *dtg_data);

/**
 * Frees allocated resources.
 */
void planHeurDTGCtxFree(plan_heur_dtg_ctx_t *dtg_ctx);

/**
 * Initializes context for next computation of the DTG heuristic.
 */
void planHeurDTGCtxInitStep(plan_heur_dtg_ctx_t *dtg_ctx,
                            plan_heur_dtg_data_t *dtg_data,
                            const plan_val_t *init_state, int init_state_size,
                            const plan_part_state_pair_t *goal, int goal_size);

/**
 * Performs one step of algorithm.
 * Updates .heur value and saves current open goal into .cur_open_goal.
 * Returns 0 if one open goal was processed, -1 if there are no more open
 * goals.
 */
int planHeurDTGCtxStep(plan_heur_dtg_ctx_t *dtg_ctx,
                       plan_heur_dtg_data_t *dtg_data);

#endif /* __PLAN_HEUR_DTG_H__ */
