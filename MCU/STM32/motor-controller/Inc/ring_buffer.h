#pragma once

#include <array>
#include <atomic>
#include <cstdint>
#include <cstddef>
#include <type_traits>

/** @brief Queue which is thread-safe for a single producer and a single consumer.
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
        const writer{write_index_.load()};
        const reader{read_index_.load()};

        if (writer >= reader) {
            return writer - reader;
        } else {
            const size_t num_upper{kCapacity + 1 - reader};
            const size_t num_lower{writer};
            return num_upper + num_lower;
        }
    }

    bool enqueue(const T& item) {
        if (isFull()) {
            return false;
        }
        buffer_[write_index_] = item;
        write_index_ = increment(write_index_);
        return true;
    }

    bool dequeue(T& item) {
        if (isEmpty()) {
            return false;
        }
        item = buffer_[read_index_];
        read_index_ = increment(read_index_);
        return true;
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
