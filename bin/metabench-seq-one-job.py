#!/usr/bin/env python2

from __future__ import print_function
import sys
import os
import subprocess
import time
import datetime
import shutil
import re
import logging
import random
import threading
import ConfigParser as cfgparser

global_start_time = time.time()

class Logger(object):
    def __init__(self, dst):
        self.out = sys.stdout
        self.fout = open(dst, 'a')

    def write(self, buf):
        self.fout.write(buf)
        self.fout.flush()
        self.out.write(buf)
        self.out.flush()

    def flush(self):
        self.fout.flush()
        self.out.flush()

def initLogging(log_dst):
    sys.stdout = Logger(log_dst)
    #sys.stderr = sys.stdout

def p(*args):
    t = time.time()
    stime = '[{0:.3f}] '.format(t - global_start_time)
    prefix = str(datetime.datetime.now()) + ' ' + stime
    sys.stdout.write(prefix)
    print(*args)
    sys.stdout.flush()

pth_lock = threading.Lock()
def pth(thid, *args):
    pth_lock.acquire()
    args = ['{0}::'.format(thid)] + list(args)
    p(*args)
    pth_lock.release()

def err(msg):
    msg = 'Error: ' + msg
    p(msg)
    raise Exception(msg)

def runCmd(cmd):
    p('CMD: {0}'.format(cmd))
    if os.system(cmd) != 0:
        sys.exit(-1)


class ConfigBench(object):
    def __init__(self, cfg, name):
        self.name = name
        section = 'metabench-bench-' + name
        self.path = cfg.get(section, 'path')
        self.pddl = {}

    def load(self, cfg):
        self._loadSeq(os.path.join(cfg.pddl_repodir, self.path))

    def _addPDDL(self, domain, problem, domain_pddl, problem_pddl):
        if domain not in self.pddl:
            self.pddl[domain] = {}
        if problem in self.pddl[domain]:
            raise Exception('{0}:{1} already added.'.format(domain, problem))
        self.pddl[domain][problem] = { 'domain' : domain_pddl,
                                       'problem' : problem_pddl
                                     }

    def _loadSeq(self, path):
        for pddl_dir in self._pddlDirs(path):
            domain = os.path.split(pddl_dir)[-1]
            for problem, pddl, domain_pddl in self._pddlDomainProblem(pddl_dir):
                self._addPDDL(domain, problem, domain_pddl, pddl)

    def _pddlDirs(self, path):
        dirs = []
        for dirpath, dirnames, filenames in os.walk(path):
            fs = [x for x in filenames if x[-5:] == '.pddl']
            if len(fs) > 0:
                dirs += [dirpath]
        return sorted(dirs)

    def _pddlDomainProblem(self, path):
        files = os.listdir(path)
        files = filter(lambda x: x.find('domain') == -1, files)
        pddls = filter(lambda x: x[-5:] == '.pddl', files)
        names = sorted([x[:-5] for x in pddls])
        for name in names:
            pddl = os.path.join(path, name + '.pddl')
            if not os.path.isfile(pddl):
                continue

            pddl_domain = self._findDomainPDDL(path, name)
            yield name, pddl, pddl_domain

    def _findDomainPDDL(self, path, problem_name):
        domain = os.path.join(path, 'domain_' + problem_name + '.pddl')
        if os.path.isfile(domain):
            return domain

        if problem_name.find('problem') != -1:
            domain = os.path.join(path, problem_name.replace('problem', 'domain') + '.pddl')
            if os.path.isfile(domain):
                return domain

        domain = os.path.join(path, problem_name + '-domain.pddl')
        if os.path.isfile(domain):
            return domain

        domain = os.path.join(path, 'domain.pddl')
        if os.path.isfile(domain):
            return domain

        raise Exception('Cannot find domain PDDL file in {0} for problem {1}.'
                            .format(path, problem_name))


