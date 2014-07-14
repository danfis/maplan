-include ../Makefile.include
-include ../Makefile.local

CFLAGS += -I../
CFLAGS += -I../../boruvka
CFLAGS += -I../../opts
CFLAGS += $(PROTOBUF_CFLAGS)
LDFLAGS += -L../ -lplan
LDFLAGS += -L../../boruvka -lboruvka
LDFLAGS += -L../../opts -lopts
LDFLAGS += $(PROTOBUF_LDFLAGS)
LDFLAGS += -pthread -lrt -lm
LDFLAGS += -lstdc++

TARGETS = search

all: $(TARGETS)

search: search.c ../libplan.a
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

.PHONY: all
