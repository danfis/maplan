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

#ifndef _GNU_SOURCE
# define _GNU_SOURCE
#endif /* _GNU_SOURCE */

#include <boruvka/alloc.h>
#include <boruvka/hfunc.h>
#include <boruvka/scc.h>
#include "plan/pddl_ground.h"
#include "plan/ma_terminate.h"
#include "pddl_err.h"

/** Minimal number of a new facts to use full instAction() */
#define MIN_NEW_FACTS_FULL_INST_ACTION 100

#define STATE_START                     0
#define STATE_WAIT_FOR_NAMES            1
#define STATE_WAIT_FOR_NAME_MAP         2
#define STATE_GROUND                    3
#define STATE_WAIT_FOR_GROUNDED_FACTS   4
#define STATE_GROUND_END                1000

/** Instantiates actions using facts in fact pool. */
static void instActions(const plan_pddl_lift_actions_t *actions,
                        plan_pddl_fact_pool_t *fact_pool,
                        plan_pddl_ground_action_pool_t *action_pool,
                        const plan_pddl_t *pddl);
/** Instantiates negative preconditions of actions and adds them to fact
 *  pool. */
static void instActionsNegativePreconditions(const plan_pddl_t *pddl,
                                             const plan_pddl_lift_actions_t *as,
                                             plan_pddl_fact_pool_t *fact_pool);
/** Finds out how many agents there are and creates all mappings between
 *  agents and objects */
static void determineAgents(plan_pddl_ground_t *g, const plan_pddl_t *pddl);

/** Copy fact IDs from src to dst that are present in fact_pool. */
static void setReachableFactIds(plan_pddl_ground_facts_t *dst,
                                plan_pddl_fact_pool_t *fact_pool,
                                const plan_pddl_facts_t *src);

/** Flip flag of the facts that are static facts.
 *  Static facts are those that are in the init state and are never changed
 *  to negative form. */
static void markStaticFacts(plan_pddl_fact_pool_t *fact_pool,
                            const plan_pddl_facts_t *init_facts)
{
    plan_pddl_fact_t fact, *init_fact, *f;
    int i, id;

    for (i = 0; i < init_facts->size; ++i){
        init_fact = init_facts->fact + i;
        if (init_fact->neg)
            continue;

        planPDDLFactCopy(&fact, init_fact);
        fact.neg = 1;
        if (!planPDDLFactPoolExist(fact_pool, &fact)){
            fact.neg = 0;
            id = planPDDLFactPoolFind(fact_pool, &fact);
            f = planPDDLFactPoolGet(fact_pool, id);
            f->stat = 1;
        }
        planPDDLFactFree(&fact);
    }
}

static void removeStatAndNegFacts(plan_pddl_fact_pool_t *fact_pool,
                                  plan_pddl_ground_action_pool_t *action_pool)
{
    int *map;

    map = BOR_CALLOC_ARR(int, fact_pool->size);
    planPDDLFactPoolCleanup(fact_pool, map);
    planPDDLGroundActionPoolRemap(action_pool, map);
    BOR_FREE(map);
}

void planPDDLGroundInit(plan_pddl_ground_t *g, const plan_pddl_t *pddl)
{
    bzero(g, sizeof(*g));
    planPDDLFactPoolInit(&g->fact_pool, pddl->predicate.size);
    planPDDLGroundActionPoolInit(&g->action_pool, &pddl->predicate, &pddl->obj);

    g->agent_size = 0;
    g->agent_to_obj = NULL;
    g->obj_to_agent = NULL;

    g->glob_to_obj = g->obj_to_glob = NULL;
    g->glob_to_pred = g->pred_to_glob = NULL;
}

void planPDDLGroundFree(plan_pddl_ground_t *g)
{
    planPDDLFactPoolFree(&g->fact_pool);
    planPDDLGroundActionPoolFree(&g->action_pool);
    planPDDLLiftActionsFree(&g->lift_action);

    if (g->goal.fact != NULL)
        BOR_FREE(g->goal.fact);
    if (g->init.fact != NULL)
        BOR_FREE(g->init.fact);

    if (g->agent_to_obj != NULL)
        BOR_FREE(g->agent_to_obj);
    if (g->obj_to_agent != NULL)
        BOR_FREE(g->obj_to_agent);

    if (g->glob_to_obj != NULL)
        BOR_FREE(g->glob_to_obj);
    if (g->obj_to_glob != NULL)
        BOR_FREE(g->obj_to_glob);
    if (g->glob_to_pred != NULL)
        BOR_FREE(g->glob_to_pred);
    if (g->pred_to_glob != NULL)
        BOR_FREE(g->pred_to_glob);
}

