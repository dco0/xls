
#pragma once

#include <cstdlib>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
// sometimes it's not defined, so we have to do this
extern int nanosleep(const struct timespec *__requested_time, struct timespec *__remaining);

#include "ulp.cpp"


struct Search {

    ULP ulp = ULP();

    size_t set_unknown_cells = 0;
    size_t start_unknown_cells = 0;

    uintmax_t solutions_found = 0;
    uintmax_t branches = 0;

    double start_time = 0;

};


#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    #ifdef _MSC_VER
        #include <intrin.h>
    #else
        #include <x86intrin.h>
    #endif
    static uint64_t read_counter() {
        return __rdtsc();
    }
#elif defined(__aarch64__)
    static uint64_t read_counter() {
        uint64_t val;
        __asm__ volatile("mrs %0, cntvct_el0" : "=r" (val));
        return val;
    }
#elif defined(__riscv) && __riscv_xlen == 64
    static uint64_t read_counter() {
        uint64_t cycles;
        __asm__ volatile("rdcycle %0" : "=r"(cycles));
        return cycles;
    }
#elif defined(__powerpc__) || defined(__ppc__)
    static uint64_t read_counter() {
        uint64_t tb;
        __asm__ volatile("mfspr %0, 268" : "=r"(tb));
        return tb;
    }
#else
    static uint64_t read_counter() {
        return __builtin_readcyclecounter();
    }
#endif

static double cycles_per_second;

static void calibrate_time() {
    struct timespec request;
    struct timespec remainder;
    // 0.01 seconds
    request.tv_sec = 0;
    request.tv_nsec = 10000000L;
    uint64_t start = read_counter();
    nanosleep(&request, &remainder);
    uint64_t end = read_counter();
    cycles_per_second = (end - start) * 100;
}

static double get_time() {
    return (double)(read_counter()) / cycles_per_second;
}
