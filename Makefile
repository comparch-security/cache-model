
MAKE = make
CXX = g++
CXXFLAGS = --std=c++11 -O2 -g -I. -fPIC

TARGETS = \
	test/test-creation \
	test/test-eviction \

OBJECTS = \
	cache/cache.o \
	attack/search.o \
	attack/create.o \
	attack/algorithm.o \
	util/query.o \

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
