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

#ifndef __PLAN_PDDL_FACT_POOL_H__
#define __PLAN_PDDL_FACT_POOL_H__

#include <boruvka/extarr.h>
#include <boruvka/htable.h>
#include <plan/pddl.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct _plan_pddl_fact_pool_t {
    bor_htable_t *htable;     /*!< Hash table for uniqueness */
    bor_extarr_t *fact;       /*!< Array of facts */
    int size;                 /*!< Overall number of facts in pool */
    bor_extarr_t **pred_fact; /*!< Facts divided by predicate */
    int *pred_fact_size;      /*!< Number of facts of respective predicate */
    int pred_size;            /*!< Number of predicates */
};
typedef struct _plan_pddl_fact_pool_t plan_pddl_fact_pool_t;

/**
 * Initializes a pool of facts, the number of predicates must be provided.
 */
void planPDDLFactPoolInit(plan_pddl_fact_pool_t *fact_pool, int pred_size);

/**
 * Frees allocated resources.
 */
void planPDDLFactPoolFree(plan_pddl_fact_pool_t *fact_pool);

/**
 * Adds a fact to the pool.
 * If successfully inserted an 0 is returned, if fact was already in the
 * pool -1 is returned.
 */
int planPDDLFactPoolAdd(plan_pddl_fact_pool_t *pool,
                        const plan_pddl_fact_t *f);

/**
 * Adds all facts from the list into pool.
 */
_bor_inline void planPDDLFactPoolAddFacts(plan_pddl_fact_pool_t *pool,
                                          const plan_pddl_facts_t *facts);

/**
 * Returns true if the fact already inserted in the pool.
 */
int planPDDLFactPoolExist(plan_pddl_fact_pool_t *pool,
                          const plan_pddl_fact_t *f);

/**
 * Returns true if all facts are in the pool.
 */
_bor_inline int planPDDLFactPoolExistFacts(plan_pddl_fact_pool_t *pool,
                                           const plan_pddl_facts_t *fs);

/**
 * Returns i'th fact stored in the pool.
 */
plan_pddl_fact_t *planPDDLFactPoolGet(const plan_pddl_fact_pool_t *pool, int i);

/**
 * Returns number of facts that are instances of the specified predicate.
 */
_bor_inline int planPDDLFactPoolPredSize(const plan_pddl_fact_pool_t *pool,
                                         int pred);

/**
 * Returns i'th fact of specified predicate.
 */
plan_pddl_fact_t *planPDDLFactPoolGetPred(const plan_pddl_fact_pool_t *pool,
                                          int pred, int i);


/**** INLINES ****/
_bor_inline void planPDDLFactPoolAddFacts(plan_pddl_fact_pool_t *pool,
                                          const plan_pddl_facts_t *facts)
{
    int i;

    for (i = 0; i < facts->size; ++i)
        planPDDLFactPoolAdd(pool, facts->fact + i);
}

_bor_inline int planPDDLFactPoolExistFacts(plan_pddl_fact_pool_t *pool,
                                           const plan_pddl_facts_t *fs)
{
    int i;

    for (i = 0; i < fs->size; ++i){
        if (!planPDDLFactPoolExist(pool, fs->fact + i))
            return 0;
    }
    return 1;
}

_bor_inline int planPDDLFactPoolPredSize(const plan_pddl_fact_pool_t *pool,
                                         int pred)
{
    return pool->pred_fact_size[pred];
}

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* __PLAN_PDDL_FACT_POOL_H__ */
