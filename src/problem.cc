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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <algorithm>
#include <boruvka/alloc.h>

#include "plan/problem.h"
#include "plan/causal_graph.h"
#include "plan/fact_id.h"
#include "problemdef.pb.h"

void planShutdownProtobuf(void)
{
    google::protobuf::ShutdownProtobufLibrary();
}

/** Forward declaration */
class AgentVarVals;

/** Parses protobuffer from the given file. Returns NULL on failure. */
static PlanProblem *parseProto(const char *fn);
/** Loads problem from protobuffer */
static int loadProblem(plan_problem_t *prob,
                       const PlanProblem *proto,
                       unsigned flags);
/** Load agents part from the protobuffer */
static void loadAgents(plan_problem_agents_t *p,
                       const PlanProblem *proto,
                       unsigned flags);

static void loadProtoProblem(plan_problem_t *prob,
                             const PlanProblem *proto,
                             const plan_var_id_t *var_map,
                             int var_size,
                             int ma_state_privacy);
static int hasUnimportantVars(const plan_causal_graph_t *cg);
static void pruneUnimportantVars(plan_problem_t *oldp,
                                 const PlanProblem *proto,
                                 const int *important_var,
                                 plan_var_id_t *var_order);
static void pruneDuplicateOps(plan_problem_t *prob);

/** Initializes agent's problem struct from global problem struct */
static void agentInitProblem(plan_problem_t *dst, const plan_problem_t *src);
/** Sets owner of the operators according to the agent names */
static void setOpOwner(plan_op_t *ops, int op_size,
                       const plan_problem_agents_t *agents);
/** Sets private flag to operators */
static void setOpPrivate(plan_op_t *ops, int op_size,
                         const AgentVarVals &vals);
/** Projects partial state to agent's facts */
static void projectPartState(plan_part_state_t *ps,
                             int agent_id,
                             const AgentVarVals &vals);
/** Project operator to agent's facts */
static bool projectOp(plan_op_t *op, int agent_id, const AgentVarVals &vals);
/** Creates projected operators in problem struct */
static void createProjectedOps(const plan_op_t *ops, int ops_size,
                               int agent_id, plan_problem_t *agent,
                               const AgentVarVals &vals);
/** Creates agent's operators array in dst problem struct */
static void createOps(const plan_op_t *ops, int op_size,
                      int agent_id, plan_problem_t *dst);
/** Sets .private_vals array in problem struct and .is_private member of
 *  var[] structures. */
static void setPrivateVals(plan_problem_t *agent, int agent_id,
                           const AgentVarVals &vals);


plan_problem_t *planProblemFromProto(const char *fn, unsigned flags)
{
    plan_problem_t *p = NULL;
    PlanProblem *proto = NULL;

    proto = parseProto(fn);
    if (proto == NULL)
        return NULL;

    p = BOR_ALLOC(plan_problem_t);
    loadProblem(p, proto, flags);
    planProblemPack(p);

    delete proto;

    return p;
}

plan_problem_agents_t *planProblemAgentsFromProto(const char *fn, unsigned flags)
{
    plan_problem_agents_t *p = NULL;
    PlanProblem *proto = NULL;

    proto = parseProto(fn);
    if (proto == NULL)
        return NULL;

    p = BOR_ALLOC(plan_problem_agents_t);
    loadProblem(&p->glob, proto, flags);
    loadAgents(p, proto, flags);
    planProblemAgentsPack(p);

    delete proto;

    return p;
}

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

    if (proto->projected_operator_size() > 0 && !proto->has_agent_id()){
        fprintf(stderr, "Error: Invalid .proto file. It contains projected"
                " operators but not agent's ID.\n");
        delete proto;
        return NULL;
    }


    return proto;
}

