/*
 * File: ticket.c
 * Author: Tudor David <tudor.david@epfl.ch>, Vasileios Trigonakis <vasileios.trigonakis@epfl.ch>
 *
 * Description: 
 *      An implementation of a ticket lock with:
 *       - proportional back-off optimization
 *       - pretetchw for write optitization for the AMD Opteron
 *           Magny-Cours processors
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013 Tudor David, Vasileios Trigonakis
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */




#ifndef _PLATFORM_DEFS_H_INCLUDED_
#define _PLATFORM_DEFS_H_INCLUDED_

#ifdef __cplusplus
extern "C" {
#endif
    /*
     * For each machine that is used, one needs to define
     *  NUMBER_OF_SOCKETS: the number of sockets the machine has
     *  CORES_PER_SOCKET: the number of cores per socket
     *  CACHE_LINE_SIZE
     *  NOP_DURATION: the duration in cycles of a noop instruction (generally 1 cycle on most small machines)
     *  the_cores - a mapping from the core ids as configured in the OS to physical cores (the OS might not alwas be configured corrrectly)
     *  get_cluster - a function that given a core id returns the socket number ot belongs to
     */


#ifdef DEFAULT
#  define NUMBER_OF_SOCKETS 1
#  define CORES_PER_SOCKET CORE_NUM
#  define CACHE_LINE_SIZE 64
# define NOP_DURATION 2
  static uint8_t  __attribute__ ((unused)) the_cores[] = {
        0, 1, 2, 3, 4, 5, 6, 7, 
        8, 9, 10, 11, 12, 13, 14, 15, 
        16, 17, 18, 19, 20, 21, 22, 23, 
        24, 25, 26, 27, 28, 29, 30, 31, 
        32, 33, 34, 35, 36, 37, 38, 39, 
        40, 41, 42, 43, 44, 45, 46, 47  
    };
#endif

#ifdef SPARC
#  define NUMBER_OF_SOCKETS 8
#  define CORES_PER_SOCKET 8
#  define CACHE_LINE_SIZE 64
# define NOP_DURATION 9

#define ALTERNATE_SOCKETS
#ifdef ALTERNATE_SOCKETS
    static uint8_t  __attribute__ ((unused)) the_cores[] = {
        0, 8, 16, 24, 32, 40, 48, 56, 
        1, 9, 17, 25, 33, 41, 49, 57, 
        2, 10, 18, 26, 34, 42, 50, 58, 
        3, 11, 19, 27, 35, 43, 51, 59, 
        4, 12, 20, 28, 36, 44, 52, 60, 
        5, 13, 21, 29, 37, 45, 53, 61, 
        6, 14, 22, 30, 38, 46, 54, 62, 
        7, 15, 23, 31, 39, 47, 55, 63 
    };

    static uint8_t the_sockets[] = {
        0, 0, 0, 0, 0, 0, 0, 0, 
        1, 1, 1, 1, 1, 1, 1, 1, 
        2, 2, 2, 2, 2, 2, 2, 2, 
        3, 3, 3, 3, 3, 3, 3, 3, 
        4, 4, 4, 4, 4, 4, 4, 4, 
        5, 5, 5, 5, 5, 5, 5, 5, 
        6, 6, 6, 6, 6, 6, 6, 6, 
        7, 7, 7, 7, 7, 7, 7, 7 
    };

#else
    static uint8_t  __attribute__ ((unused)) the_cores[] = {
        0, 1, 2, 3, 4, 5, 6, 7, 
        8, 9, 10, 11, 12, 13, 14, 15, 
        16, 17, 18, 19, 20, 21, 22, 23, 
        24, 25, 26, 27, 28, 29, 30, 31, 
        32, 33, 34, 35, 36, 37, 38, 39, 
        40, 41, 42, 43, 44, 45, 46, 47, 
        48, 49, 50, 51, 52, 53, 54, 55, 
        56, 57, 58, 59, 60, 61, 62, 63 
    };
    static uint8_t the_sockets[] = {
        0, 0, 0, 0, 0, 0, 0, 0, 
        1, 1, 1, 1, 1, 1, 1, 1, 
        2, 2, 2, 2, 2, 2, 2, 2, 
        3, 3, 3, 3, 3, 3, 3, 3, 
        4, 4, 4, 4, 4, 4, 4, 4, 
        5, 5, 5, 5, 5, 5, 5, 5, 
        6, 6, 6, 6, 6, 6, 6, 6, 
        7, 7, 7, 7, 7, 7, 7, 7 
    };

#endif
#elif defined __tile__
#define NUMBER_OF_SOCKETS 1
#define CORES_PER_SOCKET 36
#define CACHE_LINE_SIZE 64
# define NOP_DURATION 4
    static uint8_t  __attribute__ ((unused)) the_cores[] = {
        0, 1, 2, 3, 4, 5, 6, 7, 
        8, 9, 10, 11, 12, 13, 14, 15, 
        16, 17, 18, 19, 20, 21, 22, 23, 
        24, 25, 26, 27, 28, 29, 30, 31, 
        32, 33, 34, 35 
    };

#elif defined(OPTERON)
#  define NUMBER_OF_SOCKETS 8
#  define CORES_PER_SOCKET 6
#  define CACHE_LINE_SIZE 64
# define NOP_DURATION 2
    static uint8_t   __attribute__ ((unused)) the_cores[] = {
        0, 1, 2, 3, 4, 5, 6, 7, 
        8, 9, 10, 11, 12, 13, 14, 15, 
        16, 17, 18, 19, 20, 21, 22, 23, 
        24, 25, 26, 27, 28, 29, 30, 31, 
        32, 33, 34, 35, 36, 37, 38, 39, 
        40, 41, 42, 43, 44, 45, 46, 47  
    };

#elif defined(XEON)
#  define NUMBER_OF_SOCKETS 8
#  define CORES_PER_SOCKET 10
#  define CACHE_LINE_SIZE 64
# define NOP_DURATION 1
    static uint8_t   __attribute__ ((unused)) the_cores[] = {
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
        11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
        21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 
        31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
        0, 41, 42, 43, 44, 45, 46, 47, 48, 49,
        50, 51, 52, 53, 54, 55, 56, 57, 58, 59,
        60, 61, 62, 63, 64, 65, 66, 67, 68, 69,
        70, 71, 72, 73, 74, 75, 76, 77, 78, 79 
    };
    static uint8_t the_sockets[] = 
    {
        4, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 2, 2, 2, 2, 2, 2, 2, 2, 2,
        2, 3, 3, 3, 3, 3, 3, 3, 3, 3,
        3, 4, 4, 4, 4, 4, 4, 4, 4, 4,
        5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 
        6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    };

#endif

#if defined(OPTERON)
#  define PREFETCHW(x)		     asm volatile("prefetchw %0" :: "m" (*(unsigned long *)x))
#elif defined(__sparc__)
#  define PREFETCHW(x)		
#elif defined(XEON)
#  define PREFETCHW(x)		
#else
#  define PREFETCHW(x)		
#endif


#ifdef __cplusplus
}

