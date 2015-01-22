#!/usr/bin/env python2.7

import sys
import os
import re
import copy
import time
import threading
import subprocess
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

### PLAN ###
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
    task_i = 0
    for bench in benchmarks:
        domain = bench['domain']
        for problem in bench['problems']:
            domain_pddl, pddl, addl, proto \
                = planFindDefFiles(args.bench_dir, domain, problem)

            ma_agents = 0
            if addl is not None and args.ma:
                ma_agents = planMAAgents(addl)

            for r in range(args.repeats):
                rel_dir = '{0:06d}-{1}:{2}:{3:02d}'.format(task_i, domain, problem, r)
                task_i += 1
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

    args.python_path = os.path.join(root, 'third-party/protobuf/python/build/protobuf-2.6.1-py2.7.egg')
    if not os.path.isfile(args.python_path):
        print('Error: Could not find `{0}\''.format(args.python_path))
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
              'max_time' : args.max_time,
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
              'python'  : args.python_bin,
              'python_path' : args.python_path,

              # Add ten minutes for translate and validate
              'max_time' : args.max_time + (10 * 60),
              # Add half GB slack
              'max_mem' : args.max_mem + 512,
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
### PLAN END ###

### TASK ###
def taskToScratch(task):
    scratch = copy.deepcopy(task)

    scratchdir = os.environ['SCRATCHDIR']
    scratch['scratchdir'] = scratchdir
    scratch['problem']['domain_pddl'] = os.path.join(scratchdir, 'domain.pddl')
    scratch['problem']['pddl'] = os.path.join(scratchdir, 'prob.pddl')
    if task['problem']['addl']:
        scratch['problem']['addl'] = os.path.join(scratchdir, 'prob.addl')
    if task['problem']['proto']:
        scratch['problem']['proto'] = os.path.join(scratchdir, 'prob.proto')

    scratch['search']['plan'] = os.path.join(scratchdir, 'search.plan')
    scratch['search']['stdout'] = os.path.join(scratchdir, 'search.out')
    scratch['search']['stderr'] = os.path.join(scratchdir, 'search.err')
    scratch['search']['bin'] = os.path.join(scratchdir, 'search')

    scratch['validate']['stdout'] = os.path.join(scratchdir, 'validate.out')
    scratch['validate']['stderr'] = os.path.join(scratchdir, 'validate.err')

    scratch['translate']['stdout'] = os.path.join(scratchdir, 'translate.out')
    scratch['translate']['stderr'] = os.path.join(scratchdir, 'translate.err')

    for key in ['domain_pddl', 'pddl', 'addl', 'proto']:
        if task['problem'][key] is None:
            continue
        shutil.copy(task['problem'][key], scratch['problem'][key])
    shutil.copy(task['search']['bin'], scratch['search']['bin'])

    scratch['proto'] = os.path.join(scratchdir, 'proto')
    task['scratch_proto'] = scratch['proto']
    return scratch

def _cpy(src, dst):
    if os.path.isfile(src):
        shutil.copy(src, dst)
def scratchToTask(scratch, task):
    _cpy(scratch['translate']['stdout'], task['translate']['stdout'])
    _cpy(scratch['translate']['stderr'], task['translate']['stderr'])
    _cpy(scratch['search']['stdout'], task['search']['stdout'])
    _cpy(scratch['search']['stderr'], task['search']['stderr'])
    _cpy(scratch['search']['plan'], task['search']['plan'])
    _cpy(scratch['validate']['stdout'], task['validate']['stdout'])
    _cpy(scratch['validate']['stderr'], task['validate']['stderr'])
    _cpy(scratch['proto'], os.path.join(task['problem']['dir'], 'translate.proto'))

def taskDelScratch(scratch):
    os.system('rm -rf {0}/*'.format(scratch['scratchdir']))

