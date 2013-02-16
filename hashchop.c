/* 
 * Copyright (c) 2011-2012 Scott Vokes <vokes.s@gmail.com>
 *  
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *  
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <stdint.h>
#include <string.h>
#include <strings.h>
#include <assert.h>
#include "hashchop.h"

/* Abbreviations. */
typedef uint32_t UI;
typedef uint8_t UC;
#define T hashchop

uint8_t hashchop_version_major = 0;
uint8_t hashchop_version_minor = 8;

struct hashchop {
    uint8_t bits;               /* bits used for mask */
    UI mask;                    /* bitmask for matches */
    UI min;                     /* min chunk size */
    UI max;                     /* max chunk size */
    UI limit;                   /* total buffer size */
    UI ct;                      /* current buffer use */
    UC buf[];                   /* buffer */
};

static void hfree(void *p, size_t sz) { free(p); }

static hashchop_malloc_cb *HMALLOC = malloc;
static hashchop_free_cb *HFREE = hfree;

/* Malloc/free-like functions, to replace malloc and free. */
void hashchop_set_malloc(hashchop_malloc_cb *m, hashchop_free_cb *f) {
    HMALLOC = m;
    HFREE = f;
}

/* The buffer for accumulating data will be the average size * this.
 * TODO: Make this configurable? */
#define LIMIT_BUFFER_MUL 4

/* Chunks will be 2^(bits-SKEW) <= sz < 2^(bits+SKEW). */
#define HASHCHOP_BIT_SKEW 2

/* Create and return a new hashchopper. Returns NULL on error (bad BITS value). */
T *hashchop_new(uint8_t bits) {
    if (bits < HASHCHOP_MIN_BITS || bits > HASHCHOP_MAX_BITS) return NULL;

    UI max = 1 << (bits + HASHCHOP_BIT_SKEW);
    UI mask = (1 << bits) - 1;
    UI min = 1 << (bits - HASHCHOP_BIT_SKEW);
    UI limit = LIMIT_BUFFER_MUL * max;
    T *hc = HMALLOC(sizeof(*hc) + limit);
    if (hc == NULL) return NULL;   /* alloc failure */
    hc->bits = bits;
    hc->mask = mask;
    hc->min = min;
    hc->max = max;
    hc->limit = limit;
    hc->ct = 0;
    bzero(hc->buf, 2*max);
    return hc;
}

/* Sink LENGTH bytes from DATA into the hashchopper.
 * 
 * Returns OVERFLOW if the data is too large to store (and should be added
 * incrementally, in smaller pieces), FULL if the buffer is full (and should
 * be flushed with hashchop_poll first), or OK on success. */
hashchop_res hashchop_sink(T *hc, const unsigned char *data, size_t length) {
    if (length > hc->max) return HASHCHOP_ERROR_OVERFLOW;
    if (hc->ct > hc->limit) return HASHCHOP_ERROR_FULL;
    memcpy(hc->buf + hc->ct, data, length);
    hc->ct += length;
    return HASHCHOP_OK;
}

/* Find natural breaking points in the buffered input.
 * For a description of the algorithm used (a variant of the rsync
 * rolling hash algorithm), see http://rsync.samba.org/tech_report/node3.html.
 *
 * Essentially, this uses a simple checksum which is very cheap to step
 * through a byte array, looking for places (past the minimum length)
 * where the checksum AND'd with a bit mask is 0. */
static UI find_seam(T *hc) {
    UI a = 1, b = 0, len = hc->min, i = 0;
    UI mask = hc->mask, max = hc->max;
    assert(hc->ct >= hc->max);

    for (i = 0; i < len; i++) {
        UC v = hc->buf[i];
        a += v;
        b += (len - i + 1) * v;
    }
    a &= mask; b &= mask;

    for (i = len; i < max; i++) {
        UI k = i - len, l = i;
        UC nk = hc->buf[k], nl = hc->buf[l];
        UI na = (a - nk + nl);
        UI nb = (b - (l - k + 1) * nk + na);
        UI checksum = (na + (nb << 16)) & mask;
        if (checksum == 0) break;
        a = na;
        b = nb;
    }
    return i;
}

/* If available, copy the next chunk of chopped data into DATA, a buffer of
 * at least (*LENGTH) bytes, and write the chunk length in (*LENGTH).
 *
 * Returns UNDERFLOW if there is not enough data to chop another chunk (more
 * data needs to be added with hashchop_sink first), OVERFLOW if the chunk
 * is too large to fit in DATA, or OK if the chunk has been copied. */
hashchop_res hashchop_poll(T *hc, unsigned char *data, size_t *length) {
    if (hc->ct < hc->max) return HASHCHOP_ERROR_UNDERFLOW;
    UI offset = find_seam(hc);
    UI rem = hc->ct - offset;
    if (*length < offset) return HASHCHOP_ERROR_OVERFLOW;
    (*length) = offset;
    /* printf("memcpy %p -> %p, %u bytes\n", hc->buf, data, offset); */
    memcpy(data, hc->buf, offset);
    memmove(hc->buf, hc->buf + offset, rem);
    hc->ct = rem;
    return HASHCHOP_OK;
}

/* The end of the data stream has been reached, so write the remaining
 * buffered data into DATA, save its length in (*LENGTH), and
 * reset the hashchopper. Returns OK on success, or OVERFLOW if
 * (*LENGTH) says that DATA is too small to contain the data. */
hashchop_res hashchop_finish(T *hc, unsigned char *data, size_t *length) {
    if (*length < hc->ct) return HASHCHOP_ERROR_OVERFLOW;
    memcpy(data, hc->buf, hc->ct);
    (*length) = hc->ct;
    hashchop_reset(hc);
    return HASHCHOP_OK;
}

/* Reset a hashchopper, so it can be used to chop a new data stream. */
void hashchop_reset(T *hc) { hc->ct = 0; }

/* Free a hashchopper. */
void hashchop_free(T *hc) { HFREE(hc, sizeof(*hc) + hc->limit); }
