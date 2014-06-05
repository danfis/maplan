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
CFLAGS += $(JANSSON_CFLAGS)
LDFLAGS += -L. -lplan -L../boruvka -lboruvka -lm -lrt
LDFLAGS += $(JANSSON_LDFLAGS)

TARGETS  = libplan.a

OBJS  = plan var state operator dataarr succgen statespace
OBJS += path
OBJS += statespace_fifo
OBJS += statespace_heap
OBJS += search_ehc
OBJS += heuristic/goalcount

BIN_TARGETS =


OBJS_PIC    := $(foreach obj,$(OBJS),.objs/$(obj).pic.o)
OBJS 		:= $(foreach obj,$(OBJS),.objs/$(obj).o)
BIN_TARGETS := $(foreach target,$(BIN_TARGETS),bin/$(target))


ifeq '$(BINS)' 'yes'
  TARGETS += $(BIN_TARGETS)
endif

all: $(TARGETS)

libplan.a: $(OBJS)
	ar cr $@ $(OBJS)
	ranlib $@

plan/config.h: plan/config.h.m4
	$(M4) $(CONFIG_FLAGS) $< >$@

bin/plan-%: bin/%-main.c libplan.a
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)
bin/%.o: bin/%.c bin/%.h
	$(CC) $(CFLAGS) -c -o $@ $<

examples/%: examples/%.c libplan.a
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

.objs/%.pic.o: src/%.c plan/%.h plan/config.h
	$(CC) -fPIC $(CFLAGS) -c -o $@ $<
.objs/%.pic.o: src/%.c plan/config.h
	$(CC) -fPIC $(CFLAGS) -c -o $@ $<

.objs/%.o: src/%.c plan/%.h plan/config.h
	$(CC) $(CFLAGS) -c -o $@ $<
.objs/%.o: src/%.c plan/config.h
	$(CC) $(CFLAGS) -c -o $@ $<


%.h: plan/config.h
%.c: plan/config.h


clean:
	rm -f $(OBJS)
	rm -f .objs/*.o
	rm -f $(TARGETS)
	rm -f $(BIN_TARGETS)
	rm -f plan/config.h
	if [ -d testsuites ]; then $(MAKE) -C testsuites clean; fi;
	if [ -d doc ]; then $(MAKE) -C doc clean; fi;
	
check:
	$(MAKE) -C testsuites check
check-valgrind:
	$(MAKE) -C testsuites check-valgrind

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
	@echo "    JANSSON_CFLAGS    = $(JANSSON_CFLAGS)"
	@echo "    JANSSON_LDFLAGS   = $(JANSSON_LDFLAGS)"
	@echo "    PYTHON_CFLAGS     = $(PYTHON_CFLAGS)"
	@echo "    PYTHON_LDFLAGS    = $(PYTHON_LDFLAGS)"

.PHONY: all clean check check-valgrind help doc install analyze examples
