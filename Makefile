# Check if 'make' is run from the 'dht' directory (the directory where this Makefile resides).
$(if $(findstring /,$(MAKEFILE_LIST)),$(error Please only invoke this Makefile from the directory it resides in))

# Run all shell commands with bash.
SHELL := bash

CFLAGS = -g -Wall -DDEBUG_INFO -DALL_WELCOME
UNAME_S := $(shell uname -s)
ifneq ($(UNAME_S),Darwin)
LDLIBS = -lcrypt
endif

dht-example: dht-example.o dht.o

hub: hub.o dht.o

RECIPES = all clean run run_locally

.PHONY: $(RECIPES)

all: dht-example

clean:
	-rm -f dht-example dht-example.o dht-example.id dht.o *~ core

run: run-lan-hub run-lan-leaf1 run-lan-leaf2 run-lan-local-leaf
	@echo "Goals successful: $^"; rm $^

LAN_HUB = 10.0.0.10
run-lan-hub:
	@ssh $(LAN_HUB) "cd product/dht; make run-lan-local-hub"
	@echo $@ > $@

run-lan-local-hub: hub
	@PORT=3000; ./hub $$PORT &
	@echo "$@ started on $(LAN_HUB)"

run-lan-leaf1:
	@echo $@ > $@

run-lan-leaf2:
	@echo $@ > $@

run-lan-local-leaf:
	@echo $@ > $@

SMALL_CLOUD = node0 node1 node2 node3
run_locally:
	@echo "Run a local cloud with nodes $(SMALL_CLOUD)"
	@[ -x ./dht-example ] || make
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
