/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef GTK_FACE
#include <gtk/gtk.h>

#include "icons/cancel.xpm"
#include "icons/load.xpm"
#include "icons/restart_small.xpm"
#endif

#include "ainterface.h"
#include "form.h"
#include "gui.h"
#include "html.h"
#include "tools.h"

#define NARAZNIK 512
/* FIXME: Translate me (NARAZNIK)! */

#define SEXPAND(sv) \
  if((ssz - (sv + sr + NARAZNIK)) < 0) \
  { \
    stack = realloc(stack, ssz + sv  + NARAZNIK); \
    ssz += NARAZNIK + sv; \
  }

#define TEXPAND(sv) \
  if((tsz - (sv + tr + NARAZNIK)) < 0) \
  { \
    text = realloc(text, tsz + sv  + NARAZNIK); \
    tsz += NARAZNIK + sv; \
  }

char *form_get_text(int num, char *html_text, int html_len, int *form_len)
{
  char *stack = NULL;
  int i, sr, ssz;
  int formstart = FALSE;
  int commentstart = FALSE;
  int scriptstart = FALSE;
  int formnum = 0;

  ssz = 0;
  sr = 0;

  for(i = 0; i < html_len; i++)
  {
    if(commentstart)
    {
      if(!strncmp(html_text + i, "-->", 3))
      {
        commentstart = FALSE;
        i += 2;
      }
      continue;
    }
    if(scriptstart)
    {
      if(!strncasecmp(html_text + i, "</SCRIPT>", 9))
      {
        scriptstart = FALSE;
        i += 8;
      }
      continue;
    }
    if(html_text[i] == '<')
    {
      if(!strncasecmp(html_text + i + 1, "SCRIPT", 6) &&
        (i + 7) < html_len &&
        (tl_ascii_isspace(html_text[i + 7]) || html_text[i + 7] == '>'))
      {
        scriptstart = TRUE;
        continue;
      }
      else if(!strncasecmp(html_text + i + 1, "!--", 3))
      {
        commentstart = TRUE;
        continue;
      }
    }
    if(formstart)
    {
      if(!strncasecmp(html_text + i, "</FORM>", 7))
      {
        stack[sr] = '\0';
        break;
      }
      else
      {
        SEXPAND(1);
        stack[sr] = html_text[i];
        sr++;
      }
      continue;
    }
    if(!strncasecmp(html_text + i, "<FORM", 5) &&
      (i + 5) < html_len && tl_ascii_isspace(html_text[i + 5]))
    {
      if(formnum == num)
      {
        formstart = TRUE;
        ssz = 2 * NARAZNIK;
        stack = malloc(ssz);
        sr = 0;
        stack[sr] = html_text[i];
        sr++;
      }
      formnum++;
    }
  }
  *form_len = sr;
  return stack;
}

int form_field_compare(dllist_t dl1, dllist_t dl2)
{
  form_field * ff1 = (form_field *)dl1;
  form_field * ff2 = (form_field *)dl2;

  if(strcmp(ff1->name, ff2->name))
    return FALSE;

  if(strcmp(ff1->value, ff2->value))
    return FALSE;

  return TRUE;
}

int form_field_compare_name(dllist_t dl1, dllist_t dl2)
{
  form_field * ff1 = (form_field *)dl1;
  form_field * ff2 = (form_field *)dl2;

  if(!ff1->name || !ff2->name)
    return FALSE;

  if(strcmp(ff1->name, ff2->name))
    return FALSE;

  return TRUE;
}


form_field *form_field_new(char *name, char *value)
{
  form_field *rv;

  rv = _malloc(sizeof(form_field));

  rv->type = FORM_T_TEXT;
  rv->name = tl_strdup(name);
  rv->value = tl_strdup(value);

  return rv;
}

form_field *form_field_duplicate(form_field * ff)
{
  form_field *rv;

  rv = _malloc(sizeof(form_field));

  rv->type = ff->type;
  rv->name = tl_strdup(ff->name);
  rv->value = tl_strdup(ff->value);

  return rv;
}

static form_field_types form_input_type(char *str)
{
  int i;
  struct
  {
    char *str;
    form_field_types id;
  } it[] =
  {
    {"text", FORM_T_TEXT},
    {"password", FORM_T_PASSWORD},
    {"checkbox", FORM_T_CHECKBOX},
    {"radio", FORM_T_RADIO},
    {"submit", FORM_T_SUBMIT},
    {"reset", FORM_T_RESET},
    {"file", FORM_T_FILE},
    {"hidden", FORM_T_HIDDEN},
    {"image", FORM_T_IMAGE},
    {"button", FORM_T_BUTTON}
  };

  for(i = 0; i < (sizeof(it) / sizeof(it[0])); i++)
  {
    if(!strcasecmp(str, it[i].str))
      return it[i].id;
  }

  return FORM_T_UNKNOWN;
}

static form_field_info *form_field_info_new(void)
{
  form_field_info *retv = malloc(sizeof(form_field_info));

  retv->type = FORM_T_TEXT;
  retv->name = NULL;
  retv->default_value = NULL;
  retv->text = NULL;
  retv->width = 0;
  retv->height = 0;
  retv->maxlen = 0;
  retv->readonly = FALSE;
  retv->checked = FALSE;
  retv->disabled = FALSE;
  retv->multiple = FALSE;
  retv->selected = FALSE;
  retv->infos = NULL;
#if GTK_FACE
  retv->rg = NULL;
  retv->widget = NULL;
  retv->idx = -1;
#endif

  return retv;
}

static form_field_info *form_parse_inputtag(char *tag)
{
  char *p;
  form_field_info *retv;

  retv = form_field_info_new();

  if((p = html_get_attrib_from_tag(tag, "type")))
  {
    retv->type = form_input_type(p);
    free(p);
  }

  if((p = html_get_attrib_from_tag(tag, "size")))
  {
    retv->width = atoi(p);
    free(p);
  }

  if((p = html_get_attrib_from_tag(tag, "maxlength")))
  {
    retv->maxlen = atoi(p);
    free(p);
  }

  retv->name = html_get_attrib_from_tag(tag, "name");
  retv->default_value = html_get_attrib_from_tag(tag, "value");

  retv->readonly = html_tag_co_elem(tag, "readonly");
  retv->checked = html_tag_co_elem(tag, "checked");
  retv->disabled = html_tag_co_elem(tag, "disabled");

  return retv;
}

static form_field_info *form_parse_buttontag(char *tag)
{
  char *p;
  form_field_info *retv;

  retv = form_field_info_new();
  retv->type = FORM_T_BUTTON;

  if((p = html_get_attrib_from_tag(tag, "type")))
  {
    retv->type = form_input_type(p);
    free(p);
  }

  retv->name = html_get_attrib_from_tag(tag, "name");
  retv->default_value = html_get_attrib_from_tag(tag, "value");

  retv->disabled = html_tag_co_elem(tag, "disabled");

  return retv;
}

