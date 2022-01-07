CXX = clang++
CXXFLAGS = -g -O3 -mavx2 -std=c++11
LDFLAGS = -g -pthread

# obj loader
CXXFLAGS += -I../obj-loader/include
LDFLAGS += -L../obj-loader/target/release -lobj_loader -Wl,-rpath=$(shell realpath ../obj-loader/target/release)

# External dependencies
CXXFLAGS += $(shell pkg-config --cflags glfw3 vulkan)
LDFLAGS += $(shell pkg-config --libs glfw3 vulkan)

all: rendertest client

rendertest: main.o
	$(CXX) $(LDFLAGS) -o $(@) $(^)

.PHONY: test clean

client: client.o
	$(CXX) $(LDFLAGS) -o $(@) $(^)

test: rendertest
	./rendertest

clean:
	rm -f *.o rendertest client

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $(@) $(<)
