/***
 * maplan
 * -------
 * Copyright (c)2015 Daniel Fiser <danfis@danfis.cz>,
 * Agent Technology Center, Department of Computer Science,
 * Faculty of Electrical Engineering, Czech Technical University in Prague.
 * All rights reserved.
 *
 * This file is part of maplan.
 *
 * Distributed under the OSI-approved BSD License (the "License");
 * see accompanying file BDS-LICENSE for details or see
 * <http://www.opensource.org/licenses/bsd-license.php>.
 *
 * This software is distributed WITHOUT ANY WARRANTY; without even the
 * implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the License for more information.
 */

#include <boruvka/alloc.h>
#include <boruvka/rand.h>
#include "plan/search.h"
#include "plan/heur.h"
#include "plan/pot.h"

#ifdef PLAN_LP

#define EPS 1E-3

struct _lp_prog_t {
    int cols;   /*!< Number of colums (lp variables) */
    int A_rows; /*!< Number of rows in A (constranits) */
    int *A;     /*!< Constraint matrix */
    int *rhs;   /*!< Right hand side of constriants */
    int *obj;   /*!< Objective function coeficients */
};
typedef struct _lp_prog_t lp_prog_t;

struct _matrix_t {
    int cols;
    int rows;
    int offset;
    int *m;
    int unit;
};
typedef struct _matrix_t matrix_t;

struct _agent_data_t {
    plan_pot_agent_t pot;
    int lp_private_var_size; /*!< Number of LP variables for private parts
                                  of slave agents */
    int lp_var_offset;       /*!< Offset of private LP variables */
    int pot_requested;       /*!< True if potentials were requested before
                                  they were computed. */
};
typedef struct _agent_data_t agent_data_t;

struct _plan_heur_ma_pot_t {
    plan_heur_t heur;
    plan_pot_t pot;

    int agent_id;             /*!< ID of the current agent */
    int agent_size;           /*!< Number of agents in cluster */
    int pending;              /*!< Number of pending requests */
    agent_data_t *agent_data; /*!< Data related to slave agents */
    int *fact_range;          /*!< Array of fact (variable) ranges */
    int fact_range_size;      /*!< Size of .fact_range[] */
    int fact_range_lcm;       /*!< LCM of fact ranges */
    int lp_var_size;          /*!< Overall number of LP variables over all
                                   agents */
    matrix_t secure;          /*!< Matrix for encryption/decryption */

    double init_heur;         /*!< Heuristic value for the initial state */
    int ready;                /*!< True if the agent is ready to compute
                                   heuristic value. */
    unsigned flags;           /*!< Flags passed to the constructor */

    plan_state_t *state;      /*!< Pre-allocated state */
    plan_state_t *state2;     /*!< Pre-allocated state */

    bor_timer_t timer;
};
typedef struct _plan_heur_ma_pot_t plan_heur_ma_pot_t;
#define HEUR(parent) \
    bor_container_of((parent), plan_heur_ma_pot_t, heur)

static void heurDel(plan_heur_t *heur);
static int heurHeurNode(plan_heur_t *heur,
                        plan_ma_comm_t *comm,
                        plan_state_id_t state_id,
                        plan_search_t *search,
                        plan_heur_res_t *res);
static int heurUpdate(plan_heur_t *heur, plan_ma_comm_t *comm,
                      const plan_ma_msg_t *msg, plan_heur_res_t *res);
static void heurRequest(plan_heur_t *heur, plan_ma_comm_t *comm,
                        const plan_ma_msg_t *msg);
static int heurHeur(const plan_heur_ma_pot_t *h,
                    plan_state_id_t state_id,
                    const plan_search_t *search);

/** Adds range to the array */
static void addFactRange(plan_heur_ma_pot_t *h, int range);
/** Request potentials from the master agent */
static void requestPot(plan_ma_comm_t *comm);
/** Request basic info from all slave agents */
static void requestInfo(plan_ma_comm_t *comm);
/** Response basic info to the master agent */
static void responseInfo(plan_heur_ma_pot_t *h, plan_ma_comm_t *comm);
/** Process info received from slave agent */
static void processInfo(plan_heur_ma_pot_t *h, const plan_ma_msg_t *msg);
/** Process all info when they are retrieved from all slave agents */
static void processAllInfo(plan_heur_ma_pot_t *h);
/** Reqest private LPs from slave agents */
static void requestLP(plan_heur_ma_pot_t *h, plan_ma_comm_t *comm);
/** Response private LPs to the master agent */
static void responseLP(plan_heur_ma_pot_t *h, plan_ma_comm_t *comm,
                       const plan_ma_msg_t *msg);
