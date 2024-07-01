

#include <common/chan.h>
#include <common/var.h>

#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>

#include <stdio.h>


lcp_ctx_t rcvr;
lcp_ctx_t sndr;
lcp_config_t sndr_cfg;
lcp_config_t rcvr_cfg;

var_t *rcvr_vars[5];
var_t *sndr_vars[5];


static U32 millis(void);
static void log1(void*, int, S8 const*, ...);
static void log2(void*, int, S8 const*, ...);

static void init_vars(var_t **, S32, S32);


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

  init_vars(rcvr_vars, countof(rcvr_vars), 1);
  init_vars(sndr_vars, countof(sndr_vars), 0);

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
  static char const s_levels[10] = { ' ', 't', 'D', 'V', 'T', 'I', 'W', 'E' };
  int n = level / 10;
  return s_levels[n];
}

static void log1(void* priv, int level, S8 const* fmt, ...)
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
  vsnprintf(&Buf[n], sizeof(Buf) - n, fmt, args);
  va_end(args);

  log(Buf);
}

static void log2(void* priv, int level, S8 const* fmt, ...)
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
  vsnprintf(&Buf[n], sizeof(Buf) - n, fmt, args);
  va_end(args);

  log(Buf);
}

static void init_vars(var_t **vars, S32 var_cnt, S32 rcvr)
{
  U8 buf[512];

  var_t* xvar = var_create_str("", 0, "");

  if (rcvr)
  {
    vars[0] = var_create_str("IDN", 0, "Receiver V1.0" );
    vars[1] = var_create_str("SER", 0, "A4345g" );

  }
  else
  {
    vars[0] = var_create_str("IDN", 0, "Sender V1.0" );
    vars[1] = var_create_str("SER", 0, "GEZHUD4" );
  }

  vars[2] = var_create_str("VER", 0, "1.0" );
  vars[3] = var_create_f64("TMP", 0, 20.1, 0, 0, 2 );
  vars[4] = var_create_f64("VEL", 0, 1623.3, 0, 0, 2 );

#ifdef TEST_SERIALIZE
  S32 len;
  len = var_serialize(buf, 512, vars[2], false);
  var_deserialize(xvar, buf, len);

  memcmp(&vars[2], xvar, 32 );
#endif

}