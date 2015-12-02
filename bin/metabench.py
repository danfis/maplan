#!/usr/bin/env python2

from __future__ import print_function
import sys
import os
import time
import shutil
import ConfigParser as cfgparser

PYTHON = '/usr/bin/python2'

global_start_time = time.time()
def p(*args):
    t = time.time()
    sys.stdout.write('[{0:.3f}] '.format(t - global_start_time))
    print(*args)
    sys.stdout.flush()

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
        section = 'metabench-' + name
        self.path = cfg.get(section, 'path')
        self.type = cfg.get(section, 'type')
        self.pddl = {}

        if self.type in ['seq', 'unfactored', 'unfactor']:
            self._loadSeq(self.path)

        elif self.type in ['factored', 'factor']:
            self._loadFactor(self.path)

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

    def _loadFactor(self, path):
        for pddl_dir in self._pddlDirs(path):
            domain, problem = os.path.split(pddl_dir)
            domain = os.path.split(domain)[-1]
            pddls = []
            domain_pddls = []
            for p, pddl, pddl_domain in self._pddlDomainProblem(pddl_dir):
                pddls += [pddl]
                domain_pddls += [pddl_domain]
            self._addPDDL(domain, problem, domain_pddls, pddls)

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
    def __init__(self, cfg, name):
        self.name = name
        self.section = 'metabench-search-' + name
        self.validate = cfg.getboolean(self.section, 'validate')
        self.type = cfg.get(self.section, 'type')
        self.search = cfg.get(self.section, 'search')
        self.heur = cfg.get(self.section, 'heur')
        self.max_time = cfg.getint(self.section, 'max-time')
        self.max_mem = cfg.getint(self.section, 'max-mem')
        self.repeat = cfg.getint(self.section, 'repeat')

        self.bench = cfg.get(self.section, 'bench')
        self.bench = self.bench.split()
        self.bench = [ConfigBench(cfg, x) for x in self.bench]

        for b in self.bench:
            if b.type != self.type:
                err('Invalid bench {0} for search {1}!'.format(b.name, self.name))

class Config(object):
    def __init__(self, topdir):
        self.topdir = topdir
        self.repodir = os.path.join(topdir, 'repo')
        self.search_bin = os.path.join(self.repodir, 'bin/search')
        self.validate_bin = os.path.join(self.repodir, 'third-party/VAL/validate')
        self.translate_py = os.path.join(self.repodir, 'third-party/translate/translate.py')
        self.protobuf_egg = os.path.join(self.repodir, 'third-party/protobuf/python/build/protobuf-2.6.1-py2.7.egg')

        self.fn = os.path.join(topdir, 'config.cfg')
        self.cfg = cfgparser.ConfigParser()
        self.cfg.read(self.fn)
        self.repo = self.cfg.get('metabench', 'repo')
        self.commit = self.cfg.get('metabench', 'commit')
        self.cplex_includedir = None
        self.cplex_libdir = None
        if self.cfg.has_option('metabench', 'cplex-includedir'):
            self.cplex_includedir = self.cfg.get('metabench', 'cplex-includedir')
        if self.cfg.has_option('metabench', 'cplex-libdir'):
            self.cplex_libdir = self.cfg.get('metabench', 'cplex-libdir')

        self.search = self.cfg.get('metabench', 'search')
        self.search = self.search.split()
        self.search = [ConfigSearch(self.cfg, x) for x in self.search]
        print(self.cfg.items('metabench'))


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

    check_files = [cfg.search_bin,
                   cfg.validate_bin,
                   cfg.translate_py,
                   cfg.protobuf_egg
                  ]
    for f in check_files:
        if not os.path.isfile(f):
            err('Could not find `{0}\''.format(f))

def compileMaplan(cfg):
    repodir = os.path.join(cfg.topdir, 'repo')
    if os.path.exists(cfg.repodir):
        err('Directory for maplan `{0}\' already exist'.format(repodir))

    repoClone(cfg.repo, cfg.repodir)
    repoCheckout(cfg.repodir, cfg.commit)
    repoMaplanMake(cfg)


def shEmpty(cfg, path):
    sh = '''#!/bin/bash
'''
    fout = open(path, 'w')
    fout.write(sh)
    fout.close()

def shTranslateSeq(cfg, path):
    sh = '''#!/bin/bash
START=$(date +%s%N)
cd {0};
{1} --proto --output {0}/problem.proto \\
    {0}/domain.pddl {0}/problem.pddl \\
        >{0}/translate.out 2>{0}/translate.err
touch {0}/translate.done;
END=$(date +%s%N)
echo $(($END - $START)) >{0}/translate.nanotime
'''.format(path, cfg.translate_py)
    fout = open(os.path.join(path, 'translate.sh'), 'w')
    fout.write(sh)
    fout.close()