/** Process all private LPs etc and compute potentials */
static void computePot(plan_heur_ma_pot_t *h);
/** Request private part of the initial heuristic value */
static void requestInitHeur(plan_heur_ma_pot_t *h, plan_ma_comm_t *comm);
/** Response private part of the initial heuristic value to the master
 *  agent. */
static void responseInitHeur(plan_heur_ma_pot_t *h, plan_ma_comm_t *comm,
                             const plan_ma_msg_t *msg);
/** Sends potentials to the specified agent.
 *  If potentials are not computed yet, adds agent to a pending list. */
static void sendInitHeur(plan_heur_ma_pot_t *h, plan_ma_comm_t *comm, int agent_id);
/** Sends potentials to the pending agents that already requested */
static void sendInitHeurToPending(plan_heur_ma_pot_t *h, plan_ma_comm_t *comm);

/** Initializes structure, allocates matrices, etc... */
static void lpProgInit(lp_prog_t *prog, const plan_heur_ma_pot_t *h);
/** Frees allocated resources */
static void lpProgFree(lp_prog_t *prog);
/** Sets LP program matrices */
static void lpProgSet(lp_prog_t *prog, const plan_heur_ma_pot_t *h);
/** Compute LP program and stores results in main structure */
static void lpProgCompute(const lp_prog_t *prog, plan_heur_ma_pot_t *h);
//static void lpProgPrint(const lp_prog_t *prog, FILE *fout);

/** Initialize secure matrix */
static void secureInit(matrix_t *m, int num_private_vars, int num_vars,
                       int private_offset, unsigned flags);
/** Encrypt private matrices */
static void secureEncrypt(const matrix_t *m, plan_pot_agent_t *pot);
/** Decrypt potentials */
static void secureDecrypt(const matrix_t *m, double *pot, int pub_var_size);

plan_heur_t *planHeurMAPotNew(const plan_problem_t *p, unsigned flags)
{
    plan_heur_ma_pot_t *h;
    PLAN_STATE_STACK(state, p->state_pool->num_vars);

    h = BOR_ALLOC(plan_heur_ma_pot_t);
    bzero(h, sizeof(*h));
    h->flags = flags;
    _planHeurInit(&h->heur, heurDel, NULL, NULL);
    _planHeurMAInit(&h->heur, NULL, heurHeurNode, heurUpdate, heurRequest);

    planStatePoolGetState(p->state_pool, p->initial_state, &state);
    planPotInit(&h->pot, p->var, p->var_size, p->goal,
                p->op, p->op_size, &state, flags, PLAN_POT_MA);

    h->agent_id = p->agent_id;
    h->agent_size = p->num_agents;
    h->agent_data = BOR_CALLOC_ARR(agent_data_t, h->agent_size);
    h->ready = 0;

    h->state = planStateNew(p->state_pool->num_vars);
    h->state2 = planStateNew(p->state_pool->num_vars);

    return &h->heur;
}

static void heurDel(plan_heur_t *heur)
{
    plan_heur_ma_pot_t *h = HEUR(heur);
    int i;

    for (i = 0; i < h->agent_size; ++i){
        planPotAgentFree(&h->agent_data[i].pot);
    }
    if (h->agent_data)
        BOR_FREE(h->agent_data);
    if (h->fact_range)
        BOR_FREE(h->fact_range);
    if (h->secure.m != NULL)
        BOR_FREE(h->secure.m);

    planStateDel(h->state);
    planStateDel(h->state2);
    planPotFree(&h->pot);
    _planHeurFree(&h->heur);
    BOR_FREE(h);
}

static int heurHeurNode(plan_heur_t *heur,
                        plan_ma_comm_t *comm,
                        plan_state_id_t state_id,
                        plan_search_t *search,
                        plan_heur_res_t *res)
{
    plan_heur_ma_pot_t *h = HEUR(heur);

    planStatePoolGetState(search->state_pool, state_id, h->state);

    if (!h->ready){
        if (comm->node_id == 0){
            if (h->flags & PLAN_HEUR_POT_PRINT_INIT_TIME)
                borTimerStart(&h->timer);
            requestInfo(comm);
            h->pending = comm->node_size - 1;
        }else{
            requestPot(comm);
        }
        return -1;
    }

    res->heur = heurHeur(h, state_id, search);
    return 0;
}

