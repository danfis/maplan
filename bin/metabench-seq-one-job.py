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

def absPath(path):
    path = os.path.abspath(path)
    #path = os.path.realpath(path)
    path = path.replace('/auto/praha1', '/storage/praha1/home')
    path = path.replace('/auto/plzen1/home', '/storage/plzen1/home')
    path = path.replace('/auto/brno2', '/storage/brno2/home')
    path = path.replace('/auto/brno3-cerit', '/storage/brno3-cerit/home')
    path = path.replace('/mnt/storage-brno3-cerit/nfs4',
                        '/storage/brno3-cerit')
    return path


class ConfigBench(object):
    def __init__(self, cfg, name):
        self.name = name
        section = 'metabench-bench-' + name
        self.pddl_dir = cfg.cfgGet(section, 'pddl-dir', cfg.pddl_dir)
        self.path = cfg.cfgGet(section, 'path')
        self.skip_domains = cfg.cfgGet(section, 'skip-domains', '').split()
        self.pddl = {}

        if self.path is None:
            err('Missing path option in {0}'.format(section))
        if self.pddl_dir is None:
            err('Missing pddl-dir option in {0}'.format(section))
        if not os.path.isdir(self.pddl_dir):
            err('PDDL repo {0} is not a directory.'.format(self.pddl_dir))

    def load(self):
        self._loadSeq(os.path.join(self.pddl_dir, self.path))

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
            if domain in self.skip_domains:
                continue
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

        keys = ['search_bin', 'validate_bin', 'translate_py',
                'pddl_dir', 'pythonpath',
                'fd_translate_py', 'fd_preprocess', 'fd_translate_opts',
                'fd_search_bin',
                'max_time', 'max_mem', 'bench',
                'search_alg', 'heur',
                'combine', 'type']
        for key in keys:
            opt = key.replace('_', '-')
            val = cfg.cfgGet(self.section, opt, getattr(cfg, key))
            setattr(self, key, val)
        self.fd_search = None

        if search is not None:
            self.search_alg = search
        if heur is not None:
            self.heur = heur

        if self.max_time < 60:
            err('max-time must be explicitly set for at least 60 seconds')
        if self.max_mem < 100:
            err('max-mem must be explicitly set for at least 100MB')

        use_fd = [self.fd_translate_py, self.fd_preprocess,
                  self.fd_translate_opts]
        use_fd = [x for x in use_fd if x is not None]
        if len(use_fd) > 0 and len(use_fd) != 3:
            err('If you want to use FD preprocessor, ' \
                     + 'fd-translate-py, fd-preprocess, and' \
                     + 'fd-translate-opts must be defined!')
        if self.fd_search_bin is not None:
            fd_search_opt = 'fd-search-' + self.search_alg + '-' + self.heur
            self.fd_search = cfg.cfgGet(self.section, fd_search_opt, None)
            if self.fd_search is None:
                err('Missing {0} option in {1}'
                        .format(fd_search_opt, self.section))

        if self.search_bin is None and self.fd_search_bin is None:
            err('{0}: search-bin or fd-search-bin must be defined.'
                    .format(self.name))
        if self.search_bin is not None and not os.path.isfile(self.search_bin):
            err('{0}: search-bin {1} is not a file'
                    .format(self.name, self.search_bin))
        if self.fd_search_bin is not None \
           and not os.path.isfile(self.fd_search_bin):
            err('{0}: fd-search-bin {1} is not a file'
                    .format(self.name, self.fd_search_bin))

        self.bench = self.bench.split()
        self.bench = [ConfigBench(cfg, x) for x in self.bench]


