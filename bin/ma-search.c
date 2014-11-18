#include <boruvka/timer.h>
#include <plan/search.h>
#include <plan/heur.h>
#include <plan/search.h>
#include <plan/ma_search.h>
#include <plan/ma.h>
#include <opts.h>
#include <boruvka/tasks.h>

struct _th_t {
    int agent_id;
    plan_search_t *search;
    plan_ma_comm_t *comm;
    plan_path_t path;
    int res;
};
typedef struct _th_t th_t;

static char *def_proto_problem = NULL;
static char *def_search = "astar";
static char *def_list = "splaytree";
static char *def_heur = "lm-cut";
static char *def_heur_op = "projected";
static char *plan_output_fn = NULL;
static int max_time = 60 * 30; // 30 minutes
static int max_mem = 1024; // 1GB
static int progress_freq = 1000;
static int use_preferred_ops = 0;
static int use_pathmax = 0;
static char *print_dot_graph = NULL;


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
    optsAddDesc("prob", 'p', OPTS_STR, &def_proto_problem, NULL,
                "Path to .proto definition of multi-agent problem.");
    optsAddDesc("search", 's', OPTS_STR, &def_search, NULL,
                "Define search algorithm [ehc|lazy|astar] (default: astar)");
    optsAddDesc("list", 'l', OPTS_STR, &def_list, NULL,
                "Define list type [heap|bucket|rbtree|splaytree] (default: splaytree)");
    optsAddDesc("heur", 'H', OPTS_STR, &def_heur, NULL,
                "Define heuristic [goalcount|add|max|ff|ma-ff|ma-max|lm-cut] (default: lm-cut)");
    optsAddDesc("preferred-ops", 0x0, OPTS_STR, NULL,
                OPTS_CB(optUsePreferredOps),
                "Enable/disable use of preferred operators: off|pref|only."
                " (default: off)");
    optsAddDesc("heur-op", 0x0, OPTS_STR, &def_heur_op, NULL,
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
                "Maximal memory (peak memory) in MB. (default: 1GB)");
    optsAddDesc("dot-graph", 0x0, OPTS_STR, &print_dot_graph, NULL,
                "Prints problem definition as graph in DOT format in"
                " specified file. (default: None)");

    if (opts(&argc, argv) != 0){
        return -1;
    }

    if (help)
        return -1;

    if (def_proto_problem == NULL){
        fprintf(stderr, "Error: Problem file must be specified.\n");
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
                    " %ld expanded, %ld generated, found: %d\n",
            stat->elapsed_time, stat->peak_memory,
            stat->steps, stat->evaluated_states, stat->expanded_states,
            stat->generated_states,
            stat->found);

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

/*
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
*/

