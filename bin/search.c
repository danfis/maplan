#include <sys/resource.h>
#include <unistd.h>
#include <boruvka/tasks.h>
#include <plan/ma_msg.h>
#include <plan/problem.h>
#include <plan/search.h>
#include <plan/ma_search.h>

#include "options.h"

static plan_problem_t *problem = NULL;
static plan_problem_agents_t *agent_problem = NULL;

struct _progress_t {
    int max_time;
    int max_mem;
    int agent_id;
};
typedef struct _progress_t progress_t;

struct _ma_th_t {
    int agent_id;
    const options_t *opts;
    plan_search_t *search;
    plan_ma_comm_t *comm;
    plan_path_t path;
    progress_t progress_data;
    int res;
};
typedef struct _ma_th_t ma_th_t;

static struct {
    int initialized;
    pthread_t th;
    pthread_mutex_t lock;
    int sleeptime;
    int max_time;
    int max_mem;

    bor_timer_t timer;
    plan_search_t *search;
    plan_ma_search_t *ma_search[64];
    int ma_search_size;
} limit_monitor;

static void limitMonitorAbort(void)
{
    int i, aborted = 0;

    pthread_mutex_lock(&limit_monitor.lock);
    if (limit_monitor.search){
        planSearchAbort(limit_monitor.search);
        aborted = 1;
    }
    for (i = 0; i < limit_monitor.ma_search_size; ++i){
        planMASearchAbort(limit_monitor.ma_search[i]);
        aborted = 1;
    }
    pthread_mutex_unlock(&limit_monitor.lock);

    if (!aborted)
        exit(-1);
}

static void *limitMonitorTh(void *_)
{
    struct rusage usg;
    int peak_mem;
    float elapsed;

    // Enable cancelability and enable cancelation in sleep() call
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

    while (1){
        borTimerStop(&limit_monitor.timer);
        elapsed = borTimerElapsedInSF(&limit_monitor.timer);
        if ((int)elapsed > limit_monitor.max_time){
            fprintf(stderr, "Aborting due to exceeded hard time limit"
                            " (elapsed %f, limit: %d.\n",
                    elapsed, limit_monitor.max_time);
            fflush(stderr);
            limitMonitorAbort();
            break;
        }

        if (getrusage(RUSAGE_SELF, &usg) == 0){
            peak_mem = usg.ru_maxrss / 1024L;
            if (peak_mem > limit_monitor.max_mem){
                fprintf(stderr, "Aborting due to exceeded hard mem limit"
                                " (peak-mem: %d, limit: %d).\n",
                        peak_mem, limit_monitor.max_mem);
                fflush(stderr);
                limitMonitorAbort();
                break;
            }
        }

        sleep(limit_monitor.sleeptime);
    }
    return NULL;
}

static void limitMonitorStart(int sleeptime, int max_time, int max_mem)
{
    limit_monitor.initialized = 1;
    pthread_mutex_init(&limit_monitor.lock, NULL);

    limit_monitor.sleeptime = sleeptime;
    // Give the hard limit monitor 2 minutes more
    limit_monitor.max_time = max_time + (2 * 60);
    limit_monitor.max_mem = max_mem;
    borTimerStart(&limit_monitor.timer);

    limit_monitor.search = NULL;
    limit_monitor.ma_search_size = 0;

    pthread_create(&limit_monitor.th, NULL, limitMonitorTh, NULL);
}

static void limitMonitorJoin(void)
{
    pthread_cancel(limit_monitor.th);
    pthread_join(limit_monitor.th, NULL);
    pthread_mutex_destroy(&limit_monitor.lock);
}

static void limitMonitorSetSearch(plan_search_t *search)
{
    if (!limit_monitor.initialized)
        return;

    pthread_mutex_lock(&limit_monitor.lock);
    limit_monitor.search = search;
    pthread_mutex_unlock(&limit_monitor.lock);
}

