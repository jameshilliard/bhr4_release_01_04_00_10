/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include "config.h"

#ifdef HAVE_MOZJS

#include <assert.h>
#include <stdarg.h>
#include <string.h>
#include <jsapi.h>

#include "tools.h"
#include "url.h"
#include "dllist.h"
#include "jsbind.h"

#define STACK_CHUNK_SIZE 8192

#define SET_STR(s) \
  if (s) *val = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, s)); \
  else *val = JSVAL_NULL;

static JSRuntime *rt = NULL;
static JSContext *cx = NULL;
static JSObject *go = NULL;
static JSObject *scriptobj = NULL;
static JSScript *script = NULL;
static bool_t pjs_have_pavuk_url_cond_check = FALSE;

static JSBool pjs_print(JSContext *, JSObject *, uintN, jsval *, jsval *);

/* global class */
static JSClass globalClass = {
  "Global", 0,
  JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
  JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub
};

/* global methods available */
static JSFunctionSpec globalMethods[] = {
  {"print", pjs_print, 0, 0},
  {NULL, NULL, 0, 0}
};

static void pjs_error(JSContext *cx, const char *msg, JSErrorReport *rpt)
{
  if(!rpt)
    xprintf(1, "pjs: %s\n", msg);
  else
    xprintf(1, "pjs: %s: line %i: error: %s\n",
      rpt->filename, rpt->lineno, msg);
}


static JSBool pjs_print(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rv)
{
  int i;
  JSString *s;

  for(i = 0; i < argc; i++)
  {
    if(!(s = JS_ValueToString(cx, argv[i])))
    {
      JS_ReportError(cx, "pjs: print: %s", gettext("bad parameter"));
      return JS_FALSE;
    }
    xprintf(1, "%s%s", i ? " " : "", JS_GetStringBytes(s));
  }

  return JS_TRUE;
}

/* URL class implementation */
static JSBool pjs_url_get_parent(JSContext *, JSObject *, uintN, jsval *,
  jsval *);
static JSBool pjs_url_check_cond(JSContext *, JSObject *, uintN, jsval *,
  jsval *);
static JSBool pjs_url_set_property(JSContext *, JSObject *, jsval, jsval *);
static JSBool pjs_url_get_property(JSContext *, JSObject *, jsval, jsval *);
static void pjs_url_finalize(JSContext *, JSObject *);

static JSClass urlClass = {
  "PavukUrl", JSCLASS_HAS_PRIVATE,
  JS_PropertyStub, JS_PropertyStub,
  pjs_url_get_property, pjs_url_set_property,
  JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub,
  pjs_url_finalize
};

#define PJS_IS_URL(v) \
  (JS_GET_CLASS(cx, JSVAL_TO_OBJECT(v)) == &urlClass)

static JSFunctionSpec urlMethods[] = {
  {"get_parent", pjs_url_get_parent, 1, JSPROP_ENUMERATE},
  {"check_cond", pjs_url_check_cond, 1, JSPROP_ENUMERATE},
  {NULL, NULL, 0, 0}
};

enum _url_prop
{
  PJSURL_PROTOCOL,
  PJSURL_STATUS,
  PJSURL_LEVEL,
  PJSURL_REF_CNT,
  PJSURL_URLSTR,
  PJSURL_MOVED,

  PJSURL_HTTP_HOST,
  PJSURL_HTTP_PORT,
  PJSURL_HTTP_DOCUMENT,
  PJSURL_HTTP_SEARCHSTR,
  PJSURL_HTTP_ANCHOR_NAME,
  PJSURL_HTTP_USER,
  PJSURL_HTTP_PASS,

  PJSURL_FTP_HOST,
  PJSURL_FTP_PORT,
  PJSURL_FTP_USER,
  PJSURL_FTP_PASS,
  PJSURL_FTP_PATH,
  PJSURL_FTP_ANCHOR_NAME,
  PJSURL_FTP_DIR,

