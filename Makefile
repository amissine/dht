# Check if 'make' is run from the 'dht' directory (the directory where this Makefile resides).
$(if $(findstring /,$(MAKEFILE_LIST)),$(error Please only invoke this Makefile from the directory it resides in))

# Run all shell commands with bash.
SHELL := bash

CFLAGS = -g -Wall -DDEBUG_INFO -DALL_WELCOME
LFLAGS = -L /opt/local/lib
LDLIBS = -lstdc++ -lboost_system-mt
UNAME_S := $(shell uname -s)
ifneq ($(UNAME_S),Darwin)
LDLIBS = $(LDLIBS) -lcrypt
endif

dht-example: dht-example.o dht.o ../bdecode/bdec.a

hub: hub.o dht.o node.o ../bdecode/bdec.a

leaf: leaf.o dht.o node.o ../bdecode/bdec.a

../bdecode/bdec.a: ../bdecode/bdecode.cpp ../bdecode/bdecode.hpp ../bdecode/bdec.cpp ../bdecode/bdec.h
	@pushd ../bdecode; make ar; popd

RECIPES = all clean run run_locally

.PHONY: $(RECIPES)

all: dht-example

clean:
	-rm -f dht-example dht-example.o dht-example.id dht.o *~ core

run: run-lan-hub run-lan-leaf1 run-lan-leaf2 run-lan-local-leaf
	@echo "Goals successful: $^"; rm $^

HUB_PORT = 3000
#LAN_HUB = 10.0.0.10
LAN_HUB = 127.0.0.1
run-lan-hub:
	@ssh $(LAN_HUB) "cd product/dht; make run-lan-local-hub"
	@echo $@ > $@

run-lan-local-hub: hub
	@[ -d ./nodes ] || mkdir nodes
	@echo "$^ is starting locally on $(LAN_HUB):$(HUB_PORT)"; \
	 ./$^ $(HUB_PORT) > nodes/node0.out &
	@echo "  "
	@echo "  When done with fun, run 'killall -c $^' to kill the hub"
	@echo "  "

run-lan-leaf1:
	@echo $@ > $@

run-lan-leaf2:
	@echo $@ > $@

LEAF_COUNT = 12
run-lan-local-leaf: leaf
	@[ -d ./nodes ] || mkdir nodes
	@pushd nodes; let "COUNT = $(LEAF_COUNT)"; \
	 while [ $$COUNT -gt 0 ]; do \
	  NODE_OUT="node$$COUNT.out"; let "LEAF_PORT = $(HUB_PORT) + COUNT"; \
		let "--COUNT"; \
	  ../$^ $$LEAF_PORT $(LAN_HUB) $(HUB_PORT) > $$NODE_OUT & \
   done; \
	 popd; ps -ef | grep $^
	@echo "  "
	@echo "  When done with fun, run 'killall -c $^' to kill the leaves"
	@echo "  "

LOCAL_SWARM = node0 node1 node2 node3 node4 node5 node6 node7 node8 node9 node10 node11
run_locally: dht-example
	@echo "Run a local cloud with nodes $(LOCAL_SWARM)"
	@[ -x ./dht-example ] || make
	@[ -d ./nodes ] || mkdir nodes
	@pushd nodes; \
	 for node in $(LOCAL_SWARM); do \
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
	@echo "  "
	@echo "  When done with fun, run 'killall -c dht-example' to kill the nodes"
	@echo "  "
