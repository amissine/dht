#include "node.h"

int main (int argc, char **argv) {
  unsigned char myid[20];
  struct sockaddr_storage bootstrap_node;
  int s = udpFd(&argv[1], 0, &bootstrap_node, 1);
  printf("%s dealing with hub %s:%s, s %d\n", argv[0], argv[1], argv[2], s);

  init(s, myid, &bootstrap_node);
  loop(s, argv);
  uninit();

  return 0;
}
