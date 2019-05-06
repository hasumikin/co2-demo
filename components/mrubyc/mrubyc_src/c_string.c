/*! @file
  @brief
  mruby/c String object

  <pre>
  Copyright (C) 2015-2018 Kyushu Institute of Technology.
  Copyright (C) 2015-2018 Shimane IT Open-Innovation Center.

  This file is distributed under BSD 3-Clause License.

  </pre>
*/

#include "vm_config.h"
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <assert.h>

#include "value.h"
#include "vm.h"
#include "alloc.h"
#include "static.h"
#include "class.h"
#include "symbol.h"
#include "c_array.h"
#include "c_string.h"
#include "console.h"


#if MRBC_USE_STRING
//================================================================
/*! white space character test

  @param  ch	character code.
  @return	result.
*/
static int is_space( int ch )
{
  static const char ws[] = " \t\r\n\f\v";	// '\0' on tail

  int i;
  for( i = 0; i < sizeof(ws); i++ ) {
    if( ch == ws[i] ) return 1;
  }
  return 0;
}


//================================================================
/*! constructor

  @param  vm	pointer to VM.
  @param  src	source string or NULL
  @param  len	source length
  @return 	string object
*/
mrbc_value mrbc_string_new(struct VM *vm, const void *src, int len)
{
  mrbc_value value = {.tt = MRBC_TT_STRING};

  /*
    Allocate handle and string buffer.
  */
  mrbc_string *h;
  h = (mrbc_string *)mrbc_alloc(vm, sizeof(mrbc_string));
  if( !h ) return value;		// ENOMEM

  uint8_t *str = mrbc_alloc(vm, len+1);
  if( !str ) {				// ENOMEM
    mrbc_raw_free( h );
    return value;
  }

  h->ref_count = 1;
  h->tt = MRBC_TT_STRING;	// TODO: for DEBUG
  h->size = len;
  h->data = str;

  /*
    Copy a source string.
  */
  if( src == NULL ) {
    str[0] = '\0';
  } else {
    memcpy( str, src, len );
    str[len] = '\0';
  }

  value.string = h;
  return value;
}


//================================================================
/*! constructor by c string

  @param  vm	pointer to VM.
  @param  src	source string or NULL
  @return 	string object
*/
mrbc_value mrbc_string_new_cstr(struct VM *vm, const char *src)
{
  return mrbc_string_new(vm, src, (src ? strlen(src) : 0));
}


//================================================================
/*! constructor by allocated buffer

  @param  vm	pointer to VM.
  @param  buf	pointer to allocated buffer
  @param  len	length
  @return 	string object
*/
mrbc_value mrbc_string_new_alloc(struct VM *vm, void *buf, int len)
{
  mrbc_value value = {.tt = MRBC_TT_STRING};

  /*
    Allocate handle
  */
  mrbc_string *h;
  h = (mrbc_string *)mrbc_alloc(vm, sizeof(mrbc_string));
  if( !h ) return value;		// ENOMEM

  h->ref_count = 1;
  h->tt = MRBC_TT_STRING;	// TODO: for DEBUG
  h->size = len;
  h->data = buf;

  value.string = h;
  return value;
}


//================================================================
/*! destructor

  @param  str	pointer to target value
*/
void mrbc_string_delete(mrbc_value *str)
{
  mrbc_raw_free(str->string->data);
  mrbc_raw_free(str->string);
}



//================================================================
/*! clear vm_id
*/
void mrbc_string_clear_vm_id(mrbc_value *str)
{
  mrbc_set_vm_id( str->string, 0 );
  mrbc_set_vm_id( str->string->data, 0 );
}


//================================================================
/*! duplicate string

  @param  vm	pointer to VM.
  @param  s1	pointer to target value 1
  @param  s2	pointer to target value 2
  @return	new string as s1 + s2
*/
mrbc_value mrbc_string_dup(struct VM *vm, mrbc_value *s1)
{
  mrbc_string *h1 = s1->string;

  mrbc_value value = mrbc_string_new(vm, NULL, h1->size);
  if( value.string == NULL ) return value;		// ENOMEM

  memcpy( value.string->data, h1->data, h1->size + 1 );

  return value;
}


//================================================================
/*! add string (s1 + s2)

  @param  vm	pointer to VM.
  @param  s1	pointer to target value 1
  @param  s2	pointer to target value 2
  @return	new string as s1 + s2
*/
mrbc_value mrbc_string_add(struct VM *vm, const mrbc_value *s1, const mrbc_value *s2)
{
  mrbc_string *h1 = s1->string;
  mrbc_string *h2 = s2->string;

  mrbc_value value = mrbc_string_new(vm, NULL, h1->size + h2->size);
  if( value.string == NULL ) return value;		// ENOMEM

  memcpy( value.string->data,            h1->data, h1->size );
  memcpy( value.string->data + h1->size, h2->data, h2->size + 1 );

  return value;
}