static int heurUpdate(plan_heur_t *heur, plan_ma_comm_t *comm,
                      const plan_ma_msg_t *msg, plan_heur_res_t *res)
{
    plan_heur_ma_pot_t *h = HEUR(heur);
    int type, subtype, agent_id;

    type = planMAMsgType(msg);
    subtype = planMAMsgSubType(msg);
    agent_id = planMAMsgAgent(msg);

    if (type == PLAN_MA_MSG_HEUR
            && subtype == PLAN_MA_MSG_HEUR_POT_RESPONSE){
        // Slave agent receives the heuristic value for the initial state
        // and from now on it can compute the heuristics by itself.
        // The potentials were received in init-heur request.
        h->init_heur = planMAMsgPotInitHeur(msg);
        res->heur = ceil(h->init_heur - EPS);
        h->ready = 1;
        return 0;

    }else if (type == PLAN_MA_MSG_HEUR
                && subtype == PLAN_MA_MSG_HEUR_POT_INFO_RESPONSE){
        // Slave agent reponded with basic info about itself.
        processInfo(h, msg);
        --h->pending;
        if (h->pending == 0){
            processAllInfo(h);
            requestLP(h, comm);
            h->pending = comm->node_size - 1;
        }

    }else if (type == PLAN_MA_MSG_HEUR
                && subtype == PLAN_MA_MSG_HEUR_POT_LP_RESPONSE){
        // A slave agent responded with its part of the matrices needed for
        // computation of potentials.
        planMAMsgPotAgent(msg, &h->agent_data[agent_id].pot);
        --h->pending;
        if (h->pending == 0){
            computePot(h);
            requestInitHeur(h, comm);
            h->pending = comm->node_size - 1;
        }

    }else if (type == PLAN_MA_MSG_HEUR
                && subtype == PLAN_MA_MSG_HEUR_POT_INIT_HEUR_RESPONSE){
        // A slave agent responded with its private part of the heuristic
        // value of the initial state.
        h->init_heur += planMAMsgPotInitHeur(msg);
        --h->pending;
        if (h->pending == 0){
            h->ready = 1;
            sendInitHeurToPending(h, comm);
            res->heur = h->init_heur;
            if (h->flags & PLAN_HEUR_POT_PRINT_INIT_TIME){
                borTimerStop(&h->timer);
                fprintf(stderr, "[%d] Heur init time: %0.4lf s\n",
                        comm->node_id, borTimerElapsedInSF(&h->timer));
            }
            return 0;
        }

    }else{
        fprintf(stderr, "[%d] Unexpected update message: %x/%x\n",
                comm->node_id, type, subtype);
        fflush(stderr);
    }

    return -1;
}

static void heurRequest(plan_heur_t *heur, plan_ma_comm_t *comm,
                        const plan_ma_msg_t *msg)
{
    plan_heur_ma_pot_t *h = HEUR(heur);
    int type, subtype, agent_id;

    type = planMAMsgType(msg);
    subtype = planMAMsgSubType(msg);
    agent_id = planMAMsgAgent(msg);

    if (type == PLAN_MA_MSG_HEUR && subtype == PLAN_MA_MSG_HEUR_POT_REQUEST){
        // A slave agent requests heuristic value for the initial state.
        sendInitHeur(h, comm, agent_id);

    }else if (type == PLAN_MA_MSG_HEUR
                && subtype == PLAN_MA_MSG_HEUR_POT_INFO_REQUEST){
        // A master requested basic info about this agent.
        responseInfo(h, comm);

    }else if (type == PLAN_MA_MSG_HEUR
                && subtype == PLAN_MA_MSG_HEUR_POT_LP_REQUEST){
        // A master requested LP matrices.
        responseLP(h, comm, msg);

    }else if (type == PLAN_MA_MSG_HEUR
                && subtype == PLAN_MA_MSG_HEUR_POT_INIT_HEUR_REQUEST){
        // A master requested the private part of the heuristic value for
        // the initial state.
        responseInitHeur(h, comm, msg);

    }else{
        fprintf(stderr, "[%d] Unexpected request message: %x/%x\n",
                comm->node_id, type, subtype);
        fflush(stderr);
    }
}

static void addFactRange(plan_heur_ma_pot_t *h, int range)
{
    int i;

    for (i = 0; i < h->fact_range_size; ++i){
        if (h->fact_range[i] == range)
            return;
    }

    ++h->fact_range_size;
    h->fact_range = BOR_REALLOC_ARR(h->fact_range, int, h->fact_range_size);
    h->fact_range[h->fact_range_size - 1] = range;
}