static int loadProblem(plan_problem_t *p,
                       const PlanProblem *proto,
                       unsigned flags)
{
    plan_causal_graph_t *cg;
    plan_var_id_t *var_order;
    int i, size, num_agents, ma_state_privacy;


    // Load problem from the protobuffer
    ma_state_privacy = 0;
    if (flags & PLAN_PROBLEM_MA_STATE_PRIVACY)
        ma_state_privacy = 1;
    loadProtoProblem(p, proto, NULL, -1, ma_state_privacy);
    p->duplicate_ops_removed = 0;

    // Fix problem with causal graph
    if (flags & PLAN_PROBLEM_USE_CG){
        cg = planCausalGraphNew(p->var_size);
        planCausalGraphBuildFromOps(cg, p->op, p->op_size);
        planCausalGraph(cg, p->goal);

        if (hasUnimportantVars(cg)){
            size = sizeof(plan_var_id_t) * (cg->var_order_size + 1);
            var_order = (plan_var_id_t *)alloca(size);
            memcpy(var_order, cg->var_order, size);
            pruneUnimportantVars(p, proto, cg->important_var, var_order);
        }else{
            var_order = cg->var_order;
        }

        if (flags & PLAN_PROBLEM_PRUNE_DUPLICATES)
            pruneDuplicateOps(p);
        p->succ_gen = planSuccGenNew(p->op, p->op_size, var_order);
        planCausalGraphDel(cg);
    }else{
        if (flags & PLAN_PROBLEM_PRUNE_DUPLICATES)
            pruneDuplicateOps(p);
        p->succ_gen = planSuccGenNew(p->op, p->op_size, NULL);
    }

    if (flags & PLAN_PROBLEM_OP_UNIT_COST){
        for (i = 0; i < p->op_size; ++i)
            p->op[i].cost = 1;
    }

    num_agents = (flags & 0xff0u) >> 4;
    if (num_agents > 0){
        p->num_agents = num_agents;
    }

    return 0;
}



static bool sortCmpPrivateVals(const plan_problem_private_val_t &v1,
                               const plan_problem_private_val_t &v2)
{
    if (v1.var < v2.var)
        return true;
    if (v1.var == v2.var && v1.val < v2.val)
        return true;
    return false;
}

struct AgentVarVal {
    std::vector<bool> use; /*!< true for each agent if the value is used
                                in precondition or effect of its operator */
    std::vector<bool> pre; /*!< true for each agent if the value is used in
                                precondition of its operator */
    bool pub;  /*!< True if the value is public (used by more than one
                    agent) */

    AgentVarVal() : pub(false) {}

    void resize(int size)
    {
        use.resize(size, false);
        pre.resize(size, false);
    }
};

class AgentVarVals {
    int agent_size;
    std::vector<std::vector<AgentVarVal> > val;
    std::vector<std::vector<plan_problem_private_val_t> > private_vals;

  public:
    AgentVarVals(const plan_problem_t *prob, int agent_size)
        : agent_size(agent_size)
    {
        val.resize(prob->var_size);
        for (int i = 0; i < prob->var_size; ++i){
            val[i].resize(prob->var[i].range);
            for (int j = 0; j < (int)prob->var[i].range; ++j){
                val[i][j].resize(agent_size);
            }
        }

        private_vals.resize(agent_size);
    }

    /**
     * Set variable's value as used by the agent in precondition.
     */
    void setPreUse(int var, int value, int agent_id)
    {
        val[var][value].pre[agent_id] = true;
        val[var][value].use[agent_id] = true;
    }

    void setPreUse(const plan_part_state_t *ps, int agent_id)
    {
        plan_var_id_t var;
        plan_val_t val;
        int i;

        PLAN_PART_STATE_FOR_EACH(ps, i, var, val){
            setPreUse(var, val, agent_id);
        }
    }

    /**
     * Set variable's value as used by the agent in an effect.
     */
    void setEffUse(int var, int value, int agent_id)
    {
        val[var][value].use[agent_id] = true;
    }

    void setEffUse(const plan_part_state_t *ps, int agent_id)
    {
        plan_var_id_t var;
        plan_val_t val;
        int i;

        PLAN_PART_STATE_FOR_EACH(ps, i, var, val){
            setEffUse(var, val, agent_id);
        }
    }

