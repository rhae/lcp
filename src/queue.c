
#include "queue.h"

#include <string.h>   // memcpy

static inline int incr(int value, int ofs, int size)
{
  value += ofs;
  if (value == size)
  {
    value = 0;
  }
  return value;
}

void queue_init(queue_t *q, U8* mem, U16 cap)
{
  static void* A[2];
  q->mem = mem;
  q->head = 0;
  q->tail = 0;
  q->capacity = cap;
  q->elem_size = (U16) ((U8*)A[1] - (U8*)A[0]);
  q->count = 0;
}

void queue_flush(queue_t *q)
{
  q->head = 0;
  q->tail = 0;
}


int queue_push(queue_t* q, void const *e)
{
  if (q->count == q->capacity)
  {
    return -1;
  }

  memcpy(&q->mem[q->head * q->elem_size], e, q->elem_size);
  q->head = incr(q->head, 1, q->capacity);
  q->count++;
  return 0;
}

int queue_pop(queue_t*q, void **e)
{
  if (q->count == 0)
  {
    return -1;
  }

  memcpy( *e, &q->mem[q->tail * q->elem_size], q->elem_size);
  q->tail = incr(q->tail, 1, q->capacity);
  q->count--;
  return 0;
}

U16 queue_count(queue_t const *q)
{
  return q->count;
}

bool queue_empty(queue_t const *q)
{
  return q->count == 0;
}
