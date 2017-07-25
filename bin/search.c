/***
 * maplan
 * -------
 * Copyright (c)2015 Daniel Fiser <danfis@danfis.cz>,
 * Agent Technology Center, Department of Computer Science,
 * Faculty of Electrical Engineering, Czech Technical University in Prague.
 * All rights reserved.
 *
 * This file is part of maplan.
 *
 * Distributed under the OSI-approved BSD License (the "License");
 * see accompanying file BDS-LICENSE for details or see
 * <http://www.opensource.org/licenses/bsd-license.php>.
 *
 * This software is distributed WITHOUT ANY WARRANTY; without even the
 * implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the License for more information.
 */

#include <sys/resource.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <signal.h>
#include <boruvka/tasks.h>
#include <boruvka/alloc.h>
#include <plan/ma_msg.h>
#include <plan/problem.h>
#include <plan/search.h>
#include <plan/ma_search.h>

#include "options.h"

static plan_problem_t *problem = NULL;
static plan_problem_t problem2;
static plan_problem_agents_t *agent_problem = NULL;
static plan_problem_t **problems = NULL;
static int problems_size = 0;
static plan_ma_comm_inproc_pool_t *comm_pool = NULL;

struct _progress_t {
    int max_time;
    int max_mem;
    int agent_id;
};
typedef struct _progress_t progress_t;

struct _ma_t {
    int agent_id;
    const options_t *opts;
    plan_search_t *search;
    plan_ma_comm_t *comm;
    plan_path_t path;
    progress_t progress_data;
    int res;
};
typedef struct _ma_t ma_t;

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
                            " (elapsed %f, limit: %d).\n",
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


static void limitMonitorSignalKill(int signum)
{
    fprintf(stderr, "Caught %s signal.\n", strsignal(signum));
    limitMonitorAbort();
}

