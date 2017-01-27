#!/usr/bin/env python3

import sys
import os
import subprocess

def probIt(topdir):
    for root, dirs, files in os.walk(topdir):
        plans = [x for x in files if x.endswith('.plan')]
        if len(plans) == 0:
            continue

        probs = [x[:-5] for x in plans]
        for prob_name in probs:
            dom_name = os.path.split(root.strip('/'))[-1]

            prob = prob_name + '.pddl'
            doms = [prob_name + '-domain.pddl',
                    'domain_' + prob,
                    'domain.pddl']
            if prob.find('problem') != -1:
                doms += [prob.replace('problem', 'domain')]
            doms = [x for x in doms if os.path.isfile(os.path.join(root, x))]
            dom = doms[0]
            out = '{0}--{1}.proto'.format(dom_name, prob_name)
            yield dom_name, prob_name, \
                  os.path.join(root, dom), \
                  os.path.join(root, prob), \
                  os.path.join(root, prob_name + '.plan')

def translate(dom_pddl, prob_pddl, prob_proto):
    cmd = 'python2 ../third-party/translate/translate.py'
    cmd += ' --proto --output {0}'.format(prob_proto)
    cmd += ' ' + dom_pddl
    cmd += ' ' + prob_pddl
    cmd += ' >translate.log 2>translate.log'
    if os.system(cmd) != 0:
        raise Exception('translation failed -- see translate.log!')

def optimalCost(prob_plan):
    for line in open(prob_plan, 'r'):
        if line.find('Optimal cost:') != -1:
            p = line.strip().split()[-1]
            return int(p)

def testHeur(heur, prob_proto):
    cmd = ['./test-heur', heur, prob_proto]
    p = subprocess.Popen(cmd, stdout = subprocess.PIPE)
    output = p.communicate()[0]
    output = output.decode('ascii').split('\t')
    return int(output[0]), float(output[1]), float(output[2])

def main(heur, topdir):
    for dom, prob, dom_pddl, prob_pddl, prob_plan in probIt(topdir):
        print(dom, prob, end = ': ')
        sys.stdout.flush()
        translate(dom_pddl, prob_pddl, 'problem.proto')
        optimal_hval = optimalCost(prob_plan)
        print('optimal: {0}'.format(optimal_hval), end = '')
        sys.stdout.flush()
        hval, time, build_time = testHeur(heur, 'problem.proto')
        print(', value: {0}, time: {1:.6f}/{2:.6f}' \
                .format(hval, time, build_time), end = '')
        sys.stdout.flush()
        if hval <= optimal_hval:
            print(' ==> OK')
        else:
            print(' ==> FAIL!')
            sys.exit(-1)
        sys.stdout.flush()

if __name__ == '__main__':
    if len(sys.argv) != 3:
        print('Usage: {0} heur bench-dir'.format(sys.argv[0]))
        sys.exit(-1)
    sys.exit(main(sys.argv[1], sys.argv[2]))