def shValidateSeq(cfg, path):
    sh = '''#!/bin/bash
START=$(date +%s%N)
cd {0};
{1} -v {0}/domain.pddl {0}/problem.pddl {0}/plan.out \\
    >{0}/validate.out 2>{0}/validate.err
touch {0}/validate.done;
END=$(date +%s%N)
echo $(($END - $START)) >{0}/validate.nanotime
'''.format(path, cfg.validate_bin)
    fout = open(os.path.join(path, 'validate.sh'), 'w')
    fout.write(sh)
    fout.close()

def shMaplanSeq(cfg, cfg_search, path):
    sh = '''#!/bin/bash
START=$(date +%s%N)
cd {0};
{1} -p {0}/problem.proto \\
    -s {2} \\
    -H {3} \\
    --max-time {4} \\
    --max-mem {5} \\
    -o {0}/plan.out \\
    --print-heur-init \\
    >{0}/search.out 2>{0}/search.err
touch {0}/search.done;
END=$(date +%s%N)
echo $(($END - $START)) >{0}/search.nanotime
'''.format(path, cfg.search_bin, cfg_search.search, cfg_search.heur,
           cfg_search.max_time, cfg_search.max_mem)
    fout = open(os.path.join(path, 'search.sh'), 'w')
    fout.write(sh)
    fout.close()

def shSeq(cfg, cfg_search, path):
    sh = '''#!/bin/bash
START=$(date +%s%N)
cd {0};
bash translate.sh
bash search.sh
bash validate.sh

touch {0}/done;
END=$(date +%s%N)
echo $(($END - $START)) >{0}/nanotime
'''.format(path)
    fout = open(os.path.join(path, 'run.sh'), 'w')
    fout.write(sh)
    fout.close()


def createTasksSeq(topdir, cfg, cfg_search, cfg_bench, domain, problem, pddl):
    shutil.copyfile(pddl['domain'], os.path.join(topdir, 'domain.pddl'))
    shutil.copyfile(pddl['problem'], os.path.join(topdir, 'problem.pddl'))
    shTranslateSeq(cfg, topdir)
    if cfg_search.validate:
        shValidateSeq(cfg, topdir)
    else:
        shEmpty(cfg, os.path.join(topdir, 'validate.sh'))
    shMaplanSeq(cfg, cfg_search, topdir)
    shSeq(cfg, cfg_search, topdir)

def createTasksProblem(topdir, cfg, cfg_search, cfg_bench, domain, problem):
    dst = os.path.join(topdir, problem)
    os.mkdir(dst)
    for repeat in range(cfg_search.repeat):
        dst2 = os.path.join(dst, str(repeat))
        os.mkdir(dst2)
        if cfg_search.type == 'seq':
            createTasksSeq(dst2, cfg, cfg_search, cfg_bench,
                           domain, problem, cfg_bench.pddl[domain][problem])

def createTasksDomain(topdir, cfg, cfg_search, cfg_bench, domain):
    dst = os.path.join(topdir, domain)
    os.mkdir(dst)
    for problem in cfg_bench.pddl[domain]:
        createTasksProblem(dst, cfg, cfg_search, cfg_bench, domain, problem)

def createTasksSearchBench(topdir, cfg, cfg_search, cfg_bench):
    dst = os.path.join(topdir, cfg_bench.name)
    os.mkdir(dst)
    for domain in cfg_bench.pddl:
        createTasksDomain(dst, cfg, cfg_search, cfg_bench, domain)

def createTasksSearch(cfg, cfg_search):
    dst = os.path.join(cfg.topdir, cfg_search.name)
    os.mkdir(dst)
    for cfg_bench in cfg_search.bench:
        createTasksSearchBench(dst, cfg, cfg_search, cfg_bench)

def createTasks(cfg):
    for cfg_search in cfg.search:
        createTasksSearch(cfg, cfg_search)

def main(topdir):
    cfg = Config(topdir)
    #compileMaplan(cfg)
    createTasks(cfg)
    #translateSeq(cfg, '/home/danfis/tmp/metabench')
    return 0

if __name__ == '__main__':
    if len(sys.argv) != 2:
        print('Usage: {0} dir'.format(sys.argv[0]))
        sys.exit(-1)
    sys.exit(main(sys.argv[1]))
