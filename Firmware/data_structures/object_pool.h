#pragma once
/*
 * @file  object_pool.h
 * @brief Fixed-capacity pool of trivial objects.
 */

#include <array>
#include <atomic>
#include <cstddef>
#include <memory>
#include <new>
#include <type_traits>

#include "osal/critical_section.h"

namespace data_structures {

/**
 * @brief Allocates fixed-size objects from statically allocated storage.
 *
 * Allocation and release execute inside a platform critical section.
 *
 * @tparam T            Trivial object type stored in the pool.
 * @tparam kNumItems    Number of objects in the pool.
 * @tparam kAlignment   Alignment of each object.
 */
template <typename T, size_t kNumItems, size_t kAlignment = alignof(T)>
class ObjectPool;

/**
 * @brief Pool of aligned byte arrays.
 *
 * @tparam kItemSize   Size of each buffer in bytes.
 * @tparam kNumItems   Number of buffers in the pool.
 * @tparam kAlignment  Alignment of each buffer.
 */
template <size_t kItemSize, size_t kNumItems, size_t kAlignment = sizeof(void*)>
using MemoryPool = ObjectPool<std::array<uint8_t, kItemSize>, kNumItems, kAlignment>;

/**
 * @brief A byte buffer with a length field.
 */
template <size_t kBufferSizeBytes>
struct StaticBuffer {
    std::array<uint8_t, kBufferSizeBytes> bytes;
    size_t length;
};

/**
 * @brief Pool of aligned StaticBuffer<>
 *
 * @tparam kBufferSizeBytes  Size of each buffer in bytes.
 * @tparam kNumBuffers       Number of buffers in the pool.
 * @tparam kAlignment        Alignment of each buffer.
 */
template <size_t kBufferSizeBytes, size_t kNumBuffers, size_t kAlignment = sizeof(void*)>
using StaticBufferPool = ObjectPool<StaticBuffer<kBufferSizeBytes>, kNumBuffers, kAlignment>;

/*
 * ObjectPool class implementation
 */
template <typename T, size_t kNumItems, size_t kAlignment>
class ObjectPool {
    static_assert(std::is_trivial<T>::value, "T must be a trivial type");
    static_assert(kAlignment >= alignof(T), "kAlignment must be at least alignof(T)");

public:
    using Object = T;
    using ObjectPoolType = ObjectPool<T, kNumItems, kAlignment>;

    struct Deleter {
        ObjectPoolType* allocator{nullptr};
        void operator()(Object* object) const noexcept {
            if ((allocator != nullptr) && (object != nullptr)) {
                allocator->free(object);
            }
        }
    };

    /// A unique pointer which returns the object to the pool on destruction.
    using Handle = std::unique_ptr<Object, Deleter>;

    /** @brief Initializes the pool with all objects available. */
    ObjectPool() {
        for (Chunk& chunk : chunks_) {
            chunk.next = free_list_head_;
            free_list_head_ = &chunk;
        }
    }

    /**
     * @brief Allocates one object.
     * @return Handle to a default-constructed object, or an empty handle when exhausted.
     */
    Handle allocate() noexcept {
        osal::RaiiCriticalSection cs{};

        if (free_list_head_ == nullptr) {
            return nullptr;
        }

        Chunk* const new_chunk{free_list_head_};
        free_list_head_ = free_list_head_->next;
        num_items_free_.fetch_sub(1U);

        Object* const object{::new (static_cast<void*>(&new_chunk->object)) Object};
        return {object, Deleter{this}};
    }

    /** @return Number of objects currently available. */
    size_t numChunksFree() const noexcept {
        return num_items_free_.load();
    }

    ObjectPool(const ObjectPool&) = delete;
    ObjectPool& operator=(const ObjectPool&) = delete;
    ObjectPool(ObjectPool&&) = delete;
    ObjectPool& operator=(ObjectPool&&) = delete;

private:
    friend struct Deleter;

    union alignas(kAlignment) Chunk {
        Object object;
        Chunk* next;
    };

    // TODO: guard against double-free
    /**
     * @brief Returns an object to the pool.
     * @param[in] object Object previously returned by this pool and not already freed.
     */
    void free(Object* object) noexcept {
        osal::RaiiCriticalSection cs{};

        Chunk* const chunk{reinterpret_cast<Chunk*>(object)};
        chunk->next = free_list_head_;
        free_list_head_ = chunk;
        num_items_free_.fetch_add(1U);
    }

    std::array<Chunk, kNumItems> chunks_;
    Chunk* free_list_head_{nullptr};
    std::atomic<size_t> num_items_free_{kNumItems};
};

}  // namespace data_structures
