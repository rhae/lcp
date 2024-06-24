

#include <common/chan.h>

#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>

#include <stdio.h>


lcp_ctx_t rcvr;
lcp_ctx_t sndr;
lcp_config_t sndr_cfg;
lcp_config_t rcvr_cfg;

typedef struct _var
{
  S8 const* SCPI;
  S32 type;
  union {
    struct {
      F64 value;
      F64 min;
      F64 max;
      U32 prec;
      U32 flags;
    } f64;
    struct {
      S32 value;
      S32 min;
      S32 max;
      U32 flags;
    } s32;
  } data;
} var_t;

static U32 millis(void);
static void log1(void*, int, S8 const*, ...);
static void log2(void*, int, S8 const*, ...);

static var_t* create_var(S8 const *, S32 );
static var_t* create_var_f64(S8 const *, S32, F64, F64, F64, S32, U32 );
static var_t* create_var_s32(S8 const*, S32, S32, S32, S32, U32);

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

static var_t* create_var(S8 const* SCPI, S32 type)
{
  var_t* var = (var_t*)calloc(sizeof(var_t), 1);

  if (var)
  {
    var->SCPI = SCPI;
    var->type = type;
    memset(&var->data, 0, sizeof(var->data));
  }
  return var;
}

static var_t* create_var_f64(S8 const *SCPI, S32 type, F64 defaultValue, F64 minValue, F64 maxValue, S32 prec, U32 flags)
{
  var_t* var = create_var(SCPI, type);

  if (var)
  {
    var->data.f64.value = defaultValue;
    var->data.f64.min = minValue;
    var->data.f64.max = maxValue;
    var->data.f64.prec = prec;
    var->data.f64.flags = flags;
  }
  return var;
}

static var_t* create_var_s32(S8 const *SCPI, S32 type, S32 defaultValue, S32 minValue, S32 maxValue, U32 flags)
{
  var_t* var = create_var(SCPI, type);

  if (var)
  {
    var->data.s32.value = defaultValue;
    var->data.s32.min = minValue;
    var->data.s32.max = maxValue;
    var->data.s32.flags = flags;
  }
  return var;

}