static plan_heur_t *heurCreate(const char *name,
                               const plan_problem_t *prob,
                               const plan_op_t *op, int op_size,
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
    /*
    }else if (strcmp(name, "ma-max") == 0){
        heur = planHeurMARelaxMaxNew(prob);
    }else if (strcmp(name, "ma-ff") == 0){
        heur = planHeurMARelaxFFNew(prob);
    */
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
                                   const plan_op_t *op, int op_size,
                                   const plan_succ_gen_t *succ_gen,
                                   void *progress_data)
{
    plan_search_t *search = NULL;
    plan_search_params_t *params;
    //plan_search_ehc_params_t ehc_params;
    //plan_search_lazy_params_t lazy_params;
    plan_search_astar_params_t astar_params;

    /*
    if (strcmp(search_name, "ehc") == 0){
        planSearchEHCParamsInit(&ehc_params);
        ehc_params.search.heur = heurCreate(heur_name, prob,
                                            op, op_size, succ_gen);
        ehc_params.search.heur_del = 1;
        ehc_params.use_preferred_ops = use_preferred_ops;
        params = &ehc_params.search;

    }else if (strcmp(search_name, "lazy") == 0){
        planSearchLazyParamsInit(&lazy_params);
        lazy_params.search.heur = heurCreate(heur_name, prob,
                                             op, op_size, succ_gen);
        lazy_params.search.heur_del = 1;
        lazy_params.use_preferred_ops = use_preferred_ops;
        lazy_params.list = listLazyCreate(list_name);
        lazy_params.list_del = 1;
        params = &lazy_params.search;

    }else */if (strcmp(search_name, "astar") == 0){
        planSearchAStarParamsInit(&astar_params);
        astar_params.search.heur = heurCreate(heur_name, prob,
                                              op, op_size, succ_gen);
        astar_params.search.heur_del = 1;
        // TODO: Make this an option:
        //astar_params.search.ma_ack_solution = 1;
        astar_params.pathmax = use_pathmax;
        params = &astar_params.search;

    }else{
        fprintf(stderr, "Error: Unkown search algorithm: `%s'\n",
                search_name);
        return NULL;
    }

    params->progress.fn = progress;
    params->progress.freq = progress_freq;
    params->progress.data = progress_data;
    params->prob = prob;

    /*
    if (strcmp(def_search, "ehc") == 0){
        search = planSearchEHCNew(&ehc_params);
    }else if (strcmp(def_search, "lazy") == 0){
        search = planSearchLazyNew(&lazy_params);
    }else*/ if (strcmp(def_search, "astar") == 0){
        search = planSearchAStarNew(&astar_params);
    }

    return search;
}

static void printProblem(const plan_problem_agents_t *prob)
{
    int i;

    printf("Num variables: %d\n", prob->glob.var_size);
    printf("Num operators: %d\n", prob->glob.op_size);
    printf("Bytes per state: %d\n",
           planStatePackerBufSize(prob->glob.state_pool->packer));
    printf("Size of state id: %d\n", (int)sizeof(plan_state_id_t));
    printf("Num agents: %d\n", prob->agent_size);
    for (i = 0; i < prob->agent_size; ++i){
        printf("Agent[%d]\n", i);
        printf("    name: %s\n", prob->agent[i].agent_name);
        printf("    num operators: %d\n", prob->agent[i].op_size);
        printf("    projected operators: %d\n", prob->agent[i].proj_op_size);
    }
    printf("\n");
}


static void printResults(int res, plan_path_t *path)
{
    FILE *fout;

    if (res == PLAN_SEARCH_FOUND){
        printf("Solution found.\n");

        if (plan_output_fn != NULL){
            fout = fopen(plan_output_fn, "w");
            if (fout != NULL){
                planPathPrint(path, fout);
                fclose(fout);
                printf("Plan written to `%s'\n", plan_output_fn);
            }else{
                fprintf(stderr, "Error: Could not plan write to `%s'\n",
                        plan_output_fn);
            }
        }

        printf("Path Cost: %d\n", (int)planPathCost(path));

    }else if (res == PLAN_SEARCH_NOT_FOUND){
        printf("Solution NOT found.\n");

    }else if (res == PLAN_SEARCH_ABORT){
        printf("Search Aborted.\n");
    }
}

static void thRun(int id, void *data, const bor_tasks_thinfo_t *_)
{
    th_t *th = data;
    plan_ma_search_params_t params;
    plan_ma_search_t *ma_search;

    planMASearchParamsInit(&params);
    params.comm = th->comm;
    params.search = th->search;

    ma_search = planMASearchNew(&params);
    th->res = planMASearchRun(ma_search, &th->path);
    planMASearchDel(ma_search);
}

