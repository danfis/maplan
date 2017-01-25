#include <stdio.h>
#include <boruvka/timer.h>
#include <plan/problem.h>
#include <plan/heur.h>

plan_problem_t *problem = NULL;

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

static int loadProblem(const char *fn)
{
    int flags;

    flags = PLAN_PROBLEM_USE_CG;
    problem = planProblemFromProto(fn, flags);
    if (problem == NULL){
        fprintf(stderr, "Error: Could not load file `%s'\n", fn);
        return -1;
    }

    printProblem(problem);
    return 0;
}

static int heur(const char *hname)
{
    bor_timer_t timer;
    double build_time;
    plan_heur_t *heur;
    plan_heur_res_t res;
    PLAN_STATE_STACK(state, problem->var_size);

    borTimerStart(&timer);

    planStatePoolGetState(problem->state_pool, problem->initial_state, &state);

    if (strcmp(hname, "goalcount") == 0){
        heur = planHeurGoalCountNew(problem->goal);
    }else if (strcmp(hname, "add") == 0){
        heur = planHeurRelaxAddNew(problem->var, problem->var_size,
                                   problem->goal,
                                   problem->op, problem->op_size, 0);
    }else if (strcmp(hname, "max") == 0){
        /*
        heur = planHeurRelaxMaxNew(problem->var, problem->var_size,
                                   problem->goal,
                                   problem->op, problem->op_size, 0);
        */
        heur = planHeurMaxNew(problem, 0);
    }else if (strcmp(hname, "ff") == 0){
        heur = planHeurRelaxFFNew(problem->var, problem->var_size,
                                  problem->goal,
                                  problem->op, problem->op_size, 0);
    }else if (strcmp(hname, "lm-cut") == 0){
        heur = planHeurLMCutNew(problem->var, problem->var_size,
                                problem->goal,
                                problem->op, problem->op_size, 0);
    }else if (strcmp(hname, "max2") == 0){
        heur = planHeurMax2New(problem, 0);
        //heur = planHeurH2MaxNew(problem, 0);
    }else if (strcmp(hname, "lm-cut2") == 0){
        heur = planHeurH2LMCutNew(problem, 0);
    }else if (strcmp(hname, "flow") == 0){
        heur = planHeurFlowNew(problem->var, problem->var_size,
                               problem->goal,
                               problem->op, problem->op_size, 0);
    }else if (strcmp(hname, "flow-lm-cut") == 0){
        heur = planHeurFlowNew(problem->var, problem->var_size,
                               problem->goal,
                               problem->op, problem->op_size,
                               PLAN_HEUR_FLOW_LANDMARKS_LM_CUT);
    }else if (strcmp(hname, "flow-ilp") == 0){
        heur = planHeurFlowNew(problem->var, problem->var_size,
                               problem->goal,
                               problem->op, problem->op_size,
                               PLAN_HEUR_FLOW_ILP);
    }else if (strcmp(hname, "pot") == 0){
        heur = planHeurPotentialNew(problem->var, problem->var_size,
                                    problem->goal,
                                    problem->op, problem->op_size,
                                    &state, 0);
    }else if (strcmp(hname, "pot-all-synt-states") == 0){
        heur = planHeurPotentialNew(problem->var, problem->var_size,
                                    problem->goal,
                                    problem->op, problem->op_size,
                                    &state,
                                    PLAN_HEUR_POT_ALL_SYNTACTIC_STATES);
    }
    borTimerStop(&timer);
    build_time = borTimerElapsedInSF(&timer);

    planHeurResInit(&res);
    planHeurState(heur, &state, &res);

    planHeurDel(heur);
    borTimerStop(&timer);
    printf("%-20s", hname);
    if (res.heur == PLAN_HEUR_DEAD_END){
        printf("\t  DE");
    }else{
        printf("\t% 4d", (int)res.heur);
    }
    printf("\t%.8fs / %.8fs\n", borTimerElapsedInSF(&timer), build_time);
    return 0;
}

#define H(hname) \
    if (heur(hname) != 0){ \
        fprintf(stderr, "Error: %s\n", (hname)); \
        return -1; \
    }

int main(int argc, char *argv[])
{
    bor_timer_t timer;

    if (argc != 2){
        fprintf(stderr, "Usage: %s problem.proto\n", argv[0]);
        return -1;
    }

    borTimerStart(&timer);
    if (loadProblem(argv[1]) != 0)
        return -1;

    printf("\n");
    H("goalcount");
    H("add");
    H("ff");

    printf("\n");
    H("max");
    H("max2");
    H("lm-cut");
    H("lm-cut2");
    H("flow");
    H("flow-ilp");
    H("flow-lm-cut");
    H("pot");
    H("pot-all-synt-states");

    if (problem)
        planProblemDel(problem);
    borTimerStop(&timer);
    printf("\nOverall Time: %f\n", borTimerElapsedInSF(&timer));
    return 0;
}
