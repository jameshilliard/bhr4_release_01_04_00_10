/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include "config.h"

#include <string.h>
#include <unistd.h>

#include "form.h"
#include "url.h"
#include "tools.h"

/* for hexadecimal encoding */
#define HEXASC2HEXNR(x) (((x) >= '0' && (x) <= '9') ? \
      ((x) - '0') : (tl_ascii_toupper(x) - 'A' + 10))

#define HEX2CHAR(x) (HEXASC2HEXNR(*(x + 1)) << 4) + HEXASC2HEXNR(*(x + 2))

dllist *form_parse_urlencoded_query(char *str)
{
  char *p, *p2, *p3;
  char *name, *value;
  dllist *rv = NULL;

  p = str;
  for(p2 = str, p = strchr(str, '&');; p = strchr(p2, '&'))
  {
    name = NULL;
    value = NULL;

    if(p)
      *p = '\0';

    p3 = strchr(p2, '=');
    if(p3)
    {
      *p3 = '\0';
      p3++;
      value = form_decode_urlencoded_str(p3, strlen(p3));
    }
    name = form_decode_urlencoded_str(p2, strlen(p2));

    rv = dllist_append(rv, (dllist_t) form_field_new(name, value));

    _free(name);
    _free(value);

    if(p)
      p2 = p + 1;
    else
      break;
  }

  return rv;
}

char *form_decode_urlencoded_str(char *str, int len)
{
  char *res, *r;
  int i;

  if(str == NULL)
    return NULL;

  res = tl_strndup(str, len);

  for(i = 0, r = res; i < len; r++, i++)
  {
    if(str[i] == '+')
    {
      *r = ' ';
    }
    else if(str[i] == '%' && str[i + 1] && str[i + 2] &&
      tl_ascii_isxdigit(str[i + 1]) && tl_ascii_isxdigit(str[i + 2]))
    {
      *r = HEX2CHAR(str + i);
      i += 2;
    }
    else
    {
      *r = str[i];
    }
  }
  *r = '\0';
  return res;
}

char *form_encode_urlencoded_str(char *str)
{
  unsigned char *retv, *r, *p;
  char hexa[] = "0123456789ABCDEF";

  if(!str)
    return NULL;

  retv = malloc(3 * strlen(str) + 1);

  for(p = (unsigned char *) str, r = retv; *p; p++, r++)
  {
    if(*p == ' ')
    {
      *r = '+';
    }
    else if(strchr(URL_RQUERY_UNSAFE, *p) || (*p > 0x7f) || (*p < 0x20))
    {
      *r = '%';
      r++;
      *r = hexa[*p >> 4];
      r++;
      *r = hexa[*p % 16];
    }
    else
    {
      *r = *p;
    }
  }
  *r = '\0';

  return (char *) retv;
}

char *form_encode_urlencoded(dllist *fdata)
{
  char *retv = NULL;
  dllist *ptr = fdata;
  form_field *f;
  char *n, *v;

  while(ptr)
  {
    f = (form_field *) ptr->data;
    n = form_encode_urlencoded_str(f->name);
    v = form_encode_urlencoded_str(f->value);
    if(!retv)
      retv = tl_str_concat(retv, n, "=", v, NULL);
    else
      retv = tl_str_concat(retv, "&", n, "=", v, NULL);
    _free(n);
    _free(v);
    ptr = ptr->next;
  }

  return retv;
}

#define BOUNDARY_LEN 128
static char boundaryset[] =
  "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

char *form_encode_multipart_boundary()
{
  char boundarystr[BOUNDARY_LEN + 1];
  int i;

  memset(boundarystr, '\0', BOUNDARY_LEN + 1);
  strcpy(boundarystr, PACKAGE);
  for(i = strlen(boundarystr); i < BOUNDARY_LEN; i++)
  {
    boundarystr[i] = boundaryset[rand() % (sizeof(boundaryset) - 1)];
  }

  return tl_strdup(boundarystr);
}

char *form_encode_multipart(dllist *fdata, char *boundary, int *len)
{
  char *retv = NULL;
  dllist *ptr = fdata;
  form_field *f;

  *len = 0;

  while(ptr)
  {
    f = (form_field *) ptr->data;

    if(f->type == FORM_T_FILE)
    {
      int fd;
      char buf[512];
      int tmplen;
      char *tp;

      tp = strrchr(f->value, '/');
      if(!tp)
        tp = f->value;

      retv = tl_data_concat_str(len, retv, "--", boundary,
        "\r\nContent-Disposition: form-data; name=\"",
        f->name, "\"; filename=\"", tp,
        "\"\r\nContent-Type: application/octet-stream\r\n"
        "Content-Transfer-Encoding: binary\r\n\r\n", NULL);

      if((fd = open(f->value, O_BINARY | O_RDONLY)) != -1)
      {
        while((tmplen = read(fd, buf, sizeof(buf))) > 0)
        {
          retv = tl_data_concat_data(len, retv, tmplen, buf);
        }

        close(fd);

        if(*len < 0)
        {
          xperror(f->value);
          _free(retv);
          *len = 0;
          break;
        }
        retv = tl_data_concat_str(len, retv, "\r\n", NULL);
      }
      else
      {
        xperror(f->value);
        _free(retv);
        *len = 0;
        break;
      }
    }
    else
    {
      retv = tl_data_concat_str(len, retv, "--", boundary,
        "\r\nContent-Disposition: form-data; name=\"",
        f->name, "\"\r\n\r\n", f->value, "\r\n", NULL);
    }
    ptr = ptr->next;
    if(!ptr)
      retv = tl_data_concat_str(len, retv, "--", boundary, "--\r\n", NULL);
  }

  return retv;
}

char *form_encode_query(form_info *finfo, int *len)
{
  char *retv;

  *len = 0;

  if(finfo->encoding == FORM_E_MULTIPART)
  {
    retv = form_encode_multipart(finfo->infos, finfo->text, len);
  }
  else
  {
    retv = form_encode_urlencoded(finfo->infos);
    *len = strlen(retv);
  }

  return retv;
}
