#include "plan/heur.h"

void _planHeurInit(plan_heur_t *heur,
                   plan_heur_del_fn del_fn,
                   plan_heur_fn heur_fn)
{
    bzero(heur, sizeof(*heur));

    heur->del_fn  = del_fn;
    heur->heur_fn = heur_fn;
}

void _planHeurMAInit(plan_heur_t *heur,
                     plan_heur_ma_fn heur_ma_fn,
                     plan_heur_ma_update_fn heur_ma_update_fn,
                     plan_heur_ma_request_fn heur_ma_request_fn)
{
    heur->heur_ma_fn         = heur_ma_fn;
    heur->heur_ma_update_fn  = heur_ma_update_fn;
    heur->heur_ma_request_fn = heur_ma_request_fn;
}

void _planHeurFree(plan_heur_t *heur)
{
}

void planHeurDel(plan_heur_t *heur)
{
    heur->del_fn(heur);
}

void planHeur(plan_heur_t *heur, const plan_state_t *state,
              plan_heur_res_t *res)
{
    res->pref_size = 0;
    heur->heur_fn(heur, state, res);
}

long planHeurMA(plan_heur_t *heur,
                plan_ma_comm_queue_t *comm,
                const plan_state_t *state,
                plan_heur_res_t *res)
{
    if (heur->heur_ma_fn == NULL){
        fprintf(stderr, "Heur Error: planHeurMA() is not defined for"
                        " this heuristic!\n");
        exit(-1);
    }

    return heur->heur_ma_fn(heur, comm, state, res);
}

int planHeurMAUpdate(plan_heur_t *heur,
                     plan_ma_comm_queue_t *comm,
                     const plan_ma_msg_t *msg,
                     plan_heur_res_t *res)
{
    if (heur->heur_ma_update_fn == NULL){
        fprintf(stderr, "Heur Error: planHeurMAUpdate() is not defined for"
                        " this heuristic!\n");
        exit(-1);
    }

    return heur->heur_ma_update_fn(heur, comm, msg, res);
}

void planHeurMARequest(plan_heur_t *heur,
                       plan_ma_comm_queue_t *comm,
                       const plan_ma_msg_t *msg)
{
    if (heur->heur_ma_request_fn == NULL){
        fprintf(stderr, "Heur Error: planHeurMARequest() is not defined for"
                        " this heuristic!\n");
        exit(-1);
    }

    heur->heur_ma_request_fn(heur, comm, msg);
}
