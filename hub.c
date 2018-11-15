#include "node.h"

int main (int argc, char **argv) {
  unsigned char myid[20];
  int s = udpFdBnd(argv[1], 0);
  printf("%s bound to port %s, s %d\n", argv[0], argv[1], s);

  init(s, myid, 0);
  loop(s, argv);
  uninit();

  return 0;
}
