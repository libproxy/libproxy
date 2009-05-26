

all: curlget

curlget: curlget.c
	gcc curlget.c -o curlget -Wall -lcurl -std=c99 $(shell pkg-config --libs libproxy-1.0)

clean:
	rm curlget
