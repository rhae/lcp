

#include <common/chan.h>

#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>

#include <stdio.h>
#include <conio.h>

#define NUM_THREADS 2

typedef struct _th_param
{
  S8 const* name;
  chan_t* chan;
  lcp_ctx_t *ctx;
  lcp_config_t *cfg;

  HANDLE event;
  S32 keyCode;
} th_param_t;

static U32 millis( void );

lcp_ctx_t rcvr;
lcp_ctx_t sndr;
lcp_config_t sndr_cfg;
lcp_config_t rcvr_cfg;
th_param_t th_params[NUM_THREADS];

static void log1(void*, int, S8 const*, ...);
static void log2(void*, int, S8 const*, ...);

static DWORD WINAPI th_lcp(LPVOID lpParam); 

int main(int argc, char** argv)
{
  HANDLE hThreads[NUM_THREADS];
  DWORD dwThreadId[NUM_THREADS];

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

  th_params[0].name= "Snd-Thread";
  th_params[0].ctx = &sndr;
  th_params[0].cfg = &sndr_cfg;
  th_params[1].keyCode = 0;

  th_params[1].name= "Rcv-Thread";
  th_params[1].ctx = &rcvr;
  th_params[1].cfg = &rcvr_cfg;
  th_params[1].keyCode = 0;

  for (int i = 0; i < NUM_THREADS; i++)
  {
    th_params[i].event = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (th_params[i].event == NULL )
    {
      printf("CreateEvent failed, error: %lu\n", GetLastError());
      return 1;
    }
  }

  for (int i = 0; i < NUM_THREADS; i++)
  {
    hThreads[i] = CreateThread(
      NULL,              // default security attributes
      0,                 // use default stack size  
      th_lcp,  // thread function name
      &th_params[i],              // argument to thread function 
      0,                 // use default creation flags 
      &dwThreadId[i]);      // returns the thread identifier 
  }

  printf("Press key 'x' to stop threads...\n");
  while (1) {
    int ch = _getch(); // Read the key
    switch (ch)
    {
      case 'A':
      case 'B':
        break;

      default:
        for (int i = 0; i < NUM_THREADS; i++)
        {
          th_params[i].keyCode = ch;
          SetEvent(th_params[i].event);
        }
    }

    Sleep(100); // Avoid busy waiting
  }

  // Wait for all threads to finish
  WaitForMultipleObjects(NUM_THREADS, hThreads, TRUE, INFINITE);

  // Close all thread handles
  for (int i = 0; i < NUM_THREADS; ++i)
  {
    CloseHandle(hThreads[i]);
    CloseHandle(th_params[i].event);
  }

  printf("All threads have exited.\n");


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

static int s_LogLevel = LOGLEVEL_VERBOSE;

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
  n = sprintf(Buf, "[ % 6d %s:%c ]", millis(), "CHAN1", l);

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
  n = sprintf(Buf, "[ % 6d %s:%c ]", millis(), "CHAN2", l);

  va_start(args, fmt);
  vsnprintf(&Buf[n], sizeof(Buf)-n, fmt, args);
  va_end(args);

  log(Buf);
}

DWORD WINAPI th_lcp(LPVOID lpParam)
{
  th_param_t* param = (th_param_t*)lpParam;
  BOOL Done = FALSE;
  printf("Thread is %s running...\n", param->name );
  // Simulate some work

  lcp_init(param->ctx, param->cfg);
  
  for(;!Done;)
  {
    DWORD Ret = WaitForSingleObject(param->event, 10);
    switch (Ret)
    {
      case WAIT_OBJECT_0:
        switch (param->keyCode)
        {
          case 'x':
            Done = TRUE;
            break;
        }
        break;

      default:
        lcp_update(param->ctx);
        break;
    }
  }

  printf("Thread %s has finished work.\n", param->name);
  return 0;
}