  PJSURL_GOPHER_HOST,
  PJSURL_GOPHER_PORT,
  PJSURL_GOPHER_SELECTOR,

  PJSURL_FILE_FNAME,
  PJSURL_FILE_SEARCHSTR,
  PJSURL_FILE_ANCHOR_NAME,

  PJSURL_UNSUP_URLSTR,

  PJSURL_CHECK_LEVEL,
  PJSURL_MIME_TYPE,
  PJSURL_DOC_SIZE,
  PJSURL_MDTM,
  PJSURL_DOCNR,
  PJSURL_HTML_TAG,
  PJSURL_HTML_DOC,
  PJSURL_HTML_DOC_OFFSET,
  PJSURL_TAG,
  PJSURL_ATTRIB,

  PJSURL_NULL
};

typedef struct
{
  url *urlp;
  cond_info_t *condp;
  int free_url;
} url_priv_t;

static JSPropertySpec urlProperties[] = {
  {"protocol", PJSURL_PROTOCOL, JSPROP_ENUMERATE | JSPROP_READONLY},
  {"status", PJSURL_STATUS, JSPROP_ENUMERATE},
  {"level", PJSURL_LEVEL, JSPROP_ENUMERATE | JSPROP_READONLY},
  {"ref_cnt", PJSURL_REF_CNT, JSPROP_ENUMERATE | JSPROP_READONLY},
  {"urlstr", PJSURL_URLSTR, JSPROP_ENUMERATE | JSPROP_READONLY},
  {NULL, 0, 0},
};

static url_priv_t *pjs_url_obj_set_url(JSContext *cx, JSObject *obj, url *purl, cond_info_t *condp)
{
  url_priv_t *up;

  up = _malloc(sizeof(url_priv_t));

  up->urlp = purl;
  up->condp = condp;
  up->free_url = FALSE;

  JS_SetPrivate(cx, obj, up);

#define NEW_PROP(name, id) \
  JS_DefinePropertyWithTinyId(cx, obj, name, id, JSVAL_VOID, \
    pjs_url_get_property, JS_PropertyStub, \
    JSPROP_ENUMERATE | JSPROP_READONLY);

  if(purl->moved_to)
    NEW_PROP("moved_url", PJSURL_MOVED);

  switch (purl->type)
  {
  case URLT_HTTP:
  case URLT_HTTPS:
    NEW_PROP("http_host", PJSURL_HTTP_HOST);
    NEW_PROP("http_port", PJSURL_HTTP_PORT);
    NEW_PROP("http_document", PJSURL_HTTP_DOCUMENT);
    NEW_PROP("http_searchstr", PJSURL_HTTP_SEARCHSTR);
    NEW_PROP("http_anchor_name", PJSURL_HTTP_ANCHOR_NAME);
    NEW_PROP("http_user", PJSURL_HTTP_USER);
    NEW_PROP("http_password", PJSURL_HTTP_PASS);
    break;
  case URLT_FTP:
  case URLT_FTPS:
    NEW_PROP("ftp_host", PJSURL_FTP_HOST);
    NEW_PROP("ftp_port", PJSURL_FTP_PORT);
    NEW_PROP("ftp_user", PJSURL_FTP_USER);
    NEW_PROP("ftp_password", PJSURL_FTP_PASS);
    NEW_PROP("ftp_path", PJSURL_FTP_PATH);
    NEW_PROP("ftp_anchor_name", PJSURL_FTP_ANCHOR_NAME);
    NEW_PROP("ftp_dir", PJSURL_FTP_DIR);
    break;
  case URLT_FILE:
    NEW_PROP("file_name", PJSURL_FILE_FNAME);
    NEW_PROP("file_searchstr", PJSURL_FILE_SEARCHSTR);
    NEW_PROP("file_anchor_name", PJSURL_FILE_ANCHOR_NAME);
    break;
  case URLT_GOPHER:
    NEW_PROP("gopher_host", PJSURL_GOPHER_HOST);
    NEW_PROP("gopher_port", PJSURL_GOPHER_PORT);
    NEW_PROP("gopher_selector", PJSURL_GOPHER_SELECTOR);
    break;
  case URLT_UNKNOWN:
    NEW_PROP("unsupported_urlstr", PJSURL_UNSUP_URLSTR);
  default:
    break;
  }

  if(up->condp)
  {
    NEW_PROP("check_level", PJSURL_CHECK_LEVEL);
    if(up->condp->mimet)
      NEW_PROP("mime_type", PJSURL_MIME_TYPE);
    if(up->condp->full_tag)
      NEW_PROP("html_tag", PJSURL_HTML_TAG);
    if(up->condp->size >= 0)
      NEW_PROP("doc_size", PJSURL_DOC_SIZE);
    if(up->condp->time)
      NEW_PROP("modification_time", PJSURL_MDTM);
    if(up->condp->urlnr)
      NEW_PROP("doc_number", PJSURL_DOCNR);
    if(up->condp->tag)
      NEW_PROP("tag", PJSURL_TAG);
    if(up->condp->attrib)
      NEW_PROP("attrib", PJSURL_ATTRIB);
    if(up->condp->html_doc)
    {
      NEW_PROP("html_doc", PJSURL_HTML_DOC);
      NEW_PROP("html_doc_offset", PJSURL_HTML_DOC_OFFSET);
    }
  }

  return up;
}