void planPDDLGround(plan_pddl_ground_t *g, const plan_pddl_t *pddl)
{
    planPDDLGroundInit(g, pddl);
    planPDDLLiftActionsInit(&g->lift_action, &pddl->action,
                            &pddl->type_obj, &pddl->init_func,
                            pddl->obj.size, pddl->predicate.eq_pred);

    planPDDLFactPoolAddFacts(&g->fact_pool, &pddl->init_fact);
    instActionsNegativePreconditions(pddl, &g->lift_action, &g->fact_pool);
    instActions(&g->lift_action, &g->fact_pool, &g->action_pool, pddl);
    markStaticFacts(&g->fact_pool, &pddl->init_fact);
    planPDDLGroundActionPoolInst(&g->action_pool, &g->fact_pool);
    removeStatAndNegFacts(&g->fact_pool, &g->action_pool);
    determineAgents(g, pddl);

    setReachableFactIds(&g->init, &g->fact_pool, &pddl->init_fact);
    setReachableFactIds(&g->goal, &g->fact_pool, &pddl->goal);
}

static void setReachableFactIds(plan_pddl_ground_facts_t *dst,
                                plan_pddl_fact_pool_t *fact_pool,
                                const plan_pddl_facts_t *src)
{
    int i, fact_id;

    dst->size = 0;
    dst->fact = BOR_ALLOC_ARR(int, src->size);
    for (i = 0; i < src->size; ++i){
        fact_id = planPDDLFactPoolFind(fact_pool, src->fact + i);
        if (fact_id >= 0){
            dst->fact[dst->size++] = fact_id;
        }
    }

    if (dst->size != src->size){
        dst->fact = BOR_REALLOC_ARR(dst->fact, int, dst->size);
    }
}

static int cmpStr(const void *a, const void *b)
{
    const char *s1 = *(const char **)a;
    const char *s2 = *(const char **)b;
    return strcmp(s1, s2);
}

static int factorSortUniqStrArr(char **arr, int arr_size)
{
    int i, ins;

    // Sort and uniq the array
    qsort(arr, arr_size, sizeof(char *), cmpStr);
    for (i = 1, ins = 0; i < arr_size; ++i){
        if (strcmp(arr[i - 1], arr[i]) != 0)
            arr[ins++] = arr[i - 1];
    }
    arr[ins++] = arr[arr_size - 1];
    return ins;
}

static void factorSendObjPredNamesRequest(plan_ma_comm_t *comm)
{
    plan_ma_msg_t *msg;

    msg = planMAMsgNew(PLAN_MA_MSG_PDDL_GROUND,
                       PLAN_MA_MSG_PDDL_GROUND_OBJ_PRED_NAMES_REQUEST,
                       comm->node_id);
    planMACommSendToAll(comm, msg);
    planMAMsgDel(msg);
}

static void factorSendObjPredNamesResponse(const plan_pddl_ground_t *g,
                                           const plan_pddl_t *pddl,
                                           plan_ma_comm_t *comm)
{
    const plan_pddl_predicates_t *preds;
    const plan_pddl_objs_t *objs;
    plan_ma_msg_t *msg;
    int i;

    msg = planMAMsgNew(PLAN_MA_MSG_PDDL_GROUND,
                       PLAN_MA_MSG_PDDL_GROUND_OBJ_PRED_NAMES_RESPONSE,
                       comm->node_id);

    preds = &pddl->predicate;
    for (i = 0; i < preds->size; ++i){
        if (!preds->pred[i].is_private)
            planMAMsgAddPDDLGroundPredName(msg, preds->pred[i].name);
    }

    objs = &pddl->obj;
    for (i = 0; i < objs->size; ++i){
        if (!objs->obj[i].is_private)
            planMAMsgAddPDDLGroundObjName(msg, objs->obj[i].name);
    }

    planMACommSendToNode(comm, 0, msg);
    planMAMsgDel(msg);
}

