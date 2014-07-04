-include ../../Makefile.include

CXX_OBJS  = axiom
CXX_OBJS += causal_graph
CXX_OBJS += domain_transition_graph
CXX_OBJS += helper_functions
CXX_OBJS += max_dag
CXX_OBJS += mutex_group
CXX_OBJS += operator
CXX_OBJS += scc
CXX_OBJS += state
CXX_OBJS += successor_generator
CXX_OBJS += variable
CXX_OBJS += planner

CXX_OBJS := $(foreach obj,$(CXX_OBJS),.objs/$(obj).cxx.o)

TARGETS = preprocess

all: $(TARGETS)

preprocess: $(CXX_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

.objs/%.cxx.o: %.cc %.h
	$(CXX) $(CXXFLAGS) -c -o $@ $<
.objs/%.cxx.o: %.cc
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS)
	rm -f $(CXX_OBJS)
	rm -f .objs/*.o
	rm -f $(TARGETS)

.PHONY: all clean help
