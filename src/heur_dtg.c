#include <boruvka/alloc.h>
#include <plan/heur.h>
#include <plan/dtg.h>

struct _init_values_t {
    int *val_arr;          /*!< Array for all values from all variables */
    int val_arr_byte_size; /*!< Size of .value_arr in bytes */
    int **val;             /*!< Values indexed by variable ID */
    int *val_range;        /*!< Range of values for each variable */
};
typedef struct _init_values_t init_values_t;

/** Initializes structure */
static void initValuesInit(init_values_t *v,
                           const plan_var_t *var, int var_size);
/** Free allocated resources */
static void initValuesFree(init_values_t *v);
/** Remove all stored info from the structure */
static void initValuesClean(init_values_t *v);
/** Sets variable-value pair in the structre and returns 0 if var-val pair
 *  was not already set and 1 if the pair was already set. */
static int initValuesSet(init_values_t *v, plan_var_id_t var, plan_val_t val);

struct _plan_heur_dtg_t {
    plan_heur_t heur;
    plan_dtg_t dtg;
    plan_part_state_pair_t *goal;
    int goal_size;

    init_values_t init_values;
};
typedef struct _plan_heur_dtg_t plan_heur_dtg_t;

#define HEUR(parent) \
    bor_container_of((parent), plan_heur_dtg_t, heur)

static void heurDTGDel(plan_heur_t *_heur);
static void heurDTG(plan_heur_t *_heur, const plan_state_t *state,
                    plan_heur_res_t *res);

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

    initValuesInit(&hdtg->init_values, var, var_size);
    return &hdtg->heur;
}

static void heurDTGDel(plan_heur_t *_heur)
{
    plan_heur_dtg_t *hdtg = HEUR(_heur);

    initValuesFree(&hdtg->init_values);
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

    initValuesClean(&hdtg->init_values);
    for (i = 0; i < hdtg->goal_size; ++i){
        initValuesSet(&hdtg->init_values, hdtg->goal[i].var,
                      planStateGet(state, hdtg->goal[i].var));
    }

    res->heur = PLAN_HEUR_DEAD_END;
}



static void initValuesInit(init_values_t *v,
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

static void initValuesFree(init_values_t *v)
{
    BOR_FREE(v->val_arr);
    BOR_FREE(v->val);
    BOR_FREE(v->val_range);
}

static void initValuesClean(init_values_t *v)
{
    bzero(v->val_arr, v->val_arr_byte_size);
}

static int initValuesSet(init_values_t *v, plan_var_id_t var, plan_val_t val)
{
    if (v->val[var][val])
        return 1;
    v->val[var][val] = 1;
    return 0;
}