static void factorConstructMap(plan_pddl_ground_t *g, const plan_pddl_t *pddl,
                               char **name, int name_size, int is_obj,
                               int **to_glob, int **glob_to)
{
    const char *loc_name, *glob_name;
    int i, loc_id, glob_id, loc_size;

    if (is_obj){
        loc_size = pddl->obj.size;
    }else{
        loc_size = pddl->predicate.size;
    }

    // Create and initialize mapping from global ID to local ID
    *glob_to = BOR_ALLOC_ARR(int, name_size);
    for (i = 0; i < name_size; ++i)
        (*glob_to)[i] = -1;

    // Create and initialize mapping from local ID to global ID
    *to_glob = BOR_ALLOC_ARR(int, loc_size);
    for (i = 0; i < loc_size; ++i)
        (*to_glob)[i] = -1;

    // Fill mapping
    for (loc_id = 0; loc_id < loc_size; ++loc_id){
        if (is_obj){
            loc_name = pddl->obj.obj[loc_id].name;
        }else{
            loc_name = pddl->predicate.pred[loc_id].name;
        }

        for (glob_id = 0; glob_id < name_size; ++glob_id){
            glob_name = name[glob_id];
            if (strcmp(loc_name, glob_name) == 0){
                (*to_glob)[loc_id] = glob_id;
                (*glob_to)[glob_id] = loc_id;
            }
        }
    }
}

static void factorConstructObjPredMap(plan_pddl_ground_t *g,
                                      const plan_pddl_t *pddl,
                                      char **obj, int obj_size,
                                      char **pred, int pred_size,
                                      plan_ma_comm_t *comm)
{
    plan_ma_msg_t *msg;
    int i;

    // Sort and uniq global names
    obj_size = factorSortUniqStrArr(obj, obj_size);
    pred_size = factorSortUniqStrArr(pred, pred_size);

    // Construct maps for both predicates and objects
    factorConstructMap(g, pddl, obj, obj_size, 1,
                       &g->obj_to_glob, &g->glob_to_obj);
    factorConstructMap(g, pddl, pred, pred_size, 0,
                       &g->pred_to_glob, &g->glob_to_pred);


    if (comm == NULL)
        return;

    // Send maps to other agents
    msg = planMAMsgNew(PLAN_MA_MSG_PDDL_GROUND,
                       PLAN_MA_MSG_PDDL_GROUND_OBJ_PRED_MAP,
                       comm->node_id);
    for (i = 0; i < obj_size; ++i)
        planMAMsgAddPDDLGroundObjName(msg, obj[i]);
    for (i = 0; i < pred_size; ++i)
        planMAMsgAddPDDLGroundPredName(msg, pred[i]);
    planMACommSendToAll(comm, msg);
    planMAMsgDel(msg);
}

static void factorObjPredMapFromMsg(plan_pddl_ground_t *g,
                                    const plan_pddl_t *pddl,
                                    const plan_ma_msg_t *msg)
{
    char **obj = NULL, **pred = NULL;
    int obj_size = 0, pred_size = 0;

    planMAMsgPDDLGroundObjName(msg, &obj, &obj_size);
    planMAMsgPDDLGroundPredName(msg, &pred, &pred_size);
    factorConstructObjPredMap(g, pddl, obj, obj_size, pred, pred_size, NULL);

    if (obj)
        BOR_FREE(obj);
    if (pred)
        BOR_FREE(pred);
}

static void factorSendObjPredMapAck(plan_ma_comm_t *comm)
{
    plan_ma_msg_t *msg;
    msg = planMAMsgNew(PLAN_MA_MSG_PDDL_GROUND,
                       PLAN_MA_MSG_PDDL_GROUND_OBJ_PRED_MAP_ACK,
                       comm->node_id);
    planMACommSendToNode(comm, 0, msg);
    planMAMsgDel(msg);
}

static void factorSendGroundFacts(plan_pddl_ground_t *g,
                                  plan_ma_comm_t *comm)
{
    plan_ma_msg_t *msg;
    const plan_pddl_fact_t *fact;
    int i, size;

    msg = planMAMsgNew(PLAN_MA_MSG_PDDL_GROUND,
                       PLAN_MA_MSG_PDDL_GROUND_FACTS,
                       comm->node_id);

    size = g->fact_pool.size;
    for (i = 0; i < size; ++i){
        fact = planPDDLFactPoolGet(&g->fact_pool, i);
        if (fact->is_private)
            continue;

        planMAMsgAddPDDLGroundFact(msg, fact, g->obj_to_glob,
                                   g->pred_to_glob);
    }

    planMACommSendInRing(comm, msg);
    planMAMsgDel(msg);
}

static void factorRecvGroundFacts(plan_pddl_ground_t *g,
                                  const plan_ma_msg_t *msg)
{
    plan_pddl_fact_t fact;
    int i, size;

    size = planMAMsgPDDLGroundFactSize(msg);
    for (i = 0; i < size; ++i){
        planMAMsgPDDLGroundFact(msg, i, g->glob_to_obj, g->glob_to_pred, &fact);
        planPDDLFactPoolAdd(&g->fact_pool, &fact);
        planPDDLFactFree(&fact);
    }
}

