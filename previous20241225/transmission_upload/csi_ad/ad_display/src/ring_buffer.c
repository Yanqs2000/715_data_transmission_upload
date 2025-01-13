/**
 * Copyright (C) 2013 Guangzhou Tronlong Electronic Technology Co., Ltd. - www.tronlong.com
 *
 * @file ring_buffer.c
 *
 * @brief ring_buffer module: Draw the AD cache buffer.
 *
 * @author Tronlong <support@tronlong.com>
 *
 * @version V1.0
 *
 * @date 2023-5-10
 **/

#include "ring_buffer.h"

RingBuffer *ring_buffer_init(uint32_t buf_size, uint32_t buf_num)
{
    RingBuffer *ring_buf = NULL;

    ring_buf = (RingBuffer *)malloc(sizeof(RingBuffer));
    if (!ring_buf) {
        perror("malloc");
        return NULL;
    }

    ring_buf->num    = buf_num;
    ring_buf->size   = buf_size;
    ring_buf->head   = 0;
    ring_buf->tail   = 0;

    ring_buf->buf = (void *)malloc(buf_size * buf_num);
    if (!ring_buf->buf) {
        perror("malloc");
        free(ring_buf);
        ring_buf = NULL;
    }

    return ring_buf;
}

void ring_buffer_destory(RingBuffer *ring_buf)
{
    if (!ring_buf) return;

    if (ring_buf->buf) {
        free(ring_buf->buf);
        ring_buf->buf = NULL;
    }

    free(ring_buf);
    ring_buf = NULL;
}

/**
 * Check whether the ring buffer is empty
 */
bool ring_buffer_empty(RingBuffer *ring_buf)
{
    if (!ring_buf) return false;

    return ring_buf->head == ring_buf->tail;
}

/**
 * Check whether the ring buffer is fill
 */
bool ring_buffer_full(RingBuffer *ring_buf)
{
    if (!ring_buf) return false;

    return (ring_buf->tail + 1) % ring_buf->num == ring_buf->head;
}

/**
 * Put the data into ring buffer
 */
bool ring_buffer_put(RingBuffer *ring_buf, void *data)
{
    if (!ring_buf) return false;

    if (ring_buffer_full(ring_buf)) return false;

    memcpy(ring_buf->buf + ring_buf->tail * ring_buf->size, data, ring_buf->size);

    ring_buf->tail = (ring_buf->tail + 1) % ring_buf->num;

    return true;
}

/**
 * Get the data from ring buffer
 */
bool ring_buffer_get(RingBuffer *ring_buf, void *data)
{
    if (!ring_buf) return false;

    if (ring_buffer_empty(ring_buf)) return false;

    memcpy(data, ring_buf->buf + ring_buf->head * ring_buf->size, ring_buf->size);

    ring_buf->head = (ring_buf->head + 1) % ring_buf->num;

    return true;
}