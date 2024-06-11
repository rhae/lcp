

#include "chan.h"

#include "../include/lcp.h"
#include <string.h>

#define FRAME_FLAG 0x7d

enum {RX = 0, TX = 1};

U8 s_Mem[2 * LCP_MTU];
int s_Idx[2] = { 0, 0 };

void chan_mem_init(void * priv)
{
  (void)priv;
  memset(s_Mem, 0, sizeof(s_Mem));
  s_Idx[RX] = 0;
  s_Idx[TX] = 0;
}

U16 chan_mem_send(U8 b, void *priv)
{
  (void)priv;

  s_Mem[s_Idx[TX]] = b;
  s_Idx[TX]++;

  return 1;
}

U16 chan_mem_recv(U8 *b, void *priv)
{
  (void)priv;

  *b = s_Mem[s_Idx[RX]];
  if (s_Idx[RX] == s_Idx[TX])
  {
    s_Idx[RX] = 0;
    s_Idx[TX] = 0;
    return 0;
  }
  s_Idx[RX] ++;

  return 1;
}
