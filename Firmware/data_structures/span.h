#ifndef SPAN_H
#define SPAN_H

#include <stddef.h>
#include <array>

template <typename T>
class Span {
public:
    Span() = default;
    Span(T* data, size_t size) : data_(data), size_(size) {}

    template <size_t N>
    Span(T (&array)[N]) : data_(array), size_(N) {}

    template <size_t N>
    Span(std::array<T, N>& array) : data_(array.data()), size_(N) {}

    template <size_t N>
    Span(const std::array<T, N>& array) : data_(array.data()), size_(N) {}

    const T& operator[](size_t i) const {
        return data_[i];
    }

    T& operator[](size_t i) {
        return data_[i];
    }

    T* data() const { return data_; }
    T* data() { return data_; }

    T* begin() const { return data_; }
    T* end() const { return data_ + size_; }

    size_t size() const { return size_; }
    bool empty() const { return size_ == 0U; }

    Span<T> subspan(size_t start, size_t length) const {
        return Span<T>(data_ + start, length);
    }

    Span<T> subspan(size_t start) const {
        return Span<T>(data_ + start, size_ - start);
    }

private:
    T* data_{};
    size_t size_{0U};
};

#endif // SPAN_H
