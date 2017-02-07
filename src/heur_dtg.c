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
#include <boruvka/fifo.h>
#include <plan/heur.h>
#include <plan/dtg.h>

#include "heur_dtg.h"

/**
 * Main structure for DTG heuristic
 */
struct _plan_heur_dtg_t {
    plan_heur_t heur;
    plan_heur_dtg_data_t data;
    plan_heur_dtg_ctx_t ctx;
    plan_part_state_pair_t *goal;
    int goal_size;
};
typedef struct _plan_heur_dtg_t plan_heur_dtg_t;
#define HEUR(parent) \
    bor_container_of((parent), plan_heur_dtg_t, heur)

static void heurDTGDel(plan_heur_t *_heur);
static void heurDTG(plan_heur_t *_heur, const plan_state_t *state,
                    plan_heur_res_t *res);

plan_heur_t *planHeurDTGNew(const plan_problem_t *p, unsigned flags)
{
    plan_heur_dtg_t *hdtg;

    hdtg = BOR_ALLOC(plan_heur_dtg_t);
    _planHeurInit(&hdtg->heur, heurDTGDel, heurDTG, NULL);
    planHeurDTGDataInit(&hdtg->data, p->var, p->var_size, p->op, p->op_size);
    planHeurDTGCtxInit(&hdtg->ctx, &hdtg->data);

    // Save goal values
    hdtg->goal = BOR_ALLOC_ARR(plan_part_state_pair_t, p->goal->vals_size);
    hdtg->goal_size = p->goal->vals_size;
    memcpy(hdtg->goal, p->goal->vals,
           sizeof(plan_part_state_pair_t) * p->goal->vals_size);

    return &hdtg->heur;
}

static void heurDTGDel(plan_heur_t *_heur)
{
    plan_heur_dtg_t *hdtg = HEUR(_heur);

    planHeurDTGDataFree(&hdtg->data);
    planHeurDTGCtxFree(&hdtg->ctx);
    if (hdtg->goal)
        BOR_FREE(hdtg->goal);
    _planHeurFree(&hdtg->heur);
    BOR_FREE(hdtg);
}

static void heurDTG(plan_heur_t *_heur, const plan_state_t *state,
                    plan_heur_res_t *res)
{
    plan_heur_dtg_t *hdtg = HEUR(_heur);

    planHeurDTGCtxInitStep(&hdtg->ctx, &hdtg->data,
                           state->val, state->size,
                           hdtg->goal, hdtg->goal_size);
    while (planHeurDTGCtxStep(&hdtg->ctx, &hdtg->data) == 0);
    res->heur = hdtg->ctx.heur;
}



/** Explores var's DTG from val to all other values and stores paths into
 *  path argument. */
static void dtgPathExplore(const plan_dtg_t *dtg, plan_var_id_t var,
                           plan_val_t val, plan_heur_dtg_path_t *path);
/** Frees allocated memory */
static void dtgPathFree(plan_heur_dtg_path_t *path);


/** Initializes and destroyes path cache */
static void dtgPathCacheInit(plan_heur_dtg_path_cache_t *pc,
                             const plan_dtg_t *dtg);
static void dtgPathCacheFree(plan_heur_dtg_path_cache_t *pc);
/** Returns pointer to the cached dtg-path which may be empty */
static plan_heur_dtg_path_t *dtgPathCache(plan_heur_dtg_path_cache_t *pc,
                                          plan_var_id_t var,
                                          plan_val_t val);


/** Initializes structure */
static void valuesInit(plan_heur_dtg_values_t *v, const plan_dtg_t *dtg);
/** Free allocated resources */
static void valuesFree(plan_heur_dtg_values_t *v);
/** Set all values to zero */
static void valuesZeroize(plan_heur_dtg_values_t *v);
/** Sets variable-value pair in the structre if the current value is zero.
 *  Return 0 if new value was set and -1 otherwise. */
static int valuesSet(plan_heur_dtg_values_t *v,
                     plan_var_id_t var, plan_val_t val);
static int valuesGet(const plan_heur_dtg_values_t *v,
                     plan_var_id_t var, plan_val_t val);


/** Initializes structure */
static void openGoalsInit(plan_heur_dtg_open_goals_t *g, const plan_dtg_t *dtg);
/** Free allocated resources */
static void openGoalsFree(plan_heur_dtg_open_goals_t *g);
/** Set all values to zero */
static void openGoalsZeroize(plan_heur_dtg_open_goals_t *g);
/** Adds goal to the structure */
static int openGoalsAdd(plan_heur_dtg_open_goals_t *g,
                        plan_var_id_t var, plan_val_t val);