static form_field_info *form_parse_selecttag(char *tag)
{
  char *p;
  form_field_info *retv;

  retv = form_field_info_new();
  retv->type = FORM_T_SELECT;

  retv->name = html_get_attrib_from_tag(tag, "name");

  if((p = html_get_attrib_from_tag(tag, "size")))
  {
    retv->height = atoi(p);
    free(p);
  }

  retv->disabled = html_tag_co_elem(tag, "disabled");
  retv->multiple = html_tag_co_elem(tag, "multiple");

  return retv;
}

static form_field_info *form_parse_optgrouptag(char *tag)
{
  form_field_info *retv;

  retv = form_field_info_new();
  retv->type = FORM_T_OPTGROUP;

  retv->disabled = html_tag_co_elem(tag, "disabled");
  retv->text = html_get_attrib_from_tag(tag, "label");

  return retv;
}

static form_field_info *form_parse_optiontag(char *tag)
{
  form_field_info *retv;

  retv = form_field_info_new();
  retv->type = FORM_T_OPTION;

  retv->disabled = html_tag_co_elem(tag, "disabled");
  retv->selected = html_tag_co_elem(tag, "selected");
  retv->text = html_get_attrib_from_tag(tag, "label");

  retv->default_value = html_get_attrib_from_tag(tag, "value");

  return retv;
}

static form_field_info *form_parse_textareatag(char *tag)
{
  char *p;
  form_field_info *retv;

  retv = form_field_info_new();
  retv->type = FORM_T_TEXTAREA;

  if((p = html_get_attrib_from_tag(tag, "cols")))
  {
    retv->width = _atoi(p);
    free(p);
  }

  if((p = html_get_attrib_from_tag(tag, "rows")))
  {
    retv->height = _atoi(p);
    free(p);
  }

  retv->name = html_get_attrib_from_tag(tag, "name");

  retv->disabled = html_tag_co_elem(tag, "disabled");
  retv->readonly = html_tag_co_elem(tag, "readonly");

  return retv;
}

static form_info *form_parse_formtag(char *tag)
{
  char *p;
  form_info *retv = malloc(sizeof(form_info));

  retv->method = FORM_M_GET;
  retv->encoding = FORM_E_URLENCODED;
  retv->action = NULL;
  retv->infos = NULL;
  retv->text = NULL;
  retv->parent_url = NULL;

  if((p = html_get_attrib_from_tag(tag, "method")))
  {
    if(!strcasecmp(p, "GET"))
      retv->method = FORM_M_GET;
    else if(!strcasecmp(p, "POST"))
      retv->method = FORM_M_POST;
    else
      retv->method = FORM_M_UNKNOWN;

    free(p);
  }

  if((p = html_get_attrib_from_tag(tag, "enctype")))
  {
    if(!strcasecmp(p, "multipart/form-data"))
      retv->encoding = FORM_E_MULTIPART;
    else if(!strcasecmp(p, "application/x-www-form-urlencoded"))
      retv->encoding = FORM_E_URLENCODED;
    else
      retv->encoding = FORM_E_UNKNOWN;

    _free(p);
  }

  retv->action = html_get_attrib_from_tag(tag, "action");

  return retv;
}

static void form_field_info_free(form_field_info * ffi)
{
  dllist *ptr;

  _free(ffi->name);
  _free(ffi->default_value);
  _free(ffi->text);

  ptr = ffi->infos;
  while(ptr)
  {
    form_field_info_free((form_field_info *) ptr->data);
    ptr = dllist_remove_entry(ptr, ptr);
  }
  free(ffi);
}

void form_free(form_info * formi)
{
  dllist *ptr;

  _free(formi->action);
  _free(formi->text);

  ptr = formi->infos;
  while(ptr)
  {
    form_field_info_free((form_field_info *) ptr->data);
    ptr = dllist_remove_entry(ptr, ptr);
  }
  free(formi);
}

form_info *form_info_dup(form_info * formi)
{
  dllist *ptr;
  form_info *retv = malloc(sizeof(form_info));

  retv->method = formi->method;
  retv->encoding = formi->encoding;
  retv->action = tl_strdup(formi->action);
  retv->text = NULL;
  retv->parent_url = NULL;
  retv->infos = NULL;

  for(ptr = formi->infos; ptr; ptr = ptr->next)
  {
    retv->infos = dllist_append(retv->infos,
    (dllist_t) form_field_duplicate((form_field *)ptr->data));
  }

  return retv;
}

