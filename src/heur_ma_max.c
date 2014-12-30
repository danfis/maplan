#include <boruvka/alloc.h>
#include "plan/heur.h"
#include "plan/prioqueue.h"
#include "heur_relax.h"
#include "op_id_tr.h"

struct _private_t {
    plan_heur_relax_t relax;
    plan_factarr_t fact;
    plan_op_id_tr_t op_id_tr;
};
typedef struct _private_t private_t;

struct _plan_heur_ma_max_t {
    plan_heur_t heur;
    plan_heur_relax_t relax;
    plan_op_id_tr_t op_id_tr;

    int agent_id;
    plan_oparr_t *agent_op;    /*!< Operators owned by a corresponding agent */
    int *agent_changed;        /*!< True/False for each agent signaling
                                    whether an operator changed */
    int agent_size;            /*!< Number of agents in cluster */
    int cur_agent_id;          /*!< ID of the agent from which a response
                                    is expected. */
    plan_factarr_t init_state; /*!< Stored actual state for which the
                                    heuristic is computed */
    int *is_init_fact;         /*!< True/False flag for each fact whether
                                    it belongs to the .init_state */
    int *op_owner;

    private_t private;
};
typedef struct _plan_heur_ma_max_t plan_heur_ma_max_t;

#define HEUR(parent) \
    bor_container_of(parent, plan_heur_ma_max_t, heur)

static void heurDel(plan_heur_t *_heur);
static int heurMAMax(plan_heur_t *heur, plan_ma_comm_t *comm,
                     const plan_state_t *state, plan_heur_res_t *res);
static int heurMAMaxUpdate(plan_heur_t *heur, plan_ma_comm_t *comm,
                           const plan_ma_msg_t *msg, plan_heur_res_t *res);
static void heurMAMaxRequest(plan_heur_t *heur, plan_ma_comm_t *comm,
                             const plan_ma_msg_t *msg);

static void initAgent(plan_heur_ma_max_t *heur, const plan_problem_t *prob)
{
    int i, j;
    const plan_op_t *op;
    plan_oparr_t *oparr;

    heur->agent_op = NULL;
    heur->agent_changed = NULL;
    heur->agent_size = 0;

    for (i = 0; i < prob->proj_op_size; ++i){
        op = prob->proj_op + i;

        if (op->owner >= heur->agent_size){
            heur->agent_op = BOR_REALLOC_ARR(heur->agent_op, plan_oparr_t,
                                             op->owner + 1);
            for (j = heur->agent_size; j < op->owner + 1; ++j){
                oparr = heur->agent_op + j;
                oparr->size = 0;
                oparr->op = BOR_ALLOC_ARR(int, prob->proj_op_size);
            }
            heur->agent_size = op->owner + 1;
        }

        oparr = heur->agent_op + op->owner;
        oparr->op[oparr->size++] = i;
    }

    for (i = 0; i < heur->agent_size; ++i){
        oparr = heur->agent_op + i;
        oparr->op = BOR_REALLOC_ARR(oparr->op, int, oparr->size);
    }

    heur->agent_changed = BOR_ALLOC_ARR(int, heur->agent_size);
}

static void privateInit(private_t *private, const plan_problem_t *prob)
{
    plan_op_t *op;
    int op_size;
    int i;

    planProblemCreatePrivateProjOps(prob->op, prob->op_size,
                                    prob->var, prob->var_size,
                                    &op, &op_size);
    planHeurRelaxInit(&private->relax, PLAN_HEUR_RELAX_TYPE_MAX,
                      prob->var, prob->var_size, prob->goal, op, op_size);

    private->fact.fact = BOR_ALLOC_ARR(int, private->relax.cref.fact_size);
    private->fact.size = 0;
    for (i = 0; i < private->relax.cref.fact_size; ++i){
        if (private->relax.cref.fact_eff[i].size > 0
                || private->relax.cref.fact_pre[i].size > 0){
            private->fact.fact[private->fact.size++] = i;
        }
    }

    private->fact.fact = BOR_REALLOC_ARR(private->fact.fact, int,
                                         private->fact.size);

    planOpIdTrInit(&private->op_id_tr, op, op_size);
    planProblemDestroyOps(op, op_size);
}

static void privateFree(private_t *private)
{
    planHeurRelaxFree(&private->relax);
    if (private->fact.fact)
        BOR_FREE(private->fact.fact);
    planOpIdTrFree(&private->op_id_tr);
}

