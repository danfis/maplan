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
DEBUG = yes

-include Makefile.local
-include Makefile.include

SO_VERSION = 0

CFLAGS += -I. -I../boruvka/
CXXFLAGS += -I. -I../boruvka/
CXXFLAGS += $(PROTOBUF_CFLAGS)
LDFLAGS += -L. -lplan -L../boruvka -lboruvka -lm -lrt
LDFLAGS += $(PROTOBUF_LDFLAGS)

TARGETS  = libplan.a

OBJS  = problem
OBJS += var
OBJS += state
OBJS += operator
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
OBJS += search_ehc
OBJS += search_lazy
OBJS += heur
OBJS += heur_goalcount
OBJS += heur_relax
OBJS += heur_lm_cut
OBJS += prioqueue
OBJS += ma_comm_queue
OBJS += ma_agent
OBJS += ma

CXX_OBJS  = ma_msg.pb
CXX_OBJS += ma_msg

BIN_TARGETS =


OBJS_PIC    := $(foreach obj,$(OBJS),.objs/$(obj).pic.o)
OBJS 		:= $(foreach obj,$(OBJS),.objs/$(obj).o)
CXX_OBJS	:= $(foreach obj,$(CXX_OBJS),.objs/$(obj).cxx.o)
BIN_TARGETS := $(foreach target,$(BIN_TARGETS),bin/$(target))


ifeq '$(BINS)' 'yes'
  TARGETS += $(BIN_TARGETS)
endif

all: $(TARGETS)

libplan.a: $(OBJS) $(CXX_OBJS)
	ar cr $@ $(OBJS) $(CXX_OBJS)
	ranlib $@

plan/config.h: plan/config.h.m4
	$(M4) $(CONFIG_FLAGS) $< >$@

bin/plan-%: bin/%-main.c libplan.a
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)
bin/%.o: bin/%.c bin/%.h
	$(CC) $(CFLAGS) -c -o $@ $<

examples/%: examples/%.c libplan.a
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

src/ma_msg.pb.cc: src/ma_msg.proto
	cd src && $(PROTOC) --cpp_out=. ma_msg.proto

.objs/%.pic.o: src/%.c plan/%.h plan/config.h
	$(CC) -fPIC $(CFLAGS) -c -o $@ $<
.objs/%.pic.o: src/%.c plan/config.h
	$(CC) -fPIC $(CFLAGS) -c -o $@ $<

.objs/%.o: src/%.c plan/%.h plan/config.h
	$(CC) $(CFLAGS) -c -o $@ $<
.objs/%.o: src/%.c plan/config.h
	$(CC) $(CFLAGS) -c -o $@ $<

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
	rm -f $(BIN_TARGETS)
	rm -f plan/config.h
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
	@echo "    clean          - Remove all generated files"
	@echo "    install        - Install library into system"
	@echo "    analyze        - Performs static analysis using Clang Static Analyzer"
	@echo ""
	@echo "Options:"
	@echo "    CC         - Path to C compiler          (=$(CC))"
	@echo "    CXX        - Path to C++ compiler        (=$(CXX))"
	@echo "    M4         - Path to m4 macro processor  (=$(M4))"
	@echo "    SED        - Path to sed(1)              (=$(SED))"
	@echo "    PYTHON     - Path to python interpret    (=$(PYTHON))"
	@echo "    PYTHON2    - Path to python interpret v2 (=$(PYTHON2))"
	@echo "    PYTHON3    - Path to python interpret v3 (=$(PYTHON3))"
	@echo "    SCAN_BUILD - Path to scan-build          (=$(SCAN_BUILD))"
	@echo ""
	@echo "    DYNAMIC   'yes'/'no' - Set to 'yes' if also .so library should be built (=$(DYNAMIC))"
	@echo ""
	@echo "    BINS      'yes'/'no' - Set to 'yes' if binaries should be built (=$(BINS))"
	@echo ""
	@echo "    CC_NOT_GCC 'yes'/'no' - If set to 'yes' no gcc specific options will be used (=$(CC_NOT_GCC))"
	@echo ""
	@echo "    DEBUG      'yes'/'no' - Turn on/off debugging          (=$(DEBUG))"
	@echo "    PROFIL     'yes'/'no' - Compiles profiling info        (=$(PROFIL))"
	@echo "    NOWALL     'yes'/'no' - Turns off -Wall gcc option     (=$(NOWALL))"
	@echo "    NOPEDANTIC 'yes'/'no' - Turns off -pedantic gcc option (=$(NOPEDANTIC))"
	@echo ""
	@echo "    USE_MEMCHECK 'yes'/'no'  - Use memory checking during allocation (=$(USE_MEMCHECK))"
	@echo ""
	@echo "    PREFIX     - Prefix where library will be installed                             (=$(PREFIX))"
	@echo "    INCLUDEDIR - Directory where header files will be installed (PREFIX/INCLUDEDIR) (=$(INCLUDEDIR))"
	@echo "    LIBDIR     - Directory where library will be installed (PREFIX/LIBDIR)          (=$(LIBDIR))"
	@echo "    BINDIR     - Directory where binaries will be installed (PREFIX/BINDIR)         (=$(BINDIR))"
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
	@echo "    PYTHON_CFLAGS     = $(PYTHON_CFLAGS)"
	@echo "    PYTHON_LDFLAGS    = $(PYTHON_LDFLAGS)"

.PHONY: all clean check check-valgrind help doc install analyze examples
