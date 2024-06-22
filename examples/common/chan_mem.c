

#include "chan_mem.h"
#include "chan.h"

#include <lcp/lcp.h>
#include <stdlib.h>  /* calloc */

#define _MTU (LCP_MTU +32)

typedef struct _mem_ctx
{
  U8 mem[2* _MTU];
  int ri;
  int wi;
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

  if (ctx->wi == _MTU)
  {
    ctx->wi = 0;
  }

  ctx->mem[ctx->wi] = b;
  ctx->wi++;

  return 1;
}

static int _is_empty(mem_ctx_t* ctx)
{
  return (ctx->ri == ctx->wi);
}

U16 chan_mem_recv(U8 *b, void *priv)
{
  chan_t *me = (chan_t*)priv;
  mem_ctx_t* ctx = (mem_ctx_t*)me->priv;

  if (_is_empty(ctx))
  {
    return 0;
  }

  if (ctx->ri == _MTU)
  {
    ctx->ri = 0;
  }

  *b = ctx->mem[ctx->ri];
  ctx->ri++;


  return 1;
}