//================================================================
/*! append string (s1 += s2)

  @param  s1	pointer to target value 1
  @param  s2	pointer to target value 2
  @param	mrbc_error_code
*/
int mrbc_string_append(mrbc_value *s1, const mrbc_value *s2)
{
  int len1 = s1->string->size;
  int len2 = (s2->tt == MRBC_TT_STRING) ? s2->string->size : 1;

  uint8_t *str = mrbc_raw_realloc(s1->string->data, len1+len2+1);
  if( !str ) return E_NOMEMORY_ERROR;

  if( s2->tt == MRBC_TT_STRING ) {
    memcpy(str + len1, s2->string->data, len2 + 1);
  } else if( s2->tt == MRBC_TT_FIXNUM ) {
    str[len1] = s2->i;
    str[len1+1] = '\0';
  }

  s1->string->size = len1 + len2;
  s1->string->data = str;

  return 0;
}


//================================================================
/*! append c string (s1 += s2)

  @param  s1	pointer to target value 1
  @param  s2	pointer to char (c_str)
  @param	mrbc_error_code
*/
int mrbc_string_append_cstr(mrbc_value *s1, const char *s2)
{
  int len1 = s1->string->size;
  int len2 = strlen(s2);

  uint8_t *str = mrbc_raw_realloc(s1->string->data, len1+len2+1);
  if( !str ) return E_NOMEMORY_ERROR;

  memcpy(str + len1, s2, len2 + 1);

  s1->string->size = len1 + len2;
  s1->string->data = str;

  return 0;
}


//================================================================
/*! locate a substring in a string

  @param  src		pointer to target string
  @param  pattern	pointer to substring
  @param  offset	search offset
  @return		position index. or minus value if not found.
*/
int mrbc_string_index(const mrbc_value *src, const mrbc_value *pattern, int offset)
{
  char *p1 = mrbc_string_cstr(src) + offset;
  char *p2 = mrbc_string_cstr(pattern);
  int try_cnt = mrbc_string_size(src) - mrbc_string_size(pattern) - offset;

  while( try_cnt >= 0 ) {
    if( memcmp( p1, p2, mrbc_string_size(pattern) ) == 0 ) {
      return p1 - mrbc_string_cstr(src);	// matched.
    }
    try_cnt--;
    p1++;
  }

  return -1;
}


//================================================================
/*! remove the whitespace in myself

  @param  src	pointer to target value
  @param  mode	1:left-side, 2:right-side, 3:each
  @return	0 when not removed.
*/
int mrbc_string_strip(mrbc_value *src, int mode)
{
  char *p1 = mrbc_string_cstr(src);
  char *p2 = p1 + mrbc_string_size(src) - 1;

  // left-side
  if( mode & 0x01 ) {
    while( p1 <= p2 ) {
      if( *p1 == '\0' ) break;
      if( !is_space(*p1) ) break;
      p1++;
    }
  }
  // right-side
  if( mode & 0x02 ) {
    while( p1 <= p2 ) {
      if( !is_space(*p2) ) break;
      p2--;
    }
  }

  int new_size = p2 - p1 + 1;
  if( mrbc_string_size(src) == new_size ) return 0;

  char *buf = mrbc_string_cstr(src);
  if( p1 != buf ) memmove( buf, p1, new_size );
  buf[new_size] = '\0';
  mrbc_raw_realloc(buf, new_size+1);	// shrink suitable size.
  src->string->size = new_size;

  return 1;
}


//================================================================
/*! remove the CR,LF in myself

  @param  src	pointer to target value
  @return	0 when not removed.
*/
int mrbc_string_chomp(mrbc_value *src)
{
  char *p1 = mrbc_string_cstr(src);
  char *p2 = p1 + mrbc_string_size(src) - 1;

  if( *p2 == '\n' ) {
    p2--;
  }
  if( *p2 == '\r' ) {
    p2--;
  }

  int new_size = p2 - p1 + 1;
  if( mrbc_string_size(src) == new_size ) return 0;

  char *buf = mrbc_string_cstr(src);
  buf[new_size] = '\0';
  src->string->size = new_size;

  return 1;
}



//================================================================
/*! (method) +
*/
static void c_string_add(struct VM *vm, mrbc_value v[], int argc)
{
  if( v[1].tt != MRBC_TT_STRING ) {
    console_print( "Not support STRING + Other\n" );
    return;
  }

  mrbc_value value = mrbc_string_add(vm, &v[0], &v[1]);
  SET_RETURN(value);
}



