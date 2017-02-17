#!/usr/bin/env python3

import sys
import os
import subprocess

def probIt(topdir):
    for root, dirs, files in os.walk(topdir):
        plans = sorted([x for x in files if x.endswith('.plan')])
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

def testHeur(heur, prob_proto, plan_fn):
    cmd = ['./test-heur', heur, prob_proto, plan_fn]
    p = subprocess.Popen(cmd, stdout = subprocess.PIPE)
    output = p.communicate()[0]
    output = output.decode('ascii').split('\t')
    build_time = float(output[0])
    path = [x.split(':') for x in output[1:]]
    path = [(int(x[0]), x[1], float(x[2])) for x in path]
    return build_time, path

def main(heur, topdir):
    failed = []
    for dom, prob, dom_pddl, prob_pddl, prob_plan in probIt(topdir):
        print(dom, prob, end = ': ')
        sys.stdout.flush()
        translate(dom_pddl, prob_pddl, 'problem.proto')
        optimal_hval = optimalCost(prob_plan)

        build_time, path = testHeur(heur, 'problem.proto', prob_plan)
        print('build time: {0}s'.format(build_time))
        for p in path:
            optimal_hval -= p[0]
            if p[1] == 'DE' or int(p[1]) > optimal_hval:
                ok = 'FAIL'
                failed += [(dom, prob)]
            else:
                ok = 'OK'
            print('    optimal: {0}, computed: {1}, time: {2}s ==> {3}' \
                    .format(optimal_hval, p[1], p[2], ok))
        sys.stdout.flush()
        os.unlink('problem.proto')

    if len(failed) > 0:
        print('FAILED:')
        for f in failed:
            print('    {0} {1}'.format(f[0], f[1]))
        return -1
    return 0

if __name__ == '__main__':
    if len(sys.argv) != 3:
        print('Usage: {0} heur bench-dir'.format(sys.argv[0]))
        sys.exit(-1)
    sys.exit(main(sys.argv[1], sys.argv[2]))
