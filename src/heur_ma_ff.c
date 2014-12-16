#include <boruvka/alloc.h>
#include <boruvka/rbtree_int.h>

#include "plan/heur.h"
#include "heur_relax.h"

struct _plan_heur_ma_ff_t {
    plan_heur_t heur;
    plan_heur_relax_t relax;

    plan_factarr_t state;      /*!< State for which the heuristic is computed */
    plan_oparr_t relaxed_plan; /*!< Relaxed plan computed on all operators.
                                    It means that the operators are
                                    identified by its global ID */

    bor_rbtree_int_t *peer_op;          /*!< Set of operators that are
                                             requested from other peers */
    bor_rbtree_int_node_t *pre_peer_op; /*!< Pre-allocated node to
                                             remote_op set */
    int peer_op_size;                   /*!< Number of peer ops in .peer_op */

    const plan_op_t *base_op;
    int base_op_size;
};
typedef struct _plan_heur_ma_ff_t plan_heur_ma_ff_t;

#define HEUR(parent) bor_container_of(parent, plan_heur_ma_ff_t, heur)


/** Adds operator to the MA relaxed plan if not already there.
 *  Returns 0 if the operator was inserted, -1 otherwise */
static int maAddOpToRelaxedPlan(plan_heur_ma_ff_t *ma, int id, int cost);
/** Adds peer-operator to the register. Returns 0 if the operator was
 *  inserted or -1 if operator was already there or already in relaxed plan. */
static int maAddPeerOp(plan_heur_ma_ff_t *ma, int id);
/** Removes peer-operator from the registry */
static void maDelPeerOp(plan_heur_ma_ff_t *ma, int id);
/** Sends HEUR_REQUEST message to the peer */
static void maSendHeurRequest(plan_ma_comm_t *comm, int peer_id,
                              const plan_factarr_t *state, int op_id);
/** Computes heuristic value from the relaxed plan */
static void maHeur(plan_heur_ma_ff_t *heur,
                   plan_heur_res_t *res);
/** Returns operator corresponding to its global ID or NULL if this node
 *  does not know this operator */
static const plan_op_t *maOpFromId(plan_heur_ma_ff_t *heur, int op_id);
/** Performs local exploration from the initial state stored in .ma.state
 *  to the specified goal. */
static void maExploreLocal(plan_heur_ma_ff_t *heur,
                           plan_ma_comm_t *comm,
                           const plan_part_state_t *goal,
                           plan_heur_res_t *res);
/** Update relaxed plan by received local operator */
static void maUpdateLocalOp(plan_heur_ma_ff_t *heur,
                            plan_ma_comm_t *comm,
                            int op_id);


static void heurDel(plan_heur_t *_heur);
static int planHeurRelaxFFMA(plan_heur_t *heur,
                             plan_ma_comm_t *comm,
                             const plan_state_t *state,
                             plan_heur_res_t *res);
static int planHeurRelaxFFMAUpdate(plan_heur_t *heur,
                                   plan_ma_comm_t *comm,
                                   const plan_ma_msg_t *msg,
                                   plan_heur_res_t *res);
static void planHeurRelaxFFMARequest(plan_heur_t *heur,
                                     plan_ma_comm_t *comm,
                                     const plan_ma_msg_t *msg);

plan_heur_t *planHeurMARelaxFFNew(const plan_problem_t *prob)
{
    plan_heur_ma_ff_t *heur;

    heur = BOR_ALLOC(plan_heur_ma_ff_t);
    _planHeurInit(&heur->heur, heurDel, NULL);
    _planHeurMAInit(&heur->heur, planHeurRelaxFFMA,
                    planHeurRelaxFFMAUpdate, planHeurRelaxFFMARequest);

    planHeurRelaxInit(&heur->relax, PLAN_HEUR_RELAX_TYPE_ADD,
                      prob->var, prob->var_size,
                      prob->goal,
                      prob->proj_op, prob->proj_op_size);

    heur->state.fact = BOR_ALLOC_ARR(int, prob->var_size);
    heur->state.size = prob->var_size;

    heur->relaxed_plan.op = NULL;
    heur->relaxed_plan.size = 0;

    heur->peer_op = borRBTreeIntNew();
    heur->pre_peer_op = BOR_ALLOC(bor_rbtree_int_node_t);
    heur->peer_op_size = 0;

    heur->base_op = prob->proj_op;
    heur->base_op_size = prob->proj_op_size;

    return &heur->heur;
}

static void heurDel(plan_heur_t *_heur)
{
    plan_heur_ma_ff_t *heur = HEUR(_heur);
    bor_rbtree_int_node_t *n;

    _planHeurFree(&heur->heur);
    planHeurRelaxFree(&heur->relax);

    BOR_FREE(heur->state.fact);
    if (heur->relaxed_plan.op)
        BOR_FREE(heur->relaxed_plan.op);

    while ((n = borRBTreeIntExtractMin(heur->peer_op)) != NULL){
        BOR_FREE(n);
    }
    borRBTreeIntDel(heur->peer_op);
    BOR_FREE(heur->pre_peer_op);
    BOR_FREE(heur);
}


