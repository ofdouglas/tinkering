#ifndef RINGBUF_H
#define RINGBUF_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct {
    volatile uint8_t* buffer;
    volatile size_t   buffer_size;   
    volatile size_t   write_index;
    volatile size_t   read_index;
} ringbuf_t;

void ringbuf_init(ringbuf_t* ringbuf, volatile uint8_t* buffer, size_t buffer_size);

bool ringbuf_is_empty(ringbuf_t* ringbuf);

bool ringbuf_is_full(ringbuf_t* ringbuf);

bool ringbuf_enqueue(ringbuf_t* ringbuf, uint8_t data);

bool ringbuf_dequeue(ringbuf_t* ringbuf, uint8_t* data);


#endif