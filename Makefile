# Check if 'make' is run from the 'dht' directory (the directory where this Makefile resides).
$(if $(findstring /,$(MAKEFILE_LIST)),$(error Please only invoke this Makefile from the directory it resides in))

# Run all shell commands with bash.
SHELL := bash

CFLAGS = -g -Wall -DDEBUG_INFO -DALL_WELCOME
#LDLIBS = -lcrypt

dht-example: dht-example.o dht.o

RECIPES = all clean run

.PHONY: $(RECIPES)

all: dht-example

clean:
	-rm -f dht-example dht-example.o dht-example.id dht.o *~ core

SMALL_CLOUD = node0 node1 node2 node3
run:
	@echo "Run a local cloud with nodes $(SMALL_CLOUD)"
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
	 popd 