/* constructor */
static JSBool pjs_url_new(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rv)
{
  url_priv_t *up;
  url *urlp = NULL;

  if(argc != 1 || !JSVAL_IS_STRING(argv[0]))
  {
    JS_ReportError(cx, gettext("PavukUrl constructor failed\n"));
    return JS_FALSE;
  }

  urlp = url_parse(JS_GetStringBytes(JSVAL_TO_STRING(argv[0])));
  assert(urlp->type != URLT_FROMPARENT);
  up = pjs_url_obj_set_url(cx, obj, urlp, NULL);
  up->free_url = TRUE;

  return JS_TRUE;
}

/* destructor */
static void pjs_url_finalize(JSContext *cx, JSObject *obj)
{
  url_priv_t *up;

  up = JS_GetPrivate(cx, obj);

  if(up)
  {
    if(up->free_url)
    {
      free_deep_url(up->urlp);
      _free(up->urlp);
    }
    _free(up);
  }
}

static JSObject *pjs_url_to_jsobj(JSContext *cx, url *purl, cond_info_t *condp)
{
  JSObject *obj;

  obj = JS_NewObject(cx, &urlClass, NULL, NULL);
  pjs_url_obj_set_url(cx, obj, purl, condp);

  return obj;
}

static JSBool pjs_url_get_property(JSContext *cx, JSObject *obj, jsval id, jsval *val)
{
  if(JSVAL_IS_INT(id))
  {
    url_priv_t *up;
    int n;
    char *p;

    up = JS_GetPrivate(cx, obj);

    switch (JSVAL_TO_INT(id))
    {
    case PJSURL_PROTOCOL:
      p = prottable[up->urlp->type].urlid;
      SET_STR(p);
      break;
    case PJSURL_STATUS:
      *val = INT_TO_JSVAL(up->urlp->status);
      break;
    case PJSURL_LEVEL:
      *val = INT_TO_JSVAL(up->urlp->level);
      break;
    case PJSURL_REF_CNT:
      *val = INT_TO_JSVAL(up->urlp->ref_cnt);
      break;
    case PJSURL_URLSTR:
      p = url_to_urlstr(up->urlp, TRUE);
      SET_STR(p);
      _free(p);
      break;
    case PJSURL_HTTP_HOST:
    case PJSURL_FTP_HOST:
    case PJSURL_GOPHER_HOST:
      p = url_get_site(up->urlp);
      SET_STR(p);
      break;
    case PJSURL_HTTP_PORT:
    case PJSURL_FTP_PORT:
    case PJSURL_GOPHER_PORT:
      n = url_get_port(up->urlp);
      *val = INT_TO_JSVAL(n);
      break;
    case PJSURL_HTTP_DOCUMENT:
    case PJSURL_FTP_PATH:
    case PJSURL_GOPHER_SELECTOR:
    case PJSURL_FILE_FNAME:
      p = url_get_path(up->urlp);
      SET_STR(p);
      break;
    case PJSURL_HTTP_ANCHOR_NAME:
    case PJSURL_FTP_ANCHOR_NAME:
    case PJSURL_FILE_ANCHOR_NAME:
      p = url_get_anchor_name(up->urlp);
      SET_STR(p);
      break;
    case PJSURL_HTTP_SEARCHSTR:
    case PJSURL_FILE_SEARCHSTR:
      p = url_get_search_str(up->urlp);
      SET_STR(p);
      break;
    case PJSURL_HTTP_USER:
    case PJSURL_FTP_USER:
      p = url_get_user(up->urlp, NULL);
      SET_STR(p);
      break;
    case PJSURL_HTTP_PASS:
    case PJSURL_FTP_PASS:
      p = url_get_pass(up->urlp, NULL);
      SET_STR(p);
      break;
    case PJSURL_FTP_DIR:
      *val = up->urlp->p.ftp.dir ? JS_TRUE : JS_FALSE;
      break;
    case PJSURL_UNSUP_URLSTR:
      p = up->urlp->p.unsup.urlstr;
      SET_STR(p);
      break;
    case PJSURL_CHECK_LEVEL:
      *val = INT_TO_JSVAL(up->condp->level);
      break;
    case PJSURL_MIME_TYPE:
      SET_STR(up->condp->mimet);
      break;
    case PJSURL_DOC_SIZE:
      *val = INT_TO_JSVAL(up->condp->size);
      break;
    case PJSURL_MDTM:
      *val = INT_TO_JSVAL(up->condp->time);
      break;
    case PJSURL_DOCNR:
      *val = INT_TO_JSVAL(up->condp->urlnr);
      break;
    case PJSURL_HTML_TAG:
      SET_STR(up->condp->full_tag);
      break;
    case PJSURL_TAG:
      SET_STR(up->condp->tag);
      break;
    case PJSURL_ATTRIB:
      SET_STR(up->condp->attrib);
      break;
    case PJSURL_HTML_DOC:
      SET_STR(up->condp->html_doc);
      break;
    case PJSURL_HTML_DOC_OFFSET:
      *val = INT_TO_JSVAL(up->condp->html_doc_offset);
      break;
    case PJSURL_MOVED:
      if(up->urlp->moved_to)
      {
        JSObject *robj;
        robj = pjs_url_to_jsobj(cx, up->urlp->moved_to, NULL);
        *val = OBJECT_TO_JSVAL(robj);
      }
      else
        *val = JSVAL_NULL;
      break;
    }
  }

  return JS_TRUE;
}