#endif


#endif



#ifndef _UTILS_H_INCLUDED_
#define _UTILS_H_INCLUDED_

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <malloc.h>
#include <sched.h>
#include <inttypes.h>
#include <sys/time.h>
#include <unistd.h>
#ifdef __sparc__
#  include <sys/types.h>
#  include <sys/processor.h>
#  include <sys/procset.h>
#elif defined(__tile__)
#include <arch/atomic.h>
#include <arch/cycle.h>
#include <tmc/cpus.h>
#include <tmc/task.h>
#include <tmc/spin.h>
#include <sched.h>
#else
#  include <emmintrin.h>
#  include <xmmintrin.h>
#  include <numa.h>
#endif
#include <pthread.h>


#ifdef __cplusplus
extern "C" {
#endif

#define ALIGNED(N) __attribute__ ((aligned (N)))

#ifdef __sparc__
#  define PAUSE    asm volatile("rd    %%ccr, %%g0\n\t" \
        ::: "memory")

#elif defined(__tile__)
#define PAUSE cycle_relax()
#else
#define PAUSE _mm_pause()
#endif
    static inline void
        pause_rep(uint32_t num_reps)
        {
            uint32_t i;
            for (i = 0; i < num_reps; i++)
            {
                PAUSE;
                /* PAUSE; */
                /* asm volatile ("NOP"); */
            }
        }

    static inline void
        nop_rep(uint32_t num_reps)
        {
            uint32_t i;
            for (i = 0; i < num_reps; i++)
            {
                asm volatile ("NOP");
            }
        }




    //debugging functions
