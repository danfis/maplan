[metabench]
repo = https://github.com/danfis/maplan
commit = master
cplex-includedir = /opt/cplex-1263/cplex/include
cplex-libdir = /opt/cplex-1263/cplex/lib/x86-64_linux/static_pic/
#cplex-includedir = /software/cplex-126/cplex/include
#cplex-libdir = /software/cplex-126/cplex/lib/x86-64_linux/static_pic
#search = lazy-ff ma-lazy-ff
#search = all-seq lazy-ff
search = fd-opt fd-cplex-opt
run-per-domain = true

[metabench-search-all-seq]
combine = true
type = seq
search = lazy ehc astar
heur = goalcount add max ff dtg lm-cut lm-cut-inc-local lm-cut-inc-cache
       lm-cut-inc-cache:prune flow flow:lm-cut flot:ilp potential
max-time = 1800
max-mem = 8192
repeat = 10
bench = ipc-2011-sat ipc-2011-opt ipc-2014-sat ipc-2014-opt
cluster = luna

[metabench-search-lazy-ff]
type = seq
search = lazy
heur = ff
max-time = 1800
max-mem = 8192
repeat = 5
bench = ipc-2006 ipc-2014-sat
cluster = luna

[metabench-search-lazy-ff-unfactor]
type = unfactored
search = lazy
heur = ff
max-time = 1800
max-mem = 8192
repeat = 5
bench = codmap-2015-unfactored
cluster = luna

[metabench-search-lazy-ff-factor]
type = factored
search = lazy
heur = ff
max-time = 1800
max-mem = 8192
repeat = 5
bench = codmap-2015-factored
cluster = luna

[metabench-search-fd-opt]
type = seq
search = astar
heur = lm-cut
fd-translate-py = /home/danfis/dev/fast-downward-work/src/build/bin/translate/translate.py
fd-preprocess = /home/danfis/dev/fast-downward-work/src/build/bin/preprocess
fd-translate-opts = --cplex
max-time = 1800
max-mem = 8192
repeat = 1
bench = test-seq
cluster = luna

[metabench-search-fd-cplex-opt]
type = seq
search = astar
heur = lm-cut
fd-translate-py = /home/danfis/dev/fast-downward-work/src/build/bin/translate/translate.py
fd-preprocess = /home/danfis/dev/fast-downward-work/src/build/bin/preprocess
fd-translate-cplex = true
max-time = 1800
max-mem = 8192
repeat = 1
bench = test-seq
cluster = luna


[metabench-test-seq]
path = /home/danfis/dev/pddl-data/test-seq
type = seq

[metabench-test-unfactor]
path = /home/danfis/dev/plan-data/raw/test-unfactor
type = unfactored

[metabench-ipc-2006]
path = /home/danfis/dev/plan-data/raw/ipc-2006
type = seq

[metabench-ipc-2011-sat]
path = /home/danfis/dev/plan-data/raw/ipc-2011/seq-sat
type = seq

[metabench-ipc-2011-opt]
path = /home/danfis/dev/plan-data/raw/ipc-2011/seq-opt
type = seq

[metabench-ipc-2014-sat]
path = /home/danfis/dev/plan-data/raw/ipc-2014/seq-sat
type = seq

[metabench-ipc-2014-opt]
path = /home/danfis/dev/plan-data/raw/ipc-2014/seq-opt
type = seq

[metabench-codmap-2015-factored]
path = /home/danfis/dev/plan-data/raw/codmap-2015/factored
type = factored

[metabench-codmap-2015-unfactored]
path = /home/danfis/dev/plan-data/raw/codmap-2015/unfactored
type = unfactored