/** Removes i'th goal */
static void openGoalsRemove(plan_heur_dtg_open_goals_t *g, int i);


/** Returns filled path for a specified var-val pair.
 *  The path is either returned from cache or computed */
static plan_heur_dtg_path_t *dataDtgPath(plan_heur_dtg_data_t *dtg_data,
                                         plan_var_id_t var,
                                         plan_val_t val);

/** Returns minimal distance from var values stored in dtg_ctx to the
 *  specified value val. If d is non-NULL also returns the value which is
 *  nearest. */
static int ctxMinDist(plan_heur_dtg_ctx_t *dtg_ctx,
                      plan_heur_dtg_data_t *dtg_data,
                      plan_var_id_t var, plan_val_t val,
                      plan_val_t *d);
/** Returns operator with minimal cost */
static const plan_op_t *ctxMinCostOp(plan_heur_dtg_ctx_t *dtg_ctx,
                                     plan_heur_dtg_data_t *dtg_data,
                                     plan_var_id_t var,
                                     plan_val_t from, plan_val_t to);
/** Adds goal to the open-goals queue */
static void ctxAddGoal(plan_heur_dtg_ctx_t *dtg_ctx,
                       plan_heur_dtg_data_t *dtg_data,
                       plan_var_id_t var, plan_val_t val);
/** Adds value to list of satisfied values */
static void ctxAddValue(plan_heur_dtg_ctx_t *dtg_ctx,
                        plan_heur_dtg_data_t *dtg_data,
                        plan_var_id_t var, plan_val_t val);
/** Pops next open-goal from queue */
static plan_heur_dtg_open_goal_t ctxNextOpenGoal(plan_heur_dtg_ctx_t *dtg_ctx);
static plan_cost_t ctxUpdatePath(plan_heur_dtg_ctx_t *dtg_ctx,
                                 plan_heur_dtg_data_t *dtg_data,
                                 const plan_heur_dtg_open_goal_t *goal);



void planHeurDTGDataInit(plan_heur_dtg_data_t *dtg_data,
                         const plan_var_t *var, int var_size,
                         const plan_op_t *op, int op_size)
{
    planDTGInit(&dtg_data->dtg, var, var_size, op, op_size);
    dtgPathCacheInit(&dtg_data->dtg_path, &dtg_data->dtg);
}

void planHeurDTGDataInitUnknown(plan_heur_dtg_data_t *dtg_data,
                                const plan_var_t *var, int var_size,
                                const plan_op_t *op, int op_size)
{
    planDTGInitUnknown(&dtg_data->dtg, var, var_size, op, op_size);
    dtgPathCacheInit(&dtg_data->dtg_path, &dtg_data->dtg);
}

void planHeurDTGDataFree(plan_heur_dtg_data_t *dtg_data)
{
    dtgPathCacheFree(&dtg_data->dtg_path);
    planDTGFree(&dtg_data->dtg);
}

plan_heur_dtg_path_t *planHeurDTGDataPath(plan_heur_dtg_data_t *dtg_data,
                                          plan_var_id_t var,
                                          plan_val_t val)
{
    return dataDtgPath(dtg_data, var, val);
}


void planHeurDTGCtxInit(plan_heur_dtg_ctx_t *dtg_ctx,
                        const plan_heur_dtg_data_t *dtg_data)
{
    valuesInit(&dtg_ctx->values, &dtg_data->dtg);
    openGoalsInit(&dtg_ctx->open_goals, &dtg_data->dtg);
    dtg_ctx->heur = 0;
}

void planHeurDTGCtxFree(plan_heur_dtg_ctx_t *dtg_ctx)
{
    openGoalsFree(&dtg_ctx->open_goals);
    valuesFree(&dtg_ctx->values);
}

void planHeurDTGCtxInitStep(plan_heur_dtg_ctx_t *dtg_ctx,
                            plan_heur_dtg_data_t *dtg_data,
                            const plan_val_t *init_state, int init_state_size,
                            const plan_part_state_pair_t *goal, int goal_size)
{
    int i;

    // Add values from initial state
    valuesZeroize(&dtg_ctx->values);
    for (i = 0; i < init_state_size; ++i)
        ctxAddValue(dtg_ctx, dtg_data, i, init_state[i]);

    // Add goals to open-goals queue
    openGoalsZeroize(&dtg_ctx->open_goals);
    for (i = 0; i < goal_size; ++i)
        ctxAddGoal(dtg_ctx, dtg_data, goal[i].var, goal[i].val);

    dtg_ctx->heur = 0;
}