    /**
     * Sets variables' values as public if they are used by more than one
     * agent and sets them as private otherwise.
     */
    void determinePublicVals()
    {
        plan_problem_private_val_t private_val;

        for (size_t i = 0; i < val.size(); ++i){
            for (size_t j = 0; j < val[i].size(); ++j){
                int sum = 0;
                int owner = -1;
                for (size_t k = 0; k < val[i][j].use.size() && sum < 2; ++k){
                    if (val[i][j].use[k]){
                        ++sum;
                        owner = k;
                    }
                }
                if (sum > 1){
                    val[i][j].pub = true;
                }else if (sum == 1){
                    private_val.var = i;
                    private_val.val = j;
                    private_vals[owner].push_back(private_val);
                }
            }
        }

        for (size_t i = 0; i < private_vals.size(); ++i){
            std::sort(private_vals[i].begin(), private_vals[i].end(),
                      sortCmpPrivateVals);
        }
    }

    void setUse(const plan_op_t *ops, int op_size,
                const plan_part_state_t *goal)
    {
        int i, agent_id;
        const plan_op_t *op;
        uint64_t ownerarr;

        // Set goals as public for all agents
        for (i = 0; i < agent_size; ++i)
            setPreUse(goal, i);

        // Process operators from all agents and all its operators
        for (i = 0; i < op_size; ++i){
            op = ops + i;

            ownerarr = op->ownerarr;
            for (agent_id = 0; agent_id < agent_size; ++agent_id){
                if (ownerarr & 0x1){
                    setPreUse(op->pre, agent_id);
                    setEffUse(op->eff, agent_id);
                }

                ownerarr >>= 1;
            }
        }

        determinePublicVals();
    }

    /**
     * Returns true if the value is used by the specified agent.
     */
    bool used(int var, int value, int agent_id) const
    {
        return val[var][value].use[agent_id];
    }

    /**
     * Returns true if the value is used as precondition by the specified
     * agent.
     */
    bool usedAsPre(int var, int value, int agent_id) const
    {
        return val[var][value].pre[agent_id];
    }

    /**
     * Returns true if the value is public.
     */
    bool isPublic(int var, int value) const
    {
        return val[var][value].pub;
    }

    bool isPublic(const plan_part_state_t *ps) const
    {
        plan_var_id_t var;
        plan_val_t val;
        int i;

        PLAN_PART_STATE_FOR_EACH(ps, i, var, val){
            if (isPublic(var, val))
                return true;
        }

        return false;
    }

    const std::vector<plan_problem_private_val_t> &privateVals(int agent_id) const
    {
        return private_vals[agent_id];
    }
};


static void loadAgents(plan_problem_agents_t *p,
                       const PlanProblem *proto,
                       unsigned flags)
{
    int i;
    plan_problem_t *agent;

    p->agent_size = proto->agent_name_size();
    if (p->agent_size == 0){
        fprintf(stderr, "Problem Error: No agents defined!\n");
        p->agent = NULL;
        return;

    }else if (p->agent_size > 64){
        fprintf(stderr, "Problem Error: More than 64 agents defined!\n");
        p->agent = NULL;
        return;
    }

    AgentVarVals var_vals(&p->glob, p->agent_size);

    p->agent = BOR_ALLOC_ARR(plan_problem_t, p->agent_size);
    for (i = 0; i < p->agent_size; ++i){
        agent = p->agent + i;
        agentInitProblem(agent, &p->glob);
        agent->agent_name = BOR_STRDUP(proto->agent_name(i).c_str());
        agent->agent_id = i;
        agent->num_agents = p->agent_size;
    }

    // Set owners of the operators
    setOpOwner(p->glob.op, p->glob.op_size, p);

    // Determine which variable values are used by which agents' operators
    var_vals.setUse(p->glob.op, p->glob.op_size, p->glob.goal);

    // Mark private operators
    setOpPrivate(p->glob.op, p->glob.op_size, var_vals);


    for (i = 0; i < p->agent_size; ++i){
        // Create operators that belong to the specified agent
        createOps(p->glob.op, p->glob.op_size, i, p->agent + i);

        // Create projected operators
        createProjectedOps(p->glob.op, p->glob.op_size,
                           i, p->agent + i, var_vals);

        // Create successor generator from operators that are owned by the
        // agent
        p->agent[i].succ_gen = planSuccGenNew(p->agent[i].op,
                                              p->agent[i].op_size, NULL);

        setPrivateVals(p->agent + i, i, var_vals);
    }
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

        planVarInit(var, proto_var.name().c_str(), proto_var.range());
        if (proto_var.has_is_private() && proto_var.is_private())
            planVarSetPrivate(var);

        for (int j = 0; j < proto_var.fact_name_size(); ++j){
            char *name = BOR_STRDUP(proto_var.fact_name(j).c_str());
            const char *val_name = name;
            if (strncmp(name, "Atom ", 5) == 0){
                val_name += 5;

            }else if (strncmp(name, "NegatedAtom ", 12) == 0){
                name[9] = '(';
                name[10] = 'N';
                name[11] = ')';
                val_name += 9;
            }else if (strncmp(name, "(P)Atom ", 8) == 0){
                name[5] = '(';
                name[6] = 'P';
                name[7] = ')';
                val_name += 5;
            }

            planVarSetValName(var, j, val_name);
            BOR_FREE(name);
        }
    }
}

