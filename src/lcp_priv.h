

#pragma once

#ifndef __TYPEDEFS_H__
#include "lcp_types.h"
#endif

#include "queue.h"
#include <stdarg.h>

#define LOGLEVEL_ERROR 70
#define LOGLEVEL_WARN 60
#define LOGLEVEL_INFO 50
#define LOGLEVEL_TEST 40
#define LOGLEVEL_VERBOSE 30
#define LOGLEVEL_DEBUG 20
#define LOGLEVEL_TRACE 10


#define LCP_MTU 256

#ifndef countof
# define countof(x) (sizeof(x)/sizeof(x[0]))
#endif

enum
{
  LCP_NOLINK = 1,
  LCP_PROBING_1,
  LCP_PROBING_2,
  LCP_LINK,

  LCP_ERROR,
  LCP_TIMEOUT,
};

typedef struct lcp_config
{
  void* priv;
  U16 (*send)(U8, void*);
  U16 (*recv)(U8 *, void*);
  U32 (*millis)(void);

  void (*log)(void *priv, int, S8 const*, ...);
} lcp_config_t;

typedef struct lcp_state
{
  U8  state;
  U8  link_state;
  U32 last_recv;
  U32 last_probe;
  U32 probe_cnt;

  U8       tx_state;
  queue_t* qsend;
  queue_t* qrecv;
} lcp_state_t;

typedef struct lcp_ctx
{
  lcp_config_t const* cfg;
  lcp_state_t   state;
  U8*           buf;
} lcp_ctx_t;

void lcp_init( lcp_ctx_t *me, lcp_config_t const * );
void lcp_update(lcp_ctx_t *me);

int lcp_write(lcp_ctx_t *me, U8 const*, U16);
int lcp_read(lcp_ctx_t *me, U8*, U16);