static JSBool pjs_url_set_property(JSContext *cx, JSObject *obj, jsval id, jsval *val)
{
  if(JSVAL_IS_INT(id))
  {
    url_priv_t *up;
    int32 status;

    up = JS_GetPrivate(cx, obj);

    switch (JSVAL_TO_INT(id))
    {
    case PJSURL_STATUS:
      if(!JS_ValueToInt32(cx, *val, &status))
      {
        JS_ReportError(cx, "PavukUrl.status: %s\n",
          gettext("invalid assignment"));
        return JS_FALSE;
      }
      up->urlp->status = status;
      break;
    }
  }

  return JS_TRUE;
}

static JSBool pjs_url_get_parent(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rv)
{
  int n;
  url *rurl;
  url_priv_t *up;
  dllist *ptr;
  JSObject *robj;

  if(argc != 1 || !JSVAL_IS_INT(argv[0]))
  {
    JS_ReportError(cx, "PavukUrl.get_parent: %s\n",
      gettext("bad parent index"));
    return JS_FALSE;
  }

  n = JSVAL_TO_INT(argv[0]);
  up = JS_GetPrivate(cx, obj);

  ptr = dllist_nth(up->urlp->parent_url, n);

  if(ptr)
  {
    rurl = (url *) ptr->data;

    robj = pjs_url_to_jsobj(cx, rurl, NULL);

    *rv = OBJECT_TO_JSVAL(robj);
  }
  else
    *rv = JSVAL_NULL;

  return JS_TRUE;
}