#ifdef DEBUG
#  define DPRINT(args...) fprintf(stderr,args);
#  define DDPRINT(fmt, args...) printf("%s:%s:%d: "fmt, __FILE__, __FUNCTION__, __LINE__, args)
#else
#  define DPRINT(...)
#  define DDPRINT(fmt, ...)
#endif




    typedef uint64_t ticks;

    static inline double wtime(void)
    {
        struct timeval t;
        gettimeofday(&t,NULL);
        return (double)t.tv_sec + ((double)t.tv_usec)/1000000.0;
    }


#if defined(__i386__)
    static inline ticks getticks(void) {
        ticks ret;

        __asm__ __volatile__("rdtsc" : "=A" (ret));
        return ret;
    }
#elif defined(__x86_64__)
    static inline ticks getticks(void)
    {
        unsigned hi, lo;
        __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
        return ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );
    }
#elif defined(__sparc__)
    static inline ticks getticks(){
        ticks ret;
        __asm__ __volatile__ ("rd %%tick, %0" : "=r" (ret) : "0" (ret)); 
        return ret;
    }
#elif defined(__tile__)
    static inline ticks getticks(){
        return get_cycle_count();
    }
#endif

    static inline void cdelay(ticks cycles){
        ticks __ts_end = getticks() + (ticks) cycles;
        while (getticks() < __ts_end);
    }

    static inline void cpause(ticks cycles){
#if defined(XEON)
        cycles >>= 3;
        ticks i;
        for (i=0;i<cycles;i++) {
            _mm_pause();
        }
#else
        ticks i;
        for (i=0;i<cycles;i++) {
            __asm__ __volatile__("nop");
        }
#endif
    }

    static inline void udelay(unsigned int micros)
    {
        double __ts_end = wtime() + ((double) micros / 1000000);
        while (wtime() < __ts_end);
    }

    //getticks needs to have a correction because the call itself takes a
    //significant number of cycles and skewes the measurement
    static inline ticks getticks_correction_calc(void) {
#define GETTICKS_CALC_REPS 5000000
        ticks t_dur = 0;
        uint32_t i;
        for (i = 0; i < GETTICKS_CALC_REPS; i++) {
            ticks t_start = getticks();
            ticks t_end = getticks();
            t_dur += t_end - t_start;
        }
        //    printf("corr in float %f\n", (t_dur / (double) GETTICKS_CALC_REPS));
        ticks getticks_correction = (ticks)(t_dur / (double) GETTICKS_CALC_REPS);
        return getticks_correction;
    }

    static inline ticks get_noop_duration(void) {
#define NOOP_CALC_REPS 1000000
        ticks noop_dur = 0;
        uint32_t i;
        ticks corr = getticks_correction_calc();
        ticks start;
        ticks end;
        start = getticks();
        for (i=0;i<NOOP_CALC_REPS;i++) {
            __asm__ __volatile__("nop");
        }
        end = getticks();
        noop_dur = (ticks)((end-start-corr)/(double)NOOP_CALC_REPS);
        return noop_dur;
    }

    /// Round up to next higher power of 2 (return x if it's already a power
    /// of 2) for 32-bit numbers
    static inline uint32_t pow2roundup (uint32_t x){
        if (x==0) return 1;
        --x;
        x |= x >> 1;
        x |= x >> 2;
        x |= x >> 4;
        x |= x >> 8;
        x |= x >> 16;
        return x+1;
    }
#define my_random xorshf96

    /* 
     * Returns a pseudo-random value in [1;range).
     * Depending on the symbolic constant RAND_MAX>=32767 defined in stdlib.h,
     * the granularity of rand() could be lower-bounded by the 32767^th which might 
     * be too high for given values of range and initial.
     */
    static inline long rand_range(long r) {
        int m = RAND_MAX;
        long d, v = 0;

        do {
            d = (m > r ? r : m);
            v += 1 + (long) (d * ((double) rand() / ((double) (m) + 1.0)));
            r -= m;
        } while (r > 0);
        return v;
    }

    //fast but weak random number generator for the sparc machine
    static inline uint32_t fast_rand(void) {
        return ((getticks()&4294967295)>>4);
    }


    //Marsaglia's xorshf generator
    static inline unsigned long xorshf96(unsigned long* x, unsigned long* y, unsigned long* z) {          //period 2^96-1
        unsigned long t;
        (*x) ^= (*x) << 16;
        (*x) ^= (*x) >> 5;
        (*x) ^= (*x) << 1;

        t = *x;
        (*x) = *y;
        (*y) = *z;
        (*z) = t ^ (*x) ^ (*y);

        return *z;
    }

