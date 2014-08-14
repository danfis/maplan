#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <boruvka/alloc.h>

#include "plan/problem.h"
#include "plan/causalgraph.h"
#include "problemdef.pb.h"

static void loadProblem(plan_problem_t *p, const PlanProblem *proto);

plan_problem_t *planProblemFromProto(const char *fn)
{
    plan_problem_t *p = NULL;
    PlanProblem *proto = NULL;
    int fd;

    // Open file with problem definition
    fd = open(fn, O_RDONLY);
    if (fd == -1){
        fprintf(stderr, "Plan Error: Could not open file `%s'\n", fn);
        return NULL;
    }

    // Load protobuf definition from the file
    proto = new PlanProblem;
    proto->ParseFromFileDescriptor(fd);
    close(fd);

    // Check version protobuf version
    if (proto->version() != 3){
        fprintf(stderr, "Plan Error: Unknown version of problem"
                        " definition: %d\n", proto->version());
        delete proto;
        return NULL;
    }

    p = BOR_ALLOC(plan_problem_t);
    bzero(p, sizeof(*p));

    // Load problem from the protobuffer
    loadProblem(p, proto);

    delete proto;

    return p;
}

static void loadVar(plan_problem_t *p, const PlanProblem *proto)
{
    int len;

    // Determine number of variables
    len = proto->var_size();

    // Allocate space for variables
    p->var_size = len;
    p->var = BOR_ALLOC_ARR(plan_var_t, p->var_size);

    // Load all variables
    for (int i = 0; i < len; ++i){
        const PlanProblemVar &proto_var = proto->var(i);
        plan_var_t *var = p->var + i;

        var->name = strdup(proto_var.name().c_str());
        var->range = proto_var.range();
        var->fact_name = NULL;
    }
}

static void loadInitState(plan_problem_t *p, const PlanProblem *proto)
{
    plan_state_t *state;
    const PlanProblemState &proto_state = proto->init_state();

    state = planStateNew(p->state_pool);
    for (int i = 0; i < proto_state.val_size(); ++i){
        planStateSet(state, i, proto_state.val(i));
    }
    p->initial_state = planStatePoolInsert(p->state_pool, state);
    planStateDel(state);
}

static void loadGoal(plan_problem_t *p, const PlanProblem *proto)
{
    const PlanProblemPartState &proto_goal = proto->goal();

    p->goal = planPartStateNew(p->state_pool);
    for (int i = 0; i < proto_goal.val_size(); ++i){
        const PlanProblemVarVal &v = proto_goal.val(i);
        planPartStateSet(p->state_pool, p->goal, v.var(), v.val());
    }
}

static void loadOperator(plan_problem_t *p, const PlanProblem *proto)
{
    int len;

    len = proto->operator__size();

    // Allocate array for operators
    p->op_size = len;
    p->op = BOR_ALLOC_ARR(plan_operator_t, p->op_size);

    for (int i = 0; i < len; ++i){
        const PlanProblemOperator &proto_op = proto->operator_(i);
        plan_operator_t *op = p->op + i;

        planOperatorInit(p->op + i, p->state_pool);
        op->name = strdup(proto_op.name().c_str());
        op->cost = proto_op.cost();

        const PlanProblemPartState &proto_pre = proto_op.pre();
        for (int j = 0; j < proto_pre.val_size(); ++j){
            const PlanProblemVarVal &v = proto_pre.val(j);
            planOperatorSetPrecondition(op, v.var(), v.val());
        }

        const PlanProblemPartState &proto_eff = proto_op.eff();
        for (int j = 0; j < proto_eff.val_size(); ++j){
            const PlanProblemVarVal &v = proto_eff.val(j);
            planOperatorSetEffect(op, v.var(), v.val());
        }

        for (int j = 0; j < proto_op.cond_eff_size(); ++j){
            const PlanProblemCondEff &proto_cond_eff = proto_op.cond_eff(j);
            int cond_eff_id = planOperatorAddCondEff(op);

            const PlanProblemPartState &proto_pre = proto_cond_eff.pre();
            for (int k = 0; k < proto_pre.val_size(); ++k){
                const PlanProblemVarVal &v = proto_pre.val(k);
                planOperatorCondEffSetPre(op, cond_eff_id, v.var(), v.val());
            }

            const PlanProblemPartState &proto_eff = proto_cond_eff.eff();
            for (int k = 0; k < proto_eff.val_size(); ++k){
                const PlanProblemVarVal &v = proto_eff.val(k);
                planOperatorCondEffSetEff(op, cond_eff_id, v.var(), v.val());
            }
        }

        planOperatorCondEffSimplify(op);
    }
}

static void loadProblem(plan_problem_t *p, const PlanProblem *proto)
{
    plan_causal_graph_t *cg;

    loadVar(p, proto);
    p->state_pool = planStatePoolNew(p->var, p->var_size);

    loadInitState(p, proto);
    loadGoal(p, proto);
    loadOperator(p, proto);

    cg = planCausalGraphNew(p->var_size, p->op, p->op_size, p->goal);
    p->succ_gen = planSuccGenNew2(p->op, p->op_size,
                                  cg->var_order, cg->var_order_size);
    planCausalGraphDel(cg);
}