static void requestPot(plan_ma_comm_t *comm)
{
    plan_ma_msg_t *msg;
    msg = planMAMsgNew(PLAN_MA_MSG_HEUR,
                       PLAN_MA_MSG_HEUR_POT_REQUEST,
                       comm->node_id);
    planMACommSendToNode(comm, 0, msg);
    planMAMsgDel(msg);
}

static void requestInfo(plan_ma_comm_t *comm)
{
    plan_ma_msg_t *msg;
    msg = planMAMsgNew(PLAN_MA_MSG_HEUR,
                       PLAN_MA_MSG_HEUR_POT_INFO_REQUEST,
                       comm->node_id);
    planMACommSendToAll(comm, msg);
    planMAMsgDel(msg);
}

static void responseInfo(plan_heur_ma_pot_t *h, plan_ma_comm_t *comm)
{
    plan_ma_msg_t *msg;
    int i, size;

    msg = planMAMsgNew(PLAN_MA_MSG_HEUR,
                       PLAN_MA_MSG_HEUR_POT_INFO_RESPONSE,
                       comm->node_id);

    for (i = 0; i < h->pot.var_size; ++i){
        if (i != h->pot.ma_privacy_var){
            planMAMsgAddPotFactRange(msg, h->pot.var[i].range);
        }
    }
    size = h->pot.lp_var_size - h->pot.lp_var_private;
    planMAMsgSetPotLPPrivateVarSize(msg, size);
    planMACommSendToNode(comm, 0, msg);
    planMAMsgDel(msg);
}

static void processInfo(plan_heur_ma_pot_t *h, const plan_ma_msg_t *msg)
{
    int *fact_range, fact_range_size;
    int i, agent_id;
    agent_data_t *d;

    agent_id = planMAMsgAgent(msg);
    d = h->agent_data + agent_id;

    d->lp_private_var_size = planMAMsgPotLPPrivateVarSize(msg);

    fact_range_size = planMAMsgPotFactRangeSize(msg);
    if (fact_range_size > 0){
        fact_range = BOR_ALLOC_ARR(int, fact_range_size);
        planMAMsgPotFactRange(msg, fact_range);
        for (i = 0; i < fact_range_size; ++i)
            addFactRange(h, fact_range[i]);
        BOR_FREE(fact_range);
    }
}

static int gcd(int x, int y)
{
    if (x == 0)
        return y;

    while (y != 0){
        if (x > y) {
            x = x - y;
        }else{
            y = y - x;
        }
    }

    return x;
}

static int lcm(int x, int y)
{
    return (x * y) / gcd(x, y);
}

static void processAllInfo(plan_heur_ma_pot_t *h)
{
    int i, offset;

    // Compute LCM of all ranges
    h->fact_range_lcm = 1;
    for (i = 0; i < h->pot.var_size; ++i){
        if (i != h->pot.ma_privacy_var)
            h->fact_range_lcm = lcm(h->fact_range_lcm, h->pot.var[i].range);
    }
    for (i = 0; i < h->fact_range_size; ++i)
        h->fact_range_lcm = lcm(h->fact_range_lcm, h->fact_range[i]);

    // Compute overall number of LP variables offset for each slave agent
    h->lp_var_size = h->pot.lp_var_size;
    offset = h->lp_var_size;
    for (i = 0; i < h->agent_size; ++i){
        if (i != h->agent_id){
            h->lp_var_size += h->agent_data[i].lp_private_var_size;
            h->agent_data[i].lp_var_offset = offset;
            offset += h->agent_data[i].lp_private_var_size;
        }
    }
}

static void requestLP(plan_heur_ma_pot_t *h, plan_ma_comm_t *comm)
{
    plan_ma_msg_t *msg;
    int i;

    msg = planMAMsgNew(PLAN_MA_MSG_HEUR,
                       PLAN_MA_MSG_HEUR_POT_LP_REQUEST,
                       comm->node_id);
    planMAStateSetMAMsg2(h->heur.ma_state, h->state, msg);
    planMAMsgSetPotFactRangeLCM(msg, h->fact_range_lcm);
    planMAMsgSetPotLPVarSize(msg, h->lp_var_size);

    for (i = 0; i < comm->node_size; ++i){
        if (i != comm->node_id){
            planMAMsgSetPotLPVarOffset(msg, h->agent_data[i].lp_var_offset);
            planMACommSendToNode(comm, i, msg);
        }
    }
    planMAMsgDel(msg);
}