static void loadInitState(plan_problem_t *p, const PlanProblem *proto,
                          const plan_var_id_t *var_map)
{
    plan_state_t *state;
    const PlanProblemState &proto_state = proto->init_state();

    state = planStateNew(p->state_pool->num_vars);
    for (int i = 0; i < proto_state.val_size(); ++i){
        if (var_map[i] == PLAN_VAR_ID_UNDEFINED)
            continue;
        planStateSet(state, var_map[i], proto_state.val(i));
    }
    if (p->ma_privacy_var >= 0)
        planStateSet(state, p->ma_privacy_var, 0);

    p->initial_state = planStatePoolInsert(p->state_pool, state);
    planStateDel(state);
}

static void loadGoal(plan_problem_t *p, const PlanProblem *proto,
                     const plan_var_id_t *var_map)
{
    const PlanProblemPartState &proto_goal = proto->goal();

    p->goal = planPartStateNew(p->state_pool->num_vars);
    for (int i = 0; i < proto_goal.val_size(); ++i){
        const PlanProblemVarVal &v = proto_goal.val(i);
        if (var_map[v.var()] == PLAN_VAR_ID_UNDEFINED)
            continue;

        planPartStateSet(p->goal, var_map[v.var()], v.val());
    }
}

static int loadOp(plan_op_t *op, int id, const PlanProblemOperator &proto_op,
                  int var_size, const plan_var_id_t *var_map)
{
    int num_effects = 0;

    planOpInit(op, var_size);
    op->name = BOR_STRDUP(proto_op.name().c_str());
    op->cost = proto_op.cost();
    op->global_id = id;

    if (proto_op.has_owner()){
        planOpAddOwner(op, proto_op.owner());
        planOpSetFirstOwner(op);
    }

    if (proto_op.has_global_id())
        op->global_id = proto_op.global_id();

    if (proto_op.has_is_private())
        op->is_private = proto_op.is_private();

    const PlanProblemPartState &proto_pre = proto_op.pre();
    for (int j = 0; j < proto_pre.val_size(); ++j){
        const PlanProblemVarVal &v = proto_pre.val(j);
        if (var_map[v.var()] == PLAN_VAR_ID_UNDEFINED)
            continue;

        planOpSetPre(op, var_map[v.var()], v.val());
    }

    const PlanProblemPartState &proto_eff = proto_op.eff();
    for (int j = 0; j < proto_eff.val_size(); ++j){
        const PlanProblemVarVal &v = proto_eff.val(j);
        if (var_map[v.var()] == PLAN_VAR_ID_UNDEFINED)
            continue;

        planOpSetEff(op, var_map[v.var()], v.val());
        ++num_effects;
    }

    for (int j = 0; j < proto_op.cond_eff_size(); ++j){
        int num_cond_eff = 0;
        const PlanProblemCondEff &proto_cond_eff = proto_op.cond_eff(j);
        int cond_eff_id = planOpAddCondEff(op);

        const PlanProblemPartState &proto_pre = proto_cond_eff.pre();
        for (int k = 0; k < proto_pre.val_size(); ++k){
            const PlanProblemVarVal &v = proto_pre.val(k);
            if (var_map[v.var()] == PLAN_VAR_ID_UNDEFINED)
                continue;

            planOpCondEffSetPre(op, cond_eff_id, var_map[v.var()], v.val());
        }

        const PlanProblemPartState &proto_eff = proto_cond_eff.eff();
        for (int k = 0; k < proto_eff.val_size(); ++k){
            const PlanProblemVarVal &v = proto_eff.val(k);
            if (var_map[v.var()] == PLAN_VAR_ID_UNDEFINED)
                continue;

            planOpCondEffSetEff(op, cond_eff_id, var_map[v.var()], v.val());
            ++num_effects;
            ++num_cond_eff;
        }

        if (num_cond_eff == 0){
            planOpDelLastCondEff(op);
        }
    }

    return num_effects;
}

