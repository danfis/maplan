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
import ConfigParser as cfgparser
from matopddl import PlanningProblem

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
    def __init__(self, cfg, name, search = None, heur = None, name2 = None):
        if name2 is None:
            self.name = name
        else:
            self.name = name2
        self.section = 'metabench-search-' + name
        self.type = cfg.get(self.section, 'type')
        self.search = search
        if search is None:
            self.search = cfg.get(self.section, 'search')
        self.heur = heur
        if heur is None:
            self.heur = cfg.get(self.section, 'heur')
        self.max_time = cfg.getint(self.section, 'max-time')
        self.max_mem = cfg.getint(self.section, 'max-mem')
        self.repeat = cfg.getint(self.section, 'repeat')
        self.cluster = cfg.get(self.section, 'cluster')
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

        for b in self.bench:
            if b.type != self.type:
                err('Invalid bench {0} for search {1}!'.format(b.name, self.name))

class Config(object):
    def __init__(self, topdir):
        topdir = os.path.abspath(topdir)
        topdir = topdir.replace('/auto/praha1/danfis', '/storage/praha1/home/danfis')
        self.topdir = topdir
        self.repodir = os.path.join(topdir, 'repo')
        self.search_bin = os.path.join(self.repodir, 'bin/search')
        self.validate_bin = os.path.join(self.repodir, 'third-party/VAL/validate')
        self.translate_py = os.path.join(self.repodir, 'third-party/translate/translate.py')
        self.protobuf_egg = os.path.join(self.repodir, 'third-party/protobuf/python/build/protobuf-2.6.1-py2.7.egg')

        self.fn = os.path.join(topdir, 'metabench.cfg')
        self.cfg = cfgparser.ConfigParser()
        self.cfg.read(self.fn)
        self.repo = self.cfg.get('metabench', 'repo')
        self.commit = self.cfg.get('metabench', 'commit')
        self.cplex_includedir = self._cfgGet('metabench', 'cplex-includedir',
                                              '/software/cplex-126/cplex/include')
        self.cplex_libdir = self._cfgGet('metabench', 'cplex-libdir',
                                         '/software/cplex-126/cplex/lib/x86-64_linux/static_pic')

        self.run_in_batch = self._cfgGet('metabench', 'run-in-batch', 0)
        self.pythonpath = self._cfgGet('metabench', 'pythonpath', '')
        self.copyfile = self._cfgGet('metabench', 'copyfile', False)

        search = self.cfg.get('metabench', 'search')
        search = search.split()
        self.search = []
        for s in search:
            self.search += self._parseSearch(s)

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
    repodir = os.path.join(cfg.topdir, 'repo')
    if os.path.exists(cfg.repodir):
        err('Directory for maplan `{0}\' already exist'.format(repodir))

    repoClone(cfg.repo, cfg.repodir)
    repoCheckout(cfg.repodir, cfg.commit)
    repoMaplanMake(cfg)
    repoCheck(cfg)


def shEmpty(cfg, path):
    sh = '''#!/bin/bash
'''
    fout = open(path, 'w')
    fout.write(sh)
    fout.close()

def shAllocPorts(cfg, path):
    sh = '''#!/bin/bash
START=$(date +%s%N)
cd {0};

count=0
node_id=0
for node in $(cat $PBS_NODEFILE); do
    ip=$(dig +short $node);

    port=$(($RANDOM % 10000 + 10000));
    while nc -z $ip $port; do
        port=$(($RANDOM % 10000 + 10000));
        count=$(($count + 1))
        if [ $count = 20 ]; then
            rm -f {0}/ports.*;
            exit -1;
        fi
    done

    port2=$(($RANDOM % 10000 + 10000));
    while nc -z $ip $port2; do
        port2=$(($RANDOM % 10000 + 10000));
        count=$(($count + 1))
        if [ $count = 20 ]; then
            rm -f {0}/ports.*;
            exit -1;
        fi
    done

    echo "$ip $port $port2" >{0}/ports.$node_id
    node_id=$(($node_id + 1))
done;

touch {0}/alloc-ports.done
END=$(date +%s%N)
echo $(($END - $START)) >{0}/alloc-ports.nanotime
'''.format(path)
    fout = open(os.path.join(path, 'alloc-ports.sh'), 'w')
    fout.write(sh)
    fout.close()

