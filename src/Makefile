CXX = clang++
CXXFLAGS = -std=c++11 -O3 -mavx2 -fopenmp -g
LDFLAGS = -lglfw -lvulkan -ldl -mavx2 -fopenmp -lpthread -lX11 -lXxf86vm -lXrandr -lXi -g

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