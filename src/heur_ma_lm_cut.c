#include <boruvka/alloc.h>
#include <boruvka/lifo.h>

#include "plan/heur.h"
#include "heur_relax.h"
#include "op_id_tr.h"

struct _plan_heur_ma_lm_cut_t {
    plan_heur_t heur;
    plan_heur_relax_t relax;
    plan_op_id_tr_t op_id_tr;
};
typedef struct _plan_heur_ma_lm_cut_t plan_heur_ma_lm_cut_t;

#define HEUR(parent) \
    bor_container_of(parent, plan_heur_ma_lm_cut_t, heur)

static void heurDel(plan_heur_t *_heur);
static int heurMA(plan_heur_t *heur, plan_ma_comm_t *comm,
                  const plan_state_t *state, plan_heur_res_t *res);
static int heurMAUpdate(plan_heur_t *heur, plan_ma_comm_t *comm,
                        const plan_ma_msg_t *msg, plan_heur_res_t *res);
static void heurMARequest(plan_heur_t *heur, plan_ma_comm_t *comm,
                          const plan_ma_msg_t *msg);

plan_heur_t *planHeurMALMCutNew(const plan_problem_t *prob)
{
    plan_heur_ma_lm_cut_t *heur;

    heur = BOR_ALLOC(plan_heur_ma_lm_cut_t);
    _planHeurInit(&heur->heur, heurDel, NULL);
    _planHeurMAInit(&heur->heur, heurMA, heurMAUpdate, heurMARequest);

    return &heur->heur;
}

static void heurDel(plan_heur_t *_heur)
{
}

static int heurMA(plan_heur_t *heur, plan_ma_comm_t *comm,
                  const plan_state_t *state, plan_heur_res_t *res)
{
    return 0;
}

static int heurMAUpdate(plan_heur_t *heur, plan_ma_comm_t *comm,
                        const plan_ma_msg_t *msg, plan_heur_res_t *res)
{
    return 0;
}

static void heurMARequest(plan_heur_t *heur, plan_ma_comm_t *comm,
                          const plan_ma_msg_t *msg)
{
}
