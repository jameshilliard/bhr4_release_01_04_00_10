/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#ifndef _html_h_
#define _html_h_

#include "dllist.h"
#include "doc.h"

extern char *html_get_attrib_from_tag(char *, char *);
extern void rewrite_parents_links(url *, char *);
extern void rewrite_one_parent_links(url *, url *, char *);
extern int html_tag_co_elem(char *, char *);
extern void html_replace_url_in_stack(char *, char *, char *, int);

extern dllist *html_process_document(doc *, dllist **);
extern void html_process_parent_document(doc *, url *, char *);

typedef enum
{
  HTML_TAG_HEAD,
  HTML_TAG_BODY,
  HTML_TAG_TABLE,
  HTML_TAG_TH,
  HTML_TAG_TD,
  HTML_TAG_IMG,
  HTML_TAG_INPUT,
  HTML_TAG_FRAME,
  HTML_TAG_IFRAME,
  HTML_TAG_APPLET,
  HTML_TAG_SCRIPT,
  HTML_TAG_SOUND,
  HTML_TAG_BGSOUND,
  HTML_TAG_EMBED,
  HTML_TAG_AREA,
  HTML_TAG_BASE,
  HTML_TAG_FIG,
  HTML_TAG_OVERLAY,
  HTML_TAG_A,
  HTML_TAG_LINK,
  HTML_TAG_FORM,
  HTML_TAG_LAYER,
  HTML_TAG_META,
  HTML_TAG_INS,
  HTML_TAG_DEL,
  HTML_TAG_Q,
  HTML_TAG_SPAN,
  HTML_TAG_DIV,
  HTML_TAG_OBJECT,
  HTML_TAG_ADDRESS,
  HTML_TAG_BLOCKQUOTE,
  HTML_TAG_CENTER,
  HTML_TAG_H1,
  HTML_TAG_H2,
  HTML_TAG_H3,
  HTML_TAG_H4,
  HTML_TAG_H5,
  HTML_TAG_H6,
  HTML_TAG_HR,
  HTML_TAG_ISINDEX,
  HTML_TAG_P,
  HTML_TAG_PRE,
  HTML_TAG_NOSCRIPT,
  HTML_TAG_DIR,
  HTML_TAG_DL,
  HTML_TAG_DT,
  HTML_TAG_DD,
  HTML_TAG_LI,
  HTML_TAG_MENU,
  HTML_TAG_OL,
  HTML_TAG_UL,
  HTML_TAG_CAPTION,
  HTML_TAG_COLGROUP,
  HTML_TAG_COL,
  HTML_TAG_THEAD,
  HTML_TAG_TFOOT,
  HTML_TAG_TBODY,
  HTML_TAG_FIELDSET,
  HTML_TAG_BUTTON,
  HTML_TAG_LEGEND,
  HTML_TAG_LABEL,
  HTML_TAG_SELECT,
  HTML_TAG_OPTGROUP,
  HTML_TAG_OPTION,
  HTML_TAG_TEXTAREA,
  HTML_TAG_BDO,
  HTML_TAG_BR,
  HTML_TAG_FONT,
  HTML_TAG_MAP,
  HTML_TAG_SUB,
  HTML_TAG_SUP,
  HTML_TAG_ABBR,
  HTML_TAG_ACRONYM,
  HTML_TAG_CITE,
  HTML_TAG_CODE,
  HTML_TAG_DFN,
  HTML_TAG_EM,
  HTML_TAG_KBD,
  HTML_TAG_SAMP,
  HTML_TAG_STRONG,
  HTML_TAG_VAR,
  HTML_TAG_B,
  HTML_TAG_BIG,
  HTML_TAG_I,
  HTML_TAG_S,
  HTML_TAG_SMALL,
  HTML_TAG_STRIKE,
  HTML_TAG_TT,
  HTML_TAG_U,
  HTML_TAG_FRAMESET,
  HTML_TAG_NOFRAMES,
  HTML_TAG_CSOBJ,
  HTML_TAG_HACK
} html_tag_type_t;

typedef enum
{
  HTML_ATTRIB_PROFILE,
  HTML_ATTRIB_BACKGROUND,
  HTML_ATTRIB_STYLE,
  HTML_ATTRIB_SRC,
  HTML_ATTRIB_LOWSRC,
  HTML_ATTRIB_LONGDESC,
  HTML_ATTRIB_USEMAP,
  HTML_ATTRIB_CODEBASE,
  HTML_ATTRIB_HREF,
  HTML_ATTRIB_ACTION,
  HTML_ATTRIB_CONTENT,
  HTML_ATTRIB_CITE,
  HTML_ATTRIB_DATA,
  HTML_ATTRIB_HT,
  HTML_ATTRIB_JSEVENT,
  HTML_ATTRIB_HACK,
  HTML_ATTRIB_NULL
} html_tag_attrib_type_t;

typedef enum
{
  WML_TAG_GO,
  WML_TAG_A,
  WML_TAG_IMG,
  WML_TAG_OPTION
} wml_tag_type_t;

typedef enum
{
  WML_ATTRIB_HREF,
  WML_ATTRIB_SRC
} wml_tag_attrib_type_t;

typedef struct
{
  html_tag_attrib_type_t type;
  char *attrib;
  int stat;
} html_tag_atrib_t;

typedef struct
{
  html_tag_type_t type;
  char *tag;
  html_tag_atrib_t attribs[9];
} html_tag_t;

#define LINK_INLINE     (1 << 0)
#define LINK_DOWNLD     (1 << 1)
#define LINK_REMOVE     (1 << 2)
#define LINK_DISABLED   (1 << 3)
#define LINK_STYLE      (1 << 4)
#define LINK_SCRIPT     (1 << 5)
#define LINK_FORM       (1 << 6)
#define LINK_JS         (1 << 7)

extern char *html_js_patterns[];
extern html_tag_t html_link_tags[];
extern int html_link_tags_num(void);

#endif
