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
#include <boruvka/hfunc.h>
#include "plan/landmark.h"

/** Landmark record in hash table */
struct _ldm_t {
    plan_landmark_t ldm;
    int refcount;      /*!< Reference counter */
    bor_list_t htable; /*!< Connection to the hash table */
};
typedef struct _ldm_t ldm_t;
#define LDM(lst) BOR_LIST_ENTRY((lst), ldm_t, htable)

/** Landmark set */
struct _ldm_set_t {
    int size;
    ldm_t **ldm_pts;
    bor_list_t list; /*!< Connection to the list of landmarks */
};
typedef struct _ldm_set_t ldm_set_t;

static bor_htable_key_t ldmHash(const bor_list_t *k, void *ud);
static int ldmEq(const bor_list_t *k1, const bor_list_t *k2, void *ud);

static ldm_set_t *ldmSetNew(plan_landmark_set_t *l);
static void ldmSetDel(plan_landmark_cache_t *ldmc, ldm_set_t *ldms);
static ldm_t *ldmInsert(plan_landmark_cache_t *ldmc,
                        plan_landmark_t *ldm);
static void ldmDel(ldm_t *ldm);


void planLandmarkInit(plan_landmark_t *ldm, int size, const int *op_id)
{
    ldm->size = size;
    ldm->op_id = BOR_ALLOC_ARR(int, size);
    if (op_id != NULL)
        memcpy(ldm->op_id, op_id, sizeof(int) * size);
}

void planLandmarkFree(plan_landmark_t *ldm)
{
    if (ldm->op_id != NULL)
        BOR_FREE(ldm->op_id);
    bzero(ldm, sizeof(*ldm));
}

static int intCmp(const void *a, const void *b)
{
    int i = *(int *)a;
    int j = *(int *)b;
    return i - j;
}

void planLandmarkUnify(plan_landmark_t *ldm)
{
    int i, ins;

    qsort(ldm->op_id, ldm->size, sizeof(int), intCmp);
    for (ins = 1, i = 1; i < ldm->size; ++i){
        if (ldm->op_id[i] != ldm->op_id[i - 1]){
            ldm->op_id[ins++] = ldm->op_id[i];
        }
    }
    ldm->size = ins;
}

void planLandmarkSetInit(plan_landmark_set_t *ldms)
{
    bzero(ldms, sizeof(*ldms));
}

void planLandmarkSetFree(plan_landmark_set_t *ldms)
{
    int i;

    if (ldms->landmark){
        for (i = 0; i < ldms->size; ++i)
            planLandmarkFree(ldms->landmark + i);
        BOR_FREE(ldms->landmark);
    }
}

void planLandmarkSetAdd(plan_landmark_set_t *ldms, int size, int *op_id)
{
    plan_landmark_t *ldm;

    ++ldms->size;
    ldms->landmark = BOR_REALLOC_ARR(ldms->landmark, plan_landmark_t,
                                     ldms->size);
    ldm = ldms->landmark + ldms->size - 1;
    planLandmarkInit(ldm, size, op_id);
}



plan_landmark_cache_t *planLandmarkCacheNew(void)
{
    plan_landmark_cache_t *ldmc;

    ldmc = BOR_ALLOC(plan_landmark_cache_t);
    ldmc->ldm_table = borHTableNew(ldmHash, ldmEq, NULL);
    borListInit(&ldmc->ldms);

    planLandmarkSetInit(&ldmc->ldms_out);
    ldmc->ldms_alloc = 0;

    return ldmc;
}

void planLandmarkCacheDel(plan_landmark_cache_t *ldmc)
{
    bor_list_t *el, *tmp;
    ldm_set_t *ldms;

    BOR_LIST_FOR_EACH_SAFE(&ldmc->ldms, el, tmp){
        ldms = BOR_LIST_ENTRY(el, ldm_set_t, list);
        ldmSetDel(ldmc, ldms);
    }

    borHTableDel(ldmc->ldm_table);
    if (ldmc->ldms_out.landmark)
        BOR_FREE(ldmc->ldms_out.landmark);
    BOR_FREE(ldmc);
}