form_info *form_parse(char *form_text, int form_len)
{
  char *stack;
  int i, sr, ssz;
  int tagstart = FALSE;
  form_info *retv = NULL;
  form_field_info *selectgroup = NULL;
  dllist *optgroups = NULL;
  form_field_info *optgroup = NULL;
  form_field_info *lastformtag = NULL;
  form_field_info *plastformtag = NULL;
  int tsz, tr;
  char *text;
  bool_t fstag;
  char *sp;

  ssz = 2 * NARAZNIK;
  stack = malloc(ssz);
  sr = 0;

  tsz = 2 * NARAZNIK;
  text = malloc(tsz);
  tr = 0;

  for(i = 0; i < form_len; i++)
  {
    if(form_text[i] == '<')
    {
      tagstart = TRUE;
      sr = 0;
    }
    else if(form_text[i] == '>' && tagstart)
    {
      tagstart = FALSE;
      stack[sr] = '\0';
      fstag = FALSE;

#define IS_TAG(s) (!strncasecmp(stack, s, strlen(s)) && \
      sr >= strlen(s) && \
      (tl_ascii_isspace(stack[strlen(s)]) || \
       stack[strlen(s)] == '\0'))

      if(IS_TAG("FORM"))
      {
        retv = form_parse_formtag(stack);
      }
      else if(retv && IS_TAG("INPUT"))
      {
        fstag = TRUE;
        plastformtag = lastformtag;
        lastformtag = form_parse_inputtag(stack);
        retv->infos = dllist_append(retv->infos, (dllist_t)lastformtag);
      }
      else if(retv && lastformtag && IS_TAG("/INPUT"))
      {
        if(tr)
        {
          text[tr] = '\0';
          sp = text;
          while(*sp && tl_ascii_isspace(*sp))
            sp++;
          if(!lastformtag->text && *sp)
          {
            int l = strcspn(sp, "\r\n");
            lastformtag->text = tl_strndup(sp, l);
            memmove(text, sp + l, strlen(sp + l + 1));
            tr = strlen(text);
          }
          else
            tr = 0;
        }
      }
      else if(retv && IS_TAG("BUTTON"))
      {
        fstag = TRUE;

        plastformtag = lastformtag;
        lastformtag = form_parse_buttontag(stack);
        retv->infos = dllist_append(retv->infos, (dllist_t)lastformtag);
      }
      else if(retv && lastformtag && IS_TAG("/BUTTON"))
      {
        if(tr)
        {
          text[tr] = '\0';
          sp = text;
          while(*sp && tl_ascii_isspace(*sp))
            sp++;
          if(!lastformtag->text && *sp)
          {
            int l = strcspn(sp, "\r\n");
            lastformtag->text = tl_strndup(sp, l);
            memmove(text, sp + l, strlen(sp + l + 1));
            tr = strlen(text);
          }
          else
            tr = 0;
        }
      }
      else if(retv && IS_TAG("SELECT"))
      {
        fstag = TRUE;

        plastformtag = lastformtag;
        lastformtag = form_parse_selecttag(stack);
        selectgroup = lastformtag;
        retv->infos = dllist_append(retv->infos, (dllist_t)lastformtag);
      }
      else if(selectgroup && IS_TAG("/SELECT"))
      {
        selectgroup = NULL;
      }
      else if(selectgroup && IS_TAG("OPTGROUP"))
      {
        fstag = TRUE;
        plastformtag = lastformtag;
        lastformtag = form_parse_optgrouptag(stack);
        optgroup = lastformtag;
        optgroups = dllist_prepend(optgroups, (dllist_t) lastformtag);
        selectgroup->infos = dllist_append(selectgroup->infos,
        (dllist_t) lastformtag);
      }
      else if(optgroups && IS_TAG("/OPTGROUP"))
      {
        optgroups = dllist_remove_entry(optgroups, optgroups);
        if(optgroups)
          optgroup = (form_field_info *) optgroups->data;
        else
          optgroup = NULL;
      }
      else if(selectgroup && IS_TAG("OPTION"))
      {
        fstag = TRUE;

        plastformtag = lastformtag;
        lastformtag = form_parse_optiontag(stack);
        lastformtag->multiple = selectgroup->multiple;
        if(optgroup)
        {
          optgroup->infos = dllist_append(optgroup->infos,
          (dllist_t)lastformtag);
        }
        else
        {
          selectgroup->infos = dllist_append(selectgroup->infos,
          (dllist_t) lastformtag);
        }
      }
      else if(selectgroup && lastformtag && IS_TAG("/OPTION"))
      {
        if(tr && lastformtag->type == FORM_T_OPTION)
        {
          text[tr] = '\0';
          sp = text;
          while(*sp && tl_ascii_isspace(*sp))
            sp++;
          if(!lastformtag->text && *sp)
          {
            int l = strcspn(sp, "\r\n");
            lastformtag->text = tl_strndup(sp, l);
            memmove(text, sp + l, strlen(sp + l + 1));
            tr = strlen(text);
          }
          else
            tr = 0;
          /*
             <OPTION VALUE="somevalue">sometext</OPTION>
             <OPTION>somevalue</OPTION>
             If no value given, then take value from text
             - PJS
           */
          if(!lastformtag->default_value)
          {
            lastformtag->default_value = tl_strdup(lastformtag->text);
          }
        }
      }
      else if(retv && IS_TAG("TEXTAREA"))
      {
        fstag = TRUE;

        plastformtag = lastformtag;
        lastformtag = form_parse_textareatag(stack);

        retv->infos = dllist_append(retv->infos, (dllist_t)lastformtag);
      }
      else if(retv && lastformtag && IS_TAG("/TEXTAREA"))
      {
        if(tr && lastformtag->type == FORM_T_TEXTAREA)
        {
          text[tr] = '\0';
          sp = text;
          while(*sp && tl_ascii_isspace(*sp))
            sp++;
          if(!lastformtag->default_value && *sp)
            lastformtag->default_value = tl_strdup(sp);
          tr = 0;
        }
      }
      else if(IS_TAG("BR"))
      {
        TEXPAND(1);
        text[tr] = '\n';
        tr++;
      }

      if(fstag && tr)
      {
        char *sp;

        text[tr] = '\0';
        sp = text;
        while(*sp && tl_ascii_isspace(*sp))
          sp++;

        if(*sp && plastformtag)
        {
          if(plastformtag->type == FORM_T_RADIO ||
            plastformtag->type == FORM_T_CHECKBOX ||
            plastformtag->type == FORM_T_OPTION)
          {
            int ln = strcspn(sp, "\r\n");
            if(!plastformtag->text)
              plastformtag->text = tl_strndup(sp, ln);
            sp += ln;
            sp += strspn(sp, "\r\n \t");
          }
        }
        if(*sp)
        {
          form_field_info *ffi;
          dllist *last;


          ffi = form_field_info_new();
          ffi->type = FORM_T_NONFORM;
          ffi->text = tl_strdup(sp);

          if(optgroup)
          {
            last = dllist_last(optgroup->infos);
            optgroup->infos =
              dllist_insert_before(optgroup->infos, last, (dllist_t) ffi);
          }
          else if(selectgroup)
          {
            last = dllist_last(selectgroup->infos);
            selectgroup->infos =
              dllist_insert_before(selectgroup->infos, last, (dllist_t) ffi);
          }
          else
          {
            last = dllist_last(retv->infos);
            retv->infos = dllist_insert_before(retv->infos, last,
            (dllist_t) ffi);
          }
        }
        tr = 0;
      }
    }
    else if(tagstart)
    {
      SEXPAND(1);
      stack[sr] = form_text[i];
      sr++;
    }
    else
    {
      TEXPAND(1);
      text[tr] = form_text[i];
      tr++;
    }
  }
  if(retv)
  {
    retv->text = form_text;

    if(tr)
    {
      text[tr] = '\0';
      sp = text;
      while(*sp && tl_ascii_isspace(*sp))
        sp++;

      if(*sp)
      {
        form_field_info *ffi;

        ffi = form_field_info_new();
        ffi->type = FORM_T_NONFORM;
        ffi->text = tl_strdup(sp);

        retv->infos = dllist_append(retv->infos, (dllist_t)ffi);
      }
    }
  }

  _free(stack);
  _free(text);
  return retv;
}