class ConfigSearch(object):
    def __init__(self, cfg, name, search = None, heur = None, name2 = None):
        if name2 is None:
            self.name = name
        else:
            self.name = name2
        self.section = 'metabench-search-' + name
        self.search = search
        if search is None:
            self.search = cfg.get(self.section, 'search')
        self.heur = heur
        if heur is None:
            self.heur = cfg.get(self.section, 'heur')
        self.max_time = cfg.getint(self.section, 'max-time')
        self.max_mem = cfg.getint(self.section, 'max-mem')
        self.fd_translate_py = None
        self.fd_preprocess = None
        self.fd_translate_opts = ''

        if cfg.has_option(self.section, 'fd-translate-py') \
           or cfg.has_option(self.section, 'fd-preprocess'):
            if not cfg.has_option(self.section, 'fd-translate-py') \
               or not cfg.has_option(self.section, 'fd-preprocess'):
                err('If you want to use FD preprocessor, both ' \
                     + 'fd-translate-py and fd-preprocess must be defined!')
        if cfg.has_option(self.section, 'fd-translate-py'):
            self.fd_translate_py = cfg.get(self.section, 'fd-translate-py')
            self.fd_preprocess = cfg.get(self.section, 'fd-preprocess')
        if cfg.has_option(self.section, 'fd-translate-opts'):
            self.fd_translate_opts = cfg.get(self.section, 'fd-translate-opts')

        self.bench = cfg.get(self.section, 'bench')
        self.bench = self.bench.split()
        self.bench = [ConfigBench(cfg, x) for x in self.bench]


class Config(object):
    def __init__(self, topdir):
        topdir = os.path.abspath(topdir)
        topdir = topdir.replace('/auto/praha1', '/storage/praha1/home')
        self.topdir = topdir
        self.scratchdir = os.path.join(topdir, 'scratch')
        if 'SCRATCHDIR' in os.environ:
            self.scratchdir = os.environ['SCRATCHDIR']
        if not os.path.isdir(self.scratchdir):
            os.mkdir(self.scratchdir)

        self.repodir = os.path.join(topdir, 'repo')
        self.search_bin = os.path.join(self.repodir, 'bin/search')
        self.validate_bin = os.path.join(self.repodir, 'third-party/VAL/validate')
        self.translate_py = os.path.join(self.repodir, 'third-party/translate/translate.py')
        self.protobuf_egg = os.path.join(self.repodir, 'third-party/protobuf/python/build/protobuf-2.6.1-py2.7.egg')
        self.pddl_repodir = os.path.join(topdir, 'pddl')

        self.fn = os.path.join(topdir, 'metabench.cfg')
        self.cfg = cfgparser.ConfigParser()
        self.cfg.read(self.fn)
        self.repo = self.cfg.get('metabench', 'repo')
        self.commit = self.cfg.get('metabench', 'commit')
        self.pddl_repo = self.cfg.get('metabench', 'pddl_repo')
        self.pddl_commit = self.cfg.get('metabench', 'pddl_commit')
        self.cplex_includedir = self._cfgGet('metabench', 'cplex-includedir',
                                              '/software/cplex-126/cplex/include')
        self.cplex_libdir = self._cfgGet('metabench', 'cplex-libdir',
                                         '/software/cplex-126/cplex/lib/x86-64_linux/static_pic')

        self.pythonpath = self._cfgGet('metabench', 'pythonpath', '')

        self.cluster = self.cfg.get('metabench', 'cluster')
        self.ppn = self.cfg.getint('metabench', 'ppn')

        search = self.cfg.get('metabench', 'search')
        search = search.split()
        self.search = []
        for s in search:
            self.search += self._parseSearch(s)

    def loadBench(self):
        for s in self.search:
            for b in s.bench:
                b.load(self)

    def _parseSearch(self, name):
        section = 'metabench-search-' + name
        if self.cfg.has_option(section, 'combine') \
           and self.cfg.getboolean(section, 'combine'):
            search = self.cfg.get(section, 'search').split()
            heur = self.cfg.get(section, 'heur').split()
            out = []
            for s in search:
                for h in heur:
                    name2 = name + '-' + s + '-' + h
                    name2 = name2.replace(':', '-')
                    out += [ConfigSearch(self.cfg, name, s, h, name2)]
            return out

        else:
            return [ConfigSearch(self.cfg, name)]

    def _cfgGet(self, section, option, default):
        if self.cfg.has_option(section, option):
            if type(default) is bool:
                return self.cfg.getboolean(section, option)
            elif type(default) is int:
                return self.cfg.getint(section, option)
            elif type(default) is float:
                return self.cfg.getfloat(section, option)
            return self.cfg.get(section, option)
        return default



def repoClone(path, dst):
    runCmd('git clone {0} {1}'.format(path, dst))