def shTranslateSeq(cfg, path):
    pythonpath = cfg.pythonpath
    if len(pythonpath) > 0:
        pythonpath += ':'

    sh = '''#!/bin/bash
START=$(date +%s%N)
cd {0};
PYTHONPATH={3}{2}:/software/python27-modules/software/python-2.7.6/gcc/lib/python2.7/site-packages \\
python2 {1} --proto --output {0}/problem.proto \\
    {0}/domain.pddl {0}/problem.pddl \\
        >{0}/translate.out 2>{0}/translate.err
touch {0}/translate.done;
END=$(date +%s%N)
echo $(($END - $START)) >{0}/translate.nanotime
'''.format(path, cfg.translate_py, cfg.protobuf_egg, pythonpath)
    fout = open(os.path.join(path, 'translate.sh'), 'w')
    fout.write(sh)
    fout.close()

def shTranslateFDSeq(cfg, cfg_search, path):
    pythonpath = cfg.pythonpath
    if len(pythonpath) > 0:
        pythonpath += ':'

    opts = cfg_search.fd_translate_opts
    sh = '''#!/bin/bash
START=$(date +%s%N)
cd {0};
PYTHONPATH={5}{3}:/software/python27-modules/software/python-2.7.6/gcc/lib/python2.7/site-packages \\
python2 {1} {2} {0}/domain.pddl {0}/problem.pddl \\
        >{0}/fd-translate.out 2>{0}/fd-translate.err
{4} <{0}/output.sas >{0}/fd-preprocess.out 2>{0}/fd-preprocess.err
mv {0}/output {0}/problem.fd
touch {0}/translate.done;
END=$(date +%s%N)
echo $(($END - $START)) >{0}/translate.nanotime
'''.format(path, cfg_search.fd_translate_py, opts, cfg.protobuf_egg,
           cfg_search.fd_preprocess, pythonpath)
    fout = open(os.path.join(path, 'translate.sh'), 'w')
    fout.write(sh)
    fout.close()

def shTranslateUnfactor(cfg, path):
    pythonpath = cfg.pythonpath
    if len(pythonpath) > 0:
        pythonpath += ':'

    sh = '''#!/bin/bash
START=$(date +%s%N)
cd {0};
PYTHONPATH={3}{2}:/software/python27-modules/software/python-2.7.6/gcc/lib/python2.7/site-packages \\
{1} --proto --output {0}/problem.proto \\
    {0}/domain.pddl {0}/problem.pddl {0}/problem.addl \\
        >{0}/translate.out 2>{0}/translate.err
touch {0}/translate.done;
END=$(date +%s%N)
echo $(($END - $START)) >{0}/translate.nanotime
'''.format(path, cfg.translate_py, cfg.protobuf_egg, pythonpath)
    fout = open(os.path.join(path, 'translate.sh'), 'w')
    fout.write(sh)
    fout.close()