#ifdef __cplusplus
}

#endif


#endif



#ifndef _ATOMIC_OPS_H_INCLUDED_
#define _ATOMIC_OPS_H_INCLUDED_

#include <inttypes.h>

#define COMPILER_BARRIER asm volatile("" ::: "memory")
#ifdef __sparc__
/*
 *  sparc code
 */

#  include <atomic.h>

//test-and-set uint8_t
static inline uint8_t tas_uint8(volatile uint8_t *addr) {
    uint8_t oldval;
    __asm__ __volatile__("ldstub %1,%0"
            : "=r"(oldval), "=m"(*addr)
            : "m"(*addr) : "memory");
    return oldval;
}


static inline unsigned long xchg32(volatile unsigned int *m, unsigned int val)
{
    unsigned long tmp1, tmp2;

    __asm__ __volatile__(
            "       mov             %0, %1\n"
            "1:     lduw            [%4], %2\n"
            "       cas             [%4], %2, %0\n"
            "       cmp             %2, %0\n"
            "       bne,a,pn        %%icc, 1b\n"
            "        mov            %1, %0\n"
            : "=&r" (val), "=&r" (tmp1), "=&r" (tmp2)
            : "0" (val), "r" (m)
            : "cc", "memory");
    return val;
}

static inline unsigned long xchg64(volatile unsigned long *m, unsigned long val)                                                                                                             
{
    unsigned long tmp1, tmp2;

    __asm__ __volatile__(
            "       mov             %0, %1\n"
            "1:     ldx             [%4], %2\n"
            "       casx            [%4], %2, %0\n"
            "       cmp             %2, %0\n"
            "       bne,a,pn        %%xcc, 1b\n"
            "        mov            %1, %0\n"
            : "=&r" (val), "=&r" (tmp1), "=&r" (tmp2)
            : "0" (val), "r" (m)
            : "cc", "memory");
    return val;
}


//Compare-and-swap
#define CAS_PTR(a,b,c) atomic_cas_ptr(a,b,c)
#define CAS_U8(a,b,c) atomic_cas_8(a,b,c)
#define CAS_U16(a,b,c) atomic_cas_16(a,b,c)
#define CAS_U32(a,b,c) atomic_cas_32(a,b,c)
#define CAS_U64(a,b,c) atomic_cas_64(a,b,c)
//Swap
#define SWAP_PTR(a,b) atomic_swap_ptr(a,b)
#define SWAP_U8(a,b) atomic_swap_8(a,b)
#define SWAP_U16(a,b) atomic_swap_16(a,b)
#define SWAP_U32(a,b) xchg32(a,b)
#define SWAP_U64(a,b) atomic_swap_64(a,b)
//Fetch-and-increment
#define FAI_U8(a) (atomic_inc_8_nv(a)-1)
#define FAI_U16(a) (atomic_inc_16_nv(a)-1)
#define FAI_U32(a) (atomic_inc_32_nv(a)-1)
#define FAI_U64(a) (atomic_inc_64_nv(a)-1)
//Fetch-and-decrement
#define FAD_U8(a) (atomic_dec_8_nv(a,)+1)
#define FAD_U16(a) (atomic_dec_16_nv(a)+1)
#define FAD_U32(a) (atomic_dec_32_nv(a)+1)
#define FAD_U64(a) (atomic_dec_64_nv(a)+1)
//Increment-and-fetch
#define IAF_U8(a) atomic_inc_8_nv(a)
#define IAF_U16(a) atomic_inc_16_nv(a)
#define IAF_U32(a) atomic_inc_32_nv(a)
#define IAF_U64(a) atomic_inc_64_nv(a)
//Decrement-and-fetch
#define DAF_U8(a) atomic_dec_8_nv(a)
#define DAF_U16(a) atomic_dec_16_nv(a)
#define DAF_U32(a) atomic_dec_32_nv(a)
#define DAF_U64(a) atomic_dec_64_nv(a)
//Test-and-set
#define TAS_U8(a) tas_uint8(a)
//Memory barrier
#define MEM_BARRIER asm volatile("membar #LoadLoad | #LoadStore | #StoreLoad | #StoreStore"); 
//end of sparc code
#elif defined(__tile__)
/*
 *  Tilera code
 */