class Config(object):
    def __init__(self, topdir):
        topdir = absPath(topdir)
        self.topdir = topdir
        self.scratchdir = os.path.join(topdir, 'scratch')
        if 'SCRATCHDIR' in os.environ:
            self.scratchdir = os.environ['SCRATCHDIR']
        if not os.path.isdir(self.scratchdir):
            os.mkdir(self.scratchdir)

        self.fn = os.path.join(topdir, 'metabench.cfg')
        self.cfg = cfgparser.ConfigParser()
        self.cfg.read(self.fn)

        self.search_bin = self.cfgGet('metabench', 'search-bin')
        self.validate_bin = self.cfgGet('metabench', 'validate-bin')
        self.translate_py = self.cfgGet('metabench', 'translate-py')
        self.pddl_dir = self.cfgGet('metabench', 'pddl-dir')
        self.pythonpath = self.cfgGet('metabench', 'pythonpath', '')
        self.fd_translate_py = self.cfgGet('metabench', 'fd-translate-py')
        self.fd_preprocess = self.cfgGet('metabench', 'fd-preprocess')
        self.fd_translate_opts = self.cfgGet('metabench', 'fd-translate-opts')
        self.fd_search_bin = self.cfgGet('metabench', 'fd-search-bin')
        self.max_time = self.cfgGet('metabench', 'max-time', 0)
        self.max_mem = self.cfgGet('metabench', 'max-mem', 0)
        self.bench = self.cfgGet('metabench', 'bench', '')
        self.search_alg = self.cfgGet('metabench', 'search-alg', '')
        self.heur = self.cfgGet('metabench', 'heur', '')
        self.combine = self.cfgGet('metabench', 'combine', False)
        self.type = self.cfgGet('metabench', 'type', 'seq')

        self.cluster = self.cfg.get('metabench', 'cluster')
        self.ppn = self.cfg.getint('metabench', 'ppn')
        self.nodes = self.cfg.getint('metabench', 'nodes')
        if self.nodes <= 0:
            raise 'Error: nodes must be >=1'

        search = self.cfg.get('metabench', 'search')
        search = search.split()
        self.search = []
        for s in search:
            self.search += self._parseSearch(s)

    def loadBench(self):
        for s in self.search:
            for b in s.bench:
                b.load()

    def _parseSearch(self, name):
        section = 'metabench-search-' + name
        if self.cfgGet(section, 'combine', self.combine):
            search = self.cfgGet(section, 'search-alg', self.search_alg)
            search = search.split()
            if len(search) == 0:
                err('Option search-alg is not set for {0}'.format(section))
            heur = self.cfgGet(section, 'heur', self.heur).split()
            out = []
            for s in search:
                for h in heur:
                    name2 = name + '-' + s + '-' + h
                    name2 = name2.replace(':', '-')
                    out += [ConfigSearch(self, name, s, h, name2)]
            return out

        else:
            return [ConfigSearch(self, name)]

    def cfgGet(self, section, option, default = None):
        if self.cfg.has_option(section, option):
            if type(default) is bool:
                return self.cfg.getboolean(section, option)
            elif type(default) is int:
                return self.cfg.getint(section, option)
            elif type(default) is float:
                return self.cfg.getfloat(section, option)
            return self.cfg.get(section, option)
        return default



def runLocal(cfg):
    os.environ['METABENCH_TOPDIR'] = cfg.topdir
    os.environ['METABENCH_NODES'] = '1'
    os.environ['METABENCH_NODE_ID'] = '0'
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

def _createRunSh(cfg, max_time, node_id = -1):
    script = absPath(__file__)
    out = '''#!/bin/bash
module add python-2.7.5
#module add python-2.7.10-intel
python2 --version
export METABENCH_TOPDIR={0}
export METABENCH_NODES={1}
export METABENCH_NODE_ID={2}
export METABENCH_PPN={3}
export METABENCH_TIME_LIMIT={4}
python2 {5}
'''.format(cfg.topdir, cfg.nodes, node_id, cfg.ppn, int(max_time), script)
    fn = 'run-main.sh'
    if node_id >= 0:
        fn = 'run-main-{0:02d}.sh'.format(node_id)

    fout = open(os.path.join(cfg.topdir, fn), 'w')
    fout.write(out)
    fout.close()
    return fn

def createRunSh(cfg, max_time):
    return [_createRunSh(cfg, max_time, x) for x in range(cfg.nodes)]

