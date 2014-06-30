#!/usr/bin/env python

import sys
import os

REPEATS = 5
SEARCH = (
    ['-s', 'ehc', '-H', 'goalcount'],
    ['-s', 'ehc', '-H', 'add'],
    ['-s', 'ehc', '-H', 'max'],
    ['-s', 'ehc', '-H', 'ff'],

    ['-s', 'lazy', '-l', 'heap', '-H', 'goalcount'],
    ['-s', 'lazy', '-l', 'heap', '-H', 'add'],
    ['-s', 'lazy', '-l', 'heap', '-H', 'max'],
    ['-s', 'lazy', '-l', 'heap', '-H', 'ff'],

    ['-s', 'lazy', '-l', 'bucket', '-H', 'goalcount'],
    ['-s', 'lazy', '-l', 'bucket', '-H', 'add'],
    ['-s', 'lazy', '-l', 'bucket', '-H', 'max'],
    ['-s', 'lazy', '-l', 'bucket', '-H', 'ff'],

    ['-s', 'lazy', '-l', 'rbtree', '-H', 'goalcount'],
    ['-s', 'lazy', '-l', 'rbtree', '-H', 'add'],
    ['-s', 'lazy', '-l', 'rbtree', '-H', 'max'],
    ['-s', 'lazy', '-l', 'rbtree', '-H', 'ff'],

    ['-s', 'lazy', '-l', 'splaytree', '-H', 'goalcount'],
    ['-s', 'lazy', '-l', 'splaytree', '-H', 'add'],
    ['-s', 'lazy', '-l', 'splaytree', '-H', 'max'],
    ['-s', 'lazy', '-l', 'splaytree', '-H', 'ff'],
)

META_DIR = '/storage/praha1/home/danfis/libplan-output/'

META_RUN = '''#!/bin/bash
#PBS -l nodes=1:ppn=1:xeon:debian7:nfs4:cl_luna:nodecpus16
#PBS -l walltime=3h
#PBS -l mem=8gb
#PBS -l scratch=400mb:local

PROBLEM_SAS="/storage/praha1/home/danfis/libplan/data/ma-benchmarks/{sas}"
BIN="/storage/praha1/home/danfis/libplan/testsuites/search"
LOGFILE="{workdir}/log"

WORK_LOGFILE="$SCRATCHDIR/log"

trap "cp $WORK_LOGFILE $LOGFILE.0; rm -r $SCRATCHDIR" TERM EXIT

cp $PROBLEM_SAS $SCRATCHDIR/sas

echo '==RUN {search} ==' >$WORK_LOGFILE
$BIN {search} --max-mem 4000000 -p $SCRATCHDIR/sas >>$WORK_LOGFILE 2>&1
echo '==RUN {search} ==' >>$WORK_LOGFILE
$BIN {search} --max-mem 4000000 -p $SCRATCHDIR/sas >>$WORK_LOGFILE 2>&1
echo '==RUN {search} ==' >>$WORK_LOGFILE
$BIN {search} --max-mem 4000000 -p $SCRATCHDIR/sas >>$WORK_LOGFILE 2>&1
echo '==RUN {search} ==' >>$WORK_LOGFILE
$BIN {search} --max-mem 4000000 -p $SCRATCHDIR/sas >>$WORK_LOGFILE 2>&1
echo '==RUN {search} ==' >>$WORK_LOGFILE
$BIN {search} --max-mem 4000000 -p $SCRATCHDIR/sas >>$WORK_LOGFILE 2>&1

cp $WORK_LOGFILE $LOGFILE
'''


class Sas(object):
    def __init__(self, domain, problem, sas):
        self.domain = domain
        self.problem = problem
        self.sas = sas

def loadSasDir(domain, dr):
    sas = []

    files = os.listdir(dr)
    files = filter(lambda x: x[-5:] == '.json', files)
    for f in files:
        problem = f[:-5]
        sfile = domain + '/' + f
        sas += [Sas(domain, problem, sfile)]

    return sas

def loadSas(topdir):
    sas = []
    for dr in os.listdir(topdir):
        domain = dr
        dr = os.path.join(topdir, dr)
        if os.path.isdir(dr):
            sas += loadSasDir(domain, dr)

    return sas

def run(sas):
    for i in range(len(SEARCH)):
        workdir = '{0}/{1}/{2}/search-{3}'
        workdir = workdir.format(META_DIR, sas.domain, sas.problem, i)
        cmd = 'ssh meta-brno \'mkdir -p {0}\''.format(workdir)
        print('CMD:', cmd)
        os.system(cmd)

        search = ' '.join(SEARCH[i])
        run_sh = META_RUN.format(search = search, workdir = workdir,
                                 sas = sas.sas)
        with open('/tmp/run.sh', 'w') as fout:
            fout.write(run_sh)
        cmd = 'scp /tmp/run.sh meta-brno:{0}/run.sh'.format(workdir)
        print('CMD:', cmd)
        os.system(cmd)

        cmd = 'ssh meta-brno \'cd {0} && qsub run.sh\''.format(workdir)
        print('CMD:', cmd)
        os.system(cmd)

def main(topdir):
    cmd = 'ssh meta-brno \'rm -rf {0}\''.format(META_DIR)
    print('CMD:', cmd)
    os.system(cmd)

    sas = loadSas(topdir)
    for s in sas:
        print('PROBLEM:', s.domain, s.problem, s.sas)
        run(s)


if __name__ == '__main__':
    main(sys.argv[1])