//================================================================
/*! (method) *
*/
static void c_string_mul(struct VM *vm, mrbc_value v[], int argc)
{
  if( v[1].tt != MRBC_TT_FIXNUM ) {
    console_print( "TypeError\n" );	// raise?
    return;
  }

  if( v[1].i < 0 ) {
    console_printf( "ArgumentError\n" );	// raise?
    return;
  }

  mrbc_value value = mrbc_string_new(vm, NULL,
				    mrbc_string_size(&v[0]) * v[1].i);
  if( value.string == NULL ) return;		// ENOMEM

  uint8_t *p = value.string->data;
  int i;
  for( i = 0; i < v[1].i; i++ ) {
    memcpy( p, mrbc_string_cstr(&v[0]), mrbc_string_size(&v[0]) );
    p += mrbc_string_size(&v[0]);
  }
  *p = 0;

  SET_RETURN(value);
}



//================================================================
/*! (method) size, length
*/
static void c_string_size(struct VM *vm, mrbc_value v[], int argc)
{
  mrbc_int size = mrbc_string_size(&v[0]);

  SET_INT_RETURN( size );
}



//================================================================
/*! (method) to_i
*/
static void c_string_to_i(struct VM *vm, mrbc_value v[], int argc)
{
  int base = 10;
  if( argc ) {
    base = v[1].i;
    if( base < 2 || base > 36 ) {
      return;	// raise ? ArgumentError
    }
  }

  mrbc_int i = mrbc_atoi( mrbc_string_cstr(v), base );

  SET_INT_RETURN( i );
}


#if MRBC_USE_FLOAT
//================================================================
/*! (method) to_f
*/
static void c_string_to_f(struct VM *vm, mrbc_value v[], int argc)
{
  mrbc_float d = atof(mrbc_string_cstr(v));

  SET_FLOAT_RETURN( d );
}
#endif


//================================================================
/*! (method) <<
*/
static void c_string_append(struct VM *vm, mrbc_value v[], int argc)
{
  if( !mrbc_string_append( &v[0], &v[1] ) ) {
    // raise ? ENOMEM
  }
}


//================================================================
/*! (method) []
*/
static void c_string_slice(struct VM *vm, mrbc_value v[], int argc)
{
  mrbc_value *v1 = &v[1];
  mrbc_value *v2 = &v[2];

  /*
    in case of slice(nth) -> String | nil
  */
  if( argc == 1 && v1->tt == MRBC_TT_FIXNUM ) {
    int len = v->string->size;
    int idx = v1->i;
    int ch = -1;
    if( idx >= 0 ) {
      if( idx < len ) {
        ch = *(v->string->data + idx);
      }
    } else {
      idx += len;
      if( idx >= 0 ) {
        ch = *(v->string->data + idx);
      }
    }
    if( ch < 0 ) goto RETURN_NIL;

    mrbc_value value = mrbc_string_new(vm, NULL, 1);
    if( !value.string ) goto RETURN_NIL;		// ENOMEM

    value.string->data[0] = ch;
    value.string->data[1] = '\0';
    SET_RETURN(value);
    return;		// normal return
  }

  /*
    in case of slice(nth, len) -> String | nil
  */
  if( argc == 2 && v1->tt == MRBC_TT_FIXNUM && v2->tt == MRBC_TT_FIXNUM ) {
    int len = v->string->size;
    int idx = v1->i;
    if( idx < 0 ) idx += len;
    if( idx < 0 ) goto RETURN_NIL;

    int rlen = (v2->i < (len - idx)) ? v2->i : (len - idx);
						// min( v2->i, (len-idx) )
    if( rlen < 0 ) goto RETURN_NIL;

    mrbc_value value = mrbc_string_new(vm, v->string->data + idx, rlen);
    if( !value.string ) goto RETURN_NIL;		// ENOMEM

    SET_RETURN(value);
    return;		// normal return
  }

  /*
    other case
  */
  console_print( "Not support such case in String#[].\n" );
  return;


 RETURN_NIL:
  SET_NIL_RETURN();
}


//================================================================
/*! (method) []=
*/
static void c_string_insert(struct VM *vm, mrbc_value v[], int argc)
{
  int nth;
  int len;
  mrbc_value *val;

  /*
    in case of self[nth] = val
  */
  if( argc == 2 &&
      v[1].tt == MRBC_TT_FIXNUM &&
      v[2].tt == MRBC_TT_STRING ) {
    nth = v[1].i;
    len = 1;
    val = &v[2];
  }
  /*
    in case of self[nth, len] = val
  */
  else if( argc == 3 &&
	   v[1].tt == MRBC_TT_FIXNUM &&
	   v[2].tt == MRBC_TT_FIXNUM &&
	   v[3].tt == MRBC_TT_STRING ) {
    nth = v[1].i;
    len = v[2].i;
    val = &v[3];
  }
  /*
    other cases
  */
  else {
    console_print( "Not support\n" );
    return;
  }

  int len1 = v->string->size;
  int len2 = val->string->size;
  if( nth < 0 ) nth = len1 + nth;               // adjust to positive number.
  if( len > len1 - nth ) len = len1 - nth;
  if( nth < 0 || nth > len1 || len < 0) {
    console_print( "IndexError\n" );  // raise?
    return;
  }

  uint8_t *str = mrbc_realloc(vm, mrbc_string_cstr(v), len1 + len2 - len + 1);
  if( !str ) return;

  memmove( str + nth + len2, str + nth + len, len1 - nth - len + 1 );
  memcpy( str + nth, mrbc_string_cstr(val), len2 );
  v->string->size = len1 + len2 - len;

  v->string->data = str;
}


