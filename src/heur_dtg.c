#include <boruvka/alloc.h>
#include <boruvka/fifo.h>
#include <plan/heur.h>
#include <plan/dtg.h>

#include "heur_dtg.h"

static void dtgPathExplore(const plan_dtg_t *dtg, plan_var_id_t var,
                           plan_val_t val, plan_heur_dtg_path_t *path);
static void dtgPathFree(plan_heur_dtg_path_t *path);


static void dtgPathCacheInit(plan_heur_dtg_path_cache_t *pc,
                             const plan_var_t *var, int var_size);
static void dtgPathCacheFree(plan_heur_dtg_path_cache_t *pc);
static plan_heur_dtg_path_t *dtgPathCache(plan_heur_dtg_path_cache_t *pc,
                                          plan_var_id_t var,
                                          plan_val_t val);


/** Initializes structure */
static void valuesInit(plan_heur_dtg_values_t *v,
                       const plan_var_t *var, int var_size);
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
static void openGoalsInit(plan_heur_dtg_open_goals_t *g,
                          const plan_var_t *var, int var_size);
/** Free allocated resources */
static void openGoalsFree(plan_heur_dtg_open_goals_t *g);
/** Set all values to zero */
static void openGoalsZeroize(plan_heur_dtg_open_goals_t *g);
/** Adds goal to the structure */
static int openGoalsAdd(plan_heur_dtg_open_goals_t *g,
                        plan_var_id_t var, plan_val_t val);
/** Removes i'th goal */
static void openGoalsRemove(plan_heur_dtg_open_goals_t *g, int i);

/**
 * Main structure for DTG heuristic
 */
struct _plan_heur_dtg_t {
    plan_heur_t heur;
    plan_dtg_t dtg;
    plan_part_state_pair_t *goal;
    int goal_size;

    plan_heur_dtg_values_t values;
    plan_heur_dtg_open_goals_t open_goals;
    plan_heur_dtg_path_cache_t dtg_path;
};
typedef struct _plan_heur_dtg_t plan_heur_dtg_t;
#define HEUR(parent) \
    bor_container_of((parent), plan_heur_dtg_t, heur)

static void heurDTGDel(plan_heur_t *_heur);
static void heurDTG(plan_heur_t *_heur, const plan_state_t *state,
                    plan_heur_res_t *res);

static plan_heur_dtg_path_t *dtgPath(plan_heur_dtg_t *hdtg,
                                     plan_var_id_t var,
                                     plan_val_t val);
static int minDist(plan_heur_dtg_t *dtg, plan_var_id_t var, plan_val_t val,
                   plan_val_t *d);
static const plan_op_t *minCostOp(plan_heur_dtg_t *dtg, plan_var_id_t var,
                                  plan_val_t from, plan_val_t to);
static void addGoal(plan_heur_dtg_t *hdtg, plan_var_id_t var, plan_val_t val);
static void addValue(plan_heur_dtg_t *hdtg, plan_var_id_t var, plan_val_t val);
static plan_heur_dtg_open_goal_t nextOpenGoal(plan_heur_dtg_t *hdtg);


void planHeurDTGDataInit(plan_heur_dtg_data_t *dtg_data,
                         const plan_var_t *var, int var_size,
                         const plan_op_t *op, int op_size)
{
    planDTGInit(&dtg_data->dtg, var, var_size, op, op_size);
    dtgPathCacheInit(&dtg_data->dtg_path, var, var_size);
}

void planHeurDTGDataFree(plan_heur_dtg_data_t *dtg_data)
{
    dtgPathCacheFree(&dtg_data->dtg_path);
    planDTGFree(&dtg_data->dtg);
}


void planHeurDTGCtxInit(plan_heur_dtg_ctx_t *dtg_ctx,
                        const plan_var_t *var, int var_size)
{
    valuesInit(&dtg_ctx->values, var, var_size);
    openGoalsInit(&dtg_ctx->open_goals, var, var_size);
}

void planHeurDTGCtxFree(plan_heur_dtg_ctx_t *dtg_ctx)
{
    openGoalsFree(&dtg_ctx->open_goals);
    valuesFree(&dtg_ctx->values);
}


