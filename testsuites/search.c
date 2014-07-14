#include <boruvka/timer.h>
#include <plan/search.h>
#include <plan/heur.h>
#include <plan/search.h>
#include <opts.h>

static char *def_fd_problem = NULL;
static char *def_search = "ehc";
static char *def_list = "heap";
static char *def_heur = "goalcount";
static char *plan_output_fn = NULL;
static int max_time = 60 * 30; // 30 minutes
static int max_mem = 1024 * 1024; // 1GB
static int progress_freq = 10000;

static plan_list_lazy_t *listLazyCreate(const char *name)
{
    plan_list_lazy_t *list = NULL;

    if (strcmp(name, "heap") == 0){
        list = planListLazyHeapNew();
    }else if (strcmp(name, "bucket") == 0){
        list = planListLazyBucketNew();
    }else if (strcmp(name, "rbtree") == 0){
        list = planListLazyRBTreeNew();
    }else if (strcmp(name, "splaytree") == 0){
        list = planListLazySplayTreeNew();
    }else{
        fprintf(stderr, "Error: Invalid list type: `%s'\n", name);
    }

    return list;
}

static plan_heur_t *heurCreate(const char *name,
                               const plan_var_t *var, int var_size,
                               const plan_part_state_t *goal,
                               const plan_operator_t *op, int op_size,
                               const plan_succ_gen_t *succ_gen)
{
    plan_heur_t *heur = NULL;

    if (strcmp(name, "goalcount") == 0){
        heur = planHeurGoalCountNew(goal);
    }else if (strcmp(name, "add") == 0){
        heur = planHeurRelaxAddNew(var, var_size, goal, op, op_size, succ_gen);
    }else if (strcmp(name, "max") == 0){
        heur = planHeurRelaxMaxNew(var, var_size, goal, op, op_size, succ_gen);
    }else if (strcmp(name, "ff") == 0){
        heur = planHeurRelaxFFNew(var, var_size, goal, op, op_size, succ_gen);
    }else{
        fprintf(stderr, "Error: Invalid heuristic type: `%s'\n", name);
    }

    return heur;
}

static int readOpts(int argc, char *argv[])
{
    int help;

    optsAddDesc("help", 'h', OPTS_NONE, &help, NULL,
                "Print this help.");
    optsAddDesc("fd", 'f', OPTS_STR, &def_fd_problem, NULL,
                "Path to the FD's .sas problem definition.");
    optsAddDesc("search", 's', OPTS_STR, &def_search, NULL,
                "Define search algorithm [ehc|lazy] (default: ehc)");
    optsAddDesc("list", 'l', OPTS_STR, &def_list, NULL,
                "Define list type [heap|bucket|rbtree|splaytree] (default: heap)");
    optsAddDesc("heur", 'H', OPTS_STR, &def_heur, NULL,
                "Define heuristic [goalcount|add|max|ff] (default: goalcount)");
    optsAddDesc("plan-output", 'o', OPTS_STR, &plan_output_fn, NULL,
                "Path where to write resulting plan.");
    optsAddDesc("max-time", 0x0, OPTS_INT, &max_time, NULL,
                "Maximal time the search can spent on finding solution in"
                " seconds. (default: 30 minutes).");
    optsAddDesc("max-mem", 0x0, OPTS_INT, &max_mem, NULL,
                "Maximal memory (peak memory) in kb. (default: 1GB)");

    if (opts(&argc, argv) != 0){
        return -1;
    }

    if (help)
        return -1;

    if (def_fd_problem == NULL){
        fprintf(stderr, "Error: Problem must be defined (-p or -f).\n");
        return -1;
    }

    return 0;
}

static void usage(const char *progname)
{
    fprintf(stderr, "Usage: %s [OPTIONS]\n", progname);
    fprintf(stderr, "  OPTIONS:\n");
    optsPrint(stderr, "    ");
    fprintf(stderr, "\n");
}

static int progress(const plan_search_stat_t *stat)
{
    fprintf(stderr, "[%.3f s, %ld kb] %ld steps, %ld evaluated,"
                    " %ld expanded, %ld generated, not-found: %d,"
                    " found: %d\n",
            stat->elapsed_time, stat->peak_memory,
            stat->steps, stat->evaluated_states, stat->expanded_states,
            stat->generated_states,
            stat->not_found, stat->found);

    if (stat->elapsed_time > max_time){
        fprintf(stderr, "Abort: Exceeded max-time.\n");
        printf("Abort: Exceeded max-time.\n");
        return PLAN_SEARCH_ABORT;
    }

    if (stat->peak_memory > max_mem){
        fprintf(stderr, "Abort: Exceeded max-mem.\n");
        printf("Abort: Exceeded max-mem.\n");
        return PLAN_SEARCH_ABORT;
    }

    return PLAN_SEARCH_CONT;
}