static void factorMaster(plan_pddl_ground_t *g, const plan_pddl_t *pddl,
                         plan_ma_comm_t *comm)
{
    plan_ma_terminate_t terminate;
    plan_ma_msg_t *msg_names[comm->node_size];
    char **pred, **obj;
    plan_ma_msg_t *msg;
    int i, type, subtype, pred_size, obj_size, num_facts;
    int wait;

    pred = obj = NULL;
    pred_size = obj_size = 0;
    planMATerminateInit(&terminate, NULL, NULL, NULL);
    bzero(msg_names, sizeof(plan_ma_msg_t *) * comm->node_size);

    // First send request for names of all public objects and predicates
    factorSendObjPredNamesRequest(comm);
    wait = comm->node_size - 1;

    while ((msg = planMACommRecvBlock(comm, -1)) != NULL){
        type = planMAMsgType(msg);
        subtype = planMAMsgSubType(msg);

        if (type == PLAN_MA_MSG_TERMINATE){
            if (planMATerminateProcessMsg(&terminate, msg, comm) == 1){
                planMAMsgDel(msg);
                break;
            }

            planMAMsgDel(msg);
            continue;

        }else if (type != PLAN_MA_MSG_PDDL_GROUND){
            ERR_F(comm, "Expecting pddl-ground message, received something"
                  " else. (type: %d, subtype: %d)", type, subtype);
        }

        if (subtype == PLAN_MA_MSG_PDDL_GROUND_OBJ_PRED_NAMES_RESPONSE){
            // Save object and predicate names
            planMAMsgPDDLGroundPredName(msg, &pred, &pred_size);
            planMAMsgPDDLGroundObjName(msg, &obj, &obj_size);
            msg_names[planMAMsgAgent(msg)] = msg;
            msg = NULL;

            if (--wait == 0){
                // We have received all object and predicate names.
                // Construct maps and send them to othe agents.
                factorConstructObjPredMap(g, pddl, obj, obj_size,
                                          pred, pred_size, comm);
                wait = comm->node_size - 1;
            }

        }else if (subtype == PLAN_MA_MSG_PDDL_GROUND_OBJ_PRED_MAP_ACK){
            if (--wait == 0){
                instActions(&g->lift_action, &g->fact_pool, &g->action_pool, pddl);
                factorSendGroundFacts(g, comm);
            }

        }else if (subtype == PLAN_MA_MSG_PDDL_GROUND_FACTS){
            num_facts = g->fact_pool.size;
            factorRecvGroundFacts(g, msg);
            instActions(&g->lift_action, &g->fact_pool, &g->action_pool, pddl);
            factorSendGroundFacts(g, comm);

            if (num_facts == g->fact_pool.size)
                planMATerminateStart(&terminate, comm);

        }else{
            ERR_F(comm, "Expecting pddl-ground message, received something"
                  " else. (type: %d, subtype: %d)", type, subtype);
        }

        if (msg != NULL)
            planMAMsgDel(msg);
    }

    for (i = 0; i < comm->node_size; ++i){
        if (msg_names[i] != NULL)
            planMAMsgDel(msg_names[i]);
    }

    if (obj != NULL)
        BOR_FREE(obj);
    if (pred != NULL)
        BOR_FREE(pred);
    planMATerminateFree(&terminate);
}

static void factorSlave(plan_pddl_ground_t *g, const plan_pddl_t *pddl,
                        plan_ma_comm_t *comm)
{
    plan_ma_terminate_t terminate;
    plan_ma_msg_t *msg;
    int type, subtype;

    planMATerminateInit(&terminate, NULL, NULL, NULL);

    while ((msg = planMACommRecvBlock(comm, -1)) != NULL){
        type = planMAMsgType(msg);
        subtype = planMAMsgSubType(msg);

        if (type == PLAN_MA_MSG_TERMINATE){
            if (planMATerminateProcessMsg(&terminate, msg, comm) == 1){
                planMAMsgDel(msg);
                break;
            }

            planMAMsgDel(msg);
            continue;

        }else if (type != PLAN_MA_MSG_PDDL_GROUND){
            ERR_F(comm, "Expecting pddl-ground message, received something"
                  " else. (type: %d, subtype: %d)", type, subtype);
        }

        if (subtype == PLAN_MA_MSG_PDDL_GROUND_OBJ_PRED_NAMES_REQUEST){
            factorSendObjPredNamesResponse(g, pddl, comm);

        }else if (subtype == PLAN_MA_MSG_PDDL_GROUND_OBJ_PRED_MAP){
            factorObjPredMapFromMsg(g, pddl, msg);
            factorSendObjPredMapAck(comm);

        }else if (subtype == PLAN_MA_MSG_PDDL_GROUND_FACTS){
            factorRecvGroundFacts(g, msg);
            instActions(&g->lift_action, &g->fact_pool, &g->action_pool, pddl);
            factorSendGroundFacts(g, comm);

        }else{
            ERR_F(comm, "Expecting pddl-ground message, received something"
                  " else. (type: %d, subtype: %d)", type, subtype);
        }

        planMAMsgDel(msg);
    }

    planMATerminateFree(&terminate);
}

