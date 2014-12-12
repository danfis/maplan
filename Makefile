###
# libplan
# --------
# Copyright (c)2014 Daniel Fiser <danfis@danfis.cz>
#
#  This file is part of boruvka.
#
#  Distributed under the OSI-approved BSD License (the "License");
#  see accompanying file BDS-LICENSE for details or see
#  <http://www.opensource.org/licenses/bsd-license.php>.
#
#  This software is distributed WITHOUT ANY WARRANTY; without even the
#  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
#  See the License for more information.
##
-include Makefile.local
-include Makefile.include

CFLAGS += -I. -I../boruvka/
CFLAGS += $(ZMQ_CFLAGS)
CXXFLAGS += -I. -I../boruvka/
CXXFLAGS += $(PROTOBUF_CFLAGS)
CXXFLAGS += -Wno-long-long
LDFLAGS += -L. -lplan -L../boruvka -lboruvka -lm -lrt
LDFLAGS += $(PROTOBUF_LDFLAGS)
LDFLAGS += $(ZMQ_LDFLAGS)

TARGETS  = libplan.a

OBJS  = problem
OBJS += problem-fd
OBJS += var
OBJS += state
OBJS += op
OBJS += dataarr
OBJS += succgen
OBJS += path
OBJS += statespace
OBJS += list_lazy
OBJS += list_lazy_fifo
OBJS += list_lazy_heap
OBJS += list_lazy_bucket
OBJS += list_lazy_rbtree
OBJS += list_lazy_splaytree
OBJS += list
OBJS += list_tiebreaking
OBJS += search
#OBJS += search_ehc
#OBJS += search_lazy
OBJS += search_astar
OBJS += heur
OBJS += heur_goalcount
OBJS += heur_relax_add_max
OBJS += heur_relax_ff
OBJS += heur_ma_ff
#OBJS += heur_ma_max
OBJS += heur_lm_cut
OBJS += prioqueue
OBJS += ma_comm
OBJS += ma_comm_queue
ifeq '$(USE_ZMQ)' 'yes'
  OBJS += ma_comm_net
endif
#OBJS += ma
OBJS += causalgraph
OBJS += _problem
OBJS += fact_id
OBJS += ma_search
OBJS += search_applicable_ops
OBJS += search_stat
OBJS += ma_snapshot
OBJS += fact_op_cross_ref
OBJS += heur_relax
OBJS += pref_op_selector

CXX_OBJS  = ma_msg.pb
CXX_OBJS += ma_msg
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

.objs/heur_relax_%.o: src/heur_relax_%.c src/_heur_fact_op.c src/_heur_relax.c \
		src/_heur_relax_impl.c plan/heur.h plan/config.h
	$(CC) $(CFLAGS) -c -o $@ $<
.objs/heur_ma_ff.o: src/heur_ma_ff.c src/_heur_fact_op.c src/_heur_relax.c plan/heur.h plan/config.h
	$(CC) $(CFLAGS) -c -o $@ $<
.objs/heur_ma_max.o: src/heur_ma_max.c src/_heur_fact_op.c src/_heur_relax.c plan/heur.h plan/config.h
	$(CC) $(CFLAGS) -c -o $@ $<
.objs/%.o: src/%.c src/%.h plan/config.h
	$(CC) $(CFLAGS) -c -o $@ $<
.objs/%.o: src/%.c plan/%.h plan/config.h
	$(CC) $(CFLAGS) -c -o $@ $<
.objs/%.o: src/%.c plan/config.h
	$(CC) $(CFLAGS) -c -o $@ $<

.objs/ma_msg.cxx.o: src/ma_msg.cc plan/ma_msg.h src/ma_msg.pb.h plan/config.h
	$(CXX) $(CXXFLAGS) -c -o $@ $<
.objs/problem.cxx.o: src/problem.cc plan/problem.h src/problemdef.pb.h plan/config.h
	$(CXX) $(CXXFLAGS) -c -o $@ $<
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

help:
	@echo "Targets:"
	@echo "    all            - Build library"
	@echo "    doc            - Build documentation"
	@echo "    check          - Build & Run automated tests"
	@echo "    check-valgrind - Build & Run automated tests in valgrind(1)"
	@echo "    check-segfault - Build & Run automated tests in valgrind(1) set up to detect only segfaults"
	@echo "    clean          - Remove all generated files"
	@echo "    analyze        - Performs static analysis using Clang Static Analyzer"
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
	@echo "    USE_ZMQ  'yes'/'no'  - Use libzmq library (=$(USE_ZMQ))"
	@echo ""
	@echo "Variables:"
	@echo "  Note that most of can be preset or changed by user"
	@echo "    SYSTEM            = $(SYSTEM)"
	@echo "    CFLAGS            = $(CFLAGS)"
	@echo "    CXXFLAGS          = $(CXXFLAGS)"
	@echo "    LDFLAGS           = $(LDFLAGS)"
	@echo "    CONFIG_FLAGS      = $(CONFIG_FLAGS)"
	@echo "    PROTOBUF_CFLAGS   = $(PROTOBUF_CFLAGS)"
	@echo "    PROTOBUF_LDFLAGS  = $(PROTOBUF_LDFLAGS)"
	@echo "    ZMQ_CFLAGS        = $(ZMQ_CFLAGS)"
	@echo "    ZMQ_LDFLAGS       = $(ZMQ_LDFLAGS)"

.PHONY: all clean check check-valgrind help doc install analyze examples
