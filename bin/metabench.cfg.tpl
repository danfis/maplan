[metabench]
repo = https://github.com/danfis/maplan
commit = master
cplex-includedir = /home/danfis/tmp/cplex/include
cplex-libdir = /home/danfis/tmp/cplex/lib/x86-64_linux/static_pic
#cplex-includedir = /software/cplex-126/cplex/include
#cplex-libdir = /software/cplex-126/cplex/lib/x86-64_linux/static_pic
#search = lazy-ff ma-lazy-ff
search = lazy-ff-factor

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


[metabench-test-seq]
path = /home/danfis/dev/plan-data/raw/test-seq
type = seq

[metabench-test-unfactor]
path = /home/danfis/dev/plan-data/raw/test-unfactor
type = unfactored

[metabench-ipc-2006]
path = /home/danfis/dev/plan-data/raw/ipc-2006
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
