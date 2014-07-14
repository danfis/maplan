#ifndef STATE_H
#define STATE_H

#include <iostream>
#include <map>
#include <vector>
using namespace std;

class Variable;

class State {
    map<Variable *, int> values;
public:
    State() {} // TODO: Entfernen (erfordert kleines Redesign)
    State(istream &in, const vector<Variable *> &variables);

    int operator[](Variable *var) const;
    void set_value(Variable *var, int value);
    void dump() const;
};

#endif