int planPDDLGroundFactor(plan_pddl_ground_t *g, const plan_pddl_t *pddl,
                         plan_ma_comm_t *comm)
{
    planPDDLGroundInit(g, pddl);
    planPDDLLiftActionsInit(&g->lift_action, &pddl->action,
                            &pddl->type_obj, &pddl->init_func,
                            pddl->obj.size, pddl->predicate.eq_pred);
    planPDDLFactPoolAddFacts(&g->fact_pool, &pddl->init_fact);
    instActionsNegativePreconditions(pddl, &g->lift_action, &g->fact_pool);

    if (comm->node_id == 0){
        factorMaster(g, pddl, comm);
    }else{
        factorSlave(g, pddl, comm);
    }

    markStaticFacts(&g->fact_pool, &pddl->init_fact);
    planPDDLGroundActionPoolInst(&g->action_pool, &g->fact_pool);
    removeStatAndNegFacts(&g->fact_pool, &g->action_pool);
    g->agent_size = comm->node_size;
    return 0;
}

static int printCmpActions(const void *a, const void *b, void *ud)
{
    int id1 = *(int *)a;
    int id2 = *(int *)b;
    const plan_pddl_ground_action_pool_t *ap = ud;
    const plan_pddl_ground_action_t *a1 = planPDDLGroundActionPoolGet(ap, id1);
    const plan_pddl_ground_action_t *a2 = planPDDLGroundActionPoolGet(ap, id2);
    return strcmp(a1->name, a2->name);
}

void planPDDLGroundPrint(const plan_pddl_ground_t *g,
                         const plan_pddl_t *pddl, FILE *fout)
{
    const plan_pddl_fact_t *fact;
    const plan_pddl_ground_action_t *action;
    int *action_ids;
    int i;

    action_ids = BOR_ALLOC_ARR(int, g->action_pool.size);
    for (i = 0; i < g->action_pool.size; ++i)
        action_ids[i] = i;

    qsort_r(action_ids, g->action_pool.size, sizeof(int), printCmpActions,
            (void *)&g->action_pool);

    fprintf(fout, "Facts[%d]:\n", g->fact_pool.size);
    for (i = 0; i < g->fact_pool.size; ++i){
        fact = planPDDLFactPoolGet(&g->fact_pool, i);
        fprintf(fout, "    ");
        planPDDLFactPrint(&pddl->predicate, &pddl->obj, fact, fout);
        fprintf(fout, "\n");
    }

    fprintf(fout, "Actions[%d]:\n", g->action_pool.size);
    for (i = 0; i < g->action_pool.size; ++i){
        action = planPDDLGroundActionPoolGet(&g->action_pool, action_ids[i]);
        planPDDLGroundActionPrint(action, &g->fact_pool, &pddl->predicate,
                                  &pddl->obj, fout);
    }

    fprintf(fout, "Agent size: %d\n", g->agent_size);
    for (i = 0; g->agent_to_obj != NULL && i < g->agent_size; ++i){
        fprintf(fout, "    [%d -> %d] (%s)\n",
                i, g->agent_to_obj[i],
                pddl->obj.obj[g->agent_to_obj[i]].name);
    }
    if (g->agent_size > 0 && g->obj_to_agent != NULL){
        fprintf(fout, "    obj-to-agent:");
        for (i = 0; i < pddl->obj.size; ++i){
            if (g->obj_to_agent[i] >= 0)
                fprintf(fout, " (%d->%d)", i, g->obj_to_agent[i]);
        }
        fprintf(fout, "\n");
    }

    BOR_FREE(action_ids);
}



/**** INSTANTIATE ****/
static void initBoundArg(int *bound_arg, const int *bound_arg_init, int size)
{
    int i;

    if (bound_arg_init == NULL){
        for (i = 0; i < size + 1; ++i)
            bound_arg[i] = -1;
    }else{
        memcpy(bound_arg, bound_arg_init, sizeof(int) * (size + 1));
    }
}

_bor_inline int boundOwner(const int *bound_arg, int arg_size)
{
    return bound_arg[arg_size];
}

_bor_inline void boundSetOwner(int *bound_arg, int arg_size, int owner)
{
    bound_arg[arg_size] = owner;
}

