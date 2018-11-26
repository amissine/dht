#include "node.h"

static volatile sig_atomic_t dumping = 0, searching = 0, exiting = 0;

static void
sigdump(int signo)
{
    dumping = 1;
}

static void
sigtest(int signo)
{
    searching = 1;
}

static void
sigexit(int signo)
{
    exiting = 1;
}

static void
init_signals(void)
{
    struct sigaction sa;
    sigset_t ss;

    sigemptyset(&ss);
    sa.sa_handler = sigdump;
    sa.sa_mask = ss;
    sa.sa_flags = 0;
    sigaction(SIGUSR1, &sa, NULL);

    sigemptyset(&ss);
    sa.sa_handler = sigtest;
    sa.sa_mask = ss;
    sa.sa_flags = 0;
    sigaction(SIGUSR2, &sa, NULL);

    sigemptyset(&ss);
    sa.sa_handler = sigexit;
    sa.sa_mask = ss;
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
}

const unsigned char hash[20] = {
    0x54, 0x57, 0x87, 0x89, 0xdf, 0xc4, 0x23, 0xee, 0xf6, 0x03,
    0x1f, 0x81, 0x94, 0xa9, 0x3a, 0x16, 0x98, 0x8b, 0x72, 0x7b
};

/* The call-back function is called by the DHT whenever something
   interesting happens.  Right now, it only happens when we get a new value or
   when a search completes, but this may be extended in future versions. */
static void
callback(void *closure,
         int event,
         const unsigned char *info_hash,
         const void *data, size_t data_len)
{
    if(event == DHT_EVENT_SEARCH_DONE)
        printf("Search done.\n");
    else if(event == DHT_EVENT_VALUES)
        printf("Received %d values.\n", (int)(data_len / 6));
    else
        printf("Unknown DHT event %d.\n", event);
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

void init (int s, unsigned char *myid, struct sockaddr_storage *bootstrap_node) {

  int rc = set_nonblocking(s, 1);
  if (rc < 0) { perror("set_nonblocking"); exit(1); }
  rc = dht_random_bytes(myid, 20); 
  if (rc < 0) { perror("dht_random_bytes"); exit(1); }

  int fd = open("/dev/urandom", O_RDONLY);
  unsigned seed;
  read(fd, &seed, sizeof(seed));
  srandom(seed);
  close(fd);

  /* Init the dht. */
  dht_debug = stdout;
  rc = dht_init(s, -1, myid, (unsigned char*)"AM\0\0");
  if(rc < 0) {
    perror("dht_init");
    exit(1);
  }

  init_signals();

  if (bootstrap_node)
    rc = dht_ping_node((struct sockaddr*)bootstrap_node,
        sizeof(struct sockaddr_in));
  if (rc < 0) { perror("dht_ping_node"); exit(1); }
}

static inline int check_signals (char **argv) {
  if(exiting) {
    printf("\n%s exiting\n", argv[0]);
    return 1;
  }

  /* This is how you trigger a search for a torrent hash.  If port
     (the second argument) is non-zero, it also performs an announce.
     Since peers expire announced data after 30 minutes, it is a good
     idea to reannounce every 28 minutes or so. */
  if(searching) {
    dht_search(hash, 0, AF_INET, callback, NULL);
    searching = 0;
  }

  /* For debugging, or idle curiosity. */
  if(dumping) {
    dht_dump_tables(dht_debug);
    dumping = 0;
  }

  return 0;
}

void loop (int s, char **argv) {
  int rc;
  time_t tosleep = 0;
  struct sockaddr_storage from;
  socklen_t fromlen;

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
    if (check_signals(argv)) break;

    if(rc > 0) {
      fromlen = sizeof(from);
      if(FD_ISSET(s, &readfds))
        rc = recvfrom(s, buf, sizeof(buf) - 1, 0,
                      (struct sockaddr*)&from, &fromlen);
      else abort();
    }
    if(rc > 0) {
      buf[rc] = '\0';
      rc = dht_periodic(buf, rc, (struct sockaddr*)&from, fromlen,
                        &tosleep, callback, NULL);
    } else {
      rc = dht_periodic(NULL, 0, NULL, 0, &tosleep, callback, NULL);
    }
  }
}

void uninit() {

  dht_uninit();
}

/* Functions called by the DHT. */

int
dht_sendto(int sockfd, const void *buf, int len, int flags,
           const struct sockaddr *to, int tolen)
{
  int rc = sendto(sockfd, buf, len, flags, to, tolen);

#ifdef DEBUG_INFO
//  char json[1024];
//  bdec((const char *)buf, len, json, sizeof(json));
//  printf("dht_sendto buf:\n%s\n", json);
  char host[NI_MAXHOST], service[NI_MAXSERV];
  int s = getnameinfo(to, tolen,
      host, NI_MAXHOST, service, NI_MAXSERV, NI_NUMERICHOST | NI_NUMERICSERV);
  if (s == 0)
    printf("dht_sendto len %d to %s:%s sockfd %d rc %d\n", 
        len, host, service, sockfd, rc);
  else fprintf(stderr, "getnameinfo: %s\n", gai_strerror(s));
#endif

  return rc;
}

int
dht_blacklisted(const struct sockaddr *sa, int salen)
{
    return 0;
}

/* We need to provide a reasonably strong cryptographic hashing function.
   Here's how we'd do it if we had RSA's MD5 code. */
#if 0
void
dht_hash(void *hash_return, int hash_size,
         const void *v1, int len1,
         const void *v2, int len2,
         const void *v3, int len3)
{
    static MD5_CTX ctx;
    MD5Init(&ctx);
    MD5Update(&ctx, v1, len1);
    MD5Update(&ctx, v2, len2);
    MD5Update(&ctx, v3, len3);
    MD5Final(&ctx);
    if(hash_size > 16)
        memset((char*)hash_return + 16, 0, hash_size - 16);
    memcpy(hash_return, ctx.digest, hash_size > 16 ? 16 : hash_size);
}
#else
/* But for this toy example, we might as well use something weaker. */
void
dht_hash(void *hash_return, int hash_size,
         const void *v1, int len1,
         const void *v2, int len2,
         const void *v3, int len3)
{
    const char *c1 = v1, *c2 = v2, *c3 = v3;
    char key[9];                /* crypt is limited to 8 characters */
    int i;

    memset(key, 0, 9);
#define CRYPT_HAPPY(c) ((c % 0x60) + 0x20)

    for(i = 0; i < 2 && i < len1; i++)
        key[i] = CRYPT_HAPPY(c1[i]);
    for(i = 0; i < 4 && i < len1; i++)
        key[2 + i] = CRYPT_HAPPY(c2[i]);
    for(i = 0; i < 2 && i < len1; i++)
        key[6 + i] = CRYPT_HAPPY(c3[i]);
    strncpy(hash_return, crypt(key, "jc"), hash_size);
}
#endif

int
dht_random_bytes(void *buf, size_t size)
{
    int fd, rc, save;

    fd = open("/dev/urandom", O_RDONLY);
    if(fd < 0)
        return -1;

    rc = read(fd, buf, size);

    save = errno;
    close(fd);
    errno = save;

    return rc;
}
