#include <map>

#include "agent.h"

namespace {

void splitOperators(std::vector<Operator> &ops,
                    std::vector<Agent> &agents)
{
    for (size_t i = 0; i < ops.size(); ++i){
        Operator *op = &ops[i];

        int inserted = 0;
        for (size_t j = 0; j < agents.size(); ++j){
            Agent &agent = agents[j];
            if (op->get_name().find(agent.name) != std::string::npos){
                agent.ops.push_back(op);
                agent.ops_ids.push_back(i);
                op->set_owner(agent.id);
                inserted++;
            }
        }

        if (inserted == 0){
            // if the operator wasn't inserted anywhere, insert it to all
            // agents
            for (size_t j = 0; j < agents.size(); ++j){
                agents[j].ops.push_back(op);
                agents[j].ops_ids.push_back(i);
                op->set_owner(-1);
            }
        }
    }
}


void setPublicVariableValues(const std::vector<Agent> &agents,
                             const vector<pair<Variable *, int> > goals,
                             std::vector<Variable *> &variables)
{
    // prepare space for use flags in variable objects
    for (size_t i = 0; i < variables.size(); ++i){
        variables[i]->set_num_agents(agents.size());
    }

    // set goals as public for all agents
    for (size_t i = 0; i < goals.size(); ++i){
        for (size_t ai = 0; ai < agents.size(); ++ai){
            goals[i].first->set_agent_use(goals[i].second, ai);
        }
    }

    // set use flags to the variables and their values
    for (size_t agenti = 0; agenti < agents.size(); ++agenti){
        const Agent &agent = agents[agenti];
        for (size_t opi = 0; opi < agent.ops.size(); ++opi){
            Operator *op = agent.ops[opi];

            const vector<Operator::Prevail> &prevail = op->get_prevail();
            for (size_t i = 0; i < prevail.size(); ++i){
                prevail[i].var->set_agent_use(prevail[i].prev, agenti);
            }

            const vector<Operator::PrePost> &pre_post = op->get_pre_post();
            for (size_t i = 0; i < pre_post.size(); ++i){
                if (pre_post[i].pre >= 0)
                    pre_post[i].var->set_agent_use(pre_post[i].pre, agenti);
                if (pre_post[i].post >= 0)
                    pre_post[i].var->set_agent_use(pre_post[i].post, agenti);
            }
        }
    }

    // set as public those values that are used by more than one agent
    for (size_t i = 0; i < variables.size(); ++i){
        Variable *var = variables[i];
        int range = var->get_range();
        //std::cerr << "Var: " << var->get_name() << std::endl;
        for (int value = 0; value < range; ++value){
            if (var->get_num_agent_use(value) > 1)
                var->set_value_public(value);

            /*
            std::cerr << "  [" << value << "]: "
                      << var->get_num_agent_use(value)
                      << " " << var->is_value_public(value)
                      << std::endl;
            */
        }
    }
}

/** Returns true if the operator uses public value in effect or
 *  precondition.*/
bool operatorUsePublicValue(const Operator &op)
{
    const vector<Operator::Prevail> &prevail = op.get_prevail();
    const vector<Operator::PrePost> &pre_post = op.get_pre_post();

    for (size_t i = 0; i < prevail.size(); ++i){
        if (prevail[i].var->is_value_public(prevail[i].prev))
            return true;
    }

    for (size_t i = 0; i < pre_post.size(); ++i){
        if (pre_post[i].pre >= 0
                && pre_post[i].var->is_value_public(pre_post[i].pre)){
            return true;
        }

        if (pre_post[i].post >= 0
                && pre_post[i].var->is_value_public(pre_post[i].post)){
            return true;
        }
    }

    return false;
}

/** Returns true if the operator changes any variable to a public value. */
bool operatorChangeToPublicValue(const Operator &op)
{
    const vector<Operator::PrePost> &pre_post = op.get_pre_post();

    for (size_t i = 0; i < pre_post.size(); ++i){
        if (pre_post[i].post >= 0
                && pre_post[i].var->is_value_public(pre_post[i].post)){
            return true;
        }
    }

    return false;
}


void setPublicOps(std::vector<Agent> &agents)
{
    for (size_t i = 0; i < agents.size(); ++i){
        Agent &agent = agents[i];

        std::cerr << "Agent " << agent.name << std::endl;
        for (size_t i = 0; i < agent.ops.size(); ++i){
            if (operatorUsePublicValue(*agent.ops[i])){
                agent.public_ops.push_back(agent.ops_ids[i]);
                //std::cerr << "Pub Op: " << agent.ops[i]->get_name() << std::endl;
            }else{
                agent.private_ops.push_back(agent.ops_ids[i]);
                //std::cerr << "Pri Op: " << agent.ops[i]->get_name() << std::endl;
            }

            if (operatorChangeToPublicValue(*agent.ops[i])){
                agent.send_public_ops.push_back(agent.ops[i]);
                //std::cerr << "SEND" << std::endl;
            }
        }
    }
}

void projectOpPrevail(const Agent &agent, Operator &op)
{
    std::vector<Operator::Prevail> new_prevail;
    std::vector<Operator::Prevail> &prevail = op.prevail;

    for (size_t i = 0; i < prevail.size(); ++i){
        Operator::Prevail &pr = prevail[i];

        if (pr.var->is_value_public(pr.prev)
                || pr.var->is_value_used_by_agent(pr.prev, agent.id)){
            new_prevail.push_back(pr);
        }
    }

    op.prevail = new_prevail;
}

void projectOpPrePost(const Agent &agent, Operator &op)
{
    std::vector<Operator::PrePost> new_prepost;
    std::vector<Operator::PrePost> &prepost = op.pre_post;

    for (size_t i = 0; i < prepost.size(); ++i){
        Operator::PrePost &pp = prepost[i];

        if (pp.pre != -1
                && !pp.var->is_value_public(pp.pre)
                && !pp.var->is_value_used_by_agent(pp.pre, agent.id)){
            pp.pre = -1;
        }

        if (pp.post != -1
                && !pp.var->is_value_public(pp.post)
                && !pp.var->is_value_used_by_agent(pp.post, agent.id)){
            pp.post = -1;
        }

        if (pp.post != -1){
            new_prepost.push_back(pp);
        }else if (pp.pre != -1){
            Operator::Prevail pr(pp.var, pp.pre);
            op.prevail.push_back(pr);
        }
    }

    op.pre_post = new_prepost;
}

void projectOp(const Agent &agent, Operator &op)
{
    projectOpPrevail(agent, op);
    projectOpPrePost(agent, op);
}

void createProjectedOps(Agent &agent, std::vector<Operator> &ops)
{
    for (size_t i = 0; i < ops.size(); ++i){
        // copy operator
        Operator op = ops[i];

        projectOp(agent, op);
        if (op.pre_post.size() > 0)
            agent.projected_ops.push_back(op);
    }
}

void pOp(const Operator &op)
{
    for (size_t i = 0; i < op.prevail.size(); ++i){
        std::cerr << " " << op.prevail[i].var->get_level()
                  << ":" << op.prevail[i].prev
                  << ":" << op.prevail[i].prev;
    }

    for (size_t i = 0; i < op.pre_post.size(); ++i){
        std::cerr << " " << op.pre_post[i].var->get_level()
                  << ":" << op.pre_post[i].pre
                  << ":" << op.pre_post[i].post;
    }
}

} /* anon namespace */

