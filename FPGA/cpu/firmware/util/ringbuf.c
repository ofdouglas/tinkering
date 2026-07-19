#include "ringbuf.h"

size_t increment_wrapping(size_t value, size_t max_value) {
    value += 1;
    return value >= max_value ? 0 : value;
}

void ringbuf_init(ringbuf_t* ringbuf, volatile uint8_t* buffer, size_t buffer_size) {
    ringbuf->buffer = buffer;
    ringbuf->buffer_size = buffer_size;
    ringbuf->write_index = 0;
    ringbuf->read_index = 0;
}

bool ringbuf_is_empty(ringbuf_t* ringbuf) {
    return ringbuf->write_index == ringbuf->read_index;
}

bool ringbuf_is_full(ringbuf_t* ringbuf) {
    size_t next_write_index = increment_wrapping(ringbuf->write_index, ringbuf->buffer_size);
    return next_write_index == ringbuf->read_index;
}

bool ringbuf_enqueue(ringbuf_t* ringbuf, uint8_t data) {
    if (ringbuf_is_full(ringbuf)) {
        return false;
    }
    ringbuf->buffer[ringbuf->write_index] = data;
    ringbuf->write_index = increment_wrapping(ringbuf->write_index, ringbuf->buffer_size);
    return true;
}

bool ringbuf_dequeue(ringbuf_t* ringbuf, uint8_t* data) {
    if (ringbuf_is_empty(ringbuf)) {
        return false;
    }
    *data = ringbuf->buffer[ringbuf->read_index];
    ringbuf->read_index = increment_wrapping(ringbuf->read_index, ringbuf->buffer_size);
    return true;
}