#include <cu/cu.h>
#include "plan/problem.h"
#include "plan/heur.h"
#include "state_pool.h"

#define RELAX_TEST_STATE_SIZE 9
struct _relax_test_t {
    plan_val_t state[RELAX_TEST_STATE_SIZE];
    plan_cost_t heur;
    int pref_size;
    const char **pref_op;
};
typedef struct _relax_test_t relax_test_t;

static const char *relax_op_unstack_b_c[] = { "unstack b c", NULL };
static const char *relax_op_putdown_b[] = { "put-down b", NULL };
static const char *relax_op_3[] = 
    { "pick-up a", "unstack c d", "unstack d b", NULL };
static relax_test_t relax_test_add[] = {
    { { 2, 0, 1, 1, 1, 0, 3, 1, 4 }, 10, 1, relax_op_unstack_b_c },
    { { 0, 1, 1, 0, 1, 1, 3, 1, 4 }, 13, 1, relax_op_putdown_b },
    { { 4, 0, 1, 0, 1, 0, 3, 1, 4 }, 7, -1, NULL },
    { { 4, 0, 0, 1, 1, 1, 3, 0, 4 }, 10, -1, NULL },
    { { 4, 1, 0, 0, 1, 0, 3, 2, 4 }, 8, -1, NULL },
    { { 3, 1, 1, 1, 0, 0, 4, 3, 2 }, PLAN_HEUR_DEAD_END, 0, NULL },
    { { 0, 0, 0, 0, 0, 0, 4, 3, 2 }, 6, 3, relax_op_3 }
};
static int relax_test_add_size = sizeof(relax_test_add) / sizeof(relax_test_t);

static relax_test_t relax_test_max[] = {
    { { 2, 0, 1, 1, 1, 0, 3, 1, 4 }, 5, 1, relax_op_unstack_b_c },
    { { 0, 1, 1, 0, 1, 1, 3, 1, 4 }, 5, 1, relax_op_putdown_b },
    { { 4, 0, 1, 0, 1, 0, 3, 1, 4 }, 4, -1, NULL },
    { { 4, 0, 0, 1, 1, 1, 3, 0, 4 }, 4, -1, NULL },
    { { 4, 1, 0, 0, 1, 0, 3, 2, 4 }, 3, -1, NULL },
    { { 3, 1, 1, 1, 0, 0, 4, 3, 2 }, PLAN_HEUR_DEAD_END, 0, NULL },
    { { 0, 0, 0, 0, 0, 0, 4, 3, 2 }, 2, 3, relax_op_3 }
};
static int relax_test_max_size = sizeof(relax_test_max) / sizeof(relax_test_t);

static relax_test_t relax_test_ff[] = {
    { { 2, 0, 1, 1, 1, 0, 3, 1, 4 }, 6, 1, relax_op_unstack_b_c },
    { { 0, 1, 1, 0, 1, 1, 3, 1, 4 }, 6, 1, relax_op_putdown_b },
    { { 4, 0, 1, 0, 1, 0, 3, 1, 4 }, 5, -1, NULL },
    { { 4, 0, 0, 1, 1, 1, 3, 0, 4 }, 6, -1, NULL },
    { { 4, 1, 0, 0, 1, 0, 3, 2, 4 }, 6, -1, NULL },
    { { 3, 1, 1, 1, 0, 0, 4, 3, 2 }, PLAN_HEUR_DEAD_END, 0, NULL },
    { { 0, 0, 0, 0, 0, 0, 4, 3, 2 }, 6, 3, relax_op_3 }
};
static int relax_test_ff_size = sizeof(relax_test_ff) / sizeof(relax_test_t);


typedef plan_heur_t *(*heur_new_fn)(const plan_var_t *var, int var_size,
                                    const plan_part_state_t *goal,
                                    const plan_op_t *op, int op_size,
                                    const plan_succ_gen_t *succ_gen);

