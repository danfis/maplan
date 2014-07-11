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
    plan_heur_t **heur;
    plan_list_lazy_t **list;
    plan_search_t **search;
    plan_search_ehc_params_t ehc_params;
    plan_search_lazy_params_t lazy_params;
    plan_path_t path;
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

    heur   = BOR_ALLOC_ARR(plan_heur_t *, prob->agent_size);
    list   = BOR_ALLOC_ARR(plan_list_lazy_t *, prob->agent_size);
    search = BOR_ALLOC_ARR(plan_search_t *, prob->agent_size);

    for (i = 0; i < prob->agent_size; ++i){
        plan_problem_t *p = &prob->agent[i].prob;
        heur[i] = planHeurRelaxAddNew(p->var, p->var_size, p->goal,
                                      prob->agent[i].projected_op,
                                      prob->agent[i].projected_op_size,
                                      NULL);
        list[i] = planListLazySplayTreeNew();

        planSearchEHCParamsInit(&ehc_params);
        ehc_params.heur = heur[i];
        ehc_params.search.prob = &prob->agent[i].prob;

        planSearchLazyParamsInit(&lazy_params);
        lazy_params.heur = heur[i];
        lazy_params.list = list[i];
        lazy_params.search.prob = &prob->agent[i].prob;
        //search[i] = planSearchEHCNew(&ehc_params);
        search[i] = planSearchLazyNew(&lazy_params);
    }

    planPathInit(&path);
    planMARun(prob->agent_size, prob->agent, search, &path);
    planPathFree(&path);

    for (i = 0; i < prob->agent_size; ++i){
        planSearchDel(search[i]);
        planListLazyDel(list[i]);
        planHeurDel(heur[i]);
    }

    BOR_FREE(search);
    BOR_FREE(list);
    BOR_FREE(heur);

    planProblemAgentsDel(prob);

    return 0;
}

