#include <boruvka/alloc.h>
#include <boruvka/fifo.h>
#include <plan/heur.h>
#include <plan/dtg.h>

struct _dtg_path_pre_t {
    plan_val_t val;
    int len;
};
typedef struct _dtg_path_pre_t dtg_path_pre_t;

struct _dtg_path_t {
    dtg_path_pre_t *pre;
};
typedef struct _dtg_path_t dtg_path_t;

static void dtgPathExplore(const plan_dtg_t *dtg, plan_var_id_t var,
                           plan_val_t val, dtg_path_t *path);
static void dtgPathFree(dtg_path_t *path);


struct _values_t {
    int *val_arr;          /*!< Array for all values from all variables */
    int val_arr_byte_size; /*!< Size of .value_arr in bytes */
    int **val;             /*!< Values indexed by variable ID */
    int *val_range;        /*!< Range of values for each variable */
};
typedef struct _values_t values_t;

/** Initializes structure */
static void valuesInit(values_t *v, const plan_var_t *var, int var_size);
/** Free allocated resources */
static void valuesFree(values_t *v);
/** Set all values to zero */
static void valuesZeroize(values_t *v);
/** Sets variable-value pair in the structre if the current value is zero.
 *  Return 0 if new value was set and -1 otherwise. */
static int valuesSet(values_t *v, plan_var_id_t var, plan_val_t val,
                     int set_val);


struct _open_goal_t {
    plan_var_id_t var;
    plan_val_t val;
    dtg_path_t path;
};
typedef struct _open_goal_t open_goal_t;

struct _open_goals_t {
    values_t values;
    open_goal_t *goals;
    int goals_size;
    int goals_alloc;
};
typedef struct _open_goals_t open_goals_t;

/** Initializes structure */
static void openGoalsInit(open_goals_t *g, const plan_var_t *var, int var_size);
/** Free allocated resources */
static void openGoalsFree(open_goals_t *g);
/** Set all values to zero */
static void openGoalsZeroize(open_goals_t *g);
/** Adds goal to the structure */
static void openGoalsAdd(open_goals_t *g, plan_var_id_t var, plan_val_t val,
                         const plan_dtg_t *dtg, const values_t *values);

struct _plan_heur_dtg_t {
    plan_heur_t heur;
    plan_dtg_t dtg;
    plan_part_state_pair_t *goal;
    int goal_size;

    values_t values;
    open_goals_t open_goals;
};
typedef struct _plan_heur_dtg_t plan_heur_dtg_t;

#define HEUR(parent) \
    bor_container_of((parent), plan_heur_dtg_t, heur)

static void heurDTGDel(plan_heur_t *_heur);
static void heurDTG(plan_heur_t *_heur, const plan_state_t *state,
                    plan_heur_res_t *res);

static int minDist(plan_heur_dtg_t *dtg, plan_var_id_t var, plan_val_t val,
                   plan_val_t *d);
static const plan_op_t *minCostOp(plan_heur_dtg_t *dtg, plan_var_id_t var,
                                  plan_val_t from, plan_val_t to);

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
    return &hdtg->heur;
}

static void heurDTGDel(plan_heur_t *_heur)
{
    plan_heur_dtg_t *hdtg = HEUR(_heur);

    openGoalsFree(&hdtg->open_goals);
    valuesFree(&hdtg->values);
    if (hdtg->goal)
        BOR_FREE(hdtg->goal);
    planDTGFree(&hdtg->dtg);
    _planHeurFree(&hdtg->heur);
    BOR_FREE(hdtg);
}

static void heurDTG(plan_heur_t *_heur, const plan_state_t *state,
                    plan_heur_res_t *res)
{
    plan_heur_dtg_t *hdtg = HEUR(_heur);
    int i;

    valuesZeroize(&hdtg->values);
    openGoalsZeroize(&hdtg->open_goals);
    for (i = 0; i < hdtg->goal_size; ++i){
        valuesSet(&hdtg->values, hdtg->goal[i].var,
                  planStateGet(state, hdtg->goal[i].var), 1);
        openGoalsAdd(&hdtg->open_goals, hdtg->goal[i].var, hdtg->goal[i].val,
                     &hdtg->dtg, &hdtg->values);
    }

    res->heur = PLAN_HEUR_DEAD_END;
}