static void testRelax(const char *fn, heur_new_fn new_fn,
                      relax_test_t *relax_test, int relax_test_size)
{
    plan_problem_t *p;
    plan_heur_t *heur;
    plan_state_t *state;
    plan_op_t **op;
    plan_heur_res_t res;
    int op_size, i, j;
    const char **op_name;

    p = planProblemFromFD(fn);
    heur = new_fn(p->var, p->var_size, p->goal,
                  p->op, p->op_size, p->succ_gen);
    state = planStateNew(p->state_pool);

    op = alloca(sizeof(plan_op_t *) * p->op_size);
    for (i = 0; i < relax_test_size; ++i){
        for (j = 0; j < RELAX_TEST_STATE_SIZE; ++j){
            planStateSet(state, j, relax_test[i].state[j]);
        }

        planHeurResInit(&res);
        if (relax_test[i].pref_size >= 0){
            op_size = planSuccGenFind(p->succ_gen, state, op, p->op_size);
            res.pref_op = op;
            res.pref_op_size = op_size;
        }
        planHeur(heur, state, &res);

        assertEquals(res.heur, relax_test[i].heur);
        if (res.heur != relax_test[i].heur){
            fprintf(stderr, "Err: %d\n", i);
        }

        if (relax_test[i].pref_size >= 0){
            assertEquals(res.pref_size, relax_test[i].pref_size);

            if (relax_test[i].pref_op != NULL){
                op_name = relax_test[i].pref_op;
                for (j = 0; *op_name != NULL; ++op_name, ++j){
                    assertEquals(strcmp(res.pref_op[j]->name, *op_name), 0);
                }
            }
        }else{
            assertEquals(res.pref_size, 0);
        }
    }

    planStateDel(state);
    planHeurDel(heur);
    planProblemDel(p);
}

#if 0
struct _heur2_test_t {
    plan_val_t state[38];
};
typedef struct _heur2_test_t heur2_test_t;

