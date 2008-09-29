//getobjectbyX.c:

/*
    copyright 2008 Philipp 'ph3-der-loewe' Schafft <lion@lion.leolix.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    version 3.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
*/

#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <netdb.h>

#include <netdnet/dn.h>
#include <netdnet/dnetdb.h>

static char * _dnet_objhinum_string   = NULL;
static int    _dnet_objhinum_handling = DNOBJHINUM_ERROR;

struct {
 int    num;
 char * name;
} _dnet_objdb[] = {
 { 17, "FAL"   },
 { 19, "NML"   },
 { 19, "NICE"  }, // alias!
 { 23, "DTERM" },
 { 25, "MIRROR"},
 { 26, "EVR"   },
 { 27, "MAIL"  },
 { 27, "MAIL11"}, // alias!
 { 29, "PHONE" },
 { 42, "CTERM" },
 { 63, "DTR"   },
 { -1, NULL}      // END OF LIST
};

int dnet_checkobjectnumber (int num) {

 if ( _dnet_objhinum_string == NULL ) {
  if ( (_dnet_objhinum_string = getenv(DNOBJ_HINUM_ENV)) == NULL )
   _dnet_objhinum_string = DNOBJ_HINUM_DEF;

  if ( !*_dnet_objhinum_string )
   _dnet_objhinum_string = DNOBJ_HINUM_DEF;

  if ( !strcasecmp(_dnet_objhinum_string, "error") ) {
   _dnet_objhinum_handling = DNOBJHINUM_ERROR; // error case
  } else if ( !strcasecmp(_dnet_objhinum_string, "zero") ) {
   _dnet_objhinum_handling = DNOBJHINUM_ZERO; // return as object number 0
  } else if ( !strcasecmp(_dnet_objhinum_string, "return") ) {
   _dnet_objhinum_handling = DNOBJHINUM_RETURN; // return as object number unchanged
  } else if ( !strcasecmp(_dnet_objhinum_string, "alwayszero") ) {
   _dnet_objhinum_handling = DNOBJHINUM_ALWAYSZERO; // specal case to prevent app from using numbered objects
  }
 }

 if ( _dnet_objhinum_handling == DNOBJHINUM_ALWAYSZERO && num != -1 )
  return 0;

 if ( num < 256 )
  return num;

 switch (_dnet_objhinum_handling) {
  case DNOBJHINUM_ERROR:
   errno = EINVAL;
   return -1;
  case DNOBJHINUM_ZERO:
   return 0;
  case DNOBJHINUM_RETURN:
   return num;
  default:
   errno = ENOSYS;
   return -1;
 }
}

int getobjectbyname_nis(char * name) {
 char           * cur, *next;
 char             proto[16];
 struct servent * se;
 static char    * search_order = NULL;

 if ( search_order == NULL ) {
  if ( (search_order = getenv(DNOBJ_SEARCH_ENV)) == NULL )
   search_order = DNOBJ_SEARCH_DEF;
  if ( !*search_order )
   search_order = DNOBJ_SEARCH_DEF;
 }

 cur = search_order;

 while (cur && *cur) {
  if ( (next = strstr(cur, " ")) == NULL ) {
   strncpy(proto, cur, 16);
  } else {
   strncpy(proto, cur, next-cur);
   proto[next-cur] = 0;
   next++;
  }
  cur = next;

  if ( (se = getservbyname(name, proto)) != NULL ) {
   return ntohs(se->s_port);
  }
 }

 errno = ENOENT;
 return -1;
}

char * getobjectbynumber_nis(int num) {
 char           * cur, *next;
 char             proto[16];
 struct servent * se;
 static char    * search_order = NULL;

 if ( search_order == NULL ) {
  if ( (search_order = getenv(DNOBJ_SEARCH_ENV)) == NULL )
   search_order = DNOBJ_SEARCH_DEF;
  if ( !*search_order )
   search_order = DNOBJ_SEARCH_DEF;
 }

 cur = search_order;

 num = htons(num);

 while (cur && *cur) {
  if ( (next = strstr(cur, " ")) == NULL ) {
   strncpy(proto, cur, 16);
  } else {
   strncpy(proto, cur, next-cur);
   proto[next-cur] = 0;
   next++;
  }
  cur = next;

  if ( (se = getservbyport(num, proto)) != NULL ) {
   return se->s_name;
  }
 }

 errno = ENOENT;
 return NULL;
}

int getobjectbyname_static(char * name) {
 int i;

 for (i = 0; _dnet_objdb[i].num != -1; i++) {
  if ( !strcasecmp(name, _dnet_objdb[i].name) )
   return _dnet_objdb[i].num;
 }

 errno = ENOENT;
 return -1;
}

char * getobjectbynumber_static(int num) {
 int i;

 for (i = 0; _dnet_objdb[i].num != -1; i++) {
  if ( _dnet_objdb[i].num == num )
   return _dnet_objdb[i].name;
 }

 errno = ENOENT;
 return NULL;
}

int getobjectbyname(char * name) {
 int num;
 int old_errno = errno;

 if ( (num = getobjectbyname_nis(name)) == -1 )
  num = getobjectbyname_static(name);

 if ( num != -1 )
  errno = old_errno;

 return dnet_checkobjectnumber(num);
}

int getobjectbynumber(int number, char * name, size_t name_len) {
 int num;
 char * rname = NULL;
 int old_errno = errno;

 if ( (num = dnet_checkobjectnumber(number)) == -1 ) { // errno is set correctly after this call
  return -1;
 }

 if ( name_len < 2 ) {
  errno = EINVAL;
  return -1;
 }

 if ( (rname = getobjectbynumber_nis(number)) == NULL )
  rname = getobjectbynumber_static(number);

 if ( rname == NULL ) {
  errno = ENOENT;
  return -1;
 }

 errno = old_errno;

 strncpy(name, rname, name_len-1);
 name[name_len-1] = 0;

 return num;
}

//ll
