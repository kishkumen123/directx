#ifndef PCG_BASIC_H_INCLUDED
#define PCG_BASIC_H_INCLUDED 1

#include <inttypes.h>

#if __cplusplus
extern "C" {
#endif

typedef struct pcg_state_setseq_64{  // Internals are *Private*.
    u64 state;                  // RNG state.  All values are possible.
    u64 inc;                    // Controls which RNG sequence (stream) is
                                     // selected. Must *always* be odd.
} pcg_state_setseq_64;
typedef struct pcg_state_setseq_64 pcg32_random_t;

// If you *must* statically initialize it, here's one.
#define PCG32_INITIALIZER   { 0x853c49e6748fea9bULL, 0xda3e39cb94b95bdbULL }

// state for global RNGs
static pcg32_random_t pcg32_global = PCG32_INITIALIZER;

// Generate a uniformly distributed 32-bit random number
static u32 
random_r(pcg32_random_t* rng){
    u64 oldstate = rng->state;
    rng->state = oldstate * 6364136223846793005ULL + rng->inc;
    u32 xorshifted = ((oldstate >> 18u) ^ oldstate) >> 27u;
    u32 rot = oldstate >> 59u;
    return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
}
static u32 
random(){
    return random_r(&pcg32_global);
}

// Seed the rng.  Specified in two parts, state initializer and a
// sequence selection constant (a.k.a. stream id)
static void 
seed_randomr(pcg32_random_t* rng, u64 initstate, u64 initseq){
    rng->state = 0U;
    rng->inc = (initseq << 1u) | 1u;
    random_r(rng);
    rng->state += initstate;
    random_r(rng);
}
static void 
seed_random(u64 seed, u64 seq){
    seed_randomr(&pcg32_global, seed, seq);
}

// Generate a uniformly distributed number, r, where 0 <= r < bound
static u32 
random_range_r(pcg32_random_t* rng, u32 bound){
    u32 threshold = -bound % bound;

    for (;;) {
        u32 r = random_r(rng);
        if (r >= threshold)
            return r % bound;
    }
}
static u32 
random_range(u32 bound){
    return random_range_r(&pcg32_global, bound);
}

#if __cplusplus
}
#endif

#endif 
