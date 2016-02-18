#include <cu/cu.h>
#include <boruvka/alloc.h>
#include "plan/pddl_inv.h"

struct _inv_plist_t {
    char **inv;
    int size;
};
typedef struct _inv_plist_t inv_plist_t;


static int strCmp(const void *a, const void *b)
{
    const char *s1 = *(char **)a;
    const char *s2 = *(char **)b;
    return strcmp(s1, s2);
}

static int invCmp(const void *a, const void *b)
{
    const inv_plist_t *p1 = a;
    const inv_plist_t *p2 = b;
    int i, j, cmp;

    for (i = 0, j = 0; i < p1->size && j < p2->size; ++i, ++j){
        cmp = strcmp(p1->inv[i], p2->inv[j]);
        if (cmp != 0)
            return cmp;
    }
    return p1->size - p2->size;
}

static void printInvs(bor_list_t *invs, int size, const plan_pddl_ground_t *g)
{
    const plan_pddl_inv_t *inv;
    const plan_pddl_fact_t *fact;
    inv_plist_t *is;
    char fs[1024];
    int i, j, k, ins;

    is = BOR_CALLOC_ARR(inv_plist_t, size);
    i = 0;
    BOR_LIST_FOR_EACH_ENTRY(invs, const plan_pddl_inv_t, inv, list){
        is[i].inv = BOR_ALLOC_ARR(char *, inv->size);
        is[i].size = inv->size;

        for (j = 0; j < inv->size; ++j){
            fact = planPDDLFactPoolGet(&g->fact_pool, inv->fact[j]);
            ins = 0;
            if (fact->neg)
                ins = sprintf(fs, "N:");
            ins += sprintf(fs + ins, "%s:", g->pddl->predicate.pred[fact->pred].name);
            for (k = 0; k < fact->arg_size; ++k)
                ins += sprintf(fs + ins, " %s", g->pddl->obj.obj[fact->arg[k]].name);
            is[i].inv[j] = strdup(fs);
        }
        qsort(is[i].inv, is[i].size, sizeof(char *), strCmp);

        ++i;
    }
    qsort(is, size, sizeof(inv_plist_t), invCmp);

    for (i = 0; i < size; ++i){
        printf("Inv %d:\n", i);
        for (j = 0; j < is[i].size; ++j)
            printf("    %s\n", is[i].inv[j]);
    }

    for (i = 0; i < size; ++i){
        for (j = 0; j < is[i].size; ++j)
            free(is[i].inv[j]);
        BOR_FREE(is[i].inv);
    }
    BOR_FREE(is);
}

static void inferInvs(const char *domain_fn, const char *problem_fn)
{
    plan_pddl_t *d;
    plan_pddl_ground_t ground;
    bor_list_t inv;
    int inv_size;

    printf("---- Invariants %s | %s ----\n", domain_fn, problem_fn);
    d = planPDDLNew(domain_fn, problem_fn);
    if (d == NULL)
        return;

    planPDDLGroundInit(&ground, d);
    planPDDLGround(&ground);

    borListInit(&inv);
    inv_size = planPDDLInvFind(&ground, &inv);
    if (inv_size > 0)
        printInvs(&inv, inv_size, &ground);
    planPDDLInvFreeList(&inv);

    //planPDDLGroundPrint(&ground, stdout);
    planPDDLGroundFree(&ground);
    planPDDLDel(d);
    printf("---- Invariants %s | %s END ----\n", domain_fn, problem_fn);
}

TEST(testPDDLInv)
{
    inferInvs("pddl/depot-domain.pddl", "pddl/depot-pfile1.pddl");
    inferInvs("pddl/depot-domain.pddl", "pddl/depot-pfile2.pddl");
    inferInvs("pddl/depot-domain.pddl", "pddl/depot-pfile5.pddl");
    inferInvs("pddl/driverlog-domain.pddl", "pddl/driverlog-pfile1.pddl");
    inferInvs("pddl/openstacks-p03-domain.pddl", "pddl/openstacks-p03.pddl");
    inferInvs("pddl/rovers-domain.pddl", "pddl/rovers-p01.pddl");
    inferInvs("pddl/elevators08-domain.pddl", "pddl/elevators08-p01.pddl");
    inferInvs("pddl/CityCar-domain.pddl", "pddl/CityCar-p3-2-2-0-1.pddl");
}
