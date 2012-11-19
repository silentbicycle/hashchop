#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <err.h>
#include <time.h>
#include "hashchop.h"

#define CHUNK_SZ 1024

static void usage(void) {
    fprintf(stderr, "Usage: bench [BUFFER_SIZE_IN_KB] [SEED] [MASK_BITS]\n");
    exit(0);
}

static void mkrandom(unsigned int seed, unsigned char *buf, size_t len) {
    srandom(seed);
    for (size_t i = 0; i < len; i++) buf[i] = random() % 256;
}

static unsigned char *init(size_t sz, unsigned int seed) {
    unsigned char *buf = NULL;
    buf = malloc(sz);
    if (buf == NULL) { fprintf(stderr, "malloc fail\n"); exit(1); }
    mkrandom(seed, buf, sz);
    return buf;
}

static void go(size_t sz, unsigned char *buf, int bits) {
    int i = 0;
#define BUF_SZ (64L * 1024L)
    unsigned char out[BUF_SZ];
    size_t rem = BUF_SZ;
    hashchop *hc = hashchop_new(bits);
    hashchop_res res = HASHCHOP_ERROR_UNDERFLOW;
    if (hc == NULL) { fprintf(stderr, "bad bits argument\n"); exit(1); }

    for (i = 0; i < sz / CHUNK_SZ; i++) {
        do {
            res = hashchop_poll(hc, out, &rem);
            rem = BUF_SZ;
            if (res == HASHCHOP_OK) {
                ;               /* outgoing chunks not used */
            } else {
                assert(HASHCHOP_ERROR_UNDERFLOW == res);
            }
        } while (res == HASHCHOP_OK);
        res = hashchop_sink(hc, buf + (i * CHUNK_SZ), CHUNK_SZ);
        assert(HASHCHOP_OK == res); 
    }
}

int main(int argc, char **argv) {
    size_t sz = 10L * 1024L * 1024L;
    unsigned int seed = 12345;
    int bits = 14;
    clock_t pre = 0;
    clock_t post = 0;
    unsigned char *buf = NULL;
    if (argc > 1) {
        if (0 == strcmp(argv[1], "-h")) usage();
        sz = atol(argv[1]) * 1024L;
    }
    if (argc > 2) seed = atoi(argv[2]);
    if (argc > 2) bits = atoi(argv[3]);

    buf = init(sz, seed);
    pre = clock();
    if ((int)pre == -1) { fprintf(stderr, "clock fail\n"); exit(1); }
    go(sz, buf, bits);

    post = clock();
    if ((int)post == -1) { fprintf(stderr, "clock fail\n"); exit(1); }
    {
        unsigned long ticks = (unsigned long)post - pre;
        double tdelta = (double)ticks / (1.0 * CLOCKS_PER_SEC);
        printf("%lu bytes -- %lu ticks -- %.3f sec -- %.3f MB/sec\n",
            sz, ticks, tdelta, (sz / (1024L*1024L)) / tdelta);
    }
    return 0;
}
