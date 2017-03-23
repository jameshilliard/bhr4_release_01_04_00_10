/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#ifndef _dllist_h_
#define _dllist_h_

/* dllist_t is defined in config.h */
typedef int (*dlcomp_func) (dllist_t key1, dllist_t key2);
typedef void (*dlfree_func) (dllist_t key_data);

typedef struct _dllist dllist;

struct _dllist
{
  dllist_t data;
  dllist *next;
  dllist *prev;
};


extern dllist *dllist_new_entry(dllist_t);
extern dllist *dllist_append(dllist *, dllist_t);
extern dllist *dllist_prepend(dllist *, dllist_t);
extern dllist *dllist_insert(dllist *, int, dllist_t);
extern dllist *dllist_insert_after(dllist *, dllist *, dllist_t);
extern dllist *dllist_insert_before(dllist *, dllist *, dllist_t);
extern dllist *dllist_insert_list_after(dllist *, dllist *, dllist *);
extern dllist *dllist_first(dllist *);
extern dllist *dllist_last(dllist *);
extern dllist *dllist_nth(dllist *, int);
extern dllist *dllist_find(dllist *, dllist_t);
extern dllist *dllist_find2(dllist *, dllist_t , dlcomp_func);
extern unsigned int dllist_count(dllist *);
extern dllist *dllist_remove_entry(dllist *, dllist *);
extern dllist *dllist_remove_data(dllist *, dllist_t);
extern void dllist_free_all(dllist *);
extern void dllist_free_all_custom(dllist *, dlfree_func);
extern dllist *dllist_concat(dllist *, dllist *);

#endif
