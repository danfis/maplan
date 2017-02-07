#include <stdio.h>
#include "plan/problem.h"
#include "plan/search.h"

#include "state_pool.h"

void run(plan_problem_t *p, plan_state_t *init_state)
{
    plan_search_astar_params_t params;
    plan_search_t *search;
    plan_path_t path;
    int ret;

    p->initial_state = planStatePoolInsert(p->state_pool, init_state);
    planSearchAStarParamsInit(&params);
    params.search.prob = p;
    params.search.heur = planHeurLMCutNew(p, 0);
    params.search.heur_del = 1;
    search = planSearchAStarNew(&params);

    planPathInit(&path);
    ret = planSearchRun(search, &path);
    if (ret == PLAN_SEARCH_FOUND){
        printf("%d\n", planPathCost(&path));
        fflush(stdout);
    }else{
        printf("-1\n");
        fflush(stdout);
    }

    planPathFree(&path);
    planSearchDel(search);
}

int main(int argc, char *argv[])
{
    plan_problem_t *p;
    plan_state_t *state;
    state_pool_t state_pool;
    int si;

    if (argc != 3){
        fprintf(stderr, "Usage: %s problem.proto states.txt\n", argv[0]);
        return -1;
    }

    p = planProblemFromProto(argv[1], PLAN_PROBLEM_USE_CG);
    state = planStateNew(p->var_size);
    statePoolInit(&state_pool, argv[2]);
    for (si = 0; statePoolNext(&state_pool, state) == 0; ++si){
        run(p, state);
    }

    statePoolFree(&state_pool);
    planStateDel(state);
    planProblemDel(p);
    return 0;
}
