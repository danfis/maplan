#include <boruvka/timer.h>
#include "plan/search_ehc.h"

int main(int argc, char *argv[])
{
    plan_t *plan;
    plan_search_ehc_t *ehc;
    plan_path_t path;
    bor_timer_t timer;

    borTimerStart(&timer);

    plan = planNew();
    planLoadFromJsonFile(plan, "load-from-file.in2.json");

    ehc = planSearchEHCNew(plan);
    planPathInit(&path);

    planSearchEHCRun(ehc, &path);
    planPathPrint(&path, stdout);
    fprintf(stderr, "Path cost: %u\n", planPathCost(&path));

    planPathFree(&path);
    planSearchEHCDel(ehc);
    planDel(plan);

    borTimerStop(&timer);
    fprintf(stderr, "Elapsed Time: %.4f seconds\n",
            borTimerElapsedInSF(&timer));

    return 0;
}
