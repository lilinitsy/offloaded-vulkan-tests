CXX = clang++
CXXFLAGS = -std=c++11 -Og -fsanitize=address -g
LDFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi -fsanitize=address -g

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