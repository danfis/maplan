#!/usr/bin/env python2.7

import sys
import os
import re
import copy
import cPickle as pickle
import shutil
import argparse
from pprint import pprint

PYTHON = '/usr/bin/python2'
SEARCH = '/home/danfis/dev/libplan/bin/search'
TRANSLATE = '/home/danfis/dev/libplan/third-party/translate/translate.py'
VALIDATE = '/home/danfis/dev/libplan/third-party/VAL/validate'
BENCHMARKS_DIR = '/home/danfis/dev/plan-data/benchmarks'
SEARCH_REPO = 'git@gitlab.fel.cvut.cz:fiserdan/libplan.git'


def checkBin(path):
    if not os.path.isfile(path):
        print('Error: Could not find `{0}\''.format(path))
        return False
    return True

def runCmd(cmd):
    print('CMD: {0}'.format(cmd))
    if os.system(cmd) != 0:
        sys.exit(-1)

def planCheckResources(args):
    if not checkBin(args.python_bin):
        return False
    if os.path.isdir(args.name) or os.path.isfile(args.name):
        print('Error: Bench `{0}\' already exists'.format(args.name))
        return False
    if args.bench_name not in BENCHMARKS:
        print('Error: Unkown benchmark `{0}\''.format(args.bench_name))
        return False
    if len(args.branch) > 0:
        return True

    if not checkBin(args.search_bin):
        return False
    if not checkBin(args.validate_bin):
        return False
    if not checkBin(args.translate):
        return False
    return True

def planFindDefFiles(bench_dir, domain, problem):
    pddl = problem + '.pddl'

    if os.path.isfile(os.path.join(bench_dir, domain, 'domain-' + pddl)):
        domain_pddl = os.path.join(bench_dir, domain, 'domain-' + pddl)
    elif os.path.isfile(os.path.join(bench_dir, domain, problem + '-domain.pddl')):
        domain_pddl = os.path.join(bench_dir, domain, problem + '-domain.pddl')
    elif os.path.isfile(os.path.join(bench_dir, domain, 'domain.pddl')):
        domain_pddl = os.path.join(bench_dir, domain, 'domain.pddl')
    else:
        print('Error: Could not find domain PDDL for problem/domain `{0}/{1}\''.format(problem, domain))
        sys.exit(-1)

    if os.path.isfile(os.path.join(bench_dir, domain, pddl)):
        pddl = os.path.join(bench_dir, domain, pddl)
    elif os.path.isfile(os.path.join(bench_dir, domain, 'problem-' + pddl)):
        pddl = os.path.join(bench_dir, domain, 'problem-' + pddl)
    else:
        print('Error: Could not find problem `{0}\' in domain `{1}\''.format(problem, domain))
        sys.exit(-1)

    addl = None
    if os.path.isfile(pddl[:-4] + 'addl'):
        addl = pddl[:-4] + 'addl'

    proto = None
    if os.path.isfile(pddl[:-4] + 'proto'):
        proto = pddl[:-4] + 'proto'
    
    return domain_pddl, pddl, addl, proto

pat_ma_agents = re.compile(r'^.*\(:agents +([a-zA-Z0-9-_ ]+)\).*$')
def planMAAgents(addl):
    with open(addl, 'r') as fin:
        data = fin.read()
        data = data.replace('\r\n', ' ')
        data = data.replace('\n', ' ')
        data = data.replace('\r', ' ')
        match = pat_ma_agents.match(data)
        if match is None:
            print('Error: Could not parse `{0}\''.format(addl))
        agents = match.group(1).split()
        agents = filter(lambda x: len(x) > 0, agents)
        return len(agents)

