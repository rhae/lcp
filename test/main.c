

#include "chan.h"
#include "../include/lcp.h"

#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>

static U32 millis( void );

int main(int argc, char** argv)
{

  if (argc < 1)
  {
    return -1;
  }

  chan_t* chan = chan_create(argv[1]);
  lcp_config_t cfg;
  
  chan->init(0);

  cfg.send = chan->send;
  cfg.recv = chan->recv;
  cfg.millis = millis;
  cfg.priv = NULL;

  lcp_init(&cfg);

  for (int i = 0; i < 10; i++)
  {
    lcp_update();
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