def walltimePro(max_time):
    hours = max_time / 3600
    max_time = max_time % 3600
    minutes = max_time / 60
    secs = max_time % 60
    return '{0}:{1}:{2}'.format(hours, minutes, secs)

def qsub(cfg):
    pbs_pro = False
    if cfg.cluster == 'local':
        runLocal(cfg)
        return

    elif cfg.cluster == 'ungu':
        pbs_pro = True
        resources = 'host=ungu1'
        queue = 'uv@wagap-pro.cerit-sc.cz'
        max_time = 95 * 3600

    elif cfg.cluster == 'urga':
        pbs_pro = True
        resources = 'host=urga1'
        queue = 'uv@wagap-pro.cerit-sc.cz'
        max_time = 95 * 3600

    elif cfg.cluster == 'ida':
        resources = 'cl_ida'
        queue = 'q_1w@arien'
        max_time = 6 * 24 * 3600

    elif cfg.cluster == 'mandos':
        pbs_pro = True
        resources = 'cl_mandos=True'
        queue = 'q_1w@arien-pro.ics.muni.cz'
        max_time = 6 * 24 * 3600

    elif cfg.cluster == 'tarkil':
        pbs_pro = True
        resources = 'cl_tarkil=True'
        queue = 'q_1w@arien-pro.ics.muni.cz'
        max_time = 6 * 24 * 3600

    elif cfg.cluster == 'zewura':
        resources = 'cl_zewura'
        queue = 'default@wagap'
        max_time = 6 * 24 * 3600

    elif cfg.cluster == 'alfrid':
        pbs_pro = True
        resources = 'cl_alfrid=True'
        queue = '@arien-pro.ics.muni.cz'
        max_time = 47 * 3600

    elif cfg.cluster == 'zigur':
        resources = 'cl_zigur'
        queue = '@wagap'
        max_time = 6 * 24 * 3600

    elif cfg.cluster == 'zapat':
        resources = 'cl_zapat'
        queue = '@wagap'
        max_time = 6 * 24 * 3600

    else:
        err('Unkown cluster {0}'.format(cfg.cluster))

    if os.path.isfile(os.path.join(cfg.topdir, 'qsub-00')):
        err('Already qsub\'ed!')

    max_mem  = max(x.max_mem for x in cfg.search)
    max_mem *= cfg.ppn
    max_mem += 1024
    scratch = cfg.ppn * 128

    for node_id, run_sh in enumerate(createRunSh(cfg, max_time)):
        qargs = ['qsub']
        if pbs_pro:
            r  = 'select=1:ncpus={0}'.format(cfg.ppn)
            r += ':{0}'.format(resources)
            r += ':mem={0}mb'.format(max_mem)
            r += ':scratch_local={0}mb'.format(scratch)
            qargs += ['-l', r]
            qargs += ['-l', 'walltime={0}'.format(walltimePro(max_time))]
        else:
            qargs += ['-l', 'nodes=1:ppn={0}:{1}'.format(cfg.ppn, resources)]
            qargs += ['-l', 'walltime={0}s'.format(max_time)]
            qargs += ['-l', 'mem={0}mb'.format(max_mem)]
            qargs += ['-l', 'scratch={0}mb'.format(scratch)]
        qargs += ['-q', queue]
        out = os.path.join(cfg.topdir, 'main-task-{0:02d}.out'.format(node_id))
        err = os.path.join(cfg.topdir, 'main-task-{0:02d}.err'.format(node_id))
        qargs += ['-o', out]
        qargs += ['-e', err]
    #    environ  = 'METABENCH_TOPDIR={0}'.format(cfg.topdir)
    #    environ += ',METABENCH_PPN={0}'.format(cfg.ppn)
    #    environ += ',METABENCH_TIME_LIMIT={0}'.format(max_time)
    #    qargs += ['-v', environ]
        qargs += [os.path.join(cfg.topdir, run_sh)]
        print(qargs)
        out = subprocess.check_output(qargs)

        qsub_fn = os.path.join(cfg.topdir, 'qsub-{0:02d}'.format(node_id))
        fout = open(qsub_fn, 'w')
        p('{0} -> {1}'.format(out.strip(), qsub_fn))
        fout.write(out.strip())
        fout.close()