plan_heur_t *planHeurDTGNew(const plan_var_t *var, int var_size,
                            const plan_part_state_t *goal,
                            const plan_op_t *op, int op_size)
{
    plan_heur_dtg_t *hdtg;

    hdtg = BOR_ALLOC(plan_heur_dtg_t);
    _planHeurInit(&hdtg->heur, heurDTGDel, heurDTG);
    planDTGInit(&hdtg->dtg, var, var_size, op, op_size);

    // Save goal values
    hdtg->goal = BOR_ALLOC_ARR(plan_part_state_pair_t, goal->vals_size);
    hdtg->goal_size = goal->vals_size;
    memcpy(hdtg->goal, goal->vals,
           sizeof(plan_part_state_pair_t) * goal->vals_size);

    valuesInit(&hdtg->values, var, var_size);
    openGoalsInit(&hdtg->open_goals, var, var_size);
    dtgPathCacheInit(&hdtg->dtg_path, var, var_size);
    return &hdtg->heur;
}

static void heurDTGDel(plan_heur_t *_heur)
{
    plan_heur_dtg_t *hdtg = HEUR(_heur);

    dtgPathCacheFree(&hdtg->dtg_path);
    openGoalsFree(&hdtg->open_goals);
    valuesFree(&hdtg->values);
    if (hdtg->goal)
        BOR_FREE(hdtg->goal);
    planDTGFree(&hdtg->dtg);
    _planHeurFree(&hdtg->heur);
    BOR_FREE(hdtg);
}

static void updateByPath(plan_heur_dtg_t *hdtg, const plan_heur_dtg_open_goal_t *goal)
{
    plan_heur_dtg_path_t *path;
    const plan_op_t *op;
    plan_var_id_t var;
    plan_val_t val;
    int from, to;
    int _, i, len = goal->min_dist;

    path = dtgPath(hdtg, goal->var, goal->val);
    from = goal->min_val;
    for (i = 0; i < len; ++i){
        to = path->pre[from].val;
        op = minCostOp(hdtg, goal->var, from, to);

        PLAN_PART_STATE_FOR_EACH(op->eff, _, var, val)
            addValue(hdtg, var, val);
        PLAN_PART_STATE_FOR_EACH(op->pre, _, var, val)
            addGoal(hdtg, var, val);

        from = to;
    }
}

static void heurDTG(plan_heur_t *_heur, const plan_state_t *state,
                    plan_heur_res_t *res)
{
    plan_heur_dtg_t *hdtg = HEUR(_heur);
    plan_heur_dtg_open_goal_t goal;
    int i, size;

    // Add values from initial state
    valuesZeroize(&hdtg->values);
    size = planStateSize(state);
    for (i = 0; i < size; ++i)
        addValue(hdtg, i, planStateGet(state, i));

    // Add goals to open-goals queue
    openGoalsZeroize(&hdtg->open_goals);
    for (i = 0; i < hdtg->goal_size; ++i)
        addGoal(hdtg, hdtg->goal[i].var, hdtg->goal[i].val);

    res->heur = 0;
    while (hdtg->open_goals.goals_size > 0){
        goal = nextOpenGoal(hdtg);
        res->heur += goal.min_dist;
        updateByPath(hdtg, &goal);
    }
}


static plan_heur_dtg_path_t *dtgPath(plan_heur_dtg_t *hdtg,
                                     plan_var_id_t var,
                                     plan_val_t val)
{
    plan_heur_dtg_path_t *path;
    path = dtgPathCache(&hdtg->dtg_path, var, val);
    if (path->pre == NULL)
        dtgPathExplore(&hdtg->dtg, var, val, path);
    return path;
}

static int minDist(plan_heur_dtg_t *hdtg, plan_var_id_t var, plan_val_t val,
                   plan_val_t *d)
{
    plan_heur_dtg_path_t *path;
    int len, i, val_range, *vals;

    if (d != NULL)
        *d = -1;

    // Early exit if we are searching for path from val to val
    vals = hdtg->values.val[var];
    if (vals[val]){
        if (d != NULL)
            *d = val;
        return 0;
    }

    // Find out a value from registered values to which leads a minimal
    // path.
    path = dtgPath(hdtg, var, val);
    len = INT_MAX;
    val_range = hdtg->values.val_range[var];
    for (i = 0; i < val_range; ++i){
        if (vals[i] && path->pre[i].len < len){
            len = path->pre[i].len;
            if (d != NULL)
                *d = i;
        }
    }

    return len;
}