//================================================================
/*! (method) chomp
*/
static void c_string_chomp(struct VM *vm, mrbc_value v[], int argc)
{
  mrbc_value ret = mrbc_string_dup(vm, &v[0]);

  mrbc_string_chomp(&ret);

  SET_RETURN(ret);
}


//================================================================
/*! (method) chomp!
*/
static void c_string_chomp_self(struct VM *vm, mrbc_value v[], int argc)
{
  if( mrbc_string_chomp(&v[0]) == 0 ) {
    SET_RETURN( mrbc_nil_value() );
  }
}


//================================================================
/*! (method) dup
*/
static void c_string_dup(struct VM *vm, mrbc_value v[], int argc)
{
  mrbc_value ret = mrbc_string_dup(vm, &v[0]);

  SET_RETURN(ret);
}


//================================================================
/*! (method) index
*/
static void c_string_index(struct VM *vm, mrbc_value v[], int argc)
{
  int index;
  int offset;

  if( argc == 1 ) {
    offset = 0;

  } else if( argc == 2 && v[2].tt == MRBC_TT_FIXNUM ) {
    offset = v[2].i;
    if( offset < 0 ) offset += mrbc_string_size(&v[0]);
    if( offset < 0 ) goto NIL_RETURN;

  } else {
    goto NIL_RETURN;	// raise? ArgumentError
  }

  index = mrbc_string_index(&v[0], &v[1], offset);
  if( index < 0 ) goto NIL_RETURN;

  SET_INT_RETURN(index);
  return;

 NIL_RETURN:
  SET_NIL_RETURN();
}


//================================================================
/*! (method) inspect
*/
static void c_string_inspect(struct VM *vm, mrbc_value v[], int argc)
{
  char buf[10] = "\\x";
  mrbc_value ret = mrbc_string_new_cstr(vm, "\"");
  const unsigned char *s = (const unsigned char *)mrbc_string_cstr(v);
  int i;
  for( i = 0; i < mrbc_string_size(v); i++ ) {
    if( s[i] < ' ' || 0x7f <= s[i] ) {	// tiny isprint()
      buf[2] = "0123456789ABCDEF"[s[i] >> 4];
      buf[3] = "0123456789ABCDEF"[s[i] & 0x0f];
      mrbc_string_append_cstr(&ret, buf);
    } else {
      buf[3] = s[i];
      mrbc_string_append_cstr(&ret, buf+3);
    }
  }
  mrbc_string_append_cstr(&ret, "\"");

  SET_RETURN( ret );
}


//================================================================
/*! (method) ord
*/
static void c_string_ord(struct VM *vm, mrbc_value v[], int argc)
{
  int i = mrbc_string_cstr(v)[0];

  SET_INT_RETURN( i );
}