static JSBool pjs_url_check_cond(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rv)
{
  url_priv_t *up;
  char *name;
  JSString *s;
  int r, i;
  cond_info_t condp;

  condp.level = 0;
  condp.urlnr = 0;
  condp.size = 0;
  condp.time = 0L;
  condp.mimet = NULL;
  condp.params = NULL;

  up = JS_GetPrivate(cx, obj);

  if(up->condp)
  {
    condp.level = up->condp->level;
    condp.urlnr = up->condp->urlnr;
    condp.size = up->condp->size;
    condp.time = up->condp->time;
    condp.mimet = up->condp->mimet;
    condp.params = up->condp->params;
  }

  if(argc < 1)
  {
    JS_ReportError(cx, "PavukUrl.check_cond: %s\n",
      gettext("not enough parameters"));
    return JS_FALSE;
  }

  s = JS_ValueToString(cx, argv[0]);
  name = JS_GetStringBytes(s);

  for(i = 1; i < argc; i++)
  {
    char *p;

    s = JS_ValueToString(cx, argv[i]);
    p = JS_GetStringBytes(s);

    if(p)
      condp.params = dllist_append(condp.params, p);
  }

  r = url_append_one_condition(name, up->urlp, &condp);

  dllist_free_all(condp.params);

  if(r < 0)
  {
    JS_ReportError(cx, "pjs: PavukUrl.check_cond: %s - \"%s\"",
      gettext("unknown limiting condition"), name);

    return JS_FALSE;
  }

  *rv = r ? JSVAL_TRUE : JSVAL_FALSE;

  return JS_TRUE;
}

static JSObject *pjs_url_class_init(JSContext *cx, JSObject *obj)
{
  JSObject *prot;

  prot = JS_InitClass(cx, obj, NULL, &urlClass, pjs_url_new, 0,
    urlProperties, urlMethods, NULL, NULL);

  return prot;
}

/* PavukFnrules class implementation */
static JSBool pjs_fnrules_get_macro(JSContext *, JSObject *, uintN, jsval *,
  jsval *);
static JSBool pjs_fnrules_get_sub(JSContext *, JSObject *, uintN, jsval *,
  jsval *);
static JSBool pjs_fnrules_get_property(JSContext *, JSObject *, jsval,
  jsval *);

static JSClass fnrulesClass = {
  "PavukFnrules", JSCLASS_HAS_PRIVATE,
  JS_PropertyStub, JS_PropertyStub,
  pjs_fnrules_get_property, JS_PropertyStub,
  JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub
};

static JSFunctionSpec fnrulesMethods[] = {
  {"get_macro", pjs_fnrules_get_macro, 1, JSPROP_ENUMERATE},
  {"get_sub", pjs_fnrules_get_sub, 1, JSPROP_ENUMERATE},
  {NULL, NULL, 0, 0}
};

enum _fnrules_prop
{
  PJSFNRULES_URL,
  PJSFNRULES_PATTERN,
  PJSFNRULES_PATTERN_TYPE,

  PJSFNRULES_NULL
};

static JSPropertySpec fnrulesProperties[] = {
  {"url", PJSFNRULES_URL, JSPROP_ENUMERATE | JSPROP_READONLY},
  {"pattern", PJSFNRULES_PATTERN, JSPROP_ENUMERATE | JSPROP_READONLY},
  {"pattern_type", PJSFNRULES_PATTERN_TYPE,
      JSPROP_ENUMERATE | JSPROP_READONLY},
  {NULL, 0, 0},
};

