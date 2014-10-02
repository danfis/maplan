#include "plan/heur.h"

#include "_heur_relax.c"

struct _proj_op_t {
    int op_id;   /*!< Op's ID in .relax.op[] array */
    int glob_id; /*!< Global ID of the operator */
};
typedef struct _proj_op_t proj_op_t;

struct _agent_op_t {
    proj_op_t *op;
    int op_size;
};
typedef struct _agent_op_t agent_op_t;

struct _private_op_t {
    plan_cost_t cost;
    plan_cost_t value;
    int unsat;
};
typedef struct _private_op_t private_op_t;

struct _plan_heur_ma_max_t {
    plan_heur_t heur;
    plan_heur_relax_t relax;

    int *op_glob_id_to_id; /*!< Mapping between global ID and ID in
                                .relax.op[] array */
    agent_op_t *agent_op; /*!< Operators splitted by their owner */
    int agent_size;
    int agent_id;
    int cur_agent; /*!< ID of agent from which we expect repsponse */
    int *agent_change; /*!< Flag for each agent whether change affects that
                            agent. */
    factarr_t cur_state; /*!< Current state for which h^max is computed */

    int private_fact_size;     /*!< Number of private facts */
    int private_op_size;       /*!< Number of private operators */
    int *private_fact_id;      /*!< List of all private facts IDs */
    int *private_op_id;        /*!< List of all private ops IDs */
    int *private_fact;         /*!< Working array for facts */
    private_op_t *private_op;  /*!< Working array for operators */
    private_op_t *private_op_init;
    factarr_t *private_op_eff; /*!< Unrolled private effects of operators */
    oparr_t *private_fact_pre; /*!< Operators for which is the private fact
                                    is precondition */
    oparr_t *private_fact_eff; /*!< Operators for which is the private fact
                                    is effect */
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

static int maxGlobId(const plan_op_t *op, int size)
{
    int i, glob_id, max_glob_id = 0;
    for (i = 0; i < size; ++i){
        glob_id = planOpExtraMAProjOpGlobalId(op + i);
        max_glob_id = BOR_MAX(max_glob_id, glob_id);
    }

    return max_glob_id;
}

static void initAgentOp(plan_heur_ma_max_t *heur,
                        const plan_problem_t *prob)
{
    int i, j, owner, glob_id;
    agent_op_t *agent_op;
    proj_op_t *proj_op;

    heur->agent_op = NULL;
    heur->agent_size = 0;
    for (i = 0; i < prob->proj_op_size; ++i){
        owner   = planOpExtraMAProjOpOwner(prob->proj_op + i);
        glob_id = planOpExtraMAProjOpGlobalId(prob->proj_op + i);

        if (owner >= heur->agent_size){
            heur->agent_op = BOR_REALLOC_ARR(heur->agent_op,
                                             agent_op_t, owner + 1);
            for (j = heur->agent_size; j < owner + 1; ++j){
                agent_op = heur->agent_op + j;
                agent_op->op = BOR_ALLOC_ARR(proj_op_t, prob->proj_op_size);
                agent_op->op_size = 0;
            }
            heur->agent_size = owner + 1;
        }

        agent_op = heur->agent_op + owner;
        proj_op = agent_op->op + agent_op->op_size++;
        proj_op->op_id = i;
        proj_op->glob_id = glob_id;
    }

    for (i = 0; i < heur->agent_size; ++i){
        agent_op = heur->agent_op + i;
        agent_op->op = BOR_REALLOC_ARR(agent_op->op, proj_op_t,
                                       agent_op->op_size);
    }
}

static void initPrivateFact(plan_heur_ma_max_t *heur,
                            const plan_problem_t *prob)
{
    int i, len, fact_id;
    plan_problem_private_val_t *val;
    val_to_id_t *vid = &heur->relax.data.vid;

    heur->private_fact_id = BOR_ALLOC_ARR(int, heur->relax.data.fact_size);
    heur->private_fact_size = 0;
    heur->private_fact = BOR_ALLOC_ARR(int, heur->relax.data.fact_size);

    len = prob->private_val_size;
    val = prob->private_val;
    for (i = 0; i < len; ++i){
        fact_id = valToId(vid, val[i].var, val[i].val);
        heur->private_fact_id[heur->private_fact_size++] = fact_id;
    }

    heur->private_fact_id = BOR_REALLOC_ARR(heur->private_fact_id, int,
                                            heur->private_fact_size);
}

static void initProjectPrivateOpEff(plan_heur_ma_max_t *heur, int op_id)
{
    int i, j, op_size, fact_size;
    int *fact;
    factarr_t *dst_fact;

    dst_fact = heur->private_op_eff + op_id;

    fact_size = heur->relax.data.op_eff[op_id].size;
    fact = heur->relax.data.op_eff[op_id].fact;
    for (i = 0, j = 0; i < fact_size && j < heur->private_fact_size;){
        if (fact[i] == heur->private_fact_id[j]){
            ++dst_fact->size;
            dst_fact->fact = BOR_REALLOC_ARR(dst_fact->fact, int,
                                             dst_fact->size);
            dst_fact->fact[dst_fact->size - 1] = fact[i];

            ++i;
            ++j;
        }else if (fact[i] < heur->private_fact_id[j]){
            ++i;
        }else{
            ++j;
        }
    }
}

static void initProjectPrivateFact(plan_heur_ma_max_t *heur)
{
    int i, j, fact_id;
    oparr_t *src, *dst;

    for (i = 0; i < heur->private_fact_size; ++i){
        fact_id = heur->private_fact_id[i];

        dst = heur->private_fact_pre + fact_id;
        src = heur->relax.data.fact_pre + fact_id;
        dst->size = src->size;
        dst->op = BOR_ALLOC_ARR(int, dst->size);
        memcpy(dst->op, src->op, sizeof(int) * dst->size);

        for (j = 0; j < dst->size; ++j){
            heur->private_op_init[dst->op[j]].unsat += 1;
        }

        dst = heur->private_fact_eff + fact_id;
        src = heur->relax.data.fact_eff + fact_id;
        dst->size = src->size;
        dst->op = BOR_ALLOC_ARR(int, dst->size);
        memcpy(dst->op, src->op, sizeof(int) * dst->size);
    }
}

static void initPrivateOp(plan_heur_ma_max_t *heur,
                          const plan_problem_t *prob)
{
    int i, op_size;

    op_size = heur->relax.data.op_size;
    heur->private_op = BOR_ALLOC_ARR(private_op_t, op_size);
    heur->private_op_init = BOR_CALLOC_ARR(private_op_t, op_size);
    heur->private_op_eff = BOR_CALLOC_ARR(factarr_t, op_size);
    heur->private_fact_pre = BOR_CALLOC_ARR(oparr_t, heur->relax.data.fact_size);
    heur->private_fact_eff = BOR_CALLOC_ARR(oparr_t, heur->relax.data.fact_size);

    for (i = 0; i < op_size; ++i){
        initProjectPrivateOpEff(heur, i);
        heur->private_op_init[i].cost = heur->relax.data.op[i].cost;
    }
    initProjectPrivateFact(heur);
}

static void initPrivateFactOp(plan_heur_ma_max_t *heur,
                              const plan_problem_t *prob)
{
    int i, j;
    int *private_op_id;

    initPrivateFact(heur, prob);
    initPrivateOp(heur, prob);

    private_op_id = BOR_CALLOC_ARR(int, heur->relax.data.op_size);
    for (i = 0; i < heur->relax.data.op_size; ++i){
        if (heur->private_op_eff[i].size > 0)
            private_op_id[i] = 1;
    }

    for (i = 0; i < heur->relax.data.fact_size; ++i){
        for (j = 0; j < heur->private_fact_pre[i].size; ++j){
            private_op_id[heur->private_fact_pre[i].op[j]] = 1;
        }
    }

    heur->private_op_size = 0;
    for (i = 0; i < heur->relax.data.op_size; ++i)
        heur->private_op_size += private_op_id[i];

    heur->private_op_id = BOR_ALLOC_ARR(int, heur->private_op_size);
    for (i = 0, j = 0; i < heur->relax.data.op_size; ++i){
        if (private_op_id[i])
            heur->private_op_id[j++] = i;
    }
    BOR_FREE(private_op_id);
}

static void freePrivateFactOp(plan_heur_ma_max_t *heur)
{
    BOR_FREE(heur->private_fact_id);
    BOR_FREE(heur->private_fact);
    factarrFree(heur->private_op_eff, heur->relax.data.op_size);
    oparrFree(heur->private_fact_pre, heur->relax.data.fact_size);
    oparrFree(heur->private_fact_eff, heur->relax.data.fact_size);
    BOR_FREE(heur->private_op);
}

plan_heur_t *planHeurMARelaxMaxNew(const plan_problem_t *prob)
{
    plan_heur_ma_max_t *heur;
    int i, max_glob_id, glob_id;

    heur = BOR_ALLOC(plan_heur_ma_max_t);
    _planHeurInit(&heur->heur, heurDel, NULL);
    _planHeurMAInit(&heur->heur, heurMAMax,
                    heurMAMaxUpdate, heurMAMaxRequest);

    planHeurRelaxInit(&heur->relax, TYPE_MAX,
                      prob->var, prob->var_size,
                      prob->goal,
                      prob->proj_op, prob->proj_op_size,
                      NULL);

    // Create mapping between operators' global ID and it local ID
    max_glob_id = maxGlobId(prob->proj_op, prob->proj_op_size);
    ++max_glob_id;
    heur->op_glob_id_to_id = BOR_ALLOC_ARR(int, max_glob_id);
    for (i = 0; i < prob->proj_op_size; ++i){
        glob_id = planOpExtraMAProjOpGlobalId(prob->proj_op + i);
        heur->op_glob_id_to_id[glob_id] = i;
    }

    initAgentOp(heur, prob);

    heur->agent_change = BOR_ALLOC_ARR(int, heur->agent_size);
    heur->cur_state.fact = BOR_ALLOC_ARR(int, prob->var_size);
    heur->cur_state.size = prob->var_size;

    initPrivateFactOp(heur, prob);

    return &heur->heur;
}

static void heurDel(plan_heur_t *_heur)
{
    plan_heur_ma_max_t *heur = HEUR(_heur);
    int i;

    _planHeurFree(&heur->heur);
    planHeurRelaxFree(&heur->relax);

    for (i = 0; i < heur->agent_size; ++i){
        if (heur->agent_op[i].op)
            BOR_FREE(heur->agent_op[i].op);
    }
    BOR_FREE(heur->agent_op);

    if (heur->agent_change)
        BOR_FREE(heur->agent_change);
    if (heur->cur_state.fact)
        BOR_FREE(heur->cur_state.fact);
    freePrivateFactOp(heur);

    BOR_FREE(heur);
}

static void sendRequest(plan_heur_ma_max_t *heur, plan_ma_comm_t *comm,
                        int agent_id)
{
    plan_ma_msg_t *msg;
    agent_op_t *agent_op;
    int i, op_id, value;

    msg = planMAMsgNew();
    planMAMsgSetHeurMaxRequest(msg, comm->node_id,
                               heur->cur_state.fact, heur->cur_state.size);

    agent_op = heur->agent_op + agent_id;
    for (i = 0; i < agent_op->op_size; ++i){
        op_id = agent_op->op[i].glob_id;
        value = heur->relax.op[agent_op->op[i].op_id].value;
        planMAMsgHeurMaxRequestAddOp(msg, op_id, value);
    }
    planMACommSendToNode(comm, agent_id, msg);
    planMAMsgDel(msg);
}

static int heurMAMax(plan_heur_t *_heur, plan_ma_comm_t *comm,
                     const plan_state_t *state, plan_heur_res_t *res)
{
    plan_heur_ma_max_t *heur = HEUR(_heur);
    int i;

    // Remember current state
    for (i = 0; i < heur->cur_state.size; ++i)
        heur->cur_state.fact[i] = state->val[i];

    // Compute heuristic on projected problem
    planHeurRelax(&heur->relax, state, res);

    for (i = 0; i < heur->agent_size; ++i){
        heur->agent_change[i] = 1;
        if (i == comm->node_id)
            heur->agent_change[i] = 0;
    }

    heur->agent_id = comm->node_id;
    heur->cur_agent = 0;
    if (comm->node_id == 0)
        heur->cur_agent = 1;

    sendRequest(heur, comm, heur->cur_agent);

    return -1;
}

static int updateFactValue(plan_heur_ma_max_t *heur, int fact_id)
{
    oparr_t *ops;
    int i, original_value, value, op_id;

    // Remember original value before change
    original_value = heur->relax.fact[fact_id].value;

    // TODO: This has to be solved differently (maybe) because of zero-cost
    // operators
    {
        int i, fid;
        for (i = 0; i < heur->cur_state.size; ++i){
            fid = valToId(&heur->relax.data.vid, i, heur->cur_state.fact[i]);
            if (fid == fact_id){
                return 0;
            }
        }
    }

    // Determine correct value of fact
    value = INT_MAX;
    ops = heur->relax.data.fact_eff + fact_id;
    for (i = 0; i < ops->size; ++i){
        value = BOR_MIN(value, heur->relax.op[ops->op[i]].value);
    }

    if (ops->size == 0)
        value = 0;

    if (original_value != value){
        heur->relax.fact[fact_id].value = value;
        return -1;
    }

    return 0;
}

static void updateQueueWithEffects(plan_heur_ma_max_t *heur,
                                   plan_prio_queue_t *queue,
                                   int op_id)
{
    int i, fact_id, value;
    factarr_t *eff;

    eff = heur->relax.data.op_eff + op_id;
    for (i = 0; i < eff->size; ++i){
        fact_id = eff->fact[i];
        if (updateFactValue(heur, fact_id) != 0){
            value = heur->relax.fact[fact_id].value;
            planPrioQueuePush(queue, value, fact_id);
        }
    }
}

static void updateQueueWithResponseOp(plan_heur_ma_max_t *heur,
                                      int op_id, int value)
{
    // Update operator's value
    heur->relax.op[op_id].value = value;

    // Update queue with changed facts
    updateQueueWithEffects(heur, &heur->relax.queue, op_id);
}

static void updateHMaxFact(plan_heur_ma_max_t *heur,
                           plan_prio_queue_t *queue,
                           int fact_id, int fact_value)
{
    int i, *ops, ops_size, owner;
    op_t *op;

    ops      = heur->relax.data.fact_pre[fact_id].op;
    ops_size = heur->relax.data.fact_pre[fact_id].size;
    for (i = 0; i < ops_size; ++i){
        op = heur->relax.op + ops[i];
        if (op->value < op->cost + fact_value){
            op->value = op->cost + fact_value;
            updateQueueWithEffects(heur, queue, ops[i]);

            // Set flag the agent owner needs update due to this change
            owner = planOpExtraMAProjOpOwner(heur->relax.base_op + ops[i]);
            if (owner != heur->agent_id)
                heur->agent_change[owner] = 1;
        }
    }
}

static void updateHMax(plan_heur_ma_max_t *heur)
{
    int fact_id, fact_value;
    fact_t *fact;

    while (!planPrioQueueEmpty(&heur->relax.queue)){
        fact_id = planPrioQueuePop(&heur->relax.queue, &fact_value);
        fact = heur->relax.fact + fact_id;
        if (fact->value != fact_value)
            continue;

        updateHMaxFact(heur, &heur->relax.queue, fact_id, fact_value);
    }
}

static int needsUpdate(const plan_heur_ma_max_t *heur)
{
    int i, val = 0;
    for (i = 0; i < heur->agent_size; ++i)
        val += heur->agent_change[i];
    return val;
}

static int heurMAMaxUpdate(plan_heur_t *_heur, plan_ma_comm_t *comm,
                           const plan_ma_msg_t *msg, plan_heur_res_t *res)
{
    plan_heur_ma_max_t *heur = HEUR(_heur);
    int i, response_op_size, response_op_id, response_value;
    int op_id, value;
    int other_agent_id;

    if (!planMAMsgIsHeurMaxResponse(msg)){
        fprintf(stderr, "Error: Heur response isn't for h^max heuristic.\n");
        return -1;
    }

    other_agent_id = planMAMsgHeurMaxResponseAgent(msg);
    if (other_agent_id != heur->cur_agent){
        fprintf(stderr, "Error: Heur response from %d is expected, instead"
                        " response from %d is received.\n",
                        heur->cur_agent, other_agent_id);
        return -1;
    }

    heur->agent_change[other_agent_id] = 0;

    planPrioQueueInit(&heur->relax.queue);
    response_op_size = planMAMsgHeurMaxResponseOpSize(msg);
    for (i = 0; i < response_op_size; ++i){
        // Read data for operator
        response_op_id = planMAMsgHeurMaxResponseOp(msg, i, &response_value);

        // Translate operator ID from response to local ID
        op_id = heur->op_glob_id_to_id[response_op_id];

        // Determine current value of operator
        value = heur->relax.op[op_id].value;

        // Update queue of facts using this operator if we received changed
        // h^max value
        if (value != response_value)
            updateQueueWithResponseOp(heur, op_id, response_value);
    }

    // Update h^max values of facts and operators
    updateHMax(heur);

    if (!needsUpdate(heur)){
        res->heur = ctxHeur(&heur->relax, NULL);
        return 0;
    }

    do {
        ++heur->cur_agent;
        heur->cur_agent = heur->cur_agent % heur->agent_size;
    } while (!heur->agent_change[heur->cur_agent]);
    sendRequest(heur, comm, heur->cur_agent);

    return -1;
}



static void requestInitFact(plan_heur_ma_max_t *heur, int fact_id)
{
    int i, len, *ops;
    plan_cost_t value;

    len = heur->private_fact_eff[fact_id].size;
    ops = heur->private_fact_eff[fact_id].op;
    value = PLAN_COST_MAX;
    for (i = 0; i < len; ++i){
        value = BOR_MIN(value, heur->private_op[ops[i]].value);
    }

    if (len == 0)
        value = 0;

    heur->private_fact[fact_id] = value;
}

static void requestInit(plan_heur_ma_max_t *heur, const plan_ma_msg_t *msg)
{
    int i, op_id, op_len, op_value, fact_id;

    // First initialize operators values
    memcpy(heur->private_op, heur->private_op_init,
           sizeof(private_op_t) * heur->relax.data.op_size);

    op_len = planMAMsgHeurMaxRequestOpSize(msg);
    for (i = 0; i < op_len; ++i){
        op_id = planMAMsgHeurMaxRequestOp(msg, i, &op_value);
        op_id = heur->op_glob_id_to_id[op_id];
        heur->private_op[op_id].value = op_value;
    }

    // Then initialize facts as min() from incoming operators
    for (i = 0; i < heur->private_fact_size; ++i){
        fact_id = heur->private_fact_id[i];
        requestInitFact(heur, fact_id);
    }

    // Force initial facts to zero
    for (i = 0; i < heur->relax.data.vid.var_size; ++i){
        fact_id = valToId(&heur->relax.data.vid, i,
                          planMAMsgHeurMaxRequestState(msg, i));
        heur->private_fact[fact_id] = 0;
    }
}

static void requestEnqueueAllFacts(plan_heur_ma_max_t *heur,
                                   plan_prio_queue_t *queue)
{
    int i, value, fact_id;

    for (i = 0; i < heur->private_fact_size; ++i){
        fact_id = heur->private_fact_id[i];
        value = heur->private_fact[fact_id];
        planPrioQueuePush(queue, value, fact_id);
    }
}

static void requestEnqueueOpEff(plan_heur_ma_max_t *heur, int op_id,
                                plan_prio_queue_t *queue)
{
    int op_value, fact_value;
    int i, len, *fact_id;

    op_value = heur->private_op[op_id].value;

    len     = heur->private_op_eff[op_id].size;
    fact_id = heur->private_op_eff[op_id].fact;
    for (i = 0; i < len; ++i){
        fact_value = heur->private_fact[fact_id[i]];
        if (op_value < fact_value){
            fact_value = op_value;
            heur->private_fact[fact_id[i]] = fact_value;
            planPrioQueuePush(queue, fact_value, fact_id[i]);
        }
    }
}

static void requestSendResponse(plan_heur_ma_max_t *heur,
                                plan_ma_comm_t *comm,
                                const plan_ma_msg_t *req_msg)
{
    plan_ma_msg_t *msg;
    int target_agent, i, len, op_id, old_value, new_value, loc_op_id;

    msg = planMAMsgNew();
    planMAMsgSetHeurMaxResponse(msg, comm->node_id);

    len = planMAMsgHeurMaxRequestOpSize(req_msg);
    for (i = 0; i < len; ++i){
        op_id = planMAMsgHeurMaxRequestOp(req_msg, i, &old_value);

        loc_op_id = heur->op_glob_id_to_id[op_id];
        new_value = heur->private_op[loc_op_id].value;
        if (new_value != old_value){
            planMAMsgHeurMaxResponseAddOp(msg, op_id, new_value);
        }
    }

    target_agent = planMAMsgHeurMaxRequestAgent(req_msg);
    planMACommSendToNode(comm, target_agent, msg);
    planMAMsgDel(msg);
}

static void requestSendEmptyResponse(plan_ma_comm_t *comm,
                                     const plan_ma_msg_t *req)
{
    plan_ma_msg_t *msg;
    int target_agent;

    msg = planMAMsgNew();
    planMAMsgSetHeurMaxResponse(msg, comm->node_id);
    target_agent = planMAMsgHeurMaxRequestAgent(req);
    planMACommSendToNode(comm, target_agent, msg);
    planMAMsgDel(msg);
}

static void heurMAMaxRequest(plan_heur_t *_heur, plan_ma_comm_t *comm,
                             const plan_ma_msg_t *msg)
{
    plan_heur_ma_max_t *heur = HEUR(_heur);
    plan_prio_queue_t queue;
    int fact_id, fact_value;
    int i, *ops, op_len, op_id, op_value, op_value2;

    if (!planMAMsgIsHeurMaxRequest(msg)){
        fprintf(stderr, "Error: Heur request isn't for h^max heuristic.\n");
        return;
    }

    if (heur->private_op_size == 0){
        requestSendEmptyResponse(comm, msg);
        return;
    }

    // Initialize work arrays
    requestInit(heur, msg);

    // Initialize priority queue with all private facts
    planPrioQueueInit(&queue);
    requestEnqueueAllFacts(heur, &queue);

    while (!planPrioQueueEmpty(&queue)){
        fact_id = planPrioQueuePop(&queue, &fact_value);
        if (fact_value != heur->private_fact[fact_id])
            continue;

        op_len = heur->private_fact_pre[fact_id].size;
        ops    = heur->private_fact_pre[fact_id].op;
        for (i = 0; i < op_len; ++i){
            op_id = ops[i];
            op_value = heur->private_op[op_id].value;
            op_value2 = heur->private_op[op_id].cost + fact_value;
            heur->private_op[op_id].value = BOR_MAX(op_value, op_value2);

            --heur->private_op[op_id].unsat;
            if (heur->private_op[op_id].unsat == 0){
                requestEnqueueOpEff(heur, op_id, &queue);
            }
        }
    }

    // Send operator's new values back as response
    requestSendResponse(heur, comm, msg);

    planPrioQueueFree(&queue);
}