static void responseLP(plan_heur_ma_pot_t *h, plan_ma_comm_t *comm,
                       const plan_ma_msg_t *msg)
{
    plan_pot_agent_t pot;
    PLAN_STATE_STACK(state, h->pot.var_size);
    plan_ma_msg_t *mout;
    int var_size, var_offset, fact_range;

    var_size   = planMAMsgPotLPVarSize(msg);
    var_offset = planMAMsgPotLPVarOffset(msg);
    fact_range = planMAMsgPotFactRangeLCM(msg);
    planMAStateGetFromMAMsg(h->heur.ma_state, msg, &state);
    if (h->flags & PLAN_HEUR_POT_ALL_SYNTACTIC_STATES){
        planPotAgentInit(&h->pot, var_size, var_offset, NULL, fact_range, &pot);
    }else{
        planPotAgentInit(&h->pot, var_size, var_offset, &state, -1, &pot);
    }

    if (h->pot.lp_var_private < h->pot.lp_var_size){
        secureInit(&h->secure, h->pot.lp_var_size - h->pot.lp_var_private,
                   var_size, var_offset, h->flags);
        secureEncrypt(&h->secure, &pot);
    }

    mout = planMAMsgNew(PLAN_MA_MSG_HEUR,
                        PLAN_MA_MSG_HEUR_POT_LP_RESPONSE,
                        comm->node_id);
    planMAMsgSetPotAgent(mout, &pot);
    planMACommSendToNode(comm, 0, mout);
    planMAMsgDel(mout);

    planPotAgentFree(&pot);
}

static void computePot(plan_heur_ma_pot_t *h)
{
    lp_prog_t prog;

    // Set up pot.state_coef for all-synt-states
    if (h->flags & PLAN_HEUR_POT_ALL_SYNTACTIC_STATES)
        planPotSetAllSyntacticStatesRange(&h->pot, h->fact_range_lcm);

    // Compute LP program and set h->pot.pot
    lpProgInit(&prog, h);
    lpProgSet(&prog, h);
    //lpProgPrint(&prog, stderr);
    lpProgCompute(&prog, h);
    lpProgFree(&prog);

    h->init_heur = planPotStatePot(&h->pot, h->state);
}

static void requestInitHeur(plan_heur_ma_pot_t *h, plan_ma_comm_t *comm)
{
    plan_ma_msg_t *msg;
    int i;

    msg = planMAMsgNew(PLAN_MA_MSG_HEUR,
                       PLAN_MA_MSG_HEUR_POT_INIT_HEUR_REQUEST,
                       comm->node_id);
    planMAStateSetMAMsg2(h->heur.ma_state, h->state, msg);
    planMAMsgSetPotPot(msg, h->pot.pot, h->lp_var_size);
    planMAMsgSetPotLPVarSize(msg, h->lp_var_size);

    for (i = 0; i < comm->node_size; ++i){
        if (i != comm->node_id){
            planMAMsgSetPotLPVarOffset(msg, h->agent_data[i].lp_var_offset);
            planMACommSendToNode(comm, i, msg);
        }
    }
    planMAMsgDel(msg);
}

static void responseInitHeur(plan_heur_ma_pot_t *h, plan_ma_comm_t *comm,
                             const plan_ma_msg_t *msg)
{
    PLAN_STATE_STACK(state, h->pot.var_size);
    plan_ma_msg_t *mout;
    int i, v;
    double heur;

    planMAStateGetFromMAMsg(h->heur.ma_state, msg, &state);

    if (h->pot.pot != NULL)
        BOR_FREE(h->pot.pot);
    h->pot.pot = BOR_ALLOC_ARR(double, planMAMsgPotPotSize(msg));
    planMAMsgPotPot(msg, h->pot.pot);
    if (h->secure.m != NULL)
        secureDecrypt(&h->secure, h->pot.pot, h->pot.lp_var_private);

    heur = 0;
    for (i = 0; i < h->pot.var_size; ++i){
        if (i == h->pot.ma_privacy_var || !h->pot.var[i].is_private)
            continue;
        v = planStateGet(&state, i);
        heur += h->pot.pot[h->pot.var[i].lp_var_id[v]];
    }

    mout = planMAMsgNew(PLAN_MA_MSG_HEUR,
                        PLAN_MA_MSG_HEUR_POT_INIT_HEUR_RESPONSE,
                        comm->node_id);
    planMAMsgSetPotInitHeur(mout, heur);
    planMACommSendToNode(comm, 0, mout);
    planMAMsgDel(mout);
}