void form_get_default_successful(char *name, dllist * fields, dllist ** retv)
{
  dllist *ptr;

  ptr = fields;
  while(ptr)
  {
    form_field_info *ffi = (form_field_info *) ptr->data;
    char *n, *v;

    n = NULL;
    v = NULL;
    switch (ffi->type)
    {
    case FORM_T_TEXT:
    case FORM_T_PASSWORD:
    case FORM_T_FILE:
    case FORM_T_TEXTAREA:
    case FORM_T_HIDDEN:
      n = ffi->name;
      v = tl_strdup(ffi->default_value);
      break;
    case FORM_T_RADIO:
    case FORM_T_CHECKBOX:
      if(ffi->checked)
      {
        n = ffi->name;
        v = tl_strdup(ffi->default_value);
      }
      break;
    case FORM_T_OPTION:
      if(ffi->selected)
      {
        n = ffi->name ? ffi->name : name;
        v = tl_strdup(ffi->default_value);
      }
      break;
    case FORM_T_OPTGROUP:
      form_get_default_successful(name, ffi->infos, retv);
      break;
    case FORM_T_SELECT:
      form_get_default_successful(ffi->name, ffi->infos, retv);
      break;
    default:
      /* nothing to do */
      break;
    }
    if(n)
    {
      form_field *ff = _malloc(sizeof(form_field));

      ff->type = ffi->type;

      ff->name = tl_strdup(n);
      ff->value = v ? v : tl_strdup("");
      *retv = dllist_append(*retv, (dllist_t)ff);
    }
    ptr = ptr->next;
  }
}

#if GTK_FACE
static void form_get_successful(char *name, dllist * fields, dllist ** retv)
{
  dllist *ptr;

  for(ptr = fields; ptr; ptr = ptr->next)
  {
    form_field_info *ffi = (form_field_info *) ptr->data;
    char *n, *v;

    if(!ffi->widget && (ffi->type != FORM_T_HIDDEN))
      continue;

    n = NULL;
    v = NULL;
    switch (ffi->type)
    {
    case FORM_T_TEXT:
    case FORM_T_PASSWORD:
      n = ffi->name;
      v = tl_strdup(gtk_entry_get_text(GTK_ENTRY(ffi->widget)));
      break;
    case FORM_T_FILE:
      n = ffi->name;
      v = tl_strdup(gtk_entry_get_text(GTK_ENTRY(ffi->widget)));
      break;
    case FORM_T_RADIO:
    case FORM_T_CHECKBOX:
      if(GTK_TOGGLE_BUTTON(ffi->widget)->active)
      {
        n = ffi->name;
        v = tl_strdup(ffi->default_value);
      }
      break;
    case FORM_T_TEXTAREA:
      {
        int len;
        char *p;

#if GTK_FACE < 2
        len = gtk_text_get_length(GTK_TEXT(ffi->widget));
#else
        len = gtk_text_buffer_get_char_count(
          gtk_text_view_get_buffer(GTK_TEXT_VIEW(ffi->widget)));
#endif
        p = gtk_editable_get_chars(GTK_EDITABLE(ffi->widget), 0, len);
        v = tl_strdup(p);
        g_free(p);
        n = ffi->name;
      }
      break;
    case FORM_T_OPTION:
      if(GTK_IS_CLIST(ffi->widget))
      {
        if(g_list_find(GTK_CLIST(ffi->widget)->selection, (void *) ffi->idx))
        {
          n = ffi->name ? ffi->name : name;
          v = tl_strdup(ffi->default_value);
        }
      }
      else
      {
        if(GTK_CHECK_MENU_ITEM(ffi->widget)->active)
        {
          n = ffi->name ? ffi->name : name;
          v = tl_strdup(ffi->default_value);
        }
      }
      break;
    case FORM_T_HIDDEN:
      n = ffi->name;
      v = tl_strdup(ffi->default_value);
      break;
    case FORM_T_OPTGROUP:
      form_get_successful(name, ffi->infos, retv);
      break;
    case FORM_T_SELECT:
      form_get_successful(ffi->name, ffi->infos, retv);
      break;
    default:
      /* nothing to do */
      break;
    }
    if(n)
    {
      form_field *ff = _malloc(sizeof(form_field));

      ff->type = ffi->type;

      ff->name = tl_strdup(n);
      ff->value = v ? v : tl_strdup("");
      *retv = dllist_append(*retv, (dllist_t)ff);
    }
  }
}

static void form_submit_cb(GtkWidget * w, form_info * fi)
{
  form_field_info *ffi = NULL;
  dllist *fields = NULL;
  url_info *ui;

  if(w)
    ffi = (form_field_info *) gtk_object_get_user_data(GTK_OBJECT(w));

  form_get_successful(NULL, fi->infos, &fields);

  if(ffi && ffi->name)
  {
    form_field *ff = _malloc(sizeof(form_field));

    ff->type = ffi->type;
    ff->name = tl_strdup(ffi->name);
    ff->value =
      ffi->default_value ? tl_strdup(ffi->default_value) : tl_strdup("");
    fields = dllist_prepend(fields, (dllist_t) ff);
  }

  ui = url_info_new(fi->action);
  ui->type = URLI_FORM;
  ui->method = fi->method;
  ui->encoding = fi->encoding;
  ui->fields = fields;

  if(gui_cfg.config_shell)
  {
    url_info *cui;
    int row = gtk_clist_append(GTK_CLIST(gui_cfg.url_list), &fi->action);
    cui = url_info_duplicate(ui);
    gtk_clist_set_row_data_full(GTK_CLIST(gui_cfg.url_list), row, cui,
      (GtkDestroyNotify) url_info_free);
  }

  cfg.request = dllist_append(cfg.request, (dllist_t)ui);

  if(cfg.mode_started)
  {
    if(!append_starting_url(ui, fi->parent_url))
    {
      gdk_beep();
    }
  }
}

static void form_reset(dllist * fields)
{
  while(fields)
  {
    form_field_info *ffi = (form_field_info *) fields->data;

    switch (ffi->type)
    {
    case FORM_T_TEXT:
    case FORM_T_PASSWORD:
      gtk_entry_set_text(GTK_ENTRY(ffi->widget), ffi->default_value);
      break;
    case FORM_T_FILE:
      gtk_entry_set_text(GTK_ENTRY(ffi->widget), ffi->default_value);
      break;
    case FORM_T_RADIO:
    case FORM_T_CHECKBOX:
      gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(ffi->widget),
        ffi->checked);
      break;
    case FORM_T_OPTGROUP:
    case FORM_T_SELECT:
      form_reset(ffi->infos);
      break;
    case FORM_T_OPTION:
      gtk_check_menu_item_set_state(GTK_CHECK_MENU_ITEM(ffi->widget),
        ffi->selected);
      break;
    case FORM_T_TEXTAREA:
#if GTK_FACE < 2
      gtk_text_set_point(GTK_TEXT(ffi->widget), 0);
      gtk_text_forward_delete(GTK_TEXT(ffi->widget),
        gtk_text_get_length(GTK_TEXT(ffi->widget)));
      if(ffi->default_value)
        gtk_text_insert(GTK_TEXT(ffi->widget), NULL, NULL, NULL,
          ffi->default_value, strlen(ffi->default_value));
#else
      gtk_text_buffer_set_text(gtk_text_view_get_buffer(
      GTK_TEXT_VIEW(ffi->widget)), ffi->default_value
      ? ffi->default_value : "", -1);
#endif
      break;
    default:
      break;
    }

    fields = fields->next;
  }
}