def repoCheckout(repo, commit):
    runCmd('cd {0} && (git checkout -b bench origin/{1} || git checkout -b bench {1})'.format(repo, commit))

def repoMaplanMake(cfg):
    make_local = open(os.path.join(cfg.repodir, 'Makefile.local'), 'w')
    make_local.write('CFLAGS = -O3 -march=corei7\n')
    make_local.write('CXXFLAGS = -O3 -march=corei7\n')
    if cfg.cplex_includedir is not None:
        make_local.write('CPLEX_CFLAGS = -I{0}\n'.format(cfg.cplex_includedir))
    if cfg.cplex_libdir is not None:
        make_local.write('CPLEX_LDFLAGS = -L{0} -lcplex\n'.format(cfg.cplex_libdir))
    make_local.close()

    runCmd('cd {0} && make third-party'.format(cfg.repodir))
    runCmd('cd {0} && make'.format(cfg.repodir))
    runCmd('cd {0} && make -C bin'.format(cfg.repodir))

def repoCheck(cfg):
    check_files = [cfg.search_bin,
                   cfg.validate_bin,
                   cfg.translate_py,
                   cfg.protobuf_egg
                  ]
    for f in check_files:
        if not os.path.isfile(f):
            err('Could not find `{0}\''.format(f))

def compileMaplan(cfg):
    if os.path.exists(cfg.repodir):
        err('Directory for maplan `{0}\' already exist'.format(cfg.repodir))

    repoClone(cfg.repo, cfg.repodir)
    repoCheckout(cfg.repodir, cfg.commit)
    repoMaplanMake(cfg)
    repoCheck(cfg)

def checkoutPDDL(cfg):
    if os.path.exists(cfg.pddl_repodir):
        err('Directory for pddl data `{0}\' already exist'.format(cfg.pddl_repodir))

    repoClone(cfg.pddl_repo, cfg.pddl_repodir)
    repoCheckout(cfg.pddl_repodir, cfg.pddl_commit)


def runLocal(cfg):
    os.environ['METABENCH_TOPDIR'] = cfg.topdir
    os.environ['METABENCH_PPN'] = str(cfg.ppn)
    os.environ['METABENCH_TIME_LIMIT'] = str(12 * 3600)
    script = os.path.realpath(__file__)
    cmd = ['python2', script]

    with open(os.path.join(cfg.topdir, 'main-task.out'), 'w') as out:
        with open(os.path.join(cfg.topdir, 'main-task.err'), 'w') as err:
            p('Run local...')
            try:
                subprocess.check_call(cmd, stdout = out, stderr = err)
                ret = 0
            except subprocess.CalledProcessError as e:
                p('Run local failed: {0}'.format(str(e)))
                ret = e.returncode

            p('Ret: {0}'.format(ret))

def qsub(cfg):
    if cfg.cluster == 'local':
        runLocal(cfg)
        return

    elif cfg.cluster == 'ungu':
        resources = 'cl_ungu'
        queue = 'uv@wagap'
        max_time = 95 * 3600
    elif cfg.cluster == 'urga':
        resources = 'cl_urga'
        queue = 'uv@wagap'
        max_time = 95 * 3600
    else:
        err('Unkown cluster {0}'.format(cfg.cluster))

    if os.path.isfile(os.path.join(cfg.topdir, 'qsub')):
        err('Already qsub\'ed!')

    max_mem  = max(x.max_mem for x in cfg.search)
    max_mem *= cfg.ppn
    max_mem += 1024
    scratch = cfg.ppn * 128

    script = os.path.realpath(__file__)
    script = script.replace('/auto/praha1', '/storage/praha1/home')
    out = '''#!/bin/bash
module add python-2.7.5
export METABENCH_TOPDIR={0}
export METABENCH_PPN={1}
export METABENCH_TIME_LIMIT={2}
python2 {3}
'''.format(cfg.topdir, cfg.ppn, int(max_time), script)
    fout = open(os.path.join(cfg.topdir, 'run-main.sh'), 'w')
    fout.write(out)
    fout.close()

    qargs = ['qsub']
    qargs += ['-l', 'nodes=1:ppn={0}:{1}'.format(cfg.ppn, resources)]
    qargs += ['-l', 'walltime={0}s'.format(max_time)]
    qargs += ['-l', 'mem={0}mb'.format(max_mem)]
    qargs += ['-l', 'scratch={0}mb'.format(scratch)]
    qargs += ['-q', queue]
    qargs += ['-o', os.path.join(cfg.topdir, 'main-task.out')]
    qargs += ['-e', os.path.join(cfg.topdir, 'main-task.err')]
