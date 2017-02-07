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

#include <string.h>

#include <boruvka/alloc.h>

#include "plan/problem.h"
#include "plan/fact_id.h"


void planProblemInit(plan_problem_t *p)
{
    bzero(p, sizeof(*p));
    p->ma_privacy_var = -1;
}

void planProblemDel(plan_problem_t *plan)
{
    planProblemFree(plan);
    BOR_FREE(plan);
}

static void freeOps(plan_op_t *ops, int ops_size)
{
    int i;

    for (i = 0; i < ops_size; ++i){
        planOpFree(ops + i);
    }
    BOR_FREE(ops);
}

void planProblemFree(plan_problem_t *plan)
{
    int i;

    if (plan->succ_gen)
        planSuccGenDel(plan->succ_gen);

    if (plan->goal){
        planPartStateDel(plan->goal);
    }

    if (plan->var != NULL){
        for (i = 0; i < plan->var_size; ++i){
            planVarFree(plan->var + i);
        }
        BOR_FREE(plan->var);
    }

    if (plan->state_pool)
        planStatePoolDel(plan->state_pool);

    if (plan->op)
        freeOps(plan->op, plan->op_size);

    if (plan->agent_name)
        BOR_FREE(plan->agent_name);

    if (plan->proj_op)
        freeOps(plan->proj_op, plan->proj_op_size);

    if (plan->private_val)
        BOR_FREE(plan->private_val);

    bzero(plan, sizeof(*plan));
}

plan_problem_t *planProblemClone(const plan_problem_t *p)
{
    plan_problem_t *prob;

    prob = BOR_ALLOC(plan_problem_t);
    planProblemCopy(prob, p);
    return prob;
}

void planProblemCopy(plan_problem_t *dst, const plan_problem_t *src)
{
    int i;

    memcpy(dst, src, sizeof(*src));

    dst->var = BOR_ALLOC_ARR(plan_var_t, src->var_size);
    for (i = 0; i < src->var_size; ++i)
        planVarCopy(dst->var + i, src->var + i);
    dst->state_pool = planStatePoolClone(src->state_pool);
    dst->goal = planPartStateClone(src->goal);

    dst->op = BOR_ALLOC_ARR(plan_op_t, src->op_size);
    for (i = 0; i < src->op_size; ++i){
        planOpInit(dst->op + i, src->var_size);
        planOpCopy(dst->op + i, src->op + i);
    }

    if (src->succ_gen)
        dst->succ_gen = planSuccGenNew(dst->op, dst->op_size,
                                       src->succ_gen->var_order);

    if (src->agent_name)
        dst->agent_name = BOR_STRDUP(src->agent_name);
    if (src->proj_op_size > 0){
        dst->proj_op = BOR_ALLOC_ARR(plan_op_t, src->proj_op_size);
        for (i = 0; i < src->proj_op_size; ++i){
            planOpInit(dst->proj_op + i, src->var_size);
            planOpCopy(dst->proj_op + i, src->proj_op + i);
        }
    }

    if (src->private_val_size > 0){
        dst->private_val = BOR_ALLOC_ARR(plan_problem_private_val_t,
                                         src->private_val_size);
        memcpy(dst->private_val, src->private_val,
               sizeof(plan_problem_private_val_t) * src->private_val_size);
    }

    planProblemPack(dst);
}

void planProblemAgentsDel(plan_problem_agents_t *agents)
{
    int i;

    planProblemFree(&agents->glob);

    for (i = 0; i < agents->agent_size; ++i)
        planProblemFree(agents->agent + i);

    if (agents->agent)
        BOR_FREE(agents->agent);
    BOR_FREE(agents);
}

plan_problem_agents_t *planProblemAgentsClone(const plan_problem_agents_t *a)
{
    plan_problem_agents_t *prob;
    int i;

    prob = BOR_ALLOC(plan_problem_agents_t);
    planProblemCopy(&prob->glob, &a->glob);
    prob->agent_size = a->agent_size;
    prob->agent = BOR_ALLOC_ARR(plan_problem_t, prob->agent_size);
    for (i = 0; i < a->agent_size; ++i)
        planProblemCopy(prob->agent + i, a->agent + i);
    return prob;
}

void planProblemPack(plan_problem_t *p)
{
    int i;

    planPartStatePack(p->goal, p->state_pool->packer);
    for (i = 0; i < p->op_size; ++i)
        planOpPack(p->op + i, p->state_pool->packer);
    for (i = 0; i < p->proj_op_size; ++i)
        planOpPack(p->proj_op + i, p->state_pool->packer);
}

void planProblemAgentsPack(plan_problem_agents_t *p)
{
    int i;

    planProblemPack(&p->glob);
    for (i = 0; i < p->agent_size; ++i)
        planProblemPack(p->agent + i);
}


static void dotGraphOpLabel(const plan_problem_agents_t *agents,
                            int op_id, char *op_label)
{
    int agent_id;
    int size;
    int glob_op_id, proj_op_id;
    const plan_problem_t *prob;
    int owner = -1;
    int num_projs = 0;

    size = sprintf(op_label, "label=\"%d [", op_id);

    for (agent_id = 0; agent_id < agents->agent_size; ++agent_id){
        prob = agents->agent + agent_id;
        for (proj_op_id = 0; proj_op_id < prob->proj_op_size; ++proj_op_id){
            glob_op_id = prob->proj_op[proj_op_id].global_id;
            if (glob_op_id == op_id){
                owner = prob->proj_op[proj_op_id].global_id;
                ++num_projs;
                break;
            }
        }

        if (agent_id > 0)
            size += sprintf(op_label + size, ":");

        if (proj_op_id == prob->proj_op_size){
            size += sprintf(op_label + size, "N");
        }else{
            size += sprintf(op_label + size, "%d", proj_op_id);
        }
    }

    size += sprintf(op_label + size, "]\"");

    if (owner != -1){
        size += sprintf(op_label + size,
                        ",colorscheme=paired12,color=%d",
                        (2 * (owner + 1)));
    }

    if (num_projs == 1){
        size += sprintf(op_label + size, ",arrowhead=empty");
    }
}

