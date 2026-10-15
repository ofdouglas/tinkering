#include <array>
#include <cstddef>
#include <cstdint>
#include <type_traits>

#include <gtest/gtest.h>

#include "data_structures/memory_pool.h"

namespace {

/*
 * Test plan (MemoryPool<kChunkBufferSize, kNumChunks, kChunkAlignment>)
 *
 * Happy path:
 *   - Construction exposes all configured chunks as free
 *   - Allocation returns writable, distinct buffers and decrements the count
 *   - Freeing buffers restores capacity without corrupting live buffers
 *   - A freed buffer can be allocated again
 *
 * Expected errors:
 *   - Allocation from an exhausted pool returns nullptr without changing state
 *
 * Edge cases / boundary values:
 *   - A one-byte, one-chunk pool supports allocation, exhaustion, and reuse
 *   - A pool configured with zero chunks remains empty
 *   - Explicit over-alignment is honored
 *
 * Adversarial / stress-style:
 *   - Repeated allocation and free cycles preserve the free count
 *   - Copy and move operations remain disabled
 */

constexpr size_t kBufferSize{8U};
constexpr size_t kCapacity{4U};
using TestPool = data_structures::MemoryPool<kBufferSize, kCapacity>;

class MemoryPoolTest : public ::testing::Test {
protected:
    TestPool pool_;
};

// Verifies construction makes every configured chunk available.
TEST_F(MemoryPoolTest, InitiallyAllChunksAreFree) {
    EXPECT_EQ(kCapacity, pool_.numChunksFree());
}

// Verifies allocation returns writable, distinct buffers and updates the free count.
TEST_F(MemoryPoolTest, AllocationsAreDistinctAndWritable) {
    std::array<TestPool::Buffer*, kCapacity> buffers{};

    for (size_t i{0U}; i < buffers.size(); ++i) {
        buffers[i] = pool_.allocate();
        ASSERT_NE(nullptr, buffers[i]) << "i=" << i;
        EXPECT_EQ(kCapacity - i - 1U, pool_.numChunksFree());

        buffers[i]->fill(static_cast<uint8_t>(i + 1U));
        for (size_t j{0U}; j < i; ++j) {
            EXPECT_NE(buffers[j], buffers[i]);
        }
    }

    for (size_t i{0U}; i < buffers.size(); ++i) {
        for (uint8_t byte : *buffers[i]) {
            EXPECT_EQ(static_cast<uint8_t>(i + 1U), byte);
        }
    }
}

// Verifies allocation from an exhausted pool fails without changing its state.
TEST_F(MemoryPoolTest, AllocateWhenExhaustedReturnsNull) {
    for (size_t i{0U}; i < kCapacity; ++i) {
        ASSERT_NE(nullptr, pool_.allocate());
    }

    EXPECT_EQ(0U, pool_.numChunksFree());
    EXPECT_EQ(nullptr, pool_.allocate());
    EXPECT_EQ(nullptr, pool_.allocate());
    EXPECT_EQ(0U, pool_.numChunksFree());
}

// Verifies freeing a buffer restores one slot while preserving another live buffer.
TEST_F(MemoryPoolTest, FreeRestoresCapacityAndPreservesLiveBuffer) {
    TestPool::Buffer* const first{pool_.allocate()};
    TestPool::Buffer* const second{pool_.allocate()};
    ASSERT_NE(nullptr, first);
    ASSERT_NE(nullptr, second);
    second->fill(0xA5U);

    pool_.free(first);

    EXPECT_EQ(kCapacity - 1U, pool_.numChunksFree());
    for (uint8_t byte : *second) {
        EXPECT_EQ(0xA5U, byte);
    }
}

// Verifies the most recently freed storage is available for a later allocation.
TEST_F(MemoryPoolTest, FreedBufferCanBeAllocatedAgain) {
    TestPool::Buffer* const original{pool_.allocate()};
    ASSERT_NE(nullptr, original);

    pool_.free(original);
    TestPool::Buffer* const reused{pool_.allocate()};

    EXPECT_EQ(original, reused);
    EXPECT_EQ(kCapacity - 1U, pool_.numChunksFree());
}

// Verifies freeing all allocated buffers restores the pool to full capacity.
TEST_F(MemoryPoolTest, FreeingAllBuffersRestoresFullCapacity) {
    std::array<TestPool::Buffer*, kCapacity> buffers{};
    for (TestPool::Buffer*& buffer : buffers) {
        buffer = pool_.allocate();
        ASSERT_NE(nullptr, buffer);
    }

    for (TestPool::Buffer* buffer : buffers) {
        pool_.free(buffer);
    }

    EXPECT_EQ(kCapacity, pool_.numChunksFree());
}

// Verifies the minimum useful buffer and capacity support exhaustion and reuse.
TEST(MemoryPoolBoundaryTest, SingleByteSingleChunk) {
    using SingleChunkPool = data_structures::MemoryPool<1U, 1U>;
    SingleChunkPool pool;

    SingleChunkPool::Buffer* const buffer{pool.allocate()};
    ASSERT_NE(nullptr, buffer);
    (*buffer)[0] = 0x5AU;
    EXPECT_EQ(0x5AU, (*buffer)[0]);
    EXPECT_EQ(nullptr, pool.allocate());

    pool.free(buffer);
    EXPECT_EQ(1U, pool.numChunksFree());
    EXPECT_NE(nullptr, pool.allocate());
}

// Verifies a zero-capacity pool consistently reports exhaustion.
TEST(MemoryPoolBoundaryTest, ZeroChunksRemainEmpty) {
    data_structures::MemoryPool<1U, 0U> pool;

    EXPECT_EQ(0U, pool.numChunksFree());
    EXPECT_EQ(nullptr, pool.allocate());
}

// Verifies the configured chunk alignment is reflected in returned buffer addresses.
TEST(MemoryPoolAlignmentTest, ExplicitAlignmentIsHonored) {
    constexpr size_t kAlignment{64U};
    data_structures::MemoryPool<3U, 3U, kAlignment> pool;

    for (size_t i{0U}; i < 3U; ++i) {
        const auto* const buffer{pool.allocate()};
        ASSERT_NE(nullptr, buffer);
        const auto address{reinterpret_cast<std::uintptr_t>(buffer)};
        EXPECT_EQ(0U, address % kAlignment);
    }
}

// Verifies repeated allocation/free cycles leave the pool count consistent.
TEST(MemoryPoolStressTest, RepeatedAllocateAndFreeCycles) {
    data_structures::MemoryPool<16U, 2U> pool;

    for (size_t cycle{0U}; cycle < 100U; ++cycle) {
        auto* const first{pool.allocate()};
        auto* const second{pool.allocate()};
        ASSERT_NE(nullptr, first) << "cycle=" << cycle;
        ASSERT_NE(nullptr, second) << "cycle=" << cycle;
        EXPECT_EQ(nullptr, pool.allocate()) << "cycle=" << cycle;

        pool.free(first);
        pool.free(second);
        EXPECT_EQ(2U, pool.numChunksFree()) << "cycle=" << cycle;
    }
}

// Verifies ownership cannot accidentally be duplicated or transferred.
TEST(MemoryPoolTypeTraitsTest, CopyAndMoveAreDisabled) {
    EXPECT_FALSE(std::is_copy_constructible<TestPool>::value);
    EXPECT_FALSE(std::is_copy_assignable<TestPool>::value);
    EXPECT_FALSE(std::is_move_constructible<TestPool>::value);
    EXPECT_FALSE(std::is_move_assignable<TestPool>::value);
}

} // namespace
