#ifndef __PLAN_HEUR_DTG_H__
#define __PLAN_HEUR_DTG_H__

struct _plan_heur_dtg_path_pre_t {
    plan_val_t val;
    int len;
};
typedef struct _plan_heur_dtg_path_pre_t plan_heur_dtg_path_pre_t;

struct _plan_heur_dtg_path_t {
    plan_heur_dtg_path_pre_t *pre;
};
typedef struct _plan_heur_dtg_path_t plan_heur_dtg_path_t;

struct _plan_heur_dtg_path_cache_t {
    int var_size;
    plan_heur_dtg_path_t **path;
    int *range;
};
typedef struct _plan_heur_dtg_path_cache_t plan_heur_dtg_path_cache_t;

struct _plan_heur_dtg_values_t {
    int *val_arr;          /*!< Array for all values from all variables */
    int val_arr_byte_size; /*!< Size of .value_arr in bytes */
    int **val;             /*!< Values indexed by variable ID */
    int *val_range;        /*!< Range of values for each variable */
};
typedef struct _plan_heur_dtg_values_t plan_heur_dtg_values_t;

struct _plan_heur_dtg_open_goal_t {
    plan_var_id_t var;
    plan_val_t val;
    plan_val_t min_val;
    int min_dist;
};
typedef struct _plan_heur_dtg_open_goal_t plan_heur_dtg_open_goal_t;

struct _plan_heur_dtg_open_goals_t {
    plan_heur_dtg_values_t values;
    plan_heur_dtg_open_goal_t *goals;
    int goals_size;
    int goals_alloc;
};
typedef struct _plan_heur_dtg_open_goals_t plan_heur_dtg_open_goals_t;

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

#endif /* __PLAN_HEUR_DTG_H__ */
