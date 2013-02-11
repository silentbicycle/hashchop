#ifndef HASHCHOP_H
#define HASHCHOP_H

#include <stdint.h>

/* Malloc/free-like functions, to replace malloc and free. */
typedef void *(hashchop_malloc_cb)(size_t sz);
typedef void (hashchop_free_cb)(void *p, size_t sz);
void hashchop_set_malloc(hashchop_malloc_cb *m, hashchop_free_cb *f);

/* Opaque hashchop handle. */
typedef struct hashchop hashchop;

typedef enum HASHCHOP_RES {
    HASHCHOP_OK = 0,
    HASHCHOP_ERROR_UNDERFLOW = -1,
    HASHCHOP_ERROR_OVERFLOW = -2,
    HASHCHOP_ERROR_FULL = -3,
} hashchop_res;

#define HASHCHOP_MIN_BITS 8
#define HASHCHOP_MAX_BITS 30

#define T hashchop

/* Create and return a new hashchopper. Returns NULL on error (bad BITS value). */
T *hashchop_new(uint8_t bits);

/* Sink LENGTH bytes from DATA into the hashchopper.
 * 
 * Returns OVERFLOW if the data is too large to store (and should be added
 * incrementally, in smaller pieces), FULL if the buffer is full (and should
 * be flushed with hashchop_poll), or OK on success. */
hashchop_res hashchop_sink(T *hc, const unsigned char *data, size_t length);

/* If available, copy the next chunk of chopped data into DATA, a buffer of
 * at least (*LENGTH) bytes, and write the chunk length in (*LENGTH).
 *
 * Returns UNDERFLOW if there is not enough data to chop another chunk (more
 * data needs to be added with hashchop_sink first), OVERFLOW if the chunk
 * is too large to fit in DATA, or OK if the chunk has been copied. */
hashchop_res hashchop_poll(T *hc, unsigned char *data, size_t *length);

/* The end of the data stream has been reached, so write the remaining
 * buffered data into DATA, save its length in (*LENGTH), and
 * reset the hashchopper. Returns OK on success, or OVERFLOW if
 * (*LENGTH) says that DATA is too small to contain the data. */
hashchop_res hashchop_finish(T *hc, unsigned char *data, size_t *length);

/* Reset a hashchopper, so it can be used to chop a new data stream. */
void hashchop_reset(T *hc);

/* Free a hashchopper. */
void hashchop_free(T *hc);

#undef T
#endif
