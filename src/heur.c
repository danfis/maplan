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
#include "plan/heur.h"
#include "plan/search.h"

void _planHeurInit(plan_heur_t *heur,
                   plan_heur_del_fn del_fn,
                   plan_heur_state_fn heur_state_fn,
                   plan_heur_node_fn heur_node_fn)
{
    bzero(heur, sizeof(*heur));

    heur->del_fn = del_fn;
    heur->heur_state_fn = heur_state_fn;
    heur->heur_node_fn = heur_node_fn;
    heur->ma = 0;
}

void _planHeurMAInit(plan_heur_t *heur,
                     plan_heur_ma_state_fn heur_ma_state_fn,
                     plan_heur_ma_node_fn heur_ma_node_fn,
                     plan_heur_ma_update_fn heur_ma_update_fn,
                     plan_heur_ma_request_fn heur_ma_request_fn)
{
    heur->heur_ma_state_fn   = heur_ma_state_fn;
    heur->heur_ma_node_fn    = heur_ma_node_fn;
    heur->heur_ma_update_fn  = heur_ma_update_fn;
    heur->heur_ma_request_fn = heur_ma_request_fn;
    heur->ma = 1;
}

void _planHeurFree(plan_heur_t *heur)
{
}

void planHeurDel(plan_heur_t *heur)
{
    heur->del_fn(heur);
}

void planHeurNode(plan_heur_t *heur, plan_state_id_t state_id,
                  plan_search_t *search, plan_heur_res_t *res)
{
    res->pref_size = 0;
    if (heur->heur_node_fn){
        heur->heur_node_fn(heur, state_id, search, res);
    }else{
        planHeurState(heur, planSearchLoadState(search, state_id), res);
    }
}

void planHeurState(plan_heur_t *heur, const plan_state_t *state,
                   plan_heur_res_t *res)
{
    res->pref_size = 0;
    heur->heur_state_fn(heur, state, res);
}

void planHeurMAInit(plan_heur_t *heur, int agent_size, int agent_id,
                    plan_ma_state_t *ma_state)
{
    heur->ma_agent_size = agent_size;
    heur->ma_agent_id = agent_id;
    heur->ma_state = ma_state;
}

int planHeurMANode(plan_heur_t *heur,
                   plan_ma_comm_t *comm,
                   plan_state_id_t state_id,
                   plan_search_t *search,
                   plan_heur_res_t *res)
{
    if (heur->heur_ma_node_fn != NULL){
        return heur->heur_ma_node_fn(heur, comm, state_id, search, res);
    }else{
        return planHeurMAState(heur, comm,
                               planSearchLoadState(search, state_id), res);
    }
}

int planHeurMAState(plan_heur_t *heur,
                    plan_ma_comm_t *comm,
                    const plan_state_t *state,
                    plan_heur_res_t *res)
{
    if (heur->heur_ma_state_fn == NULL){
        fprintf(stderr, "Heur Error: planHeurMAState() is not defined for"
                        " this heuristic!\n");
        exit(-1);
    }

    return heur->heur_ma_state_fn(heur, comm, state, res);
}

int planHeurMAUpdate(plan_heur_t *heur,
                     plan_ma_comm_t *comm,
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
                       plan_ma_comm_t *comm,
                       const plan_ma_msg_t *msg)
{
    if (heur->heur_ma_request_fn == NULL){
        fprintf(stderr, "Heur Error: planHeurMARequest() is not defined for"
                        " this heuristic!\n");
        exit(-1);
    }

    heur->heur_ma_request_fn(heur, comm, msg);
}
