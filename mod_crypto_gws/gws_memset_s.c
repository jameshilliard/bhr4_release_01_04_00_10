/**
 * Copyright (c) 2015 GreenWave Systems.
 *
 * Author: Greenwave Systems
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifdef __KERNEL__
#include <linux/kernel.h>
#endif
#include <linux/errno.h>

#include "gws_memset_s.h"

#if !defined(USE_EXT1)

int memset_s(void *v, size_t smax, int c, size_t n) {
  if (v == NULL) return EINVAL;
  if (smax > SIZE_MAX) return EINVAL;
  if (n > smax) return EINVAL;

  volatile unsigned char *p = v;
  while (smax-- && n--) {
    *p++ = c;
  }

  return 0;
}

#endif // USE_EXT1
