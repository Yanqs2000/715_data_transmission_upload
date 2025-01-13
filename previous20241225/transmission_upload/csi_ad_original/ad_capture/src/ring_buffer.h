#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

struct RingBuffer {
    uint32_t    head;
    uint32_t    tail;
    uint32_t    num;
    uint32_t    size;
    void        *buf;
};

typedef struct RingBuffer RingBuffer;

RingBuffer *ring_buffer_init(uint32_t buf_size, uint32_t buf_num);
void ring_buffer_destory(RingBuffer *ring_buf);
bool ring_buffer_empty(RingBuffer *ring_buf);
bool ring_buffer_full(RingBuffer *ring_buf);
bool ring_buffer_put(RingBuffer *ring_buf, void *data);
bool ring_buffer_get(RingBuffer *ring_buf, void *data);

#endif /* RING_BUFFER_H */