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

#ifndef __PLAN_LANDMARK_H__
#define __PLAN_LANDMARK_H__

#include <boruvka/htable.h>
#include <boruvka/splaytree_int.h>
#include <plan/common.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * Structure holding one landmark.
 */
struct _plan_landmark_t {
    int size;   /*!< Number of operators in the landmark */
    int *op_id; /*!< Array of operators' IDs */
};
typedef struct _plan_landmark_t plan_landmark_t;

/**
 * Initializes a landmark
 */
void planLandmarkInit(plan_landmark_t *ldm, int size, const int *op_id);

/**
 * Frees a landmark
 */
void planLandmarkFree(plan_landmark_t *ldm);

/**
 * Unifies the given landmark so that op IDs are sorted and duplicates are
 * removed.
 */
void planLandmarkUnify(plan_landmark_t *ldm);


/**
 * Set of landmarks.
 */
struct _plan_landmark_set_t {
    int size; /*!< Number of landmarks in the set */
    plan_landmark_t *landmark; /*!< Array of landmarks */
};
typedef struct _plan_landmark_set_t plan_landmark_set_t;

/**
 * Initializes an empty landmark set.
 */
void planLandmarkSetInit(plan_landmark_set_t *ldms);

/**
 * Frees resources
 */
void planLandmarkSetFree(plan_landmark_set_t *ldms);

/**
 * Adds one landmark to the set.
 */
void planLandmarkSetAdd(plan_landmark_set_t *ldms, int size, int *op_id);

/**
 * Cache for landmark sets.
 */
struct _plan_landmark_cache_t {
    bor_htable_t *ldm_table; /*!< Hash table of landmarks */
    bor_splaytree_int_t *ldms; /*!< Landmark sets */
    bor_list_t prune; /*!< List of landmarks ready to be pruned */
    int prune_enable; /*!< True if cache should be pruned */

    plan_landmark_set_t ldms_out; /*!< Set used for *Get() method */
    int ldms_alloc; /*!< Size of allocated space in .ldms_out */
};
typedef struct _plan_landmark_cache_t plan_landmark_cache_t;

/**
 * Flag that enables pruning of landmark cache.
 */
#define PLAN_LANDMARK_CACHE_PRUNE 0x1

/**
 * Creates an empty landmark cache.
 */
plan_landmark_cache_t *planLandmarkCacheNew(unsigned flags);

/**
 * Frees allocated resouces.
 */
void planLandmarkCacheDel(plan_landmark_cache_t *);

/**
 * Adds a landmark set to the cache under the specified ID.
 * The cache "consumes" the content of the given landmark set, so the
 * caller should not use the landmark set again (it is zeroized anyway).
 * Returns 0 on success, -1 if ID is already in cache and in that case
 * {ldms} is not touched.
 */
int planLandmarkCacheAdd(plan_landmark_cache_t *ldmc,
                         int id, plan_landmark_set_t *ldms);

/**
 * Returns a landmark set that corresponds to the given ID or NULL of not
 * such landmark set is stored in the cache.
 * The returned landmark set points to an internal structure of the cache
 * so it should not be changed in any way.
 */
const plan_landmark_set_t *planLandmarkCacheGet(plan_landmark_cache_t *ldmc,
                                                int ldmid);

/**
 * Prune old used landmarks from cache.
 * Returns number of deleted landmarks.
 * Note 1: Pruning must be enabled (see PLAN_LANDMARK_CACHE_PRUNE).
 * Note 2: Landmarks are automatically pruned whenever a new landmark is
 * added, so usualy there is no need to call this function directly.
 */
int planLandmarkPrune(plan_landmark_cache_t *ldmc);

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* __PLAN_LANDMARK_H__ */
