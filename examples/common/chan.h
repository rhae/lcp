
#pragma once

#include <lcp/lcp.h>

typedef struct _chan
{
  void (*init)(void*);
  U16 (*send)(U8, void*);
  U16 (*recv)(U8*, void*);

  void* priv;
} chan_t;

chan_t* chan_create(S8 const* chan_uri);