static int dotGraphIsInit(const plan_problem_agents_t *agents,
                          plan_var_id_t var, plan_val_t val)
{
    plan_state_t *state;
    int ret;

    state = planStateNew(agents->glob.var_size);
    planStatePoolGetState(agents->glob.state_pool, 0, state);
    ret = (planStateGet(state, var) == val);
    planStateDel(state);

    return ret;
}

static int dotGraphIsGoal(const plan_problem_agents_t *agents,
                          plan_var_id_t var, plan_val_t val)
{
    int i;
    plan_var_id_t gvar;
    plan_val_t gval;

    PLAN_PART_STATE_FOR_EACH(agents->glob.goal, i, gvar, gval){
        if (gvar == var && gval == val)
            return 1;
    }

    return 0;
}

void planProblemAgentsDotGraph(const plan_problem_agents_t *agents,
                               FILE *fout)
{
    int op_id;
    const plan_op_t *op;
    plan_var_id_t var_pre, var_eff;
    plan_val_t val_pre, val_eff;
    plan_var_id_t var;
    plan_val_t val;
    int _i, _j;
    plan_fact_id_t fid;
    char op_label[1024];

    planFactIdInit(&fid, agents->glob.var, agents->glob.var_size, 0);

    fprintf(fout, "digraph FactOp {\n");

    for (var = 0; var < agents->glob.var_size; ++var){
        for (val = 0; val < agents->glob.var[var].range; ++val){
            fprintf(fout, "%d [label=\"%d [%d:%d]\"",
                    planFactIdVar(&fid, var, val),
                    planFactIdVar(&fid, var, val), var, val);

            if (dotGraphIsGoal(agents, var, val)){
                fprintf(fout, ",color=red,style=bold");
            }
            if (dotGraphIsInit(agents, var, val)){
                fprintf(fout, ",color=blue,style=bold");
            }
            fprintf(fout, "];\n");
        }
    }

    fprintf(fout, "{ rank=\"same\";");
    for (var = 0; var < agents->glob.var_size; ++var){
        for (val = 0; val < agents->glob.var[var].range; ++val){
            if (dotGraphIsInit(agents, var, val))
                fprintf(fout, " %d;", planFactIdVar(&fid, var, val));
        }
    }
    fprintf(fout, "}\n");

    fprintf(fout, "{ rank=\"same\";");
    for (var = 0; var < agents->glob.var_size; ++var){
        for (val = 0; val < agents->glob.var[var].range; ++val){
            if (dotGraphIsGoal(agents, var, val))
                fprintf(fout, " %d;", planFactIdVar(&fid, var, val));
        }
    }
    fprintf(fout, "}\n");

    for (op_id = 0; op_id < agents->glob.op_size; ++op_id){
        op = agents->glob.op + op_id;

        dotGraphOpLabel(agents, op_id, op_label);

        PLAN_PART_STATE_FOR_EACH(op->pre, _i, var_pre, val_pre){
            PLAN_PART_STATE_FOR_EACH(op->eff, _j, var_eff, val_eff){
                fprintf(fout, "%d -> %d [%s];\n",
                        planFactIdVar(&fid, var_pre, val_pre),
                        planFactIdVar(&fid, var_eff, val_eff),
                        op_label);
            }
        }
    }

    fprintf(fout, "}\n");

    planFactIdFree(&fid);
}

void planProblemCreatePrivateProjOps(const plan_op_t *op, int op_size,
                                     const plan_var_t *var, int var_size,
                                     plan_op_t **op_out, int *op_out_size)
{
    plan_op_t *dop;
    int i, dop_size;

    dop = BOR_ALLOC_ARR(plan_op_t, op_size);
    dop_size = 0;

    for (i = 0; i < op_size; ++i){
        planOpInit(dop + dop_size, var_size);
        planOpCopyPrivate(dop + dop_size, op + i, var);
        if (dop[dop_size].pre->vals_size > 0
                || dop[dop_size].eff->vals_size > 0
                || dop[dop_size].cond_eff_size > 0){
            ++dop_size;
        }else{
            planOpFree(dop + dop_size);
        }
    }

    dop = BOR_REALLOC_ARR(dop, plan_op_t, dop_size);

    *op_out = dop;
    *op_out_size = dop_size;
}

void planProblemDestroyOps(plan_op_t *op, int op_size)
{
    int i;

    if (op_size == 0)
        return;

    for (i = 0; i < op_size; ++i)
        planOpFree(op + i);
    BOR_FREE(op);
}

void planProblemProj(plan_problem_t *proj, const plan_problem_t *p)
{
    *proj = *p;
    proj->op = p->proj_op;
    proj->op_size = p->proj_op_size;
}

void planProblemGlobOps(plan_problem_t *proj,
                        const plan_problem_t *base,
                        const plan_problem_t *glob)
{
    *proj = *base;
    proj->op = glob->op;
    proj->op_size = glob->op_size;
}
