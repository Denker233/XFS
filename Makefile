server: server.c
	gcc -pthread -o server server.c

client: node.c
	gcc -pthread -o node node.c

.PHONY: all run_server run_client clean

all: server client

run_server: server
	./server

run_node: node
	./node

clean:
	rm -f server client