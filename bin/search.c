#include <boruvka/timer.h>
#include <plan/search.h>
#include <plan/heur.h>
#include <plan/search.h>
#include <plan/ma.h>
#include <opts.h>

static char *def_fd_problem = NULL;
static char *def_ma_problem = NULL;
static char *def_search = "ehc";
static char *def_list = "splaytree";
static char *def_heur = "goalcount";
static char *def_ma_heur_op = "projected";
static char *plan_output_fn = NULL;
static int max_time = 60 * 30; // 30 minutes
static int max_mem = 1024 * 1024; // 1GB
static int progress_freq = 10000;
static int use_preferred_ops = 0;
static int use_pathmax = 0;


static void optUsePreferredOps(const char *l, char s, const char *val)
{
    if (strcmp(val, "off") == 0){
        use_preferred_ops = PLAN_SEARCH_PREFERRED_NONE;
    }else if (strcmp(val, "pref") == 0){
        use_preferred_ops = PLAN_SEARCH_PREFERRED_PREF;
    }else if (strcmp(val, "only") == 0){
        use_preferred_ops = PLAN_SEARCH_PREFERRED_ONLY;
    }else{
        fprintf(stderr, "Error: Invalid value of `--use-preferred-ops'"
                        "option. Setting to `off'.\n");
        use_preferred_ops = PLAN_SEARCH_PREFERRED_NONE;
    }
}

