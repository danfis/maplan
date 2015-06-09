#include <time.h>
#include <stdlib.h>
#include <cu/cu.h>
#include <boruvka/alloc.h>
#include "plan/problem.h"
#include "plan/heur.h"
#include "plan/search.h"
#include "state_pool.h"
#include "../src/heur_relax.h"
#include "heur_common.h"



void runHeurTest(const char *name,
                 const char *proto, const char *states,
                 new_heur_fn new_heur,
                 int pref, int landmarks)
{
    plan_problem_t *p;
    state_pool_t state_pool;
    plan_state_t *state;
    plan_heur_t *heur;
    plan_heur_res_t res;
    int i, j, si;
    plan_op_t **pref_ops;

    printf("-----\n%s\n%s\n", name, proto);
    p = planProblemFromProto(proto, PLAN_PROBLEM_USE_CG);
    state = planStateNew(p->state_pool->num_vars);
    statePoolInit(&state_pool, states);

    pref_ops = BOR_ALLOC_ARR(plan_op_t *, p->op_size);
    for (i = 0; i < p->op_size; ++i)
        pref_ops[i] = p->op + i;

    heur = new_heur(p);
    if (heur == NULL){
        fprintf(stderr, "Test Error: Cannot create a heuristic object!\n");
        goto run_test_end;
    }

    for (si = 0; statePoolNext(&state_pool, state) == 0; ++si){
        //if (si != 4)
        //    continue;
        planHeurResInit(&res);
        if (pref){
            res.pref_op = pref_ops;
            res.pref_op_size = p->op_size;
        }
        if (landmarks)
            res.save_landmarks = 1;

        planHeurState(heur, state, &res);
        printf("[%d] ", si);
        if (res.heur == PLAN_HEUR_DEAD_END){
            printf("DEAD END");
        }else{
            printf("%d", res.heur);
        }
        printf(" ::");
        for (i = 0; i < planStateSize(state); ++i){
            printf(" %d", planStateGet(state, i));
        }
        printf("\n");

        if (pref){
            printf("Pref ops[%d]:\n", res.pref_size);
            for (i = 0; i < res.pref_size; ++i){
                printf("%s\n", res.pref_op[i]->name);
            }
        }

        if (landmarks){
            for (i = 0; i < res.landmarks.size; ++i){
                printf("Landmark [%d]:", i);
                for (j = 0; j < res.landmarks.landmark[i].size; ++j){
                    printf(" %d", res.landmarks.landmark[i].op_id[j]);
                }
                printf("\n");
            }
            planLandmarkSetFree(&res.landmarks);
        }
        fflush(stdout);
    }

run_test_end:
    BOR_FREE(pref_ops);

    if (heur)
        planHeurDel(heur);
    statePoolFree(&state_pool);
    planStateDel(state);
    planProblemDel(p);
    printf("-----\n");
}

static int stopSearch(const plan_search_stat_t *state, void *_)
{
    return PLAN_SEARCH_ABORT;
}

void runHeurAStarTest(const char *name, const char *proto,
                      new_heur_fn new_heur, int max_steps)
{
    plan_search_astar_params_t params;
    plan_search_t *search;
    plan_problem_t *prob;
    plan_path_t path;
    const plan_state_t *state;
    const plan_state_space_node_t *node;
    int si, i;

    printf("----- A* test -----\n%s\n%s\n", name, proto);
    prob = planProblemFromProto(proto, PLAN_PROBLEM_USE_CG);

    planSearchAStarParamsInit(&params);
    params.search.heur = new_heur(prob);
    params.search.heur_del = 1;
    params.search.prob = prob;
    params.search.progress.fn = stopSearch;
    params.search.progress.freq = max_steps;
    search = planSearchAStarNew(&params);

    planPathInit(&path);
    planSearchRun(search, &path);
    planPathFree(&path);

    for (si = 0; si < prob->state_pool->num_states; ++si){
        state = planSearchLoadState(search, si);
        node = planSearchLoadNode(search, si);

        printf("[%d] ", si);
        if (node->heuristic == PLAN_HEUR_DEAD_END){
            printf("DEAD END");
        }else{
            printf("%d", (int)node->heuristic);
        }
        printf(" ::");
        for (i = 0; i < planStateSize(state); ++i){
            printf(" %d", planStateGet(state, i));
        }
        printf("\n");
    }

    planSearchDel(search);
    planProblemDel(prob);
    printf("-----\n");
}