static const plan_op_t *minCostOp(plan_heur_dtg_t *hdtg, plan_var_id_t var,
                                  plan_val_t from, plan_val_t to)
{
    const plan_dtg_var_t *dtg = hdtg->dtg.dtg + var;
    const plan_dtg_trans_t *trans;
    const plan_op_t *op, *min_cost_op;
    int _, i, min_cost, cost, c;
    plan_var_id_t opvar;
    plan_val_t opval;


    trans = dtg->trans + (from * dtg->val_size) + to;
    min_cost = INT_MAX;
    min_cost_op = NULL;
    for (i = 0; i < trans->ops_size; ++i){
        op = trans->ops[i];

        cost = 0;
        PLAN_PART_STATE_FOR_EACH(op->pre, _, opvar, opval){
            c = minDist(hdtg, opvar, opval, NULL);
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

static void addGoal(plan_heur_dtg_t *hdtg, plan_var_id_t var, plan_val_t val)
{
    int idx, min_dist;
    plan_val_t min_val;

    if (valuesGet(&hdtg->values, var, val) != 0)
        return;

    idx = openGoalsAdd(&hdtg->open_goals, var, val);
    if (idx < 0)
        return;

    // Compute and cache minimal distance and corresponding value
    min_dist = minDist(hdtg, var, val, &min_val);
    hdtg->open_goals.goals[idx].min_dist = min_dist;
    hdtg->open_goals.goals[idx].min_val = min_val;
}

static void addValue(plan_heur_dtg_t *hdtg, plan_var_id_t var, plan_val_t val)
{
    int i, goals_size, min_dist;
    plan_val_t min_val;
    plan_heur_dtg_open_goal_t *goals;

    if (valuesSet(&hdtg->values, var, val) != 0)
        return;

    // Update cached min_dist and min_val members of open goals with same
    // variable.
    goals_size = hdtg->open_goals.goals_size;
    goals = hdtg->open_goals.goals;
    for (i = 0; i < goals_size; ++i){
        if (goals[i].var == var){
            min_dist = minDist(hdtg, var, goals[i].val, &min_val);
            goals[i].min_dist = min_dist;
            goals[i].min_val = min_val;
        }
    }
}

static plan_heur_dtg_open_goal_t nextOpenGoal(plan_heur_dtg_t *hdtg)
{
    int i, max_cost, max_i;
    int goals_size = hdtg->open_goals.goals_size;
    plan_heur_dtg_open_goal_t *goals = hdtg->open_goals.goals;
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
    openGoalsRemove(&hdtg->open_goals, max_i);
    return ret;
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
                             const plan_var_t *var, int var_size)
{
    int i;

    pc->path = BOR_ALLOC_ARR(plan_heur_dtg_path_t *, var_size);
    pc->range = BOR_ALLOC_ARR(int, var_size);
    for (i = 0; i < var_size; ++i){
        pc->path[i] = BOR_CALLOC_ARR(plan_heur_dtg_path_t, var[i].range);
        pc->range[i] = var[i].range;
    }
    pc->var_size = var_size;
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


static void valuesInit(plan_heur_dtg_values_t *v,
                       const plan_var_t *var, int var_size)
{
    int i, *val;

    // First compute size of the array for all values
    v->val_arr_byte_size = 0;
    for (i = 0; i < var_size; ++i)
        v->val_arr_byte_size += var[i].range;
    v->val_arr_byte_size *= sizeof(int);

    // Allocate array for all values
    v->val_arr = BOR_ALLOC_ARR(int, v->val_arr_byte_size / sizeof(int));

    // And allocate array for each variable separately
    v->val = BOR_ALLOC_ARR(int *, var_size);
    v->val_range = BOR_ALLOC_ARR(int, var_size);
    val = v->val_arr;
    for (i = 0; i < var_size; ++i){
        v->val[i] = val;
        v->val_range[i] = var[i].range;
        val += var[i].range;
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

static void openGoalsInit(plan_heur_dtg_open_goals_t *g,
                          const plan_var_t *var, int var_size)
{
    valuesInit(&g->values, var, var_size);
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