plan_heur_t *planHeurMARelaxMaxNew(const plan_problem_t *prob)
{
    plan_heur_ma_max_t *heur;
    int i;

    heur = BOR_ALLOC(plan_heur_ma_max_t);
    _planHeurInit(&heur->heur, heurDel, NULL);
    _planHeurMAInit(&heur->heur, heurMAMax, heurMAMaxUpdate, heurMAMaxRequest);
    planHeurRelaxInit(&heur->relax, PLAN_HEUR_RELAX_TYPE_MAX,
                      prob->var, prob->var_size, prob->goal,
                      prob->proj_op, prob->proj_op_size);
    planOpIdTrInit(&heur->op_id_tr, prob->proj_op, prob->proj_op_size);

    initAgent(heur, prob);
    heur->init_state.size = prob->var_size;
    heur->init_state.fact = BOR_ALLOC_ARR(int, heur->init_state.size);
    heur->is_init_fact = BOR_ALLOC_ARR(int, heur->relax.cref.fact_size);

    heur->op_owner = BOR_ALLOC_ARR(int, prob->proj_op_size);
    for (i = 0; i < prob->proj_op_size; ++i)
        heur->op_owner[i] = prob->proj_op[i].owner;

    privateInit(&heur->private, prob);

    return &heur->heur;
}

static void heurDel(plan_heur_t *_heur)
{
    int i;
    plan_heur_ma_max_t *heur = HEUR(_heur);

    BOR_FREE(heur->agent_changed);
    for (i = 0; i < heur->agent_size; ++i){
        if (heur->agent_op[i].op)
            BOR_FREE(heur->agent_op[i].op);
    }
    BOR_FREE(heur->agent_op);

    BOR_FREE(heur->init_state.fact);
    BOR_FREE(heur->is_init_fact);
    BOR_FREE(heur->op_owner);

    privateFree(&heur->private);

    planOpIdTrFree(&heur->op_id_tr);
    planHeurRelaxFree(&heur->relax);
    _planHeurFree(&heur->heur);
    BOR_FREE(heur);
}


static void setInitState(plan_heur_ma_max_t *heur, const plan_state_t *state)
{
    int i, fact_id;

    bzero(heur->is_init_fact, sizeof(int) * heur->relax.cref.fact_size);
    for (i = 0; i < heur->init_state.size; ++i){
        heur->init_state.fact[i] = state->val[i];
        fact_id = planFactId(&heur->relax.cref.fact_id, i, state->val[i]);
        heur->is_init_fact[fact_id] = 1;
    }
}

static void sendRequest(plan_heur_ma_max_t *heur, plan_ma_comm_t *comm,
                        int agent_id)
{
    plan_ma_msg_t *msg;
    plan_oparr_t *oparr;
    int i, op_id, value;

    msg = planMAMsgNew(PLAN_MA_MSG_HEUR, PLAN_MA_MSG_HEUR_MAX_REQUEST,
                       planMACommId(comm));
    planMAMsgHeurMaxSetRequest(msg, heur->init_state.fact,
                               heur->init_state.size);

    oparr = heur->agent_op + agent_id;
    for (i = 0; i < oparr->size; ++i){
        op_id = planOpIdTrGlob(&heur->op_id_tr, oparr->op[i]);
        value = heur->relax.op[oparr->op[i]].value;
        planMAMsgHeurMaxAddOp(msg, op_id, value);
    }
    planMACommSendToNode(comm, agent_id, msg);
    planMAMsgDel(msg);
}

static int heurMAMax(plan_heur_t *_heur, plan_ma_comm_t *comm,
                     const plan_state_t *state, plan_heur_res_t *res)
{
    plan_heur_ma_max_t *heur = HEUR(_heur);
    int i;

    // Store state for which we are evaluating heuristics
    setInitState(heur, state);

    // Every agent must be requested at least once, so make sure of that.
    for (i = 0; i < heur->agent_size; ++i){
        heur->agent_changed[i] = 1;
        if (i == planMACommId(comm))
            heur->agent_changed[i] = 0;
    }
    heur->cur_agent_id = (planMACommId(comm) + 1) % heur->agent_size;
    heur->agent_id = comm->node_id;

    // h^max heuristics for all facts and operators reachable from the
    // state
    planHeurRelaxFull(&heur->relax, state);

    // Send request to next agent
    sendRequest(heur, comm, heur->cur_agent_id);

    return -1;
}

