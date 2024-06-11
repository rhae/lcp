// ideas from https://github.com/tobithiel/Queue
#pragma once

#include "lcp_types.h"

#include <stdbool.h>

typedef struct queue
{
  U8*          mem;
  int head;
  int tail;
  U16          capacity;
  U16          elem_size;
  U16          count;
} queue_t;

void queue_init(queue_t*, U8*, U16);
void queue_flush(queue_t*);

int queue_push( queue_t*, void*);
int queue_pop( queue_t*, void **);
U16 queue_count( queue_t const*);
bool queue_empty(queue_t const*);