class TaskTranslate(object):
    def __init__(self, task):
        # This does not work :(
        #os.system('module add python-2.7.6-gcc')
        #os.system('module add python27-modules-gcc')

        cmd = ''
        if task['python_path'] is not None:
            path = task['python_path']
            path += ':/software/python27-modules/software/python-2.7.6/gcc/lib/python2.7/site-packages'
            if 'PYTHONPATH' in os.environ:
                path += ':' + os.environ['PYTHONPATH']
            cmd += 'PYTHONPATH="{0}" '.format(path)
        cmd += task['python']
        cmd += ' ' + task['translate']['bin']
        cmd += ' ' + task['problem']['domain_pddl']
        cmd += ' ' + task['problem']['pddl']
        if task['problem']['addl'] is not None:
            cmd += ' {0}'.format(task['problem']['addl'])
        cmd += ' --proto --output {0}'.format(task['proto'])
        cmd += ' >{0}'.format(task['translate']['stdout'])
        cmd += ' 2>{0}'.format(task['translate']['stderr'])
        self.cmd = cmd

    def run(self):
        start_time = time.time()
        print('CMD: {0}'.format(self.cmd))
        os.system(self.cmd)
        end_time = time.time()
        overall_time = end_time - start_time
        print('Translate Time: {0}'.format(overall_time))

class TaskSearch(object):
    def __init__(self, task):
        args = [task['search']['bin']]
        args += [str(x) for x in task['search']['opts']]
        args += ['-o', task['search']['plan']]
        if 'proto' in task:
            args += ['-p', task['proto']]
        else:
            args += ['-p', task['problem']['proto']]
        if task['problem']['ma']:
            args += ['--ma']

        self.args = args
        self.stdout = task['search']['stdout']
        self.stderr = task['search']['stderr']
        self.pipe = None

        # add 10 minutes slack for timeout
        self.timeout = task['search']['max_time'] + (10 * 60)

    def run(self):
        fout = open(self.stdout, 'w')
        ferr = open(self.stderr, 'w')

        abort_timeout = threading.Timer(self.timeout, self.abort)
        abort_timeout.start()

        start_time = time.time()
        print('CMD: {0}'.format(' '.join(self.args)))
        self.pipe = subprocess.Popen(self.args, stdout = fout, stderr = ferr)
        self.pipe.wait()
        end_time = time.time()
        overall_time = end_time - start_time
        print('Search Time: {0}'.format(overall_time))

        abort_timeout.cancel()

    def abort(self):
        if self.pipe is not None:
            self.pipe.terminate()
        print >>sys.stderr, 'TIMEOUT'
        sys.stderr.flush()
        print('TIMEOUT')

class TaskValidate(object):
    def __init__(self, task):
        cmd  = task['validate']['bin']
        cmd += ' ' + task['problem']['domain_pddl']
        cmd += ' ' + task['problem']['pddl']
        cmd += ' ' + task['search']['plan']
        cmd += ' >{0}'.format(task['validate']['stdout'])
        cmd += ' 2>{0}'.format(task['validate']['stderr'])

        self.cmd = cmd

    def run(self):
        start_time = time.time()
        print('CMD: {0}'.format(self.cmd))
        os.system(self.cmd)
        end_time = time.time()
        overall_time = end_time - start_time
        print('Validate Time: {0}'.format(overall_time))

def mainTask():
    start_time = time.time()
    plan_data = os.environ['BENCH_DATA_FN']
    task_i = int(os.environ['BENCH_TASK_I'])

    bench = pickle.load(open(plan_data, 'r'))
    task = bench['tasks'][task_i]
    print 'Task:'
    pprint(task)
    print

    scratch = taskToScratch(task)
    print 'Scratch Task:'
    pprint(scratch)
    print

    translate = TaskTranslate(scratch)
    search = TaskSearch(scratch)
    validate = TaskValidate(scratch)

    translate.run()
    search.run()
    validate.run()

    scratchToTask(scratch, task)
    taskDelScratch(scratch)

    end_time = time.time()
    overall_time = end_time - start_time
    print('Overall Time: {0}'.format(overall_time))
### TASK END ###


