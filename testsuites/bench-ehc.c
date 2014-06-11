#include <boruvka/timer.h>
#include "plan/search_ehc.h"
#include "plan/heur_goalcount.h"
#include "plan/heur_relax.h"

#define MAX_TIME (60 * 30) // 30 minutes
#define MAX_MEM (1024 * 1024) // 1GB

static int progress(const plan_search_stat_t *stat)
{
    fprintf(stderr, "[%.3f s, %ld kb] %ld steps, %ld evaluated,"
                    " %ld expanded, %ld generated, dead-end: %d,"
                    " found-solution: %d\n",
            stat->elapsed_time, stat->peak_memory,
            stat->steps, stat->evaluated_states, stat->expanded_states,
            stat->generated_states,
            stat->dead_end, stat->found_solution);

    if (stat->elapsed_time > MAX_TIME
            || stat->peak_memory > MAX_MEM){
        fprintf(stderr, "Aborting...\n");
        return PLAN_SEARCH_ABORT;
    }
    return PLAN_SEARCH_CONTINUE;
}

int main(int argc, char *argv[])
{
    plan_search_ehc_params_t params;
    plan_search_ehc_t *ehc;
    plan_path_t path;
    bor_timer_t timer;

    if (argc != 3){
        fprintf(stderr, "Usage: %s [gc|add|max|ff] problem.json\n", argv[0]);
        return -1;
    }

    borTimerStart(&timer);

    planSearchEHCParamsInit(&params);
    params.progress_fn = progress;
    params.progress_freq = 100000L;

    params.prob = planProblemFromJson(argv[2]);

    if (strcmp(argv[1], "gc") == 0){
        params.heur = planHeurGoalCountNew(params.prob->goal);
    }else if (strcmp(argv[1], "add") == 0){
        params.heur = planHeurRelaxAddNew(params.prob);
    }else if (strcmp(argv[1], "max") == 0){
        params.heur = planHeurRelaxMaxNew(params.prob);
    }else if (strcmp(argv[1], "ff") == 0){
        params.heur = planHeurRelaxFFNew(params.prob);
    }else{
        fprintf(stderr, "Error: Invalid heuristic type\n");
        return -1;
    }

    ehc = planSearchEHCNew(&params);
    planPathInit(&path);

    planSearchEHCRun(ehc, &path);
    planPathPrint(&path, stdout);
    fprintf(stderr, "Path cost: %d\n", (int)planPathCost(&path));

    planPathFree(&path);
    planHeurDel(params.heur);
    planSearchEHCDel(ehc);
    planProblemDel(params.prob);

    borTimerStop(&timer);
    fprintf(stderr, "Elapsed Time: %.4f seconds\n",
            borTimerElapsedInSF(&timer));

    return 0;
}
