#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>

#include "udp.h"

static volatile sig_atomic_t exiting = 0;

static void
sigexit (int signo)
{
    exiting = 1;
}

static void
init_signals (void)
{
    struct sigaction sa;
    sigset_t ss;

    sigemptyset(&ss);
    sa.sa_handler = sigexit;
    sa.sa_mask = ss;
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);

    sigemptyset(&ss);
    sa.sa_handler = sigexit;
    sa.sa_mask = ss;
    sa.sa_flags = 0;
    sigaction(SIGTERM, &sa, NULL);
}

static int
set_nonblocking (int fd, int nonblocking)
{
    int rc;
    rc = fcntl(fd, F_GETFL, 0);
    if(rc < 0)
        return -1;

    rc = fcntl(fd, F_SETFL,
               nonblocking ? (rc | O_NONBLOCK) : (rc & ~O_NONBLOCK));
    if(rc < 0)
        return -1;

    return 0;
}

static unsigned char buf[4096];

int main (int argc, char **argv) {
  printf("%s binding to port %s\n", argv[0], argv[1]);

  time_t tosleep = 0;
  struct sockaddr_storage from;
  socklen_t fromlen;

  int s = udpFdBnd(argv[1], 0);
  int rc = set_nonblocking(s, 1);
  if (rc < 0) { perror("set_nonblocking"); exit(1); }

  init_signals();

  while(1) {
    struct timeval tv;
    fd_set readfds;
    tv.tv_sec = tosleep;
    tv.tv_usec = random() % 1000000;

    FD_ZERO(&readfds);
    FD_SET(s, &readfds);
    rc = select(s + 1, &readfds, NULL, NULL, &tv);
    if(rc < 0) {
      if(errno != EINTR) {
        perror("select");
        sleep(1);
      }
    }
    if(exiting) {
      printf("%s exiting\n", argv[0]);
      break;
    }

    if(rc > 0) {
      fromlen = sizeof(from);
      if(FD_ISSET(s, &readfds))
        rc = recvfrom(s, buf, sizeof(buf) - 1, 0,
                      (struct sockaddr*)&from, &fromlen);
      else
        abort();
    }
    if(rc > 0) {
      buf[rc] = '\0';
      printf("%s\n", buf);
    }
  }
  return 0;
}