//================================================================
/*! (method) split
*/
static void c_string_split(struct VM *vm, mrbc_value v[], int argc)
{
  mrbc_value ret = mrbc_array_new(vm, 0);
  if( mrbc_string_size(&v[0]) == 0 ) goto DONE;

  // check limit parameter.
  int limit = 0;
  if( argc >= 2 ) {
    if( v[2].tt != MRBC_TT_FIXNUM ) {
      console_print( "TypeError\n" );     // raise?
      return;
    }
    limit = v[2].i;
    if( limit == 1 ) {
      mrbc_array_push( &ret, &v[0] );
      mrbc_dup( &v[0] );
      goto DONE;
    }
  }

  // check separator parameter.
  mrb_value sep = (argc == 0) ? mrbc_string_new_cstr(vm, " ") : v[1];
  switch( sep.tt ) {
  case MRBC_TT_NIL:
    sep = mrbc_string_new_cstr(vm, " ");
    break;

  case MRBC_TT_STRING:
    break;

  default:
    console_print( "TypeError\n" );     // raise?
    return;
  }

  int flag_strip = (mrbc_string_cstr(&sep)[0] == ' ') &&
		   (mrbc_string_size(&sep) == 1);
  int offset = 0;
  int sep_len = mrbc_string_size(&sep);
  if( sep_len == 0 ) sep_len++;

  while( 1 ) {
    int pos, len;

    if( flag_strip ) {
      for( ; offset < mrbc_string_size(&v[0]); offset++ ) {
	if( !is_space( mrbc_string_cstr(&v[0])[offset] )) break;
      }
      if( offset > mrbc_string_size(&v[0])) break;
    }

    // check limit
    if( limit > 0 && mrbc_array_size(&ret)+1 >= limit ) {
      pos = -1;
      goto SPLIT_ITEM;
    }

    // split by space character.
    if( flag_strip ) {
      pos = offset;
      for( ; pos < mrbc_string_size(&v[0]); pos++ ) {
	if( is_space( mrbc_string_cstr(&v[0])[pos] )) break;
      }
      len = pos - offset;
      goto SPLIT_ITEM;
    }

    // split by each character.
    if( mrbc_string_size(&sep) == 0 ) {
      pos = (offset < mrbc_string_size(&v[0])-1) ? offset : -1;
      len = 1;
      goto SPLIT_ITEM;
    }

    // split by specified character.
    pos = mrbc_string_index( &v[0], &sep, offset );
    len = pos - offset;


  SPLIT_ITEM:
    if( pos < 0 ) len = mrbc_string_size(&v[0]) - offset;

    mrb_value v1 = mrbc_string_new(vm, mrbc_string_cstr(&v[0]) + offset, len);
    mrbc_array_push( &ret, &v1 );

    if( pos < 0 ) break;
    offset = pos + sep_len;
  }

  // remove trailing empty item
  if( limit == 0 ) {
    while( 1 ) {
      int idx = mrbc_array_size(&ret) - 1;
      if( idx < 0 ) break;

      mrb_value v1 = mrbc_array_get( &ret, idx );
      if( mrbc_string_size(&v1) != 0 ) break;

      mrbc_array_remove(&ret, idx);
      mrbc_string_delete( &v1 );
    }
  }

  if( argc == 0 || v[1].tt == MRBC_TT_NIL ) {
    mrbc_string_delete(&sep);
  }

 DONE:
  SET_RETURN( ret );
}


//================================================================
/*! (method) sprintf
*/
static void c_object_sprintf(struct VM *vm, mrbc_value v[], int argc)
{
  static const int BUF_INC_STEP = 32;	// bytes.

  mrbc_value *format = &v[1];
  if( format->tt != MRBC_TT_STRING ) {
    console_printf( "TypeError\n" );	// raise?
    return;
  }

  int buflen = BUF_INC_STEP;
  char *buf = mrbc_alloc(vm, buflen);
  if( !buf ) { return; }	// ENOMEM raise?

  mrbc_printf pf;
  mrbc_printf_init( &pf, buf, buflen, mrbc_string_cstr(format) );

  int i = 2;
  int ret;
  while( 1 ) {
    mrbc_printf pf_bak = pf;
    ret = mrbc_printf_main( &pf );
    if( ret == 0 ) break;	// normal break loop.
    if( ret < 0 ) goto INCREASE_BUFFER;

    if( i > argc ) {console_print("ArgumentError\n"); break;}	// raise?

    // maybe ret == 1
    switch(pf.fmt.type) {
    case 'c':
      if( v[i].tt == MRBC_TT_FIXNUM ) {
	ret = mrbc_printf_char( &pf, v[i].i );
      } else if( v[i].tt == MRBC_TT_STRING ) {
	ret = mrbc_printf_char( &pf, mrbc_string_cstr(&v[i])[0] );
      }
      break;

    case 's':
      if( v[i].tt == MRBC_TT_STRING ) {
	ret = mrbc_printf_bstr( &pf, mrbc_string_cstr(&v[i]), mrbc_string_size(&v[i]),' ');
      } else if( v[i].tt == MRBC_TT_SYMBOL ) {
	ret = mrbc_printf_str( &pf, mrbc_symbol_cstr( &v[i] ), ' ');
      }
      break;

    case 'd':
    case 'i':
    case 'u':
      if( v[i].tt == MRBC_TT_FIXNUM ) {
	ret = mrbc_printf_int( &pf, v[i].i, 10);
#if MRBC_USE_FLOAT
      } else if( v[i].tt == MRBC_TT_FLOAT ) {
	ret = mrbc_printf_int( &pf, (mrbc_int)v[i].d, 10);
#endif
      } else if( v[i].tt == MRBC_TT_STRING ) {
	mrbc_int ival = atol(mrbc_string_cstr(&v[i]));
	ret = mrbc_printf_int( &pf, ival, 10 );
      }
      break;

    case 'b':
    case 'B':
      if( v[i].tt == MRBC_TT_FIXNUM ) {
	ret = mrbc_printf_bit( &pf, v[i].i, 1);
      }
      break;

    case 'x':
    case 'X':
      if( v[i].tt == MRBC_TT_FIXNUM ) {
	ret = mrbc_printf_bit( &pf, v[i].i, 4);
      }
      break;

    case 'o':
      if( v[i].tt == MRBC_TT_FIXNUM ) {
	ret = mrbc_printf_bit( &pf, v[i].i, 3);
      }
      break;

#if MRBC_USE_FLOAT
    case 'f':
    case 'e':
    case 'E':
    case 'g':
    case 'G':
      if( v[i].tt == MRBC_TT_FLOAT ) {
	ret = mrbc_printf_float( &pf, v[i].d );
      } else if( v[i].tt == MRBC_TT_FIXNUM ) {
	ret = mrbc_printf_float( &pf, v[i].i );
      }
      break;
#endif

    default:
      break;
    }
    if( ret >= 0 ) {
      i++;
      continue;		// normal next loop.
    }

    // maybe buffer full. (ret == -1)
    if( pf.fmt.width > BUF_INC_STEP ) buflen += pf.fmt.width;
    pf = pf_bak;

  INCREASE_BUFFER:
    buflen += BUF_INC_STEP;
    buf = mrbc_realloc(vm, pf.buf, buflen);
    if( !buf ) { return; }	// ENOMEM raise? TODO: leak memory.
    mrbc_printf_replace_buffer(&pf, buf, buflen);
  }
  mrbc_printf_end( &pf );

  buflen = mrbc_printf_len( &pf );
  mrbc_realloc(vm, pf.buf, buflen+1);	// shrink suitable size.

  mrbc_value value = mrbc_string_new_alloc( vm, pf.buf, buflen );

  SET_RETURN(value);
}