static int updateFactValue(plan_heur_ma_max_t *heur, int fact_id)
{
    int i, original_value, value, reached_by_op;
    int size, *ops, op_id, num;

    // Skip facts from initial state
    if (heur->is_init_fact[fact_id])
        return 0;

    size = heur->relax.cref.fact_eff[fact_id].size;
    if (size == 0)
        return 0;

    // Remember original value before change
    original_value = heur->relax.fact[fact_id].value;

    // Determine correct value of fact and operator by which it is reached
    ops = heur->relax.cref.fact_eff[fact_id].op;
    value = INT_MAX;
    reached_by_op = heur->relax.fact[fact_id].supp;
    for (num = 0, i = 0; i < size; ++i){
        op_id = ops[i];

        // Consider only operators that are somehow reachable from initial
        // state. Since we started from projected problem, the operators
        // that have zero .unsat cannot be reachable even in non-projected
        // problem.
        if (heur->relax.op[op_id].unsat != 0)
            continue;

        ++num;
        if (value > heur->relax.op[op_id].value){
            value = heur->relax.op[op_id].value;
            reached_by_op = op_id;
        }
    }

    if (num == 0)
        return 0;

    // Record new value and supporting operator
    heur->relax.fact[fact_id].value = value;
    heur->relax.fact[fact_id].supp = reached_by_op;

    if (value != original_value)
        return -1;
    return 0;
}

static void updateQueueWithEffects(plan_heur_ma_max_t *heur,
                                   plan_prio_queue_t *queue,
                                   int op_id)
{
    int i, fact_id, value, supp_op;
    plan_factarr_t *eff;

    eff = heur->relax.cref.op_eff + op_id;
    for (i = 0; i < eff->size; ++i){
        fact_id = eff->fact[i];
        supp_op = heur->relax.fact[fact_id].supp;
        if (supp_op != op_id)
            continue;

        if (updateFactValue(heur, fact_id) != 0){
            value = heur->relax.fact[fact_id].value;
            planPrioQueuePush(queue, value, fact_id);
        }
    }
}

static void updateHMaxFact(plan_heur_ma_max_t *heur,
                           plan_prio_queue_t *queue,
                           int fact_id, int fact_value)
{
    int i, *ops, ops_size, owner, op_id;
    plan_heur_relax_op_t *op;

    ops      = heur->relax.cref.fact_pre[fact_id].op;
    ops_size = heur->relax.cref.fact_pre[fact_id].size;
    for (i = 0; i < ops_size; ++i){
        op = heur->relax.op + ops[i];
        if (op->value < op->cost + fact_value){
            op->value = op->cost + fact_value;
            updateQueueWithEffects(heur, queue, ops[i]);

            // Set flag the agent owner needs update due to this change
            op_id = heur->relax.cref.op_id[ops[i]];
            if (op_id >= 0){
                owner = heur->op_owner[op_id];
                if (owner != heur->agent_id)
                    heur->agent_changed[owner] = 1;
            }
        }
    }
}

static void updateHMax(plan_heur_ma_max_t *heur, plan_prio_queue_t *queue)
{
    int fact_id, fact_value;
    plan_heur_relax_fact_t *fact;

    while (!planPrioQueueEmpty(queue)){
        fact_id = planPrioQueuePop(queue, &fact_value);
        fact = heur->relax.fact + fact_id;
        if (fact->value != fact_value)
            continue;

        updateHMaxFact(heur, queue, fact_id, fact_value);
    }
}

static int needsUpdate(const plan_heur_ma_max_t *heur)
{
    int i, val = 0;
    for (i = 0; i < heur->agent_size; ++i)
        val += heur->agent_changed[i];
    return val;
}

static void updateOpValue(plan_heur_ma_max_t *heur, const plan_ma_msg_t *msg,
                          plan_prio_queue_t *queue)
{
    int i, *update_op, update_op_size, op_id, value;
    int response_op_size, response_op_id, response_value;

    response_op_size = planMAMsgHeurMaxOpSize(msg);
    update_op = alloca(sizeof(int) * response_op_size);
    update_op_size = 0;
    for (i = 0; i < response_op_size; ++i){
        // Read data for operator
        response_op_id = planMAMsgHeurMaxOp(msg, i, &response_value);

        // Translate operator ID from response to local ID
        op_id = planOpIdTrLoc(&heur->op_id_tr, response_op_id);

        // Determine current value of operator
        value = heur->relax.op[op_id].value;

        // Record updated value of operator and remember operator for
        // later.
        if (value != response_value){
            heur->relax.op[op_id].value = response_value;
            update_op[update_op_size++] = op_id;
        }
    }

    // All updated values are record now we can check the effects of the
    // changed operators.
    for (i = 0; i < update_op_size; ++i){
        // Update queue with changed facts
        updateQueueWithEffects(heur, queue, update_op[i]);
    }
}