static int maAddOpToRelaxedPlan(plan_heur_ma_ff_t *ma, int id, int cost)
{
    int i;

    if (ma->relaxed_plan.size <= id){
        i = ma->relaxed_plan.size;
        ma->relaxed_plan.size = id + 1;
        ma->relaxed_plan.op = BOR_REALLOC_ARR(ma->relaxed_plan.op, int,
                                              ma->relaxed_plan.size);
        // Initialize the newly allocated memory
        for (; i < ma->relaxed_plan.size; ++i){
            ma->relaxed_plan.op[i] = -1;
        }
    }

    if (ma->relaxed_plan.op[id] == -1){
        ma->relaxed_plan.op[id] = cost;
        return 0;
    }

    return -1;
}

static int maAddPeerOp(plan_heur_ma_ff_t *ma, int id)
{
    bor_rbtree_int_node_t *n;

    if (id < ma->relaxed_plan.size && ma->relaxed_plan.op[id] >= 0)
        return -1;

    n = borRBTreeIntInsert(ma->peer_op, id, ma->pre_peer_op);
    if (n == NULL){
        // The ID was inserted, preallocate next peer_op
        ma->pre_peer_op = BOR_ALLOC(bor_rbtree_int_node_t);
        // Increase the counter
        ++ma->peer_op_size;
        return 0;
    }

    return -1;
}

static void maDelPeerOp(plan_heur_ma_ff_t *ma, int id)
{
    bor_rbtree_int_node_t *n;

    n = borRBTreeIntFind(ma->peer_op, id);
    if (n != NULL){
        borRBTreeIntRemove(ma->peer_op, n);
        --ma->peer_op_size;
        BOR_FREE(n);
    }
}

static void maSendHeurRequest(plan_ma_comm_t *comm, int peer_id,
                              const plan_factarr_t *state, int op_id)
{
    plan_ma_msg_t *msg;

    msg = planMAMsgNew(PLAN_MA_MSG_HEUR, PLAN_MA_MSG_HEUR_FF_REQUEST,
                       comm->node_id);
    planMAMsgHeurFFSetRequest(msg, state->fact, state->size, op_id);
    planMACommSendToNode(comm, peer_id, msg);
    planMAMsgDel(msg);
}

static void maHeur(plan_heur_ma_ff_t *heur,
                   plan_heur_res_t *res)
{
    int i;
    plan_cost_t hval = 0;

    for (i = 0; i < heur->relaxed_plan.size; ++i){
        if (heur->relaxed_plan.op[i] > 0)
            hval += heur->relaxed_plan.op[i];
    }

    res->heur = hval;
    // TODO: preferred operators
}

static const plan_op_t *maOpFromId(plan_heur_ma_ff_t *heur, int op_id)
{
    int i;
    const plan_op_t *op = NULL;

    // TODO: Create new object plan_op_id_tr_t
    for (i = 0; i < heur->base_op_size; ++i){
        op = heur->base_op + i;
        if (op->global_id == op_id)
            return op;
    }

    return NULL;
}

static void maExploreLocal(plan_heur_ma_ff_t *heur,
                           plan_ma_comm_t *comm,
                           const plan_part_state_t *goal,
                           plan_heur_res_t *res)
{
    PLAN_STATE_STACK(state, heur->relax.cref.fact_id.var_size);
    const plan_op_t *op;
    int i, global_id, owner;

    // Initialize initial state
    for (i = 0; i < heur->relax.cref.fact_id.var_size; ++i){
        planStateSet(&state, i, heur->state.fact[i]);
    }

    // Compute heuristic from the initial state to the specified goal
    if (!goal){
        planHeurRelax(&heur->relax, &state);
        res->heur = heur->relax.fact[heur->relax.cref.goal_id].value;
    }else{
        res->heur = planHeurRelax2(&heur->relax, &state, goal);
    }
    if (res->heur == PLAN_HEUR_DEAD_END)
        return;

    // Mark relaxed plan
    if (!goal){
        planHeurRelaxMarkPlan(&heur->relax);
    }else{
        planHeurRelaxMarkPlan2(&heur->relax, goal);
    }

    for (i = 0; i < heur->relax.cref.op_size; ++i){
        if (!heur->relax.plan_op[i])
            continue;

        // Get the corresponding operator
        op = heur->base_op + i;
        global_id = op->global_id;
        owner = op->owner;

        if (owner != comm->node_id){
            // The operator is owned by remote peer.
            // Add it to the set of operators we are waiting for from
            // other peers
            if (maAddPeerOp(heur, global_id) == 0){
                // Send a request to the owner
                maSendHeurRequest(comm, owner, &heur->state, global_id);
            }
        }

        // Add the operator to the relaxed plan
        maAddOpToRelaxedPlan(heur, global_id, op->cost);
    }
}