plan_landmark_set_id_t planLandmarkCacheAdd(plan_landmark_cache_t *ldmc,
                                            plan_landmark_set_t *ldms_in)
{
    ldm_set_t *ldms;
    ldm_t *ldm;
    int i;

    ldms = ldmSetNew(ldms_in);
    for (i = 0; i < ldms->size; ++i){
        ldm = ldmInsert(ldmc, ldms_in->landmark + i);
        ldms->ldm_pts[i] = ldm;
    }
    borListAppend(&ldmc->ldms, &ldms->list);

    BOR_FREE(ldms_in->landmark);
    bzero(ldms_in, sizeof(*ldms_in));
    return ldms;
}

const plan_landmark_set_t *planLandmarkCacheGet(plan_landmark_cache_t *ldmc,
                                                plan_landmark_set_id_t ldmid)
{
    ldm_set_t *ldms = (ldm_set_t *)ldmid;
    plan_landmark_set_t *out = &ldmc->ldms_out;
    int i;

    if (ldms->size > ldmc->ldms_alloc){
        ldmc->ldms_alloc = ldms->size;
        out->landmark = BOR_REALLOC_ARR(out->landmark, plan_landmark_t,
                                        ldmc->ldms_alloc);
    }

    out->size = ldms->size;
    for (i = 0; i < ldms->size; ++i)
        out->landmark[i] = ldms->ldm_pts[i]->ldm;
    return out;
}

static bor_htable_key_t ldmHash(const bor_list_t *k, void *ud)
{
    ldm_t *ldm = LDM(k);
    return borCityHash_64(ldm->ldm.op_id, sizeof(int) * ldm->ldm.size);
}

static int ldmEq(const bor_list_t *k1, const bor_list_t *k2, void *ud)
{
    ldm_t *l1 = LDM(k1);
    ldm_t *l2 = LDM(k2);

    if (l1->ldm.size != l2->ldm.size)
        return 0;
    return !memcmp(l1->ldm.op_id, l2->ldm.op_id, sizeof(int) * l1->ldm.size);
}

static ldm_set_t *ldmSetNew(plan_landmark_set_t *l)
{
    ldm_set_t *ldms;

    ldms = BOR_ALLOC(ldm_set_t);
    ldms->size = l->size;
    ldms->ldm_pts = BOR_ALLOC_ARR(ldm_t *, ldms->size);
    borListInit(&ldms->list);
    return ldms;
}

static void ldmSetDel(plan_landmark_cache_t *ldmc, ldm_set_t *ldms)
{
    int i;

    for (i = 0; i < ldms->size; ++i){
        if (--ldms->ldm_pts[i]->refcount == 0){
            borHTableErase(ldmc->ldm_table, &ldms->ldm_pts[i]->htable);
            ldmDel(ldms->ldm_pts[i]);
        }
    }
    BOR_FREE(ldms->ldm_pts);
    BOR_FREE(ldms);
}

static ldm_t *ldmInsert(plan_landmark_cache_t *ldmc,
                        plan_landmark_t *ldm)
{
    ldm_t *l;
    bor_list_t *ins;

    planLandmarkUnify(ldm);

    l = BOR_ALLOC(ldm_t);
    l->ldm = *ldm;
    l->refcount = 1;

    ins = borHTableInsertUnique(ldmc->ldm_table, &l->htable);
    if (ins != NULL){
        BOR_FREE(l);
        planLandmarkFree(ldm);
        l = LDM(ins);
        ++l->refcount;
    }

    return l;
}

static void ldmDel(ldm_t *ldm)
{
    planLandmarkFree(&ldm->ldm);
    BOR_FREE(ldm);
}
