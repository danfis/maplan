
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
LDFLAGS += $(PROTOBUF_LDFLAGS)
LDFLAGS += -lstdc++

BENCH_CFLAGS = -O3 -pipe -march=corei7
#BENCH_CFLAGS += -g
BENCH_CFLAGS += -I./ -I../
BENCH_CFLAGS += -I../../boruvka
BENCH_CFLAGS += -I../../opts
BENCH_CFLAGS += $(JANSSON_CFLAGS)

CHECK_REG=cu/check-regressions
CHECK_TS ?=

TARGETS = test search

OBJS  = load-from-file.o
OBJS += state.o
OBJS += dataarr.o
OBJS += succgen.o
OBJS += statespace.o
OBJS += search_ehc.o
OBJS += search_lazy.o
OBJS += heur.o
OBJS += list_lazy.o
OBJS += t_ma_comm_queue.o

all: $(TARGETS)

test: main.c cu $(OBJS)
	$(CC) $(CFLAGS) -o $@ $< $(OBJS) $(LDFLAGS)
search: search.c ../libplan.a
	$(CC) $(BENCH_CFLAGS) -o $@ $< $(LDFLAGS) -L../../opts -lopts

t_ma_comm_queue.o: t_ma_comm_queue.c
	$(CC) $(CFLAGS) -I../src -c -o $@ $<
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
             --error-limit=no --suppressions=valgrind-suppress.supp \
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

.PHONY: all clean check check-valgrind cu
	
