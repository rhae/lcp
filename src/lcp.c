
#include "lcp_priv.h"

#include "crc.h"
#include "queue.h"

#include <string.h>   /* memcp */

#define PROBE_TIMEOUT 1000
#define PROBE_INTERVAL 2000
#define COMM_TIMEOUT 1000
#define MAX_PROBE_FAILS 5

#define _MTU ((LCP_MTU) + 8)

#ifndef LO_BYTE
# define LO_BYTE( x ) ((x) & 0xFF)
#endif

#ifndef HI_BYTE
# define HI_BYTE( x ) (((x) >> 8) & 0xFF)
#endif

enum {SEND = 1, WAIT_ACK, RECV };

static int send_probe( int mirror );
static int recv_probe();
static int handle_rxtx();
static int send(U8 const* pkt, U16 size);
static int recv(U8* data, U16 size);

static lcp_config_t s_cfg = { 0 };
static lcp_state_t s_state = { 0 };
static U8 s_buf[2 * _MTU];

void lcp_init( lcp_config_t const *cfg )
{
  memcpy( &s_cfg, cfg, sizeof(lcp_config_t));
  s_state.state = LCP_NOLINK;
  memset(s_buf, 0, sizeof(s_buf));
}

void lcp_update()
{
  int ret;
  U32 now = s_cfg.millis();
  S32 delta;

  switch (s_state.state)
  {
    case LCP_NOLINK:
    case LCP_ERROR:
    case LCP_TIMEOUT:
      ret = send_probe( 0 );
      if (ret == 0)
      {
        s_state.last_probe = now;
        s_state.state = LCP_PROBING;
      }
      break;

    case LCP_PROBING:
      ret = recv_probe();
      if (ret)
      {
        delta = now - s_state.last_probe;
        if (delta > PROBE_TIMEOUT)
        {
          s_state.probe_cnt++;
        }
        if (s_state.probe_cnt > MAX_PROBE_FAILS)
        {
          s_state.state = LCP_ERROR;
        }
      }
      else
      {
        send_probe( 1 );
        s_state.state = LCP_LINK;
      }
      break;

    case LCP_LINK:

      ret = handle_rxtx();
      if (!ret)
      {
        delta = now - s_state.last_probe;
        if (delta > PROBE_INTERVAL)
        {
          ret = send_probe(0);
          if (ret == 0)
          {
            s_state.last_probe = now;
            s_state.state = LCP_PROBING;
          }
        }
      }
      break;

  }
}

int lcp_write(U8 const* buf, U16 size)
{
  return 0;
}

int lcp_read(U8 * buf, U16 size)
{
  return 0;
}


#define FRAME_FLAG 0x7e
#define FRAME_ESC  0x7d


int fill_probe(U8 *pkt, int mirror)
{
#define _FILL( x ) *p = x; p++

  U16 crc;
  U8* p = pkt;
  _FILL( 0xFF );
  _FILL( 0x03 );
  _FILL( LO_BYTE(0xC021));
  _FILL( HI_BYTE(0xC021));
  if (mirror)
  {
    _FILL( 'O' );
    _FILL( 'L' );
    _FILL( 'L' );
    _FILL( 'E' );
    _FILL( 'H' );
  }
  else
  {
    _FILL( 'H' );
    _FILL( 'E' );
    _FILL( 'L' );
    _FILL( 'L' );
    _FILL( 'O' );
  }

  crc = crc_calc(pkt, p - pkt);

  _FILL(LO_BYTE(crc));
  _FILL(HI_BYTE(crc));

#undef _FILL
  return p - pkt;
}

int chk_crc(U8* pkt, U16 size)
{
  U16 crc_recv = (pkt[size - 1] << 8) + (pkt[size - 2]);
  U16 crc = crc_calc(pkt, size - 2);

  if (crc == crc_recv)
  {
    return 0;
  }

  return -1;
}

static int send_probe(int mirror)
{
  int len = fill_probe(s_buf, mirror );
  int n = send(s_buf, (U16)len);
  return n > 0 ? 0 : n;
}

static int recv_probe()
{
  U8 *p = s_buf + _MTU;
  int len = recv(p, _MTU);
  int chk = 0;
  if (len > 0)
  {
    chk = chk_crc(p, (U16)len);
  }
  return chk;
}

static int fill_data(U8* pkt, U8 const* data, U16 size)
{
#define _FILL(x) *p = x; p++;
#define _FILLC(x) *p = x; p++; crc = crc_calc_byte( *p, crc )

  U16 crc = crc_init();

  U8* p = pkt;
  _FILLC( 0xFF );
  _FILLC( 0x03 );
  _FILLC( LO_BYTE(0x7000));
  _FILLC( HI_BYTE(0x7000));
  for (int i = 0; (int)i < size; i++)
  {
    _FILLC(*data);
  }
  crc = crc_calc_byte(0, crc);
  _FILL( LO_BYTE(crc));
  _FILL( HI_BYTE(crc));

  return pkt - p;
}

static int send(U8 const* pkt, U16 size)
{
  s_cfg.send(FRAME_FLAG, s_cfg.priv );
  for (U16 i = 0; i < size; i++)
  {
    U8 b = pkt[i];

    if (b == FRAME_FLAG || b == FRAME_ESC )
    {
      s_cfg.send(FRAME_ESC, s_cfg.priv);
      b ^= 0x20;
    }
    s_cfg.send(b, s_cfg.priv);

  }
  s_cfg.send(FRAME_FLAG, s_cfg.priv );
  return size + 2;
}

static int recv(U8 *data, U16 size)
{
  enum { START, ESC, DATA, END };
  U8 state = START;
  U8* buf = data;
  U8 b;
  int i;
  int esc_cnt = 2;  // Number of escape bytes + 2 Frame delimiter

  for (i = 0; i < (int)size && state != END; i++)
  {
    U16 n = s_cfg.recv(&b, s_cfg.priv);
    if (n == 1)
    {
      switch (state)
      {
        case START:
          if (b == FRAME_FLAG)
          {
            state = DATA;
          }
          else
          {
            state = END;
            i = -1;
          }
          break;

        case ESC:
          state = DATA;
          *buf = b ^ 0x20;
          buf++;
          esc_cnt++;
          break;

        case DATA:
          if (b == FRAME_FLAG)
          {
            state = END;
          }
          else if (b == FRAME_ESC)
          {
            state = ESC;
          }
          else
          {
            *buf = b;
            buf++;
          }
          break;
      }
    }
  }
  return i > 0 ? i - esc_cnt : i;
}

static int handle_rxtx()
{
  U16 cnt;
  switch (s_state.tx_state)
  {
    case SEND:
      cnt = queue_count(s_state.qsend);
      if ( cnt > 0 )
      {
        U8* buf = &s_buf[0];
        U8* data;
        queue_pop(s_state.qsend, &data);
        fill_data(s_buf, data, LCP_MTU);
      }
      break;

    case WAIT_ACK:
      break;

    case RECV:
      break;
  }

  return 0;
}

