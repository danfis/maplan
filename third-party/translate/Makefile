include ../../Makefile.include

TARGETS = problemdef_pb2.py

all: $(TARGETS)

%_pb2.py: %.proto
	$(PROTOC) --python_out=. $<

clean:
	rm -f $(TARGETS)

.PHONY: all clean