#    environ  = 'METABENCH_TOPDIR={0}'.format(cfg.topdir)
#    environ += ',METABENCH_PPN={0}'.format(cfg.ppn)
#    environ += ',METABENCH_TIME_LIMIT={0}'.format(max_time)
#    qargs += ['-v', environ]
    qargs += [os.path.join(cfg.topdir, 'run-main.sh')]
    print(qargs)
    out = subprocess.check_output(qargs)

    qsub_fn = os.path.join(cfg.topdir, 'qsub')
    fout = open(qsub_fn, 'w')
    p('{0} -> {1}'.format(out.strip(), qsub_fn))
    fout.write(out.strip())
    fout.close()

def main(topdir):
    initLogging(os.path.join(topdir, 'metabench.log'))
    p('========== START ==========')

    cfg = Config(topdir)

    if not os.path.exists(cfg.repodir):
        compileMaplan(cfg)
    else:
        repoCheck(cfg)

    if not os.path.exists(cfg.pddl_repodir):
        checkoutPDDL(cfg)
    cfg.loadBench()

    qsub(cfg)

    #createTasks(cfg)
    #qsub(cfg)
    return 0



class ProblemQueue(object):
    def __init__(self, cfg):
        self._queue = []
        self._lock = threading.Lock()

        class Item(object):
            def __init__(self, search, bench, dom, prob):
                self.search = search
                self.bench = bench
                self.domain = dom
                self.problem = prob

        for search in cfg.search:
            for bench in search.bench:
                for domain in bench.pddl:
                    for problem in bench.pddl[domain]:
                        d = (search, bench, domain, problem,
                             bench.pddl[domain][problem]['domain'],
                             bench.pddl[domain][problem]['problem'])
                        self._queue += [d]

    def pop(self):
        self._lock.acquire()
        if len(self._queue) > 0:
            ret = self._queue.pop()
        else:
            ret = (None, None, None, None, None, None)
        self._lock.release()
        return ret

