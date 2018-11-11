# Check if 'make' is run from the 'dht' directory (the directory where this Makefile resides).
$(if $(findstring /,$(MAKEFILE_LIST)),$(error Please only invoke this Makefile from the directory it resides in))

# Run all shell commands with bash.
SHELL := bash

CFLAGS = -g -Wall -DDEBUG_INFO -DALL_WELCOME
#LDLIBS = -lcrypt

dht-example: dht-example.o dht.o

RECIPES = all clean run run_locally

.PHONY: $(RECIPES)

all: dht-example

clean:
	-rm -f dht-example dht-example.o dht-example.id dht.o *~ core

run: run-remote-known_node run-remote-node1 run-local-node
	@echo "Goals successful: $^"; rm $^

run-remote-known_node:
	@ssh $(KNOWN_NODE)
	@echo $@ > $@

run-remote-node1:
	@echo $@ > $@

run-local-node:
	@echo $@ > $@

SMALL_CLOUD = node0 node1 node2 node3
run_locally:
	@echo "Run a local cloud with nodes $(SMALL_CLOUD)"
	@[ -d ./nodes ] || mkdir nodes
	@pushd nodes; \
	 for node in $(SMALL_CLOUD); do \
		if [ $$node = 'node0' ]; then \
		 let "PORT = 3000" "PORT0 = PORT"; \
		 echo "node0, port $$PORT, is the only boostrap node"; \
		 ../dht-example -4 -i $$node.id -b 127.0.0.1 $$PORT > $$node.out & \
	  else \
		 let "++PORT"; \
		 echo "$$node, port $$PORT, joins the cloud"; \
		 ../dht-example -4 -i $$node.id $$PORT 127.0.0.1 $$PORT0 > $$node.out & \
		fi; \
	 done; \
	 popd; \
	 ps -ef | grep dht-example 
