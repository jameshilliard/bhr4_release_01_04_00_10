/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "config.h"
#include "dlhash.h"

dlhash *dlhash_new(unsigned int size, dlkey_func key_func,
  dlhash_func hash_func, dlcomp_func comp_func)
{
  dlhash *retv = malloc(sizeof(dlhash));
  assert(retv != NULL);

  retv->size = size;
  retv->key_func = key_func;
  retv->hash_func = hash_func;
  retv->comp_func = comp_func;
  retv->free_func = NULL;
  retv->keyfree_func = NULL;
  retv->nodes = (dllist **) calloc(size, sizeof(dllist *));
  assert(retv->nodes != NULL);
  memset(retv->nodes, '\0', size * sizeof(dllist *));

  return retv;
}

void dlhash_set_free_func(dlhash * hash, dlfree_func free_func,
  dlkeyfree_func keyfree_func)
{
  hash->free_func = free_func;
  hash->keyfree_func = keyfree_func;
}

void dlhash_empty(dlhash * hash)
{
  unsigned int i = 0;

  for(i = 0; i < hash->size; i++)
  {
    while(hash->nodes[i])
    {
      if(hash->free_func)
        hash->free_func(hash->nodes[i]->data);
      hash->nodes[i] = dllist_remove_entry(hash->nodes[i], hash->nodes[i]);
    }
  }
}

void dlhash_free(dlhash * hash)
{
  dlhash_empty(hash);
  free(hash->nodes);
  free(hash);
}

void dlhash_insert(dlhash * hash, dllist_t key_data)
{
  dllist_t key;
  unsigned int key_class;

  key = hash->key_func(key_data);
  key_class = hash->hash_func(hash->size, key);

  hash->nodes[key_class] = dllist_append(hash->nodes[key_class], key_data);

  if(hash->keyfree_func)
    hash->keyfree_func(key);
}

void dlhash_remove(dlhash * hash, dllist_t key_data)
{
  dllist_t key1, key2;
  unsigned int key_class;
  dllist *ptr;

  key1 = hash->key_func(key_data);
  key_class = hash->hash_func(hash->size, key1);

  ptr = hash->nodes[key_class];
  while(ptr)
  {
    key2 = hash->key_func(ptr->data);
    if(hash->comp_func(key1, key2))
    {
      dllist *tptr;

      if(hash->free_func)
        hash->free_func(ptr->data);
      tptr = ptr->next;
      hash->nodes[key_class] =
        dllist_remove_entry(hash->nodes[key_class], ptr);
      ptr = tptr;
    }
    else
      ptr = ptr->next;

    if(hash->keyfree_func)
      hash->keyfree_func(key2);
  }

  if(hash->keyfree_func)
    hash->keyfree_func(key1);
}

void dlhash_exclude(dlhash * hash, dllist_t key_data)
{
  dllist_t key1, key2;
  unsigned int key_class;
  dllist *ptr;

  key1 = hash->key_func(key_data);
  key_class = hash->hash_func(hash->size, key1);

  ptr = hash->nodes[key_class];
  while(ptr)
  {
    key2 = hash->key_func(ptr->data);

    if(hash->comp_func(key1, key2))
    {
      dllist *tptr;

      tptr = ptr->next;
      hash->nodes[key_class] =
        dllist_remove_entry(hash->nodes[key_class], ptr);
      ptr = tptr;
    }
    else
      ptr = ptr->next;

    if(hash->keyfree_func)
      hash->keyfree_func(key2);
  }

  if(hash->keyfree_func)
    hash->keyfree_func(key1);
}

dllist *dlhash_get_class(dlhash * hash, dllist_t key_data)
{
  dllist_t key;
  unsigned int key_class;

  key = hash->key_func(key_data);
  key_class = hash->hash_func(hash->size, key);

  if(hash->keyfree_func)
    hash->keyfree_func(key);

  return hash->nodes[key_class];
}

void dlhash_exclude_exact(dlhash * hash, dllist_t key_data)
{
  dllist_t key;
  unsigned int key_class;
  dllist *ptr;

  key = hash->key_func(key_data);
  key_class = hash->hash_func(hash->size, key);

  if(hash->keyfree_func)
    hash->keyfree_func(key);

  if((ptr = dllist_find(hash->nodes[key_class], key_data)))
    hash->nodes[key_class] = dllist_remove_entry(hash->nodes[key_class], ptr);
}

dllist_t dlhash_find_by_key(dlhash * hash, dllist_t key1)
{
  dllist_t key2;
  dllist_t retv = 0;
  unsigned int key_class;
  dllist *ptr;

  key_class = hash->hash_func(hash->size, key1);

  ptr = hash->nodes[key_class];

  while(ptr)
  {
    key2 = hash->key_func(ptr->data);

    if(hash->comp_func(key1, key2))
    {
      retv = ptr->data;
      if(hash->keyfree_func)
        hash->keyfree_func(key2);
      break;
    }
    ptr = ptr->next;

    if(hash->keyfree_func)
      hash->keyfree_func(key2);
  }

  return retv;
}

dllist_t dlhash_find(dlhash * hash, dllist_t key_data)
{
  dllist_t key;
  dllist_t retv = 0;

  key = hash->key_func(key_data);

  retv = dlhash_find_by_key(hash, key);

  if(hash->keyfree_func)
    hash->keyfree_func(key);

  return retv;
}

void dlhash_resize(dlhash * hash, unsigned int new_size)
{
  unsigned int i, old_size;
  dllist **old_nodes;

  if(hash->size == new_size)
    return;

  old_nodes = hash->nodes;
  old_size = hash->size;

  hash->size = new_size;
  hash->nodes = (dllist **) calloc(new_size, sizeof(dllist *));
  assert(hash->nodes != NULL);
  memset(hash->nodes, '\0', new_size * sizeof(dllist *));

  for(i = 0; i < old_size; i++)
  {
    while(old_nodes[i])
    {
      dlhash_insert(hash, old_nodes[i]->data);
      old_nodes[i] = dllist_remove_entry(old_nodes[i], old_nodes[i]);
    }
  }
  free(old_nodes);
}