static JSObject *pjs_fnrules_to_jsobj(JSContext *cx, struct lfname_lsp_interp *interp)
{
  JSObject *obj;

  obj = JS_NewObject(cx, &fnrulesClass, NULL, NULL);
  JS_SetPrivate(cx, obj, interp);

  return obj;
}

/* constructor */
static JSBool pjs_fnrules_new(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rv)
{
  JS_ReportError(cx,
    gettext("PavukFnrules constructor call prohibited from script\n"));
  return JS_TRUE;
}

static JSBool pjs_fnrules_get_property(JSContext *cx, JSObject *obj, jsval id, jsval *val)
{
  if(JSVAL_IS_INT(id))
  {
    struct lfname_lsp_interp *interp;
    JSObject *uobj;

    interp = JS_GetPrivate(cx, obj);

    switch (JSVAL_TO_INT(id))
    {
    case PJSFNRULES_URL:
      uobj = pjs_url_to_jsobj(cx, interp->urlp, NULL);
      *val = OBJECT_TO_JSVAL(uobj);
      break;
    case PJSFNRULES_PATTERN:
      SET_STR(interp->orig->matchstr);
      break;
    case PJSFNRULES_PATTERN_TYPE:
      *val = INT_TO_JSVAL(interp->orig->type);
      break;
    }
  }
  return JS_TRUE;
}

static JSBool pjs_fnrules_get_macro(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *val)
{
  struct lfname_lsp_interp *interp;
  const char *v;
  JSString *s;

  if(argc != 1 || !(s = JS_ValueToString(cx, argv[0])))
  {
    JS_ReportError(cx, "PavukFnrules.get_macro: %s\n",
      gettext("bad parameters"));
    return JS_FALSE;
  }

  interp = JS_GetPrivate(cx, obj);
  v = JS_GetStringBytes(s);

  if(v && v[0] == '%' && lfname_check_macro(v[1]) && !v[2])
    v = lfname_interp_get_macro(interp, v[1]);

  SET_STR(v);

  return JS_TRUE;
}

static JSBool pjs_fnrules_get_sub(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *val)
{
  struct lfname_lsp_interp *interp;
  char *v;
  int32 n;

  if(argc != 1 || !JSVAL_IS_INT(argv[0]))
  {
    JS_ReportError(cx, "PavukFnrules.get_sub: %s\n",
      gettext("bad parameters"));
    return JS_FALSE;
  }

  interp = JS_GetPrivate(cx, obj);

  JS_ValueToInt32(cx, argv[0], &n);

  v = lfname_re_sub(interp->orig, interp->urlstr, n);

  SET_STR(v);

  return JS_TRUE;
}

static JSObject *pjs_fnrules_class_init(JSContext *cx, JSObject *obj)
{
  JSObject *prot;

  prot = JS_InitClass(cx, obj, NULL, &fnrulesClass, pjs_fnrules_new, 0,
    fnrulesProperties, fnrulesMethods, NULL, NULL);

  return prot;
}

static int pjs_func_exist(JSContext *cx, JSObject *obj, char *name)
{
  jsval rv;

  if(!JS_GetProperty(cx, obj, name, &rv) || rv == JSVAL_VOID)
    return FALSE;
  else
    return TRUE;
}

int pjs_init(void)
{
  LOCK_MOZJS;

  if(!rt)
  {
    rt = JS_Init(1000000L);
    cx = JS_NewContext(rt, STACK_CHUNK_SIZE);
    JS_SetErrorReporter(cx, pjs_error);
    go = JS_NewObject(cx, &globalClass, NULL, NULL);
    JS_InitStandardClasses(cx, go);
    JS_DefineFunctions(cx, go, globalMethods);

    pjs_url_class_init(cx, go);
    pjs_fnrules_class_init(cx, go);

    pjs_load_script(priv_cfg.js_script_file);

    pjs_have_pavuk_url_cond_check =
      pjs_func_exist(cx, go, "pavuk_url_cond_check");
  }

  UNLOCK_MOZJS;

  return 0;
}

