

#include "chan.h"
#include "chan_mem.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

chan_t* chan_create(S8 const* chan_uri)
{

  chan_t* chan = NULL;
  int ret = strncmp("mem:", chan_uri, 4);

  if (ret == 0)
  {
    chan = (chan_t*)calloc(1, sizeof(chan_t));
    chan->init = chan_mem_init;
    chan->send = chan_mem_send;
    chan->recv = chan_mem_recv;
  }

  return chan;
}
