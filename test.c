#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "hashchop.h"
#include "greatest.h"

typedef unsigned char UC;

/* Fill a buffer with reasonably random binary data. */
static void mkrandom(unsigned int seed, unsigned char *buf, size_t len) {
    srandom(seed);
    for (size_t i = 0; i < len; i++) buf[i] = random() % 256;
}

TEST constructor_should_reject_chopper_with_invalid_bits_values() {
    hashchop *hc = hashchop_new(HASHCHOP_MIN_BITS - 1);
    ASSERT_EQ(NULL, hc);
    hc = hashchop_new(HASHCHOP_MAX_BITS + 1);
    ASSERT_EQ(NULL, hc);
    PASS();
}

TEST a_chopper_given_one_chunk_and_closed_should_return_the_one_chunk() {
    hashchop *hc = hashchop_new(10);
    ASSERT(hc);
    size_t sz = 1000;
    UC data[sz], out[sz];
    mkrandom(0, data, sz);
    hashchop_res res = hashchop_sink(hc, data, sz);
    ASSERT_EQ(HASHCHOP_OK, res);
    ASSERT_EQ(HASHCHOP_OK, hashchop_finish(hc, out, &sz));
    ASSERT_EQ(1000, sz);
    for (size_t i = 0; i < sz; i++) {
        ASSERT_EQ(data[i], out[i]);
    }
    PASS();
}

TEST test_a_chopper_given_too_large_a_chunk_should_reject_it() {
    int bits = 10;
    hashchop *hc = hashchop_new(bits);
    ASSERT(hc);
    size_t sz = (2 << (bits + 2)) + 1;
    UC data[sz];
    /* reject -- it won't fit */
    ASSERT_EQ(HASHCHOP_ERROR_OVERFLOW, hashchop_sink(hc, data, sz));
    /* verify that no bytes were ever added */
    ASSERT_EQ(HASHCHOP_OK, hashchop_finish(hc, data, &sz));
    ASSERT_EQ(0, sz);
    PASS();
}

TEST chopping_should_break_and_restore_to_same() {
    int bits = 10;
    hashchop *hc = hashchop_new(bits);
    ASSERT(hc);
    size_t sz = 512 * 100;
    UC data[sz], out[sz];
    size_t used = 0, rem = sz;
    mkrandom(24, data, sz);

    hashchop_res res = HASHCHOP_ERROR_UNDERFLOW;
    for (int i = 0; i < 100; i++) {
        rem = sz - used;
        do {
            res = hashchop_poll(hc, out + used, &rem);
            if (res == HASHCHOP_OK) {
                used += rem;
            } else {
                ASSERT_EQ(HASHCHOP_ERROR_UNDERFLOW, res);
            }
            rem = sz - used;
        } while (res == HASHCHOP_OK);
        res = hashchop_sink(hc, data + (i * 512), 512);
        ASSERT_EQ(HASHCHOP_OK, res); 
    }

    rem = sz - used;
    ASSERT_EQ(HASHCHOP_OK, hashchop_finish(hc, out + used, &rem));
    used += rem;
    ASSERT_EQ(sz, used);

    for (size_t i = 0; i < sz; i++) { ASSERT_EQ(data[i], out[i]); }
    PASS();
}