void agentsCreate(const std::vector<std::string> &names,
                  std::vector<Variable *> &variables,
                  std::vector<Operator> &operators,
                  vector<pair<Variable *, int> > &goals,
                  std::vector<Agent> &agents)
{
    // set ID to operators
    for (size_t i = 0; i < operators.size(); ++i){
        operators[i].set_id(i);
    }

    // create agents and set their names
    for (size_t i = 0; i < names.size(); ++i){
        Agent agent;
        agent.name = names[i];
        agent.id   = i;
        agents.push_back(agent);
    }

    // split operators between agents
    splitOperators(operators, agents);

    // set variable values as public if they are used by more than one
    // agent
    setPublicVariableValues(agents, goals, variables);

    // determine public operators for each agent
    setPublicOps(agents);

    for (size_t i = 0; i < names.size(); ++i){
        createProjectedOps(agents[i], operators);
    }

    /*
    for (size_t i = 0; i < names.size(); ++i){
        std::cerr << "Agent: " << i << " " << agents[i].name << std::endl;
        for (size_t j = 0; j < agents[i].projected_ops.size(); ++j){
            Operator &op = agents[i].projected_ops[j];
            std::cerr << op.name;
            pOp(op);
            std::cerr << std::endl;
        }
    }
    */
}

void changeValueOrdering(std::vector<Variable *> &variables,
                         std::vector<Operator> &ops,
                         State &initial_state,
                         vector<pair<Variable *, int> > &goals,
                         vector<MutexGroup> mutexes)
{
    std::map<Variable *, std::vector<int> > trans;

    for (size_t i = 0; i < variables.size(); ++i){
        std::vector<int> t;
        variables[i]->reorderValues(t);
        trans.insert(std::make_pair(variables[i], t));
    }

    // fix operators
    for (size_t i = 0; i < ops.size(); ++i){
        std::vector<Operator::Prevail> &prevail = ops[i].get_prevail();
        for (size_t j = 0; j < prevail.size(); ++j){
            prevail[j].prev = trans[prevail[j].var][prevail[j].prev];
        }

        std::vector<Operator::PrePost> &pre_post = ops[i].get_pre_post();
        for (size_t j = 0; j < pre_post.size(); ++j){
            if (pre_post[j].pre >= 0)
                pre_post[j].pre = trans[pre_post[j].var][pre_post[j].pre];
            if (pre_post[j].post >= 0)
                pre_post[j].post = trans[pre_post[j].var][pre_post[j].post];
        }
    }

    // fix initial state
    for (size_t i = 0; i < variables.size(); ++i){
        Variable *v = variables[i];
        initial_state.set_value(v, trans[v][initial_state[v]]);
    }

    // fix goal
    for (size_t i = 0; i < goals.size(); ++i){
        goals[i].second = trans[goals[i].first][goals[i].second];
    }

    // fix mutexes
    for (size_t i = 0; i < mutexes.size(); ++i){
        vector<pair<const Variable *, int> > &facts = mutexes[i].get_facts();
        for (size_t j = 0; j < facts.size(); ++j){
            facts[j].second = trans[(Variable *)facts[j].first][facts[j].second];
        }
    }
}

void Agent::generate_cpp_input(ofstream &outfile) const
{
    outfile << "begin_agent" << endl;
    outfile << name << endl;
    outfile << id << endl;

    outfile << "begin_agent_private_operators" << endl;
    outfile << private_ops.size() << endl;
    for (size_t i = 0; i < private_ops.size(); ++i){
        outfile << private_ops[i] << endl;
    }
    outfile << "end_agent_private_operators" << endl;

    outfile << "begin_agent_public_operators" << endl;
    outfile << public_ops.size() << endl;
    for (size_t i = 0; i < public_ops.size(); ++i){
        outfile << public_ops[i] << endl;
    }
    outfile << "end_agent_public_operators" << endl;

    // TODO: send_public_ops.

    outfile << "begin_agent_projected_operators" << endl;
    outfile << projected_ops.size() << endl;
    for (size_t i = 0; i < projected_ops.size(); ++i){
        projected_ops[i].generate_cpp_input(outfile);
        outfile << projected_ops[i].get_id() << endl;
        outfile << projected_ops[i].get_owner() << endl;
    }
    outfile << "end_agent_projected_operators" << endl;

    outfile << "end_agent" << endl;
}
