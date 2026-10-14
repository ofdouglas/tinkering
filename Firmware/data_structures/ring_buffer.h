#pragma once
/*
 * @file  ring_buffer.h
 * @brief Ring buffer which is thread-safe for a single producer and a single consumer.
 */
 
#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include <cstddef>
#include <type_traits>

#include "util/span.h"

/** 
 * @brief FIFO queue which is thread-safe for a single producer and a single consumer.
 *
 * @tparam T The type of the items in the queue.
 * @tparam kCapacity The capacity of the queue. (kCapacity + 1) T objects will be allocated.
 */
template <typename T, size_t kCapacity>
class RingBuffer {
public:
    static_assert(std::is_default_constructible<T>::value, "T must be default constructible");
    static_assert(std::is_copy_constructible<T>::value, "T must be copy constructible");
    static_assert(std::is_copy_assignable<T>::value, "T must be copy assignable");
    static_assert(kCapacity > 0U, "kCapacity must be greater than 0");

    RingBuffer() = default;

    RingBuffer(const RingBuffer&) = delete;
    RingBuffer& operator=(const RingBuffer&) = delete;
    RingBuffer(RingBuffer&&) = delete;
    RingBuffer& operator=(RingBuffer&&) = delete;

    bool isEmpty() const {
        return write_index_ == read_index_;
    }

    bool isFull() const {
        return increment(write_index_) == read_index_;
    }

    size_t size() const {
        const size_t writer = write_index_.load();
        const size_t reader = read_index_.load();

        if (writer >= reader) {
            return writer - reader;
        } else {
            const size_t num_upper{kCapacity + 1 - reader};
            const size_t num_lower{writer};
            return num_upper + num_lower;
        }
    }

    size_t available() const {
        return buffer_.size() - 1U - size();
    }

    bool enqueue(const T& item) {
        if (isFull()) {
            return false;
        }
        buffer_[write_index_] = item;
        write_index_ = increment(write_index_);
        return true;
    }

    bool enqueue(util::Span<const T> items) {
        if (items.size() == 0U) {
            return true;
        }

        const size_t num_to_enqueue{std::min(items.size(), available())};
        if ((num_to_enqueue == 0U)) {
            return false;
        }

        size_t writer{write_index_.load()};
        const size_t copy_size_hi{std::min(num_to_enqueue, buffer_.size() - writer)};
        const size_t copy_size_low{num_to_enqueue - copy_size_hi};

        std::copy_n(items.data(), copy_size_hi, &buffer_[writer]);
        std::copy_n(items.data() + copy_size_hi, copy_size_low, &buffer_[0U]);

        writer += num_to_enqueue;
        if (writer >= buffer_.size()) {
            writer -= buffer_.size();
        }
        write_index_.store(writer);

        return num_to_enqueue == items.size();
    }

    bool dequeue(T& item) {
        if (isEmpty()) {
            return false;
        }
        item = buffer_[read_index_];
        read_index_ = increment(read_index_);
        return true;
    }

    size_t dequeue(util::Span<T> data) {
        size_t i = 0U;
        while (i < data.size()) {
            if (!dequeue(data[i])) {
                break;
            }
            i++;
        }
        return i;
    }

    // For DMA (use when starting a transfer)
    util::Span<const T> readBulk() {
        size_t reader{read_index_.load()};
        const size_t writer{write_index_.load()};
        const size_t upper_bound{writer >= reader ? writer : buffer_.size()};

        util::Span<const T> result(&buffer_[reader], upper_bound - reader);
        return result;
    }

    void discardBulk(size_t count) {
        size_t reader{read_index_.load()};

        reader += count;
        if (reader >= buffer_.size()) {
            reader -= buffer_.size();
        }
        read_index_.store(reader);
    }

    bool peek(T& item) const {
        if (isEmpty()) {
            return false;
        }
        item = buffer_[read_index_];
        return true;
    }

    void clear() {
        while (!isEmpty()) {
            T item;
            (void)dequeue(item);
        }
    }

private:
    size_t increment(size_t index) const {
        return (index == kCapacity) ? 0U : index + 1U;
    }

    std::array<T, kCapacity + 1U> buffer_{};
    std::atomic<size_t> write_index_{};
    std::atomic<size_t> read_index_{};
};