static void form_reset_cb(GtkWidget * w, form_info * fi)
{
  form_reset(fi->infos);
}

static GtkWidget *form_render_optgroup(form_field_info * sel, dllist * dl)
{
  GtkWidget *menu, *mi;
  char *p;
  dllist *ptr;

  menu = gtk_menu_new();
  gtk_widget_realize(menu);

  ptr = dl;
  while(ptr)
  {
    form_field_info *ffi = (form_field_info *) ptr->data;

    mi = NULL;
    switch (ffi->type)
    {
    case FORM_T_OPTION:
      if(ptr->next &&
        (((form_field_info *) ptr->next->data)->type == FORM_T_NONFORM))
      {
        if(ffi->text)
          p = ffi->text;
        else
          p = ((form_field_info *) ptr->next->data)->text;
        ptr = ptr->next;
      }
      else if(ffi->text)
        p = ffi->text;
      else if(ffi->default_value)
        p = ffi->default_value;
      else
        p = "option";
      if(ffi->multiple)
        mi = gtk_check_menu_item_new_with_label(p);
      else
      {
        mi = gtk_radio_menu_item_new_with_label(sel->rg, p);
        sel->rg = gtk_radio_menu_item_group(GTK_RADIO_MENU_ITEM(mi));
      }
      break;
    case FORM_T_OPTGROUP:
      if(ptr->next &&
        (((form_field_info *) ptr->next->data)->type == FORM_T_NONFORM))
      {
        if(ffi->text)
          p = ffi->text;
        else
          p = ((form_field_info *) ptr->next->data)->text;
        ptr = ptr->next;
      }
      else if(ffi->text)
        p = ffi->text;
      else
        p = "optgroup";
      mi = gtk_menu_item_new_with_label(p);
      gtk_menu_item_set_submenu(GTK_MENU_ITEM(mi),
        form_render_optgroup(sel, ffi->infos));
      break;
    default:
      xprintf(0, gettext("Unsupported form type in context: %d\n"),
        ffi->type);
      break;
    }
    if(mi)
    {
      gtk_menu_append(GTK_MENU(menu), mi);
      gtk_widget_show(mi);
      gtk_object_set_user_data(GTK_OBJECT(mi), ffi);
      gtk_widget_set_sensitive(mi, !ffi->disabled);
    }
    ffi->widget = mi;

    ptr = ptr->next;
  }

  return menu;
}

static GtkWidget *form_render_selection(form_field_info * ffi)
{
  GtkWidget *w, *menu;

  if(ffi->height > 1)
  {
    dllist *ptr;
    GtkWidget *swin = NULL;


    w = gtk_clist_new(1);
    if(ffi->multiple)
    {
      gtk_clist_set_selection_mode(GTK_CLIST(w), GTK_SELECTION_MULTIPLE);
    }
    else
    {
      gtk_clist_set_selection_mode(GTK_CLIST(w), GTK_SELECTION_SINGLE);
    }
    gtk_object_set_user_data(GTK_OBJECT(w), ffi);
    gtk_widget_set_sensitive(w, !ffi->disabled);
    for(ptr = ffi->infos; ptr; ptr = ptr->next)
    {
      form_field_info *offi = (form_field_info *) ptr->data;

      if(offi->type == FORM_T_OPTION)
      {
        char *p;

        if(offi->text)
          p = offi->text;
        else if(offi->default_value)
          p = offi->default_value;
        else
          p = "option";

        offi->idx = gtk_clist_append(GTK_CLIST(w), &p);
        gtk_clist_set_selectable(GTK_CLIST(w), offi->idx, !offi->disabled);
        if(offi->selected)
          gtk_clist_select_row(GTK_CLIST(w), offi->idx, 0);
        offi->widget = w;
      }
      else
      {
        xprintf(0, gettext("Unsupported form type in context: %d\n"),
          ffi->type);
      }
    }
    ffi->widget = w;

    swin = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin),
      GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_show(swin);
    gtk_container_add(GTK_CONTAINER(swin), w);

    gtk_widget_show(w);
    if(swin)
      w = swin;
  }
  else
  {
    w = gtk_option_menu_new();
    menu = form_render_optgroup(ffi, ffi->infos);
    gtk_option_menu_set_menu(GTK_OPTION_MENU(w), menu);
    gtk_object_set_user_data(GTK_OBJECT(w), ffi);
    gtk_widget_set_sensitive(w, !ffi->disabled);
  }

  return w;
}