//================================================================
/*! (method) printf
*/
static void c_object_printf(struct VM *vm, mrbc_value v[], int argc)
{
  c_object_sprintf(vm, v, argc);
  console_nprint( mrbc_string_cstr(v), mrbc_string_size(v) );
  SET_NIL_RETURN();
}


//================================================================
/*! (method) lstrip
*/
static void c_string_lstrip(struct VM *vm, mrbc_value v[], int argc)
{
  mrbc_value ret = mrbc_string_dup(vm, &v[0]);

  mrbc_string_strip(&ret, 0x01);	// 1: left side only

  SET_RETURN(ret);
}


//================================================================
/*! (method) lstrip!
*/
static void c_string_lstrip_self(struct VM *vm, mrbc_value v[], int argc)
{
  if( mrbc_string_strip(&v[0], 0x01) == 0 ) {	// 1: left side only
    SET_RETURN( mrbc_nil_value() );
  }
}


//================================================================
/*! (method) rstrip
*/
static void c_string_rstrip(struct VM *vm, mrbc_value v[], int argc)
{
  mrbc_value ret = mrbc_string_dup(vm, &v[0]);

  mrbc_string_strip(&ret, 0x02);	// 2: right side only

  SET_RETURN(ret);
}


//================================================================
/*! (method) rstrip!
*/
static void c_string_rstrip_self(struct VM *vm, mrbc_value v[], int argc)
{
  if( mrbc_string_strip(&v[0], 0x02) == 0 ) {	// 2: right side only
    SET_RETURN( mrbc_nil_value() );
  }
}


//================================================================
/*! (method) strip
*/
static void c_string_strip(struct VM *vm, mrbc_value v[], int argc)
{
  mrbc_value ret = mrbc_string_dup(vm, &v[0]);

  mrbc_string_strip(&ret, 0x03);	// 3: left and right

  SET_RETURN(ret);
}


//================================================================
/*! (method) strip!
*/
static void c_string_strip_self(struct VM *vm, mrbc_value v[], int argc)
{
  if( mrbc_string_strip(&v[0], 0x03) == 0 ) {	// 3: left and right
    SET_RETURN( mrbc_nil_value() );
  }
}


//================================================================
/*! (method) to_sym
*/
static void c_string_to_sym(struct VM *vm, mrbc_value v[], int argc)
{
  mrbc_value ret = mrbc_symbol_new(vm, mrbc_string_cstr(&v[0]));

  SET_RETURN(ret);
}


//================================================================
/*! (method) tr

  Pattern syntax

  <syntax> ::= (<pattern>)* | '^' (<pattern>)*
  <pattern> ::= <in order> | <range>
  <in order> ::= (<ch>)+
  <range> ::= <ch> '-' <ch>
*/
struct tr_pattern {
  uint8_t type;		// 1:in-order, 2:range
  uint8_t flag_reverse;
  int16_t n;
  struct tr_pattern *next;
  char ch[];
};

static void tr_free_pattern( struct tr_pattern *pat )
{
  while( pat ) {
    struct tr_pattern *p = pat->next;
    mrbc_raw_free( pat );
    pat = p;
  }
}

static struct tr_pattern * tr_parse_pattern( struct VM *vm, const mrb_value *v_pattern, int flag_reverse_enable )
{
  const char *pattern = mrbc_string_cstr( v_pattern );
  int pattern_length = mrbc_string_size( v_pattern );
  int flag_reverse = 0;
  struct tr_pattern *ret = NULL;

