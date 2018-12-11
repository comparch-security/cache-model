
MAKE = make
CXX = g++
CXXFLAGS = --std=c++11 -g -O2 -I.

TARGETS = test/test-creation
OBJECTS = cache/cache.o attack/search.o attack/create.o attack/algorithm.o util/report.o util/print.o
HEADERS = $(wildcard cache/*.hpp) $(wildcard attack/*.hpp) $(wildcard util/*.hpp)

all: $(TARGETS)

datagen/librandomgen.a:
	$(MAKE) -C datagen $(filename $@)

$(OBJECTS): %.o:%.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(TARGETS): test/% : test/%.cpp $(OBJECTS) datagen/librandomgen.a
	$(CXX) $(CXXFLAGS) $^ -o $@

clean:
	-rm $(OBJECTS) $(TARGETS)
