#ifndef OPERATOR_H
#define OPERATOR_H

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <set>
using namespace std;

class Variable;

class Operator {
public:
    struct Prevail {
        Variable *var;
        int prev;
        Prevail(Variable *v, int p) : var(v), prev(p) {}
    };
    struct EffCond {
        Variable *var;
        int cond;
        EffCond(Variable *v, int c) : var(v), cond(c) {}
    };
    struct PrePost {
        Variable *var;
        int pre, post;
        bool is_conditional_effect;
        vector<EffCond> effect_conds;
        PrePost(Variable *v, int pr, int po) : var(v), pre(pr), post(po) {
            is_conditional_effect = false;
        }
        PrePost(Variable *v, vector<EffCond> ecs, int pr, int po) : var(v), pre(pr),
                                                                    post(po), effect_conds(ecs) {is_conditional_effect = true; }
    };

    string name;
    int owner;
    vector<Prevail> prevail;    // var, val
    vector<PrePost> pre_post; // var, old-val, new-val
    int cost;
    int id;
public:
    std::set<int> send_agents;
    Operator(istream &in, const vector<Variable *> &variables);

    void strip_unimportant_effects();
    bool is_redundant() const;

    void dump() const;
    int get_encoding_size() const;
    void generate_cpp_input(ofstream &outfile) const;
    int get_cost() const {return cost; }
    string get_name() const {return name; }
    const vector<Prevail> &get_prevail() const {return prevail; }
    vector<Prevail> &get_prevail() {return prevail; }
    const vector<PrePost> &get_pre_post() const {return pre_post; }
    vector<PrePost> &get_pre_post() {return pre_post; }
    void set_id(int id) { this->id = id; }
    int get_id() const { return id; }

    void set_owner(int o) { owner = o; }
    int get_owner() const { return owner; }
};

extern void strip_operators(vector<Operator> &operators);

#endif