int planHeurDTGCtxStep(plan_heur_dtg_ctx_t *dtg_ctx,
                       plan_heur_dtg_data_t *dtg_data)
{
    plan_heur_dtg_open_goal_t goal;

    if (dtg_ctx->open_goals.goals_size == 0)
        return -1;

    goal = ctxNextOpenGoal(dtg_ctx);
    dtg_ctx->heur += ctxUpdatePath(dtg_ctx, dtg_data, &goal);
    dtg_ctx->cur_open_goal = goal;
    return 0;
}





static void dtgPathExplore(const plan_dtg_t *_dtg, plan_var_id_t var,
                           plan_val_t val, plan_heur_dtg_path_t *path)
{
    const plan_dtg_var_t *dtg = _dtg->dtg + var;
    const plan_dtg_trans_t *trans;
    bor_fifo_t fifo;
    plan_cost_t len;
    plan_val_t v;
    int i;

    if (dtg->trans == NULL)
        return;

    path->pre = BOR_ALLOC_ARR(plan_heur_dtg_path_pre_t, dtg->val_size);
    for (i = 0; i < dtg->val_size; ++i){
        path->pre[i].val = -1;
        path->pre[i].len = INT_MAX;
    }

    borFifoInit(&fifo, sizeof(plan_val_t));
    path->pre[val].val = val;
    path->pre[val].len = 0;
    borFifoPush(&fifo, &val);
    while (!borFifoEmpty(&fifo)){
        v = *(plan_val_t *)borFifoFront(&fifo);
        borFifoPop(&fifo);
        len = path->pre[v].len;

        trans = dtg->trans + v;
        for (i = 0; i < dtg->val_size; ++i, trans += dtg->val_size){
            if (i == v || path->pre[i].val != -1 || trans->ops_size == 0)
                continue;
            path->pre[i].val = v;
            path->pre[i].len = len + 1;
            borFifoPush(&fifo, &i);
        }
    }
}

static void dtgPathFree(plan_heur_dtg_path_t *path)
{
    if (path->pre)
        BOR_FREE(path->pre);
}

static void dtgPathCacheInit(plan_heur_dtg_path_cache_t *pc,
                             const plan_dtg_t *dtg)
{
    int i;

    pc->path = BOR_ALLOC_ARR(plan_heur_dtg_path_t *, dtg->var_size);
    pc->range = BOR_ALLOC_ARR(int, dtg->var_size);
    for (i = 0; i < dtg->var_size; ++i){
        pc->path[i] = BOR_CALLOC_ARR(plan_heur_dtg_path_t, dtg->dtg[i].val_size);
        pc->range[i] = dtg->dtg[i].val_size;
    }
    pc->var_size = dtg->var_size;
}

static void dtgPathCacheFree(plan_heur_dtg_path_cache_t *pc)
{
    int i, j;

    for (i = 0; i < pc->var_size; ++i){
        for (j = 0; j < pc->range[i]; ++j){
            if (pc->path[i][j].pre != NULL)
                dtgPathFree(&pc->path[i][j]);
        }
        BOR_FREE(pc->path[i]);
    }
    BOR_FREE(pc->path);
    BOR_FREE(pc->range);
}

static plan_heur_dtg_path_t *dtgPathCache(plan_heur_dtg_path_cache_t *pc,
                                          plan_var_id_t var,
                                          plan_val_t val)
{
    return &pc->path[var][val];
}


static void valuesInit(plan_heur_dtg_values_t *v, const plan_dtg_t *dtg)
{
    int i, *val;

    // First compute size of the array for all values
    v->val_arr_byte_size = 0;
    for (i = 0; i < dtg->var_size; ++i)
        v->val_arr_byte_size += dtg->dtg[i].val_size;
    v->val_arr_byte_size *= sizeof(int);

    // Allocate array for all values
    v->val_arr = BOR_ALLOC_ARR(int, v->val_arr_byte_size / sizeof(int));

    // And allocate array for each variable separately
    v->val = BOR_ALLOC_ARR(int *, dtg->var_size);
    v->val_range = BOR_ALLOC_ARR(int, dtg->var_size);
    val = v->val_arr;
    for (i = 0; i < dtg->var_size; ++i){
        v->val[i] = val;
        v->val_range[i] = dtg->dtg[i].val_size;
        val += dtg->dtg[i].val_size;
    }
}

