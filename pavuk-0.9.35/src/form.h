/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#ifndef _form_h_
#define _form_h_

#include "dllist.h"

typedef enum
{
  FORM_T_TEXT,
  FORM_T_PASSWORD,
  FORM_T_CHECKBOX,
  FORM_T_RADIO,
  FORM_T_SUBMIT,
  FORM_T_RESET,
  FORM_T_FILE,
  FORM_T_HIDDEN,
  FORM_T_IMAGE,
  FORM_T_BUTTON,
  FORM_T_SELECT,
  FORM_T_OPTION,
  FORM_T_OPTGROUP,
  FORM_T_TEXTAREA,
  FORM_T_NONFORM,
  FORM_T_UNKNOWN
} form_field_types;

typedef struct
{
  form_method method;
  form_encoding encoding;
  char *action;
  dllist *infos;
  char *text;
  void *parent_url;
} form_info;

typedef struct
{
  form_field_types type;
  char *name;
  char *value;
} form_field;

extern dllist *form_parse_urlencoded_query(char *);
extern char *form_decode_urlencoded_str(char *, int);
extern char *form_encode_urlencoded_str(char *);
extern char *form_encode_urlencoded(dllist *);
extern char *form_encode_multipart_boundary(void);
extern char *form_encode_multipart(dllist *, char *, int *);
extern char *form_encode_query(form_info *, int *);

extern form_field *form_field_new(char *, char *);
extern form_field *form_field_duplicate(form_field *);
extern int form_field_compare(dllist_t, dllist_t);
extern int form_field_compare_name(dllist_t, dllist_t);

extern form_info *form_info_dup(form_info *);


#ifdef GTK_FACE
#include <gtk/gtkwidget.h>
#endif

typedef struct
{
  form_field_types type;
  char *name;
  char *default_value;
  char *text;
  int width;
  int height;
  int maxlen;
  int readonly;
  int checked;
  int disabled;
  int multiple;
  int selected;
  dllist *infos;
#ifdef GTK_FACE
  GtkWidget *widget;
  GSList *rg;
  int idx;
#endif
} form_field_info;

extern char *form_get_text(int, char *, int, int *);
extern void form_free(form_info *);
extern form_info *form_parse(char *, int);
extern void form_get_default_successful(char *, dllist *, dllist **);

#ifdef GTK_FACE
extern void form_edit_dlg(void);
extern void form_edit_dlg_clear(void);
#endif

#endif