heur2_test_t heur2_test[] = {
    { { 1, 0, 1, 1, 1, 2, 0, 2, 0, 0, 2, 1, 1, 1, 2, 1, 1, 1, 1, 0
        , 1, 1, 1, 0, 0, 0, 0, 0, 16, 14, 5, 6, 8, 7, 15, 11, 12, 10 } },
    { { 1, 0, 1, 1, 1, 2, 0, 2, 0, 3, 2, 1, 1, 1, 2, 1, 1, 1, 0, 1
        , 1, 1, 1, 0, 0, 1, 0, 0, 16, 14, 5, 6, 2, 7, 15, 11, 12, 10 } },
    { { 1, 0, 1, 1, 1, 2, 0, 2, 0, 3, 2, 1, 1, 3, 2, 1, 1, 1, 0, 1
        , 1, 1, 0, 1, 0, 1, 1, 0, 16, 14, 5, 6, 2, 7, 15, 11, 3, 10 } },
    { { 1, 0, 1, 1, 1, 2, 0, 2, 0, 3, 2, 1, 1, 1, 3, 1, 1, 1, 0, 1
        , 0, 1, 1, 0, 1, 1, 0, 1, 16, 14, 5, 6, 2, 7, 15, 11, 12, 4 } },
    { { 1, 0, 1, 1, 1, 2, 0, 2, 0, 0, 2, 1, 1, 1, 3, 1, 1, 1, 1, 0
        , 0, 1, 1, 0, 1, 0, 0, 1, 16, 14, 5, 6, 8, 7, 15, 11, 12, 4 } },
    { { 1, 0, 1, 1, 1, 2, 0, 2, 0, 3, 2, 1, 1, 3, 3, 1, 1, 1, 0, 1
        , 0, 1, 0, 1, 1, 1, 1, 1, 16, 14, 5, 6, 2, 7, 15, 11, 3, 4 } },
    { { 1, 1, 1, 1, 1, 2, 0, 2, 0, 3, 2, 1, 1, 1, 3, 1, 1, 1, 0, 1
        , 0, 1, 1, 0, 1, 1, 0, 1, 16, 14, 5, 6, 2, 7, 15, 11, 12, 4 } },
    { { 1, 2, 1, 1, 1, 2, 0, 2, 0, 3, 2, 1, 1, 1, 3, 1, 1, 1, 0, 1
        , 0, 1, 1, 0, 1, 1, 0, 1, 16, 14, 5, 6, 2, 7, 15, 11, 12, 4 } },
    { { 1, 0, 1, 1, 1, 2, 0, 2, 0, 3, 2, 1, 1, 1, 3, 1, 1, 1, 0, 1
        , 0, 1, 1, 0, 1, 0, 0, 1, 16, 14, 5, 6, 0, 7, 15, 11, 12, 4 } },
    { { 0, 0, 1, 1, 1, 2, 0, 2, 0, 3, 2, 1, 1, 1, 3, 1, 1, 1, 0, 1
        , 0, 1, 1, 0, 1, 1, 0, 1, 16, 14, 5, 6, 2, 7, 15, 11, 12, 4 } },
    { { 2, 0, 1, 1, 1, 2, 0, 2, 0, 3, 2, 1, 1, 1, 3, 1, 1, 1, 0, 1
        , 0, 1, 1, 0, 1, 1, 0, 1, 16, 14, 5, 6, 2, 7, 15, 11, 12, 4 } },
    { { 1, 2, 1, 1, 1, 2, 0, 2, 0, 0, 2, 1, 1, 1, 3, 1, 1, 1, 1, 0
        , 0, 1, 1, 0, 1, 0, 0, 1, 16, 14, 5, 6, 8, 7, 15, 11, 12, 4 } },
    { { 1, 2, 1, 1, 1, 2, 0, 2, 0, 3, 2, 1, 1, 1, 2, 1, 1, 1, 0, 1
        , 1, 1, 1, 0, 0, 1, 0, 0, 16, 14, 5, 6, 2, 7, 15, 11, 12, 10 } },
    { { 1, 2, 1, 1, 1, 2, 0, 2, 0, 3, 2, 1, 1, 3, 3, 1, 1, 1, 0, 1
        , 0, 1, 0, 1, 1, 1, 1, 1, 16, 14, 5, 6, 2, 7, 15, 11, 3, 4 } },
    { { 1, 2, 1, 1, 1, 2, 0, 2, 0, 3, 2, 1, 1, 1, 3, 1, 1, 1, 0, 1
        , 0, 1, 1, 0, 1, 1, 0, 0, 16, 14, 5, 6, 2, 7, 15, 11, 12, 0 } },
    { { 1, 2, 1, 1, 1, 2, 0, 2, 0, 0, 2, 1, 1, 1, 3, 1, 1, 1, 1, 0
        , 0, 1, 1, 0, 1, 0, 0, 0, 16, 14, 5, 6, 8, 7, 15, 11, 12, 0 } },
    { { 1, 2, 1, 1, 1, 2, 0, 2, 0, 3, 3, 1, 1, 1, 3, 1, 1, 0, 0, 1
        , 1, 1, 1, 0, 1, 1, 0, 1, 16, 14, 5, 6, 2, 4, 15, 11, 12, 0 } },
    { { 1, 2, 1, 1, 1, 2, 0, 2, 0, 3, 2, 1, 1, 3, 3, 1, 1, 1, 0, 1
        , 0, 1, 0, 1, 1, 1, 1, 0, 16, 14, 5, 6, 2, 7, 15, 11, 3, 0 } },
    { { 1, 0, 1, 1, 1, 2, 0, 2, 0, 3, 2, 1, 1, 1, 3, 1, 1, 1, 0, 1
        , 0, 1, 1, 0, 1, 1, 0, 0, 16, 14, 5, 6, 2, 7, 15, 11, 12, 0 } },
    { { 1, 1, 1, 1, 1, 2, 0, 2, 0, 3, 2, 1, 1, 1, 3, 1, 1, 1, 0, 1
        , 0, 1, 1, 0, 1, 1, 0, 0, 16, 14, 5, 6, 2, 7, 15, 11, 12, 0 } },
    { { 0, 2, 1, 1, 1, 2, 0, 2, 0, 3, 2, 1, 1, 1, 3, 1, 1, 1, 0, 1
        , 0, 1, 1, 0, 1, 1, 0, 0, 16, 14, 5, 6, 2, 7, 15, 11, 12, 0 } },
    { { 2, 2, 1, 1, 1, 2, 0, 2, 0, 3, 2, 1, 1, 1, 3, 1, 1, 1, 0, 1
        , 0, 1, 1, 0, 1, 1, 0, 0, 16, 14, 5, 6, 2, 7, 15, 11, 12, 0 } },
    { { 1, 2, 1, 1, 1, 2, 0, 2, 0, 0, 3, 1, 1, 1, 3, 1, 1, 0, 1, 0
        , 1, 1, 1, 0, 1, 0, 0, 1, 16, 14, 5, 6, 8, 4, 15, 11, 12, 0 } },
    { { 1, 2, 1, 1, 1, 2, 0, 2, 0, 3, 3, 1, 1, 3, 3, 1, 1, 0, 0, 1
        , 1, 1, 0, 1, 1, 1, 1, 1, 16, 14, 5, 6, 2, 4, 15, 11, 3, 0 } },
    { { 1, 0, 1, 1, 1, 2, 0, 2, 0, 3, 3, 1, 1, 1, 3, 1, 1, 0, 0, 1
        , 1, 1, 1, 0, 1, 1, 0, 1, 16, 14, 5, 6, 2, 4, 15, 11, 12, 0 } },
    { { 1, 0, 1, 1, 1, 2, 0, 2, 0, 0, 3, 1, 1, 1, 3, 1, 1, 0, 1, 0
        , 1, 1, 1, 0, 1, 0, 0, 1, 16, 14, 5, 6, 8, 4, 15, 11, 12, 0 } },
    { { 1, 0, 1, 1, 1, 2, 0, 2, 0, 3, 3, 1, 1, 3, 3, 1, 1, 0, 0, 1
        , 1, 1, 0, 1, 1, 1, 1, 1, 16, 14, 5, 6, 2, 4, 15, 11, 3, 0 } },
    { { 1, 1, 1, 1, 1, 2, 0, 2, 0, 3, 3, 1, 1, 1, 3, 1, 1, 0, 0, 1
        , 1, 1, 1, 0, 1, 1, 0, 1, 16, 14, 5, 6, 2, 4, 15, 11, 12, 0 } },
    { { 1, 0, 1, 1, 1, 2, 0, 2, 0, 3, 3, 1, 1, 1, 3, 1, 1, 0, 0, 1
        , 1, 1, 1, 0, 1, 0, 0, 1, 16, 14, 5, 6, 0, 4, 15, 11, 12, 0 } },
    { { 0, 0, 1, 1, 1, 2, 0, 2, 0, 3, 3, 1, 1, 1, 3, 1, 1, 0, 0, 1
        , 1, 1, 1, 0, 1, 1, 0, 1, 16, 14, 5, 6, 2, 4, 15, 11, 12, 0 } },
    { { 2, 0, 1, 1, 1, 2, 0, 2, 0, 3, 3, 1, 1, 1, 3, 1, 1, 0, 0, 1
        , 1, 1, 1, 0, 1, 1, 0, 1, 16, 14, 5, 6, 2, 4, 15, 11, 12, 0 } },
    { { 1, 0, 1, 1, 1, 2, 0, 2, 0, 3, 2, 1, 1, 1, 3, 1, 1, 1, 0, 1
        , 0, 1, 1, 0, 1, 0, 0, 0, 16, 14, 5, 6, 0, 7, 15, 11, 12, 0 } },
    { { 1, 0, 1, 1, 1, 2, 0, 2, 3, 3, 3, 1, 1, 1, 3, 1, 0, 0, 1, 1
        , 1, 1, 1, 0, 1, 1, 0, 1, 16, 14, 5, 2, 0, 4, 15, 11, 12, 0 } },
    { { 1, 0, 1, 1, 1, 2, 0, 2, 0, 3, 3, 1, 1, 3, 3, 1, 1, 0, 0, 1
        , 1, 1, 0, 1, 1, 0, 1, 1, 16, 14, 5, 6, 0, 4, 15, 11, 3, 0 } },
    { { 1, 1, 1, 1, 1, 2, 0, 2, 0, 3, 3, 1, 1, 1, 3, 1, 1, 0, 0, 1
        , 1, 1, 1, 0, 1, 0, 0, 1, 16, 14, 5, 6, 0, 4, 15, 11, 12, 0 } },
    { { 1, 2, 1, 1, 1, 2, 0, 2, 0, 3, 3, 1, 1, 1, 3, 1, 1, 0, 0, 1
        , 1, 1, 1, 0, 1, 0, 0, 1, 16, 14, 5, 6, 0, 4, 15, 11, 12, 0 } },
    { { 1, 0, 1, 1, 1, 2, 0, 2, 0, 3, 3, 1, 1, 1, 3, 1, 1, 0, 0, 1
        , 1, 1, 1, 0, 1, 1, 0, 1, 16, 14, 5, 6, 0, 4, 15, 11, 12, 2 } },
    { { 0, 0, 1, 1, 1, 2, 0, 2, 0, 3, 3, 1, 1, 1, 3, 1, 1, 0, 0, 1
        , 1, 1, 1, 0, 1, 0, 0, 1, 16, 14, 5, 6, 0, 4, 15, 11, 12, 0 } },
    { { 2, 0, 1, 1, 1, 2, 0, 2, 0, 3, 3, 1, 1, 1, 3, 1, 1, 0, 0, 1
        , 1, 1, 1, 0, 1, 0, 0, 1, 16, 14, 5, 6, 0, 4, 15, 11, 12, 0 } },
    { { 1, 0, 1, 1, 1, 2, 0, 2, 3, 3, 2, 1, 1, 1, 3, 1, 0, 1, 1, 1
        , 0, 1, 1, 0, 1, 1, 0, 0, 16, 14, 5, 2, 0, 7, 15, 11, 12, 0 } },
    { { 1, 0, 1, 1, 1, 2, 0, 2, 3, 3, 3, 1, 1, 3, 3, 1, 0, 0, 1, 1
        , 1, 1, 0, 1, 1, 1, 1, 1, 16, 14, 5, 2, 0, 4, 15, 11, 3, 0 } },
    { { 1, 1, 1, 1, 1, 2, 0, 2, 3, 3, 3, 1, 1, 1, 3, 1, 0, 0, 1, 1
        , 1, 1, 1, 0, 1, 1, 0, 1, 16, 14, 5, 2, 0, 4, 15, 11, 12, 0 } },
    { { 1, 2, 1, 1, 1, 2, 0, 2, 3, 3, 3, 1, 1, 1, 3, 1, 0, 0, 1, 1
        , 1, 1, 1, 0, 1, 1, 0, 1, 16, 14, 5, 2, 0, 4, 15, 11, 12, 0 } },
    { { 1, 2, 1, 1, 1, 2, 0, 2, 3, 3, 2, 1, 1, 1, 3, 1, 0, 1, 1, 1
        , 0, 1, 1, 0, 1, 1, 0, 0, 16, 14, 5, 2, 0, 7, 15, 11, 12, 0 } },
    { { 1, 2, 1, 1, 1, 2, 0, 2, 3, 3, 3, 1, 1, 3, 3, 1, 0, 0, 1, 1
        , 1, 1, 0, 1, 1, 1, 1, 1, 16, 14, 5, 2, 0, 4, 15, 11, 3, 0 } },
    { { 1, 2, 1, 1, 1, 2, 0, 2, 3, 3, 3, 1, 1, 1, 3, 1, 0, 0, 1, 1
        , 1, 1, 1, 0, 1, 1, 0, 0, 16, 14, 5, 2, 0, 0, 15, 11, 12, 0 } },
    { { 0, 2, 1, 1, 1, 2, 0, 2, 3, 3, 3, 1, 1, 1, 3, 1, 0, 0, 1, 1
        , 1, 1, 1, 0, 1, 1, 0, 1, 16, 14, 5, 2, 0, 4, 15, 11, 12, 0 } },
    { { 2, 2, 1, 1, 1, 2, 0, 2, 3, 3, 3, 1, 1, 1, 3, 1, 0, 0, 1, 1
        , 1, 1, 1, 0, 1, 1, 0, 1, 16, 14, 5, 2, 0, 4, 15, 11, 12, 0 } },
    { { 1, 2, 1, 1, 1, 2, 0, 2, 0, 3, 3, 1, 1, 1, 3, 1, 1, 0, 0, 1
        , 1, 1, 1, 0, 1, 0, 0, 0, 16, 14, 5, 6, 0, 0, 15, 11, 12, 0 } },
    { { 1, 2, 1, 1, 1, 2, 0, 3, 3, 3, 3, 1, 1, 1, 3, 0, 0, 1, 1, 1
        , 1, 1, 1, 0, 1, 1, 0, 1, 16, 14, 4, 2, 0, 0, 15, 11, 12, 0 } },
    { { 1, 2, 1, 1, 1, 2, 0, 2, 3, 3, 3, 1, 1, 3, 3, 1, 0, 0, 1, 1
        , 1, 1, 0, 1, 1, 1, 1, 0, 16, 14, 5, 2, 0, 0, 15, 11, 3, 0 } },
    { { 1, 0, 1, 1, 1, 2, 0, 2, 3, 3, 3, 1, 1, 1, 3, 1, 0, 0, 1, 1
        , 1, 1, 1, 0, 1, 1, 0, 0, 16, 14, 5, 2, 0, 0, 15, 11, 12, 0 } },
    { { 1, 1, 1, 1, 1, 2, 0, 2, 3, 3, 3, 1, 1, 1, 3, 1, 0, 0, 1, 1
        , 1, 1, 1, 0, 1, 1, 0, 0, 16, 14, 5, 2, 0, 0, 15, 11, 12, 0 } },
    { { 1, 2, 1, 1, 1, 2, 0, 2, 3, 3, 3, 1, 1, 1, 3, 1, 0, 0, 1, 1
        , 1, 1, 1, 0, 1, 1, 0, 1, 16, 14, 5, 2, 4, 0, 15, 11, 12, 0 } },
    { { 1, 2, 1, 1, 1, 2, 0, 2, 0, 3, 3, 1, 1, 1, 3, 1, 1, 0, 0, 1
        , 1, 1, 1, 0, 1, 0, 0, 1, 16, 14, 5, 6, 4, 0, 15, 11, 12, 0 } },
    { { 1, 2, 1, 1, 1, 2, 0, 2, 3, 2, 3, 1, 1, 1, 3, 1, 0, 1, 1, 0
        , 1, 1, 1, 0, 1, 1, 0, 0, 16, 14, 5, 2, 7, 0, 15, 11, 12, 0 } },
    { { 1, 2, 1, 1, 1, 2, 0, 2, 3, 3, 3, 1, 1, 3, 3, 1, 0, 0, 1, 1
        , 1, 1, 0, 1, 1, 1, 1, 1, 16, 14, 5, 2, 4, 0, 15, 11, 3, 0 } },
    { { 1, 0, 1, 1, 1, 2, 0, 2, 3, 3, 3, 1, 1, 1, 3, 1, 0, 0, 1, 1
        , 1, 1, 1, 0, 1, 1, 0, 1, 16, 14, 5, 2, 4, 0, 15, 11, 12, 0 } },
    { { 1, 1, 1, 1, 1, 2, 0, 2, 3, 3, 3, 1, 1, 1, 3, 1, 0, 0, 1, 1
        , 1, 1, 1, 0, 1, 1, 0, 1, 16, 14, 5, 2, 4, 0, 15, 11, 12, 0 } },
    { { 0, 2, 1, 1, 1, 2, 0, 2, 3, 3, 3, 1, 1, 1, 3, 1, 0, 0, 1, 1
        , 1, 1, 1, 0, 1, 1, 0, 1, 16, 14, 5, 2, 4, 0, 15, 11, 12, 0 } },
    { { 2, 2, 1, 1, 1, 2, 0, 2, 3, 3, 3, 1, 1, 1, 3, 1, 0, 0, 1, 1
        , 1, 1, 1, 0, 1, 1, 0, 1, 16, 14, 5, 2, 4, 0, 15, 11, 12, 0 } },
    { { 2, 2, 1, 1, 1, 2, 0, 2, 0, 3, 3, 1, 1, 1, 3, 1, 1, 0, 0, 1
        , 1, 1, 1, 0, 1, 0, 0, 1, 16, 14, 5, 6, 4, 0, 15, 11, 12, 0 } },
    { { 2, 2, 1, 1, 1, 2, 0, 2, 3, 2, 3, 1, 1, 1, 3, 1, 0, 1, 1, 0
        , 1, 1, 1, 0, 1, 1, 0, 0, 16, 14, 5, 2, 7, 0, 15, 11, 12, 0 } },
    { { 2, 2, 1, 1, 1, 2, 0, 2, 3, 3, 3, 1, 1, 3, 3, 1, 0, 0, 1, 1
        , 1, 1, 0, 1, 1, 1, 1, 1, 16, 14, 5, 2, 4, 0, 15, 11, 3, 0 } },
    { { 2, 0, 1, 1, 1, 2, 0, 2, 3, 3, 3, 1, 1, 1, 3, 1, 0, 0, 1, 1
        , 1, 1, 1, 0, 1, 1, 0, 1, 16, 14, 5, 2, 4, 0, 15, 11, 12, 0 } },
    { { 2, 1, 1, 1, 1, 2, 0, 2, 3, 3, 3, 1, 1, 1, 3, 1, 0, 0, 1, 1
        , 1, 1, 1, 0, 1, 1, 0, 1, 16, 14, 5, 2, 4, 0, 15, 11, 12, 0 } }
};
int heur2_test_len = sizeof(heur2_test) / sizeof(heur2_test_t);