static void sendInitHeur(plan_heur_ma_pot_t *h, plan_ma_comm_t *comm,
                            int agent_id)
{
    plan_ma_msg_t *msg;

    if (!h->ready){
        // Potentials are not computed yet -- set agent as pending
        h->agent_data[agent_id].pot_requested = 1;
        return;
    }

    msg = planMAMsgNew(PLAN_MA_MSG_HEUR,
                       PLAN_MA_MSG_HEUR_POT_RESPONSE,
                       comm->node_id);
    planMAMsgSetPotInitHeur(msg, h->init_heur);
    planMACommSendToNode(comm, agent_id, msg);
    planMAMsgDel(msg);
}

static void sendInitHeurToPending(plan_heur_ma_pot_t *h, plan_ma_comm_t *comm)
{
    int i;

    for (i = 0; i < h->agent_size; ++i){
        if (i != h->agent_id && h->agent_data[i].pot_requested){
            sendInitHeur(h, comm, i);
            h->agent_data[i].pot_requested = 0;
        }
    }
}

static int heurHeur(const plan_heur_ma_pot_t *h,
                    plan_state_id_t state_id,
                    const plan_search_t *search)
{
    plan_state_space_t *state_space = (plan_state_space_t *)search->state_space;
    const plan_state_space_node_t *node, *parent_node;
    double local_heur, parent_heur, parent_stored_heur;
    int heur;

    node = planStateSpaceNode(state_space, state_id);
    parent_node = planStateSpaceNode(state_space, node->parent_state_id);
    planStatePoolGetState(search->state_pool, node->parent_state_id, h->state2);

    local_heur = planPotStatePot(&h->pot, h->state);
    parent_heur = planPotStatePot(&h->pot, h->state2);
    parent_stored_heur = parent_node->heuristic;
    heur = local_heur + (parent_stored_heur - parent_heur);
    return heur;
}

/*
static void lpProgPrint(const lp_prog_t *prog, FILE *fout)
{
    int i, j;

    fprintf(fout, "LP PROG:\n");
    fprintf(fout, "cols: %d\n", prog->cols);
    fprintf(fout, "A-rows: %d\n", prog->A_rows);

    fprintf(fout, "A:\n");
    for (i = 0; i < prog->A_rows; ++i){
        for (j = 0; j < prog->cols; ++j){
            fprintf(fout, " %d", prog->A[i * prog->cols + j]);
        }
        fprintf(fout, " | %d\n", prog->rhs[i]);
    }

    fprintf(fout, "obj:\n");
    for (i = 0; i < prog->cols; ++i)
        fprintf(fout, " %d", prog->obj[i]);
    fprintf(fout, "\n");
}
*/

static void lpProgInit(lp_prog_t *prog, const plan_heur_ma_pot_t *h)
{
    int i;

    bzero(prog, sizeof(*prog));

    prog->cols = h->lp_var_size;

    // Number of rows:
    // One for goal constraints
    prog->A_rows = 1;

    // Constraints for operators
    prog->A_rows += h->pot.prob.op_size;
    for (i = 0; i < h->agent_size; ++i){
        if (i != h->agent_id)
            prog->A_rows += h->agent_data[i].pot.pub_op.rows;
    }

    // And another constraints for maxpots
    prog->A_rows += h->pot.prob.maxpot_size;
    for (i = 0; i < h->agent_size; ++i){
        if (i != h->agent_id)
            prog->A_rows += h->agent_data[i].pot.maxpot.rows;
    }

    // Allocate constraint matrix and RHS
    prog->A = BOR_CALLOC_ARR(int, prog->cols * prog->A_rows);
    prog->rhs = BOR_CALLOC_ARR(int, prog->A_rows);
    // and array for coeficients
    prog->obj = BOR_CALLOC_ARR(int, prog->cols);
}

static void lpProgFree(lp_prog_t *prog)
{
    if (prog->A != NULL)
        BOR_FREE(prog->A);
    if (prog->rhs != NULL)
        BOR_FREE(prog->rhs);
    if (prog->obj != NULL)
        BOR_FREE(prog->obj);
}

static void lpProgSetConstr(lp_prog_t *prog, int row,
                            const plan_pot_constr_t *constr)
{
    int i, cell;

    cell = prog->cols * row;
    for (i = 0; i < constr->coef_size; ++i)
        prog->A[cell + constr->var_id[i]] = constr->coef[i];
    prog->rhs[row] = constr->rhs;
}

static void lpProgSetConstrs(lp_prog_t *prog, int row_offset,
                             const plan_pot_constr_t *constr, int constr_size)
{
    int i;

    for (i = 0; i < constr_size; ++i)
        lpProgSetConstr(prog, row_offset + i, constr + i);
}