def shTranslateFactor(cfg, path):
    pythonpath = cfg.pythonpath
    if len(pythonpath) > 0:
        pythonpath += ':'

    sh = '''#!/bin/bash
START=$(date +%s%N)
cd {0};

node_id=$PBS_NODENUM

num_agents=$(cat num-agents)
agent_id=0
agent_url=""
while [ $agent_id != $num_agents ]; do
    a=($(cat ports.$agent_id))
    ip=${{a[0]}}
    port=${{a[1]}}
    port2=${{a[2]}}
    agent_url="$agent_url --agent-url tcp://$ip:$port2"

    agent_id=$(($agent_id + 1))
done

PYTHONPATH={3}{2}:/software/python27-modules/software/python-2.7.6/gcc/lib/python2.7/site-packages \\
{1} --proto --output {0}/problem.${{node_id}}.proto \\
    $agent_url \\
    --agent-id $node_id \\
    {0}/domain.${{node_id}}.pddl {0}/problem.${{node_id}}.pddl \\
        >{0}/translate.${{node_id}}.out 2>{0}/translate.${{node_id}}.err
touch {0}/translate.${{node_id}}.done;
END=$(date +%s%N)
echo $(($END - $START)) >{0}/translate.${{node_id}}.nanotime
'''.format(path, cfg.translate_py, cfg.protobuf_egg, pythonpath)
    fout = open(os.path.join(path, 'translate.sh'), 'w')
    fout.write(sh)
    fout.close()

def shValidateSeq(cfg, path):
    sh = '''#!/bin/bash
START=$(date +%s%N)
cd {0};
if [ -f {0}/plan.out ]; then
    {1} -v {0}/domain.pddl {0}/problem.pddl {0}/plan.out \\
        >{0}/validate.out 2>{0}/validate.err
else
    echo "NO PLAN" >validate.out
fi
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

def shMaplanFDSeq(cfg, cfg_search, path):
    sh = '''#!/bin/bash
START=$(date +%s%N)
cd {0};
{1} --fd-problem {0}/problem.fd \\
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

def shMaplanUnfactor(cfg, cfg_search, path):
    sh = '''#!/bin/bash
START=$(date +%s%N)
cd {0};
{1} -p {0}/problem.proto \\
    -s {2} \\
    -H {3} \\
    --max-time {4} \\
    --max-mem {5} \\
    --ma-unfactor \\
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

def shMaplanFactor(cfg, cfg_search, path):
    sh = '''#!/bin/bash
START=$(date +%s%N)
cd {0};

node_id=$PBS_NODENUM

num_agents=$(cat num-agents)
agent_id=0
tcp=""
while [ $agent_id != $num_agents ]; do
    a=($(cat ports.$agent_id))
    ip=${{a[0]}}
    port=${{a[1]}}
    port2=${{a[2]}}
    tcp="$tcp --tcp $ip:$port2"

    agent_id=$(($agent_id + 1))
done

{1} -p {0}/problem.${{node_id}}.proto \\
    -s {2} \\
    -H {3} \\
    --max-time {4} \\
    --max-mem {5} \\
    --ma-factor \\
    $tcp \\
    --tcp-id $node_id \\
    -o {0}/plan.${{node_id}}.out \\
    --print-heur-init \\
    >{0}/search.${{node_id}}.out 2>{0}/search.${{node_id}}.err
touch {0}/search.${{node_id}}.done;
END=$(date +%s%N)
echo $(($END - $START)) >{0}/search.${{node_id}}.nanotime
'''.format(path, cfg.search_bin, cfg_search.search, cfg_search.heur,
           cfg_search.max_time, cfg_search.max_mem)
    fout = open(os.path.join(path, 'search.sh'), 'w')
    fout.write(sh)
    fout.close()

def shSeq(cfg, cfg_search, path):
    sh = '''#!/bin/bash
START=$(date +%s%N)
cd {0};
ulimit -v {1}
bash translate.sh
bash search.sh
bash validate.sh

touch {0}/done;
END=$(date +%s%N)
echo $(($END - $START)) >{0}/nanotime
'''.format(path, cfg_search.max_mem * 1024)
    fout = open(os.path.join(path, 'run.sh'), 'w')
    fout.write(sh)
    fout.close()

def shFactorRun(cfg, cfg_search, path):
    sh = '''#!/bin/bash
START=$(date +%s%N)
cd {0};

node_id=$PBS_NODENUM
bash translate.sh
bash search.sh
#bash validate.sh

