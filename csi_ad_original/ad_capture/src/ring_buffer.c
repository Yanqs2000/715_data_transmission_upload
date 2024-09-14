#include "ring_buffer.h"
//缓冲区初始化
RingBuffer *ring_buffer_init(uint32_t buf_size, uint32_t buf_num)
{
    RingBuffer *ring_buf = NULL;

    ring_buf = (RingBuffer *)malloc(sizeof(RingBuffer));
    if (!ring_buf) 
    {
        perror("malloc");
        return NULL;
    }
    //环形缓冲区机制
    /*类似数组，假设容量为6，开始的0位置为head指针位置，最后的5位置是tail指针位置*/
    ring_buf->num    = buf_num;
    ring_buf->size   = buf_size;
    ring_buf->head   = 0;//head索引 - 在这里表示读出的位置索引
    ring_buf->tail   = 0;//tail索引 - 在这里表示写入的位置索引

    ring_buf->buf = (void *)malloc(buf_size * buf_num);//为缓冲区开辟一块内存空间
    if (!ring_buf->buf)  
    {
        perror("malloc");
        free(ring_buf);
        ring_buf = NULL;
    }

    return ring_buf;
}

//缓冲区销毁，防止内存泄漏
void ring_buffer_destory(RingBuffer *ring_buf)
{
    if (!ring_buf) return;

    if (ring_buf->buf) 
    {
        free(ring_buf->buf); //释放内存，以免内存泄漏
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
    //若ring_buf未定义，则直接返回false
    if (!ring_buf) return false;
    //若head和tail相等，则说明缓冲区为空，返回true
    return ring_buf->head == ring_buf->tail;
}

/**
 * Check whether the ring buffer is fill
 */
bool ring_buffer_full(RingBuffer *ring_buf)
{
    //若ring_buf未定义，则直接返回false
    if (!ring_buf) return false;
    // 当head指针比tail指针少一个时（0和-1），表明缓冲区是满的。（例如head为0，tail为4，数量为5）
    // 若tail+1除以缓冲区数量所得余数等于head，则说明缓冲区已满，返回true
    return (ring_buf->tail + 1) % ring_buf->num == ring_buf->head;
}

/**
 * Put the data into ring buffer
 */
bool ring_buffer_put(RingBuffer *ring_buf, void *data)
{
    if (!ring_buf) return false;
    //把数据放入环形缓冲区，缓冲区满则无法放入
    if (ring_buffer_full(ring_buf)) return false;
    //memepy函数（深拷贝）将data数据复制到ring_buf缓冲区中,其写入位置为tail指针
    //ring_buf->buf为整个环形缓冲区的起始位置，整个公式为计算当前tail索引在缓冲区的实际位置
    memcpy(ring_buf->buf + ring_buf->tail * ring_buf->size, data, ring_buf->size);
    //tail指针的相对位置
    ring_buf->tail = (ring_buf->tail + 1) % ring_buf->num;

    return true;
}

/**
 * Get the data from ring buffer
 */
bool ring_buffer_get(RingBuffer *ring_buf, void *data)
{
    if (!ring_buf) return false;
    //从缓冲区中取出数据，缓冲区为空则无法取出
    if (ring_buffer_empty(ring_buf)) return false;
    //memepy函数（深拷贝）将ring_buf缓冲区中的数据复制到data中,其读取位置为head指针
    memcpy(data, ring_buf->buf + ring_buf->head * ring_buf->size, ring_buf->size);
    //head指针的相对位置
    ring_buf->head = (ring_buf->head + 1) % ring_buf->num;

    return true;
}