TEST test_the_same_data_sunk_in_diff_sizes_should_chunk_similarly() {
    int bits = 10;
    hashchop *hc1 = hashchop_new(bits), *hc2 = hashchop_new(bits);
    ASSERT(hc1);  ASSERT(hc2);
    size_t sz = 512 * 100;
    UC data[sz], out1[sz], out2[sz];
    size_t used = 0, rem = sz;
    mkrandom(17, data, sz);

    hashchop_res res = HASHCHOP_ERROR_UNDERFLOW;
    for (int i = 0; i < 100; i++) {
        rem = sz - used;
        do {
            res = hashchop_poll(hc1, out1 + used, &rem);
            if (res == HASHCHOP_OK) {
                used += rem;
            } else {
                ASSERT_EQ(HASHCHOP_ERROR_UNDERFLOW, res);
            }
            rem = sz - used;
        } while (res == HASHCHOP_OK);
        res = hashchop_sink(hc1, data + (i * 512), 512);
        ASSERT_EQ(HASHCHOP_OK, res); 
    }
    rem = sz - used;
    ASSERT_EQ(HASHCHOP_OK, hashchop_finish(hc1, out1 + used, &rem));
    used += rem;
    ASSERT_EQ(sz, used);

    used = 0;
    for (int i = 0; i < 200; i++) {
        rem = sz - used;
        hashchop_res res = HASHCHOP_ERROR_UNDERFLOW;
        do {
            res = hashchop_poll(hc2, out2 + used, &rem);
            if (res == HASHCHOP_OK) {
                used += rem;
            } else {
                ASSERT_EQ(HASHCHOP_ERROR_UNDERFLOW, res);
            }
            rem = sz - used;
        } while (res == HASHCHOP_OK);
        res = hashchop_sink(hc2, data + (i * 256), 256);
        ASSERT_EQ(HASHCHOP_OK, res); 
    }
    rem = sz - used;
    ASSERT_EQ(HASHCHOP_OK, hashchop_finish(hc2, out2 + used, &rem));
    used += rem;
    ASSERT_EQ(sz, used);

    for (size_t i = 0; i < sz; i++) {
        ASSERT_EQ(data[i], out1[i]);
        ASSERT_EQ(out1[i], out2[i]);
    }
    PASS();
}

TEST average_chunk_size_should_be_close_to_2_expt_bits(int bits) {
    hashchop *hc = hashchop_new(bits);
    ASSERT(hc);
    size_t sz = 50 * (1 << bits);
    UC *data = malloc(sz);
    UC *out = malloc(sz);
    unsigned int count = 0, total = 0;
    size_t used = 0, rem = sz;
    mkrandom(24, data, sz);

    int chunk_sz = 1 << (bits - 1);

    for (int i = 0; i < sz/chunk_sz; i++) {
        rem = sz - used;
        hashchop_res res = HASHCHOP_ERROR_UNDERFLOW;
        do {
            res = hashchop_poll(hc, out + used, &rem);
            if (res == HASHCHOP_OK) {
                count++;
                total += rem;
                used += rem;
                if (GREATEST_IS_VERBOSE())
                    fprintf(stdout, "%d bits (%u), %zu bytes, avg %.3f\n",
                        bits, 1 << bits, rem, total / (double) count);
            }
        } while (res == HASHCHOP_OK);
        res = hashchop_sink(hc, data + i*chunk_sz, chunk_sz);
        if (res != HASHCHOP_OK) {
            fprintf(stderr, "res is %d\n", res);
            ASSERT_EQ(HASHCHOP_OK, res); 
        }
    }

    rem = sz - used;
    ASSERT_EQ(HASHCHOP_OK, hashchop_finish(hc, out + used, &rem));
    free(data);
    free(out);

    double avg = total / (double) count;
    if (GREATEST_IS_VERBOSE()) {
        fprintf(stderr, "total: %u, count %u\n", total, count);
        fprintf(stderr, "avg: %.1f, expected: %.1f ~ %.1f\n",
            avg, 0.9*(1 << bits), 1.1*((1 << bits) + (1 << (bits - 2))));
    }
    ASSERT(avg >= 0.9*(1 << bits));
    ASSERT(avg <= 1.1*(((1 << bits) + 1) << (bits - 2)));
    PASS();
}

/* Get a (fairly simple) hash for a buffer. */
static unsigned int hash_buf(unsigned char *buf, size_t length) {
    unsigned int h = 0;
    assert(buf);
    for (size_t i=0; i<length; i++) h = 257*h + buf[i];
    return h;
}

