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

#ifdef PLAN_USE_CPLEX

struct _fact_var_map_t {
    int *val;
    int range;
    int max;
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
};
typedef struct _plan_heur_potential_t plan_heur_potential_t;
#define HEUR(parent) bor_container_of((parent), plan_heur_potential_t, heur)

static void heurPotentialDel(plan_heur_t *_heur);
static void heurPotential(plan_heur_t *_heur, const plan_state_t *state,
                          plan_heur_res_t *res);

plan_heur_t *planHeurPotentialNew(const plan_var_t *var, int var_size,
                                  const plan_part_state_t *goal,
                                  const plan_op_t *op, int op_size,
                                  unsigned flags)
{
    plan_heur_potential_t *heur;

    heur = BOR_ALLOC(plan_heur_potential_t);
    bzero(heur, sizeof(*heur));
    _planHeurInit(&heur->heur, heurPotentialDel, heurPotential, NULL);

    factMapInit(&heur->fact_map, var, var_size, goal, op, op_size, flags);

    return &heur->heur;
}

static void heurPotentialDel(plan_heur_t *_heur)
{
    plan_heur_potential_t *h = HEUR(_heur);

    factMapFree(&h->fact_map);
    _planHeurFree(&h->heur);
    BOR_FREE(h);
}

static void heurPotential(plan_heur_t *_heur, const plan_state_t *state,
                          plan_heur_res_t *res)
{
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

    fprintf(stderr, "FACT-MAP SIZE: %d\n", map->size);
    for (i = 0; i < map->var_size; ++i){
        for (j = 0; j < map->var[i].range; ++j)
            fprintf(stderr, "[%d] %d --> %d\n", i, j, map->var[i].val[j]);
        fprintf(stderr, "[%d] max --> %d\n", i, map->var[i].max);
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

#else /* PLAN_USE_CPLEX */
#warn "heur-potential needs CPLEX library."

plan_heur_t *planHeurPotentialNew(const plan_var_t *var, int var_size,
                                  const plan_part_state_t *goal,
                                  const plan_op_t *op, int op_size,
                                  unsigned flags)
{
    fprintf(stderr, "Fatal Error: heur-potential needs CPLEX!\n");
    exit(-1);
    return NULL;
}
#endif /* PLAN_USE_CPLEX */