static void valuesFree(plan_heur_dtg_values_t *v)
{
    BOR_FREE(v->val_arr);
    BOR_FREE(v->val);
    BOR_FREE(v->val_range);
}

static void valuesZeroize(plan_heur_dtg_values_t *v)
{
    bzero(v->val_arr, v->val_arr_byte_size);
}

static int valuesSet(plan_heur_dtg_values_t *v,
                     plan_var_id_t var, plan_val_t val)
{
    if (val >= v->val_range[var])
        return -1;

    if (v->val[var][val] != 0)
        return -1;
    v->val[var][val] = 1;
    return 0;
}

static int valuesGet(const plan_heur_dtg_values_t *v,
                     plan_var_id_t var, plan_val_t val)
{
    return v->val[var][val];
}

static void openGoalsInit(plan_heur_dtg_open_goals_t *g, const plan_dtg_t *dtg)
{
    valuesInit(&g->values, dtg);
    g->goals_size = 0;
    g->goals_alloc = 10;
    g->goals = BOR_ALLOC_ARR(plan_heur_dtg_open_goal_t, g->goals_alloc);
}

static void openGoalsFree(plan_heur_dtg_open_goals_t *g)
{
    valuesFree(&g->values);
    if (g->goals)
        BOR_FREE(g->goals);
}

static void openGoalsZeroize(plan_heur_dtg_open_goals_t *g)
{
    valuesZeroize(&g->values);
    g->goals_size = 0;
}

static int openGoalsAdd(plan_heur_dtg_open_goals_t *g,
                        plan_var_id_t var, plan_val_t val)
{
    plan_heur_dtg_open_goal_t *goal;

    if (valuesGet(&g->values, var, val) != 0)
        return -1;

    if (g->goals_size >= g->goals_alloc){
        g->goals_alloc *= 2;
        g->goals = BOR_REALLOC_ARR(g->goals, plan_heur_dtg_open_goal_t,
                                   g->goals_alloc);
    }

    goal = g->goals + g->goals_size;
    goal->var = var;
    goal->val = val;
    valuesSet(&g->values, var, val);
    ++g->goals_size;

    return g->goals_size - 1;
}

static void openGoalsRemove(plan_heur_dtg_open_goals_t *g, int i)
{
    if (g->goals_size > i)
        g->goals[i] = g->goals[g->goals_size - 1];
    --g->goals_size;
}



static plan_heur_dtg_path_t *dataDtgPath(plan_heur_dtg_data_t *dtg_data,
                                         plan_var_id_t var,
                                         plan_val_t val)
{
    plan_heur_dtg_path_t *path;
    path = dtgPathCache(&dtg_data->dtg_path, var, val);
    if (path->pre == NULL)
        dtgPathExplore(&dtg_data->dtg, var, val, path);
    return path;
}

static int ctxMinDist(plan_heur_dtg_ctx_t *dtg_ctx,
                      plan_heur_dtg_data_t *dtg_data,
                      plan_var_id_t var, plan_val_t val,
                      plan_val_t *d)
{
    plan_heur_dtg_path_t *path;
    int len, i, val_range, *vals;

    if (d != NULL)
        *d = -1;

    // Early exit if we are searching for path from val to val
    vals = dtg_ctx->values.val[var];
    if (vals[val]){
        if (d != NULL)
            *d = val;
        return 0;
    }

    // Find out a value from registered values to which leads a minimal
    // path.
    path = dataDtgPath(dtg_data, var, val);
    len = INT_MAX;
    val_range = dtg_ctx->values.val_range[var];
    for (i = 0; i < val_range; ++i){
        if (vals[i] && path->pre[i].len < len){
            len = path->pre[i].len;
            if (d != NULL)
                *d = i;
        }
    }

    return len;
}