TEST make_data_with_change_and_check_reuse(size_t sz, size_t bytes_changed,
                                           unsigned int seed, unsigned int bits) {
    if (GREATEST_IS_VERBOSE())
        printf("-- seed %d, %zu bytes, %zu bytes changed\n", seed, sz, bytes_changed);
    hashchop *hc = hashchop_new(bits);
    hashchop *hc2 = hashchop_new(bits);
    ASSERT(hc);
    UC *data = malloc(sz);
    UC *data2 = malloc(sz);
    ASSERT(data); ASSERT(data2);
    UC out[1 << ((bits + 2) + 1)];
    unsigned int counts = 0xFFFFF;
    unsigned int *hash_counts = malloc(counts * sizeof(unsigned int));
    ASSERT(hash_counts);
    size_t used = 0, rem = sz;
    bzero(data, sz);
    mkrandom(seed, data, sz);
    memcpy(data2, data, sz);
    mkrandom(seed, data2 + sz/2, bytes_changed);
    bzero(hash_counts, counts*sizeof(unsigned int));

    hashchop_res res = HASHCHOP_ERROR_UNDERFLOW;
    /* first pass, original data */
    for (int i = 0; i < sz / 512; i++) {
        rem = sz - used;
        do {
            res = hashchop_poll(hc, out, &rem);
            if (res == HASHCHOP_OK) {
                used += rem;
                unsigned int hash = hash_buf(out, rem);
                hash_counts[hash & counts]++;
            } else {
                ASSERT_EQ(HASHCHOP_ERROR_UNDERFLOW, res);
            }
            rem = sz - used;
        } while (rem == HASHCHOP_OK);
        res = hashchop_sink(hc, data + (i * 512), 512);
        ASSERT_EQ(HASHCHOP_OK, res); 
    }
    rem = sz - used;
    ASSERT_EQ(HASHCHOP_OK, hashchop_finish(hc, out, &rem));

    hash_counts[hash_buf(out, rem) & counts]++;
    used += rem;
    ASSERT_EQ(sz, used);

    used = 0;
    /* second pass, modified data */
    for (int i = 0; i < sz / 512; i++) {
        rem = sz - used;
        do {
            res = hashchop_poll(hc2, out, &rem);
            if (res == HASHCHOP_OK) {
                unsigned int hash = hash_buf(out, rem);
                if (0) fprintf(stderr, "i %d -> %u, -> %s (change at %lu)\n",
                    i, i * 512, hash_counts[hash & counts] == 0 ? "NEW" : "dup", sz/2);
                hash_counts[hash & counts]++;
                used += rem;
            } else {
                ASSERT_EQ(HASHCHOP_ERROR_UNDERFLOW, res);
            }
            rem = sz - used;
        } while (res == HASHCHOP_OK);
        res = hashchop_sink(hc2, data2 + (i * 512), 512);
        ASSERT_EQ(HASHCHOP_OK, res); 
    }
    rem = sz - used;
    ASSERT_EQ(HASHCHOP_OK, hashchop_finish(hc2, out, &rem));
    hash_counts[hash_buf(out, rem) & counts]++;
    used += rem;
    ASSERT_EQ(sz, used);

    unsigned int singles = 0, dups = 0;
    for (unsigned int i = 0; i < counts; i++) {
        if (hash_counts[i] == 0)
            ;
        else if (hash_counts[i] == 1)
            singles++;
        else {
            dups++;
        }
    }
    double ratio = singles / (double) (singles + dups);
    if (ratio >= 0.05) {
        fprintf(stderr, "re-use %% too low, %.2f, (%u singles, %u dupes)\n",
            100.0 * (1.0 - ratio), singles, dups);
        ASSERTm("> 95% should be re-used", ratio < 0.05);
    }
    free(data); free(data2);
    free(hash_counts);
    PASS();
}