static void lpProgAddSubmatrix(lp_prog_t *prog, int row_offset,
                               const plan_pot_submatrix_t *sub)
{
    int i, j, row, cell, sub_cell;

    for (i = 0; i < sub->rows; ++i){
        row = row_offset + i;
        cell = row * prog->cols;

        sub_cell = i * sub->cols;
        for (j = 0; j < sub->cols; ++j)
            prog->A[cell + j] += sub->coef[sub_cell + j];
    }
}

static void lpProgSetGoal(lp_prog_t *prog, const plan_heur_ma_pot_t *h)
{
    const int *goal;
    int i, j;

    lpProgSetConstr(prog, 0, &h->pot.prob.goal);
    for (i = 0; i < h->agent_size; ++i){
        if (i == h->agent_id)
            continue;

        goal = h->agent_data[i].pot.goal;
        if (goal == NULL)
            continue;

        for (j = 0; j < prog->cols; ++j)
            prog->A[j] += goal[j];
    }
}

static int lpProgSetOps(lp_prog_t *prog, const plan_heur_ma_pot_t *h)
{
    int i, j, offset;

    lpProgSetConstrs(prog, 1, h->pot.prob.op, h->pot.prob.op_size);
    offset = 1 + h->pot.prob.op_size;

    for (i = 0; i < h->agent_size; ++i){
        if (i == h->agent_id)
            continue;

        lpProgAddSubmatrix(prog, offset, &h->agent_data[i].pot.pub_op);
        lpProgAddSubmatrix(prog, offset, &h->agent_data[i].pot.priv_op);

        for (j = 0; j < h->agent_data[i].pot.pub_op.rows; ++j)
            prog->rhs[j + offset] = h->agent_data[i].pot.op_cost[j];

        offset += h->agent_data[i].pot.pub_op.rows;
    }

    return offset;
}

static void lpProgSetMaxpot(lp_prog_t *prog, int offset,
                            const plan_heur_ma_pot_t *h)
{
    int i, off;

    lpProgSetConstrs(prog, offset, h->pot.prob.maxpot, h->pot.prob.maxpot_size);

    off = offset + h->pot.prob.maxpot_size;
    for (i = 0; i < h->agent_size; ++i){
        if (i == h->agent_id)
            continue;

        lpProgAddSubmatrix(prog, off, &h->agent_data[i].pot.maxpot);
        off += h->agent_data[i].pot.maxpot.rows;
    }
}

static void lpProgSetObj(lp_prog_t *prog, const plan_heur_ma_pot_t *h)
{
    const int *coef;
    int i, j;

    memcpy(prog->obj, h->pot.prob.state_coef,
           sizeof(int) * h->pot.lp_var_size);

    for (i = 0; i < h->agent_size; ++i){
        if (i == h->agent_id)
            continue;

        coef = h->agent_data[i].pot.coef;
        if (coef == NULL)
            continue;

        for (j = 0; j < prog->cols; ++j)
            prog->obj[j] += coef[j];
    }
}

static void lpProgSet(lp_prog_t *prog, const plan_heur_ma_pot_t *h)
{
    int offset;

    lpProgSetGoal(prog, h);
    offset = lpProgSetOps(prog, h);
    lpProgSetMaxpot(prog, offset, h);
    lpProgSetObj(prog, h);
}

static void lpProgCompute(const lp_prog_t *prog, plan_heur_ma_pot_t *h)
{
    plan_lp_t *lp;
    unsigned lp_flags = 0u;
    int i, j, r;

    lp_flags |= h->pot.prob.lp_flags;
    lp_flags |= PLAN_LP_MAX;

    lp = planLPNew(prog->A_rows, prog->cols, lp_flags);

    for (i = 0; i < prog->cols; ++i)
        planLPSetVarRange(lp, i, -1E30, 1E7);

    for (i = 0; i < prog->A_rows; ++i){
        r = i * prog->cols;
        for (j = 0; j < prog->cols; ++j, ++r){
            if (prog->A[r] != 0)
                planLPSetCoef(lp, i, j, prog->A[r]);
        }

        planLPSetRHS(lp, i, prog->rhs[i], 'L');
    }

    for (i = 0; i < prog->cols; ++i)
        planLPSetObj(lp, i, prog->obj[i]);

    h->pot.pot = BOR_CALLOC_ARR(double, prog->cols);
    if (planLPSolve(lp, NULL, h->pot.pot) != 0){
        fprintf(stderr, "Error: LP has no solution!\n");
        exit(-1);
    }
    planLPDel(lp);
}



