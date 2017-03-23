/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#ifndef _dlhash_h_
#define _dlhash_h_

#include "dllist.h"

typedef unsigned int (*dlhash_func) (unsigned int, dllist_t key);
typedef dllist_t (*dlkey_func) (dllist_t key_data);
typedef void (*dlkeyfree_func) (dllist_t key);

typedef struct _dlhash
{
  unsigned int size;
  dllist **nodes;
  dlkey_func key_func;
  dlkeyfree_func keyfree_func;
  dlhash_func hash_func;
  dlcomp_func comp_func;
  dlfree_func free_func;
} dlhash;

extern dlhash *dlhash_new(unsigned int, dlkey_func, dlhash_func, dlcomp_func);
extern void dlhash_set_free_func(dlhash *, dlfree_func, dlkeyfree_func);
extern void dlhash_empty(dlhash *);
extern void dlhash_free(dlhash *);
extern void dlhash_insert(dlhash *, dllist_t key_data);
extern void dlhash_remove(dlhash *, dllist_t key_data);
extern void dlhash_exclude(dlhash *, dllist_t key_data);
extern void dlhash_exclude_exact(dlhash *, dllist_t key_data);
extern dllist *dlhash_get_class(dlhash *, dllist_t key_data);
extern dllist_t dlhash_find(dlhash *, dllist_t key_data);
extern dllist_t dlhash_find_by_key(dlhash *, dllist_t key);
extern void dlhash_resize(dlhash *, unsigned int);

#endif
