server: server.c
	gcc -pthread server.c -o server

client: node.c
	gcc -pthread node.c -o node

.PHONY: all run_server run_client clean

all: server client

run_server: server
	./server

run_node: node
	./node

clean:
	rm -f server client