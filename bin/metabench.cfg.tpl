[metabench]
repo = https://github.com/danfis/maplan
commit = master
cplex-includedir = /home/danfis/tmp/cplex/include
cplex-libdir = /home/danfis/tmp/cplex/lib/x86-64_linux/static_pic
bench = ipc-2006 ipc-2014-sat codmap-2015-factored codmap-2015-unfactored

[metabench-search-1]
validate = true
type = seq
search = lazy
heur = ff
max-time = 1800
max-mem = 8192
repeat = 5
bench = ipc-2006 ipc-2014-sat codmap-2015-factored codmap-2015-unfactored


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
