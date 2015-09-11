-include Makefile.local
-include Makefile.include

CFLAGS += -I.
CFLAGS += $(BORUVKA_CFLAGS)
CFLAGS += $(NANOMSG_CFLAGS)
CFLAGS += $(LP_SOLVE_CFLAGS)
CFLAGS += $(CPLEX_CFLAGS)
CXXFLAGS += -I.
CXXFLAGS += $(BORUVKA_CFLAGS)
CXXFLAGS += $(PROTOBUF_CFLAGS)
CXXFLAGS += -Wno-long-long

TARGETS  = libplan.a

OBJS  = problem
OBJS += var
OBJS += state
OBJS += part_state
OBJS += state_packer
OBJS += state_pool
OBJS += op
OBJS += op_id_tr
OBJS += succ_gen
OBJS += causal_graph
OBJS += path
OBJS += state_space
OBJS += prio_queue
OBJS += landmark
OBJS += fact_id
OBJS += list_lazy
OBJS += list_lazy_fifo
OBJS += list_lazy_heap
OBJS += list_lazy_bucket
OBJS += list_lazy_rbtree
OBJS += list_lazy_splaytree
OBJS += list
OBJS += list_tiebreaking
OBJS += search
OBJS += search_applicable_ops
OBJS += search_stat
OBJS += search_lazy_base
OBJS += search_ehc
OBJS += search_lazy
OBJS += search_astar
OBJS += heur
OBJS += dtg
OBJS += fact_op_cross_ref
OBJS += pref_op_selector
OBJS += heur_relax
OBJS += heur_goalcount
OBJS += heur_relax_add_max
OBJS += heur_relax_ff
OBJS += heur_lm_cut
OBJS += heur_dtg
OBJS += heur_flow
OBJS += heur_potential
OBJS += heur_ma_ff
OBJS += heur_ma_dtg
OBJS += heur_ma_max
OBJS += heur_ma_lm_cut
OBJS += heur_ma_potential
OBJS += heur_ma_pot
OBJS += msg_schema
OBJS += ma_msg
OBJS += ma_comm_nanomsg
OBJS += ma_search
OBJS += ma_snapshot
OBJS += ma_private_state
OBJS += ma_state
OBJS += lp
OBJS += pot

CXX_OBJS  =
CXX_OBJS += problem
CXX_OBJS += problemdef.pb

OBJS 		:= $(foreach obj,$(OBJS),.objs/$(obj).o)
CXX_OBJS	:= $(foreach obj,$(CXX_OBJS),.objs/$(obj).cxx.o)

all: $(TARGETS)

libplan.a: $(OBJS) $(CXX_OBJS)
	ar cr $@ $(OBJS) $(CXX_OBJS)
	ranlib $@

plan/config.h: plan/config.h.m4
	$(M4) $(CONFIG_FLAGS) $< >$@

src/%.pb.h src/%.pb.cc: src/%.proto
	cd src && $(PROTOC) --cpp_out=. $(notdir $<)

.objs/heur_relax.o: src/heur_relax.c src/heur_relax.h src/_heur_relax_explore.h plan/config.h
	$(CC) $(CFLAGS) -c -o $@ $<
.objs/%.o: src/%.c src/%.h plan/config.h
	$(CC) $(CFLAGS) -c -o $@ $<
.objs/%.o: src/%.c plan/%.h plan/config.h
	$(CC) $(CFLAGS) -c -o $@ $<
.objs/%.o: src/%.c plan/config.h
	$(CC) $(CFLAGS) -c -o $@ $<

.objs/ma_msg.cxx.o: src/ma_msg.cc plan/ma_msg.h src/ma_msg.pb.h plan/config.h
	$(CXX) $(CXXFLAGS) -Wno-pedantic -c -o $@ $<
.objs/ma_msg.pb.cxx.o: src/ma_msg.pb.cc src/ma_msg.pb.h
	$(CXX) $(CXXFLAGS) -Wno-pedantic -c -o $@ $<
.objs/problem.cxx.o: src/problem.cc plan/problem.h src/problemdef.pb.h plan/config.h
	$(CXX) $(CXXFLAGS) -Wno-pedantic -c -o $@ $<
.objs/problemdef.pb.cxx.o: src/problemdef.pb.cc src/problemdef.pb.h
	$(CXX) $(CXXFLAGS) -Wno-pedantic -c -o $@ $<
.objs/%.cxx.o: src/%.cc plan/%.h plan/config.h
	$(CXX) $(CXXFLAGS) -c -o $@ $<
.objs/%.cxx.o: src/%.cc plan/config.h
	$(CXX) $(CXXFLAGS) -c -o $@ $<