static int heurMAMaxUpdate(plan_heur_t *_heur, plan_ma_comm_t *comm,
                           const plan_ma_msg_t *msg, plan_heur_res_t *res)
{
    plan_heur_ma_max_t *heur = HEUR(_heur);
    int other_agent_id;
    plan_prio_queue_t queue;

    if (planMAMsgType(msg) != PLAN_MA_MSG_HEUR
            || planMAMsgSubType(msg) != PLAN_MA_MSG_HEUR_MAX_RESPONSE){
        fprintf(stderr, "Error[%d]: Heur response isn't for h^max"
                        " heuristic.\n", comm->node_id);
        return -1;
    }

    other_agent_id = planMAMsgAgent(msg);
    if (other_agent_id != heur->cur_agent_id){
        fprintf(stderr, "Error[%d]: Heur response from %d is expected, instead"
                        " response from %d is received.\n",
                        comm->node_id, heur->cur_agent_id, other_agent_id);
        return -1;
    }

    heur->agent_changed[other_agent_id] = 0;

    planPrioQueueInit(&queue);

    // Update values of the operators in response
    updateOpValue(heur, msg, &queue);

    // Update h^max values of facts and operators
    updateHMax(heur, &queue);

    planPrioQueueFree(&queue);

    if (!needsUpdate(heur)){
        res->heur = heur->relax.fact[heur->relax.cref.goal_id].value;
        return 0;
    }

    do {
        heur->cur_agent_id = (heur->cur_agent_id + 1) % heur->agent_size;
    } while (!heur->agent_changed[heur->cur_agent_id]);
    sendRequest(heur, comm, heur->cur_agent_id);

    return -1;
}



// TODO: Move all this to heur_relax.c
static void requestInit(private_t *private,
                        const plan_fact_id_t *fid,
                        const plan_op_id_tr_t *op_id_tr,
                        const plan_ma_msg_t *msg,
                        plan_prio_queue_t *queue)
{
    int i, op_id, op_len, op_value, fact_id;

    // First initialize operators' and facts' values
    memcpy(private->relax.op, private->relax.op_init,
           sizeof(plan_heur_relax_op_t) * private->relax.cref.op_size);
    memcpy(private->relax.fact, private->relax.fact_init,
           sizeof(plan_heur_relax_fact_t) * private->relax.cref.fact_size);

    op_len = planMAMsgHeurMaxOpSize(msg);
    for (i = 0; i < op_len; ++i){
        op_id = planMAMsgHeurMaxOp(msg, i, &op_value);
        op_id = planOpIdTrLoc(op_id_tr, op_id);
        if (op_id < 0)
            continue;
        private->relax.op[op_id].value = op_value;

        // We push artificial fact into queue with priority equaling to
        // value of the operator. When we pop this value from queue we just
        // decrease .unsat counter and check whether all other
        // preconditions were met. This is the way how to prevent cycling
        // between facts and operators because we pretend that these
        // artificial facts are initial state so the rest of the algorithm
        // so (more or less) same as in ordinary h^max heuristic.
        private->relax.op[op_id].unsat += 1;
        planPrioQueuePush(queue, op_value, private->relax.cref.fact_size + op_id);
    }

    // Set facts from initial state to 0 and add them to the queue
    for (i = 0; i < fid->var_size; ++i){
        fact_id = planFactId(fid, i, planMAMsgHeurMaxStateVal(msg, i));

        // Initial facts initialize to zero and push them into queue
        private->relax.fact[fact_id].value = 0;
        planPrioQueuePush(queue, 0, fact_id);
    }
    fact_id = private->relax.cref.no_pre_id;
    private->relax.fact[fact_id].value = 0;
    planPrioQueuePush(queue, 0, fact_id);
}

static void requestEnqueueOpEff(private_t *private, int op_id,
                                plan_prio_queue_t *queue)
{
    int fact_value, op_value;
    int i, len, *fact_ids, fact_id;

    op_value = private->relax.op[op_id].value;
    len      = private->relax.cref.op_eff[op_id].size;
    fact_ids = private->relax.cref.op_eff[op_id].fact;
    for (i = 0; i < len; ++i){
        fact_id = fact_ids[i];
        fact_value = private->relax.fact[fact_id].value;

        if (fact_value > op_value){
            private->relax.fact[fact_id].value = op_value;
            planPrioQueuePush(queue, op_value, fact_id);
        }
    }
}

