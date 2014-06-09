#include <boruvka/timer.h>
#include "plan/search_ehc.h"

int main(int argc, char *argv[])
{
    plan_problem_t *prob;
    plan_search_ehc_t *ehc;
    plan_path_t path;
    bor_timer_t timer;

    if (argc != 2){
        fprintf(stderr, "Usage: %s problem.json\n", argv[0]);
        return -1;
    }

    borTimerStart(&timer);

    prob = planProblemFromJson(argv[1]);

    ehc = planSearchEHCNew(prob);
    planPathInit(&path);

    planSearchEHCRun(ehc, &path);
    planPathPrint(&path, stdout);
    fprintf(stderr, "Path cost: %d\n", (int)planPathCost(&path));

    planPathFree(&path);
    planSearchEHCDel(ehc);
    planProblemDel(prob);

    borTimerStop(&timer);
    fprintf(stderr, "Elapsed Time: %.4f seconds\n",
            borTimerElapsedInSF(&timer));

    return 0;
}
