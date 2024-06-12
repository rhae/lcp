

#include "chan_mem.h"
#include "chan.h"

#include "../include/lcp.h"
#include <stdlib.h>  /* calloc */

#define _MTU (LCP_MTU +32)

typedef struct _mem_ctx
{
  U8 mem[2* _MTU];
  int idx;
} mem_ctx_t;


void chan_mem_init(void * me)
{
  chan_t* ch = (chan_t*)me;
  ch->priv = calloc(sizeof(mem_ctx_t), 1);
}

U16 chan_mem_send(U8 b, void *priv)
{
  chan_t *me = (chan_t*)priv;
  mem_ctx_t* ctx = (mem_ctx_t*)me->priv;

  if (ctx->idx == _MTU)
  {
    return 0;
  }

  ctx->mem[ctx->idx] = b;
  ctx->idx++;

  return 1;
}

U16 chan_mem_recv(U8 *b, void *priv)
{
  chan_t *me = (chan_t*)priv;
  mem_ctx_t* ctx = (mem_ctx_t*)me->priv;

  if (ctx->idx == 0)
  {
    return 0;
  }

  ctx->idx--;
  *b = ctx->mem[ctx->idx];


  return 1;
}

