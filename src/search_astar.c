#include <boruvka/alloc.h>

#include "plan/search.h"
#include "plan/list.h"

struct _plan_search_astar_t {
    plan_search_t search;

    plan_list_t *list; /*!< Open-list */
    plan_heur_t *heur; /*!< Heuristic function */
    int heur_del;      /*!< True if .heur should be deleted */
};
typedef struct _plan_search_astar_t plan_search_astar_t;

#define SEARCH_FROM_PARENT(parent) \
    bor_container_of((parent), plan_search_astar_t, search)

/** Frees allocated resorces */
static void planSearchAStarDel(plan_search_t *_search);
/** Initializes search. This must be call exactly once. */
static int planSearchAStarInit(plan_search_t *_search);
/** Performes one step in the algorithm. */
static int planSearchAStarStep(plan_search_t *_search,
                               plan_search_step_change_t *change);
/** Injects a new state into open-list */
static int planSearchAStarInjectState(plan_search_t *_search,
                                      plan_state_id_t state_id,
                                      plan_cost_t cost,
                                      plan_cost_t heuristic);


void planSearchAStarParamsInit(plan_search_astar_params_t *p)
{
    bzero(p, sizeof(*p));
    planSearchParamsInit(&p->search);
}

plan_search_t *planSearchAStarNew(const plan_search_astar_params_t *params)
{
    plan_search_astar_t *search;

    search = BOR_ALLOC(plan_search_astar_t);

    _planSearchInit(&search->search, &params->search,
                    planSearchAStarDel,
                    planSearchAStarInit,
                    planSearchAStarStep,
                    planSearchAStarInjectState);

    search->list     = planListTieBreaking(2);
    search->heur     = params->heur;
    search->heur_del = params->heur_del;

    return &search->search;
}

static void planSearchAStarDel(plan_search_t *_search)
{
    plan_search_astar_t *search = SEARCH_FROM_PARENT(_search);

    _planSearchFree(&search->search);
    if (search->list)
        planListDel(search->list);
    if (search->heur_del)
        planHeurDel(search->heur);
    BOR_FREE(search);
}

static int planSearchAStarInit(plan_search_t *_search)
{
    //plan_search_astar_t *search = SEARCH_FROM_PARENT(_search);
    // TODO
    return PLAN_SEARCH_CONT;
}

static int planSearchAStarStep(plan_search_t *_search,
                             plan_search_step_change_t *change)
{
    //plan_search_astar_t *search = SEARCH_FROM_PARENT(_search);

    if (change)
        planSearchStepChangeReset(change);
    // TODO
    return PLAN_SEARCH_CONT;
}

static int planSearchAStarInjectState(plan_search_t *_search, plan_state_id_t state_id,
                                    plan_cost_t cost, plan_cost_t heuristic)
{
    //plan_search_astar_t *search = SEARCH_FROM_PARENT(_search);
    // TODO
    return -1;
}