  int i = 0;
  if( flag_reverse_enable && pattern_length >= 2 && pattern[i] == '^' ) {
    flag_reverse = 1;
    i++;
  }

  struct tr_pattern *pat1;
  while( i < pattern_length ) {
    // is range pattern ?
    if( (i+2) < pattern_length && pattern[i+1] == '-' ) {
      pat1 = mrbc_alloc( vm, sizeof(struct tr_pattern) + 2 );
      if( pat1 != NULL ) {
	pat1->type = 2;
	pat1->flag_reverse = flag_reverse;
	pat1->n = pattern[i+2] - pattern[i] + 1;
	pat1->next = NULL;
	pat1->ch[0] = pattern[i];
	pat1->ch[1] = pattern[i+2];
      }
      i += 3;

    } else {
      // in order pattern.
      int start_pos = i++;
      while( i < pattern_length ) {
	if( (i+2) < pattern_length && pattern[i+1] == '-' ) break;
	i++;
      }

      int len = i - start_pos;
      pat1 = mrbc_alloc( vm, sizeof(struct tr_pattern) + len );
      if( pat1 != NULL ) {
	pat1->type = 1;
	pat1->flag_reverse = flag_reverse;
	pat1->n = len;
	pat1->next = NULL;
	memcpy( pat1->ch, &pattern[start_pos], len );
      }
    }

    // connect linked list.
    if( ret == NULL ) {
      ret = pat1;
    } else {
      struct tr_pattern *p = ret;
      while( p->next != NULL ) { p = p->next; }
      p->next = pat1;
    }
  }

  return ret;
}

static int tr_find_character( const struct tr_pattern *pat, int ch )
{
  int ret = -1;
  int n_sum = 0;
  int flag_reverse = pat ? pat->flag_reverse : 0;

  while( pat != NULL ) {
    if( pat->type == 1 ) {	// in-order
      int i;
      for( i = 0; i < pat->n; i++ ) {
	if( pat->ch[i] == ch ) ret = n_sum + i;
      }
    } else {	// pat->type == 2  range
      if( pat->ch[0] <= ch && ch <= pat->ch[1] ) ret = n_sum + ch - pat->ch[0];
    }
    n_sum += pat->n;
    pat = pat->next;
  }

  if( flag_reverse ) {
    return (ret < 0) ? INT_MAX : -1;
  }
  return ret;
}

static int tr_get_character( const struct tr_pattern *pat, int n_th )
{
  int n_sum = 0;
  while( pat != NULL ) {
    if( n_th < (n_sum + pat->n) ) {
      int i = (n_th - n_sum);
      return (pat->type == 1) ? pat->ch[i] :pat->ch[0] + i;
    }
    if( pat->next == NULL ) {
      return (pat->type == 1) ? pat->ch[pat->n - 1] : pat->ch[1];
    }
    n_sum += pat->n;
    pat = pat->next;
  }

  return -1;
}

static int tr_main( struct VM *vm, mrbc_value v[], int argc )
{
  if( !(argc == 2 && v[1].tt == MRBC_TT_STRING && v[2].tt == MRBC_TT_STRING)) {
    console_print("ArgumentError\n");	// raise?
    return -1;
  }

  struct tr_pattern *pat = tr_parse_pattern( vm, &v[1], 1 );
  if( pat == NULL ) return 0;

  struct tr_pattern *rep = tr_parse_pattern( vm, &v[2], 0 );

  int flag_changed = 0;
  char *s = mrbc_string_cstr( &v[0] );
  int len = mrbc_string_size( &v[0] );
  int i;
  for( i = 0; i < len; i++ ) {
    int n = tr_find_character( pat, s[i] );
    if( n < 0 ) continue;

    flag_changed = 1;
    if( rep == NULL ) {
      memmove( s + i, s + i + 1, len - i );
      len--;
      i--;
    } else {
      s[i] = tr_get_character( rep, n );
    }
  }

  tr_free_pattern( pat );
  tr_free_pattern( rep );

  v[0].string->size = len;
  v[0].string->data[len] = 0;

  return flag_changed;
}

static void c_string_tr(struct VM *vm, mrbc_value v[], int argc)
{
  mrb_value ret = mrbc_string_dup( vm, &v[0] );
  SET_RETURN( ret );
  tr_main(vm, v, argc);
}


//================================================================
/*! (method) tr!
*/
static void c_string_tr_self(struct VM *vm, mrbc_value v[], int argc)
{
  int flag_changed = tr_main(vm, v, argc);

  if( !flag_changed ) {
    SET_NIL_RETURN();
  }
}


