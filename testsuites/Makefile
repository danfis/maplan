
ifneq '$(DEBUG)' 'no'
  DEBUG := yes
endif

-include ../Makefile.include
-include ../Makefile.local

CFLAGS += -I./ -I../
CFLAGS += -I../../boruvka
CFLAGS += $(JANSSON_CFLAGS)
LDFLAGS += -Lcu/ -lcu -lrt -lm -L../ -lplan -L../../boruvka -lboruvka -pthread
LDFLAGS += $(JANSSON_LDFLAGS)

BENCH_CFLAGS = -O3 -pipe -march=core2
BENCH_CFLAGS += -I./ -I../
BENCH_CFLAGS += -I../../boruvka
BENCH_CFLAGS += $(JANSSON_CFLAGS)

CHECK_REG=cu/check-regressions
CHECK_TS ?=

TARGETS = test bench-ehc

OBJS = main.o load-from-file.o state.o dataarr.o succgen.o statespace.o \
       search_ehc.o

all: $(TARGETS)

test: cu $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)
bench-ehc: bench-ehc.c
	$(CC) $(BENCH_CFLAGS) -o $@ $< $(LDFLAGS)

%.o: %.c %.h
	$(CC) $(CFLAGS) -c -o $@ $<
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

check: all
	@echo ""
	@echo "----------------------------------------";
	./test $(CHECK_TS)
	@echo "----------------------------------------";
	@echo "Checking regressions:";
	$(PYTHON2) $(CHECK_REG) regressions
	@echo ""

check-valgrind: all
	valgrind -q --leak-check=full --show-reachable=yes --trace-children=yes \
             --error-limit=no \
             ./test $(CHECK_TS)

check-valgrind-gen-suppressions: all
	valgrind -q --leak-check=full --show-reachable=yes --trace-children=yes \
             --gen-suppressions=all --log-file=out --error-limit=no \
             ./test $(CHECK_TS)

cu:
	$(MAKE) ENABLE_TIMER=yes -C cu/

clean:
	rm -f *.o
	rm -f objs/*.o
	rm -f $(TARGETS)
	rm -f tmp.*
	rm -f regressions/tmp.*
	rm -f $(BENCH_HEAP)

.PHONY: all clean check check-valgrind cu bench-heap
	