static GtkWidget *form_render(form_info * fi, GtkWidget * pbox)
{
  GtkWidget *box, *w;
  dllist *ptr, *ptr2;
  char *p;

  box = gtk_vbox_new(FALSE, 2);
  gtk_box_pack_start(GTK_BOX(pbox), box, FALSE, FALSE, 1);
  gtk_widget_show(box);

  for(ptr = fi->infos; ptr; ptr = ptr->next)
  {
    form_field_info *ffi;

    ffi = (form_field_info *) ptr->data;

    w = NULL;

    switch (ffi->type)
    {
    case FORM_T_TEXT:
      w = gtk_entry_new();
      if(ffi->maxlen)
        gtk_entry_set_max_length(GTK_ENTRY(w), ffi->maxlen);
      if(ffi->width)
        gtk_widget_set_usize(w,
          ffi->width * 10 /*gdk_string_width(w->style->font, "xxx") */ , -1);
      break;
    case FORM_T_PASSWORD:
      w = gtk_entry_new();
      if(ffi->maxlen)
        gtk_entry_set_max_length(GTK_ENTRY(w), ffi->maxlen);
      if(ffi->width)
        gtk_widget_set_usize(w,
          ffi->width * 10 /*gdk_string_width(w->style->font, "xxx") */ , -1);
      gtk_entry_set_visibility(GTK_ENTRY(w), FALSE);
      break;
    case FORM_T_CHECKBOX:
      if(ffi->text)
        p = ffi->text;
      else if(ptr->next &&
        (((form_field_info *) ptr->next->data)->type == FORM_T_NONFORM))
      {
        p = ((form_field_info *) ptr->next->data)->text;
        ptr = ptr->next;
      }
      else
        p = "check";

      w = gtk_check_button_new_with_label(p);
      gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(w), ffi->checked);
      break;
    case FORM_T_RADIO:
      {
        GSList *rg = NULL;

        ptr2 = ptr->prev;
        while(ptr2)
        {
          form_field_info *tffi = (form_field_info *) ptr2->data;

          if(ffi->type == tffi->type &&
            tffi->name && ffi->name && !strcmp(tffi->name, ffi->name))
          {
            rg = gtk_radio_button_group(GTK_RADIO_BUTTON(tffi->widget));
            break;
          }
          ptr2 = ptr2->prev;
        }
        if(ffi->text)
          p = ffi->text;
        else if(ptr->next &&
          (((form_field_info *) ptr->next->data)->type == FORM_T_NONFORM))
        {
          p = ((form_field_info *) ptr->next->data)->text;
          ptr = ptr->next;
        }
        else
          p = "radio";

        w = gtk_radio_button_new_with_label(rg, p);
        gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(w), ffi->checked);
      }
      break;
    case FORM_T_SUBMIT:
    case FORM_T_IMAGE:
      if(ffi->name)
        p = ffi->name;
      else
        p = "Submit";
      w = gtk_button_new_with_label(p);

      gtk_signal_connect(GTK_OBJECT(w), "clicked",
        GTK_SIGNAL_FUNC(form_submit_cb), fi);
      break;
    case FORM_T_RESET:
      if(ffi->name)
        p = ffi->name;
      else
        p = "Reset";
      w = gtk_button_new_with_label(p);

      gtk_signal_connect(GTK_OBJECT(w), "clicked",
        GTK_SIGNAL_FUNC(form_reset_cb), fi);
      break;
    case FORM_T_FILE:
      w = gtk_entry_new();
      break;
    case FORM_T_HIDDEN:
      break;
    case FORM_T_BUTTON:
      if(ffi->default_value)
        p = ffi->default_value;
      else
        p = "Button";
      w = gtk_button_new_with_label(p);
      break;
    case FORM_T_SELECT:
      w = form_render_selection(ffi);
      break;
    case FORM_T_TEXTAREA:
#if GTK_FACE < 2
      w = gtk_text_new(NULL, NULL);
#else
      w = gtk_text_view_new();
#endif
      gtk_widget_set_usize(w, 250, 150);
#if GTK_FACE < 2
      gtk_text_set_editable(GTK_TEXT(w), ffi->readonly);
#else
      gtk_text_view_set_editable(GTK_TEXT_VIEW(w), ffi->readonly);
#endif
      break;
    case FORM_T_UNKNOWN:
    case FORM_T_OPTGROUP:
    case FORM_T_OPTION:
      xprintf(0, gettext("Unsupported form type in context: %d\n"),
        ffi->type);
      break;
    case FORM_T_NONFORM:
      w = gtk_label_new(ffi->text);
      break;
    }
    if(w)
    {
      gtk_box_pack_start(GTK_BOX(box), w, FALSE, FALSE, 1);
      gtk_widget_show(w);
      gtk_object_set_user_data(GTK_OBJECT(w), ffi);
      gtk_widget_set_sensitive(w, !ffi->disabled);
      if(!ffi->widget)
        ffi->widget = w;
    }
  }

  return box;
}

struct form_edit_dlg_t
{
  GtkWidget *top;
  GtkWidget *tab;
  GtkWidget *url_list;
  GtkWidget *action;
  GtkWidget *method;
  GtkWidget *encoding;
  GtkWidget *swin;
  GtkWidget *form;
  GtkWidget *formnr;
  GtkWidget *fs;
  form_info *formdata;
  char *docdata;
  int doclen;
  long formsnum;
  url *parent_url;
};

static void PopdownW(GtkObject * object, gpointer func_data)
{
  gtk_widget_hide(GTK_WIDGET(func_data));
}

static void form_edit_dlg_refresh_list(GtkWidget * w,
  struct form_edit_dlg_t *fedlg)
{
  int i;

  gtk_clist_freeze(GTK_CLIST(fedlg->url_list));
  gtk_clist_clear(GTK_CLIST(fedlg->url_list));
  LOCK_CFG_URLHASH;
  for(i = 0; i < cfg.url_hash_tbl->size; i++)
  {
    dllist *ptr = cfg.url_hash_tbl->nodes[i];

    while(ptr)
    {
      url *urlp = (url *) ptr->data;

      if(urlp->status & URL_HAVE_FORMS)
      {
        char *urlstr;
        int row;

        urlstr = url_to_urlstr(urlp, FALSE);
        row = gtk_clist_append(GTK_CLIST(fedlg->url_list), &urlstr);
        gtk_clist_set_row_data(GTK_CLIST(fedlg->url_list), row, urlp);
        _free(urlstr);
      }

      ptr = ptr->next;
    }
  }
  UNLOCK_CFG_URLHASH;
  gtk_clist_thaw(GTK_CLIST(fedlg->url_list));
}

static void form_edit_dlg_browse(struct form_edit_dlg_t *fedlg)
{
  GtkWidget *hbox, *vbox, *label, *swin, *button;

  hbox = gtk_hbox_new(FALSE, 2);
  gtk_widget_show(hbox);
  label = gtk_label_new(gettext("Browse"));
  gtk_notebook_append_page(GTK_NOTEBOOK(fedlg->tab), hbox, label);

  swin = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin),
    GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start(GTK_BOX(hbox), swin, TRUE, TRUE, 2);
  gtk_widget_show(swin);

  fedlg->url_list = gtk_clist_new(1);
  gtk_clist_set_column_auto_resize(GTK_CLIST(fedlg->url_list), 0, TRUE);
  gtk_clist_set_selection_mode(GTK_CLIST(fedlg->url_list),
    GTK_SELECTION_BROWSE);
  gtk_clist_set_column_title(GTK_CLIST(fedlg->url_list), 0,
    gettext("URLs with forms"));
  gtk_clist_column_titles_show(GTK_CLIST(fedlg->url_list));
  gtk_container_add(GTK_CONTAINER(swin), fedlg->url_list);
  gtk_widget_show(fedlg->url_list);

  vbox = gtk_vbox_new(FALSE, 2);
  gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 2);
  gtk_widget_show(vbox);

  button =
    guitl_pixmap_button(restart_small_xpm, NULL, gettext("Refresh URL list"));
  gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 10);
  gtk_widget_show(button);

  gtk_signal_connect(GTK_OBJECT(button), "clicked",
    GTK_SIGNAL_FUNC(form_edit_dlg_refresh_list), fedlg);
}

static void form_edit_dlg_switch_form(GtkWidget * w,
  struct form_edit_dlg_t *fedlg)
{
  char *formdata;
  int formlen;
  long nr = (long) gtk_object_get_user_data(GTK_OBJECT(w));