static void limitMonitorAddMASearch(plan_ma_search_t *ma_search)
{
    if (!limit_monitor.initialized)
        return;

    pthread_mutex_lock(&limit_monitor.lock);
    limit_monitor.ma_search[limit_monitor.ma_search_size++] = ma_search;
    pthread_mutex_unlock(&limit_monitor.lock);
}


static int progress(const plan_search_stat_t *stat, void *data)
{
    progress_t *p = (progress_t *)data;

    fprintf(stderr, "%02d:: [%.3f s, %ld MB] %ld steps, %ld evaluated,"
                    " %ld expanded, %ld generated, found: %d\n",
            p->agent_id,
            stat->elapsed_time, stat->peak_memory,
            stat->steps, stat->evaluated_states, stat->expanded_states,
            stat->generated_states,
            stat->found);
    fflush(stderr);

    if (p->max_time > 0 && stat->elapsed_time > p->max_time){
        fprintf(stderr, "%02d:: Abort: Exceeded max-time.\n", p->agent_id);
        fflush(stderr);
        printf("Abort: Exceeded max-time.\n");
        return PLAN_SEARCH_ABORT;
    }

    if (p->max_mem > 0 && stat->peak_memory > p->max_mem){
        fprintf(stderr, "%02d:: Abort: Exceeded max-mem.\n", p->agent_id);
        fflush(stderr);
        printf("Abort: Exceeded max-mem.\n");
        return PLAN_SEARCH_ABORT;
    }

    return PLAN_SEARCH_CONT;
}

static void printProblem(const plan_problem_t *prob)
{
    printf("Num variables: %d\n", prob->var_size);
    printf("Num operators: %d\n", prob->op_size);
    printf("Bytes per state: %d\n",
           planStatePackerBufSize(prob->state_pool->packer));
    printf("Size of state id: %d\n", (int)sizeof(plan_state_id_t));
    printf("Duplicate operators removed: %d\n", prob->duplicate_ops_removed);
    fflush(stdout);
}

static void printProblemMA(const plan_problem_agents_t *prob)
{
    int i;

    printProblem(&prob->glob);
    printf("Num agents: %d\n", prob->agent_size);
    for (i = 0; i < prob->agent_size; ++i){
        printf("Agent[%d]\n", i);
        printf("    name: %s\n", prob->agent[i].agent_name);
        printf("    num operators: %d\n", prob->agent[i].op_size);
        printf("    projected operators: %d\n", prob->agent[i].proj_op_size);
    }
    fflush(stdout);
}

static void printStat(const plan_search_stat_t *stat, const char *prefix)
{
    printf("%sSearch Time: %f\n", prefix, stat->elapsed_time);
    printf("%sSteps: %ld\n", prefix, stat->steps);
    printf("%sEvaluated States: %ld\n", prefix, stat->evaluated_states);
    printf("%sExpanded States: %ld\n", prefix, stat->expanded_states);
    printf("%sGenerated States: %ld\n", prefix, stat->generated_states);
    printf("%sPeak Memory: %ld kb\n", prefix, stat->peak_memory);
    fflush(stdout);
}

static void printResults(const options_t *o, int res, plan_path_t *path)
{
    FILE *fout;

    if (res == PLAN_SEARCH_FOUND){
        printf("Solution found.\n");

        if (o->output != NULL){
            if (strcmp(o->output, "-") == 0){
                planPathPrint(path, stdout);
                printf("Plan written to stdout\n");
            }else{
                fout = fopen(o->output, "w");
                if (fout != NULL){
                    planPathPrint(path, fout);
                    fclose(fout);
                    printf("Plan written to `%s'\n", o->output);
                }else{
                    fprintf(stderr, "Error: Could not plan write to `%s'\n",
                            o->output);
                }
            }
        }

        printf("Plan Cost: %d\n", (int)planPathCost(path));
        printf("Plan Length: %d\n", planPathLen(path));

    }else if (res == PLAN_SEARCH_NOT_FOUND){
        printf("Solution NOT found.\n");

    }else if (res == PLAN_SEARCH_ABORT){
        printf("Search Aborted.\n");
    }
    fflush(stdout);
}

