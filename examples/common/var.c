

#include "var.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>


var_t* var_create(S8 const* SCPI, S32 type, U32 flags)
{
  var_t* var = (var_t*)calloc(sizeof(var_t), 1);

  if (var)
  {
    strncpy(var->SCPI, SCPI, 16);
    var->type = type;
    var->flags = flags;
    memset(&var->data, 0, sizeof(var->data));
  }
  return var;
}

var_t* var_create_f64(S8 const *SCPI, U32 flags, F64 defaultValue, F64 minValue, F64 maxValue, S32 prec)
{
  var_t* var = var_create(SCPI, TYPE_DOUBLE, flags );

  if (var)
  {
    var->data.f64.value = defaultValue;
    var->data.f64.min = minValue;
    var->data.f64.max = maxValue;
    var->data.f64.prec = prec;
  }
  return var;
}

var_t* var_create_s32(S8 const *SCPI, U32 flags, S32 defaultValue, S32 minValue, S32 maxValue)
{
  var_t* var = var_create(SCPI, TYPE_INT, flags);

  if (var)
  {
    var->data.s32.value = defaultValue;
    var->data.s32.min = minValue;
    var->data.s32.max = maxValue;
  }
  return var;
}

var_t* var_create_str(S8 const *SCPI, U32 flags, S8 const* value)
{
  var_t* var = var_create(SCPI, TYPE_STRING, flags);

  if (var)
  {
    memset(var->data.str.value, 0, STRBUF_LEN);
    if (value)
    {
      memcpy(var->data.str.value, value, strlen(value));
    }
  }

  return var;
}

typedef struct {
  U8* ptr;
} IOBuf;

static U8 *IOBuf_init(IOBuf *B, U8* p)
{
  B->ptr = p;
  return p;
}

static void IOBuf_put8(IOBuf* B, U8 src)
{
  *B->ptr = src;
  B->ptr++;
}

static void IOBuf_putn(IOBuf *B, void const* src, S32 len)
{
  memcpy(B->ptr, src, len);
  B->ptr += len;
}

static void IOBuf_setn(IOBuf *B, U8 val, S32 len)
{
  memset(B->ptr, val, len);
  B->ptr += len;
}

static void IOBuf_get8(IOBuf *B, U8 *dst)
{
  *dst = *B->ptr;
  B->ptr++;
}

static void IOBuf_getn(IOBuf *B, U8* dst, S32 len)
{
  memcpy(dst, B->ptr, len);
  B->ptr += len;
}

union _conv_ {
  S32 s32;
  F64 f64;
  U8  bytes[8];
};

S32 var_serialize(U8* Buf, S32 BufSize, var_t const *var, bool with_descr)
{
  assert(Buf);
  assert(BufSize > 0);
  assert(var);

  IOBuf __B, *B;
  U8 type = var->type;
  S32 len = strlen(var->SCPI);

  if (with_descr) {
    type += 0x80U;
  }

  B = &__B;
  IOBuf_init(B, Buf);
  IOBuf_put8(B, 16);
  if (len < 16)
  {
    IOBuf_putn(B, var->SCPI, len );
    IOBuf_setn(B, 0, 16 - len);
  }
  else
  {
    IOBuf_putn(B, var->SCPI, 15 );
    IOBuf_setn(B, 0, 1 );
  }

  IOBuf_put8(B, type);
  IOBuf_putn(B, &var->flags, 4);
  switch (var->type)
  {
    case TYPE_INT:
      IOBuf_putn(B, &var->data.s32.value, 4);

      if (with_descr)
      {
        IOBuf_putn(B, &var->data.s32.min, 4);
        IOBuf_putn(B, &var->data.s32.max, 4);
      }
      break;

    case TYPE_DOUBLE:
      IOBuf_putn(B, &var->data.f64.value, 8);
      if (with_descr)
      {
        IOBuf_putn(B, &var->data.f64.min, 8);
        IOBuf_putn(B, &var->data.f64.max, 8);
        IOBuf_putn(B, &var->data.f64.prec, 4);
      }
      break;

    case TYPE_STRING:
      IOBuf_putn(B, &var->data.str.value, STRBUF_LEN);
      break;
  }

  return B->ptr - Buf;
}

S32 var_deserialize( var_t *var, U8 const* Buf, S32 BufSize )
{
  assert(Buf);
  assert(BufSize > 0);
  assert(var);

  IOBuf __B, *B;
  U8 byte;
  S32 len = 0;
  bool with_descr = false;
  
  B = &__B;
  IOBuf_init(B, (U8*) Buf);

  IOBuf_get8(B, &byte);
  len += byte;
  IOBuf_getn(B, (U8*)var->SCPI, len);

  IOBuf_get8(B, &byte );
  IOBuf_getn(B, (U8*)&var->flags, 4);
  var->type = byte & 0x7f;
  with_descr = byte & 0x80;
  switch (var->type)
  {
    case TYPE_INT:
      IOBuf_getn(B, (U8*)&var->data.s32.value, 4);


      if (with_descr)
      {
        IOBuf_getn(B, (U8*)&var->data.s32.min, 4);
        IOBuf_getn(B, (U8*)&var->data.s32.max, 4);
      }
      break;

    case TYPE_DOUBLE:
      IOBuf_getn(B, (U8*)&var->data.f64.value, 8);
      if (with_descr)
      {
        IOBuf_getn(B, (U8*)&var->data.f64.min, 8);
        IOBuf_getn(B, (U8*)&var->data.f64.max, 8);
        IOBuf_getn(B, (U8*)&var->data.f64.prec, 4);
      }
      break;

    case TYPE_STRING:
      IOBuf_getn(B, (U8*)&var->data.str.value[0], STRBUF_LEN);
      break;
  }

  return B->ptr - Buf;
}
