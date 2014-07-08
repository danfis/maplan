#ifndef AGENT_H
#define AGENT_H

#include "operator.h"
#include "variable.h"
#include "state.h"
#include "mutex_group.h"

struct Agent {
    std::string name;
    int id;
    vector<int> public_ops;  /*!< IDs of public operators */
    vector<int> private_ops; /*!< IDs of private operators */
    vector<Operator *> send_public_ops;
    vector<Operator> projected_ops;
    vector<Operator *> ops; /*!< Array of operators assigned to this agent */
    vector<int> ops_ids;    /*!< IDs of corresponding operators in .ops[]
                                 array */

    void generate_cpp_input(ofstream &outfile) const;
};

void agentsCreate(const std::vector<std::string> &names,
                  std::vector<Variable *> &variables,
                  std::vector<Operator> &operators,
                  vector<pair<Variable *, int> > &goals,
                  std::vector<Agent> &agents);

/**
 * Change ordering of the values within each variable so the first are the
 * public values and then the private.
 */
void changeValueOrdering(std::vector<Variable *> &variables,
                         std::vector<Operator> &ops,
                         State &initial_state,
                         vector<pair<Variable *, int> > &goals,
                         vector<MutexGroup> mutexes);

#endif /* AGENT_H */
