#include <boruvka/alloc.h>
#include "plan/heur.h"

struct _plan_heur_ma_dtg_t {
    plan_heur_t heur;
};
typedef struct _plan_heur_ma_dtg_t plan_heur_ma_dtg_t;
#define HEUR(parent) \
    bor_container_of((parent), plan_heur_ma_dtg_t, heur)

static void hdtgDel(plan_heur_t *heur);
static int hdtgHeur(plan_heur_t *heur, plan_ma_comm_t *comm,
                    const plan_state_t *state, plan_heur_res_t *res);
static int hdtgUpdate(plan_heur_t *heur, plan_ma_comm_t *comm,
                      const plan_ma_msg_t *msg, plan_heur_res_t *res);
static void hdtgRequest(plan_heur_t *heur, plan_ma_comm_t *comm,
                        const plan_ma_msg_t *msg);

plan_heur_t *planHeurMADTGNew(const plan_problem_t *agent_def)
{
    plan_heur_ma_dtg_t *hdtg;

    hdtg = BOR_ALLOC(plan_heur_ma_dtg_t);
    _planHeurInit(&hdtg->heur, hdtgDel, NULL);
    _planHeurMAInit(&hdtg->heur, hdtgHeur, hdtgUpdate, hdtgRequest);

    return &hdtg->heur;
}

static void hdtgDel(plan_heur_t *heur)
{
    plan_heur_ma_dtg_t *hdtg = HEUR(heur);

    _planHeurFree(&hdtg->heur);
    BOR_FREE(hdtg);
}

static int hdtgHeur(plan_heur_t *heur, plan_ma_comm_t *comm,
                    const plan_state_t *state, plan_heur_res_t *res)
{
    // TODO
    return 0;
}

static int hdtgUpdate(plan_heur_t *heur, plan_ma_comm_t *comm,
                      const plan_ma_msg_t *msg, plan_heur_res_t *res)
{
    // TODO
    return 0;
}

static void hdtgRequest(plan_heur_t *heur, plan_ma_comm_t *comm,
                        const plan_ma_msg_t *msg)
{
    // TODO
}
