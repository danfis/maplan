#include <algorithm>
#include "variable.h"

#include "helper_functions.h"

#include <cassert>
using namespace std;

Variable::Variable(istream &in) {
    int range;
    check_magic(in, "begin_variable");
    in >> ws >> name >> layer >> range >> ws;
    values.resize(range);
    for (int i = 0; i < range; ++i)
        getline(in, values[i]);
    check_magic(in, "end_variable");
    level = -1;
    necessary = false;
    value_public.resize(range, false);
}

void Variable::set_level(int theLevel) {
    assert(level == -1);
    level = theLevel;
}

int Variable::get_level() const {
    return level;
}

void Variable::set_necessary() {
    assert(necessary == false);
    necessary = true;
}

int Variable::get_range() const {
    return values.size();
}

string Variable::get_name() const {
    return name;
}

bool Variable::is_necessary() const {
    return necessary;
}

void Variable::dump() const {
    // TODO: Dump values (and other information that might be missing?)
    //       or get rid of this if it's no longer needed.
    cout << name << " [range " << get_range();
    if (level != -1)
        cout << "; level " << level;
    if (is_derived())
        cout << "; derived; layer: " << layer;
    cout << "]" << endl;
}

void Variable::generate_cpp_input(ofstream &outfile) const {
    outfile << "begin_variable" << endl
            << name << endl
            << layer << endl
            << values.size() << endl;
    for (size_t i = 0; i < values.size(); ++i)
        outfile << values[i] << endl;
    outfile << "end_variable" << endl;
}

void Variable::set_num_agents(int num)
{
    int range = get_range();

    agent_use.resize(range);
    for (int i = 0; i < range; ++i){
        agent_use[i].resize(num, false);
    }
}

int Variable::get_num_agent_use(int value)
{
    int num = 0;
    for (size_t i = 0; i < agent_use[value].size(); ++i){
        if (agent_use[value][i]){
            num += 1;
        }
    }

    return num;
}

struct ReorderCmp {
    const vector<bool> &pub;

    ReorderCmp(const vector<bool> &value_public)
        : pub(value_public)
    {}

    bool operator()(const int &a, const int &b)
    {
        return pub[a] && !pub[b];
    }
};

void Variable::reorderValues(std::vector<int> &translate)
{
    int range = get_range();
    std::vector<int> vals;

    /*
    std::cerr << "V " << name << std::endl;
    for (size_t i = 0; i < values.size(); ++i){
        std::cerr << " " << i
                  << " " << values[i]
                  << " " << value_public[i]
                  << std::endl;
    }
    */

    vals.resize(range);
    for (int i = 0; i < range; ++i)
        vals[i] = i;

    ReorderCmp cmp(value_public);
    std::stable_sort(vals.begin(), vals.end(), cmp);

    translate.resize(range);
    for (size_t i = 0; i < vals.size(); ++i){
        translate[vals[i]] = i;
    }

    vector<string> values_src(values.begin(), values.end());
    vector<bool> value_public_src(value_public.begin(), value_public.end());
    vector<vector<bool> > agent_use_src(agent_use.begin(), agent_use.end());
    for (size_t i = 0; i < translate.size(); ++i){
        values[translate[i]] = values_src[i];
        value_public[translate[i]] = value_public_src[i];
        agent_use[translate[i]] = agent_use_src[i];
    }

    /*
    std::cerr << "V " << name << std::endl;
    for (size_t i = 0; i < values.size(); ++i){
        std::cerr << " " << i
                  << " " << values[i]
                  << " " << value_public[i]
                  << std::endl;
    }
    std::cerr << std::endl;
    */
}