  if(fedlg->formdata)
  {
    form_free(fedlg->formdata);
    fedlg->formdata = NULL;
    gtk_widget_destroy(fedlg->form);
    gtk_label_set(GTK_LABEL(fedlg->method), "");
    gtk_label_set(GTK_LABEL(fedlg->encoding), "");
    gtk_label_set(GTK_LABEL(fedlg->action), "");
  }

  formdata = form_get_text(nr, fedlg->docdata, fedlg->doclen, &formlen);

  if(!formdata)
    return;

  fedlg->formdata = form_parse(formdata, formlen);
  fedlg->formdata->parent_url = fedlg->parent_url;

  gtk_label_set(GTK_LABEL(fedlg->method),
    (fedlg->formdata->method ==
      FORM_M_GET) ? "GET" : (fedlg->formdata->method ==
      FORM_M_POST) ? "POST" : gettext("unknown"));
  gtk_label_set(GTK_LABEL(fedlg->encoding),
    (fedlg->formdata->encoding ==
      FORM_E_URLENCODED) ? "application/x-www-form-urlencoded" : (fedlg->
      formdata->encoding ==
      FORM_E_MULTIPART) ? "multipart/form-data" : gettext("unknown"));
  gtk_label_set(GTK_LABEL(fedlg->action), fedlg->formdata->action);

  fedlg->form = gtk_vbox_new(FALSE, 2);
  gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(fedlg->swin),
    fedlg->form);
  gtk_widget_show(fedlg->form);

  form_render(fedlg->formdata, fedlg->form);
  /* FIXME: The following line showed up to be wrong after ansification,
     thought I have no idea what to do with it.
     form_reset(NULL, fedlg); */
}

static char *form_load_doc(char *docfile, int *doclen)
{
  char *retv = NULL;
  char buf[1024];
  int fd;
  int len;

  *doclen = 0;
  if(!docfile)
    return NULL;

  if((fd = open(docfile, O_BINARY | O_RDONLY)) < 0)
  {
    xperror(docfile);
    return NULL;
  }

  while((len = read(fd, buf, sizeof(buf))) > 0)
  {
    retv = _realloc(retv, *doclen + len + 1);
    memmove(retv + *doclen, buf, len);
    *doclen += len;
  }

  if(len < 0)
    xperror(docfile);

  close(fd);

  return retv;
}

static int form_edit_dlg_load_html_file(char *filename,
  struct form_edit_dlg_t *fedlg)
{
  GtkWidget *menu, *fmi = NULL;

  if(fedlg->formdata)
  {
    form_free(fedlg->formdata);
    fedlg->formdata = NULL;
    gtk_widget_destroy(fedlg->form);
    gtk_label_set(GTK_LABEL(fedlg->method), "");
    gtk_label_set(GTK_LABEL(fedlg->encoding), "");
    gtk_label_set(GTK_LABEL(fedlg->action), "");
  }
  if(filename)
    fedlg->docdata = form_load_doc(filename, &fedlg->doclen);

  if(!fedlg->docdata)
    return -1;

  menu = gtk_menu_new();
  for(fedlg->formsnum = 0;; fedlg->formsnum++)
  {
    int formlen;
    char *formdata;
    GtkWidget *mi;
    char pom[10];

    formdata =
      form_get_text(fedlg->formsnum, fedlg->docdata, fedlg->doclen, &formlen);

    if(!formdata)
      break;

    _free(formdata);

    sprintf(pom, "%ld", fedlg->formsnum);
    mi = gtk_menu_item_new_with_label(pom);
    gtk_object_set_user_data(GTK_OBJECT(mi), (gpointer) fedlg->formsnum);
    gtk_signal_connect(GTK_OBJECT(mi), "activate",
      GTK_SIGNAL_FUNC(form_edit_dlg_switch_form), fedlg);
    gtk_widget_show(mi);
    gtk_menu_append(GTK_MENU(menu), mi);

    if(!fedlg->formsnum)
      fmi = mi;
  }

  gtk_option_menu_set_menu(GTK_OPTION_MENU(fedlg->formnr), menu);

  if(!fedlg->formsnum)
    return -1;

  gtk_option_menu_set_history(GTK_OPTION_MENU(fedlg->formnr), 0);
  form_edit_dlg_switch_form(fmi, fedlg);

  return 0;
}

static void form_edit_dlg_switch_page(GtkWidget * w, GtkNotebookPage * pg,
  gint pgnr, struct form_edit_dlg_t *fedlg)
{
  if(pgnr == 1)
  {
    if(fedlg->docdata)
    {
      _free(fedlg->docdata);
      gtk_widget_destroy(GTK_OPTION_MENU(fedlg->formnr)->menu);
    }
    if(GTK_CLIST(fedlg->url_list)->selection)
    {
      url *urlp;

      urlp = (url *) gtk_clist_get_row_data(GTK_CLIST(fedlg->url_list),
        GPOINTER_TO_INT(GTK_CLIST(fedlg->url_list)->selection->data));

      fedlg->parent_url = urlp;
      form_edit_dlg_load_html_file(url_to_filename(urlp, FALSE), fedlg);
    }
  }
}

static void form_edit_dlg_load_file_ok(GtkWidget * w,
  struct form_edit_dlg_t *fedlg)
{
  char *p =
    (gchar *) gtk_file_selection_get_filename(GTK_FILE_SELECTION(fedlg->fs));

  fedlg->parent_url = NULL;
  form_edit_dlg_load_html_file(p, fedlg);
  gtk_widget_destroy(fedlg->fs);
}

static void form_edit_dlg_load_file(GtkWidget * w,
  struct form_edit_dlg_t *fedlg)
{
  if(!fedlg->fs)
  {
    fedlg->fs = gtk_file_selection_new(gettext("Pavuk: load form file"));

    gtk_signal_connect(GTK_OBJECT(fedlg->fs), "destroy",
      GTK_SIGNAL_FUNC(gtk_widget_destroyed), &fedlg->fs);

    gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(fedlg->fs)->ok_button),
      "clicked", GTK_SIGNAL_FUNC(form_edit_dlg_load_file_ok), fedlg);

    gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(fedlg->fs)->
        cancel_button), "clicked", GTK_SIGNAL_FUNC(PopdownW), fedlg->fs);
  }
  gtk_widget_show(fedlg->fs);
  if(GTK_WIDGET_REALIZED(fedlg->fs))
    gdk_window_raise(fedlg->fs->window);
}

