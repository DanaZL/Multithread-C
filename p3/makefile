all: proxy
proxy: proxy.cpp sockets_create.c buffer.c
	g++ -o proxy proxy.cpp sockets_create.c buffer.c -g -std=c++11 -lev -Wall -Werror
clean:
	rm -rf proxy *.tmp
