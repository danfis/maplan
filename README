# MAPlan

## Download
The repository of MAPlan is currently hosted on gitlab:
  $ git clone https://gitlab.com/danfis/maplan.git

## Compilation
MAPlan has some dependencies on other libraries and compilation may require
some configuration.
Use

  $ make help

and you should see how all Makefile variables are set in the current
configuration. The Makefiles are organized so that Makefile.include
contains detection of dependencies and it sets up all variables and
Makefile (that includes Makefile.include) contains all the rules.
Additionally you can create Makefile.local (which is not tracked by git)
that is always included before Makefile.include so you can override
(almost) all settings that are done in Makefile.include by setting the
right variables in Makefile.local.

### Configuration
Besides overriding configurations of dependencies, Makefile.local must be used
to configure dependency on LP solver, if you wish to use modules depending on
it (like potential or flow heuristics). The planner will, however, compile
without them and if you try to use them, it will write an error message and
exit.

To set up depndency on an LP solver, you need to set corresponding
variables in Makefile.local. MAPlan can currently use lpsolve (open-source)
or CPLEX solvers and we would recommend CPLEX.

So if you install CPLEX you need to set up CPLEX_CFLAGS to use CPLEX's
include directory (-I/path/to/include/dir) and CPLEX_LDFLAGS to link to
CPLEX's library (-L/path/to/lib/dir -lcplex). Look into Makefile.local.tpl for
the example Makefile.local file.

### Dependecies
MAPlan depends on some other libraries, but all of them are listed in
third-party/ directory (either directly or as git's submodule) so you
should not need to install any. But one of the dependencies is google's
protobuffers that is almost impossible to use if you have already
system-wide installation.

So if you *don't* have installed protobuffers on your computer you should
be able to compile all dependencies by:
  $ cd third-party
  $ make boruvka opts protobuf nanomsg translate

If you have protobuffers already installed on the computer you have to skip
compiling protobuffers from third-party/ directory:
  $ cd third-party
  $ make boruvka opts nanomsg translate

Now 'make help' should show all *_CFLAGS and *_LDFLAGS variables properly
set up.

### Compilation of MAPlan
Once you have compiled and set up all dependencies, you can compile MAPlan
by using make from the top directory:
  $ make
  $ make -C bin

Now in bin/ directory should be 'search' binary that when called without
parameters (or with -h or --help) should print list of parameters
that can be used. Currently, the planner needs pre-processed problems in
protobuf format that is produced by modified Fast Downward's translate
program located in third-party/translate/ directory.


## Running MAPlan
MAPlan can run in several modes, as a sequential planner and as a multi-agent
planner running either in threads using a shared memory for communication, or
in separate processes using TPC/IP communication channels. Currently, for all
of these cases the Fast-Downward's translator located in third-party/translate
is needed to preprocess PDDL files.

In all cases, running the planner (bin/search) with -h option shows all
options available. The most notable are:

 * -p defines path to the preprocessed input problem
 * -s defines search algorithm
 * -H defines heuristic function

### Sequential Planner
First, preprocess the input PDDL files using third-party/translate/translate.py
with --proto option:

  $ ./third-party/translate/translate.py --proto --output problem.proto path/to/domain.pddl path/to/problem.pddl

This generates the input problem in protobuf format and writes it into
problem.proto file.

Now you can run the planner, for example with lm-cut heuristic and A* search
algorithm:
  $ ./bin/search -p problem.proto -H lm-cut -s astar -o plan.out

### Multi-agent Planner in Threads
Preprocessing factored MA-PDDL files can be done with
third-party/translate/translate-factored.sh script. Run this script from the
directory with factored MA-PDDL files corresponding to the problem you want to
solve, for example using the CoDMAP depot domain:
  $ cd path/to/factored/depot/pfile1
  $ /path/to/third-party/translate/translate-factored.sh

This generates a set of .proto files in the same directory, each corresponding
to one agent.

Now, you can run planner with --ma-factor-dir option, for example:
  $ ./bin/search --ma-factor-dir -p path/to/factored/depot/pfile1 -H lm-cut -s astar

### Multi-agent Planner in Processes
The preprocessing is the same as in the previous case (use
third-party/translate/translate-factored.sh).

The option --ma-factor switches to multi-agent solver using TCP/IP
communication channels.

The configuration of TCP/IP channels is done using --tcp and --tcp-id options.
Each agent runs in its own process, so you'll need to run as many ./bin/search
planners as you have agents in your problem. The --tcp option defines
ip-address:port TCP address, so you need to define as many --tcp options as
you have agents and each planner needs to have defined the same --tcp options
in the same order. Lastly, --tcp-id defines ID of the current agent, counting
from 0 to the number of agents minus 1.

So for example to run the multi-agent multi-process planner on
driverlog/pfile1 problem from CoDMAP, you need to
1. Run third-party/translate/translate-factored.sh in the directory with the
problem, which will generate two files (for each agent) driver1.proto and
driver2.proto.
2. Then you need to choose some available ports and determine the IP address.
Let's say we will run both agents on the same machine with ports 10000 and
10001.
3. Run one planner per agent:
  $ ./bin/search --ma-factor --tcp 127.0.0.1:10000 --tcp 127.0.0.1:10001 --tcp-id 0 -p path/to/driver1.proto -o plan0.out -H lm-cut -s astar &
  $ ./bin/search --ma-factor --tcp 127.0.0.1:10000 --tcp 127.0.0.1:10001 --tcp-id 1 -p path/to/driver2.proto -o plan1.out -H lm-cut -s astar &
