
#include "lcp_priv.h"

#include "crc.h"
#include <lcp/queue.h>

#include <string.h>   /* memcp */
#include <stdlib.h>   /* calloc */

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

static int send_probe(lcp_ctx_t*, int mirror );
static int recv_probe(lcp_ctx_t*);
static int handle_rxtx(lcp_ctx_t*);
static int send(lcp_ctx_t*, U8 const* pkt, U16 size);
static int recv(lcp_ctx_t*, U8* data, U16 size);

#define LOG_ERROR( ...)  do { if( me->cfg->log) me->cfg->log( (void*)me->cfg, LOGLEVEL_ERROR, __VA_ARGS__ );} while(0)
#define LOG_WARN( ...)  do { if( me->cfg->log) me->cfg->log( (void*)me->cfg, LOGLEVEL_WARN, __VA_ARGS__ );} while(0)
#define LOG_INFO( ...)  do { if( me->cfg->log) me->cfg->log( (void*)me->cfg, LOGLEVEL_INFO, __VA_ARGS__ );} while(0)
#define LOG_TEST( ...)  do { if( me->cfg->log) me->cfg->log( (void*)me->cfg, LOGLEVEL_TEST, __VA_ARGS__ );} while(0)
#define LOG_VERBOSE( ...)  do { if( me->cfg->log) me->cfg->log( (void*)me->cfg, LOGLEVEL_VERBOSE, __VA_ARGS__ );} while(0)
#define LOG_DEBUG( ...)  do { if( me->cfg->log) me->cfg->log( (void*)me->cfg, LOGLEVEL_DEBUG, __VA_ARGS__ );} while(0)
#define LOG_TRACE( ...)  do { if( me->cfg->log) me->cfg->log( (void*) me->cfg, LOGLEVEL_TRACE, __VA_ARGS__ );} while(0)

void lcp_init( lcp_ctx_t *me, lcp_config_t const *cfg )
{
  me->cfg = cfg;
  memset(&me->state, 0, sizeof(lcp_state_t));
  me->state.state = LCP_NOLINK;
  me->buf = calloc(_MTU * 2, 1);

  LOG_TRACE("init done. ");
}

void lcp_update( lcp_ctx_t *me )
{
  int ret;
  U32 now = me->cfg->millis();
  S32 delta;
  S32 state = me->state.state;

  LOG_TRACE("> lcp_update");

  switch (me->state.state)
  {
    case LCP_NOLINK:
    case LCP_ERROR:
    case LCP_TIMEOUT:
      ret = send_probe(me, 0 );
      if (ret == 0)
      {
        me->state.last_probe = now;
        me->state.state = LCP_PROBING_1;
      }
      break;

    case LCP_PROBING_1:
    case LCP_PROBING_2:
      ret = recv_probe(me);
      if (ret == 0)
      {
        if (me->state.state == LCP_PROBING_1)
        {
          send_probe(me, 1 );
          me->state.state = LCP_PROBING_2;
        }
        else
        {
          if (!me->state.link_state)
          {
            LOG_INFO("  Link ESTABLISHED");
          }
          else
          {
            LOG_INFO("  Link active");
          }
          me->state.state = LCP_LINK;
          me->state.link_state = 1;
        }
      }
      else
      {
        delta = now - me->state.last_probe;
        if (delta > PROBE_TIMEOUT)
        {
          LOG_WARN("  probe timeout.");
          me->state.probe_cnt++;
        }
        if (me->state.probe_cnt > MAX_PROBE_FAILS)
        {
          LOG_ERROR("  Link establish failed. checked %d times.", me->state.probe_cnt);
          me->state.state = LCP_ERROR;
        }
      }
      break;

    case LCP_LINK:

      ret = handle_rxtx(me);
      if (!ret)
      {
        delta = now - me->state.last_probe;
        if (delta > PROBE_INTERVAL)
        {
          LOG_VERBOSE("  No traffic. Check link.");
          ret = send_probe(me,0);
          if (ret == 0)
          {
            me->state.last_probe = now;
            me->state.state = LCP_PROBING_1;
          }
        }
      }
      break;
  }
  if (state != me->state.state)
  {
    LOG_DEBUG("  state change: %d -> %d", state, me->state.state);
  }
  LOG_TRACE("< lcp_update");
}

int lcp_write(lcp_ctx_t *me, U8 const* buf, U16 size)
{
  return 0;
}

int lcp_read(lcp_ctx_t *me, U8 * buf, U16 size)
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

  crc = crc_calc(pkt, (U16)(p - pkt));

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

static int send_probe(lcp_ctx_t *me, int mirror)
{
  LOG_TRACE("> send_probe");
  int len = fill_probe(me->buf, mirror );
  int n = send(me, me->buf, (U16)len);
  LOG_TRACE("< send_probe");
  return n > 0 ? 0 : n;
}

static int recv_probe(lcp_ctx_t *me)
{
  U8 *p = me->buf + _MTU;
  LOG_TRACE("> recv_probe");
  int len = recv(me, p, _MTU);
  int chk = 0;
  if (len > 0)
  {
    chk = chk_crc(p, (U16)len);
  }
  LOG_DEBUG("  CRC %s", chk == 0 ? "OK" : "WRONG" );
  LOG_TRACE("< recv_probe");
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

static int send(lcp_ctx_t *me, U8 const* pkt, U16 size)
{
  LOG_TRACE("> send", size);
  LOG_DEBUG("  send %d bytes: %x %x %x", size, pkt[0], pkt[1], pkt[2]);

  me->cfg->send(FRAME_FLAG, me->cfg->priv );
  for (U16 i = 0; i < size; i++)
  {
    U8 b = pkt[i];

    if (b == FRAME_FLAG || b == FRAME_ESC )
    {
      me->cfg->send(FRAME_ESC, me->cfg->priv);
      b ^= 0x20;
    }
    me->cfg->send(b, me->cfg->priv);

  }
  me->cfg->send(FRAME_FLAG, me->cfg->priv );

  LOG_TRACE("< send", size);
  return size + 2;
}

static int recv(lcp_ctx_t *me, U8 *data, U16 size)
{
  enum { START, ESC, DATA, END };
  U8 state = START;
  U8* buf = data;
  U8 b;
  int i;
  int esc_cnt = 2;  // Number of escape bytes + 2 Frame delimiter
  LOG_TRACE("> recv");

  for (i = 0; i < (int)size && state != END; i++)
  {
    U16 n = me->cfg->recv(&b, me->cfg->priv);
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

  int ret = i > 0 ? i - esc_cnt : i;
  LOG_DEBUG("  state = %d, ret = %d", state, ret);
  if (ret > 0)
  {
    LOG_DEBUG("  recv bytes %x %x %x", data[0], data[1], data[2]);
  }
  LOG_TRACE("< recv");
  return ret;
}

static int handle_rxtx(lcp_ctx_t *me)
{
  U16 cnt;
  int state = me->state.tx_state;

  LOG_TRACE("> handle_rxtx");
  switch (me->state.tx_state)
  {
    case SEND:
      cnt = queue_count(me->state.qsend);
      if ( cnt > 0 )
      {
        U8* data;
        queue_pop(me->state.qsend, &data);
        fill_data(me->buf, data, LCP_MTU);
      }
      break;

    case WAIT_ACK:
      break;

    case RECV:
      break;
  }
  if (state != me->state.tx_state)
  {
    LOG_DEBUG("tx_state: %d -> %d", state, me->state.tx_state );
  }

  LOG_TRACE("< handle_rxtx");
  return 0;
}

