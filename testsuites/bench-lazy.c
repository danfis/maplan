#include <boruvka/timer.h>
#include "plan/search_lazy.h"

int main(int argc, char *argv[])
{
    plan_search_lazy_params_t params;
    plan_search_t *lazy;
    plan_path_t path;
    bor_timer_t timer;

    if (argc != 4){
        fprintf(stderr, "Usage: %s [gc|add|max|ff] [heap|bucket] problem.json\n", argv[0]);
        return -1;
    }

    borTimerStart(&timer);

    planSearchLazyParamsInit(&params);

    params.search.prob = planProblemFromJson(argv[3]);

    if (strcmp(argv[1], "gc") == 0){
        params.heur = planHeurGoalCountNew(params.search.prob->goal);
    }else if (strcmp(argv[1], "add") == 0){
        params.heur = planHeurRelaxAddNew(params.search.prob);
    }else if (strcmp(argv[1], "max") == 0){
        params.heur = planHeurRelaxMaxNew(params.search.prob);
    }else if (strcmp(argv[1], "ff") == 0){
        params.heur = planHeurRelaxFFNew(params.search.prob);
    }else{
        fprintf(stderr, "Error: Invalid heuristic type\n");
        return -1;
    }

    if (strcmp(argv[2], "heap") == 0){
        params.list = planListLazyHeapNew();
    }else if (strcmp(argv[2], "bucket") == 0){
        params.list = planListLazyBucketNew();
    }else{
        fprintf(stderr, "Error: Invalid list type\n");
        return -1;
    }


    lazy = planSearchLazyNew(&params);
    planPathInit(&path);

    planSearchRun(lazy, &path);
    planPathPrint(&path, stdout);
    fprintf(stderr, "Path cost: %d\n", (int)planPathCost(&path));

    planPathFree(&path);
    planListLazyDel(params.list);
    planHeurDel(params.heur);
    planSearchDel(lazy);
    planProblemDel(params.search.prob);

    borTimerStop(&timer);
    fprintf(stderr, "Elapsed Time: %.4f seconds\n",
            borTimerElapsedInSF(&timer));

    return 0;
}

