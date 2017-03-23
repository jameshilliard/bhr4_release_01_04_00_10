/**
 * Copyright (c) 2015 GreenWave Systems.
 *
 * Author: Greenwave Systems
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef GWS_MEMSET_S_H
#define GWS_MEMSET_S_H

#if defined(__STDC_LIB_EXT1__)
  #if (__STDC_LIB_EXT1__ >= 201112L)
    #define USE_EXT1 1
    #define __STDC_WANT_LIB_EXT1__ 1 /* Want the ext1 functions */
  #endif
#endif

#include <linux/string.h>

#if !defined(USE_EXT1)

/*
 * Note: should be removed when C11 is supported
 *
 * Copies 'c' into each of the first 'n' characters of the object pointed to by 'v'.
 * v pointer to the object to fill
 * smax size of the destination
 * c fill byte
 * n number of bytes to fill
 */
int memset_s(void *v, size_t smax, int c, size_t n);

#endif // USE_EXT1

#endif /* GWS_MEMSET_S_H */