static int checkBoundArgEqFact(const plan_pddl_fact_t *eq,
                               int obj_size,
                               const int *bound_arg)
{
    int i, val[2];

    for (i = 0; i < 2; ++i){
        val[i] = eq->arg[i];
        if (val[i] < 0){
            val[i] = val[i] + obj_size;
        }else{
            val[i] = bound_arg[val[i]];
        }
    }

    return val[0] == val[1];
}

static int checkBoundArgEq(const plan_pddl_lift_action_t *lift_action,
                           const int *bound_arg)
{
    const plan_pddl_fact_t *eq;
    int i, size, obj_size, check;

    obj_size = lift_action->obj_size;
    size = lift_action->eq.size;
    for (i = 0; i < size; ++i){
        eq = lift_action->eq.fact + i;
        check = checkBoundArgEqFact(eq, obj_size, bound_arg);
        if (eq->neg && check){
            return 0;
        }else if (!eq->neg && !check){
            return 0;
        }
    }

    return 1;
}

static int unboundArg(const int *bound_arg, int arg_size)
{
    int arg_i;

    for (arg_i = 0; arg_i < arg_size; ++arg_i){
        if (bound_arg[arg_i] == -1)
            return arg_i;
    }
    return -1;
}

static void instActionBound(const plan_pddl_lift_action_t *lift_action,
                            plan_pddl_fact_pool_t *fact_pool,
                            plan_pddl_ground_action_pool_t *action_pool,
                            const int *bound_arg)
{
    // Check (= ...) equality predicate if it holds for the current binding
    // of arguments.
    if (!checkBoundArgEq(lift_action, bound_arg))
        return;

    // Ground action and if successful instatiate grounded
    planPDDLGroundActionPoolAdd(action_pool, lift_action, bound_arg, fact_pool);
}

static int instBoundOwner(const plan_pddl_lift_action_t *lift_action,
                          const plan_pddl_fact_t *fact,
                          int *bound_arg)
{
    int arg_size = lift_action->param_size;
    int owner, bound_owner;

    // Determine the owner object that is forced through action bound
    // argument.
    owner = -1;
    if (lift_action->owner_param >= 0
            && bound_arg[lift_action->owner_param] >= 0)
        owner = bound_arg[lift_action->owner_param];

    // Check that the owner corresponds to the owner of the binding fact
    if (fact != NULL && fact->owner >= 0){
        if (owner >= 0 && fact->owner != owner)
            return -1;
        owner = fact->owner;
    }

    // Check that the owner corresponds to the owner bound in previous
    // steps.
    bound_owner = boundOwner(bound_arg, arg_size);
    if (bound_owner >= 0){
        if (owner >= 0 && owner != bound_owner)
            return -1;
        owner = bound_owner;
    }

    // Bound the owner ID
    boundSetOwner(bound_arg, arg_size, owner);
    return 0;
}

static int instBoundArg(const plan_pddl_lift_action_t *lift_action,
                        const plan_pddl_fact_t *pre,
                        const plan_pddl_fact_t *fact,
                        const int *bound_arg_init,
                        int *bound_arg)
{
    int arg_size = lift_action->param_size;
    int objs_size = lift_action->obj_size;
    int i, var, val;

    initBoundArg(bound_arg, bound_arg_init, arg_size);
    for (i = 0; i < pre->arg_size; ++i){
        // Get variable ID (or constant ID)
        var = pre->arg[i];
        // and resolve ID to object ID
        if (var < 0){
            val = var + objs_size;
        }else{
            val = bound_arg[var];
        }

        // Check if values in the current argument equals
        if (val >= 0 && val != fact->arg[i])
            return -1;

        if (val == -1){
            if (!lift_action->allowed_arg[var][fact->arg[i]])
                return -1;
            bound_arg[var] = fact->arg[i];
        }
    }

    return instBoundOwner(lift_action, fact, bound_arg);
}

static void instActionBoundPre(const plan_pddl_lift_action_t *lift_action,
                               plan_pddl_fact_pool_t *fact_pool,
                               plan_pddl_ground_action_pool_t *action_pool,
                               const int *bound_arg_init);

static void instActionBoundFree(const plan_pddl_lift_action_t *lift_action,
                                plan_pddl_fact_pool_t *fact_pool,
                                plan_pddl_ground_action_pool_t *action_pool,
                                const int *bound_arg_init,
                                int unbound, int obj_id)
{
    int arg_size = lift_action->param_size;
    int bound_arg[arg_size + 1];

    initBoundArg(bound_arg, bound_arg_init, arg_size);

    if (!lift_action->allowed_arg[unbound][obj_id])
        return;

    // Try an allowed object ID but first check whether it causes
    // conflict in owners.
    bound_arg[unbound] = obj_id;
    if (instBoundOwner(lift_action, NULL, bound_arg) != 0)
        return;

    instActionBoundPre(lift_action, fact_pool, action_pool, bound_arg);
}