static void printInitHeur(const options_t *o, plan_search_t *search)
{
    if (o->print_heur_init){
        printf("Init State Heur: %d\n",
                (int)planSearchStateHeur(search, search->initial_state));
        fflush(stdout);
    }
}

static void printInitHeurMA(const options_t *o, plan_search_t *search,
                            int agent_id)
{
    if (o->print_heur_init){
        printf("%02d:: Init State Heur: %d\n", agent_id,
                (int)planSearchStateHeur(search, search->initial_state));
        fflush(stdout);
    }
}

static int loadProblem(const options_t *o)
{
    bor_timer_t load_timer;
    int flags;

    borTimerStart(&load_timer);

    flags = PLAN_PROBLEM_USE_CG;
    if (o->op_unit_cost)
        flags |= PLAN_PROBLEM_OP_UNIT_COST;

    if (o->ma_unfactor){
        agent_problem = planProblemAgentsFromProto(o->proto, flags);
        if (agent_problem->agent_size <= 1){
            // TODO: Maybe only warning and switch to single-agent mode.
            fprintf(stderr, "Error: Cannot run multi-agent planner on one"
                    " agent problem. It's just silly.\n");
            return -1;
        }
    }else{
        problem = planProblemFromProto(o->proto, flags);
    }

    if (problem == NULL && agent_problem == NULL){
        fprintf(stderr, "Error: Could not load file `%s'\n", o->proto);
        return -1;
    }else{
        borTimerStop(&load_timer);
        if (problem != NULL)
            printProblem(problem);
        if (agent_problem != NULL)
            printProblemMA(agent_problem);
        printf("\nLoading Time: %f\n", borTimerElapsedInSF(&load_timer));
        printf("\n");
    }

    return 0;
}

static int dotGraph(const options_t *o)
{
    FILE *fout;

    if (!o->dot_graph)
        return 0;

    if (agent_problem == NULL){
        fprintf(stderr, "Error: Can generate dot-graph only for multi-agent"
                        " problem definition.\n");
        return -1;
    }

    fout = fopen(o->dot_graph, "w");
    if (fout){
        planProblemAgentsDotGraph(agent_problem, fout);
        fclose(fout);
    }else{
        fprintf(stderr, "Error: Could not open file `%s'\n",
                o->dot_graph);
        return -1;
    }

    return 0;
}

static plan_list_lazy_t *listLazyCreate(const options_t *o)
{
    plan_list_lazy_t *list = NULL;

    if (optionsSearchOpt(o, "list-heap")){
        list = planListLazyHeapNew();
    }else if (optionsSearchOpt(o, "list-bucket")){
        list = planListLazyBucketNew();
    }else if (optionsSearchOpt(o, "list-rb")){
        list = planListLazyRBTreeNew();
    }else if (optionsSearchOpt(o, "list-splay")){
        list = planListLazySplayTreeNew();
    }else{
        list = planListLazySplayTreeNew();
    }

    return list;
}

static plan_heur_t *_heurNew(const char *name,
                             const plan_problem_t *prob,
                             const plan_op_t *op, int op_size)
{
    plan_heur_t *heur = NULL;

    if (strcmp(name, "goalcount") == 0){
        heur = planHeurGoalCountNew(prob->goal);
    }else if (strcmp(name, "add") == 0){
        heur = planHeurRelaxAddNew(prob->var, prob->var_size,
                                   prob->goal, op, op_size);
    }else if (strcmp(name, "max") == 0){
        heur = planHeurRelaxMaxNew(prob->var, prob->var_size,
                                   prob->goal, op, op_size);
    }else if (strcmp(name, "ff") == 0){
        heur = planHeurRelaxFFNew(prob->var, prob->var_size,
                                  prob->goal, op, op_size);
    }else if (strcmp(name, "dtg") == 0){
        heur = planHeurDTGNew(prob->var, prob->var_size,
                              prob->goal, op, op_size);
    }else if (strcmp(name, "lm-cut") == 0){
        heur = planHeurLMCutNew(prob->var, prob->var_size,
                                prob->goal, op, op_size);
    }else if (strcmp(name, "ma-max") == 0){
        heur = planHeurMARelaxMaxNew(prob);
    }else if (strcmp(name, "ma-ff") == 0){
        heur = planHeurMARelaxFFNew(prob);
    }else if (strcmp(name, "ma-lm-cut") == 0){
        heur = planHeurMALMCutNew(prob);
    }else if (strcmp(name, "ma-dtg") == 0){
        heur = planHeurMADTGNew(prob);
    }else{
        fprintf(stderr, "Error: Invalid heuristic type: `%s'\n", name);
    }

    return heur;
}

