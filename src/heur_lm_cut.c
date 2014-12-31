#include <boruvka/alloc.h>
#include <boruvka/lifo.h>
#include "plan/heur.h"

#include "heur_relax.h"

struct _plan_heur_lm_cut_t {
    plan_heur_t heur;
    plan_heur_relax_t relax;

    int *fact_goal_zone; /*!< Flag for each fact whether it is in goal zone */
    int *fact_in_queue;
    plan_oparr_t cut;    /*!< Array of cut operators */
};
typedef struct _plan_heur_lm_cut_t plan_heur_lm_cut_t;

#define HEUR(parent) \
    bor_container_of((parent), plan_heur_lm_cut_t, heur)

/** Delete method */
static void planHeurLMCutDel(plan_heur_t *_heur);
/** Main function that returns heuristic value. */
static void planHeurLMCut(plan_heur_t *_heur, const plan_state_t *state,
                          plan_heur_res_t *res);

plan_heur_t *planHeurLMCutNew(const plan_var_t *var, int var_size,
                              const plan_part_state_t *goal,
                              const plan_op_t *op, int op_size,
                              const plan_succ_gen_t *succ_gen)
{
    plan_heur_lm_cut_t *heur;

    heur = BOR_ALLOC(plan_heur_lm_cut_t);
    _planHeurInit(&heur->heur, planHeurLMCutDel,
                  planHeurLMCut);
    planHeurRelaxInit(&heur->relax, PLAN_HEUR_RELAX_TYPE_MAX,
                      var, var_size, goal, op, op_size);

    heur->fact_goal_zone = BOR_ALLOC_ARR(int, heur->relax.cref.fact_size);
    heur->fact_in_queue  = BOR_ALLOC_ARR(int, heur->relax.cref.fact_size);
    heur->cut.op = BOR_ALLOC_ARR(int, heur->relax.cref.op_size);
    heur->cut.size = 0;

    return &heur->heur;
}

static void planHeurLMCutDel(plan_heur_t *_heur)
{
    plan_heur_lm_cut_t *heur = HEUR(_heur);

    BOR_FREE(heur->cut.op);
    BOR_FREE(heur->fact_goal_zone);
    BOR_FREE(heur->fact_in_queue);
    planHeurRelaxFree(&heur->relax);
    _planHeurFree(&heur->heur);
    BOR_FREE(heur);
}



static void markGoalZoneRecursive(plan_heur_lm_cut_t *heur, int fact_id)
{
    int i, len, *op_ids, op_id;
    plan_heur_relax_op_t *op;

    if (heur->fact_goal_zone[fact_id])
        return;

    heur->fact_goal_zone[fact_id] = 1;

    len    = heur->relax.cref.fact_eff[fact_id].size;
    op_ids = heur->relax.cref.fact_eff[fact_id].op;
    for (i = 0; i < len; ++i){
        op_id = op_ids[i];
        op = heur->relax.op + op_id;
        if (op->cost == 0 && op->supp != -1)
            markGoalZoneRecursive(heur, op->supp);
    }
}

static void markGoalZone(plan_heur_lm_cut_t *heur)
{
    // Zeroize goal-zone flags and recursively mark facts in the goal-zone
    bzero(heur->fact_goal_zone, sizeof(int) * heur->relax.cref.fact_size);
    markGoalZoneRecursive(heur, heur->relax.cref.goal_id);
}



static void findCutAddInit(plan_heur_lm_cut_t *heur,
                           const plan_state_t *state,
                           bor_lifo_t *queue)
{
    plan_var_id_t var, len;
    plan_val_t val;
    int id;

    len = planStateSize(state);
    for (var = 0; var < len; ++var){
        val = planStateGet(state, var);
        id = planFactId(&heur->relax.cref.fact_id, var, val);
        heur->fact_in_queue[id] = 1;
        borLifoPush(queue, &id);
    }
    id = heur->relax.cref.fake_pre[0].fact_id;
    heur->fact_in_queue[id] = 1;
    borLifoPush(queue, &id);
}

