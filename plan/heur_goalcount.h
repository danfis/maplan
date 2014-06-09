#ifndef __PLAN_HEUR_GOALCOUNT_H__
#define __PLAN_HEUR_GOALCOUNT_H__

#include <plan/heur.h>


/**
 * Goal Count Heuristic
 * =====================
 */
struct _plan_heur_goalcount_t {
    plan_heur_t heur;
    const plan_part_state_t *goal;
};
typedef struct _plan_heur_goalcount_t plan_heur_goalcount_t;

/**
 * Creates a heuristic.
 * Note that it borrows the given pointer so be sure the heuristic is
 * deleted before the given partial state.
 */
plan_heur_t *planHeurGoalCountNew(const plan_part_state_t *goal);

/**
 * Deletes the heuristic object.
 */
void planHeurGoalCountDel(plan_heur_t *h);

#endif /* __PLAN_HEUR_GOALCOUNT_H__ */