#include <arch/atomic.h>
#include <arch/cycle.h>
//atomic operations interface
//Compare-and-swap
#define CAS_PTR(a,b,c) arch_atomic_val_compare_and_exchange(a,b,c)
#define CAS_U8(a,b,c)  arch_atomic_val_compare_and_exchange(a,b,c)
#define CAS_U16(a,b,c) arch_atomic_val_compare_and_exchange(a,b,c)
#define CAS_U32(a,b,c) arch_atomic_val_compare_and_exchange(a,b,c)
#define CAS_U64(a,b,c) arch_atomic_val_compare_and_exchange(a,b,c)
//Swap
#define SWAP_PTR(a,b) arch_atomic_exchange(a,b)
#define SWAP_U8(a,b) arch_atomic_exchange(a,b)
#define SWAP_U16(a,b) arch_atomic_exchange(a,b)
#define SWAP_U32(a,b) arch_atomic_exchange(a,b)
#define SWAP_U64(a,b) arch_atomic_exchange(a,b)
//Fetch-and-increment
#define FAI_U8(a) arch_atomic_increment(a)
#define FAI_U16(a) arch_atomic_increment(a)
#define FAI_U32(a) arch_atomic_increment(a)
#define FAI_U64(a) arch_atomic_increment(a)
//Fetch-and-decrement
#define FAD_U8(a) arch_atomic_decrement(a)
#define FAD_U16(a) arch_atomic_decrement(a)
#define FAD_U32(a) arch_atomic_decrement(a)
#define FAD_U64(a) arch_atomic_decrement(a)
//Increment-and-fetch
#define IAF_U8(a) (arch_atomic_increment(a)+1)
#define IAF_U16(a) (arch_atomic_increment(a)+1)
#define IAF_U32(a) (arch_atomic_increment(a)+1)
#define IAF_U64(a) (arch_atomic_increment(a)+1)
//Decrement-and-fetch
#define DAF_U8(a) (arch_atomic_decrement(a)-1)
#define DAF_U16(a) (arch_atomic_decrement(a)-1)
#define DAF_U32(a) (arch_atomic_decrement(a)-1)
#define DAF_U64(a) (arch_atomic_decrement(a)-1)
//Test-and-set
#define TAS_U8(a) arch_atomic_val_compare_and_exchange(a,0,0xff)
//Memory barrier
#define MEM_BARRIER arch_atomic_full_barrier()
//Relax CPU
//define PAUSE cycle_relax()

//end of tilera code
#else
/*
 *  x86 code
 */

#  include <xmmintrin.h>

//Swap pointers
static inline void* swap_pointer(volatile void* ptr, void *x) {
#  ifdef __i386__
    __asm__ __volatile__("xchgl %0,%1"
            :"=r" ((unsigned) x)
            :"m" (*(volatile unsigned *)ptr), "0" (x)
            :"memory");

    return x;
#  elif defined(__x86_64__)
    __asm__ __volatile__("xchgq %0,%1"
            :"=r" ((unsigned long long) x)
            :"m" (*(volatile long long *)ptr), "0" ((unsigned long long) x)
            :"memory");

    return x;
#  endif
}

//Swap uint64_t
static inline uint64_t swap_uint64(volatile uint64_t* target,  uint64_t x) {
    __asm__ __volatile__("xchgq %0,%1"
            :"=r" ((uint64_t) x)
            :"m" (*(volatile uint64_t *)target), "0" ((uint64_t) x)
            :"memory");

    return x;
}

//Swap uint32_t
static inline uint32_t swap_uint32(volatile uint32_t* target,  uint32_t x) {
    __asm__ __volatile__("xchgl %0,%1"
            :"=r" ((uint32_t) x)
            :"m" (*(volatile uint32_t *)target), "0" ((uint32_t) x)
            :"memory");

    return x;
}

//Swap uint16_t
static inline uint16_t swap_uint16(volatile uint16_t* target,  uint16_t x) {
    __asm__ __volatile__("xchgw %0,%1"
            :"=r" ((uint16_t) x)
            :"m" (*(volatile uint16_t *)target), "0" ((uint16_t) x)
            :"memory");

    return x;
}

