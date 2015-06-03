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
    bor_list_t ldms; /*!< Landmark sets */
};
typedef struct _plan_landmark_cache_t plan_landmark_cache_t;

typedef void * plan_landmark_set_id_t;

/**
 * Creates an empty landmark cache.
 */
plan_landmark_cache_t *planLandmarkCacheNew(void);

/**
 * Frees allocated resouces.
 */
void planLandmarkCacheDel(plan_landmark_cache_t *);

/**
 * Adds a landmark set to the cache.
 * The cache "consumes" the content of the given landmark set, so the
 * caller should not use the landmark set again (it is zeroized anyway).
 */
plan_landmark_set_id_t planLandmarkCacheAdd(plan_landmark_cache_t *ldmc,
                                            plan_landmark_set_t *ldms);

/**
 * Returns a landmark set that corresponds to the given ID or NULL of not
 * such landmark set is stored in the cache.
 * The returned landmark set points to the internal structure of the cache
 * so it should not be changed in any way.
 */
const plan_landmark_set_t *planLandmarkCacheGet(plan_landmark_cache_t *ldmc,
                                                plan_landmark_set_id_t ldmid);

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* __PLAN_LANDMARK_H__ */
