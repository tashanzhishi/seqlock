#pragma once

#include <emmintrin.h>
#include <stdint.h>

#define force_inline __inline__ __attribute__((always_inline))
#define HI_CACHELINE_SIZE 64

#define	hi_compiler_barrier() do {		\
	asm volatile ("" : : : "memory");	\
} while(0)

#define	hi_mb() _mm_mfence()
#define	hi_wmb() _mm_sfence()
#define	hi_rmb() _mm_lfence()
#define hi_pause() _mm_pause()
#define hi_smp_mb() hi_mb()
#define hi_smp_wmb() hi_compiler_barrier()
#define hi_smp_rmb() hi_compiler_barrier()

static force_inline
uint64_t hi_rdtsc() {
    union {
        uint64_t tsc_64;
        struct {
            uint32_t lo_32;
            uint32_t hi_32;
        };
    } tsc;
    asm volatile("rdtsc" : "=a" (tsc.lo_32), "=d" (tsc.hi_32));
    return tsc.tsc_64;
}