static void form_edit_dlg_fill(struct form_edit_dlg_t *fedlg)
{
  GtkWidget *label, *button, *vbox, *pbox, *frame;
  GtkWidget *menu;

  vbox = gtk_vbox_new(FALSE, 2);
  gtk_widget_show(vbox);
  label = gtk_label_new(gettext("Fill forms"));
  gtk_notebook_append_page(GTK_NOTEBOOK(fedlg->tab), vbox, label);

  pbox = gtk_table_new(2, 4, FALSE);
  gtk_box_pack_start(GTK_BOX(vbox), pbox, FALSE, FALSE, 2);
  gtk_widget_show(pbox);

  label = gtk_label_new(gettext("Form number: "));
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
  gtk_table_attach(GTK_TABLE(pbox), label, 0, 1, 0, 1, GTK_FILL, GTK_FILL, 2,
    2);
  gtk_widget_show(label);

  fedlg->formnr = gtk_option_menu_new();
  gtk_widget_set_usize(fedlg->formnr, 45, -1);
  gtk_table_attach(GTK_TABLE(pbox), fedlg->formnr, 1, 2, 0, 1, GTK_SHRINK,
    GTK_SHRINK, 2, 2);
  menu = gtk_menu_new();
  gtk_option_menu_set_menu(GTK_OPTION_MENU(fedlg->formnr), menu);
  gtk_widget_show(fedlg->formnr);

  label = gtk_label_new(gettext("Action URL: "));
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
  gtk_table_attach(GTK_TABLE(pbox), label, 0, 1, 1, 2, GTK_FILL, GTK_FILL, 2,
    2);
  gtk_widget_show(label);

  fedlg->action = gtk_label_new("");
  gtk_table_attach(GTK_TABLE(pbox), fedlg->action, 1, 2, 1, 2, GTK_FILL,
    GTK_FILL, 2, 2);
  gtk_widget_show(fedlg->action);

  label = gtk_label_new(gettext("Request method: "));
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
  gtk_table_attach(GTK_TABLE(pbox), label, 0, 1, 2, 3, GTK_FILL, GTK_FILL, 2,
    2);
  gtk_widget_show(label);

  fedlg->method = gtk_label_new("");
  gtk_table_attach(GTK_TABLE(pbox), fedlg->method, 1, 2, 2, 3, GTK_FILL,
    GTK_FILL, 2, 2);
  gtk_widget_show(fedlg->method);

  label = gtk_label_new(gettext("Request encoding: "));
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
  gtk_table_attach(GTK_TABLE(pbox), label, 0, 1, 3, 4, GTK_FILL, GTK_FILL, 2,
    2);
  gtk_widget_show(label);

  fedlg->encoding = gtk_label_new("");
  gtk_table_attach(GTK_TABLE(pbox), fedlg->encoding, 1, 2, 3, 4, GTK_FILL,
    GTK_FILL, 2, 2);
  gtk_widget_show(fedlg->encoding);

  frame = gtk_frame_new(gettext("HTML form content"));
  gtk_box_pack_start(GTK_BOX(vbox), frame, TRUE, TRUE, 2);
  gtk_widget_show(frame);

  fedlg->swin = gtk_scrolled_window_new(NULL, NULL);
  gtk_widget_set_usize(fedlg->swin, 400, 200);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(fedlg->swin),
    GTK_POLICY_ALWAYS, GTK_POLICY_ALWAYS);
  gtk_container_add(GTK_CONTAINER(frame), fedlg->swin);
  gtk_widget_show(fedlg->swin);

  pbox = gtk_hbutton_box_new();
  gtk_box_pack_start(GTK_BOX(vbox), pbox, FALSE, FALSE, 2);
  gtk_widget_show(pbox);

  button = guitl_pixmap_button(load_xpm, NULL, gettext("Load HTML file ..."));
  gtk_container_add(GTK_CONTAINER(pbox), button);
  gtk_widget_show(button);

  gtk_signal_connect(GTK_OBJECT(button), "clicked",
    GTK_SIGNAL_FUNC(form_edit_dlg_load_file), fedlg);

}

static void form_edit_dlg_close(GtkWidget * w, struct form_edit_dlg_t *fedlg)
{
  gtk_widget_hide(fedlg->top);
  _free(fedlg->docdata);
  if(fedlg->formdata)
  {
    form_free(fedlg->formdata);
    fedlg->formdata = NULL;
    gtk_widget_destroy(fedlg->form);
  }
  gtk_notebook_set_page(GTK_NOTEBOOK(fedlg->tab), 0);
}

static struct form_edit_dlg_t fedlg_info =
  { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };

void form_edit_dlg_clear(void)
{
  if(!fedlg_info.top)
    return;

  if(!MT_IS_MAIN_THREAD())
  {
    GDK_THREADS_ENTER();
  }

  form_edit_dlg_refresh_list(NULL, &fedlg_info);
  fedlg_info.parent_url = NULL;
  form_edit_dlg_load_html_file(NULL, &fedlg_info);

  if(!MT_IS_MAIN_THREAD())
  {
    GDK_THREADS_LEAVE();
  }
}

void form_edit_dlg(void)
{
  if(!fedlg_info.top)
  {
    GtkWidget *tbox, *bbox, *button;

    fedlg_info.top = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(fedlg_info.top),
      gettext("Pavuk: HTML forms editor"));
    gtk_signal_connect(GTK_OBJECT(fedlg_info.top), "destroy",
      GTK_SIGNAL_FUNC(gtk_widget_destroyed), &fedlg_info.top);

    tbox = gtk_vbox_new(FALSE, 2);
    gtk_container_add(GTK_CONTAINER(fedlg_info.top), tbox);
    gtk_widget_show(tbox);

    fedlg_info.tab = gtk_notebook_new();
    gtk_box_pack_start(GTK_BOX(tbox), fedlg_info.tab, TRUE, TRUE, 2);
    gtk_widget_show(fedlg_info.tab);

    form_edit_dlg_browse(&fedlg_info);
    form_edit_dlg_fill(&fedlg_info);

    gtk_signal_connect(GTK_OBJECT(fedlg_info.tab), "switch_page",
      GTK_SIGNAL_FUNC(form_edit_dlg_switch_page), &fedlg_info);

    bbox = gtk_hbutton_box_new();
    gtk_box_pack_start(GTK_BOX(tbox), bbox, FALSE, FALSE, 2);
    gtk_widget_show(bbox);

    button = guitl_pixmap_button(cancel_xpm, NULL, gettext("Close"));
    gtk_container_add(GTK_CONTAINER(bbox), button);
    gtk_widget_show(button);

    gtk_signal_connect(GTK_OBJECT(button), "clicked",
      GTK_SIGNAL_FUNC(form_edit_dlg_close), &fedlg_info);
  }
  form_edit_dlg_refresh_list(NULL, &fedlg_info);
  gtk_widget_show(fedlg_info.top);
  if(GTK_WIDGET_REALIZED(fedlg_info.top))
    gdk_window_raise(fedlg_info.top->window);
}

#endif /* GTK_FACE */