static void requestSendResponse(private_t *private,
                                plan_ma_comm_t *comm,
                                const plan_op_id_tr_t *op_id_tr,
                                const plan_ma_msg_t *req_msg)
{
    plan_ma_msg_t *msg;
    int target_agent, i, len, op_id, old_value, new_value, loc_op_id;

    msg = planMAMsgNew(PLAN_MA_MSG_HEUR, PLAN_MA_MSG_HEUR_MAX_RESPONSE,
                       planMACommId(comm));

    len = planMAMsgHeurMaxOpSize(req_msg);
    for (i = 0; i < len; ++i){
        op_id = planMAMsgHeurMaxOp(req_msg, i, &old_value);

        loc_op_id = planOpIdTrLoc(op_id_tr, op_id);
        if (loc_op_id < 0)
            continue;

        new_value = private->relax.op[loc_op_id].value;
        if (new_value != old_value){
            planMAMsgHeurMaxAddOp(msg, op_id, new_value);
        }
    }

    target_agent = planMAMsgAgent(req_msg);
    planMACommSendToNode(comm, target_agent, msg);
    planMAMsgDel(msg);
}

static void requestSendEmptyResponse(plan_ma_comm_t *comm,
                                     const plan_ma_msg_t *req)
{
    plan_ma_msg_t *msg;
    int target_agent;

    msg = planMAMsgNew(PLAN_MA_MSG_HEUR, PLAN_MA_MSG_HEUR_MAX_RESPONSE,
                       planMACommId(comm));
    target_agent = planMAMsgAgent(req);
    planMACommSendToNode(comm, target_agent, msg);
    planMAMsgDel(msg);
}

static void requestMain(private_t *private, plan_prio_queue_t *queue,
                        int fact_size)
{
    int i, fact_id, fact_value, op_id, op_len, *ops, op_value, op_value2;

    while (!planPrioQueueEmpty(queue)){
        fact_id = planPrioQueuePop(queue, &fact_value);

        if (fact_id >= fact_size){
            // The poped fact is an artificial fact that signals that the
            // corresponding operator should be processed. This way we
            // prevent cycling between facts and operators.
            op_id = fact_id - fact_size;
            --private->relax.op[op_id].unsat;
            if (private->relax.op[op_id].unsat == 0)
                requestEnqueueOpEff(private, op_id, queue);
            continue;
        }

        if (fact_value != private->relax.fact[fact_id].value)
            continue;

        op_len = private->relax.cref.fact_pre[fact_id].size;
        ops    = private->relax.cref.fact_pre[fact_id].op;
        for (i = 0; i < op_len; ++i){
            op_id = ops[i];
            op_value = private->relax.op[op_id].value;
            op_value2 = private->relax.op[op_id].cost + fact_value;
            private->relax.op[op_id].value = BOR_MAX(op_value, op_value2);

            --private->relax.op[op_id].unsat;
            if (private->relax.op[op_id].unsat == 0)
                requestEnqueueOpEff(private, op_id, queue);
        }
    }
}
static void heurMAMaxRequest(plan_heur_t *_heur, plan_ma_comm_t *comm,
                             const plan_ma_msg_t *msg)
{
    plan_heur_ma_max_t *heur = HEUR(_heur);
    private_t *private = &heur->private;
    plan_prio_queue_t queue;

    if (planMAMsgType(msg) != PLAN_MA_MSG_HEUR
            || planMAMsgSubType(msg) != PLAN_MA_MSG_HEUR_MAX_REQUEST){
        fprintf(stderr, "Error[%d]: Heur request isn't for h^max"
                        " heuristic.\n", comm->node_id);
        return;
    }

    // Early exit if we have no private operators
    if (private->relax.cref.op_size == 0){
        requestSendEmptyResponse(comm, msg);
        return;
    }

    // Initialize priority queue with all private facts
    planPrioQueueInit(&queue);

    // Initialize work arrays
    requestInit(private, &private->relax.cref.fact_id, &private->op_id_tr,
                msg, &queue);

    // Run main h^max loop
    requestMain(private, &queue, private->relax.cref.fact_size);

    // Send operator's new values back as response
    requestSendResponse(private, comm, &private->op_id_tr, msg);

    planPrioQueueFree(&queue);
}
