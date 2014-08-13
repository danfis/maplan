#ifndef VARIABLE_H
#define VARIABLE_H

#include <iostream>
#include <vector>
using namespace std;

class Variable {
    vector<string> values;
    string name;
    int layer;
    int level;
    bool necessary;
    vector<bool> value_public;
    // true if an agent use the value as a precondition or an effect
    vector<vector<bool> > agent_use;
    // true if an agent use the value as a precondition
    vector<vector<bool> > agent_pre_use;
public:
    Variable(istream &in);
    void set_level(int level);
    void set_necessary();
    int get_level() const;
    bool is_necessary() const;
    int get_range() const;
    string get_name() const;
    int get_layer() const {return layer; }
    bool is_derived() const {return layer != -1; }
    void generate_cpp_input(ofstream &outfile) const;
    void dump() const;
    string get_fact_name(int value) const {return values[value]; }

    bool is_value_public(int value) const
        { return value_public[value]; }
    void set_value_public(int value, bool pub = true)
        { value_public[value] = pub; }
    void set_num_agents(int num);
    void set_agent_use(int value, int agent)
        { agent_use[value][agent] = true; }
    void set_agent_pre_use(int value, int agent)
        { agent_pre_use[value][agent] = true; }
    bool is_value_used_by_agent(int value, int agent) const
        { return agent_use[value][agent]; }
    bool is_value_used_by_agent_as_pre(int value, int agent) const
        { return agent_pre_use[value][agent]; }
    int get_num_agent_use(int value);

    void reorderValues(std::vector<int> &translate);
};

#endif
