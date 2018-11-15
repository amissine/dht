#ifndef NODE_H
#define NODE_H

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/select.h>
#include <sys/socket.h>

#include "udp.h"
#include "dht.h"

void init (int s, unsigned char *myid, struct sockaddr_storage *bootstrap_node);
void loop (int s, char **argv);
void uninit();

#endif // NODE_H
