#include "plan/heur.h"

#define HEUR_RELAX_MAX
#define HEUR_RELAX_MAX_FULL
#define HEUR_FACT_OP_FACT_EFF
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

struct _private_t {
    int fact_size;     /*!< Number of private facts */
    int op_size;       /*!< Number of private operators */
    int *fact_id;      /*!< List of all private facts IDs */
    int *op_id;        /*!< List of all private ops IDs */
    int *fact;         /*!< Working array for facts */
    private_op_t *op;  /*!< Working array for operators */
    private_op_t *op_init;

    plan_heur_factarr_t *op_eff; /*!< Unrolled private effects of operators */
    plan_heur_oparr_t *fact_pre; /*!< Operators for which is the private
                                      fact is precondition */
    plan_heur_oparr_t *fact_eff; /*!< Operators for which is the private
                                      fact is effect */
};
typedef struct _private_t private_t;

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
    plan_heur_factarr_t cur_state; /*!< Current state for which h^max is computed */
    int *cur_state_flag; /*!< 1 for each fact in .cur_state */

    private_t private; /*!< Data for processing requests */
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

/** Initializes private_t struct from problem definition */
static void privateInit(private_t *private,
                        const plan_heur_fact_op_t *fact_op,
                        const plan_problem_t *prob);
/** Frees allocated resources */
static void privateFree(private_t *private,
                        const plan_heur_fact_op_t *fact_op);




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

