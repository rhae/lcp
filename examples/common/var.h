
#pragma once

#include <lcp/lcp_types.h>

#include <stdbool.h>

#ifndef STRBUF_LEN
# define STRBUF_LEN 128
#endif

#define TYPE_INT 1
#define TYPE_DOUBLE 2
#define TYPE_ENUM 3    // TODO
#define TYPE_STRING 4
#define TYPE_BINARY 5  // TODO
#define TYPE_ALIAS 6  // TODO

typedef struct _var
{
  S8 SCPI[16];
  S32 type;
  U32 flags;
  union {
    struct {
      S32 min;
      S32 max;
      S32 value;
    } s32;
    struct {
      F64 min;
      F64 max;
      U32 prec;
      F64 value;
    } f64;
    struct {
      S8 value[STRBUF_LEN];
    } str;
    struct {
      F64 min;
      F64 max;
      F64 prec;
      void *src;
    } alias;
  } data;
} var_t;

var_t* var_create(S8 const *, S32, U32 );
var_t* var_create_f64(S8 const *, U32, F64, F64, F64, S32 );
var_t* var_create_s32(S8 const*, U32, S32, S32, S32);
var_t* var_create_str(S8 const*, U32, S8 const*);


S32 var_serialize(U8*, S32, var_t const*, bool);
S32 var_deserialize(var_t *, U8 const*, S32);
