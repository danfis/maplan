#include <boruvka/timer.h>
#include <boruvka/alloc.h>
#include <boruvka/tasks.h>
#include <plan/search.h>
#include <plan/heur.h>
#include <plan/search.h>
#include <plan/ma.h>

int main(int argc, char *argv[])
{
    plan_problem_agents_t *prob = NULL;
    plan_search_t **search;
    plan_search_ehc_params_t ehc_params;
    plan_search_lazy_params_t lazy_params;
    plan_ma_agent_path_op_t *path;
    int path_size;
    bor_timer_t timer;
    int i, res;
    FILE *fout;

    if (argc != 2){
        fprintf(stderr, "Usage: %s prob.asas\n", argv[0]);
        return -1;
    }

    borTimerStart(&timer);

    prob = planProblemAgentsFromFD(argv[1]);
    if (prob == NULL || prob->agent_size <= 1){
        fprintf(stderr, "Error: More than one agent must be defined (got %d).",
                prob->agent_size);
        return -1;
    }

    search = BOR_ALLOC_ARR(plan_search_t *, prob->agent_size);

    for (i = 0; i < prob->agent_size; ++i){
        plan_problem_t *p = &prob->agent[i].prob;
        plan_heur_t *heur;
        heur = planHeurRelaxAddNew(p->var, p->var_size, p->goal,
                                   prob->agent[i].projected_op,
                                   prob->agent[i].projected_op_size,
                                   NULL);
        heur = planHeurRelaxAddNew(p->var, p->var_size, p->goal,
                                   prob->prob.op, prob->prob.op_size,
                                   prob->prob.succ_gen);

        /*
        planSearchEHCParamsInit(&ehc_params);
        ehc_params.heur = heur[i];
        ehc_params.search.prob = &prob->agent[i].prob;
        */

        planSearchLazyParamsInit(&lazy_params);
        lazy_params.heur = heur;
        lazy_params.heur_del = 1;
        lazy_params.list = planListLazySplayTreeNew();
        lazy_params.list_del = 1;
        lazy_params.search.prob = &prob->agent[i].prob;
        //search[i] = planSearchEHCNew(&ehc_params);
        search[i] = planSearchLazyNew(&lazy_params);
    }

    res = planMARun(prob->agent_size, search, &path, &path_size);
    if (res == PLAN_SEARCH_FOUND){
        //fprintf(stdout, "Solution found.\n");
        for (i = 0; i < path_size; ++i){
            fprintf(stdout, "(%s)\n", path[i].name);
        }

        if (path)
            planMAAgentPathFree(path, path_size);
    }

    for (i = 0; i < prob->agent_size; ++i){
        planSearchDel(search[i]);
    }

    BOR_FREE(search);

    planProblemAgentsDel(prob);

    return 0;
}