int pjs_destroy(void)
{
  LOCK_MOZJS;

  if(rt)
  {
    if(script)
    {
      JS_RemoveRoot(cx, &scriptobj);
      JS_DestroyScript(cx, script);
    }
    JS_GC(cx);
    JS_DestroyContext(cx);
    JS_Finish(rt);

    cx = NULL;
    rt = NULL;
    go = NULL;
    script = NULL;
    scriptobj = NULL;
    pjs_have_pavuk_url_cond_check = FALSE;
  }

  UNLOCK_MOZJS;

  return 0;
}

int pjs_load_script(char *name)
{
  jsval rv;

  JS_ClearPendingException(cx);
  if(script)
  {
    JS_RemoveRoot(cx, &scriptobj);
    JS_DestroyScript(cx, script);
    script = NULL;
    scriptobj = NULL;
  }

  if(name)
  {
    script = JS_CompileFile(cx, go, name);

    if(!script)
      return -1;

    scriptobj = JS_NewScriptObject(cx, script);
    JS_AddNamedRoot(cx, &scriptobj, "pjs_script");

    JS_ExecuteScript(cx, go, script, &rv);
  }

  return 0;
}

int pjs_load_script_string(char *str)
{
  jsval rv;

  LOCK_MOZJS;
  JS_ClearPendingException(cx);
  if(script)
  {
    JS_RemoveRoot(cx, &scriptobj);
    JS_DestroyScript(cx, script);
    script = NULL;
    scriptobj = NULL;
  }

  script = JS_CompileScript(cx, go, str, strlen(str), "intbuffer", 0);
  if(!script)
  {
    UNLOCK_MOZJS;
    return -1;
  }

  scriptobj = JS_NewScriptObject(cx, script);
  JS_AddNamedRoot(cx, &scriptobj, "pjs_script");

  JS_ExecuteScript(cx, go, script, &rv);

  UNLOCK_MOZJS;
  return 0;
}

int pjs_execute(char *scr)
{
  jsval rval;
  int rv;

  LOCK_MOZJS;
  rv = JS_EvaluateScript(cx, go, scr, strlen(scr), "pjs", 0, &rval);
  UNLOCK_MOZJS;

  return rv;
}

int pjs_run_cond_check_func(url *urlp, cond_info_t *condp)
{
  jsval param[2];
  jsval rv;
  JSObject *ourl;
  int ret;

  if(!pjs_have_pavuk_url_cond_check || !script)
    return -1;

  LOCK_MOZJS;

  ourl = pjs_url_to_jsobj(cx, urlp, condp);
  param[0] = OBJECT_TO_JSVAL(ourl);
  param[1] = INT_TO_JSVAL(condp->level);

  rv = JSVAL_FALSE;

  if(!JS_CallFunctionName(cx, go, "pavuk_url_cond_check", 2, param, &rv))
    ret = -1;
  else
  {
    if(JSVAL_IS_BOOLEAN(rv))
      ret = (rv == JSVAL_TRUE);
    else
      ret = -1;
  }

  JS_GC(cx);

  UNLOCK_MOZJS;

  return ret;
}

char *pjs_run_fnrules_func(char *name, struct lfname_lsp_interp *interp)
{
  char *ret = NULL;
  JSObject *rl;
  jsval param[1];
  JSString *s;
  jsval rv;

  LOCK_MOZJS;
  if(pjs_func_exist(cx, go, name))
  {
    rl = pjs_fnrules_to_jsobj(cx, interp);
    param[0] = OBJECT_TO_JSVAL(rl);

    rv = JSVAL_FALSE;

    if(JS_CallFunctionName(cx, go, name, 1, param, &rv))
    {
      s = JS_ValueToString(cx, rv);
      if(s)
        ret = tl_strdup(JS_GetStringBytes(s));
    }

    JS_GC(cx);
  }
  else
  {
    xprintf(1, gettext("pjs: There is no \"%s\" JavaScript function!\n"),
      name);
  }
  UNLOCK_MOZJS;

  return ret;
}

#endif /* HAVE_MOZJS */