static int readOpts(int argc, char *argv[])
{
    int help;

    optsAddDesc("help", 'h', OPTS_NONE, &help, NULL,
                "Print this help.");
    optsAddDesc("fd", 0x0, OPTS_STR, &def_fd_problem, NULL,
                "Path to the FD's .sas problem definition.");
    optsAddDesc("ma", 0x0, OPTS_STR, &def_ma_problem, NULL,
                "Path to definition of multi-agent problem.");
    optsAddDesc("search", 's', OPTS_STR, &def_search, NULL,
                "Define search algorithm [ehc|lazy|astar] (default: ehc)");
    optsAddDesc("list", 'l', OPTS_STR, &def_list, NULL,
                "Define list type [heap|bucket|rbtree|splaytree] (default: splaytree)");
    optsAddDesc("heur", 'H', OPTS_STR, &def_heur, NULL,
                "Define heuristic [goalcount|add|max|ff|lm-cut] (default: goalcount)");
    optsAddDesc("use-preferred-ops", 'p', OPTS_STR, NULL,
                OPTS_CB(optUsePreferredOps),
                "Enable/disable use of preferred operators: off|pref|only."
                " (default: off)");
    optsAddDesc("ma-heur-op", 0x0, OPTS_STR, &def_ma_heur_op, NULL,
                "Which type of operators are used for heuristic"
                " [global,projected,local] (default: projected)");
    optsAddDesc("pathmax", 0x0, OPTS_NONE, &use_pathmax, NULL,
                "Use pathmax correction. (default: false)");
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

    if (def_fd_problem == NULL && def_ma_problem == NULL){
        fprintf(stderr, "Error: Problem must be defined (--fd or --ma).\n");
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

static int progress(const plan_search_stat_t *stat, void *data)
{
    if (data != NULL){
        fprintf(stderr, "%02d::", *(int *)data);
    }

    fprintf(stderr, "[%.3f s, %ld kb] %ld steps, %ld evaluated,"
                    " %ld expanded, %ld generated, not-found: %d,"
                    " found: %d\n",
            stat->elapsed_time, stat->peak_memory,
            stat->steps, stat->evaluated_states, stat->expanded_states,
            stat->generated_states,
            stat->not_found, stat->found);

    if (max_time > 0 && stat->elapsed_time > max_time){
        fprintf(stderr, "Abort: Exceeded max-time.\n");
        printf("Abort: Exceeded max-time.\n");
        return PLAN_SEARCH_ABORT;
    }

    if (max_mem > 0 && stat->peak_memory > max_mem){
        fprintf(stderr, "Abort: Exceeded max-mem.\n");
        printf("Abort: Exceeded max-mem.\n");
        return PLAN_SEARCH_ABORT;
    }

    return PLAN_SEARCH_CONT;
}

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
                               plan_problem_t *prob,
                               const plan_operator_t *op, int op_size,
                               const plan_succ_gen_t *succ_gen)
{
    plan_heur_t *heur = NULL;

    if (strcmp(name, "goalcount") == 0){
        heur = planHeurGoalCountNew(prob->goal);
    }else if (strcmp(name, "add") == 0){
        heur = planHeurRelaxAddNew(prob->var, prob->var_size,
                                   prob->goal, op, op_size, succ_gen);
    }else if (strcmp(name, "max") == 0){
        heur = planHeurRelaxMaxNew(prob->var, prob->var_size,
                                   prob->goal, op, op_size, succ_gen);
    }else if (strcmp(name, "ff") == 0){
        heur = planHeurRelaxFFNew(prob->var, prob->var_size,
                                  prob->goal, op, op_size, succ_gen);
    }else if (strcmp(name, "lm-cut") == 0){
        heur = planHeurLMCutNew(prob->var, prob->var_size,
                                prob->goal, op, op_size, succ_gen);
    }else{
        fprintf(stderr, "Error: Invalid heuristic type: `%s'\n", name);
    }

    return heur;
}

static plan_search_t *searchCreate(const char *search_name,
                                   const char *heur_name,
                                   const char *list_name,
                                   plan_problem_t *prob,
                                   const plan_operator_t *op, int op_size,
                                   const plan_succ_gen_t *succ_gen,
                                   void *progress_data)
{
    plan_search_t *search = NULL;
    plan_search_params_t *params;
    plan_search_ehc_params_t ehc_params;
    plan_search_lazy_params_t lazy_params;
    plan_search_astar_params_t astar_params;

    if (strcmp(search_name, "ehc") == 0){
        planSearchEHCParamsInit(&ehc_params);
        ehc_params.heur = heurCreate(heur_name, prob, op, op_size, succ_gen);
        ehc_params.heur_del = 1;
        ehc_params.use_preferred_ops = use_preferred_ops;
        params = &ehc_params.search;

    }else if (strcmp(search_name, "lazy") == 0){
        planSearchLazyParamsInit(&lazy_params);
        lazy_params.heur = heurCreate(heur_name, prob, op, op_size, succ_gen);
        lazy_params.heur_del = 1;
        lazy_params.use_preferred_ops = use_preferred_ops;
        lazy_params.list = listLazyCreate(list_name);
        lazy_params.list_del = 1;
        params = &lazy_params.search;

    }else if (strcmp(search_name, "astar") == 0){
        planSearchAStarParamsInit(&astar_params);
        astar_params.heur = heurCreate(heur_name, prob, op, op_size, succ_gen);
        astar_params.heur_del = 1;
        astar_params.pathmax = use_pathmax;
        params = &astar_params.search;

    }else{
        fprintf(stderr, "Error: Unkown search algorithm: `%s'\n",
                search_name);
        return NULL;
    }

    params->progress_fn = progress;
    params->progress_freq = progress_freq;
    params->progress_data = progress_data;
    params->prob = prob;

    if (strcmp(def_search, "ehc") == 0){
        search = planSearchEHCNew(&ehc_params);
    }else if (strcmp(def_search, "lazy") == 0){
        search = planSearchLazyNew(&lazy_params);
    }else if (strcmp(def_search, "astar") == 0){
        search = planSearchAStarNew(&astar_params);
    }

    return search;
}

static void printProblem(const plan_problem_t *prob,
                         const plan_problem_agents_t *ma_prob)
{
    int i;

    printf("Num variables: %d\n", prob->var_size);
    printf("Num operators: %d\n", prob->op_size);
    printf("Bytes per state: %d\n",
           planStatePackerBufSize(prob->state_pool->packer));
    printf("Size of state id: %d\n", (int)sizeof(plan_state_id_t));
    if (ma_prob){
        printf("Num agents: %d\n", ma_prob->agent_size);
        for (i = 0; i < ma_prob->agent_size; ++i){
            printf("Agent[%d]\n", i);
            printf("    name: %s\n", ma_prob->agent[i].name);
            printf("    num operators: %d\n",
                    ma_prob->agent[i].prob.op_size);
            printf("    projected operators: %d\n",
                    ma_prob->agent[i].projected_op_size);
        }
    }
    printf("\n");
}


static int runSingleThread(plan_problem_t *prob)
{
    plan_search_t *search;
    plan_path_t path;
    FILE *fout;
    int res;

    search = searchCreate(def_search, def_heur, def_list, prob,
                          prob->op, prob->op_size, prob->succ_gen, NULL);
    if (search == NULL)
        return -1;

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
        printf("Solution NOT found.\n");

    }else if (res == PLAN_SEARCH_ABORT){
        printf("Search Aborted.\n");
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

    return 0;
}

static int runMA(plan_problem_agents_t *ma_prob)
{
    plan_search_t *search[ma_prob->agent_size];
    int agent_id[ma_prob->agent_size];
    plan_operator_t *heur_op;
    int heur_op_size;
    plan_succ_gen_t *heur_succ_gen;
    plan_ma_agent_path_op_t *path = NULL;
    int path_size = 0;
    int i, res;
    plan_cost_t cost;
    FILE *fout;

    for (i = 0; i < ma_prob->agent_size; ++i){
        agent_id[i] = i;

        if (strcmp(def_ma_heur_op, "global") == 0){
            heur_op       = ma_prob->prob.op;
            heur_op_size  = ma_prob->prob.op_size;
            heur_succ_gen = ma_prob->prob.succ_gen;

        }else if (strcmp(def_ma_heur_op, "projected") == 0){
            heur_op       = ma_prob->agent[i].projected_op;
            heur_op_size  = ma_prob->agent[i].projected_op_size;
            heur_succ_gen = NULL;

        }else if (strcmp(def_ma_heur_op, "local") == 0){
            heur_op       = ma_prob->agent[i].prob.op;
            heur_op_size  = ma_prob->agent[i].prob.op_size;
            heur_succ_gen = ma_prob->agent[i].prob.succ_gen;

        }else{
            return -1;
        }

        search[i] = searchCreate(def_search, def_heur, def_list,
                                 &ma_prob->agent[i].prob,
                                 heur_op, heur_op_size, heur_succ_gen,
                                 &agent_id[i]);
        if (search[i] == NULL)
            return -1;
    }

    res = planMARun(ma_prob->agent_size, search, &path, &path_size);
    if (res == PLAN_SEARCH_FOUND){
        printf("Solution found.\n");

        if (plan_output_fn != NULL){
            fout = fopen(plan_output_fn, "w");
            if (fout != NULL){
                for (i = 0; i < path_size; ++i){
                    fprintf(fout, "(%s)\n", path[i].name);
                }
                fclose(fout);
                printf("Plan written to `%s'\n", plan_output_fn);

            }else{
                fprintf(stderr, "Error: Could not plan write to `%s'\n",
                        plan_output_fn);
            }
        }

        cost = 0;
        for (i = 0; i < path_size; ++i)
            cost += path[i].cost;
        printf("Path Cost: %d\n", (int)cost);

        if (path)
            planMAAgentPathFree(path, path_size);

    }else if (res == PLAN_SEARCH_NOT_FOUND){
        printf("Solution NOT found.\n");

    }else if (res == PLAN_SEARCH_ABORT){
        printf("Search Aborted.\n");
    }

    for (i = 0; i < ma_prob->agent_size; ++i){
        printf("Agent[%d] stats:\n", i);
        printf("    Search Time: %f\n", search[i]->stat.elapsed_time);
        printf("    Steps: %ld\n", search[i]->stat.steps);
        printf("    Evaluated States: %ld\n", search[i]->stat.evaluated_states);
        printf("    Expanded States: %ld\n", search[i]->stat.expanded_states);
        printf("    Generated States: %ld\n", search[i]->stat.generated_states);
        printf("    Peak Memory: %ld kb\n", search[i]->stat.peak_memory);
        planSearchDel(search[i]);
    }

    return 0;
}

int main(int argc, char *argv[])
{
    plan_problem_t *prob = NULL;
    plan_problem_agents_t *ma_prob = NULL;
    bor_timer_t timer;
    int res;

    if (readOpts(argc, argv) != 0){
        usage(argv[0]);
        return -1;
    }

    borTimerStart(&timer);

    printf("FD Problem: %s\n", def_fd_problem);
    printf("MA Problem: %s\n", def_ma_problem);
    printf("Search: %s\n", def_search);
    printf("List: %s\n", def_list);
    printf("Heur: %s\n", def_heur);
    printf("Use preferred operators: ");
    if (use_preferred_ops == PLAN_SEARCH_PREFERRED_NONE){
        printf("Off\n");
    }else if (use_preferred_ops == PLAN_SEARCH_PREFERRED_PREF){
        printf("Pref\n");
    }else if (use_preferred_ops == PLAN_SEARCH_PREFERRED_ONLY){
        printf("Only\n");
    }
    printf("Max Time: %d s\n", max_time);
    printf("Max Mem: %d kb\n", max_mem);
    printf("MA heuristic operators: %s\n", def_ma_heur_op);
    printf("Pathmax: %d\n", use_pathmax);
    printf("\n");

    if (def_fd_problem){
        prob = planProblemFromFD(def_fd_problem);
    }else if (def_ma_problem){
        ma_prob = planProblemAgentsFromFD(def_ma_problem);
    }

    if (prob != NULL){
        printProblem(prob, NULL);
    }else if (ma_prob != NULL){
        printProblem(&ma_prob->prob, ma_prob);
    }else{
        return -1;
    }

    borTimerStop(&timer);
    printf("Loading Time: %f s\n", borTimerElapsedInSF(&timer));
    printf("\n");

    if (prob){
        if ((res = runSingleThread(prob)) != 0)
            return res;

    }else if (ma_prob){
        if ((res = runMA(ma_prob)) != 0)
            return res;
    }

    if (prob){
        planProblemDel(prob);
    }else if (ma_prob){
        planProblemAgentsDel(ma_prob);
    }

    borTimerStop(&timer);
    printf("Overall Time: %f s\n", borTimerElapsedInSF(&timer));

    optsClear();

    return 0;
}
