
#pragma once

#undef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <stddef.h>
#include <stdint.h>


// basic options

// debugging level
#ifndef DEBUG_LEVEL
#define DEBUG_LEVEL 6
#endif

// maximum number of preprocessing iterations
static constexpr uintmax_t MAX_PREPROCESSING_ITERATIONS = UINTMAX_C(4096);

// for VLS backend, how often to check the time
static constexpr const uintmax_t CHECK_TIME_EVERY_THIS_MANY_BRANCHES = UINTMAX_C(65536);



// settings for the grid cache

// minimum cache size (per grid size)
static constexpr size_t GRID_CACHE_BASE_SIZE = 16UZ;
// maximum cache size (per grid size)
static constexpr size_t GRID_CACHE_MAX_SIZE = 256UZ;
// determines how fast the cache grows
// higher numbers - slower, lower numbers - faster
static constexpr double GRID_CACHE_SIZE_SCALING = 10;
// max allocated memory (default: 10 megabytes aka `10 * (1UZ << 20)`)
static constexpr size_t GRID_CACHE_MAX_MEMORY = 10 * (1UZ << 20);