static void loadOperator(plan_problem_t *p, const PlanProblem *proto,
                         const plan_var_id_t *var_map)
{
    int i, len, ins;
    int num_effects;

    len = proto->operator__size();

    // Allocate array for operators
    p->op_size = len;
    p->op = BOR_ALLOC_ARR(plan_op_t, p->op_size);

    for (i = 0, ins = 0; i < len; ++i){
        num_effects = loadOp(p->op + ins, ins, proto->operator_(i),
                             p->var_size, var_map);

        if (num_effects > 0){
            planOpCondEffSimplify(p->op + ins);
            ++ins;
        }else{
            planOpFree(p->op + ins);
        }
    }

    if (ins != p->op_size){
        // Reset number of operators
        p->op_size = ins;
        // and give back some memory
        p->op = BOR_REALLOC_ARR(p->op, plan_op_t, p->op_size);
    }
}

static void loadProjOperator(plan_problem_t *p, const PlanProblem *proto,
                             const plan_var_id_t *var_map)
{
    int i, len;

    len = proto->projected_operator_size();
    if (len == 0)
        return;

    // Allocate array for operators
    p->proj_op_size = len;
    p->proj_op = BOR_ALLOC_ARR(plan_op_t, p->proj_op_size);

    for (i = 0; i < len; ++i){
        loadOp(p->proj_op + i, i, proto->projected_operator(i),
               p->var_size, var_map);
        planOpCondEffSimplify(p->proj_op + i);
    }
}

static void loadProtoProblem(plan_problem_t *p,
                             const PlanProblem *proto,
                             const plan_var_id_t *var_map,
                             int var_size,
                             int ma_state_privacy)
{
    bzero(p, sizeof(*p));

    p->ma_privacy_var = -1;

    loadVar(p, proto, var_map, var_size);
    if (ma_state_privacy){
        p->var_size += 1;
        p->var = BOR_REALLOC_ARR(p->var, plan_var_t, p->var_size);
        p->ma_privacy_var = p->var_size - 1;
        planVarInitMAPrivacy(p->var + p->ma_privacy_var);
    }

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
    loadProjOperator(p, proto, var_map);

    if (proto->has_agent_id()){
        p->agent_id = proto->agent_id();
        if (proto->agent_name_size() == 1)
            p->agent_name = BOR_STRDUP(proto->agent_name(0).c_str());
    }
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
    int i, id, var_size, ma_state_privacy;
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

    ma_state_privacy = p->ma_privacy_var >= 0;
    planProblemFree(p);
    loadProtoProblem(p, proto, var_map, var_size, ma_state_privacy);
}

static int cmpPartState(const plan_part_state_t *p1,
                        const plan_part_state_t *p2)
{
    int i;

    // First sort part-states with less values set
    if (p1->vals_size < p2->vals_size){
        return -1;
    }else if (p1->vals_size > p2->vals_size){
        return 1;
    }

    // We assume that in .vals_size are values sorted according to variable
    // ID
    for (i = 0; i < p1->vals_size; ++i){
        if (p1->vals[i].var != p2->vals[i].var){
            return p1->vals[i].var - p2->vals[i].var;
        }else if (p1->vals[i].val != p2->vals[i].val){
            return p1->vals[i].val - p2->vals[i].val;
        }
    }

    return 0;
}

