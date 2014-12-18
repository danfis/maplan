-include ../Makefile.local
-include ../Makefile.include

CFLAGS += -I../
CFLAGS += -I../../boruvka
CFLAGS += -I../../opts
CFLAGS += $(PROTOBUF_CFLAGS)
CFLAGS += $(ZMQ_CFLAGS)
LDFLAGS += -L../ -lplan
LDFLAGS += -L../../boruvka -lboruvka
LDFLAGS += -L../../opts -lopts
LDFLAGS += $(PROTOBUF_LDFLAGS)
LDFLAGS += $(ZMQ_LDFLAGS)
LDFLAGS += -pthread -lrt -lm
LDFLAGS += -lstdc++

ifeq '$(BIN_STATIC)' 'yes'
  LDFLAGS += -static
endif

OBJS  = search.o
OBJS += options.o

TARGETS = search

all: $(TARGETS)

search: ../libplan.a $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)
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
