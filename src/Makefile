CXX = clang++
CFLAGS = -std=c++11 -O3
LDFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi

rendertest: main.o
	$(CXX) $(LDFLAGS) -o $(@) $(^)

.PHONY: test clean

test: rendertest
	./rendertest

clean:
	rm -f *.o rendertest

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $(@) $(<)