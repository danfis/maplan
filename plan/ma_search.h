#ifndef __PLAN_MA_SEARCH_H__
#define __PLAN_MA_SEARCH_H__

#include <plan/heur.h>
#include <plan/path.h>
#include <plan/statespace.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * Common parameters for all search algorithms.
 */
struct _plan_ma_search_params_t {
    plan_ma_comm_t *comm; /*!< Communication channel between agents */

    int verify_solution;  /*!< Set to true if you want verify optimal
                               solution with other agents */

    plan_state_id_t initial_state;
    const plan_part_state_t *goal;
    plan_state_pool_t *state_pool;
    plan_state_space_t *state_space;
    plan_heur_t *heur;
    const plan_succ_gen_t *succ_gen;
};
typedef struct _plan_ma_search_params_t plan_ma_search_params_t;

/**
 * Initialize parameters.
 */
void planMASearchParamsInit(plan_ma_search_params_t *params);


/**
 * Main mutli-agent search structure.
 */
struct _plan_ma_search_t {
    plan_ma_comm_t *comm; /*!< Communication channel between agents */
    int terminated;       /*!< True if search was already terminated */
    const plan_ma_search_params_t *params;
};
typedef struct _plan_ma_search_t plan_ma_search_t;

/**
 * Initializes ma-search structure.
 */
void planMASearchInit(plan_ma_search_t *search,
                      const plan_ma_search_params_t *params);

/**
 * Frees allocated resources.
 */
void planMASearchFree(plan_ma_search_t *search);

/**
 * Searches for the path from the initial state to the goal as defined via
 * parameters.
 * Returns PLAN_SEARCH_FOUND if the solution was found and in this case the
 * path is returned via path argument.
 * If the plan was not found, PLAN_SEARCH_NOT_FOUND is returned.
 * If the search progress was aborted by the "progess" callback,
 * PLAN_SEARCH_ABORT is returned.
 */
int planMASearchRun(plan_ma_search_t *search, plan_path_t *path);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __PLAN_MA_SEARCH_H__ */
