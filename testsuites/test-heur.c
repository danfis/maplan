#include <stdio.h>
#include <boruvka/timer.h>
#include <plan/problem.h>
#include <plan/heur.h>

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
        /*
        heur = planHeurLMCutNew(problem->var, problem->var_size,
                                problem->goal,
                                problem->op, problem->op_size, 0);
        */
        heur = planHeurLMCutXNew(problem, 0);
    }else if (strcmp(hname, "max2") == 0){
        heur = planHeurMax2New(problem, 0);
        //heur = planHeurH2MaxNew(problem, 0);
    }else if (strcmp(hname, "lm-cut2") == 0){
        //heur = planHeurH2LMCutNew(problem, 0);
        heur = planHeurLMCut2New(problem, 0);
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
    }else{
        fprintf(stderr, "Error: Unknown heuristic: `%s'\n", hname);
        exit(-1);
    }
    borTimerStop(&timer);
    build_time = borTimerElapsedInSF(&timer);

    planHeurResInit(&res);
    planHeurState(heur, &state, &res);

    planHeurDel(heur);
    borTimerStop(&timer);
    if (res.heur == PLAN_HEUR_DEAD_END){
        printf("DE");
    }else{
        printf("%d", (int)res.heur);
    }
    printf("\t%.8f\t%.8f\n", borTimerElapsedInSF(&timer), build_time);
    return 0;
}

#define H(hname) \
    if (heur(hname) != 0){ \
        fprintf(stderr, "Error: %s\n", (hname)); \
        return -1; \
    }

int main(int argc, char *argv[])
{
    if (argc != 3){
        fprintf(stderr, "Usage: %s heur-name problem.proto\n", argv[0]);
        return -1;
    }

    if (loadProblem(argv[2]) != 0)
        return -1;

    H(argv[1]);

    if (problem)
        planProblemDel(problem);
    return 0;
}
