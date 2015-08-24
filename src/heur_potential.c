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
#include <plan/config.h>
#include <plan/heur.h>

#ifdef PLAN_LP
#include "lp.h"

struct _fact_var_map_t {
    int *val;
    int range;
    int max;
    int ma_privacy;
};
typedef struct _fact_var_map_t fact_var_map_t;

struct _fact_map_t {
    int size;
    fact_var_map_t *var;
    int var_size;
};
typedef struct _fact_map_t fact_map_t;

static void factMapInit(fact_map_t *map,
                        const plan_var_t *var, int var_size,
                        const plan_part_state_t *goal,
                        const plan_op_t *op, int op_size,
                        unsigned flags);
static void factMapFree(fact_map_t *map);


struct _plan_heur_potential_t {
    plan_heur_t heur;
    fact_map_t fact_map;
    double *pot;
    plan_lp_t *lp;
};
typedef struct _plan_heur_potential_t plan_heur_potential_t;
#define HEUR(parent) bor_container_of((parent), plan_heur_potential_t, heur)

static void heurPotentialDel(plan_heur_t *_heur);
static void heurPotential(plan_heur_t *_heur, const plan_state_t *state,
                          plan_heur_res_t *res);
static void lpInit(plan_heur_potential_t *h,
                   const plan_var_t *var, int var_size,
                   const plan_part_state_t *goal,
                   const plan_op_t *op, int op_size,
                   unsigned flags);
static void lpSolve(plan_heur_potential_t *h,
                    const plan_state_t *state,
                    double *pot);

plan_heur_t *planHeurPotentialNew(const plan_var_t *var, int var_size,
                                  const plan_part_state_t *goal,
                                  const plan_op_t *op, int op_size,
                                  const plan_state_t *init_state,
                                  unsigned flags)
{
    plan_heur_potential_t *heur;

    heur = BOR_ALLOC(plan_heur_potential_t);
    bzero(heur, sizeof(*heur));
    _planHeurInit(&heur->heur, heurPotentialDel, heurPotential, NULL);

    factMapInit(&heur->fact_map, var, var_size, goal, op, op_size, flags);
    heur->pot = BOR_CALLOC_ARR(double, heur->fact_map.size);
    lpInit(heur, var, var_size, goal, op, op_size, flags);
    if (init_state != NULL)
        lpSolve(heur, init_state, heur->pot);

    return &heur->heur;
}

static void heurPotentialDel(plan_heur_t *_heur)
{
    plan_heur_potential_t *h = HEUR(_heur);

    if (h->lp)
        planLPDel(h->lp);
    if (h->pot != NULL)
        BOR_FREE(h->pot);
    factMapFree(&h->fact_map);
    _planHeurFree(&h->heur);
    BOR_FREE(h);
}

static void heurPotential(plan_heur_t *_heur, const plan_state_t *state,
                          plan_heur_res_t *res)
{
    plan_heur_potential_t *h = HEUR(_heur);
    int var, val;
    double pot = 0.;

    for (var = 0; var < h->fact_map.var_size; ++var){
        if (h->fact_map.var[var].ma_privacy)
            continue;
        val = planStateGet(state, var);
        pot += h->pot[h->fact_map.var[var].val[val]];
    }

    res->heur = pot;
    res->heur = BOR_MAX(0, res->heur);
}

static void lpSetOp(plan_heur_potential_t *h, int row_id,
                    const plan_op_t *op, unsigned flags)
{
    plan_var_id_t var;
    plan_val_t val, pre_val;
    int i, eff_id, pre_id;
    double cost;

    PLAN_PART_STATE_FOR_EACH(op->eff, i, var, val){
        eff_id = h->fact_map.var[var].val[val];
        pre_val = planPartStateGet(op->pre, var);
        if (pre_val == PLAN_VAL_UNDEFINED){
            pre_id = h->fact_map.var[var].max;
        }else{
            pre_id = h->fact_map.var[var].val[pre_val];
        }

        if (pre_id < 0 || eff_id < 0){
            fprintf(stderr, "Error: Invalid variable IDs!\n");
            exit(-1);
        }

        planLPSetCoef(h->lp, row_id, eff_id, -1.);
        planLPSetCoef(h->lp, row_id, pre_id, 1.);
    }

    cost = op->cost;
    if (flags & PLAN_HEUR_OP_UNIT_COST){
        cost = 1.;
    }else if (flags & PLAN_HEUR_OP_COST_PLUS_ONE){
        cost = op->cost + 1.;
    }
    planLPSetRHS(h->lp, row_id, cost, 'L');
}

static void lpSetGoal(plan_heur_potential_t *h, int row_id,
                      const plan_part_state_t *goal)
{
    int var, val, col;

    for (var = 0; var < h->fact_map.var_size; ++var){
        val = planPartStateGet(goal, var);
        if (val == PLAN_VAL_UNDEFINED){
            col = h->fact_map.var[var].max;
        }else{
            col = h->fact_map.var[var].val[val];
        }
        planLPSetCoef(h->lp, row_id, col, 1.);
    }
    planLPSetRHS(h->lp, row_id, 0., 'L');
}