static void maUpdateLocalOp(plan_heur_ma_ff_t *heur,
                            plan_ma_comm_t *comm,
                            int op_id)
{
    const plan_op_t *op;
    plan_heur_res_t res;

    op = maOpFromId(heur, op_id);
    if (op == NULL)
        return;

    if (maAddOpToRelaxedPlan(heur, op_id, op->cost) == 0){
        planHeurResInit(&res);
        maExploreLocal(heur, comm, op->pre, &res);
    }
}

static int planHeurRelaxFFMA(plan_heur_t *_heur,
                             plan_ma_comm_t *comm,
                             const plan_state_t *state,
                             plan_heur_res_t *res)
{
    plan_heur_ma_ff_t *heur = HEUR(_heur);
    int i;

    // Remember the state for which we want to compute heuristic
    for (i = 0; i < heur->state.size; ++i)
        heur->state.fact[i] = planStateGet(state, i);

    // Reset relaxed plan
    for (i = 0; i < heur->relaxed_plan.size; ++i)
        heur->relaxed_plan.op[i] = -1;

    // Explore projected state space
    maExploreLocal(heur, comm, NULL, res);
    if (res->heur == PLAN_HEUR_DEAD_END)
        return 0;

    // If we are waiting for responses from other peers, postpone actual
    // computation of the heuristic value.
    if (heur->peer_op_size > 0){
        return -1;
    }

    // Compute heuristic value
    maHeur(heur, res);
    return 0;
}

static int planHeurRelaxFFMAUpdate(plan_heur_t *_heur,
                                   plan_ma_comm_t *comm,
                                   const plan_ma_msg_t *msg,
                                   plan_heur_res_t *res)
{
    plan_heur_ma_ff_t *heur = HEUR(_heur);
    int i, len, op_id, cost, owner, from_agent;

    from_agent = planMAMsgAgent(msg);

    maDelPeerOp(heur, planMAMsgHeurFFOpId(msg));

    // Then explore all other peer-operators
    len = planMAMsgHeurFFOpSize(msg);
    for (i = 0; i < len; ++i){
        op_id = planMAMsgHeurFFOp(msg, i, &cost, &owner);

        if (owner == comm->node_id){
            maUpdateLocalOp(heur, comm, op_id);

        }else{
            if (owner != from_agent && maAddPeerOp(heur, op_id) == 0){
                maSendHeurRequest(comm, owner, &heur->state, op_id);
            }
            maAddOpToRelaxedPlan(heur, op_id, cost);
        }
    }

    if (heur->peer_op_size > 0)
        return -1;

    maHeur(heur, res);
    return 0;
}

static void maSendEmptyResponse(plan_ma_comm_t *comm,
                                int peer_id, int op_id)
{
    plan_ma_msg_t *resp;

    resp = planMAMsgNew(PLAN_MA_MSG_HEUR, PLAN_MA_MSG_HEUR_FF_RESPONSE,
                        comm->node_id);
    planMAMsgHeurFFSetResponse(resp, op_id);
    planMACommSendToNode(comm, peer_id, resp);
    planMAMsgDel(resp);
}

static void planHeurRelaxFFMARequest(plan_heur_t *_heur,
                                     plan_ma_comm_t *comm,
                                     const plan_ma_msg_t *msg)
{
    plan_heur_ma_ff_t *heur = HEUR(_heur);
    PLAN_STATE_STACK(state, heur->relax.cref.fact_id.var_size);
    plan_ma_msg_t *response;
    const plan_op_t *op;
    int i, op_id, agent_id;
    int global_id, owner;
    plan_cost_t h, cost;

    op_id = planMAMsgHeurFFOpId(msg);
    agent_id = planMAMsgAgent(msg);

    // Initialize initial state
    planMAMsgHeurFFState(msg, &state);

    // Find target operator
    op = maOpFromId(heur, op_id);
    if (op == NULL){
        maSendEmptyResponse(comm, agent_id, op_id);
        return;
    }

    // Compute heuristic from the initial state to the precondition of the
    // requested operator.
    h = planHeurRelax2(&heur->relax, &state, op->pre);
    if (h == PLAN_HEUR_DEAD_END){
        maSendEmptyResponse(comm, agent_id, op_id);
        return;
    }
    planHeurRelaxMarkPlan2(&heur->relax, op->pre);

    // Now .relaxed_plan is filled, so write to the response and send it
    // back.
    response = planMAMsgNew(PLAN_MA_MSG_HEUR, PLAN_MA_MSG_HEUR_FF_RESPONSE,
                            comm->node_id);
    planMAMsgHeurFFSetResponse(response, op_id);
    for (i = 0; i < heur->relax.cref.op_size; ++i){
        if (!heur->relax.plan_op[i])
            continue;

        op = heur->base_op + i;
        global_id = op->global_id;
        owner = op->owner;
        cost = op->cost;

        planMAMsgHeurFFAddOp(response, global_id, cost, owner);
    }
    planMACommSendToNode(comm, agent_id, response);

    planMAMsgDel(response);
}
