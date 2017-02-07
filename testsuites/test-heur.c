#include <stdio.h>
#include <boruvka/timer.h>
#include <plan/problem.h>
#include <plan/heur.h>
#include <plan/search.h>
#include <plan/arr.h>

plan_problem_t *problem = NULL;

static int loadProblem(const char *fn)
{
    int flags;

    flags = PLAN_PROBLEM_USE_CG;
    problem = planProblemFromProto(fn, flags);
    if (problem == NULL){
        fprintf(stderr, "Error: Could not load file `%s'\n", fn);
        return -1;
    }
    return 0;
}

static void loadPlan(const char *fname, plan_arr_int_t *op_ids)
{
    FILE *fin;
    char *line = NULL;
    size_t size = 0;
    ssize_t lsize;
    int i, found;

    fin = fopen(fname, "r");
    if (fin == NULL){
        fprintf(stderr, "Error: Could not open plan file `%s'\n", fname);
        exit(-1);
    }

    while ((lsize = getline(&line, &size, fin)) > 0){
        if (lsize < 3 || line[0] != '(' || line[lsize - 2] != ')')
            continue;

        found = 0;
        line[lsize - 2] = 0;
        for (i = 0; i < problem->op_size; ++i){
            if (strcmp(line + 1, problem->op[i].name) == 0){
                planArrIntAdd(op_ids, i);
                found = 1;
                break;
            }
        }

        if (!found){
            fprintf(stderr, "Error: Could not find operator `%s'\n", line + 1);
            exit(-1);
        }
    }

    fclose(fin);
    if (line != NULL)
        free(line);
}

static int heur(const char *hname, const plan_arr_int_t *plan)
{
    bor_timer_t timer;
    double build_time;
    plan_search_astar_params_t sparams;
    plan_search_t *search;
    plan_heur_t *heur;
    plan_heur_res_t res;
    PLAN_STATE_STACK(state, problem->var_size);
    plan_state_id_t state_id;
    plan_state_space_node_t *node;
    int op_id;

    borTimerStart(&timer);

    planStatePoolGetState(problem->state_pool, problem->initial_state, &state);

    if (strcmp(hname, "goalcount") == 0){
        heur = planHeurGoalCountNew(problem->goal);
    }else if (strcmp(hname, "add") == 0){
        heur = planHeurAddNew(problem, 0);
    }else if (strcmp(hname, "max") == 0){
        heur = planHeurMaxNew(problem, 0);
    }else if (strcmp(hname, "ff") == 0){
        heur = planHeurRelaxFFNew(problem, 0);
    }else if (strcmp(hname, "lm-cut") == 0){
        heur = planHeurLMCutNew(problem, 0);
    }else if (strcmp(hname, "lm-cut-inc-local") == 0){
        heur = planHeurLMCutIncLocalNew(problem, 0);
    }else if (strcmp(hname, "lm-cut-inc-cache") == 0){
        heur = planHeurLMCutIncCacheNew(problem, 0, 0);
    }else if (strcmp(hname, "max2") == 0){
        heur = planHeurMax2New(problem, 0);
    }else if (strcmp(hname, "lm-cut2") == 0){
        heur = planHeurLMCut2New(problem, 0);
    }else if (strcmp(hname, "flow") == 0){
        heur = planHeurFlowNew(problem, 0);
    }else if (strcmp(hname, "flow-lm-cut") == 0){
        heur = planHeurFlowNew(problem, PLAN_HEUR_FLOW_LANDMARKS_LM_CUT);
    }else if (strcmp(hname, "flow-ilp") == 0){
        heur = planHeurFlowNew(problem, PLAN_HEUR_FLOW_ILP);
    }else if (strcmp(hname, "pot") == 0){
        heur = planHeurPotentialNew(problem, &state, 0);
    }else if (strcmp(hname, "pot-all-synt-states") == 0){
        heur = planHeurPotentialNew(problem, &state,
                                    PLAN_HEUR_POT_ALL_SYNTACTIC_STATES);
    }else{
        fprintf(stderr, "Error: Unknown heuristic: `%s'\n", hname);
        exit(-1);
    }
    borTimerStop(&timer);
    build_time = borTimerElapsedInSF(&timer);
    printf("%.8f", build_time);

    planSearchAStarParamsInit(&sparams);
    sparams.search.heur = heur;
    sparams.search.heur_del = 1;
    sparams.search.prob = problem;
    search = planSearchAStarNew(&sparams);

    borTimerStart(&timer);
    planHeurResInit(&res);
    planHeurNode(heur, 0, search, &res);
    borTimerStop(&timer);
    if (res.heur == PLAN_HEUR_DEAD_END){
        printf("\t0:DE:%.8f", borTimerElapsedInSF(&timer));
    }else{
        printf("\t0:%d:%.8f", (int)res.heur, borTimerElapsedInSF(&timer));
    }

    search->init_step_fn(search);
    state_id = search->initial_state;
    PLAN_ARR_INT_FOR_EACH(plan, op_id){
        borTimerStart(&timer);
        state_id = planSearchApplyOp(search, state_id, problem->op + op_id);
        borTimerStop(&timer);

        node = planStateSpaceNode(search->state_space, state_id);
        printf("\t%d:", problem->op[op_id].cost);
        if (node->heuristic == PLAN_HEUR_DEAD_END){
            printf("DE");
        }else{
            printf("%d", (int)node->heuristic);
        }
        printf(":%.8f", borTimerElapsedInSF(&timer));
    }
    printf("\n");

    planSearchDel(search);
    return 0;
}

int main(int argc, char *argv[])
{
    plan_arr_int_t plan;

    if (argc != 3 && argc != 4){
        fprintf(stderr, "Usage: %s heur-name problem.proto [problem.plan]\n",
                argv[0]);
        return -1;
    }

    if (loadProblem(argv[2]) != 0)
        return -1;

    planArrIntInit(&plan, 4);
    if (argc >= 4)
        loadPlan(argv[3], &plan);

    if (heur(argv[1], &plan) != 0){
        fprintf(stderr, "Error: %s\n", argv[1]);
        return -1;
    }

    if (problem)
        planProblemDel(problem);
    planArrIntFree(&plan);
    return 0;
}