def main(topdir):
    initLogging(os.path.join(topdir, 'metabench.log'))
    p('========== START ==========')

    cfg = Config(topdir)
    cfg.loadBench()

    p('Creating output directory...')
    if not os.path.isdir(os.path.join(cfg.topdir, 'out')):
        os.mkdir(os.path.join(cfg.topdir, 'out'))

    qsub(cfg)

    #createTasks(cfg)
    #qsub(cfg)
    return 0



class ProblemQueue(object):
    def __init__(self, cfg, nodes, node_id):
        self._queue = []
        self._lock = threading.Lock()

        for search in sorted(cfg.search, key = lambda x: x.name):
            for bench in sorted(search.bench, key = lambda x: x.name):
                for domain in sorted(bench.pddl.keys()):
                    for problem in sorted(bench.pddl[domain].keys()):
                        d = (search, bench, domain, problem,
                             bench.pddl[domain][problem]['domain'],
                             bench.pddl[domain][problem]['problem'])
                        self._queue += [d]
        if nodes > 0 and node_id >= 0:
            self._queue = self._queue[node_id::nodes]

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
            if self.cfg_search.fd_search_bin is not None:
                self.searchFD()
            else:
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

    def searchFD(self):
        self.log('fd-search: START')

        max_time  = self.cfg_search.max_time
        max_time -= self.translate_time
        max_time = int(max_time)
        max_mem = self.cfg_search.max_mem

        cmd  = [self.cfg_search.fd_search_bin]
        cmd += ['--search', '\'' + self.cfg_search.fd_search + '\'']
        cmd += ['--internal-plan-file',
                os.path.join(self.outscratchdir, 'plan.out')]
        cmd += ['<{0}'.format(os.path.join(self.outscratchdir, 'problem.fd'))]

        self.log('fd-search: cmd: {0}'.format(' '.join(cmd)))
        ret, out, err, tm = self._runCmd(cmd, max_mem = max_mem)
        self.log('fd-search: --> {0}'.format(ret))
        self.log('fd-search: time {0}'.format(tm))
        self.search_time = tm
        self.writeOutErr('fd-search', out, err)
        self.copyFromScratch('plan.out')

        return ret

    def search(self):
        self.log('search: START')

        max_time  = self.cfg_search.max_time
        max_time -= self.translate_time
        max_time = int(max_time)
        max_mem = self.cfg_search.max_mem

        cmd  = [self.cfg_search.search_bin]
        cmd += ['--fd-problem', os.path.join(self.outscratchdir, 'problem.fd')]
        cmd += ['-s', self.cfg_search.search_alg]
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

        if self.cfg_search.validate_bin is None:
            self.log('Skipping validation because validate-bin is not set.')
            return

        if os.path.isfile(os.path.join(self.outscratchdir, 'plan.out')):
            cmd  = [self.cfg_search.validate_bin]
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
    paths = [cfg.pythonpath]
    paths = [x for x in paths if len(x) > 0]
    pythonpath = ':'.join(paths)

    p('Setting PYTHONPATH: {0}'.format(pythonpath))
    os.environ['PYTHONPATH'] = pythonpath

def mainRun(topdir, nodes, node_id, ppn, time_limit):
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
    queue = ProblemQueue(cfg, nodes, node_id)

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
        nodes = int(os.environ['METABENCH_NODES'])
        node_id = int(os.environ['METABENCH_NODE_ID'])
        ppn = int(os.environ['METABENCH_PPN'])
        time_limit = int(os.environ['METABENCH_TIME_LIMIT'])
        sys.exit(mainRun(topdir, nodes, node_id, ppn, time_limit))

    if len(sys.argv) != 2:
        print('Usage: {0} dir'.format(sys.argv[0]))
        sys.exit(-1)
    sys.exit(main(sys.argv[1]))
