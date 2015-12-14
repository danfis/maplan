-include ../Makefile.local
-include ../Makefile.include

CFLAGS += -I../
CFLAGS += $(BORUVKA_CFLAGS)
CFLAGS += $(OPTS_CFLAGS)
CFLAGS += $(PROTOBUF_CFLAGS)
CFLAGS += $(LP_CFLAGS)
LDFLAGS += -L../ -lplan
LDFLAGS += $(BORUVKA_LDFLAGS)
LDFLAGS += $(OPTS_LDFLAGS)
LDFLAGS += $(PROTOBUF_LDFLAGS)
LDFLAGS += -pthread -lrt -lm
LDFLAGS += -lstdc++
LDFLAGS += $(LP_LDFLAGS)

ifeq '$(BIN_STATIC)' 'yes'
  LDFLAGS += -static
endif

OBJS  = search.o
OBJS += options.o

TARGETS = search invariants

all: $(TARGETS)

search: ../libplan.a $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)
invariants: ../libplan.a invariants.c
	$(CC) $(CFLAGS) -o $@ invariants.c $(LDFLAGS)
ma-search: ma-search.c ../libplan.a
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

%.o: %.c %.h
	$(CC) $(CFLAGS) -c -o $@ $<
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(TARGETS)
	rm -f *.o

.PHONY: all