static int minDist(plan_heur_dtg_t *dtg, plan_var_id_t var, plan_val_t val,
                   plan_val_t *d)
{
    dtg_path_t path;
    int len, i;

    dtgPathExplore(&dtg->dtg, var, val, &path);

    len = PLAN_COST_MAX;
    for (i = 0; i < dtg->dtg.dtg[var].val_size; ++i){
        if (path.pre[i].len < len){
            len = path.pre[i].len;
            if (d != NULL)
                *d = path.pre[i].val;
        }
    }
    dtgPathFree(&path);

    return len;
}

static const plan_op_t *minCostOp(plan_heur_dtg_t *hdtg, plan_var_id_t var,
                                  plan_val_t from, plan_val_t to)
{
    const plan_dtg_var_t *dtg = hdtg->dtg.dtg + var;
    const plan_dtg_trans_t *trans;
    const plan_op_t *op, *min_cost_op;
    int _, i, min_cost, cost;
    plan_var_id_t opvar;
    plan_val_t opval;


    trans = dtg->trans + (from * dtg->val_size) + to;
    min_cost = INT_MAX;
    min_cost_op = NULL;
    for (i = 0; i < trans->ops_size; ++i){
        op = trans->ops[i];

        cost = 0;
        PLAN_PART_STATE_FOR_EACH(op->pre, _, opvar, opval)
            cost += minDist(hdtg, opvar, opval, NULL);

        if (cost < min_cost){
            min_cost = cost;
            min_cost_op = op;
        }
    }

    return min_cost_op;
}

static void dtgPathExplore(const plan_dtg_t *_dtg, plan_var_id_t var,
                           plan_val_t val, dtg_path_t *path)
{
    const plan_dtg_var_t *dtg = _dtg->dtg + var;
    const plan_dtg_trans_t *trans;
    bor_fifo_t fifo;
    plan_cost_t len;
    plan_val_t v;
    int i;

    path->pre = BOR_ALLOC_ARR(dtg_path_pre_t, dtg->val_size);
    for (i = 0; i < dtg->val_size; ++i){
        path->pre[i].val = -1;
        path->pre[i].len = PLAN_COST_MAX;
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

static void dtgPathFree(dtg_path_t *path)
{
    if (path->pre)
        BOR_FREE(path->pre);
}

static void valuesInit(values_t *v, const plan_var_t *var, int var_size)
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

static void valuesFree(values_t *v)
{
    BOR_FREE(v->val_arr);
    BOR_FREE(v->val);
    BOR_FREE(v->val_range);
}

static void valuesZeroize(values_t *v)
{
    bzero(v->val_arr, v->val_arr_byte_size);
}

static int valuesSet(values_t *v, plan_var_id_t var, plan_val_t val,
                     int set_val)
{
    if (v->val[var][val] != 0)
        return -1;
    v->val[var][val] = set_val;
    return 0;
}

static void openGoalsInit(open_goals_t *g, const plan_var_t *var, int var_size)
{
    valuesInit(&g->values, var, var_size);
    g->goals_size = 0;
    g->goals_alloc = 10;
    g->goals = BOR_ALLOC_ARR(open_goal_t, g->goals_alloc);
}

static void openGoalsFree(open_goals_t *g)
{
    valuesFree(&g->values);
    if (g->goals)
        BOR_FREE(g->goals);
}

static void openGoalsZeroize(open_goals_t *g)
{
    valuesZeroize(&g->values);
    g->goals_size = 0;
}

static void openGoalsAdd(open_goals_t *g, plan_var_id_t var, plan_val_t val,
                         const plan_dtg_t *dtg, const values_t *values)
{
    int idx;
    open_goal_t *goal;

    idx = g->values.val[var][val];
    if (idx != 0)
        return;

    if (g->goals_size >= g->goals_alloc){
        g->goals_alloc *= 2;
        g->goals = BOR_REALLOC_ARR(g->goals, open_goal_t, g->goals_alloc);
    }

    goal = g->goals + g->goals_size;
    goal->var = var;
    goal->val = val;
    valuesSet(&g->values, var, val, g->goals_size + 1);
    ++g->goals_size;

    dtgPathExplore(dtg, var, val, &goal->path);
}
