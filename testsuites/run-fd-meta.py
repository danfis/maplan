#!/usr/bin/env python

import sys
import os

REPEATS = 5
FD_SEARCH = (
    'ehc(goalcount())',
    'ehc(add())',
    'ehc(hmax())',
    'ehc(ff())',

    'lazy(single(goalcount()))',
    'lazy(single(add()))',
    'lazy(single(hmax()))',
    'lazy(single(ff()))',
)

META_DIR = '/storage/praha1/home/danfis/fd-output/'

META_RUN = '''#!/bin/bash
#PBS -l nodes=1:ppn=1:xeon:debian7:nfs4:cl_luna:nodecpus16
#PBS -l walltime=2h
#PBS -l mem=8gb
#PBS -l scratch=400mb:local

PROBLEM_SAS="/storage/praha1/home/danfis/libplan/data/ma-benchmarks/{sas}"
DOWNWARD="/storage/praha1/home/danfis/fast-downward2/src/search/downward-release"
LOGFILE="{workdir}/log"

WORK_LOGFILE="$SCRATCHDIR/log"

trap "cp $WORK_LOGFILE $LOGFILE.0; rm -r $SCRATCHDIR" TERM EXIT

cp $PROBLEM_SAS $SCRATCHDIR/sas

echo '==RUN {search} ==' >$WORK_LOGFILE
$DOWNWARD --search '{search}' <$SCRATCHDIR/sas >>$WORK_LOGFILE 2>&1
echo '==RUN {search} ==' >>$WORK_LOGFILE
$DOWNWARD --search '{search}' <$SCRATCHDIR/sas >>$WORK_LOGFILE 2>&1
echo '==RUN {search} ==' >>$WORK_LOGFILE
$DOWNWARD --search '{search}' <$SCRATCHDIR/sas >>$WORK_LOGFILE 2>&1
echo '==RUN {search} ==' >>$WORK_LOGFILE
$DOWNWARD --search '{search}' <$SCRATCHDIR/sas >>$WORK_LOGFILE 2>&1
echo '==RUN {search} ==' >>$WORK_LOGFILE
$DOWNWARD --search '{search}' <$SCRATCHDIR/sas >>$WORK_LOGFILE 2>&1

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
    files = filter(lambda x: x[-4:] == '.sas', files)
    for f in files:
        problem = f[:-4]
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
    for i in range(len(FD_SEARCH)):
        workdir = '{0}/{1}/{2}/search-{3}'
        workdir = workdir.format(META_DIR, sas.domain, sas.problem, i)
        cmd = 'ssh meta-brno \'mkdir -p {0}\''.format(workdir)
        print('CMD:', cmd)
        os.system(cmd)

        search = FD_SEARCH[i]
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

