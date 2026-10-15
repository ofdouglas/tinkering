#pragma once
/*
 * @file  memory_pool.h
 * @brief Fixed-capacity pool of equally sized byte buffers.
 */

#include <array>
#include <atomic>
#include <cstdint>
#include <cstddef>
#include <new>

#include "util/span.h"
#include "logging/logging.h"
#include "osal/critical_section.h"

namespace data_structures {

/**
 * @brief Provides fixed-size buffers from statically allocated storage.
 *
 * Allocation and release execute inside a platform critical section.
 *
 * @tparam kChunkBufferSize Size of each buffer in bytes.
 * @tparam kNumChunks       Number of buffers in the pool.
 * @tparam kChunkAlignment  Alignment of each buffer.
 */
template <size_t kChunkBufferSize, size_t kNumChunks, size_t kChunkAlignment = sizeof(void*)>
class MemoryPool {
public:
    /// Byte-buffer type returned by the pool.
    using Buffer = std::array<uint8_t, kChunkBufferSize>;

    /** @brief Initializes the pool with all buffers available. */
    MemoryPool() {
        for (Chunk& c : chunks_) {
            c.next = free_list_head_;
            free_list_head_ = &c;
        }
    }

    /**
     * @brief Allocates one buffer.
     * @return Pointer to an uninitialized buffer, or nullptr when the pool is exhausted.
     */
    Buffer* allocate() noexcept {
        osal::RaiiCriticalSection cs{};

        if (free_list_head_ == nullptr) {
            return nullptr;
        }

        Chunk* new_chunk = free_list_head_;
        free_list_head_ = free_list_head_->next;
        num_chunks_free_.fetch_sub(1U);

        // Construct a Buffer in-place, to explicitly change the union type to Buffer.
        return ::new (static_cast<void*>(&new_chunk->buffer)) Buffer;
    }

    // TODO: guard against double-free
    /**
     * @brief Returns a buffer to the pool.
     * @param[in] buffer Buffer previously returned by this pool and not already freed.
     */
    void free(Buffer* buffer) noexcept {
        osal::RaiiCriticalSection cs{};

        Chunk* chunk = reinterpret_cast<Chunk*>(buffer);
        chunk->next = free_list_head_;
        free_list_head_ = chunk;
        num_chunks_free_.fetch_add(1);
    }

    /** @return Number of buffers currently available. */
    size_t numChunksFree() const noexcept { return num_chunks_free_.load(); }

    // Disable copying and moving
    MemoryPool(const MemoryPool&) = delete;
    MemoryPool& operator=(const MemoryPool&) = delete;
    MemoryPool(MemoryPool&&) = delete;
    MemoryPool& operator=(MemoryPool&&) = delete;
    
private:
    // Note: allocated chunks are represented as Buffer* 
    union alignas(kChunkAlignment) Chunk {
        Buffer buffer;
        Chunk* next;
    };

    std::array<Chunk, kNumChunks> chunks_;
    Chunk* free_list_head_{nullptr};
    std::atomic<size_t> num_chunks_free_{kNumChunks};
};

} // namespace data_structures