touch {0}/done.$node_id;
END=$(date +%s%N)
echo $(($END - $START)) >{0}/nanotime.$node_id
'''.format(path)
    fout = open(os.path.join(path, 'factor-run.sh'), 'w')
    fout.write(sh)
    fout.close()

def shFactor(cfg, path):
    sh = '''#!/bin/bash
START=$(date +%s%N)
cd {0};
bash alloc-ports.sh
if [ ! -f {0}/ports.0 ]; then
    echo "No ports allocated!"
    exit -1
fi

pbsdsh -- bash {0}/factor-run.sh

touch {0}/done;
END=$(date +%s%N)
echo $(($END - $START)) >{0}/nanotime
'''.format(path)
    fout = open(os.path.join(path, 'run.sh'), 'w')
    fout.write(sh)
    fout.close()

def shBatch(cfg, path, dirs):
    sh = '''#!/bin/bash
START=$(date +%s%N)
cd {0};
'''.format(path)
    for d in dirs:
        sh += '''
if [ ! -f {0}/done ]; then
    bash {0}/run.sh >{0}/task.out 2>{0}/task.err;
fi
'''.format(d)
    sh += '''
touch {0}/done;
END=$(date +%s%N)
echo $(($END - $START)) >{0}/nanotime
'''.format(path)
    fout = open(os.path.join(path, 'run-batch.sh'), 'w')
    fout.write(sh)
    fout.close()

    fout = open(os.path.join(path, 'size'), 'w')
    fout.write(str(len(dirs)))
    fout.close()

    for d in dirs:
        fout = open(os.path.join(d, 'batch'), 'w')
        fout.write(path)
        fout.close()


def createTasksSeq(topdir, cfg, cfg_search, cfg_bench, domain, problem, pddl):
    if os.path.exists(os.path.join(topdir, 'domain.pddl')):
        p('Skipping:', topdir, 'already created')
        return;
    p('Creating', topdir)
    if cfg.copyfile:
        shutil.copyfile(pddl['domain'], os.path.join(topdir, 'domain.pddl'))
        shutil.copyfile(pddl['problem'], os.path.join(topdir, 'problem.pddl'))
    else:
        os.link(pddl['domain'], os.path.join(topdir, 'domain.pddl'))
        os.link(pddl['problem'], os.path.join(topdir, 'problem.pddl'))
    if cfg_search.fd_translate_py is not None:
        shTranslateFDSeq(cfg, cfg_search, topdir)
    else:
        shTranslateSeq(cfg, topdir)
    shValidateSeq(cfg, topdir)
    if cfg_search.fd_translate_py is not None:
        shMaplanFDSeq(cfg, cfg_search, topdir)
    else:
        shMaplanSeq(cfg, cfg_search, topdir)
    shSeq(cfg, cfg_search, topdir)

def createTasksUnfactor(topdir, cfg, cfg_search, cfg_bench, domain, problem, pddl):
    if os.path.exists(os.path.join(topdir, 'domain.pddl')):
        p('Skipping:', topdir, 'already created')
        return;
    p('Creating', topdir)
    pp = PlanningProblem(pddl['domain'], pddl['problem'])
    pp.write_pddl_domain(os.path.join(topdir, 'domain.pddl'))
    pp.write_pddl_problem(os.path.join(topdir, 'problem.pddl'))
    pp.write_addl(os.path.join(topdir, 'problem.addl'))
    pp.write_agent_list(os.path.join(topdir, 'problem.agents'))
    shTranslateUnfactor(cfg, topdir)
    shValidateSeq(cfg, topdir)
    shMaplanUnfactor(cfg, cfg_search, topdir)
    shSeq(cfg, cfg_search, topdir)

def createTasksFactor(topdir, cfg, cfg_search, cfg_bench, domain, problem, pddl):
    if os.path.exists(os.path.join(topdir, 'domain.0.pddl')):
        p('Skipping:', topdir, 'already created')
        return;
    p('Creating', topdir)
    num_agents = len(pddl['domain'])
    for i in range(num_agents):
        dom = 'domain.{0}.pddl'.format(i)
        prob = 'problem.{0}.pddl'.format(i)
        if cfg.copyfile:
            shutil.copyfile(pddl['domain'][i], os.path.join(topdir, dom))
            shutil.copyfile(pddl['problem'][i], os.path.join(topdir, prob))
        else:
            os.link(pddl['domain'][i], os.path.join(topdir, dom))
            os.link(pddl['problem'][i], os.path.join(topdir, prob))
    with open(os.path.join(topdir, 'num-agents'), 'w') as fout:
        fout.write('{0}'.format(num_agents))
        fout.close()

    shAllocPorts(cfg, topdir)
    shTranslateFactor(cfg, topdir)
    #shValidateSeq(cfg, topdir)
    shMaplanFactor(cfg, cfg_search, topdir)
    shFactorRun(cfg, cfg_search, topdir)
    shFactor(cfg, topdir)

def createTasksProblem(topdir, cfg, cfg_search, cfg_bench, domain, problem):
    dst = os.path.join(topdir, problem)
    if not os.path.isdir(dst):
        p('Creating', dst)
        os.mkdir(dst)
    for repeat in range(cfg_search.repeat):
        dst2 = os.path.join(dst, str(repeat))
        if not os.path.isdir(dst2):
            os.mkdir(dst2)
        if cfg_search.type == 'seq':
            createTasksSeq(dst2, cfg, cfg_search, cfg_bench,
                           domain, problem, cfg_bench.pddl[domain][problem])
        elif cfg_search.type in ['unfactored', 'unfactor']:
            createTasksUnfactor(dst2, cfg, cfg_search, cfg_bench,
                                domain, problem, cfg_bench.pddl[domain][problem])
        elif cfg_search.type in ['factored', 'factor']:
            createTasksFactor(dst2, cfg, cfg_search, cfg_bench,
                              domain, problem, cfg_bench.pddl[domain][problem])
        else:
            raise Exception('Unkown type {0}'.format(cfg_search.type))

def createDomainBatches(cfg, topdir):
    task_dirs = []
    for root, dirs, files in os.walk(topdir):
        if 'run.sh' in files:
            task_dirs += [root]

    random.shuffle(task_dirs)
    for bi, i in enumerate(range(0, len(task_dirs), cfg.run_in_batch)):
        dst = os.path.join(topdir, 'batch-{0:06d}'.format(bi))
        if not os.path.isdir(dst):
            p('Creating', dst)
            os.mkdir(dst)
        shBatch(cfg, dst, task_dirs[i:i+cfg.run_in_batch])

def createTasksDomain(topdir, cfg, cfg_search, cfg_bench, domain):
    dst = os.path.join(topdir, domain)
    if not os.path.isdir(dst):
        p('Creating', dst)
        os.mkdir(dst)

    for problem in cfg_bench.pddl[domain]:
        createTasksProblem(dst, cfg, cfg_search, cfg_bench, domain, problem)
    createDomainBatches(cfg, dst)

def createTasksSearchBench(topdir, cfg, cfg_search, cfg_bench):
    dst = os.path.join(topdir, cfg_bench.name)
    if not os.path.isdir(dst):
        p('Creating', dst)
        os.mkdir(dst)
    for domain in cfg_bench.pddl:
        createTasksDomain(dst, cfg, cfg_search, cfg_bench, domain)

def createTasksSearch(cfg, cfg_search):
    dst = os.path.join(cfg.topdir, cfg_search.name)
    if not os.path.isdir(dst):
        p('Creating', dst)
        os.mkdir(dst)
    for cfg_bench in cfg_search.bench:
        createTasksSearchBench(dst, cfg, cfg_search, cfg_bench)

def createTasks(cfg):
    for cfg_search in cfg.search:
        createTasksSearch(cfg, cfg_search)




pat_ma_agents = re.compile(r'^.*\(:agents +([a-zA-Z0-9-_ ]+)\).*$')
def numAgents(addl):
    with open(addl, 'r') as fin:
        data = fin.read()
        data = data.replace('\r\n', ' ')
        data = data.replace('\n', ' ')
        data = data.replace('\r', ' ')
        match = pat_ma_agents.match(data)
        if match is None:
            p('Error: Could not parse `{0}\''.format(addl))
        agents = match.group(1).split()
        agents = filter(lambda x: len(x) > 0, agents)
        return len(agents)

def qsubCmd(cfg, cfg_search, root, script = 'run.sh', size = 1):
    max_time = cfg_search.max_time + 600
    max_mem = cfg_search.max_mem + 128
    if cfg_search.cluster == 'luna':
        resources = 'cl_luna'
        queue = '@arien'
    elif cfg_search.cluster == 'zapat':
        resources = 'cl_zapat'
        queue = '@wagap'
    elif cfg_search.cluster == 'zebra':
        resources = 'cl_zebra'
        queue = '@wagap'
    elif cfg_search.cluster == 'urga':
        resources = 'cl_urga'
        queue = 'uv@wagap'
    elif cfg_search.cluster == 'ungu':
        resources = 'cl_ungu'
        queue = 'uv@wagap'

    if cfg_search.type == 'seq':
        ppn = 1
        nodes = 1
    elif cfg_search.type in ['unfactored', 'unfactor']:
        ppn = numAgents(os.path.join(root, 'problem.addl'))
        ppn = min(ppn + 1, 16)
        nodes = 1
    elif cfg_search.type in ['factored', 'factor']:
        nodes = int(open(os.path.join(root, 'num-agents'), 'r').read())
        nodes = min(nodes, 35)
        ppn = 1
    else:
        raise Exception('TODO')

    qargs = ['qsub']
    qargs += ['-l', 'nodes={0}:ppn={1}:{2}'.format(nodes, ppn, resources)]
    qargs += ['-l', 'walltime={0}s'.format(max_time * size)]
    qargs += ['-l', 'mem={0}mb'.format(max_mem)]
    qargs += ['-q', queue]
    qargs += ['-o', os.path.join(root, 'task.out')]
    qargs += ['-e', os.path.join(root, 'task.err')]
    #qargs += ['-v', 'BENCH_DATA_FN={0},BENCH_TASK_I={1}'.format(bench_fn, task_i)]
    qargs += [os.path.join(root, script)]
    return qargs

def qsubTopdir(cfg, cfg_search, topdir):
    script = 'run.sh'
    if cfg.run_in_batch > 0:
        script = 'run-batch.sh'

    for root, dirs, files in os.walk(topdir):
        if script in files:
            if 'qsub' in files:
                p('qsub: Skipping', root)
                continue

            size = 1
            if cfg.run_in_batch > 0:
                fin = open(os.path.join(root, 'size'), 'r')
                size = int(fin.read())
                fin.close()

            cmd = qsubCmd(cfg, cfg_search, root, script, size)
            out = subprocess.check_output(cmd)
            qsub_fn = os.path.join(root, 'qsub')
            with open(qsub_fn, 'w') as fout:
                p('{0} -> {1}'.format(out.strip(), qsub_fn))
                fout.write(out)

def qsub(cfg):
    for cfg_search in cfg.search:
        topdir = os.path.join(cfg.topdir, cfg_search.name)
        qsubTopdir(cfg, cfg_search, topdir)


def main(topdir):
    initLogging(os.path.join(topdir, 'metabench.log'))
    p('========== START ==========')

    cfg = Config(topdir)
    if not os.path.exists(cfg.repodir):
        compileMaplan(cfg)
    else:
        repoCheck(cfg)
    createTasks(cfg)
    qsub(cfg)
    return 0

if __name__ == '__main__':
    if len(sys.argv) != 2:
        print('Usage: {0} dir'.format(sys.argv[0]))
        sys.exit(-1)
    sys.exit(main(sys.argv[1]))