### QSUB ###
def mainQSub(args):
    bench = pickle.load(open(args.bench_data, 'r'))

    bench_fn = os.path.abspath(args.bench_data)
    self_fn = os.path.abspath(sys.argv[0])
    for task_i, task in enumerate(bench['tasks']):
        nodes = 1

        ppn = 1
        if task['problem']['ma']:
            ppn = task['problem']['ma-agents'] + 1
        ppn = min(ppn, 16)

        qargs = ['qsub']
        qargs += ['-l', 'nodes={0}:ppn={1}:{2}'.format(nodes, ppn, args.resources)]
        qargs += ['-l', 'walltime={0}s'.format(task['max_time'])]
        qargs += ['-l', 'mem={0}mb'.format(task['max_mem'])]
        qargs += ['-l', 'scratch=400mb:local']
        qargs += ['-o', task['stdout']]
        qargs += ['-e', task['stderr']]
        qargs += ['-v', 'BENCH_DATA_FN={0},BENCH_TASK_I={1}'.format(bench_fn, task_i)]
        qargs += [self_fn]

        cmd = ' '.join(qargs)
        print('CMD {0}'.format(cmd))
        os.system(cmd)
### QSUB END ###


BENCHMARKS = {
    'ma-full' : [
        { 'domain' : 'blocksworld',
          'problems' : [ 'probBLOCKS-10-0', 'probBLOCKS-10-1',
                         'probBLOCKS-10-2', 'probBLOCKS-11-0',
                         'probBLOCKS-11-1', 'probBLOCKS-11-2',
                         'probBLOCKS-12-0', 'probBLOCKS-12-1',
                         'probBLOCKS-13-0', 'probBLOCKS-13-1',
                         'probBLOCKS-14-0', 'probBLOCKS-14-1',
                         'probBLOCKS-15-0', 'probBLOCKS-15-1',
                         'probBLOCKS-16-1', 'probBLOCKS-16-2',
                         'probBLOCKS-17-0', 'probBLOCKS-4-0',
                         'probBLOCKS-4-1', 'probBLOCKS-4-2',
                         'probBLOCKS-5-0', 'probBLOCKS-5-1',
                         'probBLOCKS-5-2', 'probBLOCKS-6-0',
                         'probBLOCKS-6-1', 'probBLOCKS-6-2',
                         'probBLOCKS-7-0', 'probBLOCKS-7-1',
                         'probBLOCKS-7-2', 'probBLOCKS-8-0',
                         'probBLOCKS-8-1', 'probBLOCKS-8-2',
                         'probBLOCKS-9-0', 'probBLOCKS-9-1',
                         'probBLOCKS-9-2', ]
        },

        { 'domain' : 'depot',
          'problems' : [ 'pfile1', 'pfile10', 'pfile11', 'pfile12',
                         'pfile13', 'pfile14', 'pfile15', 'pfile16',
                         'pfile17', 'pfile18', 'pfile19', 'pfile2',
                         'pfile20', 'pfile3', 'pfile4', 'pfile5', 'pfile6',
                         'pfile7', 'pfile8', 'pfile9', ]
        },

        { 'domain' : 'driverlog',
          'problems' : [ 'pfile1', 'pfile10', 'pfile11', 'pfile12',
                         'pfile13', 'pfile14', 'pfile15', 'pfile16',
                         'pfile17', 'pfile18', 'pfile19', 'pfile2',
                         'pfile20', 'pfile3', 'pfile4', 'pfile5', 'pfile6',
                         'pfile7', 'pfile8', 'pfile9', ]
        },

        { 'domain' : 'elevators08',
          'problems' : [ 'p01', 'p02', 'p03', 'p04', 'p05', 'p06', 'p07',
                         'p08', 'p09', 'p10', 'p11', 'p12', 'p13', 'p14',
                         'p15', 'p16', 'p17', 'p18', 'p19', 'p20', 'p21',
                         'p22', 'p23', 'p24', 'p25', 'p26', 'p27', 'p28',
                         'p29', 'p30', ]
        },

        { 'domain' : 'logistics00',
          'problems' : [ 'probLOGISTICS-10-0', 'probLOGISTICS-10-1',
                         'probLOGISTICS-11-0', 'probLOGISTICS-11-1',
                         'probLOGISTICS-12-0', 'probLOGISTICS-12-1',
                         'probLOGISTICS-13-0', 'probLOGISTICS-13-1',
                         'probLOGISTICS-14-0', 'probLOGISTICS-14-1',
                         'probLOGISTICS-15-0', 'probLOGISTICS-15-1',
                         'probLOGISTICS-4-0', 'probLOGISTICS-5-0',
                         'probLOGISTICS-6-0', 'probLOGISTICS-7-0',
                         'probLOGISTICS-8-0', 'probLOGISTICS-8-1',
                         'probLOGISTICS-9-0', 'probLOGISTICS-9-1', ]
        },

        { 'domain' : 'mablocks',
          'problems' : [ '3-10-3', '3-10-5', '3-15-3', '3-15-5', '3-5-3',
                         '3-5-5', '4-10-3', '4-10-5', '4-15-3', '4-15-5',
                         '4-5-3', '4-5-5', '5-10-3', '5-10-5', '5-15-3',
                         '5-15-5', '5-5-3', '5-5-5', '6-10-3', '6-10-5',
                         '6-15-3', '6-15-5', '6-5-3', '6-5-5', ]
        },

        { 'domain' : 'masatellites',
          'problems' : [ 'satellites-a10', 'satellites-a12',
                         'satellites-a14', 'satellites-a16',
                         'satellites-a8', ]
        },

        { 'domain' : 'openstacks',
          'problems' : [ 'p01', 'p02', 'p03', 'p04', 'p05', 'p06', 'p07',
                         'p08', 'p09', 'p10', 'p11', 'p12', 'p13', 'p14',
                         'p15', 'p16', 'p17', 'p18', 'p19', 'p20', 'p21',
                         'p22', 'p23', 'p24', 'p25', 'p26', 'p27', 'p28',
                         'p29', 'p30', ]
        },

        { 'domain' : 'rovers',
          'problems' : [ 'p03', 'p04', 'p05', 'p06', 'p07',
                         'p08', 'p09', 'p10', 'p11', 'p12', 'p13', 'p14',
                         'p15', 'p16', 'p17', 'p18', 'p19', 'p20', ]
        },

        { 'domain' : 'rovers-large',
          'problems' : [ 'p21', 'p22', 'p23', 'p24', 'p25', 'p26', 'p27',
                         'p28', 'p29', 'p30', 'p31', 'p32', 'p33', 'p34',
                         'p35', 'p36', 'p37', 'p38', 'p39', 'p40', ]
        },

        { 'domain' : 'satellites',
          'problems' : [ 'p03-pfile3',
                         'p04-pfile4', 'p05-pfile5', 'p06-pfile6',
                         'p07-pfile7', 'p08-pfile8', 'p09-pfile9',
                         'p10-pfile10', 'p11-pfile11', 'p12-pfile12',
                         'p13-pfile13', 'p14-pfile14', 'p15-pfile15',
                         'p16-pfile16', 'p17-pfile17', 'p18-pfile18',
                         'p19-pfile19', 'p20-pfile20', ]
        },

        { 'domain' : 'satellites-hc',
          'problems' : [ 'p21-HC-pfile1', 'p22-HC-pfile2', 'p23-HC-pfile3',
                         'p24-HC-pfile4', 'p25-HC-pfile5', 'p26-HC-pfile6',
                         'p27-HC-pfile7', 'p28-HC-pfile8', 'p29-HC-pfile9',
                         'p30-HC-pfile10', 'p31-HC-pfile11',
                         'p32-HC-pfile12', 'p33-HC-pfile13',
                         'p34-HC-pfile14', 'p35-HC-pfile15', ]
        },

        { 'domain' : 'sokoban',
          'problems' : [ 'p01', 'p02', 'p03', 'p04', 'p05', 'p06', 'p07',
                         'p08', 'p09', 'p10', ]
        },

        { 'domain' : 'test',
          'problems' : [ 'pfile' ],
        },

        { 'domain' : 'transport',
          'problems' : [ 'p01', 'p02', 'p03', 'p04', 'p05', 'p06', 'p07',
                         'p08', 'p09', 'p10', ]
        },

        { 'domain' : 'variance',
          'problems' : [ 'logistics-a3', ]
        },

        { 'domain' : 'woodworking08',
          'problems' : [ 'p01', 'p02', 'p03', 'p04', 'p05', 'p06', 'p07',
                         'p08', 'p09', 'p10', 'p11', 'p12', 'p13', 'p14',
                         'p15', 'p16', 'p17', 'p18', 'p19', 'p20', 'p21',
                         'p22', 'p23', 'p24', 'p25', 'p26', 'p27', 'p28',
                         'p29', 'p30', ]
        },

        { 'domain' : 'woodworkingFMAP',
          'problems' : [ 'p01', 'p02', 'p03', 'p04', 'p05', 'p06', 'p07',
                         'p08', 'p09', 'p10', 'p11', 'p12', 'p13', 'p14',
                         'p15', 'p16', 'p17', 'p18', 'p19', 'p20', 'p21',
                         'p22', 'p23', 'p24', 'p25', 'p26', 'p27', 'p28',
                         'p29', 'p30', ]
        },

        { 'domain' : 'zenotravel',
          'problems' : [ 'pfile10', 'pfile12', 'pfile13',
                         'pfile14', 'pfile15', 'pfile16', 'pfile17',
                         'pfile18', 'pfile19', 'pfile20',
                         'pfile3', 'pfile4', 'pfile5', 'pfile6', 'pfile7',
                         'pfile8', 'pfile9', ]
        },
    ],

    'ma-easy' : [
        { 'domain' : 'depot',
          'problems' : [ 'pfile1', 'pfile2' ],
        },

        { 'domain' : 'driverlog',
          'problems' : [ 'pfile1', 'pfile2' ],
        },

        { 'domain' : 'test',
          'problems' : [ 'pfile' ],
        },
    ],

    'full' : [
        { 'domain' : 'blocksworld',
          'problems' : [ 'probBLOCKS-10-0', 'probBLOCKS-10-1',
                         'probBLOCKS-10-2', 'probBLOCKS-11-0',
                         'probBLOCKS-11-1', 'probBLOCKS-11-2',
                         'probBLOCKS-12-0', 'probBLOCKS-12-1',
                         'probBLOCKS-13-0', 'probBLOCKS-13-1',
                         'probBLOCKS-14-0', 'probBLOCKS-14-1',
                         'probBLOCKS-15-0', 'probBLOCKS-15-1',
                         'probBLOCKS-16-1', 'probBLOCKS-16-2',
                         'probBLOCKS-17-0', 'probBLOCKS-4-0',
                         'probBLOCKS-4-1', 'probBLOCKS-4-2',
                         'probBLOCKS-5-0', 'probBLOCKS-5-1',
                         'probBLOCKS-5-2', 'probBLOCKS-6-0',
                         'probBLOCKS-6-1', 'probBLOCKS-6-2',
                         'probBLOCKS-7-0', 'probBLOCKS-7-1',
                         'probBLOCKS-7-2', 'probBLOCKS-8-0',
                         'probBLOCKS-8-1', 'probBLOCKS-8-2',
                         'probBLOCKS-9-0', 'probBLOCKS-9-1',
                         'probBLOCKS-9-2', ]
        },

        { 'domain' : 'depot',
          'problems' : [ 'pfile1', 'pfile10', 'pfile11', 'pfile12',
                         'pfile13', 'pfile14', 'pfile15', 'pfile16',
                         'pfile17', 'pfile18', 'pfile19', 'pfile2',
                         'pfile20', 'pfile3', 'pfile4', 'pfile5', 'pfile6',
                         'pfile7', 'pfile8', 'pfile9', ]
        },

        { 'domain' : 'driverlog',
          'problems' : [ 'pfile1', 'pfile10', 'pfile11', 'pfile12',
                         'pfile13', 'pfile14', 'pfile15', 'pfile16',
                         'pfile17', 'pfile18', 'pfile19', 'pfile2',
                         'pfile20', 'pfile3', 'pfile4', 'pfile5', 'pfile6',
                         'pfile7', 'pfile8', 'pfile9', ]
        },

        { 'domain' : 'elevators08',
          'problems' : [ 'p01', 'p02', 'p03', 'p04', 'p05', 'p06', 'p07',
                         'p08', 'p09', 'p10', 'p11', 'p12', 'p13', 'p14',
                         'p15', 'p16', 'p17', 'p18', 'p19', 'p20', 'p21',
                         'p22', 'p23', 'p24', 'p25', 'p26', 'p27', 'p28',
                         'p29', 'p30', ]
        },

        { 'domain' : 'logistics00',
          'problems' : [ 'probLOGISTICS-10-0', 'probLOGISTICS-10-1',
                         'probLOGISTICS-11-0', 'probLOGISTICS-11-1',
                         'probLOGISTICS-12-0', 'probLOGISTICS-12-1',
                         'probLOGISTICS-13-0', 'probLOGISTICS-13-1',
                         'probLOGISTICS-14-0', 'probLOGISTICS-14-1',
                         'probLOGISTICS-15-0', 'probLOGISTICS-15-1',
                         'probLOGISTICS-4-0', 'probLOGISTICS-5-0',
                         'probLOGISTICS-6-0', 'probLOGISTICS-7-0',
                         'probLOGISTICS-8-0', 'probLOGISTICS-8-1',
                         'probLOGISTICS-9-0', 'probLOGISTICS-9-1', ]
        },

        { 'domain' : 'mablocks',
          'problems' : [ '3-10-3', '3-10-5', '3-15-3', '3-15-5', '3-5-3',
                         '3-5-5', '4-10-3', '4-10-5', '4-15-3', '4-15-5',
                         '4-5-3', '4-5-5', '5-10-3', '5-10-5', '5-15-3',
                         '5-15-5', '5-5-3', '5-5-5', '6-10-3', '6-10-5',
                         '6-15-3', '6-15-5', '6-5-3', '6-5-5', ]
        },

        { 'domain' : 'masatellites',
          'problems' : [ 'satellites-a10', 'satellites-a12',
                         'satellites-a14', 'satellites-a16',
                         'satellites-a8', ]
        },

        { 'domain' : 'openstacks',
          'problems' : [ 'p01', 'p02', 'p03', 'p04', 'p05', 'p06', 'p07',
                         'p08', 'p09', 'p10', 'p11', 'p12', 'p13', 'p14',
                         'p15', 'p16', 'p17', 'p18', 'p19', 'p20', 'p21',
                         'p22', 'p23', 'p24', 'p25', 'p26', 'p27', 'p28',
                         'p29', 'p30', ]
        },

        { 'domain' : 'rovers',
          'problems' : [ 'p01', 'p02', 'p03', 'p04', 'p05', 'p06', 'p07',
                         'p08', 'p09', 'p10', 'p11', 'p12', 'p13', 'p14',
                         'p15', 'p16', 'p17', 'p18', 'p19', 'p20', ]
        },

        { 'domain' : 'rovers-large',
          'problems' : [ 'p21', 'p22', 'p23', 'p24', 'p25', 'p26', 'p27',
                         'p28', 'p29', 'p30', 'p31', 'p32', 'p33', 'p34',
                         'p35', 'p36', 'p37', 'p38', 'p39', 'p40', ]
        },

        { 'domain' : 'satellites',
          'problems' : [ 'p01-pfile1', 'p02-pfile2', 'p03-pfile3',
                         'p04-pfile4', 'p05-pfile5', 'p06-pfile6',
                         'p07-pfile7', 'p08-pfile8', 'p09-pfile9',
                         'p10-pfile10', 'p11-pfile11', 'p12-pfile12',
                         'p13-pfile13', 'p14-pfile14', 'p15-pfile15',
                         'p16-pfile16', 'p17-pfile17', 'p18-pfile18',
                         'p19-pfile19', 'p20-pfile20', ]
        },

        { 'domain' : 'satellites-hc',
          'problems' : [ 'p21-HC-pfile1', 'p22-HC-pfile2', 'p23-HC-pfile3',
                         'p24-HC-pfile4', 'p25-HC-pfile5', 'p26-HC-pfile6',
                         'p27-HC-pfile7', 'p28-HC-pfile8', 'p29-HC-pfile9',
                         'p30-HC-pfile10', 'p31-HC-pfile11',
                         'p32-HC-pfile12', 'p33-HC-pfile13',
                         'p34-HC-pfile14', 'p35-HC-pfile15', ]
        },

        { 'domain' : 'sokoban',
          'problems' : [ 'p01', 'p02', 'p03', 'p04', 'p05', 'p06', 'p07',
                         'p08', 'p09', 'p10', ]
        },

        { 'domain' : 'test',
          'problems' : [ 'pfile' ],
        },

        { 'domain' : 'transport',
          'problems' : [ 'p01', 'p02', 'p03', 'p04', 'p05', 'p06', 'p07',
                         'p08', 'p09', 'p10', ]
        },

        { 'domain' : 'variance',
          'problems' : [ 'logistics-a3', ]
        },

        { 'domain' : 'woodworking08',
          'problems' : [ 'p01', 'p02', 'p03', 'p04', 'p05', 'p06', 'p07',
                         'p08', 'p09', 'p10', 'p11', 'p12', 'p13', 'p14',
                         'p15', 'p16', 'p17', 'p18', 'p19', 'p20', 'p21',
                         'p22', 'p23', 'p24', 'p25', 'p26', 'p27', 'p28',
                         'p29', 'p30', ]
        },

        { 'domain' : 'woodworkingFMAP',
          'problems' : [ 'p01', 'p02', 'p03', 'p04', 'p05', 'p06', 'p07',
                         'p08', 'p09', 'p10', 'p11', 'p12', 'p13', 'p14',
                         'p15', 'p16', 'p17', 'p18', 'p19', 'p20', 'p21',
                         'p22', 'p23', 'p24', 'p25', 'p26', 'p27', 'p28',
                         'p29', 'p30', ]
        },

        { 'domain' : 'zenotravel',
          'problems' : [ 'pfile1', 'pfile10', 'pfile12', 'pfile13',
                         'pfile14', 'pfile15', 'pfile16', 'pfile17',
                         'pfile18', 'pfile19', 'pfile2', 'pfile20',
                         'pfile3', 'pfile4', 'pfile5', 'pfile6', 'pfile7',
                         'pfile8', 'pfile9', ]
        },
    ],
}


if __name__ == '__main__':
    if len(sys.argv) == 1:
        sys.exit(mainTask())

    parser = argparse.ArgumentParser()
    parser.add_argument('--bench-dir', dest = 'bench_dir', type = str,
                        default = BENCHMARKS_DIR)
    parser.add_argument('--python-bin', dest = 'python_bin', type = str,
                        default = PYTHON)
    parser.add_argument('--python-path', dest = 'python_path', type = str,
                        default = None)
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

    parser_bench = subparsers.add_parser('qsub')
    parser_bench.add_argument('bench_data')
    parser_bench.add_argument('-r', dest = 'resources', required = True)

    parser_results = subparsers.add_parser('results')

    args = parser.parse_args()

    if args.command == 'plan':
        sys.exit(mainPlan(args))
    elif args.command == 'qsub':
        sys.exit(mainQSub(args))