static int cmpOp(const plan_op_t *op1, const plan_op_t *op2)
{
    int i, cmp;
    plan_op_cond_eff_t *ce1, *ce2;

    if (op1->pre->vals_size != op2->pre->vals_size)
        return op1->pre->vals_size - op2->pre->vals_size;
    if (op1->eff->vals_size != op2->eff->vals_size)
        return op1->eff->vals_size - op2->eff->vals_size;
    if (op1->cond_eff_size != op2->cond_eff_size)
        return op1->cond_eff_size - op2->cond_eff_size;

    if ((cmp = cmpPartState(op1->pre, op2->pre)) != 0)
        return cmp;
    if ((cmp = cmpPartState(op1->eff, op2->eff)) != 0)
        return cmp;

    for (i = 0; i < op1->cond_eff_size; ++i){
        ce1 = op1->cond_eff + i;
        ce2 = op2->cond_eff + i;
        if ((cmp = cmpPartState(ce1->pre, ce2->pre)) != 0)
            return cmp;
        if ((cmp = cmpPartState(ce1->eff, ce2->eff)) != 0)
            return cmp;
    }

    return 0;
}

static int duplicateOpsCmp(const void *a, const void *b)
{
    const plan_op_t *op1 = *(const plan_op_t **)a;
    const plan_op_t *op2 = *(const plan_op_t **)b;
    return cmpOp(op1, op2);
}

static void pruneDuplicateOps(plan_problem_t *prob)
{
    plan_op_t **sorted_ops;
    int i, ins;

    // Sort operators so that duplicates are one after other
    sorted_ops = BOR_ALLOC_ARR(plan_op_t *, prob->op_size);
    for (i = 0; i < prob->op_size; ++i)
        sorted_ops[i] = prob->op + i;
    qsort(sorted_ops, prob->op_size, sizeof(plan_op_t *), duplicateOpsCmp);

    // Free duplicate operators and mark their position with global_id set
    // to -1
    for (i = 1; i < prob->op_size; ++i){
        if (cmpOp(sorted_ops[i - 1], sorted_ops[i]) == 0){
            planOpFree(sorted_ops[i - 1]);
            sorted_ops[i - 1]->global_id = -1;
        }
    }

    // Squash operators to a continuous array
    for (i = 0, ins = 0; i < prob->op_size; ++i, ++ins){
        if (prob->op[i].global_id == -1){
            --ins;
        }else if (ins != i){
            prob->op[ins] = prob->op[i];
            prob->op[ins].global_id = ins;
        }
    }
    prob->duplicate_ops_removed = prob->op_size - ins;
    prob->op_size = ins;

    BOR_FREE(sorted_ops);
}

static void agentInitProblem(plan_problem_t *dst, const plan_problem_t *src)
{
    int i;
    plan_state_t *state;

    memcpy(dst, src, sizeof(*src));

    dst->var_size = src->var_size;
    dst->var = BOR_ALLOC_ARR(plan_var_t, src->var_size);
    for (i = 0; i < src->var_size; ++i){
        planVarCopy(dst->var + i, src->var + i);
    }

    if (!src->state_pool)
        return;

    dst->state_pool = planStatePoolNew(dst->var, dst->var_size);

    state = planStateNew(src->state_pool->num_vars);
    planStatePoolGetState(src->state_pool, src->initial_state, state);
    dst->initial_state = planStatePoolInsert(dst->state_pool, state);
    planStateDel(state);

    dst->goal = planPartStateNew(dst->state_pool->num_vars);
    planPartStateCopy(dst->goal, src->goal);

    dst->op_size = 0;
    dst->op = NULL;
    dst->succ_gen = NULL;
    dst->agent_name = NULL;
    dst->proj_op = NULL;
    dst->proj_op_size = 0;
    dst->private_val = NULL;
    dst->private_val_size = 0;
}