class ProblemTask(object):
    def __init__(self, thid, cfg, task):
        self.thid = thid
        self.cfg = cfg
        self.cfg_search = task[0]
        self.cfg_bench = task[1]
        self.domain = task[2]
        self.problem = task[3]
        self.domain_pddl = task[4]
        self.problem_pddl = task[5]

        self.prefix = self._prefix()
        self.outdir = self._outDir()
        self.outscratchdir = self._outScratchDir()

        self.translate_time = 0
        self.search_time = 0

    def run(self):
        pth(self.thid, 'Run', self.domain, self.problem, '-->', self.outdir)
        if os.path.isdir(self.outdir):
            pth(self.thid, self.domain, self.problem + ':', 'Skipping...')
            return
        else:
            os.mkdir(self.outdir)
            shutil.rmtree(self.outscratchdir, ignore_errors = True)
            os.mkdir(self.outscratchdir)

        self._log = open(os.path.join(self.outdir, 'task.log'), 'w')

        if self.translate() == 0:
            self.search()
            self.validate()

        shutil.rmtree(self.outscratchdir)
        self._log.close()

    def translate(self):
        if self.cfg_search.fd_translate_py is not None:
            return self.translateFD()
        else:
            raise 'self.translateProto() not supported yet!'

    def translateFD(self):
        self.log('translate: START')

        # Copy domain and problem to scratchdir
        self.log('translate: Copying files to scratch...')
        dom_pddl = os.path.join(self.outscratchdir, 'domain.pddl')
        prob_pddl = os.path.join(self.outscratchdir, 'problem.pddl')
        shutil.copyfile(self.domain_pddl, dom_pddl)
        shutil.copyfile(self.problem_pddl, prob_pddl)

        translate_cmd = ['cd', self.outscratchdir, '&&']
        translate_cmd += ['python2']
        translate_cmd += [self.cfg_search.fd_translate_py]
        translate_cmd += self.cfg_search.fd_translate_opts.split()
        translate_cmd += [dom_pddl, prob_pddl]
        self.log('translate: cmd: {0}'.format(' '.join(translate_cmd)))

        preprocess_cmd  = ['cd', self.outscratchdir, '&&']
        preprocess_cmd += [self.cfg_search.fd_preprocess]
        preprocess_cmd += ['<output.sas']
        self.log('translate: preprocess-cmd: {0}'.format(' '.join(preprocess_cmd)))

        ret, out, err, tm = self._runCmd(translate_cmd,
                                         max_mem = self.cfg_search.max_mem,
                                         bash = True)
        self.log('translate: translate --> {0}'.format(ret))
        self.log('translate: translate time {0}'.format(tm))
        self.translate_time += tm
        self.writeOutErr('translate', out, err)
        if ret != 0:
            return ret
        if not self.copyFromScratch('output.sas', 'problem.sas'):
            return -1

        ret, out, err, tm = self._runCmd(preprocess_cmd,
                                         max_mem = self.cfg_search.max_mem,
                                         bash = True)
        self.log('translate: preprocess --> {0}'.format(ret))
        self.log('translate: preprocess time {0}'.format(tm))
        self.translate_time += tm
        self.writeOutErr('preprocess', out, err)
        if ret != 0:
            return ret
        if not self.copyFromScratch('output', 'problem.fd') \
           or not self.copyWithinScratch('output', 'problem.fd'):
            return -1

        return 0

    def search(self):
        self.log('search: START')

        max_time  = self.cfg_search.max_time
        max_time -= self.translate_time
        max_time = int(max_time)
        max_mem = self.cfg_search.max_mem

        cmd  = [self.cfg.search_bin]
        cmd += ['--fd-problem', os.path.join(self.outscratchdir, 'problem.fd')]
        cmd += ['-s', self.cfg_search.search]
        cmd += ['-H', self.cfg_search.heur]
        cmd += ['--max-time', str(max_time)]
        cmd += ['--max-mem', str(max_mem)]
        cmd += ['-o', os.path.join(self.outscratchdir, 'plan.out')]
        cmd += ['--print-heur-init']

        self.log('search: cmd: {0}'.format(' '.join(cmd)))
        ret, out, err, tm = self._runCmd(cmd, max_mem = max_mem)
        self.log('search: --> {0}'.format(ret))
        self.log('search: time {0}'.format(tm))
        self.search_time = tm
        self.writeOutErr('search', out, err)
        self.copyFromScratch('plan.out')

        return ret

    def validate(self):
        self.log('validate: START')

        if os.path.isfile(os.path.join(self.outscratchdir, 'plan.out')):
            cmd  = [self.cfg.validate_bin]
            cmd += ['-v']
            cmd += [os.path.join(self.outscratchdir, 'domain.pddl')]
            cmd += [os.path.join(self.outscratchdir, 'problem.pddl')]
            cmd += [os.path.join(self.outscratchdir, 'plan.out')]
            ret, out, err, tm = self._runCmd(cmd)
        else:
            ret, out, err, tm = 0, 'NO PLAN', '', 0

        self.log('validate: --> {0}'.format(ret))
        self.log('validate: time {0}'.format(tm))
        self.validate_time = tm
        self.writeOutErr('validate', out, err)


    def _runCmd(self, cmd, infile = None, max_mem = None, bash = False):
        if max_mem is not None:
            mem = str(max_mem * 1024)
            cmd = ['ulimit -v {0} &&'.format(mem)] + cmd
            bash = True

        if bash:
            cmd = ' '.join(cmd)
            cmd = ['bash', '-c', cmd]
        self.log('CMD: {0}'.format(cmd))

        stdin = None
        if infile is not None:
            stdin = open(infile, 'r')
        tstart = time.time()
        proc = subprocess.Popen(cmd, stdout = subprocess.PIPE,
                                     stderr = subprocess.PIPE,
                                     stdin = stdin)
        if stdin is not None:
            stdin.close()

        out, err = proc.communicate()
        ret = proc.returncode
        return ret, out, err, time.time() - tstart

    def copyFromScratch(self, fn, tofn = None):
        if tofn is None:
            tofn = fn

        if os.path.isfile(os.path.join(self.outscratchdir, fn)):
            self.log('Copy {0}/{2} --> {1}/{3}' \
                        .format(self.outscratchdir, self.outdir, fn, tofn))
            src = os.path.join(self.outscratchdir, fn)
            dst = os.path.join(self.outdir, tofn)
            shutil.copyfile(src, dst)
            return True

        else:
            self.log('Error: Cannot copy {0}/{2} --> {1}/{3}' \
                        .format(self.outscratchdir, self.outdir, fn, tofn))
            return False

    def copyWithinScratch(self, fn, tofn):
        if os.path.isfile(os.path.join(self.outscratchdir, fn)):
            self.log('Copy {0}/{1} --> {0}/{2}' \
                        .format(self.outscratchdir, fn, tofn))
            src = os.path.join(self.outscratchdir, fn)
            dst = os.path.join(self.outscratchdir, tofn)
            shutil.copyfile(src, dst)
            return True

        else:
            self.log('Error: Cannot copy {0}/{1} --> {0}/{2}' \
                        .format(self.outscratchdir, fn, tofn))
            return False

    def writeOutErr(self, prefix, out, err):
        fout = open(os.path.join(self.outdir, prefix + '.out'), 'w')
        fout.write(out)
        fout.close()

        fout = open(os.path.join(self.outdir, prefix + '.err'), 'w')
        fout.write(err)
        fout.close()

        self.log(prefix + '.out:::-->')
        self.log(out.rstrip())
        self.log(prefix + '.out:::<--')
        self.log(prefix + '.err:::-->')
        self.log(err.rstrip())
        self.log(prefix + '.err:::<--')

    def log(self, *args):
        print(*args, file = self._log)
        self._log.flush()

    def _prefix(self):
        return '{0}--{1}--{2}--{3}'.format(self.cfg_search.name,
                                           self.cfg_bench.name,
                                           self.domain, self.problem)
    def _outDir(self):
        return os.path.join(self.cfg.topdir, 'out', self.prefix)

    def _outScratchDir(self):
        return os.path.join(self.cfg.scratchdir, self.prefix)