static void limitMonitorStart(int sleeptime, int max_time, int max_mem)
{
    struct sigaction s;

    limit_monitor.initialized = 1;
    pthread_mutex_init(&limit_monitor.lock, NULL);

    limit_monitor.sleeptime = sleeptime;
    // Give the hard limit monitor 2 minutes more
    limit_monitor.max_time = max_time + (2 * 60);
    limit_monitor.max_mem = max_mem;
    borTimerStart(&limit_monitor.timer);

    limit_monitor.search = NULL;
    limit_monitor.ma_search_size = 0;

    bzero(&s, sizeof(s));
    s.sa_handler = limitMonitorSignalKill;
    sigaction(SIGTERM, &s, NULL);
    sigaction(SIGINT, &s, NULL);

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
    if (prob->agent_name != NULL){
        printf("Agent name: %s\n", prob->agent_name);
        printf("Agent ID: %d\n", prob->agent_id);
        printf("Num agents: %d\n", prob->num_agents);
    }
    if (prob->proj_op != NULL){
        printf("Num projected operators: %d\n", prob->proj_op_size);
    }
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

static int cmpProblems(const void *a, const void *b)
{
    const plan_problem_t *p1 = *(const plan_problem_t **)a;
    const plan_problem_t *p2 = *(const plan_problem_t **)b;
    return p1->agent_id - p2->agent_id;
}

static char **protoFilesInDir(const char *dirname, int *sizeout)
{
    char **files = NULL;
    int size = 0;
    int dnlen, fnlen;
    DIR *dir;
    struct dirent *ent;

    *sizeout = -1;
    dir = opendir(dirname);
    if (dir == NULL){
        fprintf(stderr, "Error: Could not open directory `%s' (%s)\n",
                dirname, strerror(errno));
        return NULL;
    }

    dnlen = strlen(dirname);
    while ((ent = readdir(dir)) != NULL){
        fnlen = strlen(ent->d_name);
        if (fnlen >= 7 && strcmp(ent->d_name + fnlen - 6, ".proto") == 0){
            files = BOR_REALLOC_ARR(files, char *, ++size);
            files[size - 1] = BOR_ALLOC_ARR(char, dnlen + fnlen + 2);
            sprintf(files[size - 1], "%s/%s", dirname, ent->d_name);
        }
    }
    closedir(dir);

    *sizeout = size;
    return files;
}

static int loadProblemMAFactorDir(const options_t *o)
{
    char **files;
    int i, files_size, flags;

    files = protoFilesInDir(o->proto, &files_size);
    if (files == NULL){
        if (files_size == 0)
            fprintf(stderr, "Error: Could not find any .proto files in"
                            " directory `%s'.\n", o->proto);
        return -1;
    }

    flags  = PLAN_PROBLEM_MA_STATE_PRIVACY;
    flags |= PLAN_PROBLEM_NUM_AGENTS(files_size);

    problems_size = files_size;
    problems = BOR_ALLOC_ARR(plan_problem_t *, problems_size);
    for (i = 0; i < files_size; ++i){
        printf("Loading `%s'...\n", files[i]);
        problems[i] = planProblemFromProto(files[i], flags);
        if (problems[i] == NULL){
            fprintf(stderr, "Error: Could not load problem definition from"
                            " the file `%s'.\n", files[i]);
            return -1;
        }
        printProblem(problems[i]);
    }

    qsort(problems, problems_size, sizeof(plan_problem_t *), cmpProblems);

    for (i = 0; i < files_size; ++i)
        BOR_FREE(files[i]);
    if (files)
        BOR_FREE(files);
    return 0;
}

static int loadProblemAgents(const options_t *o)
{
    int flags;

    flags = PLAN_PROBLEM_USE_CG;
    agent_problem = planProblemAgentsFromProto(o->proto, flags);

    if (agent_problem->agent_size <= 1){
        // TODO: Maybe only warning and switch to single-agent mode.
        fprintf(stderr, "Error: Cannot run multi-agent planner on one"
                        " agent problem. It's just silly.\n");
        return -1;
    }

    if (agent_problem == NULL){
        fprintf(stderr, "Error: Could not load file `%s'\n", o->proto);
        return -1;
    }

    printProblemMA(agent_problem);
    return 0;
}

static int loadProblemSeq(const options_t *o)
{
    int flags;

    flags = PLAN_PROBLEM_USE_CG;
    problem = NULL;
    if (o->proto != NULL){
        problem = planProblemFromProto(o->proto, flags);
        if (problem == NULL){
            fprintf(stderr, "Error: Could not load file `%s'\n", o->proto);
            return -1;
        }
    }else if (o->fd != NULL){
        problem = planProblemFromFD(o->fd);
        if (problem == NULL){
            fprintf(stderr, "Error: Could not load file `%s'\n", o->fd);
            return -1;
        }
    }

    printProblem(problem);
    return 0;
}

static int loadProblemMAFactor(const options_t *o)
{
    int flags;

    flags  = PLAN_PROBLEM_MA_STATE_PRIVACY;
    flags |= PLAN_PROBLEM_NUM_AGENTS(o->tcp_size);

    problem = planProblemFromProto(o->proto, flags);
    if (problem == NULL){
        fprintf(stderr, "Error: Could not load file `%s'\n", o->proto);
        return -1;
    }

    if (problem->agent_id != o->tcp_id){
        fprintf(stderr, "Error: Agent ID from .proto file (%d) differs from"
                        " agent ID specified by --tcp-id (%d)!\n",
                        problem->agent_id, o->tcp_id);
        return -1;
    }

    printProblem(problem);
    return 0;
}

static int loadProblem(const options_t *o)
{
    bor_timer_t load_timer;
    int ret;

    borTimerStart(&load_timer);

    if (o->ma_factor){
        ret = loadProblemMAFactor(o);
    }else if (o->ma_factor_dir){
        ret = loadProblemMAFactorDir(o);
    }else if (o->ma_unfactor){
        ret = loadProblemAgents(o);
    }else{
        ret = loadProblemSeq(o);
    }

    borTimerStop(&load_timer);
    printf("\nLoading Time: %f\n", borTimerElapsedInSF(&load_timer));
    printf("\n");

    return ret;
}

static void delProblem(void)
{
    int i;

    if (problem != NULL)
        planProblemDel(problem);
    if (agent_problem != NULL)
        planProblemAgentsDel(agent_problem);
    for (i = 0; i < problems_size; ++i)
        planProblemDel(problems[i]);
    if (problems != NULL)
        BOR_FREE(problems);
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

static plan_heur_t *_heurNew(const options_t *o,
                             const char *name,
                             const plan_problem_t *prob)
{
    plan_state_t *state;
    plan_heur_t *heur = NULL;
    unsigned flags = 0;
    unsigned flags2 = 0;

    if (optionsHeurOpt(o, "op-cost1"))
        flags |= PLAN_HEUR_OP_UNIT_COST;
    if (optionsHeurOpt(o, "op-cost+1"))
        flags |= PLAN_HEUR_OP_COST_PLUS_ONE;

    if (strcmp(name, "goalcount") == 0){
        heur = planHeurGoalCountNew(prob->goal);
    }else if (strcmp(name, "add") == 0){
        heur = planHeurAddNew(prob, flags);
    }else if (strcmp(name, "relax-add") == 0){
        heur = planHeurRelaxAddNew(prob, flags);
    }else if (strcmp(name, "max") == 0){
        heur = planHeurMaxNew(prob, flags);
    }else if (strcmp(name, "relax-max") == 0){
        heur = planHeurRelaxMaxNew(prob, flags);
    }else if (strcmp(name, "ff") == 0){
        heur = planHeurRelaxFFNew(prob, flags);
    }else if (strcmp(name, "dtg") == 0){
        heur = planHeurDTGNew(prob, 0);
    }else if (strcmp(name, "max2") == 0){
        heur = planHeurH2MaxNew(prob, flags);
    }else if (strcmp(name, "lm-cut") == 0){
        heur = planHeurLMCutNew(prob, flags);
    }else if (strcmp(name, "relax-lm-cut") == 0){
        heur = planHeurRelaxLMCutNew(prob, flags);
    }else if (strcmp(name, "lm-cut-inc-local") == 0){
        heur = planHeurLMCutIncLocalNew(prob, flags);
    }else if (strcmp(name, "relax-lm-cut-inc-local") == 0){
        heur = planHeurRelaxLMCutIncLocalNew(prob, flags);
    }else if (strcmp(name, "lm-cut-inc-cache") == 0){
        if (optionsHeurOpt(o, "prune"))
            flags2 |= PLAN_LANDMARK_CACHE_PRUNE;
        heur = planHeurLMCutIncCacheNew(prob, flags, flags2);
    }else if (strcmp(name, "relax-lm-cut-inc-cache") == 0){
        if (optionsHeurOpt(o, "prune"))
            flags2 |= PLAN_LANDMARK_CACHE_PRUNE;
        heur = planHeurRelaxLMCutIncCacheNew(prob, flags, flags2);
    }else if (strcmp(name, "lm-cut2") == 0){
        heur = planHeurLMCut2New(prob, flags);
    }else if (strcmp(name, "flow") == 0){
        if (optionsHeurOpt(o, "ilp"))
            flags |= PLAN_HEUR_FLOW_ILP;
        if (optionsHeurOpt(o, "lm-cut"))
            flags |= PLAN_HEUR_FLOW_LANDMARKS_LM_CUT;
        heur = planHeurFlowNew(prob, flags);
    }else if (strcmp(name, "pot") == 0){
        if (optionsHeurOpt(o, "all-synt-states"))
            flags |= PLAN_HEUR_POT_ALL_SYNTACTIC_STATES;
        state = planStateNew(prob->state_pool->num_vars);
        planStatePoolGetState(prob->state_pool, prob->initial_state, state);
        heur = planHeurPotentialNew(prob, state, flags);
        planStateDel(state);
    }else if (strcmp(name, "ma-max") == 0){
        heur = planHeurMARelaxMaxNew(prob, flags);
    }else if (strcmp(name, "ma-ff") == 0){
        heur = planHeurMARelaxFFNew(prob);
    }else if (strcmp(name, "ma-lm-cut") == 0){
        heur = planHeurMALMCutNew(prob);
    }else if (strcmp(name, "ma-dtg") == 0){
        heur = planHeurMADTGNew(prob);
    }else if (strcmp(name, "ma-pot") == 0){
        if (optionsHeurOpt(o, "all-synt-states"))
            flags |= PLAN_HEUR_POT_ALL_SYNTACTIC_STATES;
        if (optionsHeurOpt(o, "encrypt-off"))
            flags |= PLAN_HEUR_POT_ENCRYPTION_OFF;
        if (optionsHeurOpt(o, "print-init-time"))
            flags |= PLAN_HEUR_POT_PRINT_INIT_TIME;
        heur = planHeurMAPotNew(prob, flags);
    }else if (strcmp(name, "ma-pot-proj") == 0){
        if (optionsHeurOpt(o, "all-synt-states"))
            flags |= PLAN_HEUR_POT_ALL_SYNTACTIC_STATES;
        heur = planHeurMAPotProjNew(prob, flags);
    }else{
        fprintf(stderr, "Error: Invalid heuristic type: `%s'\n", name);
    }

    return heur;
}

static plan_heur_t *heurNew(const options_t *o,
                            const plan_problem_t *prob)
{
    plan_heur_t *heur;

    heur = _heurNew(o, o->heur, prob);
    return heur;
}

static plan_heur_t *heurNewMA(const options_t *o,
                              const plan_problem_t *prob,
                              const plan_problem_t *glob)
{
    plan_heur_t *heur;

    if (optionsHeurOpt(o, "loc")){
        heur = _heurNew(o, o->heur, prob);

    }else if (optionsHeurOpt(o, "glob")){
        if (glob == NULL){
            fprintf(stderr, "Error: Heuristic based on global operators"
                            " (:glob option) cannot be created.\n");
            return NULL;
        }
        planProblemGlobOps(&problem2, prob, glob);
        heur = _heurNew(o, o->heur, &problem2);

    }else{
        planProblemProj(&problem2, prob);
        heur = _heurNew(o, o->heur, &problem2);
    }

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



static void maRun(int agent_id, ma_t *ma)
{
    plan_ma_search_params_t params;
    plan_ma_search_t *ma_search;

    planMASearchParamsInit(&params);
    params.comm = ma->comm;
    params.search = ma->search;
    if (strcmp(ma->opts->search, "astar") == 0){
        params.verify_solution = 1;
    }else{
        params.verify_solution = 0;
    }

    ma_search = planMASearchNew(&params);
    limitMonitorAddMASearch(ma_search);
    ma->res = planMASearchRun(ma_search, &ma->path);
    planMASearchDel(ma_search);
}

static plan_ma_comm_t *maInproc(int agent_id, int agent_size)
{
    if (comm_pool == NULL)
        comm_pool = planMACommInprocPoolNew(agent_size);
    return planMACommInprocNew(comm_pool, agent_id);
}

static int maInit(ma_t *ma, int agent_id, const options_t *o,
                  plan_problem_t *prob, plan_problem_t *globprob)
{
    plan_heur_t *heur;

    ma->agent_id = agent_id;
    ma->opts = o;
    ma->progress_data.max_time = o->max_time;
    ma->progress_data.max_mem = o->max_mem;
    ma->progress_data.agent_id = agent_id;

    heur = heurNewMA(o, prob, globprob);
    if (heur == NULL)
        return -1;

    ma->search = searchNew(o, prob, heur, &ma->progress_data);

    planPathInit(&ma->path);

    if (o->tcp_id >= 0){
        ma->comm = planMACommTCPNew(agent_id, o->tcp_size,
                                    (const char **)o->tcp, 0);
    }else if (agent_problem){
        ma->comm = maInproc(agent_id, agent_problem->agent_size);
    }else if (problems_size > 0){
        ma->comm = maInproc(agent_id, problems_size);
    }else{
        fprintf(stderr, "Error: Cannot create communication channel.\n");
        return -1;
    }

    if (ma->comm == NULL){
        fprintf(stderr, "Error: Cannot create a communication channel.\n");
        return -1;
    }

    return 0;
}

static int maInitUnfactored(ma_t *ma, int agent_id, const options_t *o)
{

    return maInit(ma, agent_id, o, agent_problem->agent + agent_id,
                  &agent_problem->glob);
}

static int maInitFactored(ma_t *ma, int agent_id, const options_t *o)
{
    if (problem != NULL)
        return maInit(ma, agent_id, o, problem, NULL);
    return maInit(ma, agent_id, o, problems[agent_id], NULL);
}

static void maFree(ma_t *ma)
{
    planPathFree(&ma->path);
    planSearchDel(ma->search);
    planMACommDel(ma->comm);
}

static void maPrintResults(ma_t *ma, int size, const options_t *o)
{
    plan_path_t path_factored;
    int i;

    printf("\n");

    // All agents know the result
    if (o->ma_factor && size == 1){
        planPathCopyFactored(&path_factored, &ma[0].path, ma[0].agent_id);
        printResults(o, ma[0].res, &path_factored);
        planPathFree(&path_factored);
    }else{
        printResults(o, ma[0].res, &ma[0].path);
    }
    for (i = 0; i < size; ++i)
        printInitHeurMA(o, ma[i].search, ma[i].agent_id);

    printf("\n");
    for (i = 0; i < size; ++i){
        printf("Agent[%d] stats:\n", ma[i].agent_id);
        printStat(&ma[i].search->stat, "    ");
    }
}



static void maThRun(int id, void *data, const bor_tasks_thinfo_t *_)
{
    maRun(id, (ma_t *)data);
}

static int maUnfactoredThread(const options_t *o)
{
    bor_tasks_t *tasks;
    int agent_size = agent_problem->agent_size;
    ma_t ma[agent_size];
    int i;

    for (i = 0; i < agent_size; ++i){
        if (maInitUnfactored(ma + i, i, o) != 0)
            return -1;
    }

    tasks = borTasksNew(agent_size);
    for (i = 0; i < agent_size; ++i)
        borTasksAdd(tasks, maThRun, i, ma + i);
    borTasksRun(tasks);
    borTasksDel(tasks);

    maPrintResults(ma, agent_size, o);
    for (i = 0; i < agent_size; ++i)
        maFree(ma + i);

    return 0;
}

static int maUnfactoredTCP(const options_t *o)
{
    int agent_id = o->tcp_id;
    ma_t ma;

    if (o->tcp_id >= o->tcp_size){
        fprintf(stderr, "Error: Invalid definition of tcp-based cluster.\n");
        return -1;
    }

    if (maInitUnfactored(&ma, agent_id, o) != 0)
        return -1;
    maRun(agent_id, &ma);
    maPrintResults(&ma, 1, o);
    maFree(&ma);

    return 0;
}

static int maUnfactored(const options_t *o)
{
    if (o->tcp_id == -1)
        return maUnfactoredThread(o);
    return maUnfactoredTCP(o);
}

static int maFactoredThread(const options_t *o)
{
    bor_tasks_t *tasks;
    int agent_size = problems_size;
    ma_t ma[agent_size];
    int i;

    for (i = 0; i < agent_size; ++i){
        if (maInitFactored(ma + i, i, o) != 0)
            return -1;
    }

    tasks = borTasksNew(agent_size);
    for (i = 0; i < agent_size; ++i)
        borTasksAdd(tasks, maThRun, i, ma + i);
    borTasksRun(tasks);
    borTasksDel(tasks);

    maPrintResults(ma, agent_size, o);
    for (i = 0; i < agent_size; ++i)
        maFree(ma + i);

    return 0;
}

static int maFactoredTCP(const options_t *o)
{
    int agent_id = o->tcp_id;
    ma_t ma;

    if (o->tcp_id >= o->tcp_size){
        fprintf(stderr, "Error: Invalid definition of tcp-based cluster.\n");
        return -1;
    }

    if (maInitFactored(&ma, agent_id, o) != 0)
        return -1;
    maRun(agent_id, &ma);
    maPrintResults(&ma, 1, o);
    maFree(&ma);

    return 0;
}

int main(int argc, char *argv[])
{
    options_t *opts;
    bor_timer_t timer;

    borTimerStart(&timer);

    if ((opts = options(argc, argv)) == NULL)
        return -1;

    if (opts->hard_limit_sleeptime > 0){
        limitMonitorStart(opts->hard_limit_sleeptime,
                          opts->max_time, opts->max_mem);
    }

    if (loadProblem(opts) != 0)
        return -1;
    if (dotGraph(opts) != 0)
        return -1;

    if (opts->ma_unfactor){
        if (maUnfactored(opts) != 0)
            return -1;
    }else if (opts->ma_factor){
        if (maFactoredTCP(opts) != 0)
            return -1;
    }else if (opts->ma_factor_dir){
        if (maFactoredThread(opts) != 0)
            return -1;
    }else{
        if (singleThread(opts) != 0)
            return -1;
    }

    if (opts->hard_limit_sleeptime > 0)
        limitMonitorJoin();

    if (comm_pool != NULL)
        planMACommInprocPoolDel(comm_pool);
    delProblem();
    optionsFree();
    planShutdownProtobuf();

    borTimerStop(&timer);
    printf("\nOverall Time: %f\n", borTimerElapsedInSF(&timer));
    return 0;
}
