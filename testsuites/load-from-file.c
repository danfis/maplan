#include <cu/cu.h>
#include "plan/problem.h"

static void pVar(const plan_var_t *var, int var_size, FILE *fout)
{
    int i, j;

    fprintf(fout, "Vars[%d]:\n", var_size);
    for (i = 0; i < var_size; ++i){
        fprintf(fout, "[%d] name: `%s', range: %d, is_private:", i, var[i].name, var[i].range);
        for (j = 0; j < var[i].range; ++j)
            fprintf(fout, " %d", var[i].is_private[j]);
        fprintf(fout, "\n");
    }
}

static void pInitState(const plan_state_pool_t *state_pool, plan_state_id_t sid, FILE *fout)
{
    plan_state_t *state;
    int i, size;

    state = planStateNew(state_pool->num_vars);
    planStatePoolGetState(state_pool, sid, state);
    size = planStateSize(state);
    fprintf(fout, "Initial state:");
    for (i = 0; i < size; ++i){
        fprintf(fout, " %d:%d", i, (int)planStateGet(state, i));
    }
    fprintf(fout, "\n");
    planStateDel(state);
}

static void pPartState(const plan_part_state_t *p, FILE *fout)
{
    plan_var_id_t var;
    plan_val_t val;
    int i;

    PLAN_PART_STATE_FOR_EACH(p, i, var, val){
        fprintf(fout, " %d:%d", (int)var, (int)val);
    }
}

static void pGoal(const plan_part_state_t *p, FILE *fout)
{
    fprintf(fout, "Goal:");
    pPartState(p, fout);
    fprintf(fout, "\n");
}

static void pOp(const plan_op_t *op, int op_size, FILE *fout)
{
    int i, j;

    fprintf(fout, "Ops[%d]:\n", op_size);
    for (i = 0; i < op_size; ++i){
        fprintf(fout, "[%d] cost: %d, gid: %d, owner: %d (%lx),"
               " private: %d, recv_agent: %lx, name: `%s'\n",
               i, (int)op[i].cost, op[i].global_id,
               op[i].owner, (unsigned long)op[i].ownerarr,
               op[i].is_private,
               (unsigned long)op[i].recv_agent,
               op[i].name);
        fprintf(fout, "[%d] pre:", i);
        pPartState(op[i].pre, fout);
        fprintf(fout, ", eff:");
        pPartState(op[i].eff, fout);
        fprintf(fout, "\n");

        for (j = 0; j < op[i].cond_eff_size; ++j){
            fprintf(fout, "[%d] cond_eff[%d]:", i, j);
            fprintf(fout, " pre:");
            pPartState(op[i].cond_eff[j].pre, fout);
            fprintf(fout, ", eff:");
            pPartState(op[i].cond_eff[j].eff, fout);
            fprintf(fout, "\n");
        }
    }
}

static void pPrivateVal(const plan_problem_private_val_t *pv, int pvsize, FILE *fout)
{
    int i;

    fprintf(fout, "PrivateVal:");
    for (i = 0; i < pvsize; ++i){
        fprintf(fout, " %d:%d", pv[i].var, pv[i].val);
    }
    fprintf(fout, "\n");
}

static void pProblem(const plan_problem_t *p, FILE *fout)
{
    pVar(p->var, p->var_size, fout);
    pInitState(p->state_pool, p->initial_state, fout);
    pGoal(p->goal, fout);
    pOp(p->op, p->op_size, fout);
    fprintf(fout, "Succ Gen: %d\n", (int)(p->succ_gen != NULL));
}

TEST(testLoadFromProto)
{
    plan_problem_t *p;
    int flags;

    flags = PLAN_PROBLEM_USE_CG | PLAN_PROBLEM_PRUNE_DUPLICATES;
    p = planProblemFromProto("../data/ma-benchmarks/rovers/p03.proto", flags);
    printf("---- testLoadFromProto ----\n");
    pProblem(p, stdout);
    printf("---- testLoadFromProto END ----\n");
    planProblemDel(p);
}

TEST(testLoadFromProtoCondEff)
{
    plan_problem_t *p;

    p = planProblemFromProto("../data/ipc2014/satisficing/CityCar/p3-2-2-0-1.proto",
                             PLAN_PROBLEM_USE_CG);
    printf("---- testLoadFromProtoCondEff ----\n");
    pProblem(p, stdout);
    printf("---- testLoadFromProtoCondEff END ----\n");
    planProblemDel(p);
}

static void pAgent(const plan_problem_t *p, FILE *fout)
{
    plan_op_t *private_op;
    int private_op_size;
    planProblemCreatePrivateProjOps(p->op, p->op_size, p->var, p->var_size,
                                    &private_op, &private_op_size);

    fprintf(fout, "++++ %s ++++\n", p->agent_name);
    fprintf(fout, "Agent ID: %d\n", p->agent_id);
    pVar(p->var, p->var_size, fout);
    pPrivateVal(p->private_val, p->private_val_size, fout);
    pInitState(p->state_pool, p->initial_state, fout);
    pGoal(p->goal, fout);
    pOp(p->op, p->op_size, fout);
    fprintf(fout, "Succ Gen: %d\n", (int)(p->succ_gen != NULL));
    fprintf(fout, "Proj op:\n");
    pOp(p->proj_op, p->proj_op_size, fout);
    fprintf(fout, "Private Proj op:\n");
    pOp(private_op, private_op_size, fout);
    fprintf(fout, "++++ %s END ++++\n", p->agent_name);

    planProblemDestroyOps(private_op, private_op_size);
}

static void testAgentProto(const char *proto, FILE *fout)
{
    plan_problem_agents_t *agents;
    int i;
    int flags;

    flags = PLAN_PROBLEM_USE_CG | PLAN_PROBLEM_PRUNE_DUPLICATES;
    agents = planProblemAgentsFromProto(proto, flags);
    assertNotEquals(agents, NULL);
    if (agents == NULL)
        return;

    fprintf(fout, "---- %s ----\n", proto);
    pVar(agents->glob.var, agents->glob.var_size, fout);
    pInitState(agents->glob.state_pool, agents->glob.initial_state, fout);
    pGoal(agents->glob.goal, fout);
    pOp(agents->glob.op, agents->glob.op_size, fout);
    fprintf(fout, "Succ Gen: %d\n", (int)(agents->glob.succ_gen != NULL));

    for (i = 0; i < agents->agent_size; ++i)
        pAgent(agents->agent + i, fout);

    fprintf(fout, "---- %s END ----\n", proto);
    planProblemAgentsDel(agents);
}

TEST(testLoadAgentFromProto)
{
    testAgentProto("../data/ma-benchmarks/rovers/p03.proto", stdout);
    testAgentProto("../data/ma-benchmarks/depot/pfile1.proto", stdout);
}
