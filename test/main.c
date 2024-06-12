

#include "chan.h"
#include "../include/lcp.h"

#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>

static U32 millis( void );

lcp_ctx_t rcvr;
lcp_ctx_t sndr;
lcp_config_t sndr_cfg;
lcp_config_t rcvr_cfg;

int main(int argc, char** argv)
{

  if (argc < 1)
  {
    return -1;
  }

  chan_t* chan1 = chan_create(argv[1]);
  chan_t* chan2 = chan_create(argv[1]);
  
  sndr_cfg.send = chan2->send;
  sndr_cfg.recv = chan1->recv;
  sndr_cfg.millis = millis;
  sndr_cfg.priv = chan1;

  rcvr_cfg.send = chan1->send;
  rcvr_cfg.recv = chan2->recv;
  rcvr_cfg.millis = millis;
  rcvr_cfg.priv = chan2;

  lcp_init(&rcvr, &rcvr_cfg);
  lcp_init(&sndr, &sndr_cfg);

  for (int i = 0; i < 10; i++)
  {
    lcp_update(&sndr);
    lcp_update(&rcvr);
  }

  return 0;
}

static U32 millis()
{
  static LARGE_INTEGER StartingTime;
  static LARGE_INTEGER Frequency;
  LARGE_INTEGER Now, ElapsedMicroseconds;

  static int InitDone = FALSE;
  if (!InitDone)
  {
    InitDone = TRUE;
    QueryPerformanceFrequency(&Frequency);
    QueryPerformanceCounter(&StartingTime);
  }

  QueryPerformanceCounter(&Now);

  ElapsedMicroseconds.QuadPart = Now.QuadPart - StartingTime.QuadPart;
  ElapsedMicroseconds.QuadPart *= 1000; // *1000;
  ElapsedMicroseconds.QuadPart /= Frequency.QuadPart;

  return ElapsedMicroseconds.u.LowPart;
}