static int run(plan_problem_agents_t *prob)
{
    bor_tasks_t *tasks;
    th_t th[prob->agent_size];
    plan_op_t *heur_op;
    int heur_op_size;
    plan_succ_gen_t *heur_succ_gen;
    plan_ma_comm_queue_pool_t *comm_pool;
    int i, res;

    comm_pool = planMACommQueuePoolNew(prob->agent_size);

    tasks = borTasksNew(prob->agent_size);
    for (i = 0; i < prob->agent_size; ++i){
        th[i].agent_id = i;

        if (strcmp(def_heur_op, "global") == 0){
            heur_op       = prob->glob.op;
            heur_op_size  = prob->glob.op_size;
            heur_succ_gen = prob->glob.succ_gen;

        }else if (strcmp(def_heur_op, "projected") == 0){
            heur_op       = prob->agent[i].proj_op;
            heur_op_size  = prob->agent[i].proj_op_size;
            heur_succ_gen = NULL;

        }else if (strcmp(def_heur_op, "local") == 0){
            heur_op       = prob->agent[i].op;
            heur_op_size  = prob->agent[i].op_size;
            heur_succ_gen = prob->agent[i].succ_gen;

        }else{
            return -1;
        }

        th[i].search = searchCreate(def_search, def_heur, def_list,
                                    prob->agent + i,
                                    heur_op, heur_op_size, heur_succ_gen,
                                    &th[i].agent_id);
        if (th[i].search == NULL)
            return -1;

        planPathInit(&th[i].path);
        th[i].comm = planMACommQueue(comm_pool, i);
        borTasksAdd(tasks, thRun, i, th + i);
    }

    borTasksRun(tasks);
    borTasksDel(tasks);

    res = 0;
    for (i = 0; i < prob->agent_size; ++i){
        if (th[i].res != PLAN_SEARCH_NOT_FOUND){
            printResults(th[i].res, &th[i].path);
            res = 1;
        }
    }
    if (res == 0)
        printResults(PLAN_SEARCH_NOT_FOUND, NULL);

    for (i = 0; i < prob->agent_size; ++i){
        printf("Agent[%d] stats:\n", i);
        printf("    Search Time: %f\n", th[i].search->stat.elapsed_time);
        printf("    Steps: %ld\n", th[i].search->stat.steps);
        printf("    Evaluated States: %ld\n", th[i].search->stat.evaluated_states);
        printf("    Expanded States: %ld\n", th[i].search->stat.expanded_states);
        printf("    Generated States: %ld\n", th[i].search->stat.generated_states);
        printf("    Peak Memory: %ld kb\n", th[i].search->stat.peak_memory);

        planSearchDel(th[i].search);
        planPathFree(&th[i].path);
    }
    planMACommQueuePoolDel(comm_pool);

    return 0;
}

int main(int argc, char *argv[])
{
    plan_problem_agents_t *prob = NULL;
    bor_timer_t timer;
    int res;

    if (readOpts(argc, argv) != 0){
        usage(argv[0]);
        return -1;
    }

    borTimerStart(&timer);

    printf("Problem: %s\n", def_proto_problem);
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
    printf("MA heuristic operators: %s\n", def_heur_op);
    printf("Pathmax: %d\n", use_pathmax);
    printf("\n");

    prob = planProblemAgentsFromProto(def_proto_problem, PLAN_PROBLEM_USE_CG);

    if (prob == NULL){
        printProblem(prob);
        fprintf(stderr, "Error: Could not load problem definition.\n");
        return -1;
    }

    if (print_dot_graph){
        if (prob != NULL){
            FILE *fout = fopen(print_dot_graph, "w");
            if (fout == NULL){
                fprintf(stderr, "Error: Could not open file `%s' for"
                                " writing DOT graph.\n", print_dot_graph);
                return -1;
            }
            planProblemAgentsDotGraph(prob, fout);
            fclose(fout);
            return 0;
        }
    }

    borTimerStop(&timer);
    printf("Loading Time: %f s\n", borTimerElapsedInSF(&timer));
    printf("\n");

    res = run(prob);
    planProblemAgentsDel(prob);

    borTimerStop(&timer);
    printf("Overall Time: %f s\n", borTimerElapsedInSF(&timer));

    optsClear();

    planShutdownProtobuf();

    return res;
}
