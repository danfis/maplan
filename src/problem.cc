#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <boruvka/alloc.h>

#include "plan/problem.h"
#include "plan/causalgraph.h"
#include "problemdef.pb.h"

static void loadProtoProblem(plan_problem_t *prob,
                             const PlanProblem *proto,
                             const plan_var_id_t *var_map,
                             int var_size);
static int hasUnimportantVars(const plan_causal_graph_t *cg);
static void pruneUnimportantVars(plan_problem_t *oldp,
                                 const PlanProblem *proto,
                                 const int *important_var,
                                 plan_var_id_t *var_order);

static PlanProblem *parseProto(const char *fn);
static int loadProblem(plan_problem_t *prob, const PlanProblem *proto,
                       int flags);

static PlanProblem *parseProto(const char *fn)
{
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

    return proto;
}

static int loadProblem(plan_problem_t *p, const PlanProblem *proto,
                       int flags)
{
    plan_causal_graph_t *cg;
    plan_var_id_t *var_order;
    int size;


    // Load problem from the protobuffer
    loadProtoProblem(p, proto, NULL, -1);

    // Fix problem with causal graph
    if (flags & PLAN_PROBLEM_USE_CG){
        cg = planCausalGraphNew(p->var_size, p->op, p->op_size, p->goal);

        if (hasUnimportantVars(cg)){
            size = sizeof(plan_var_id_t) * (cg->var_order_size + 1);
            var_order = (plan_var_id_t *)alloca(size);
            memcpy(var_order, cg->var_order, size);
            pruneUnimportantVars(p, proto, cg->important_var, var_order);
        }else{
            var_order = cg->var_order;
        }

        p->succ_gen = planSuccGenNew(p->op, p->op_size, var_order);
        planCausalGraphDel(cg);
    }else{
        p->succ_gen = planSuccGenNew(p->op, p->op_size, NULL);
    }

    return 0;
}

plan_problem_t *planProblemFromProto(const char *fn, int flags)
{
    plan_problem_t *p = NULL;
    PlanProblem *proto = NULL;

    proto = parseProto(fn);
    if (proto == NULL)
        return NULL;

    p = BOR_ALLOC(plan_problem_t);
    loadProblem(p, proto, flags);

    delete proto;

    return p;
}

plan_problem_agents_t *planProblemAgentsFromProto(const char *fn, int flags)
{
    return NULL;
}

static void loadVar(plan_problem_t *p, const PlanProblem *proto,
                    const plan_var_id_t *var_map, int var_size)
{
    int len;

    // Determine number of variables
    len = proto->var_size();

    // Allocate space for variables
    p->var_size = len;
    if (var_size != -1)
        p->var_size = var_size;
    p->var = BOR_ALLOC_ARR(plan_var_t, p->var_size);

    // Load all variables
    for (int i = 0; i < len; ++i){
        if (var_map && var_map[i] == PLAN_VAR_ID_UNDEFINED)
            continue;

        const PlanProblemVar &proto_var = proto->var(i);
        plan_var_t *var;
        if (var_map){
            var = p->var + var_map[i];
        }else{
            var = p->var + i;
        }

        var->name = strdup(proto_var.name().c_str());
        var->range = proto_var.range();
    }
}

static void loadInitState(plan_problem_t *p, const PlanProblem *proto,
                          const plan_var_id_t *var_map)
{
    plan_state_t *state;
    const PlanProblemState &proto_state = proto->init_state();

    state = planStateNew(p->state_pool);
    for (int i = 0; i < proto_state.val_size(); ++i){
        if (var_map[i] == PLAN_VAR_ID_UNDEFINED)
            continue;
        planStateSet(state, var_map[i], proto_state.val(i));
    }
    p->initial_state = planStatePoolInsert(p->state_pool, state);
    planStateDel(state);
}

static void loadGoal(plan_problem_t *p, const PlanProblem *proto,
                     const plan_var_id_t *var_map)
{
    const PlanProblemPartState &proto_goal = proto->goal();

    p->goal = planPartStateNew(p->state_pool);
    for (int i = 0; i < proto_goal.val_size(); ++i){
        const PlanProblemVarVal &v = proto_goal.val(i);
        if (var_map[v.var()] == PLAN_VAR_ID_UNDEFINED)
            continue;

        planPartStateSet(p->state_pool, p->goal, var_map[v.var()], v.val());
    }
}

