#include <plan/ma_msg.h>
#include <plan/problem.h>
#include <plan/search.h>

#include "options.h"

struct _progress_t {
    int max_time;
    int max_mem;
};
typedef struct _progress_t progress_t;

static int progress(const plan_search_stat_t *stat, void *data)
{
    progress_t *p = (progress_t *)data;

    fprintf(stderr, "[%.3f s, %ld MB] %ld steps, %ld evaluated,"
                    " %ld expanded, %ld generated, found: %d\n",
            stat->elapsed_time, stat->peak_memory,
            stat->steps, stat->evaluated_states, stat->expanded_states,
            stat->generated_states,
            stat->found);

    if (p->max_time > 0 && stat->elapsed_time > p->max_time){
        fprintf(stderr, "Abort: Exceeded max-time.\n");
        printf("Abort: Exceeded max-time.\n");
        return PLAN_SEARCH_ABORT;
    }

    if (p->max_mem > 0 && stat->peak_memory > p->max_mem){
        fprintf(stderr, "Abort: Exceeded max-mem.\n");
        printf("Abort: Exceeded max-mem.\n");
        return PLAN_SEARCH_ABORT;
    }

    return PLAN_SEARCH_CONT;
}

static void printProblem(const plan_problem_t *prob)
{
    //int i;

    printf("Num variables: %d\n", prob->var_size);
    printf("Num operators: %d\n", prob->op_size);
    printf("Bytes per state: %d\n",
           planStatePackerBufSize(prob->state_pool->packer));
    printf("Size of state id: %d\n", (int)sizeof(plan_state_id_t));
    /*
    printf("Num agents: %d\n", prob->agent_size);
    for (i = 0; i < prob->agent_size; ++i){
        printf("Agent[%d]\n", i);
        printf("    name: %s\n", prob->agent[i].agent_name);
        printf("    num operators: %d\n", prob->agent[i].op_size);
        printf("    projected operators: %d\n", prob->agent[i].proj_op_size);
    }
    */
    printf("\n");
}

static void printStat(const plan_search_stat_t *stat)
{
    printf("\n");
    printf("Search Time: %f\n", stat->elapsed_time);
    printf("Steps: %ld\n", stat->steps);
    printf("Evaluated States: %ld\n", stat->evaluated_states);
    printf("Expanded States: %ld\n", stat->expanded_states);
    printf("Generated States: %ld\n", stat->generated_states);
    printf("Peak Memory: %ld kb\n", stat->peak_memory);
    fflush(stdout);
}

static void printResults(const options_t *o, int res, plan_path_t *path,
                         plan_search_t *search)
{
    FILE *fout;

    if (res == PLAN_SEARCH_FOUND){
        printf("Solution found.\n");

        if (o->output != NULL){
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

        printf("Path Cost: %d\n", (int)planPathCost(path));

    }else if (res == PLAN_SEARCH_NOT_FOUND){
        printf("Solution NOT found.\n");

    }else if (res == PLAN_SEARCH_ABORT){
        printf("Search Aborted.\n");
    }

    if (o->print_heur_init){
        printf("Init State Heur: %d\n",
                (int)planSearchStateHeur(search, search->initial_state));
    }
}


static plan_list_lazy_t *listLazyCreate(const options_t *o)
{
    plan_list_lazy_t *list = NULL;

    if (optionsSearchOpt(o, "list-heap")){
        list = planListLazyHeapNew();
    }else if (optionsSearchOpt(o, "list-bucket") == 0){
        list = planListLazyBucketNew();
    }else if (optionsSearchOpt(o, "list-rb") == 0){
        list = planListLazyRBTreeNew();
    }else if (optionsSearchOpt(o, "list-splay") == 0){
        list = planListLazySplayTreeNew();
    }else{
        list = planListLazySplayTreeNew();
    }

    return list;
}

static plan_heur_t *_heurNew(const char *name,
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
    }else if (strcmp(name, "lm-cut") == 0){
        heur = planHeurLMCutNew(prob->var, prob->var_size,
                                prob->goal, op, op_size, succ_gen);
    /*
    }else if (strcmp(name, "ma-max") == 0){
        heur = planHeurMARelaxMaxNew(prob);
    */
    }else if (strcmp(name, "ma-ff") == 0){
        heur = planHeurMARelaxFFNew(prob);
    }else{
        fprintf(stderr, "Error: Invalid heuristic type: `%s'\n", name);
    }

    return heur;
}

static plan_heur_t *heurNew(const options_t *o,
                            const plan_problem_t *prob)
{
    plan_heur_t *heur;

    heur = _heurNew(o->heur, prob, prob->op, prob->op_size,
                    prob->succ_gen);
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
    plan_problem_t *prob;
    plan_heur_t *heur;
    plan_search_t *search;
    plan_path_t path;
    progress_t progress_data;
    int res;

    // Load problem file
    prob = planProblemFromProto(o->proto, PLAN_PROBLEM_USE_CG);
    if (prob == NULL){
        fprintf(stderr, "Error: Could not load file `%s'\n", o->proto);
        return -1;
    }else{
        printProblem(prob);
    }

    // Create heuristic function
    heur = heurNew(o, prob);

    // Create search algorithm
    progress_data.max_time = o->max_time;
    progress_data.max_mem = o->max_mem;
    search = searchNew(o, prob, heur, &progress_data);

    // Run search
    planPathInit(&path);
    res = planSearchRun(search, &path);
    printResults(o, res, &path, search);
    printStat(&search->stat);

    planPathFree(&path);
    planSearchDel(search);

    return 0;
}

int main(int argc, char *argv[])
{
    options_t opts;
    bor_timer_t timer;

    borTimerStart(&timer);

    if (options(&opts, argc, argv) != 0)
        return -1;

    if (singleThread(&opts) != 0)
        return -1;

    optionsFree(&opts);
    planShutdownProtobuf();

    borTimerStop(&timer);
    printf("Overall Time: %f\n", borTimerElapsedInSF(&timer));
    return 0;
}