static void lpSetMaxPot(plan_heur_potential_t *h, int row_id,
                        int col_id, int maxpot_id)
{
    planLPSetCoef(h->lp, row_id, col_id, 1.);
    planLPSetCoef(h->lp, row_id, maxpot_id, -1.);
    planLPSetRHS(h->lp, row_id, 0., 'L');
}

static void lpInit(plan_heur_potential_t *h,
                   const plan_var_t *var, int var_size,
                   const plan_part_state_t *goal,
                   const plan_op_t *op, int op_size,
                   unsigned flags)
{
    unsigned lp_flags = 0u;
    int i, j, num_maxpot, row_id;
    const fact_var_map_t *fv;

    // Copy cplex-num-threads flags
    lp_flags |= (flags & (0x3fu << 8u));
    // Set maximalization
    lp_flags |= PLAN_LP_MAX;

    for (num_maxpot = 0, i = 0; i < h->fact_map.var_size; ++i){
        if (h->fact_map.var[i].max >= 0)
            num_maxpot += h->fact_map.var[i].range;
    }

    // Number of rows is one per operator + one for the goal + (pot <
    // maxpot) constraints.
    // Number of columns was detected before in fact-map.
    h->lp = planLPNew(op_size + 1 + num_maxpot, h->fact_map.size, lp_flags);

    // Set all variables as free
    for (i = 0; i < h->fact_map.size; ++i)
        planLPSetVarFree(h->lp, i);

    // Set operator constraints
    for (row_id = 0; row_id < op_size; ++row_id)
        lpSetOp(h, row_id, op + row_id, flags);

    // Set goal constraint
    lpSetGoal(h, row_id++, goal);

    // Set maxpot constraints
    for (i = 0; i < h->fact_map.var_size; ++i){
        fv = h->fact_map.var + i;
        if (fv->max < 0)
            continue;

        for (j = 0; j < fv->range; ++j)
            lpSetMaxPot(h, row_id++, fv->val[j], fv->max);
    }
}

static void lpSolve(plan_heur_potential_t *h,
                    const plan_state_t *state,
                    double *pot)
{
    int i, col;

    // First zeroize objective
    for (i = 0; i < h->fact_map.size; ++i)
        planLPSetObj(h->lp, i, 0.);

    // Then set simple objective
    for (i = 0; i < h->fact_map.var_size; ++i){
        if (h->fact_map.var[i].ma_privacy)
            continue;

        col = planStateGet(state, i);
        col = h->fact_map.var[i].val[col];
        planLPSetObj(h->lp, col, 1.);
    }

    planLPSolve(h->lp, pot);
}


static void factMapInit(fact_map_t *map,
                        const plan_var_t *var, int var_size,
                        const plan_part_state_t *goal,
                        const plan_op_t *op, int op_size,
                        unsigned flags)
{
    int i, j;
    plan_var_id_t op_var;

    map->size = 0;
    map->var = BOR_CALLOC_ARR(fact_var_map_t, var_size);
    map->var_size = var_size;
    for (i = 0; i < var_size; ++i){
        if (var[i].ma_privacy){
            map->var[i].ma_privacy = 1;
            continue;
        }

        map->var[i].val = BOR_ALLOC_ARR(int, var[i].range);
        map->var[i].range = var[i].range;
        for (j = 0; j < var[i].range; ++j)
            map->var[i].val[j] = map->size++;
        map->var[i].max = -1;
    }

    // Determine maxpot variables from the goal
    for (i = 0; i < var_size; ++i){
        if (!planPartStateIsSet(goal, i))
            map->var[i].max = map->size++;
    }

    // Determine maxpot variables from missing preconditions of operators
    for (i = 0; i < op_size; ++i){
        PLAN_PART_STATE_FOR_EACH_VAR(op[i].eff, j, op_var){
            if (map->var[op_var].max >= 0)
                continue;
            if (!planPartStateIsSet(op[i].pre, op_var))
                map->var[op_var].max = map->size++;
        }
    }
}

static void factMapFree(fact_map_t *map)
{
    int i;

    for (i = 0; i < map->var_size; ++i){
        if (map->var[i].val != NULL)
            BOR_FREE(map->var[i].val);
    }

    if (map->var != NULL)
        BOR_FREE(map->var);
}

#else /* PLAN_LP */

plan_heur_t *planHeurPotentialNew(const plan_var_t *var, int var_size,
                                  const plan_part_state_t *goal,
                                  const plan_op_t *op, int op_size,
                                  unsigned flags)
{
    fprintf(stderr, "Fatal Error: heur-potential needs some LP library!\n");
    return NULL;
}
#endif /* PLAN_LP */
