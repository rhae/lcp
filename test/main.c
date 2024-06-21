

#include "chan.h"
#include "../include/lcp.h"

#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>

#include <stdio.h>

static U32 millis( void );

lcp_ctx_t rcvr;
lcp_ctx_t sndr;
lcp_config_t sndr_cfg;
lcp_config_t rcvr_cfg;

static void log1(void*, int, S8 const*, ...);
static void log2(void*, int, S8 const*, ...);

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
  sndr_cfg.log = log1;

  rcvr_cfg.send = chan1->send;
  rcvr_cfg.recv = chan2->recv;
  rcvr_cfg.millis = millis;
  rcvr_cfg.priv = chan2;
  rcvr_cfg.log = log2;

  lcp_init(&rcvr, &rcvr_cfg);
  lcp_init(&sndr, &sndr_cfg);

  for (int i = 0; i < 100; i++)
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

static int s_LogLevel = LOGLEVEL_DEBUG;

static void log(S8 const* buf)
{
  puts(buf);
}

static char fmt_level(int level)
{
  static const s_levels[10] = { ' ', 't', 'D', 'V', 'T', 'I', 'W', 'E' };
  int n = level / 10;
  return s_levels[n];
}

static void log1(void *priv, int level, S8 const *fmt, ...)
{
  S8 Buf[512] = { 0 };
  int n;
  va_list args;

  if (level < s_LogLevel)
  {
    return;
  }

  char l = fmt_level(level);
  n = sprintf(Buf, "[ %s:%c ]", "CHAN1", l);

  va_start(args, fmt);
  vsnprintf(&Buf[n], sizeof(Buf)-n, fmt, args);
  va_end(args);

  log(Buf);
}

static void log2(void *priv, int level, S8 const *fmt, ...)
{
  S8 Buf[512] = { 0 };
  int n;
  va_list args;

  if (level < s_LogLevel)
  {
    return;
  }

  char l = fmt_level(level);
  n = sprintf(Buf, "[ %s:%c ]", "CHAN2", l);

  va_start(args, fmt);
  vsnprintf(&Buf[n], sizeof(Buf)-n, fmt, args);
  va_end(args);

  log(Buf);
}