static void setOpOwner(plan_op_t *ops, int op_size,
                       const plan_problem_agents_t *agents)
{
    int i, j;
    plan_op_t *op;
    std::vector<std::string> name_token;

    // Create name token for each agent
    name_token.resize(agents->agent_size);
    for (i = 0; i < agents->agent_size; ++i){
        name_token[i] = " ";
        name_token[i] += agents->agent[i].agent_name;
    }

    for (i = 0; i < op_size; ++i){
        op = ops + i;

        for (j = 0; j < agents->agent_size; ++j){
            if (strstr(op->name, name_token[j].c_str()) != NULL){
                planOpAddOwner(op, j);
            }
        }

        if (op->ownerarr == 0){
            // if the operator wasn't inserted anywhere, insert it to all
            // agents
            for (j = 0; j < agents->agent_size; ++j){
                planOpAddOwner(op, j);
            }
        }
    }
}

static void setOpPrivate(plan_op_t *ops, int op_size,
                         const AgentVarVals &vals)
{
    plan_op_t *op;

    for (int opi = 0; opi < op_size; ++opi){
        op = ops + opi;
        if (!vals.isPublic(op->eff) && !vals.isPublic(op->pre)){
            op->is_private = 1;
        }
    }
}

static void projectPartState(plan_part_state_t *ps,
                             int agent_id,
                             const AgentVarVals &vals)
{
    plan_var_id_t var;
    plan_val_t val;
    int i;
    std::vector<int> unset;

    PLAN_PART_STATE_FOR_EACH(ps, i, var, val){
        if (!vals.isPublic(var, val) && !vals.used(var, val, agent_id))
            unset.push_back(var);
    }

    for (size_t i = 0; i < unset.size(); ++i)
        planPartStateUnset(ps, unset[i]);
}

static bool projectOp(plan_op_t *op, int agent_id, const AgentVarVals &vals)
{
    projectPartState(op->pre, agent_id, vals);
    projectPartState(op->eff, agent_id, vals);

    if (op->eff->vals_size > 0 || op->pre->vals_size > 0)
        return true;
    return false;
}

static void createProjectedOps(const plan_op_t *ops, int ops_size,
                               int agent_id, plan_problem_t *agent,
                               const AgentVarVals &vals)
{
    plan_op_t *proj_op;

    // Allocate enough memory
    agent->proj_op = BOR_ALLOC_ARR(plan_op_t, ops_size);
    agent->proj_op_size = 0;

    for (int opi = 0; opi < ops_size; ++opi){
        if (ops[opi].ownerarr == 0)
            continue;

        proj_op = agent->proj_op + agent->proj_op_size;

        planOpInit(proj_op, agent->var_size);
        planOpCopy(proj_op, ops + opi);

        if (projectOp(proj_op, agent_id, vals)){
            if (planOpIsOwner(proj_op, agent_id)){
                proj_op->owner = agent_id;
            }else{
                planOpSetFirstOwner(proj_op);
            }
            ++agent->proj_op_size;
        }else{
            planOpFree(proj_op);
        }
    }

    // Given back unused memory
    agent->proj_op = BOR_REALLOC_ARR(agent->proj_op, plan_op_t,
                                     agent->proj_op_size);
}

static void createOps(const plan_op_t *ops, int op_size,
                      int agent_id, plan_problem_t *dst)
{
    int i;
    const plan_op_t *op;

    dst->op = BOR_ALLOC_ARR(plan_op_t, op_size);
    dst->op_size = 0;

    for (i = 0; i < op_size; ++i){
        op = ops + i;
        if (planOpIsOwner(op, agent_id)){
            planOpInit(dst->op + dst->op_size, dst->var_size);
            planOpCopy(dst->op + dst->op_size, op);
            dst->op[dst->op_size].owner = agent_id;
            ++dst->op_size;
        }
    }

    dst->op = BOR_REALLOC_ARR(dst->op, plan_op_t, dst->op_size);
}

static void setPrivateVals(plan_problem_t *agent, int agent_id,
                           const AgentVarVals &vals)
{
    const std::vector<plan_problem_private_val_t> &pv
                = vals.privateVals(agent_id);

    agent->private_val_size = pv.size();
    if (agent->private_val_size == 0)
        return;

    agent->private_val = BOR_ALLOC_ARR(plan_problem_private_val_t,
                                       agent->private_val_size);
    for (size_t i = 0; i < pv.size(); ++i){
        agent->private_val[i] = pv[i];
        planVarSetPrivateVal(agent->var + pv[i].var, pv[i].val);
    }
}