static void instActionBoundPre(const plan_pddl_lift_action_t *lift_action,
                               plan_pddl_fact_pool_t *fact_pool,
                               plan_pddl_ground_action_pool_t *action_pool,
                               const int *bound_arg_init)
{
    int arg_size = lift_action->param_size;
    int obj_size = lift_action->obj_size;
    int i, unbound;

    unbound = unboundArg(bound_arg_init, arg_size);
    if (unbound >= 0){
        // Bound by all possible objects
        for (i = 0; i < obj_size; ++i){
            instActionBoundFree(lift_action, fact_pool, action_pool,
                                bound_arg_init, unbound, i);
        }

    }else{
        instActionBound(lift_action, fact_pool, action_pool, bound_arg_init);
    }
}

static void instAction(const plan_pddl_lift_action_t *lift_action,
                       plan_pddl_fact_pool_t *fact_pool,
                       plan_pddl_ground_action_pool_t *action_pool,
                       const int *bound_arg_init,
                       int pre_i)
{
    const plan_pddl_fact_t *fact, *pre;
    int facts_size, i, bound_arg[lift_action->param_size + 1];

    pre = lift_action->pre.fact + pre_i;
    facts_size = planPDDLFactPoolPredSize(fact_pool, pre->pred);
    for (i = 0; i < facts_size; ++i){
        fact = planPDDLFactPoolGetPred(fact_pool, pre->pred, i);
        // Precondition and the fact must really match
        if (fact->neg != pre->neg)
            continue;

        // Bound action arguments using the current fact
        if (instBoundArg(lift_action, pre, fact, bound_arg_init, bound_arg) != 0)
            continue;

        if (pre_i == lift_action->pre.size - 1){
            // If this was the last precondition then try to instantiate
            // action using bounded arguments.
            instActionBoundPre(lift_action, fact_pool, action_pool, bound_arg);

        }else{
            // Recursion -- bound next precondition
            instAction(lift_action, fact_pool, action_pool,
                       bound_arg, pre_i + 1);
        }
    }
}

static void instActionFromFact(const plan_pddl_lift_action_t *lift_action,
                               plan_pddl_fact_pool_t *fact_pool,
                               plan_pddl_ground_action_pool_t *action_pool,
                               int fact_id)
{
    const plan_pddl_fact_t *fact, *pre;
    int i, pre_size = lift_action->pre.size;
    int bound_arg[lift_action->param_size + 1];

    fact = planPDDLFactPoolGet(fact_pool, fact_id);
    for (i = 0; i < pre_size; ++i){
        pre = lift_action->pre.fact + i;
        if (pre->pred != fact->pred || pre->neg != fact->neg)
            continue;

        if (instBoundArg(lift_action, pre, fact, NULL, bound_arg) != 0)
            continue;

        instAction(lift_action, fact_pool, action_pool, bound_arg, 0);
    }
}

static void instActions(const plan_pddl_lift_actions_t *actions,
                        plan_pddl_fact_pool_t *fact_pool,
                        plan_pddl_ground_action_pool_t *action_pool,
                        const plan_pddl_t *pddl)
{
    const plan_pddl_lift_action_t *lift_action;
    int i, fact_id, size, num_facts, diff;

    num_facts = 0;
    while (num_facts != fact_pool->size){
        diff = fact_pool->size - num_facts;
        fact_id = num_facts;
        num_facts = fact_pool->size;

        if (diff >= MIN_NEW_FACTS_FULL_INST_ACTION){
            for (i = 0; i < actions->size; ++i){
                lift_action = actions->action + i;
                instAction(lift_action, fact_pool, action_pool, NULL, 0);
            }

        }else{
            size = fact_pool->size;
            for (; fact_id < size; ++fact_id){
                for (i = 0; i < actions->size; ++i){
                    lift_action = actions->action + i;
                    instActionFromFact(lift_action, fact_pool, action_pool, fact_id);
                }
            }
        }
    }
}
/**** INSTANTIATE END ****/