//Swap uint8_t
static inline uint8_t swap_uint8(volatile uint8_t* target,  uint8_t x) {
    __asm__ __volatile__("xchgb %0,%1"
            :"=r" ((uint8_t) x)
            :"m" (*(volatile uint8_t *)target), "0" ((uint8_t) x)
            :"memory");

    return x;
}

//test-and-set uint8_t
static inline uint8_t tas_uint8(volatile uint8_t *addr) {
    uint8_t oldval;
    __asm__ __volatile__("xchgb %0,%1"
            : "=q"(oldval), "=m"(*addr)
            : "0"((unsigned char) 0xff), "m"(*addr) : "memory");
    return (uint8_t) oldval;
}

//atomic operations interface
//Compare-and-swap
#define CAS_PTR(a,b,c) __sync_val_compare_and_swap(a,b,c)
#define CAS_U8(a,b,c) __sync_val_compare_and_swap(a,b,c)
#define CAS_U16(a,b,c) __sync_val_compare_and_swap(a,b,c)
#define CAS_U32(a,b,c) __sync_val_compare_and_swap(a,b,c)
#define CAS_U64(a,b,c) __sync_val_compare_and_swap(a,b,c)
//Swap
#define SWAP_PTR(a,b) swap_pointer(a,b)
#define SWAP_U8(a,b) swap_uint8(a,b)
#define SWAP_U16(a,b) swap_uint16(a,b)
#define SWAP_U32(a,b) swap_uint32(a,b)
#define SWAP_U64(a,b) swap_uint64(a,b)
//Fetch-and-increment
#define FAI_U8(a) __sync_fetch_and_add(a,1)
#define FAI_U16(a) __sync_fetch_and_add(a,1)
#define FAI_U32(a) __sync_fetch_and_add(a,1)
#define FAI_U64(a) __sync_fetch_and_add(a,1)
//Fetch-and-decrement
#define FAD_U8(a) __sync_fetch_and_sub(a,1)
#define FAD_U16(a) __sync_fetch_and_sub(a,1)
#define FAD_U32(a) __sync_fetch_and_sub(a,1)
#define FAD_U64(a) __sync_fetch_and_sub(a,1)
//Increment-and-fetch
#define IAF_U8(a) __sync_add_and_fetch(a,1)
#define IAF_U16(a) __sync_add_and_fetch(a,1)
#define IAF_U32(a) __sync_add_and_fetch(a,1)
#define IAF_U64(a) __sync_add_and_fetch(a,1)
//Decrement-and-fetch
#define DAF_U8(a) __sync_sub_and_fetch(a,1)
#define DAF_U16(a) __sync_sub_and_fetch(a,1)
#define DAF_U32(a) __sync_sub_and_fetch(a,1)
#define DAF_U64(a) __sync_sub_and_fetch(a,1)
//Test-and-set
#define TAS_U8(a) tas_uint8(a)
//Memory barrier
#define MEM_BARRIER __sync_synchronize()
//Relax CPU
//#define PAUSE _mm_pause()

/*End of x86 code*/
#endif


#endif
#ifndef _TICKET_H_
#define _TICKET_H_

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#if defined(PLATFORM_NUMA)
#  include <numa.h>
#endif
#include <pthread.h>

/* setting of the back-off based on the length of the queue */
#define TICKET_BASE_WAIT 512
#define TICKET_MAX_WAIT  4095
#define TICKET_WAIT_NEXT 128

#define TICKET_ON_TW0_CLS 0	/* Put the head and the tail on separate 
                               cache lines (O: not, 1: do)*/
typedef struct ticketlock_t 
{
    volatile uint32_t head;
#if TICKET_ON_TW0_CLS == 1
    uint8_t padding0[CACHE_LINE_SIZE - 4];
#endif
    volatile uint32_t tail;
#ifdef ADD_PADDING
    uint8_t padding1[CACHE_LINE_SIZE - 8];
#  if TICKET_ON_TW0_CLS == 1
    uint8_t padding2[4];
#  endif
#endif
} ticketlock_t;



int ticket_trylock(ticketlock_t* lock);
void ticket_acquire(ticketlock_t* lock);
void ticket_release(ticketlock_t* lock);
int is_free_ticket(ticketlock_t* t);