%.h: plan/config.h
%.c: plan/config.h


clean:
	rm -f $(OBJS)
	rm -f $(CXX_OBJS)
	rm -f .objs/*.o
	rm -f $(TARGETS)
	rm -f plan/config.h
	rm -f src/*.pb.{cc,h}
	if [ -d bin ]; then $(MAKE) -C bin clean; fi;
	if [ -d testsuites ]; then $(MAKE) -C testsuites clean; fi;
	if [ -d doc ]; then $(MAKE) -C doc clean; fi;
	
check:
	$(MAKE) -C testsuites check
check-valgrind:
	$(MAKE) -C testsuites check-valgrind
check-segfault:
	$(MAKE) -C testsuites check-segfault

doc:
	$(MAKE) -C doc

analyze: clean
	$(SCAN_BUILD) $(MAKE)

submodule:
	$(MAKE) -C third-party submodule
third-party:
	$(MAKE) -C third-party
third-party-clean:
	$(MAKE) -C third-party clean

help:
	@echo "Targets:"
	@echo "    all            - Build library"
	@echo "    doc            - Build documentation"
	@echo "    check          - Build & Run automated tests"
	@echo "    check-valgrind - Build & Run automated tests in valgrind(1)"
	@echo "    check-segfault - Build & Run automated tests in valgrind(1) set up to detect only segfaults"
	@echo "    clean          - Remove all generated files"
	@echo "    analyze        - Perform static analysis using Clang Static Analyzer"
	@echo "    submodule      - Fetch all submodules using git."
	@echo "    third-party    - Build all third-party projects."
	@echo "    third-party-clean - Clean all third-party projects."
	@echo ""
	@echo "Options:"
	@echo "    CC         - Path to C compiler          (=$(CC))"
	@echo "    CXX        - Path to C++ compiler        (=$(CXX))"
	@echo "    M4         - Path to m4 macro processor  (=$(M4))"
	@echo "    SCAN_BUILD - Path to scan-build          (=$(SCAN_BUILD))"
	@echo "    PROTOC     - Path to protobuf compiler   (=$(PROTOC))"
	@echo "    PYTHON2    - Path to python v2 interpret (=$(PYTHON2))"
	@echo ""
	@echo "    DEBUG      'yes'/'no' - Turn on/off debugging   (=$(DEBUG))"
	@echo "    PROFIL     'yes'/'no' - Compiles profiling info (=$(PROFIL))"
	@echo ""
	@echo "    USE_NANOMSG 'yes'/'no'  - Use libnanomsg library (=$(USE_NANOMSG))"
	@echo ""
	@echo "Variables:"
	@echo "  Note that most of can be preset or changed by user"
	@echo "    SYSTEM            = $(SYSTEM)"
	@echo "    CFLAGS            = $(CFLAGS)"
	@echo "    CXXFLAGS          = $(CXXFLAGS)"
	@echo "    LDFLAGS           = $(LDFLAGS)"
	@echo "    CONFIG_FLAGS      = $(CONFIG_FLAGS)"
	@echo "    USE_LOCAL_OPTS    = $(USE_LOCAL_OPTS)"
	@echo "    OPTS_CFLAGS       = $(OPTS_CFLAGS)"
	@echo "    OPTS_LDFLAGS      = $(OPTS_LDFLAGS)"
	@echo "    USE_LOCAL_BORUVKA = $(USE_LOCAL_BORUVKA)"
	@echo "    BORUVKA_CFLAGS    = $(BORUVKA_CFLAGS)"
	@echo "    BORUVKA_LDFLAGS   = $(BORUVKA_LDFLAGS)"
	@echo "    USE_LOCAL_PROTOBUF = $(USE_LOCAL_PROTOBUF)"
	@echo "    PROTOBUF_CFLAGS   = $(PROTOBUF_CFLAGS)"
	@echo "    PROTOBUF_LDFLAGS  = $(PROTOBUF_LDFLAGS)"
	@echo "    USE_LOCAL_NANOMSG = $(USE_LOCAL_NANOMSG)"
	@echo "    NANOMSG_CFLAGS    = $(NANOMSG_CFLAGS)"
	@echo "    NANOMSG_LDFLAGS   = $(NANOMSG_LDFLAGS)"
	@echo ""
	@echo "    LP                 = $(LP)"
	@echo "    LP_CFLAGS          = $(LP_CFLAGS)"
	@echo "    LP_LDFLAGS         = $(LP_LDFLAGS)"
	@echo "    USE_CPLEX          = $(USE_CPLEX)"
	@echo "    USE_LP_SOLVE       = $(USE_LP_SOLVE)"

.PHONY: all clean check check-valgrind help doc install analyze examples submodule third-party