static void loadOperator(plan_problem_t *p, const PlanProblem *proto,
                         const plan_var_id_t *var_map)
{
    int i, len, ins;

    len = proto->operator__size();

    // Allocate array for operators
    p->op_size = len;
    p->op = BOR_ALLOC_ARR(plan_operator_t, p->op_size);

    for (i = 0, ins = 0; i < len; ++i){
        int num_effects = 0;
        const PlanProblemOperator &proto_op = proto->operator_(i);
        plan_operator_t *op = p->op + ins;

        planOperatorInit(op, p->state_pool);
        op->name = strdup(proto_op.name().c_str());
        op->cost = proto_op.cost();

        const PlanProblemPartState &proto_pre = proto_op.pre();
        for (int j = 0; j < proto_pre.val_size(); ++j){
            const PlanProblemVarVal &v = proto_pre.val(j);
            if (var_map[v.var()] == PLAN_VAR_ID_UNDEFINED)
                continue;

            planOperatorSetPrecondition(op, var_map[v.var()], v.val());
        }

        const PlanProblemPartState &proto_eff = proto_op.eff();
        for (int j = 0; j < proto_eff.val_size(); ++j){
            const PlanProblemVarVal &v = proto_eff.val(j);
            if (var_map[v.var()] == PLAN_VAR_ID_UNDEFINED)
                continue;

            planOperatorSetEffect(op, var_map[v.var()], v.val());
            ++num_effects;
        }

        for (int j = 0; j < proto_op.cond_eff_size(); ++j){
            int num_cond_eff = 0;
            const PlanProblemCondEff &proto_cond_eff = proto_op.cond_eff(j);
            int cond_eff_id = planOperatorAddCondEff(op);

            const PlanProblemPartState &proto_pre = proto_cond_eff.pre();
            for (int k = 0; k < proto_pre.val_size(); ++k){
                const PlanProblemVarVal &v = proto_pre.val(k);
                if (var_map[v.var()] == PLAN_VAR_ID_UNDEFINED)
                    continue;

                planOperatorCondEffSetPre(op, cond_eff_id, var_map[v.var()], v.val());
            }

            const PlanProblemPartState &proto_eff = proto_cond_eff.eff();
            for (int k = 0; k < proto_eff.val_size(); ++k){
                const PlanProblemVarVal &v = proto_eff.val(k);
                if (var_map[v.var()] == PLAN_VAR_ID_UNDEFINED)
                    continue;

                planOperatorCondEffSetEff(op, cond_eff_id, var_map[v.var()], v.val());
                ++num_effects;
                ++num_cond_eff;
            }

            if (num_cond_eff == 0){
                planOperatorDelLastCondEff(op);
            }
        }

        if (num_effects > 0){
            planOperatorCondEffSimplify(op);
            ++ins;
        }else{
            planOperatorFree(op);
        }
    }

    if (ins != p->op_size){
        // Reset number of operators
        p->op_size = ins;
        // and give back some memory
        p->op = BOR_REALLOC_ARR(p->op, plan_operator_t, p->op_size);
    }
}

static void loadProtoProblem(plan_problem_t *p,
                             const PlanProblem *proto,
                             const plan_var_id_t *var_map,
                             int var_size)
{
    bzero(p, sizeof(*p));

    loadVar(p, proto, var_map, var_size);
    p->state_pool = planStatePoolNew(p->var, p->var_size);

    if (var_map == NULL){
        plan_var_id_t *_var_map;
        _var_map = (plan_var_id_t *)alloca(sizeof(plan_var_id_t) * p->var_size);
        for (int i = 0; i < p->var_size; ++i)
            _var_map[i] = i;
        var_map = _var_map;
    }

    loadInitState(p, proto, var_map);
    loadGoal(p, proto, var_map);
    loadOperator(p, proto, var_map);
}

static int hasUnimportantVars(const plan_causal_graph_t *cg)
{
    int i;
    for (i = 0; i < cg->var_size && cg->important_var[i]; ++i);
    return i != cg->var_size;
}

static void pruneUnimportantVars(plan_problem_t *p,
                                 const PlanProblem *proto,
                                 const int *important_var,
                                 plan_var_id_t *var_order)
{
    int i, id, var_size;
    plan_var_id_t *var_map;

    // Create mapping from old var ID to the new ID
    var_map = (plan_var_id_t *)alloca(sizeof(plan_var_id_t) * p->var_size);
    var_size = 0;
    for (i = 0, id = 0; i < p->var_size; ++i){
        if (important_var[i]){
            var_map[i] = id++;
            ++var_size;
        }else{
            var_map[i] = PLAN_VAR_ID_UNDEFINED;
        }
    }

    // fix var-order array
    for (; *var_order != PLAN_VAR_ID_UNDEFINED; ++var_order){
        *var_order = var_map[*var_order];
    }

    planProblemFree(p);
    loadProtoProblem(p, proto, var_map, var_size);
}
