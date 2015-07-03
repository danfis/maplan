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
#include "plan/pddl_lift_action.h"

static int planPDDLLiftActionPreCmp(const void *a, const void *b)
{
    const plan_pddl_fact_t *f1 = a;
    const plan_pddl_fact_t *f2 = b;
    int cmp, i;

    cmp = f1->arg_size - f2->arg_size;
    if (cmp == 0)
        cmp = f1->pred - f2->pred;
    for (i = 0; cmp == 0 && i < f1->arg_size; ++i)
        cmp = f1->arg[i] - f2->arg[i];
    return cmp;
}

static void planPDDLLiftActionPrepare(plan_pddl_lift_action_t *a,
                              const plan_pddl_type_obj_t *type_obj,
                              int obj_size)
{
    int i, j, var, *objs;

    qsort(a->pre.fact, a->pre.size, sizeof(plan_pddl_fact_t),
          planPDDLLiftActionPreCmp);

    a->allowed_arg = BOR_ALLOC_ARR(int *, a->action->param.size);
    for (i = 0; i < a->action->param.size; ++i){
        objs = BOR_CALLOC_ARR(int, obj_size);
        var = a->action->param.obj[i].type;
        for (j = 0; j < type_obj->map_size[var]; ++j)
            objs[type_obj->map[var][j]] = 1;
        a->allowed_arg[i] = objs;
    }
}

void planPDDLLiftActionsInit(plan_pddl_lift_actions_t *action,
                             const plan_pddl_actions_t *pddl_action,
                             const plan_pddl_type_obj_t *type_obj,
                             const plan_pddl_facts_t *func,
                             int obj_size, int eq_fact_id)
{
    plan_pddl_lift_action_t *a;
    const plan_pddl_action_t *pddl_a;
    plan_pddl_fact_t *f;
    const plan_pddl_fact_t *pddl_f;
    int i, prei;

    action->size = pddl_action->size;
    action->action = BOR_CALLOC_ARR(plan_pddl_lift_action_t, action->size);

    for (i = 0; i < action->size; ++i){
        a = action->action + i;
        a->action_id = i;

        pddl_a = pddl_action->action + i;
        a->action = pddl_a;
        a->param_size = pddl_a->param.size;
        a->obj_size = obj_size;
        a->func = func;

        for (prei = 0; prei < pddl_a->pre.size; ++prei){
            pddl_f = pddl_a->pre.fact + prei;
            if (pddl_f->pred == eq_fact_id){
                f = planPDDLFactsAdd(&a->eq);
            }else{
                if (pddl_f->neg){
                    f = planPDDLFactsAdd(&a->pre_neg);
                    planPDDLFactCopy(f, pddl_f);
                }
                f = planPDDLFactsAdd(&a->pre);
            }
            planPDDLFactCopy(f, pddl_f);

        }

        planPDDLLiftActionPrepare(a, type_obj, obj_size);
    }
}

void planPDDLLiftActionsFree(plan_pddl_lift_actions_t *as)
{
    int i, j;

    for (i = 0; i < as->size; ++i){
        planPDDLFactsFree(&as->action[i].pre);
        planPDDLFactsFree(&as->action[i].pre_neg);
        planPDDLFactsFree(&as->action[i].eq);
        if (as->action[i].allowed_arg != NULL){
            for (j = 0; j < as->action[i].action->param.size; ++j){
                if (as->action[i].allowed_arg[j] != NULL)
                    BOR_FREE(as->action[i].allowed_arg[j]);
            }
            BOR_FREE(as->action[i].allowed_arg);
        }
    }


    if (as->action != NULL)
        BOR_FREE(as->action);
}

static void _planPDDLLiftActionFactsPrint(const plan_pddl_lift_action_t *a,
                                          const plan_pddl_facts_t *fs,
                                          const plan_pddl_predicates_t *pred,
                                          const plan_pddl_objs_t *obj,
                                          const char *header,
                                          FILE *fout)
{
    int i, j;

    fprintf(fout, "    %s[%d]:\n", header, fs->size);
    for (i = 0; i < fs->size; ++i){
        fprintf(fout, "      ");
        if (fs->fact[i].neg)
            fprintf(fout, "N:");
        fprintf(fout, "%s:", pred->pred[fs->fact[i].pred].name);
        for (j = 0; j < fs->fact[i].arg_size; ++j){
            if (fs->fact[i].arg[j] < 0){
                fprintf(fout, " %s", obj->obj[fs->fact[i].arg[j] + obj->size].name);
            }else{
                fprintf(fout, " %s",
                        a->action->param.obj[fs->fact[i].arg[j]].name);
            }
        }
        fprintf(fout, "\n");
    }
}

void planPDDLLiftActionsPrint(const plan_pddl_lift_actions_t *a,
                              const plan_pddl_predicates_t *pred,
                              const plan_pddl_objs_t *obj,
                              FILE *fout)
{
    const plan_pddl_lift_action_t *lift;
    int i;

    fprintf(fout, "Lift Actions[%d]:\n", a->size);
    for (i = 0; i < a->size; ++i){
        lift = a->action + i;
        fprintf(fout, "  %s:\n", lift->action->name);
        _planPDDLLiftActionFactsPrint(lift, &lift->pre, pred, obj, "pre", fout);
        _planPDDLLiftActionFactsPrint(lift, &lift->pre_neg, pred, obj, "pre-neg", fout);
        _planPDDLLiftActionFactsPrint(lift, &lift->eq, pred, obj, "eq", fout);
    }
}

int planPDDLLiftActionFirstAllowedObj(const plan_pddl_lift_action_t *lift_action, int arg_i)
{
    int i;
    int obj_size = lift_action->obj_size;

    for (i = 0; i < obj_size; ++i){
        if (lift_action->allowed_arg[arg_i][i])
            return i;
    }

    return -1;
}