static void testHeur2(heur_new_fn new_fn)
{
    plan_problem_t *p;
    plan_heur_t *heur;
    plan_state_t *state;
    plan_heur_res_t res;
    int i, j, op_size;
    plan_cost_t *hval;
    plan_cost_t *hval2;

    p = planProblemFromFD("../data/ma-benchmarks/depot/pfile5.sas");
    state = planStateNew(p->state_pool);

    op_size = BOR_MIN(p->op_size, 200);
    hval  = alloca(sizeof(plan_cost_t) * op_size);
    hval2 = alloca(sizeof(plan_cost_t) * op_size);

    for (i = 0; i < heur2_test_len; ++i){
        for (j = 0; j < 38; ++j)
            planStateSet(state, j, heur2_test[i].state[j]);

        heur = new_fn(p->var, p->var_size, p->goal,
                      p->op, p->op_size, p->succ_gen);
        for (j = 0; j < op_size; ++j){
            planHeurResInit(&res);
            planHeur2(heur, state, p->op[j].pre, &res);
            hval2[j] = res.heur;
        }
        planHeurDel(heur);

        for (j = 0; j < op_size; ++j){
            heur = new_fn(p->var, p->var_size, p->op[j].pre,
                          p->op, p->op_size, p->succ_gen);
            planHeurResInit(&res);
            planHeur(heur, state, &res);
            hval[j] = res.heur;
            planHeurDel(heur);
        }

        for (j = 0; j < op_size; ++j){
            assertEquals(hval[j], hval2[j]);
        }

        break;
    }

    planStateDel(state);
    planProblemDel(p);
}
#endif

