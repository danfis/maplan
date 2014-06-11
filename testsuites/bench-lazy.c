#include <boruvka/timer.h>
#include "plan/search_lazy.h"
#include "plan/heur_goalcount.h"
#include "plan/heur_relax.h"
#include "plan/list_lazy.h"

int main(int argc, char *argv[])
{
    plan_problem_t *prob;
    plan_search_lazy_t *lazy;
    plan_heur_t *heur;
    plan_list_lazy_t *list;
    plan_path_t path;
    bor_timer_t timer;

    if (argc != 2){
        fprintf(stderr, "Usage: %s problem.json\n", argv[0]);
        return -1;
    }

    borTimerStart(&timer);

    prob = planProblemFromJson(argv[1]);

    heur = planHeurGoalCountNew(prob->goal);
    //heur = planHeurRelaxAddNew(prob);
    //heur = planHeurRelaxMaxNew(prob);
    //heur = planHeurRelaxFFNew(prob);

    list = planListLazyHeapNew();

    lazy = planSearchLazyNew(prob, heur, list);
    planPathInit(&path);

    planSearchLazyRun(lazy, &path);
    planPathPrint(&path, stdout);
    fprintf(stderr, "Path cost: %d\n", (int)planPathCost(&path));

    planPathFree(&path);
    planListLazyDel(list);
    planHeurDel(heur);
    planSearchLazyDel(lazy);
    planProblemDel(prob);

    borTimerStop(&timer);
    fprintf(stderr, "Elapsed Time: %.4f seconds\n",
            borTimerElapsedInSF(&timer));

    return 0;
}

