#include <boruvka/timer.h>
#include "plan/search_lazy.h"
#include "plan/heur_goalcount.h"
#include "plan/heur_relax.h"
#include "plan/list_lazy.h"

int main(int argc, char *argv[])
{
    plan_search_lazy_params_t params;
    plan_search_t *lazy;
    plan_path_t path;
    bor_timer_t timer;

    if (argc != 2){
        fprintf(stderr, "Usage: %s problem.json\n", argv[0]);
        return -1;
    }

    borTimerStart(&timer);

    planSearchLazyParamsInit(&params);

    params.search.prob = planProblemFromJson(argv[1]);

    params.heur = planHeurGoalCountNew(params.search.prob->goal);
    //heur = planHeurRelaxAddNew(prob);
    //heur = planHeurRelaxMaxNew(prob);
    //heur = planHeurRelaxFFNew(prob);

    params.list = planListLazyHeapNew();

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

