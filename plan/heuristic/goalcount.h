#ifndef __PLAN_HEURISTIC_GOALCOUNT_H__
#define __PLAN_HEURISTIC_GOALCOUNT_H__

#include <plan/state.h>


struct _plan_heuristic_goalcount_t {
    const plan_part_state_t *goal;
};
typedef struct _plan_heuristic_goalcount_t plan_heuristic_goalcount_t;

/**
 * Creates a heuristic.
 * Note that it borrows the given pointer so be sure the heuristic is
 * deleted before the given partial state.
 */
plan_heuristic_goalcount_t *planHeuristicGoalCountNew(
                const plan_part_state_t *goal);

/**
 * Deletes the heuristic object.
 */
void planHeuristicGoalCountDel(plan_heuristic_goalcount_t *h);

/**
 * Computes a heuristic value corresponding to the state.
 */
unsigned planHeuristicGoalCount(plan_heuristic_goalcount_t *h,
                                const plan_state_t *state);

#endif /* __PLAN_HEURISTIC_GOALCOUNT_H__ */
