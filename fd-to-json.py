#!/usr/bin/env python

import sys
import json

def assertTerm(term, exp):
    if term != exp:
        msg = 'Expecting `{0}\', got `{1}\' instead'.format(exp, term)
        raise AssertionError(msg)


def loadVersion(data, json_data):
    assertTerm(data[0], "begin_version")
    version = int(data[1])
    assertTerm(data[2], "end_version")

    json_data['version'] = version

    return data[3:], version

def loadMetric(data, json_data):
    assertTerm(data[0], "begin_metric")
    json_data['use_metric'] = bool(int(data[1]))
    assertTerm(data[2], "end_metric")
    return data[3:]

def loadVariable(data):
    assertTerm(data[0], 'begin_variable')

    var = { 'name'      : data[1],
            'layer'     : int(data[2]),
            'range'     : int(data[3]),
            'fact_name' : []
          }
    for i in range(var['range']):
        var['fact_name'].append(data[i + 4])
    assertTerm(data[4 + var['range']], 'end_variable')

    return data[(5 + var['range']):], var

def loadVariables(data, json_data):
    count = int(data[0])
    data = data[1:]

    json_data['variable'] = []
    for i in range(count):
        data, var = loadVariable(data)
        json_data['variable'].append(var)

    return data

def loadMutexGroup(data):
    assertTerm(data[0], 'begin_mutex_group')
    count = int(data[1])

    mutex_group = {}
    data = data[2:]
    for i in range(count):
        var, val = [int(x) for x in data[0].split()]
        if var in mutex_group:
            raise Exception('{0} already in mutex_group'.format(var))
        mutex_group[var] = val
        data = data[1:]

    assertTerm(data[0], 'end_mutex_group')

    return data[1:], mutex_group

def loadMutexes(data, json_data):
    count = int(data[0])
    data = data[1:]

    json_data['mutex_group'] = []
    for i in range(count):
        data, mutex_group = loadMutexGroup(data)
        json_data['mutex_group'].append(mutex_group)

    return data

def loadInitialState(data, json_data):
    assertTerm(data[0], 'begin_state')
    data = data[1:]

    vals = data[:len(json_data['variable'])]
    json_data['initial_state'] = [int(x) for x in vals]
    data = data[len(json_data['variable']):]

    assertTerm(data[0], 'end_state')
    return data[1:]

def loadGoal(data, json_data):
    assertTerm(data[0], 'begin_goal')
    data = data[1:]

    count = int(data[0])
    data = data[1:]

    json_data['goal'] = {}
    for i in range(count):
        var, val = [int(x) for x in data[0].split()]
        json_data['goal'][var] = val
        data = data[1:]

    assertTerm(data[0], 'end_goal')
    return data[1:]

def loadPrevail(data, json_data):
    var, val = [int(x) for x in data[0].split()]
    json_data[var] = val
    return data[1:]

def loadPrePost(data, json_data):
    line = data[0].split()

    prevail_count = int(line[0])
    line = line[1:]
    for j in range(prevail_count):
        # TODO: load prevails as before
        raise Exception('Loading per-pre_post prevail not implemented.')
        pass

    var, pre, post = [int(x) for x in line]
    json_data[var] = { 'pre'  : pre,
                       'post' : post }
    return data[1:]

def loadOperator(data, use_metric):
    assertTerm(data[0], 'begin_operator')
    data = data[1:]

    operator = { 'name'     : data[0],
                 'prevail'  : {},
                 'pre_post' : {},
                 'cost'     : 0
               }
    data = data[1:]

    prevail_count = int(data[0])
    data = data[1:]
    for i in range(prevail_count):
        data = loadPrevail(data, operator['prevail'])

    pre_post_count = int(data[0])
    data = data[1:]
    for i in range(pre_post_count):
        data = loadPrePost(data, operator['pre_post'])

    if use_metric:
        operator['cost'] = int(data[0])
    else:
        operator['cost'] = 1

    data = data[1:]
    assertTerm(data[0], 'end_operator')

    return data[1:], operator

def loadOperators(data, json_data):
    count = int(data[0])
    data = data[1:]

    json_data['operator'] = []
    for i in range(count):
        data, operator = loadOperator(data, json_data['use_metric'])
        json_data['operator'].append(operator)
    return data