int create_ticketlock(ticketlock_t* the_lock);
ticketlock_t* init_ticketlocks(uint32_t num_locks);
//void init_thread_ticketlocks(uint32_t thread_num);
void free_ticketlocks(ticketlock_t* the_locks);

#if defined(MEASURE_CONTENTION)
extern void ticket_print_contention_stats(void);
double ticket_avg_queue(void);
#endif	/* MEASURE_CONTENTION */

#endif


/* enable measure contantion to collect statistics about the 
   average queuing per lock acquisition */

static inline uint32_t
sub_abs(const uint32_t a, const uint32_t b)
{
  if (a > b)
    {
      return a - b;
    }
  else
    {
      return b - a;
    }
}

int
ticket_trylock(ticketlock_t* lock) 
{
  uint32_t me = lock->tail;
  uint32_t me_new = me + 1;
  uint64_t cmp = ((uint64_t) me << 32) + me_new; 
  uint64_t cmp_new = ((uint64_t) me_new << 32) + me_new; 
  uint64_t* la = (uint64_t*) lock;
  if (CAS_U64(la, cmp, cmp_new) == cmp) 
    {
      return 0;
    }
  return 1;
}

void
ticket_acquire(ticketlock_t* lock) 
{
  uint32_t my_ticket = IAF_U32(&(lock->tail));


#if defined(OPTERON_OPTIMIZE)
  uint32_t wait = TICKET_BASE_WAIT;
  uint32_t distance_prev = 1;

  while (1)
    {
      PREFETCHW(lock);
      uint32_t cur = lock->head;
      if (cur == my_ticket)
        {
	  break;
        }
      uint32_t distance = sub_abs(cur, my_ticket);

      if (distance > 1)
        {
	  if (distance != distance_prev)
            {
	      distance_prev = distance;
	      wait = TICKET_BASE_WAIT;
            }

	  nop_rep(distance * wait);
	  /* wait = (wait + TICKET_BASE_WAIT) & TICKET_MAX_WAIT; */
        }
      else
        {
	  nop_rep(TICKET_WAIT_NEXT);
        }

      if (distance > 20)
        {
	  sched_yield();
	  /* pthread_yield(); */
        }
    }

#else  /* !OPTERON_OPTIMIZE */
  /* backoff proportional to the distance would make sense even without the PREFETCHW */
  /* however, I did some tests on the Niagara and it performed worse */

#  if defined(__x86_64__)

  uint32_t wait = TICKET_BASE_WAIT;
  uint32_t distance_prev = 1;

  while (1)
    {
      uint32_t cur = lock->head;
      if (cur == my_ticket)
        {
	  break;
        }
      uint32_t distance = sub_abs(cur, my_ticket);

      if (distance > 1)
        {
	  if (distance != distance_prev)
            {
	      distance_prev = distance;
	      wait = TICKET_BASE_WAIT;
            }

	  nop_rep(distance * wait);
        }
      else
        {
	  nop_rep(TICKET_WAIT_NEXT);
        }

      if (distance > 20)
        {
	  sched_yield();
        }
    }
#  else
  while (lock->head != my_ticket)
    {
      PAUSE;
    }
#  endif
#endif	/* OPTERON_OPTIMIZE */
}

void
ticket_release(ticketlock_t* lock) 
{
#ifdef __tile__
  MEM_BARRIER;
#endif
#if defined(OPTERON_OPTIMIZE)
  PREFETCHW(lock);
#endif	/* OPTERON */
  COMPILER_BARRIER;
  lock->head++;
}


int create_ticketlock(ticketlock_t* the_lock) 
{
    the_lock->head=1;
    the_lock->tail=0;
    MEM_BARRIER;
    return 0;
}


int is_free_ticket(ticketlock_t* t)
{
  if ((t->head - t->tail) == 1) 
    {
      return 1;
    }
  return 0;
}


ticketlock_t* 
init_ticketlocks(uint32_t num_locks) 
{
  ticketlock_t* the_locks;
  the_locks = (ticketlock_t*) malloc(num_locks * sizeof(ticketlock_t));
  uint32_t i;
  for (i = 0; i < num_locks; i++) 
    {
      the_locks[i].head=1;
      the_locks[i].tail=0;
    }
  MEM_BARRIER;
  return the_locks;
}

void
free_ticketlocks(ticketlock_t* the_locks) 
{
  free(the_locks);
}


