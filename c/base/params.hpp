
#pragma once

#undef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <stddef.h>


// basic options

#define DEBUG_LEVEL 6


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