static plan_heur_t *heurNew(const options_t *o,
                            const plan_problem_t *prob)
{
    plan_heur_t *heur;

    heur = _heurNew(o->heur, prob, prob->op, prob->op_size);
    return heur;
}

static plan_heur_t *heurNewMA(const options_t *o,
                              const plan_problem_agents_t *prob,
                              int agent_id)
{
    plan_heur_t *heur;
    const plan_op_t *op;
    int op_size;

    if (optionsHeurOpt(o, "loc")){
        op = prob->agent[agent_id].op;
        op_size = prob->agent[agent_id].op_size;

    }else if (optionsHeurOpt(o, "glob")){
        op = prob->glob.op;
        op_size = prob->glob.op_size;

    }else{
        op = prob->agent[agent_id].proj_op;
        op_size = prob->agent[agent_id].proj_op_size;
    }

    heur = _heurNew(o->heur, prob->agent + agent_id, op, op_size);
    return heur;
}

static plan_search_t *searchNew(const options_t *o,
                                plan_problem_t *prob,
                                plan_heur_t *heur,
                                void *progress_data)
{
    plan_search_t *search = NULL;
    plan_search_params_t *params;
    plan_search_ehc_params_t ehc_params;
    plan_search_lazy_params_t lazy_params;
    plan_search_astar_params_t astar_params;
    int use_preferred_ops = PLAN_SEARCH_PREFERRED_NONE;
    int use_pathmax = 0;

    if (optionsSearchOpt(o, "pref")){
        use_preferred_ops = PLAN_SEARCH_PREFERRED_PREF;
    }else if (optionsSearchOpt(o, "pref_only")){
        use_preferred_ops = PLAN_SEARCH_PREFERRED_ONLY;
    }

    if (optionsSearchOpt(o, "pathmax")){
        use_pathmax = 1;
    }

    if (strcmp(o->search, "ehc") == 0){
        planSearchEHCParamsInit(&ehc_params);
        ehc_params.use_preferred_ops = use_preferred_ops;
        params = &ehc_params.search;

    }else if (strcmp(o->search, "lazy") == 0){
        planSearchLazyParamsInit(&lazy_params);
        lazy_params.use_preferred_ops = use_preferred_ops;
        lazy_params.list = listLazyCreate(o);
        lazy_params.list_del = 1;
        params = &lazy_params.search;

    }else if (strcmp(o->search, "astar") == 0){
        planSearchAStarParamsInit(&astar_params);
        astar_params.pathmax = use_pathmax;
        params = &astar_params.search;

    }else{
        return NULL;
    }

    params->heur = heur;
    params->heur_del = 1;
    params->progress.fn = progress;
    params->progress.freq = o->progress_freq;
    params->progress.data = progress_data;
    params->prob = prob;

    if (strcmp(o->search, "ehc") == 0){
        search = planSearchEHCNew(&ehc_params);
    }else if (strcmp(o->search, "lazy") == 0){
        search = planSearchLazyNew(&lazy_params);
    }else if (strcmp(o->search, "astar") == 0){
        search = planSearchAStarNew(&astar_params);
    }

    return search;
}

