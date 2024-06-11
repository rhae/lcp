

#pragma once

#ifndef __TYPEDEFS_H__
#include "lcp_types.h"
#endif

#include "queue.h"

#define LCP_MTU 256

#ifndef countof
# define countof(x) (sizeof(x)/sizeof(x[0]))
#endif

enum
{
  LCP_NOLINK = 1,
  LCP_PROBING,
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
} lcp_config_t;

typedef struct lcp_state
{
  U8  state;
  U32 last_recv;
  U32 last_probe;
  U32 probe_cnt;

  U8  tx_state;
  queue_t* qsend;
  queue_t* qrecv;
} lcp_state_t;

void lcp_init( lcp_config_t const * );
void lcp_update();

int lcp_write(U8 const*, U16);
int lcp_read(U8*, U16);

