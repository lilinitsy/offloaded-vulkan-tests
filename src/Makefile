CFLAGS = -std=c++11 -O3
LDFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi

rendertest: main.cpp
	clang++ $(CFLAGS) -o rendertest main.cpp $(LDFLAGS)

.PHONY: test clean

test: rendertest
	./rendertest

clean:
	rm -f rendertest