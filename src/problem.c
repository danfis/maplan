#include <string.h>

#include <boruvka/alloc.h>

#include "plan/problem.h"
#include "fact_id.h"


void planProblemInit(plan_problem_t *p)
{
    bzero(p, sizeof(*p));
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


void planProblemAgentsDel(plan_problem_agents_t *agents)
{
    int i, opid;
    plan_problem_t *agent;

    planProblemFree(&agents->glob);

    for (i = 0; i < agents->agent_size; ++i){
        agent = agents->agent + i;

        for (opid = 0; opid < agent->op_size; ++opid)
            planOpExtraMAOpFree(agent->op + opid);
    }

    for (i = 0; i < agents->agent_size; ++i)
        planProblemFree(agents->agent + i);

    if (agents->agent)
        BOR_FREE(agents->agent);
    BOR_FREE(agents);
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
            glob_op_id = planOpExtraMAProjOpGlobalId(prob->proj_op + proj_op_id);
            if (glob_op_id == op_id){
                owner = planOpExtraMAProjOpOwner(prob->proj_op + proj_op_id);
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
        size += sprintf(op_label + size, ",fontcolor=10");
    }
}

static int dotGraphIsInit(const plan_problem_agents_t *agents,
                          plan_var_id_t var, plan_val_t val)
{
    plan_state_t *state;
    int ret;

    state = planStateNew(agents->glob.state_pool);
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

    planFactIdInit(&fid, agents->glob.var, agents->glob.var_size);

    fprintf(fout, "digraph FactOp {\n");

    for (var = 0; var < agents->glob.var_size; ++var){
        for (val = 0; val < agents->glob.var[var].range; ++val){
            fprintf(fout, "%d [label=\"%d [%d:%d]\"",
                    planFactId(&fid, var, val),
                    planFactId(&fid, var, val), var, val);

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
                fprintf(fout, " %d;", planFactId(&fid, var, val));
        }
    }
    fprintf(fout, "}\n");

    fprintf(fout, "{ rank=\"same\";");
    for (var = 0; var < agents->glob.var_size; ++var){
        for (val = 0; val < agents->glob.var[var].range; ++val){
            if (dotGraphIsGoal(agents, var, val))
                fprintf(fout, " %d;", planFactId(&fid, var, val));
        }
    }
    fprintf(fout, "}\n");

    for (op_id = 0; op_id < agents->glob.op_size; ++op_id){
        op = agents->glob.op + op_id;

        dotGraphOpLabel(agents, op_id, op_label);

        PLAN_PART_STATE_FOR_EACH(op->pre, _i, var_pre, val_pre){
            PLAN_PART_STATE_FOR_EACH(op->eff, _j, var_eff, val_eff){
                fprintf(fout, "%d -> %d [%s];\n",
                        planFactId(&fid, var_pre, val_pre),
                        planFactId(&fid, var_eff, val_eff),
                        op_label);
            }
        }
    }

    fprintf(fout, "}\n");

    planFactIdFree(&fid);
}
