/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include <stdlib.h>

#include "config.h"
#include "dllist.h"

dllist *dllist_new_entry(dllist_t data)
{
  dllist *retv = (dllist *) malloc(sizeof(dllist));

  if(retv)
  {
    retv->data = data;
    retv->next = NULL;
    retv->prev = NULL;
  }

  return retv;
}

dllist *dllist_append(dllist * list, dllist_t data)
{
  dllist *nentry = dllist_new_entry(data);

  if(list)
  {
    dllist *last = dllist_last(list);

    last->next = nentry;
    nentry->prev = last;

    return list;
  }
  else
    return nentry;

}

dllist *dllist_prepend(dllist * list, dllist_t data)
{
  dllist *nentry = dllist_new_entry(data);

  if(list)
  {
    dllist *first = dllist_first(list);

    first->prev = nentry;
    nentry->next = first;
  }

  return nentry;
}

dllist *dllist_insert(dllist * list, int position, dllist_t data)
{
  if(!list)
    return dllist_new_entry(data);

  if(position <= 0)
    return dllist_prepend(list, data);
  else
  {
    dllist *nth = dllist_nth(list, position);

    if(!nth)
      return dllist_append(list, data);
    else
    {
      dllist *prev_nth;
      dllist *nentry = dllist_new_entry(data);

      prev_nth = nth->prev;
      nentry->next = nth;
      nth->prev = nentry;

      if(prev_nth)
      {
        prev_nth->next = nentry;
        nentry->prev = prev_nth;

        return list;
      }
      else
        return nentry;
    }
  }
}

dllist *dllist_insert_after(dllist * list, dllist * node, dllist_t data)
{
  if(!list)
    return dllist_new_entry(data);

  if(!node)
    return dllist_append(list, data);
  else
  {
    dllist *next_node;
    dllist *nentry = dllist_new_entry(data);

    next_node = node->next;
    nentry->prev = node;
    nentry->next = next_node;

    if(next_node)
      next_node->prev = nentry;

    return list;
  }
}

dllist *dllist_insert_before(dllist * list, dllist * node, dllist_t data)
{
  if(!list)
    return dllist_new_entry(data);

  if(!node)
    return dllist_append(list, data);

  if(!node->prev)
    return dllist_prepend(list, data);
  else
  {
    dllist *prev_node;
    dllist *nentry = dllist_new_entry(data);

    prev_node = node->prev;
    prev_node->next = nentry;
    nentry->next = node;
    nentry->prev = prev_node;
    node->prev = nentry;

    return list;
  }
}

dllist *dllist_insert_list_after(dllist * list1, dllist * node,
  dllist * list2)
{
  dllist *last;

  if(!list1)
    return list2;

  last = dllist_last(list2);

  if(!last)
    return list1;

  if(!node)
  {
    last->next = list1;
    list1->prev = last;

    return list2;
  }
  else
  {
    dllist *next_node;

    next_node = node->next;
    if(next_node)
      next_node->prev = last;
    last->next = next_node;
    node->next = list2;
    list2->prev = node;

    return list1;
  }
}

dllist *dllist_first(dllist * list)
{
  if(list)
    while(list->prev)
      list = list->prev;

  return list;
}

dllist *dllist_last(dllist * list)
{
  if(list)
    while(list->next)
      list = list->next;

  return list;
}


dllist *dllist_nth(dllist * list, int n)
{
  while(n > 0 && list)
  {
    n--;
    list = list->next;
  }

  return list;
}

dllist *dllist_find(dllist * list, dllist_t data)
{
  while(list)
  {
    if(list->data == data)
    {
      break;
    }
    list = list->next;
  }

  return list;
}

dllist *dllist_find2(dllist * list, dllist_t data, dlcomp_func comp_func)
{
  while(list)
  {
    if(comp_func(list->data, data))
    {
      break;
    }
    list = list->next;
  }

  return list;
}

unsigned int dllist_count(dllist * list)
{
  unsigned int retv = 0;

  while(list)
  {
    retv++;
    list = list->next;
  }

  return retv;
}

dllist *dllist_remove_entry(dllist * list, dllist * entry)
{
  if(entry)
  {
    dllist *next_node, *prev_node;

    next_node = entry->next;
    prev_node = entry->prev;

    if(prev_node)
      prev_node->next = next_node;
    if(next_node)
      next_node->prev = prev_node;

    if(entry == list)
      list = next_node;

    free(entry);
  }

  return list;
}

dllist *dllist_remove_data(dllist * list, dllist_t data)
{
  dllist *pom;

  while((pom = dllist_find(list, data)))
  {
    list = dllist_remove_entry(list, pom);
  }

  return list;
}

void dllist_free_all(dllist * list)
{
  dllist *pom;

  while(list)
  {
    pom = list;
    list = list->next;

    free(pom);
  }
}

void dllist_free_all_custom(dllist * list, dlfree_func free_func)
{
  dllist *pom;

  while(list)
  {
    pom = list;
    list = list->next;

    free_func(pom->data);
    free(pom);
  }
}

dllist *dllist_concat(dllist * list1, dllist * list2)
{
  dllist *retv = list1;

  if(list2)
  {
    dllist *last = dllist_last(list1);

    if(last)
      last->next = list2;
    else
      retv = list2;

    list2->prev = last;
  }

  return retv;
}

