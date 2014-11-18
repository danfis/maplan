-include ../Makefile.include
-include ../Makefile.local

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

#TARGETS = search
TARGETS = ma-search

all: $(TARGETS)

search: search.c ../libplan.a
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)
ma-search: ma-search.c ../libplan.a
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f $(TARGETS)

.PHONY: all
