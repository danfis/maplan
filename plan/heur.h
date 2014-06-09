#ifndef __PLAN_HEUR_H__
#define __PLAN_HEUR_H__

#include <plan/common.h>
#include <plan/state.h>

/**
 * Heuristic Function API
 * =======================
 */

typedef void (*plan_heur_del_fn)(void *heur);
typedef plan_cost_t (*plan_heur_fn)(void *heur, const plan_state_t *state);

struct _plan_heur_t {
    plan_heur_del_fn del_fn;
    plan_heur_fn heur_fn;
};
typedef struct _plan_heur_t plan_heur_t;

/**
 * Initializes heuristics.
 * For internal use.
 */
void planHeurInit(plan_heur_t *heur,
                  plan_heur_del_fn del_fn,
                  plan_heur_fn heur_fn);

/**
 * Frees allocated resources.
 * For internal use.
 */
void planHeurFree(plan_heur_t *heur);

/**
 * Deletes heuristics object.
 */
void planHeurDel(plan_heur_t *heur);

/**
 * Returns a heuristic value.
 */
plan_cost_t planHeur(plan_heur_t *heur, const plan_state_t *state);

#endif /* __PLAN_HEUR_H__ */