//================================================================
/*! (method) start_with?
*/
static void c_string_start_with(struct VM *vm, mrbc_value v[], int argc)
{
  if( !(argc == 1 && v[1].tt == MRBC_TT_STRING)) {
    console_print("ArgumentError\n");	// raise?
    return;
  }

  int ret;
  if( mrbc_string_size(&v[0]) < mrbc_string_size(&v[1]) ) {
    ret = 0;
  } else {
    ret = (memcmp( mrbc_string_cstr(&v[0]), mrbc_string_cstr(&v[1]),
		   mrbc_string_size(&v[1]) ) == 0);
  }

  SET_BOOL_RETURN(ret);
}


//================================================================
/*! (method) end_with?
*/
static void c_string_end_with(struct VM *vm, mrbc_value v[], int argc)
{
  if( !(argc == 1 && v[1].tt == MRBC_TT_STRING)) {
    console_print("ArgumentError\n");	// raise?
    return;
  }

  int ret;
  int offset = mrbc_string_size(&v[0]) - mrbc_string_size(&v[1]);
  if( offset < 0 ) {
    ret = 0;
  } else {
    ret = (memcmp( mrbc_string_cstr(&v[0]) + offset, mrbc_string_cstr(&v[1]),
		   mrbc_string_size(&v[1]) ) == 0);
  }

  SET_BOOL_RETURN(ret);
}


//================================================================
/*! (method) include?
*/
static void c_string_include(struct VM *vm, mrbc_value v[], int argc)
{
  if( !(argc == 1 && v[1].tt == MRBC_TT_STRING)) {
    console_print("ArgumentError\n");	// raise?
    return;
  }

  int ret = mrbc_string_index( &v[0], &v[1], 0 );
  SET_BOOL_RETURN(ret >= 0);
}


//================================================================
/*! initialize
*/
void mrbc_init_class_string(struct VM *vm)
{
  mrbc_class_string = mrbc_define_class(vm, "String", mrbc_class_object);

  mrbc_define_method(vm, mrbc_class_string, "+",	c_string_add);
  mrbc_define_method(vm, mrbc_class_string, "*",	c_string_mul);
  mrbc_define_method(vm, mrbc_class_string, "size",	c_string_size);
  mrbc_define_method(vm, mrbc_class_string, "length",	c_string_size);
  mrbc_define_method(vm, mrbc_class_string, "to_i",	c_string_to_i);
  mrbc_define_method(vm, mrbc_class_string, "to_s",	c_ineffect);
  mrbc_define_method(vm, mrbc_class_string, "<<",	c_string_append);
  mrbc_define_method(vm, mrbc_class_string, "[]",	c_string_slice);
  mrbc_define_method(vm, mrbc_class_string, "[]=",	c_string_insert);
  mrbc_define_method(vm, mrbc_class_string, "chomp",	c_string_chomp);
  mrbc_define_method(vm, mrbc_class_string, "chomp!",	c_string_chomp_self);
  mrbc_define_method(vm, mrbc_class_string, "dup",	c_string_dup);
  mrbc_define_method(vm, mrbc_class_string, "index",	c_string_index);
  mrbc_define_method(vm, mrbc_class_string, "inspect",	c_string_inspect);
  mrbc_define_method(vm, mrbc_class_string, "ord",	c_string_ord);
  mrbc_define_method(vm, mrbc_class_string, "split",	c_string_split);
  mrbc_define_method(vm, mrbc_class_string, "lstrip",	c_string_lstrip);
  mrbc_define_method(vm, mrbc_class_string, "lstrip!",	c_string_lstrip_self);
  mrbc_define_method(vm, mrbc_class_string, "rstrip",	c_string_rstrip);
  mrbc_define_method(vm, mrbc_class_string, "rstrip!",	c_string_rstrip_self);
  mrbc_define_method(vm, mrbc_class_string, "strip",	c_string_strip);
  mrbc_define_method(vm, mrbc_class_string, "strip!",	c_string_strip_self);
  mrbc_define_method(vm, mrbc_class_string, "to_sym",	c_string_to_sym);
  mrbc_define_method(vm, mrbc_class_string, "intern",	c_string_to_sym);
  mrbc_define_method(vm, mrbc_class_string, "tr",	c_string_tr);
  mrbc_define_method(vm, mrbc_class_string, "tr!",	c_string_tr_self);
  mrbc_define_method(vm, mrbc_class_string, "start_with?", c_string_start_with);
  mrbc_define_method(vm, mrbc_class_string, "end_with?", c_string_end_with);
  mrbc_define_method(vm, mrbc_class_string, "include?",	c_string_include);

#if MRBC_USE_FLOAT
  mrbc_define_method(vm, mrbc_class_string, "to_f",	c_string_to_f);
#endif

  mrbc_define_method(vm, mrbc_class_object, "sprintf",	c_object_sprintf);
  mrbc_define_method(vm, mrbc_class_object, "printf",	c_object_printf);
}


#endif // MRBC_USE_STRING