static void findCutEnqueueEffects(plan_heur_lm_cut_t *heur,
                                  int op_id, bor_lifo_t *queue)
{
    int i, len, *facts, fact_id;
    int in_cut = 0;

    len   = heur->relax.cref.op_eff[op_id].size;
    facts = heur->relax.cref.op_eff[op_id].fact;
    for (i = 0; i < len; ++i){
        fact_id = facts[i];

        // Determine whether the operator belongs to cut
        if (!in_cut && heur->fact_goal_zone[fact_id]){
            heur->cut.op[heur->cut.size++] = op_id;
            in_cut = 1;
        }

        if (!heur->fact_in_queue[fact_id]
                && !heur->fact_goal_zone[fact_id]){
            heur->fact_in_queue[fact_id] = 1;
            borLifoPush(queue, &fact_id);
        }
    }
}

static void findCut(plan_heur_lm_cut_t *heur, const plan_state_t *state)
{
    int i, len, *ops, fact_id, op_id;
    bor_lifo_t queue;

    // Zeroize in-queue flags
    bzero(heur->fact_in_queue, sizeof(int) * heur->relax.cref.fact_size);

    // Reset output structure
    heur->cut.size = 0;

    // Initialize queue and adds initial state
    borLifoInit(&queue, sizeof(int));
    findCutAddInit(heur, state, &queue);

    while (!borLifoEmpty(&queue)){
        // Pop next fact from queue
        fact_id = *(int *)borLifoBack(&queue);
        borLifoPop(&queue);

        len = heur->relax.cref.fact_pre[fact_id].size;
        ops = heur->relax.cref.fact_pre[fact_id].op;
        for (i = 0; i < len; ++i){
            op_id = ops[i];

            if (heur->relax.op[op_id].supp == fact_id){
                findCutEnqueueEffects(heur, op_id, &queue);
            }
        }
    }

    borLifoFree(&queue);
}

static plan_cost_t updateCutCost(const plan_oparr_t *cut, plan_heur_relax_op_t *op)
{
    int cut_cost = INT_MAX;
    int i, len, op_id;
    const int *ops;

    len = cut->size;
    ops = cut->op;

    // Find minimal cost from the cut operators
    cut_cost = op[ops[0]].cost;
    for (i = 1; i < len; ++i){
        op_id = ops[i];
        cut_cost = BOR_MIN(cut_cost, op[op_id].cost);
    }

    // Substract the minimal cost from all cut operators
    for (i = 0; i < len; ++i){
        op_id = ops[i];
        op[op_id].cost  -= cut_cost;
        op[op_id].value -= cut_cost;
    }

    return cut_cost;
}

static void planHeurLMCut(plan_heur_t *_heur, const plan_state_t *state,
                          plan_heur_res_t *res)
{
    plan_heur_lm_cut_t *heur = HEUR(_heur);
    plan_cost_t h = PLAN_HEUR_DEAD_END;

    // Compute initial h^max
    planHeurRelax(&heur->relax, state);

    // Check whether the goal was reached, if so prepare output variable
    if (heur->relax.fact[heur->relax.cref.goal_id].value >= 0)
        h = 0;

    while (heur->relax.fact[heur->relax.cref.goal_id].value > 0){
        // Mark facts connected with a goal by zero-cost operators in
        // justification graph.
        markGoalZone(heur);

        // Find operators that are reachable from the initial state and are
        // connected with the goal-zone facts in justification graph.
        findCut(heur, state);

        // If no cut was found we have reached dead-end
        if (heur->cut.size == 0){
            // TODO: This is error, isn't it?
            h = PLAN_HEUR_DEAD_END;
            break;
        }

        // Determine the minimal cost from all cut-operators. Substract
        // this cost from their cost and add it to the final heuristic
        // value.
        h += updateCutCost(&heur->cut, heur->relax.op);

        // Performat incremental h^max computation using changed operator
        // costs
        planHeurRelaxIncMax(&heur->relax, heur->cut.op, heur->cut.size);
    }

    res->heur = h;
}
