#pragma once

#include <stdint.h>
#include <unistd.h>
#include "hi_x86gcc.h"

struct hi_seqlock_t {
    hi_seqlock_t() {
        sequence = 0;
    }

    uint32_t sequence;
};

static force_inline
void write_seqlock(hi_seqlock_t* seqlock) {
    ++seqlock->sequence;
    hi_smp_wmb();
}

static force_inline
void write_sequnlock(hi_seqlock_t* seqlock) {
    hi_smp_wmb();
    seqlock->sequence++;
}

/*
 * if timeout, reader will return sequence(odd number)
*/
static force_inline
uint32_t read_seqbegin(hi_seqlock_t* seqlock, uint64_t timeout_hz) {
    uint64_t first_hz = 0;
    uint32_t ret;
repeat:
    ret = seqlock->sequence;
    hi_smp_rmb();
    if (ret & 1) {
        uint64_t now_hz = hi_rdtsc();
        if (first_hz == 0) {
            first_hz = now_hz;
        } else if (now_hz - first_hz > timeout_hz) {
            return ret;
        }
        hi_pause();
        goto repeat;
    }
    return ret;
}

static force_inline
uint32_t is_read_seqbegin_timeout(uint32_t seq) {
    return (seq & 1);
}

/*
 * reader will loop until get the seqlock
*/
static force_inline
uint32_t read_seqbegin(hi_seqlock_t* seqlock) {
    uint32_t ret;
repeat:
    ret = seqlock->sequence;
    hi_smp_rmb();
    if (ret & 1) {
        hi_pause();
        goto repeat;
    }
    return ret;
}

static force_inline
int read_seqretry(hi_seqlock_t* seqlock, uint32_t start) {
    hi_smp_rmb();
    return (seqlock->sequence != start);
}