int main(int argc, char *argv[])
{
    plan_problem_t *prob = NULL;
    plan_list_lazy_t *list;
    plan_heur_t *heur;
    plan_search_t *search;
    plan_search_ehc_params_t ehc_params;
    plan_search_lazy_params_t lazy_params;
    plan_search_params_t *params;
    plan_path_t path;
    bor_timer_t timer;
    int res;
    FILE *fout;

    if (readOpts(argc, argv) != 0){
        usage(argv[0]);
        return -1;
    }

    borTimerStart(&timer);

    printf("Problem: %s\n", def_fd_problem);
    printf("Search: %s\n", def_search);
    printf("List: %s\n", def_list);
    printf("Heur: %s\n", def_heur);
    printf("Max Time: %d s\n", max_time);
    printf("Max Mem: %d kb\n", max_mem);
    printf("\n");

    if (def_fd_problem){
        prob = planProblemFromFD(def_fd_problem);
    }
    if (prob == NULL){
        return -1;
    }

    printf("Num variables: %d\n", prob->var_size);
    printf("Num operators: %d\n", prob->op_size);
    printf("Bytes per state: %d\n",
           planStatePackerBufSize(prob->state_pool->packer));
    printf("Size of state id: %d\n", (int)sizeof(plan_state_id_t));
    borTimerStop(&timer);
    printf("Loading Time: %f s\n", borTimerElapsedInSF(&timer));
    printf("\n");

    if ((list = listLazyCreate(def_list)) == NULL)
        return -1;

    heur = heurCreate(def_heur, prob->var, prob->var_size, prob->goal,
                      prob->op, prob->op_size, prob->succ_gen);
    if (heur == NULL)
        return -1;

    if (strcmp(def_search, "ehc") == 0){
        planSearchEHCParamsInit(&ehc_params);
        ehc_params.heur = heur;
        params = &ehc_params.search;
    }else if (strcmp(def_search, "lazy") == 0){
        planSearchLazyParamsInit(&lazy_params);
        lazy_params.heur = heur;
        lazy_params.list = list;
        params = &lazy_params.search;
    }else{
        fprintf(stderr, "Error: Unkown search algorithm.\n");
        return -1;
    }

    params->progress_fn = progress;
    params->progress_freq = progress_freq;
    params->prob = prob;

    if (strcmp(def_search, "ehc") == 0){
        search = planSearchEHCNew(&ehc_params);
    }else if (strcmp(def_search, "lazy") == 0){
        search = planSearchLazyNew(&lazy_params);
    }

    borTimerStop(&timer);
    printf("Init Time: %f s\n", borTimerElapsedInSF(&timer));

    planPathInit(&path);
    res = planSearchRun(search, &path);
    if (res == PLAN_SEARCH_FOUND){
        printf("Solution found.\n");

        if (plan_output_fn != NULL){
            fout = fopen(plan_output_fn, "w");
            if (fout != NULL){
                planPathPrint(&path, fout);
                fclose(fout);
                printf("Plan written to `%s'\n", plan_output_fn);
            }else{
                fprintf(stderr, "Error: Could not plan write to `%s'\n",
                        plan_output_fn);
            }
        }

        printf("Path Cost: %d\n", (int)planPathCost(&path));

    }else if (res == PLAN_SEARCH_NOT_FOUND){
        printf("Solution NOT found.");

    }else if (res == PLAN_SEARCH_ABORT){
        printf("Search Aborted.");
    }

    printf("\n");
    printf("Search Time: %f\n", search->stat.elapsed_time);
    printf("Steps: %ld\n", search->stat.steps);
    printf("Evaluated States: %ld\n", search->stat.evaluated_states);
    printf("Expanded States: %ld\n", search->stat.expanded_states);
    printf("Generated States: %ld\n", search->stat.generated_states);
    printf("Peak Memory: %ld kb\n", search->stat.peak_memory);

    planPathFree(&path);

    planSearchDel(search);
    planHeurDel(heur);
    planListLazyDel(list);
    planProblemDel(prob);

    borTimerStop(&timer);
    printf("Overall Time: %f s\n", borTimerElapsedInSF(&timer));

    optsClear();

    return 0;
}