static int singleThread(const options_t *o)
{
    plan_heur_t *heur;
    plan_search_t *search;
    plan_path_t path;
    progress_t progress_data;
    int res;

    // Create heuristic function
    heur = heurNew(o, problem);

    // Create search algorithm
    progress_data.max_time = o->max_time;
    progress_data.max_mem = o->max_mem;
    progress_data.agent_id = 0;
    search = searchNew(o, problem, heur, &progress_data);
    limitMonitorSetSearch(search);

    // Run search
    planPathInit(&path);
    res = planSearchRun(search, &path);

    printf("\n");
    printResults(o, res, &path);
    printInitHeur(o, search);
    printf("\n");
    printStat(&search->stat, "");
    fflush(stdout);

    planPathFree(&path);
    planSearchDel(search);

    return 0;
}

static void maThRun(int id, void *data, const bor_tasks_thinfo_t *_)
{
    ma_th_t *th = data;
    plan_ma_search_params_t params;
    plan_ma_search_t *ma_search;

    planMASearchParamsInit(&params);
    params.comm = th->comm;
    params.search = th->search;
    if (strcmp(th->opts->search, "astar") == 0){
        params.verify_solution = 1;
    }else{
        params.verify_solution = 0;
    }

    ma_search = planMASearchNew(&params);
    limitMonitorAddMASearch(ma_search);
    th->res = planMASearchRun(ma_search, &th->path);
    planMASearchDel(ma_search);
}

static int maUnfactored(const options_t *o)
{
    bor_tasks_t *tasks;
    ma_th_t th[agent_problem->agent_size];
    plan_heur_t *heur;
    int i;

    tasks = borTasksNew(agent_problem->agent_size);

    for (i = 0; i < agent_problem->agent_size; ++i){
        th[i].agent_id = i;
        th[i].opts = o;
        th[i].progress_data.max_time = o->max_time;
        th[i].progress_data.max_mem = o->max_mem;
        th[i].progress_data.agent_id = i;

        heur = heurNewMA(o, agent_problem, i);
        th[i].search = searchNew(o, agent_problem->agent + i, heur,
                                 &th[i].progress_data);

        planPathInit(&th[i].path);
        th[i].comm = planMACommInprocNew(i, agent_problem->agent_size);
        borTasksAdd(tasks, maThRun, i, th + i);
    }

    borTasksRun(tasks);
    borTasksDel(tasks);


    printf("\n");
    for (i = 0; i < agent_problem->agent_size; ++i){
        if (th[i].res != PLAN_SEARCH_NOT_FOUND){
            printResults(o, th[i].res, &th[i].path);
            break;

        }
    }
    if (i == agent_problem->agent_size)
        printResults(o, PLAN_SEARCH_NOT_FOUND, NULL);
    for (i = 0; i < agent_problem->agent_size; ++i)
        printInitHeurMA(o, th[i].search, i);

    printf("\n");
    for (i = 0; i < agent_problem->agent_size; ++i){
        printf("Agent[%d] stats:\n", i);
        printStat(&th[i].search->stat, "    ");
    }

    for (i = 0; i < agent_problem->agent_size; ++i){
        planPathFree(&th[i].path);
        planSearchDel(th[i].search);
        planMACommDel(th[i].comm);
    }

    return 0;
}

int main(int argc, char *argv[])
{
    options_t opts;
    bor_timer_t timer;

    borTimerStart(&timer);

    if (options(&opts, argc, argv) != 0)
        return -1;

    if (opts.hard_limit_sleeptime > 0){
        limitMonitorStart(opts.hard_limit_sleeptime,
                          opts.max_time, opts.max_mem);
    }

    if (loadProblem(&opts) != 0)
        return -1;
    if (dotGraph(&opts) != 0)
        return -1;

    if (opts.ma_unfactor){
        if (maUnfactored(&opts) != 0)
            return -1;
    }else{
        if (singleThread(&opts) != 0)
            return -1;
    }

    if (opts.hard_limit_sleeptime > 0)
        limitMonitorJoin();

    if (problem != NULL)
        planProblemDel(problem);
    if (agent_problem != NULL)
        planProblemAgentsDel(agent_problem);
    optionsFree(&opts);
    planShutdownProtobuf();

    borTimerStop(&timer);
    printf("\nOverall Time: %f\n", borTimerElapsedInSF(&timer));
    return 0;
}