def planLoadProblems(args):
    problems = []

    benchmarks = BENCHMARKS[args.bench_name]
    for bench in benchmarks:
        domain = bench['domain']
        for problem in bench['problems']:
            domain_pddl, pddl, addl, proto \
                = planFindDefFiles(args.bench_dir, domain, problem)

            ma_agents = 0
            if addl is not None and args.ma:
                ma_agents = planMAAgents(addl)

            for r in range(args.repeats):
                rel_dir = '{0}:{1}:{2:02d}'.format(domain, problem, r)
                d = {}
                d['domain_name']  = domain
                d['problem_name'] = problem
                d['dir']          = rel_dir
                d['repeat']       = r
                d['domain_pddl']  = domain_pddl
                d['pddl']         = pddl
                d['addl']         = addl
                d['ma']           = args.ma
                d['ma-agents']    = ma_agents
                d['proto']        = proto
                problems.append(d)

    return problems

def planCloneRepo(args, bench_root):
    args.search_bin = 'asdf'
    root = os.path.join(bench_root, 'repo')
    runCmd('git clone {0} {1}'.format(SEARCH_REPO, root))
    runCmd('cd {0} && git checkout -b bench origin/{1}'.format(root, args.branch))
    runCmd('cd {0} && CFLAGS="-O3 -march=corei7" CXXFLAGS="-O3 -march=corei7" make third-party'.format(root))
    runCmd('cd {0} && CFLAGS="-O3 -march=corei7" CXXFLAGS="-O3 -march=corei7" make'.format(root))
    runCmd('cd {0} && CFLAGS="-O3 -march=corei7" CXXFLAGS="-O3 -march=corei7" make -C bin'.format(root))

    args.search_bin = os.path.join(root, 'bin/search')
    if not os.path.isfile(args.search_bin):
        print('Error: Could not find `{0}\''.format(args.search_bin))
        sys.exit(-1)

    args.validate_bin = os.path.join(root, 'third-party/VAL/validate')
    if not os.path.isfile(args.validate_bin):
        print('Error: Could not find `{0}\''.format(args.validate_bin))
        sys.exit(-1)

    args.translate = os.path.join(root, 'third-party/translate/translate.py')
    if not os.path.isfile(args.translate):
        print('Error: Could not find `{0}\''.format(args.translate))
        sys.exit(-1)


def planPrepareDisk(args, problems_in):
    root = os.path.join(os.path.abspath('.'), args.name)
    os.mkdir(root)

    if len(args.branch) > 0:
        planCloneRepo(args, root)

    problems = []
    for problem in problems_in:
        d = copy.copy(problem)
        d['dir'] = os.path.join(root, problem['dir'])
        d['domain_pddl'] = os.path.basename(problem['domain_pddl'])
        d['domain_pddl'] = os.path.join(d['dir'], d['domain_pddl'])
        d['pddl'] = os.path.basename(problem['pddl'])
        d['pddl'] = os.path.join(d['dir'], d['pddl'])
        if d['addl']:
            d['addl'] = os.path.basename(problem['addl'])
            d['addl'] = os.path.join(d['dir'], d['addl'])
        if d['proto']:
            d['proto'] = os.path.basename(problem['proto'])
            d['proto'] = os.path.join(d['dir'], d['proto'])

        os.mkdir(d['dir'])
        shutil.copy(problem['domain_pddl'], d['domain_pddl'])
        shutil.copy(problem['pddl'], d['pddl'])
        if d['addl']:
            shutil.copy(problem['addl'], d['addl'])
        if d['proto']:
            shutil.copy(problem['proto'], d['proto'])

        problems.append(d)
    return problems