static void secureAddRow(matrix_t *m, int dst_row, int add_row, int c)
{
    int i, src, dst;

    src = m->cols * add_row;
    dst = m->cols * dst_row;
    for (i = 0; i < m->cols; ++i)
        m->m[dst++] += m->m[src++] * c;
}

static int secureRandInt(bor_rand_t *rnd, int range)
{
    int v;

    do {
        v = borRand(rnd, 0, range);
    } while (v < 0 || v >= range);
    return v;
}

static void secureInit(matrix_t *m, int num_private_vars, int num_vars,
                       int private_offset, unsigned flags)
{
    bor_rand_t rnd;
    int i, ins, rounds, src, dst, c;

    if (num_private_vars == 0)
        return;

    m->cols = num_private_vars;
    m->rows = num_vars;
    m->offset = private_offset;
    m->m = BOR_CALLOC_ARR(int, m->cols * m->rows);
    m->unit = 0;

    if (flags & PLAN_HEUR_POT_ENCRYPTION_OFF){
        m->unit = 1;
        return;
    }

    // Create part of the unit matrix
    ins = private_offset * m->cols;
    for (i = 0; i < num_private_vars; ++i){
        m->m[ins] = 1;
        ins += m->cols + 1;
    }

    // And sum random rows so we preserve rank of the matrix
    borRandInit(&rnd);
    for (rounds = 0; rounds < num_private_vars; ++rounds){
        src = secureRandInt(&rnd, num_private_vars);
        dst = secureRandInt(&rnd, num_private_vars);
        if (src == dst){
            --rounds;
            continue;
        }

        c = secureRandInt(&rnd, 2);
        if (c == 0)
            c = -1;
        secureAddRow(m, dst + private_offset, src + private_offset, c);
    }
}

static void secureEncryptSubmatrix(const matrix_t *m,
                                   plan_pot_submatrix_t *sub)
{
    int *src, row, col, k, ins, sum;

    if (sub->cols * sub->rows == 0)
        return;

    if (m->unit)
        return;

    // Copy private part
    src = BOR_ALLOC_ARR(int, m->cols * sub->rows);
    ins = 0;
    for (row = 0; row < sub->rows; ++row){
        for (col = 0; col < m->cols; ++col)
            src[ins++] = sub->coef[row * sub->cols + col + m->offset];
    }

    for (row = 0; row < sub->rows; ++row){
        for (col = 0; col < sub->cols; ++col){
            sum = 0;
            for (k = 0; k < m->cols; ++k)
                sum += m->m[col * m->cols + k] * src[row * m->cols + k];
            sub->coef[row * sub->cols + col] = sum;
        }
    }

    BOR_FREE(src);
}

static void secureEncryptArr(const matrix_t *m, int *arr)
{
    int *src, i, j, sum;

    if (m->unit)
        return;

    src = BOR_ALLOC_ARR(int, m->cols);
    for (i = 0; i < m->cols; ++i)
        src[i] = arr[m->offset + i];

    for (i = 0; i < m->rows; ++i){
        sum = 0;
        for (j = 0; j < m->cols; ++j)
            sum += src[j] * m->m[i * m->cols + j];
        arr[i] = sum;
    }
    BOR_FREE(src);
}

static void secureEncrypt(const matrix_t *m, plan_pot_agent_t *pot)
{
    if (m->unit)
        return;
    secureEncryptSubmatrix(m, &pot->priv_op);
    secureEncryptSubmatrix(m, &pot->maxpot);
    secureEncryptArr(m, pot->goal);
    secureEncryptArr(m, pot->coef);
}

static void secureDecrypt(const matrix_t *m, double *pot, int pub_var_size)
{
    int i, j;
    double sum, *dst;

    if (m->unit){
        for (i = 0; i < m->cols; ++i)
            pot[i + pub_var_size] = pot[i + m->offset];
        return;
    }

    dst = BOR_ALLOC_ARR(double, m->cols);
    for (i = 0; i < m->cols; ++i){
        sum = 0.;
        for (j = 0; j < m->rows; ++j)
            sum += pot[j] * m->m[j * m->cols + i];
        dst[i] = sum;
    }

    for (i = 0; i < m->cols; ++i)
        pot[i + pub_var_size] = dst[i];

    BOR_FREE(dst);
}

#else /* PLAN_LP */

plan_heur_t *planHeurMAPotNew(const plan_problem_t *p, unsigned flags)
{
    fprintf(stderr, "Error: Cannot create Potential heuristic because no"
                    " LP-solver was available during compilation!\n");
    fflush(stderr);
    return NULL;
}

#endif /* PLAN_LP */