def loadAxiom(data):
    assertTerm(data[0], 'begin_rule')
    data = data[1:]

    axiom = { 'pre_post' : {} }
    data = loadPerPost(data, axiom['pre_post'])

    assertTerm(data[0], 'end_rule')
    return data[1:], axiom

def loadAxioms(data, json_data):
    count = int(data[0])
    data = data[1:]

    json_data['axiom'] = []
    for i in range(count):
        data, axiom = loadAxiom(data)
        json_data['axiom'].append(axiom)

    return data

def loadSuccessorGeneratorRec(data, var):
    sg_type, v = data[0].split()
    data = data[1:]

    if sg_type == 'switch':
        sg = { 'var'       : int(v),
               'operators' : [],
               'switch'    : {},
               'default'   : None,
             }

        data, sg['operators'] = loadSuccessorGeneratorRec(data, var)
        for i in range(var[sg['var']]['range']):
            data, switch = loadSuccessorGeneratorRec(data, var)
            sg['switch'][i] = switch

        data, sg['default'] = loadSuccessorGeneratorRec(data, var)
        return data, sg

    elif sg_type == 'check':
        count = int(v)
        if count == 0:
            operators = []
        else:
            operators = [int(x) for x in data[:count]]
            data = data[count:]
        return data, operators

    else:
        raise Exception('Unkown successor generator type')

def loadSuccessorGenerator(data, json_data):
    assertTerm(data[0], "begin_SG")
    data = data[1:]

    data, sg = loadSuccessorGeneratorRec(data, json_data['variable'])
    json_data['successor_generator'] = sg

    assertTerm(data[0], "end_SG")
    return data[1:]

def loadDomainTransitionGraph(data, var, var_range):
    assertTerm(data[0], 'begin_DTG')
    data = data[1:]

    dtg = { 'variable'    : var,
            'transitions' : [],
          }

    for val in range(var_range):
        trans = []

        trans_count = int(data[0])
        data = data[1:]

        for i in range(trans_count):
            target = int(data[0])
            operator = int(data[1])
            data = data[2:]

            t = { 'target'       : target,
                  'operator'     : operator,
                  'precondition' : {}
                }

            precond_count = int(data[0])
            data = data[1:]
            for j in range(precond_count):
                var, val = [int(x) for x in data[0].split()]
                data = data[1:]
                t['precondition'][var] = val

            trans.append(t)

        dtg['transitions'].append(trans)


    assertTerm(data[0], 'end_DTG')
    return data[1:], dtg

def loadDomainTransitionGraphs(data, json_data):
    count = len(json_data['variable'])

    dtg = {}
    for i in range(count):
        var = i
        var_range = json_data['variable'][i]['range']
        data, dtg[i] = loadDomainTransitionGraph(data, var, var_range)
    json_data['domain_transition_graph'] = dtg

    return data

def loadCausalGraph(data, json_data):
    assertTerm(data[0], "begin_CG")
    data = data[1:]

    cg = []
    for from_node in range(len(json_data['variable'])):
        node = []

        num_succ = int(data[0])
        data = data[1:]
        for i in range(num_succ):
            to, weight = [int(x) for x in data[0].split()]
            n = { 'to'     : to,
                  'weight' : weight }
            node.append(n)
            data = data[1:]

        cg.append(node)

    json_data['causal_graph'] = cg

    assertTerm(data[0], "end_CG")
    return data[1:]

def main():
    # output json data
    json_data = {}

    # load whole input file a list of terms
    data = sys.stdin.read().split('\n')

    data, version = loadVersion(data, json_data)
    if version != 3:
        print('Expected version {0}, got {1}. I don\'t know how to parse' \
              ' this version.')

    data = loadMetric(data, json_data)
    data = loadVariables(data, json_data)
    data = loadMutexes(data, json_data)
    data = loadInitialState(data, json_data)
    data = loadGoal(data, json_data)
    data = loadOperators(data, json_data)
    data = loadAxioms(data, json_data)
    data = loadSuccessorGenerator(data, json_data)
    data = loadDomainTransitionGraphs(data, json_data)
    data = loadCausalGraph(data, json_data)

    print(json.dumps(json_data))


if __name__ == '__main__':
    if len(sys.argv) > 1:
        print('Usage: {0} <in.sas >out.json'.format(sys.argv[0]))
        sys.exit(-1)

    main()