TEST make_data_with_deletion_and_check_reuse(size_t sz, size_t bytes_deleted,
                                             unsigned int seed, unsigned int bits) {
    if (GREATEST_IS_VERBOSE())
        printf("-- seed %d, %zu bytes, %zu bytes changed\n", seed, sz, bytes_deleted);
    hashchop *hc = hashchop_new(bits);
    hashchop *hc2 = hashchop_new(bits);
    ASSERT(hc);
    UC *data = malloc(sz);
    UC *data2 = malloc(sz);
    size_t sz2 = sz - bytes_deleted;
    ASSERT(data); ASSERT(data2);
    UC out[1 << ((bits + 2) + 1)];
    unsigned int counts = 0xFFFFF;
    unsigned int *hash_counts = malloc(counts * sizeof(unsigned int));
    ASSERT(hash_counts);
    size_t used = 0, rem = sz;
    bzero(data, sz);
    mkrandom(seed, data, sz);
    memcpy(data2, data, sz);
    memmove(data2 + sz/2, data2 + sz/2 + bytes_deleted,
        sz/2 - bytes_deleted);
    bzero(hash_counts, counts*sizeof(unsigned int));

    hashchop_res res = HASHCHOP_ERROR_UNDERFLOW;
    /* first pass, original data */
    for (int i = 0; i < sz / 512; i++) {
        rem = sz - used;
        do {
            res = hashchop_poll(hc, out, &rem);
            if (res == HASHCHOP_OK) {
                used += rem;
                unsigned int hash = hash_buf(out, rem);
                hash_counts[hash & counts]++;
            } else {
                ASSERT_EQ(HASHCHOP_ERROR_UNDERFLOW, res);
            }
            rem = sz - used;
        } while (res == HASHCHOP_OK);
        res = hashchop_sink(hc, data + (i * 512), 512);
        ASSERT_EQ(HASHCHOP_OK, res); 
    }
    rem = sz - used;
    ASSERT_EQ(HASHCHOP_OK, hashchop_finish(hc, out, &rem));

    hash_counts[hash_buf(out, rem) & counts]++;
    used += rem;
    ASSERT_EQ(sz, used);

    used = 0;
    /* Second pass, modified data.
     * Just sink the bytes 1 at a time, rather than doing N-byte blocks and then the
     * remaining. Yeah, yeah, it's slow. */
    for (int i = 0; i < sz2; i++) {
        rem = sz2 - used;
        do {
            res = hashchop_poll(hc2, out, &rem);
            if (res == HASHCHOP_OK) {
                unsigned int hash = hash_buf(out, rem);
                if (0) fprintf(stderr, "i %d -> %u, -> %s (change at %lu)\n",
                    i, i * 512, hash_counts[hash & counts] == 0 ? "NEW" : "dup", sz2/2);
                hash_counts[hash & counts]++;
                used += rem;
            } else {
                ASSERT_EQ(HASHCHOP_ERROR_UNDERFLOW, res);
            }
            rem = sz2 - used;
        } while (res == HASHCHOP_OK);
        res = hashchop_sink(hc2, data2 + i, 1);
        ASSERT_EQ(HASHCHOP_OK, res); 
    }
    rem = sz2 - used;
    ASSERT_EQ(HASHCHOP_OK, hashchop_finish(hc2, out, &rem));
    hash_counts[hash_buf(out, rem) & counts]++;
    used += rem;
    ASSERT_EQ(sz2, used);

    unsigned int singles = 0, dups = 0;
    for (unsigned int i = 0; i < counts; i++) {
        if (hash_counts[i] == 0)
            ;
        else if (hash_counts[i] == 1)
            singles++;
        else {
            dups++;
        }
    }
    double ratio = singles / (double) (singles + dups);
    if (ratio >= 0.05) {
        fprintf(stderr, "re-use %% too low, %.2f, (%u singles, %u dupes)\n",
            100.0 * (1.0 - ratio), singles, dups);
        ASSERTm("> 95% should be re-used", ratio < 0.05);
    }
    free(data); free(data2);
    free(hash_counts);
    PASS();
}

SUITE(suite) {
    RUN_TEST(constructor_should_reject_chopper_with_invalid_bits_values);
    RUN_TEST(a_chopper_given_one_chunk_and_closed_should_return_the_one_chunk);
    RUN_TEST(test_a_chopper_given_too_large_a_chunk_should_reject_it);
    RUN_TEST(chopping_should_break_and_restore_to_same);
    RUN_TEST(test_the_same_data_sunk_in_diff_sizes_should_chunk_similarly);

    for (int bits = 10; bits < 16; bits++) {
        RUN_TESTp(average_chunk_size_should_be_close_to_2_expt_bits, bits);
    }

    static const int MB = 1024 * 1024;
    
    for (int seed = 0; seed < 5; seed++) {
        for (int sz_mul = 1; sz_mul < 4; sz_mul++) {
            unsigned int sz = sz_mul * MB;
            for (int change = 1; change < 20; change += 6) {
                RUN_TESTp(make_data_with_change_and_check_reuse, sz, change, seed, 10);
            }
        }
    }

    for (int seed = 0; seed < 5; seed++) {
        unsigned int sz = 64 * MB;
        RUN_TESTp(make_data_with_change_and_check_reuse, sz, 10, seed, 16);
    }

    for (int seed = 0; seed < 5; seed++) {
        unsigned int sz = 64 * MB;
        RUN_TESTp(make_data_with_deletion_and_check_reuse, sz, 10, seed, 16);
    }
}

/* Add all the definitions that need to be in the test runner's main file. */
GREATEST_MAIN_DEFS();

int main(int argc, char **argv) {
    GREATEST_MAIN_BEGIN();      /* command-line arguments, initialization. */
    RUN_SUITE(suite);
    GREATEST_MAIN_END();        /* display results */
}
