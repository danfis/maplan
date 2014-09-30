#include "plan/heur.h"

#include "_heur_relax.c"

struct _plan_heur_ma_max_t {
    plan_heur_t heur;
    plan_heur_relax_t relax;
};
typedef struct _plan_heur_ma_max_t plan_heur_ma_max_t;

#define HEUR(parent) \
    bor_container_of(parent, plan_heur_ma_max_t, heur)

static void heurDel(plan_heur_t *_heur);
static int heurMAMax(plan_heur_t *heur, plan_ma_comm_t *comm,
                     const plan_state_t *state, plan_heur_res_t *res);
static int heurMAMaxUpdate(plan_heur_t *heur, plan_ma_comm_t *comm,
                           const plan_ma_msg_t *msg, plan_heur_res_t *res);
static void heurMAMaxRequest(plan_heur_t *heur, plan_ma_comm_t *comm,
                             const plan_ma_msg_t *msg);

plan_heur_t *planHeurMARelaxMaxNew(const plan_problem_t *prob)
{
    plan_heur_ma_max_t *heur;

    heur = BOR_ALLOC(plan_heur_ma_max_t);
    _planHeurInit(&heur->heur, heurDel, NULL);
    _planHeurMAInit(&heur->heur, heurMAMax,
                    heurMAMaxUpdate, heurMAMaxRequest);

    planHeurRelaxInit(&heur->relax, TYPE_MAX,
                      prob->var, prob->var_size,
                      prob->goal,
                      prob->proj_op, prob->proj_op_size,
                      NULL);

    return &heur->heur;
}

static void heurDel(plan_heur_t *_heur)
{
    plan_heur_ma_max_t *heur = HEUR(_heur);

    _planHeurFree(&heur->heur);
    planHeurRelaxFree(&heur->relax);
    BOR_FREE(heur);
}

static int heurMAMax(plan_heur_t *heur, plan_ma_comm_t *comm,
                     const plan_state_t *state, plan_heur_res_t *res)
{
    res->heur = PLAN_HEUR_DEAD_END;
    return 0;
}

static int heurMAMaxUpdate(plan_heur_t *heur, plan_ma_comm_t *comm,
                           const plan_ma_msg_t *msg, plan_heur_res_t *res)
{
    return 0;
}

static void heurMAMaxRequest(plan_heur_t *heur, plan_ma_comm_t *comm,
                             const plan_ma_msg_t *msg)
{
}