plan_heur_t *planHeurMARelaxMaxNew(const plan_problem_t *prob)
{
    plan_heur_ma_max_t *heur;
    int i, max_glob_id, glob_id;

    heur = BOR_ALLOC(plan_heur_ma_max_t);
    _planHeurInit(&heur->heur, heurDel, NULL);
    _planHeurMAInit(&heur->heur, heurMAMax,
                    heurMAMaxUpdate, heurMAMaxRequest);

    planHeurRelaxInit(&heur->relax,
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
    heur->cur_state_flag = BOR_ALLOC_ARR(int, heur->relax.data.fact_id.fact_size);

    privateInit(&heur->private, &heur->relax.data, prob);

    return &heur->heur;
}

static void heurDel(plan_heur_t *_heur)
{
    plan_heur_ma_max_t *heur = HEUR(_heur);
    int i;

    privateFree(&heur->private, &heur->relax.data);

    for (i = 0; i < heur->agent_size; ++i){
        if (heur->agent_op[i].op)
            BOR_FREE(heur->agent_op[i].op);
    }
    BOR_FREE(heur->agent_op);

    if (heur->agent_change)
        BOR_FREE(heur->agent_change);
    if (heur->cur_state.fact)
        BOR_FREE(heur->cur_state.fact);
    if (heur->cur_state_flag)
        BOR_FREE(heur->cur_state_flag);
    if (heur->op_glob_id_to_id)
        BOR_FREE(heur->op_glob_id_to_id);

    planHeurRelaxFree(&heur->relax);
    _planHeurFree(&heur->heur);
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
    int i, fact_id;

    // Remember current state
    bzero(heur->cur_state_flag, sizeof(int) * heur->relax.data.fact_id.fact_size);
    for (i = 0; i < heur->cur_state.size; ++i){
        heur->cur_state.fact[i] = state->val[i];
        fact_id = planHeurFactId(&heur->relax.data.fact_id, i, state->val[i]);
        heur->cur_state_flag[fact_id] = 1;
    }

    // Compute heuristic on projected problem
    planHeurRelax(&heur->relax, state, res);

    for (i = 0; i < heur->agent_size; ++i){
        heur->agent_change[i] = 1;
        if (i == comm->node_id)
            heur->agent_change[i] = 0;
    }

    heur->agent_id = comm->node_id;
    heur->cur_agent = (heur->agent_id + 1) % heur->agent_size;

    sendRequest(heur, comm, heur->cur_agent);

    return -1;
}

static int updateFactValue(plan_heur_ma_max_t *heur, int fact_id)
{
    plan_heur_oparr_t *ops;
    int i, original_value, value;

    if (heur->cur_state_flag[fact_id])
        return 0;

    // Remember original value before change
    original_value = heur->relax.fact[fact_id].value;

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
    plan_heur_factarr_t *eff;

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
    plan_heur_op_t *op;

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

    // Mark the agent we are receiving from as change-free
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

    planPrioQueueFree(&heur->relax.queue);

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






/** Request Processing **/
static void requestInit(private_t *private,
                        const plan_heur_fact_op_t *fact_op,
                        const int *op_glob_id_to_id,
                        const plan_ma_msg_t *msg,
                        plan_prio_queue_t *queue)
{
    int i, op_id, op_len, op_value, fact_id;

    // First initialize operators values
    memcpy(private->op, private->op_init,
           sizeof(private_op_t) * fact_op->op_size);

    op_len = planMAMsgHeurMaxRequestOpSize(msg);
    for (i = 0; i < op_len; ++i){
        op_id = planMAMsgHeurMaxRequestOp(msg, i, &op_value);
        op_id = op_glob_id_to_id[op_id];
        private->op[op_id].value = op_value;

        // We push artificial fact into queue with priority equaling to
        // value of the operator. When we pop this value from queue we just
        // decrease .unsat counter and check whether all other
        // preconditions were met. This is the way how to prevent cycling
        // between facts and operators because we pretend that these
        // artificial facts are initial state so the rest of the algorithm
        // so (more or less) same as in ordinary h^max heuristic.
        private->op[op_id].unsat += 1;
        planPrioQueuePush(queue, op_value, fact_op->fact_size + op_id);
    }

    // Initialize fact values as INT_MAX
    for (i = 0; i < private->fact_size; ++i){
        fact_id = private->fact_id[i];
        private->fact[fact_id] = INT_MAX;
    }

    // Set facts from initial state to 0 and add them to the queue
    for (i = 0; i < fact_op->fact_id.var_size; ++i){
        fact_id = planHeurFactId(&fact_op->fact_id, i,
                                 planMAMsgHeurMaxRequestState(msg, i));

        // Initial facts initialize to zero and push them into queue
        private->fact[fact_id] = 0;
        planPrioQueuePush(queue, 0, fact_id);
    }
}

static void requestEnqueueOpEff(private_t *private, int op_id,
                                plan_prio_queue_t *queue)
{
    int fact_value, op_value;
    int i, len, *fact_ids, fact_id;

    op_value = private->op[op_id].value;
    len      = private->op_eff[op_id].size;
    fact_ids = private->op_eff[op_id].fact;
    for (i = 0; i < len; ++i){
        fact_id = fact_ids[i];
        fact_value = private->fact[fact_id];

        if (fact_value > op_value){
            private->fact[fact_id] = op_value;
            planPrioQueuePush(queue, op_value, fact_id);
        }
    }
}

static void requestSendResponse(private_t *private,
                                plan_ma_comm_t *comm,
                                const int *op_glob_id_to_id,
                                const plan_ma_msg_t *req_msg)
{
    plan_ma_msg_t *msg;
    int target_agent, i, len, op_id, old_value, new_value, loc_op_id;

    msg = planMAMsgNew();
    planMAMsgSetHeurMaxResponse(msg, comm->node_id);

    len = planMAMsgHeurMaxRequestOpSize(req_msg);
    for (i = 0; i < len; ++i){
        op_id = planMAMsgHeurMaxRequestOp(req_msg, i, &old_value);

        loc_op_id = op_glob_id_to_id[op_id];
        new_value = private->op[loc_op_id].value;
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
            --private->op[op_id].unsat;
            if (private->op[op_id].unsat == 0)
                requestEnqueueOpEff(private, op_id, queue);
            continue;
        }

        if (fact_value != private->fact[fact_id])
            continue;

        op_len = private->fact_pre[fact_id].size;
        ops    = private->fact_pre[fact_id].op;
        for (i = 0; i < op_len; ++i){
            op_id = ops[i];
            op_value = private->op[op_id].value;
            op_value2 = private->op[op_id].cost + fact_value;
            private->op[op_id].value = BOR_MAX(op_value, op_value2);

            --private->op[op_id].unsat;
            if (private->op[op_id].unsat == 0)
                requestEnqueueOpEff(private, op_id, queue);
        }
    }
}

static void heurMAMaxRequest(plan_heur_t *_heur, plan_ma_comm_t *comm,
                             const plan_ma_msg_t *msg)
{
    plan_heur_ma_max_t *heur = HEUR(_heur);
    plan_prio_queue_t queue;

    if (!planMAMsgIsHeurMaxRequest(msg)){
        fprintf(stderr, "Error: Heur request isn't for h^max heuristic.\n");
        return;
    }

    // Early exit if we have no private operators
    if (heur->private.op_size == 0){
        requestSendEmptyResponse(comm, msg);
        return;
    }

    // Initialize priority queue with all private facts
    planPrioQueueInit(&queue);

    // Initialize work arrays
    requestInit(&heur->private, &heur->relax.data, heur->op_glob_id_to_id,
                msg, &queue);

    // Run main h^max loop
    requestMain(&heur->private, &queue, heur->relax.data.fact_size);

    // Send operator's new values back as response
    requestSendResponse(&heur->private, comm, heur->op_glob_id_to_id, msg);

    planPrioQueueFree(&queue);
}




/** Private Part Initialization **/

/** Finds all private facts and stores them in internal structure. */
static void privateCollectFacts(private_t *private,
                                const plan_heur_fact_id_t *fid,
                                const plan_problem_t *prob)
{
    int i, len, fact_id;
    plan_problem_private_val_t *val;

    len = prob->private_val_size;
    val = prob->private_val;
    private->fact_size = 0;
    for (i = 0; i < len; ++i){
        fact_id = planHeurFactId(fid, val[i].var, val[i].val);
        private->fact_id[private->fact_size++] = fact_id;
    }
}

/** Project private facts of operators */
static void privateProjectOps(private_t *private,
                              const plan_heur_fact_op_t *fact_op)
{
    int i, j, k;
    plan_heur_factarr_t *dst;
    const plan_heur_factarr_t *src;

    for (i = 0; i < fact_op->op_size; ++i){
        dst = private->op_eff + i;
        src = fact_op->op_eff + i;

        dst->fact = BOR_ALLOC_ARR(int, src->size);
        dst->size = 0;
        for (j = 0, k = 0; j < src->size && k < private->fact_size;){
            if (src->fact[j] == private->fact_id[k]){
                dst->fact[dst->size++] = private->fact_id[k];
                ++j;
                ++k;
            }else if (src->fact[j] < private->fact_id[k]){
                ++j;
            }else{
                ++k;
            }
        }

        dst->fact = BOR_REALLOC_ARR(dst->fact, int, dst->size);
    }
}

/** Projects private facts */
static void privateProjectFacts(private_t *private,
                                const plan_heur_fact_op_t *fact_op)
{
    int i, j, fact_id;
    plan_heur_oparr_t *src, *dst;

    for (i = 0; i < private->fact_size; ++i){
        fact_id = private->fact_id[i];

        dst = private->fact_pre + fact_id;
        src = fact_op->fact_pre + fact_id;
        dst->size = src->size;
        dst->op = BOR_ALLOC_ARR(int, dst->size);
        memcpy(dst->op, src->op, sizeof(int) * dst->size);

        for (j = 0; j < dst->size; ++j){
            private->op_init[dst->op[j]].unsat += 1;
        }

        dst = private->fact_eff + fact_id;
        src = fact_op->fact_eff + fact_id;
        dst->size = src->size;
        dst->op = BOR_ALLOC_ARR(int, dst->size);
        memcpy(dst->op, src->op, sizeof(int) * dst->size);
    }
}

static void privateInitOps(private_t *private,
                           const plan_heur_fact_op_t *fact_op)
{
    int i, j;
    int *private_op_id;

    // Set 0/1 flag for each operator that is private (contains at least
    // one private fact in pre or eff)
    private_op_id = BOR_CALLOC_ARR(int, fact_op->op_size);
    for (i = 0; i < fact_op->op_size; ++i){
        if (private->op_eff[i].size > 0)
            private_op_id[i] = 1;
    }
    for (i = 0; i < fact_op->fact_size; ++i){
        for (j = 0; j < private->fact_pre[i].size; ++j){
            private_op_id[private->fact_pre[i].op[j]] = 1;
        }
    }

    // Record IDs private operators and also set operator's cost
    private->op_size = 0;
    for (i = 0, j = 0; i < fact_op->op_size; ++i){
        if (private_op_id[i]){
            private->op_id[private->op_size++] = i;
            private->op_init[i].cost = fact_op->op[i].cost;
        }
    }

    // Give back some memory
    private->op_id = BOR_REALLOC_ARR(private->op_id, int, private->op_size);

    BOR_FREE(private_op_id);
}

static void privateInit(private_t *private,
                        const plan_heur_fact_op_t *fact_op,
                        const plan_problem_t *prob)
{
    // First allocate all memory
    private->fact_id   = BOR_ALLOC_ARR(int, prob->private_val_size);
    private->op_id     = BOR_CALLOC_ARR(int, fact_op->op_size);
    private->fact      = BOR_ALLOC_ARR(int, fact_op->fact_size);
    private->op        = BOR_ALLOC_ARR(private_op_t, fact_op->op_size);
    private->op_init   = BOR_CALLOC_ARR(private_op_t, fact_op->op_size);
    private->op_eff    = BOR_CALLOC_ARR(plan_heur_factarr_t, fact_op->op_size);
    private->fact_pre  = BOR_CALLOC_ARR(plan_heur_oparr_t, fact_op->fact_size);
    private->fact_eff  = BOR_CALLOC_ARR(plan_heur_oparr_t, fact_op->fact_size);

    // Set up .fact_id[] and .fact_size
    privateCollectFacts(private, &fact_op->fact_id, prob);
    // Set up .op_eff[]
    privateProjectOps(private, fact_op);
    // Set up .fact_pre[] and .fact_eff[]
    privateProjectFacts(private, fact_op);
    // Fininsh .op_init[] and fill .op_id[] and set .op_size
    privateInitOps(private, fact_op);
}

static void privateFree(private_t *private,
                        const plan_heur_fact_op_t *fact_op)
{
    BOR_FREE(private->fact_id);
    BOR_FREE(private->op_id);
    BOR_FREE(private->fact);
    BOR_FREE(private->op);
    BOR_FREE(private->op_init);
    planHeurFactarrFree(private->op_eff, fact_op->op_size);
    planHeurOparrFree(private->fact_pre, fact_op->fact_size);
    planHeurOparrFree(private->fact_eff, fact_op->fact_size);
}