static const plan_op_t *ctxMinCostOp(plan_heur_dtg_ctx_t *dtg_ctx,
                                     plan_heur_dtg_data_t *dtg_data,
                                     plan_var_id_t var,
                                     plan_val_t from, plan_val_t to)
{
    const plan_dtg_var_t *dtg = dtg_data->dtg.dtg + var;
    const plan_dtg_trans_t *trans;
    const plan_op_t *op, *min_cost_op;
    int _, i, min_cost, cost, c;
    plan_var_id_t opvar;
    plan_val_t opval;


    if (dtg->trans == NULL)
        return NULL;
    trans = dtg->trans + (from * dtg->val_size) + to;
    if (trans->ops_size == 0)
        return NULL;

    min_cost = INT_MAX;
    min_cost_op = trans->ops[0];
    for (i = 0; i < trans->ops_size; ++i){
        op = trans->ops[i];

        cost = 0;
        PLAN_PART_STATE_FOR_EACH(op->pre, _, opvar, opval){
            c = ctxMinDist(dtg_ctx, dtg_data, opvar, opval, NULL);
            if (c == INT_MAX){
                cost = INT_MAX;
                break;
            }
            cost += c;
        }

        if (cost < min_cost){
            min_cost = cost;
            min_cost_op = op;
        }
    }

    return min_cost_op;
}

static void ctxAddGoal(plan_heur_dtg_ctx_t *dtg_ctx,
                       plan_heur_dtg_data_t *dtg_data,
                       plan_var_id_t var, plan_val_t val)
{
    int idx, min_dist;
    plan_val_t min_val;

    if (valuesGet(&dtg_ctx->values, var, val) != 0)
        return;

    idx = openGoalsAdd(&dtg_ctx->open_goals, var, val);
    if (idx < 0)
        return;

    // Compute and cache minimal distance and corresponding value
    min_dist = ctxMinDist(dtg_ctx, dtg_data, var, val, &min_val);
    dtg_ctx->open_goals.goals[idx].min_dist = min_dist;
    dtg_ctx->open_goals.goals[idx].min_val = min_val;
    dtg_ctx->open_goals.goals[idx].path = dataDtgPath(dtg_data, var, val);
}

static void ctxAddValue(plan_heur_dtg_ctx_t *dtg_ctx,
                        plan_heur_dtg_data_t *dtg_data,
                        plan_var_id_t var, plan_val_t val)
{
    int i, goals_size, min_dist;
    plan_val_t min_val;
    plan_heur_dtg_open_goal_t *goals;

    if (valuesSet(&dtg_ctx->values, var, val) != 0)
        return;

    // Update cached min_dist and min_val members of open goals with same
    // variable.
    goals_size = dtg_ctx->open_goals.goals_size;
    goals = dtg_ctx->open_goals.goals;
    for (i = 0; i < goals_size; ++i){
        if (goals[i].var == var){
            min_dist = ctxMinDist(dtg_ctx, dtg_data, var, goals[i].val, &min_val);
            goals[i].min_dist = min_dist;
            goals[i].min_val = min_val;
            goals[i].path = dataDtgPath(dtg_data, var, goals[i].val);
        }
    }
}

static plan_heur_dtg_open_goal_t ctxNextOpenGoal(plan_heur_dtg_ctx_t *dtg_ctx)
{
    int i, max_cost, max_i;
    int goals_size = dtg_ctx->open_goals.goals_size;
    plan_heur_dtg_open_goal_t *goals = dtg_ctx->open_goals.goals;
    plan_heur_dtg_open_goal_t ret;

    max_cost = -1;
    max_i = 0;
    for (i = 0; i < goals_size; ++i){
        if (goals[i].min_dist > max_cost){
            max_cost = goals[i].min_dist;
            max_i = i;
        }
    }

    ret = goals[max_i];
    openGoalsRemove(&dtg_ctx->open_goals, max_i);
    return ret;
}

static plan_cost_t ctxUpdatePath(plan_heur_dtg_ctx_t *dtg_ctx,
                                 plan_heur_dtg_data_t *dtg_data,
                                 const plan_heur_dtg_open_goal_t *goal)
{
    plan_heur_dtg_path_t *path;
    const plan_op_t *op;
    plan_var_id_t var;
    plan_val_t val;
    int from, to;
    int _, i, len = goal->min_dist;
    plan_cost_t cost = 0;

    path = dataDtgPath(dtg_data, goal->var, goal->val);
    from = goal->min_val;
    if (from < 0)
        return 0;
    for (i = 0; i < len; ++i){
        to = path->pre[from].val;
        op = ctxMinCostOp(dtg_ctx, dtg_data, goal->var, from, to);

        PLAN_PART_STATE_FOR_EACH(op->eff, _, var, val)
            ctxAddValue(dtg_ctx, dtg_data, var, val);
        PLAN_PART_STATE_FOR_EACH(op->pre, _, var, val)
            ctxAddGoal(dtg_ctx, dtg_data, var, val);

        if (op->eff->vals_size > 0 && op->pre->vals_size > 0)
            cost += 1;

        from = to;
    }

    return cost;
}