def runTh(thid, cfg, queue):
    while True:
        next_task = queue.pop()
        if None in next_task:
            pth(thid, 'No more jobs.')
            break

        cur_time = time.time()
        if cfg.end_time - cur_time < next_task[0].max_time + 600:
            pth(thid, 'Not enough time for next task! Terminating...')
            break

        task = ProblemTask(thid, cfg, next_task)
        task.run()

def initPython(jobid, cfg):
    paths = [cfg.pythonpath, cfg.protobuf_egg,
             '/software/python27-modules/software/python-2.7.6/gcc/lib/python2.7/site-packages']
    paths = [x for x in paths if len(x) > 0]
    pythonpath = ':'.join(paths)

    p('Setting PYTHONPATH: {0}'.format(pythonpath))
    os.environ['PYTHONPATH'] = pythonpath

def mainRun(topdir, ppn, time_limit):
    start_time = time.time()
    end_time = start_time + time_limit
    jobid = 'local'
    if 'PBS_JOBID' in os.environ:
        jobid = os.environ['PBS_JOBID']

    os.chdir(topdir)
    initLogging(os.path.join(topdir, 'run-{0}.log'.format(jobid)))
    p('========== START ==========')

    cfg = Config(topdir)
    p('SCRATCHDIR: {0}'.format(cfg.scratchdir))
    cfg.start_time = start_time
    cfg.end_time = end_time

    initPython(jobid, cfg)

    p('Loading benchmarks...')
    cfg.loadBench()

    p('Creating problem queue...')
    queue = ProblemQueue(cfg)

    p('Creating output directory...')
    if not os.path.isdir(os.path.join(cfg.topdir, 'out')):
        os.mkdir(os.path.join(cfg.topdir, 'out'))

    p('Creating threads...')
    ths = [threading.Thread(target = runTh, args = (i, cfg, queue))
            for i in range(ppn)]

    p('Starting threads...')
    for t in ths:
        t.start()
    for t in ths:
        t.join()
    p('All threads joined.')

if __name__ == '__main__':
    if 'METABENCH_TOPDIR' in os.environ:
        topdir = os.environ['METABENCH_TOPDIR']
        ppn = int(os.environ['METABENCH_PPN'])
        time_limit = int(os.environ['METABENCH_TIME_LIMIT'])
        sys.exit(mainRun(topdir, ppn, time_limit))

    if len(sys.argv) != 2:
        print('Usage: {0} dir'.format(sys.argv[0]))
        sys.exit(-1)
    sys.exit(main(sys.argv[1]))