static plan_heur_t *goalCountNew(const plan_var_t *var, int var_size,
                                 const plan_part_state_t *goal,
                                 const plan_op_t *op, int op_size,
                                 const plan_succ_gen_t *succ_gen)
{
    return planHeurGoalCountNew(goal);
}


TEST(testHeurGoalCount)
{
    //testHeur2(goalCountNew);
}

TEST(testHeurRelaxAdd)
{
    testRelax("load-from-file.in1.sas", planHeurRelaxAddNew,
              relax_test_add, relax_test_add_size);
    //testHeur2(planHeurRelaxAddNew);
}

TEST(testHeurRelaxMax)
{
    testRelax("load-from-file.in1.sas", planHeurRelaxMaxNew,
              relax_test_max, relax_test_max_size);
    //testHeur2(planHeurRelaxMaxNew);
}

TEST(testHeurRelaxFF)
{
    testRelax("load-from-file.in1.sas", planHeurRelaxFFNew,
              relax_test_ff, relax_test_ff_size);
    //testHeur2(planHeurRelaxFFNew);
}

static void runTestHeurRelaxLMCut(const char *proto, const char *states)
{
    plan_problem_t *p;
    state_pool_t state_pool;
    plan_state_t *state;
    plan_heur_t *heur;
    plan_heur_res_t res;
    int i, si;

    printf("-----\nLM-CUT\n%s\n", proto);
    p = planProblemFromProto(proto, PLAN_PROBLEM_USE_CG);
    state = planStateNew(p->state_pool);
    statePoolInit(&state_pool, states);

    heur = planHeurLMCutNew(p->var, p->var_size, p->goal,
                            p->op, p->op_size, p->succ_gen);

    for (si = 0; statePoolNext(&state_pool, state) == 0; ++si){
        planHeurResInit(&res);
        planHeur(heur, state, &res);
        printf("[%d] %d ::", si, res.heur);
        for (i = 0; i < planStateSize(state); ++i){
            printf(" %d", planStateGet(state, i));
        }
        printf("\n");
    }

    planHeurDel(heur);
    statePoolFree(&state_pool);
    planStateDel(state);
    planProblemDel(p);
    printf("-----\n");
}

TEST(testHeurRelaxLMCut)
{
    runTestHeurRelaxLMCut("../data/ma-benchmarks/depot/pfile1.proto",
                          "states/depot-pfile1.txt");
    runTestHeurRelaxLMCut("../data/ma-benchmarks/depot/pfile5.proto",
                          "states/depot-pfile5.txt");
    runTestHeurRelaxLMCut("../data/ma-benchmarks/rovers/p03.proto",
                          "states/rovers-p03.txt");
    runTestHeurRelaxLMCut("../data/ma-benchmarks/rovers/p15.proto",
                          "states/rovers-p15.txt");
}
