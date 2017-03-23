/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include <string.h>

#include "config.h"
#include "dlhash.h"
#include "dlhash_tools.h"
#include "url.h"

unsigned int str_hash_func(unsigned int size, dllist_t key)
{
  unsigned char *p = (unsigned char *) key;
  unsigned int retv = 0;

  while(*p)
  {
    retv = (retv + *p) % size;
    p++;
  }

  return retv;
}

int str_comp_func(dllist_t key1, dllist_t key2)
{
  return (!strcmp((void *)key1, (void *)key2));
}

dllist_t url_key_func(dllist_t key_data)
{
  return key_data;
}

unsigned int url_hash_func(unsigned int size, dllist_t key)
{
  url *purl = (url *) key;
  unsigned int retv = 0;
  unsigned char *p;
  unsigned int i = 0;

  p = (unsigned char *) url_get_path(purl);

  while(*p)
  {
    retv = (retv + i * (unsigned int) *p) % size;
    p++;
    i++;
  }

  p = (unsigned char *) url_get_search_str(purl);

  if(p)
  {
    while(*p)
    {
      retv = (retv + i * (unsigned int) *p) % size;
      p++;
      i++;
    }
  }

  return retv;
}

dllist_t fn_key_func(dllist_t key_data)
{
  url *keyd = (url *) key_data;

  return (dllist_t) url_to_filename(keyd, TRUE);
}

void url_free_func(dllist_t key_data)
{
  free_deep_url((url *) key_data);
  free((url *) key_data);
}