def planTasks(args, problems):
    search_opts = [ '-s', args.search_alg,
                    '-H', args.heur,
                    '--max-time', args.max_time,
                    '--max-mem', args.max_mem,
                    '--print-heur-init',
                  ]
              
    tasks = []
    for problem in problems:
        s = { 'plan'   : os.path.join(problem['dir'], 'search.plan'),
              'stdout' : os.path.join(problem['dir'], 'search.out'),
              'stderr' : os.path.join(problem['dir'], 'search.err'),
              'bin'    : os.path.join(problem['dir'], 'search'),
              'opts'   : search_opts,
            }
        shutil.copy(args.search_bin, s['bin'])

        d = { 'problem' : problem,
              'search'  : s,
              'validate' : { 'bin' : args.validate_bin,
                             'stdout' : os.path.join(problem['dir'], 'validate.out'),
                             'stderr' : os.path.join(problem['dir'], 'validate.err'),
                           },
              'translate' : { 'bin' : args.translate,
                              'stdout' : os.path.join(problem['dir'], 'translate.out'),
                              'stderr' : os.path.join(problem['dir'], 'translate.err'),
                            },
              'stdout'  : os.path.join(problem['dir'], 'stdout'),
              'stderr'  : os.path.join(problem['dir'], 'stderr'),
            }
        tasks.append(d)
    return tasks

def mainPlan(args):
    if not planCheckResources(args):
        return -1

    print('Loading problems...')
    problems = planLoadProblems(args)
    if problems is None:
        return -1

    print('Preparing disk data...')
    problems = planPrepareDisk(args, problems)
    if problems is None:
        return -1

    print('Preparing tasks...')
    tasks = planTasks(args, problems)
    if tasks is None:
        return -1

    bench = { 'name' : args.name,
              'dir'  : os.path.abspath(args.name),
              'argv' : sys.argv,
              'tasks' : tasks,
            }

    bench_fn = os.path.join(bench['dir'], 'bench.pickle')
    with open(bench_fn, 'w') as fout:
        pickle.dump(bench, fout)
    print('Benchmark plan saved to `{0}\''.format(bench_fn))
    return 0



BENCHMARKS = {
    'ma-easy' : [
        { 'domain' : 'depot',
          'problems' : [ 'pfile1', 'pfile2' ],
        },

        { 'domain' : 'driverlog',
          'problems' : [ 'pfile1', 'pfile2' ],
        },

        { 'domain' : 'mablocks',
          'problems' : [ '5-15-3' ]
        },

        { 'domain' : 'openstacks',
          'problems' : [ 'p07' ]
        },
    ]
}


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--bench-dir', dest = 'bench_dir', type = str,
                        default = BENCHMARKS_DIR)
    parser.add_argument('--python-bin', dest = 'python_bin', type = str,
                        default = PYTHON)
    parser.add_argument('--search-bin', dest = 'search_bin', type = str,
                        default = SEARCH)
    parser.add_argument('--validate-bin', dest = 'validate_bin', type = str,
                        default = VALIDATE)
    parser.add_argument('--translate', dest = 'translate', type = str,
                        default = TRANSLATE)

    subparsers = parser.add_subparsers(dest = 'command')

    parser_bench = subparsers.add_parser('plan')
    parser_bench.add_argument('-r', '--repeats', dest = 'repeats', type = int,
                              default = 5)
    parser_bench.add_argument('-n', '--name', dest = 'name', type = str,
                              required = True)
    parser_bench.add_argument('-b', '--bench-name', dest = 'bench_name',
                              type = str, required = True)
    parser_bench.add_argument('--ma', action = 'store_const', const = True,
                              dest = 'ma', default = False)
    parser_bench.add_argument('-s', '--search-alg', dest = 'search_alg',
                              type = str, required = True)
    parser_bench.add_argument('-H', '--heur', dest = 'heur',
                              type = str, required = True)
    parser_bench.add_argument('--max-time', dest = 'max_time',
                              type = int, required = True)
    parser_bench.add_argument('--max-mem', dest = 'max_mem',
                              type = int, required = True)
    parser_bench.add_argument('--branch', dest = 'branch', type = str,
                              default = '')

    parser_task = subparsers.add_parser('task')

    parser_results = subparsers.add_parser('results')

    args = parser.parse_args()

    if args.command == 'plan':
        sys.exit(mainPlan(args))

