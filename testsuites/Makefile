
ifneq '$(DEBUG)' 'no'
  DEBUG := yes
endif

-include ../Makefile.include
-include ../Makefile.local

CFLAGS += -I./ -I../
CFLAGS += $(BORUVKA_CFLAGS)
CFLAGS += $(PROTOBUF_CFLAGS)
CFLAGS += $(NANOMSG_CFLAGS)
CFLAGS += $(LP_SOLVE_CFLAGS)
CFLAGS += $(CPLEX_CFLAGS)
LDFLAGS += -Lcu/ -lcu -lrt -lm -L../ -lplan -pthread
LDFLAGS += $(BORUVKA_LDFLAGS)
LDFLAGS += $(PROTOBUF_LDFLAGS)
LDFLAGS += $(NANOMSG_LDFLAGS)
LDFLAGS += -lstdc++
LDFLAGS += $(LP_SOLVE_LDFLAGS)
LDFLAGS += $(CPLEX_LDFLAGS)

CHECK_REG=cu/check-regressions
CHECK_TS ?=

TARGETS = test optimal-cost msg-schema-gen msg-schema-load

OBJS  = load-from-file.o
OBJS += state.o
OBJS += succgen.o
OBJS += statespace.o
#OBJS += search_ehc.o
#OBJS += search_lazy.o
OBJS += search_astar.o
OBJS += heur.o
OBJS += heur_flow.o
OBJS += heur_ma.o
OBJS += list_lazy.o
OBJS += list.o
OBJS += ma_comm_nanomsg.o
OBJS += causalgraph.o
OBJS += state_pool.o
OBJS += ma_search.o
OBJS += heur_admissible.o
OBJS += dtg.o
OBJS += ma_private_state.o
OBJS += landmark.o
OBJS += msg_schema.o

all: $(TARGETS)

test: main.c cu $(OBJS)
	$(CC) $(CFLAGS) -o $@ $< $(OBJS) $(LDFLAGS)
search: search.c ../libplan.a
	$(CC) $(BENCH_CFLAGS) -o $@ $< $(LDFLAGS) -L../../opts -lopts
optimal-cost: optimal-cost.c state_pool.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
msg-schema-gen: msg-schema-gen.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
msg-schema-load: msg-schema-load.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

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

check-segfault: all
	valgrind -q --trace-children=yes \
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
	rm -f regressions/temp.*
	rm -f $(BENCH_HEAP)

.PHONY: all clean check check-valgrind cu