/**** INSTANTIATE NEGATIVE PRECONDITIONS ****/
static void instActionNegPreWrite(const plan_pddl_t *pddl,
                                  const plan_pddl_lift_action_t *lift_action,
                                  const plan_pddl_fact_t *pre,
                                  int *bound_arg,
                                  plan_pddl_fact_pool_t *fact_pool)
{
    plan_pddl_fact_t f;

    // Termination of recursion -- record the grounded fact
    planPDDLFactCopy(&f, pre);
    memcpy(f.arg, bound_arg, sizeof(int) * f.arg_size);

    // Check if the fact is correct from point of privateness
    if (planPDDLFactSetPrivate(&f, &pddl->predicate, &pddl->obj) < 0){
        planPDDLFactFree(&f);
        return;
    }

    // Write only those negative facts that are not present in positive
    // form
    f.neg = 0;
    if (!planPDDLFactPoolExist(fact_pool, &f)){
        f.neg = 1;
        planPDDLFactPoolAdd(fact_pool, &f);
    }
    planPDDLFactFree(&f);
}

static void instActionNegPre(const plan_pddl_t *pddl,
                             const plan_pddl_lift_action_t *lift_action,
                             const plan_pddl_fact_t *pre,
                             int *bound_arg, int arg_i,
                             plan_pddl_fact_pool_t *fact_pool)
{
    int i, var, *allowed;

    if (arg_i == pre->arg_size){
        instActionNegPreWrite(pddl, lift_action, pre, bound_arg, fact_pool);
        return;
    }

    var = pre->arg[arg_i];
    if (var < 0){
        bound_arg[arg_i] = var + lift_action->obj_size;
        instActionNegPre(pddl, lift_action, pre, bound_arg,
                         arg_i + 1, fact_pool);

    }else{
        allowed = lift_action->allowed_arg[var];
        for (i = 0; i < lift_action->obj_size; ++i){
            if (!allowed[i])
                continue;
            bound_arg[arg_i] = i;
            instActionNegPre(pddl, lift_action, pre, bound_arg,
                             arg_i + 1, fact_pool);
        }
    }
}

static void instActionNegPres(const plan_pddl_t *pddl,
                              const plan_pddl_lift_action_t *lift_action,
                              plan_pddl_fact_pool_t *fact_pool)
{
    const plan_pddl_fact_t *fact;
    int i, *bound_arg;

    for (i = 0; i < lift_action->pre_neg.size; ++i){
        fact = lift_action->pre_neg.fact + i;
        bound_arg = BOR_ALLOC_ARR(int, fact->arg_size);
        instActionNegPre(pddl, lift_action, fact, bound_arg, 0, fact_pool);
        BOR_FREE(bound_arg);
    }
}

static void instActionsNegativePreconditions(const plan_pddl_t *pddl,
                                             const plan_pddl_lift_actions_t *as,
                                             plan_pddl_fact_pool_t *fact_pool)
{
    int i;
    for (i = 0; i < as->size; ++i)
        instActionNegPres(pddl, as->action + i, fact_pool);
}
/**** INSTANTIATE NEGATIVE PRECONDITIONS END ****/

static void setFactOwners(plan_pddl_fact_pool_t *fact_pool, int *obj)
{
    const plan_pddl_fact_t *fact;
    int i;

    for (i = 0; i < fact_pool->size; ++i){
        fact = planPDDLFactPoolGet(fact_pool, i);
        if (fact->owner >= 0)
            obj[fact->owner] = 0;
    }
}

static void setOpOwners(plan_pddl_ground_action_pool_t *action_pool, int *obj)
{
    const plan_pddl_ground_action_t *a;
    int i;

    for (i = 0; i < action_pool->size; ++i){
        a = planPDDLGroundActionPoolGet(action_pool, i);
        if (a->owner >= 0)
            obj[a->owner] = 0;
    }
}

static void determineAgents(plan_pddl_ground_t *g, const plan_pddl_t *pddl)
{
    int i;

    // Initialize mapping from object to agent
    g->obj_to_agent = BOR_ALLOC_ARR(int, pddl->obj.size);
    for (i = 0; i < pddl->obj.size; ++i)
        g->obj_to_agent[i] = -1;

    // Find out all owners of facts and actions
    setFactOwners(&g->fact_pool, g->obj_to_agent);
    setOpOwners(&g->action_pool, g->obj_to_agent);

    // Determine number of owners and assign agent ID
    g->agent_size = 0;
    for (i = 0; i < pddl->obj.size; ++i){
        if (g->obj_to_agent[i] == 0)
            g->obj_to_agent[i] = g->agent_size++;
    }

    // Create mapping from agent ID to object ID
    g->agent_to_obj = BOR_ALLOC_ARR(int, g->agent_size);
    for (i = 0; i < pddl->obj.size; ++i){
        if (g->obj_to_agent[i] >= 0)
            g->agent_to_obj[g->obj_to_agent[i]] = i;
    }
}
