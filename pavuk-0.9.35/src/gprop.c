/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include "config.h"
#ifdef GTK_FACE
#include <glib.h>
#include <stdlib.h>
#include <string.h>

#include "dllist.h"
#include "gprop.h"
#include "tools.h"

static struct
{
  char *id;
  int type;
} typetab[] =
{
  {"str.", GPROP_STR},
  {"int.", GPROP_INT},
  {"bool.", GPROP_BOOL}
};

static dllist *gprop_list = NULL;

gprop *gprop_parse(char *str)
{
  gprop *gp;
  int i;
  char *p = NULL;

  gp = _malloc(sizeof(gprop));

  for(i = 0; i < (sizeof(typetab) / sizeof(typetab[0])); i++)
  {
    if(!strncmp(str, typetab[i].id, strlen(typetab[i].id)))
    {
      gp->type = typetab[i].type;
      p = str + strlen(typetab[i].id);
    }
  }

  if(!p)
  {
    _free(gp);
    return NULL;
  }

  i = strcspn(p, "=");

  if(!i)
  {
    _free(gp);
    return NULL;
  }

  gp->name = tl_strndup(p, i);

  p += i;
  p += strspn(p, "=");

  switch (gp->type)
  {
  case GPROP_STR:
    gp->value = g_strdup(p);
    break;
  case GPROP_INT:
    gp->value = (void *) atoi(p);
    break;
  case GPROP_BOOL:
    gp->value = (void *) (!strcmp(p, "true"));
    break;
  }

  return gp;
}

char *gprop_dump(gprop * gp)
{
  static char pom[256];
  char *p = pom;

  switch (gp->type)
  {
  case GPROP_STR:
    snprintf(pom, sizeof(pom), "%s%s=%s", typetab[gp->type].id,
      gp->name, (char *) gp->value);
    break;
  case GPROP_INT:
    snprintf(pom, sizeof(pom), "%s%s=%d", typetab[gp->type].id, gp->name, (int) gp->value);
    break;
  case GPROP_BOOL:
    snprintf(pom, sizeof(pom), "%s%s=%s", typetab[gp->type].id,
      gp->name, gp->value ? "true" : "false");
    break;
  }

  return p;
}

void gprop_add(gprop * gp)
{
  gprop_list = dllist_append(gprop_list, (dllist_t) gp);
}


static int gprop_find(dllist_t gp, dllist_t name)
{
  return !strcmp((char *) name, ((gprop *)gp)->name);
}

void gprop_set_str(char *name, char *value)
{
  dllist *node;
  gprop *gp;

  node = dllist_find2(gprop_list, (dllist_t) name, gprop_find);

  if(node)
  {
    gp = (gprop *) node->data;

    g_free(gp->value);
    gp->type = GPROP_STR;
    gp->value = tl_strdup(value);
  }
  else
  {
    gp = _malloc(sizeof(gprop));

    gp->type = GPROP_STR;
    gp->name = tl_strdup(name);
    gp->value = tl_strdup(value);

    gprop_list = dllist_append(gprop_list, (dllist_t) gp);
  }
}

void gprop_set_int(char *name, int value)
{
  dllist *node;
  gprop *gp;

  node = dllist_find2(gprop_list, (dllist_t) name, gprop_find);

  if(node)
  {
    gp = (gprop *) node->data;

    gp->type = GPROP_INT;
    gp->value = (void *) value;
  }
  else
  {
    gp = _malloc(sizeof(gprop));

    gp->type = GPROP_INT;
    gp->name = tl_strdup(name);
    gp->value = (void *) value;

    gprop_list = dllist_append(gprop_list, (dllist_t) gp);
  }
}

void gprop_set_bool_t(char *name, int value)
{
  dllist *node;
  gprop *gp;

  node = dllist_find2(gprop_list, (dllist_t) name, gprop_find);

  if(node)
  {
    gp = (gprop *) node->data;

    gp->type = GPROP_BOOL;
    gp->value = (void *) value;
  }
  else
  {
    gp = _malloc(sizeof(gprop));

    gp->type = GPROP_BOOL;
    gp->name = tl_strdup(name);
    gp->value = (void *) value;

    gprop_list = dllist_append(gprop_list, (dllist_t) gp);
  }
}

int gprop_get_str(char *name, char **valp)
{
  dllist *node;
  gprop *gp;

  node = dllist_find2(gprop_list, (dllist_t) name, gprop_find);

  if(node)
  {
    gp = (gprop *) node->data;

    *valp = (char *) gp->value;
  }

  return (node != NULL);
}

int gprop_get_int(char *name, int *valp)
{
  dllist *node;
  gprop *gp;

  node = dllist_find2(gprop_list, (dllist_t) name, gprop_find);

  if(node)
  {
    gp = (gprop *) node->data;

    *valp = (int) gp->value;
  }

  return (node != NULL);
}

int gprop_get_bool_t(char *name, int *valp)
{
  dllist *node;
  gprop *gp;

  node = dllist_find2(gprop_list, (dllist_t) name, gprop_find);

  if(node)
  {
    gp = (gprop *) node->data;

    *valp = (int) gp->value;
  }

  return (node != NULL);
}

void gprop_save(FILE * f)
{
  dllist *lst = gprop_list;
  char *p;

  while(lst)
  {
    p = (char *) gprop_dump((gprop *) lst->data);
    if(p)
    {
      fprintf(f, "Property: %s\n", p);
    }
    lst = lst->next;
  }
}
#endif
