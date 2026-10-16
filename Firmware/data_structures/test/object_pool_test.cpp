#include <array>
#include <cstddef>
#include <cstdint>
#include <type_traits>

#include <gtest/gtest.h>

#include "data_structures/object_pool.h"

namespace {

/*
 * Test plan (ObjectPool<T, kNumItems, kAlignment>, MemoryPool alias, StaticBuffer)
 *
 * Happy path:
 *   - Construction exposes all configured chunks as free
 *   - Allocation returns writable, distinct objects and decrements the count
 *   - Destroying a handle restores capacity without corrupting live objects
 *   - A released object can be allocated again
 *   - ObjectPool stores StaticBuffer with independent payload and length
 *   - MemoryPool is ObjectPool of a byte array
 *
 * Expected errors:
 *   - Allocation from an exhausted pool returns an empty handle without changing state
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
    std::array<TestPool::Handle, kCapacity> buffers{};

    for (size_t i{0U}; i < buffers.size(); ++i) {
        buffers[i] = pool_.allocate();
        ASSERT_NE(nullptr, buffers[i]) << "i=" << i;
        EXPECT_EQ(kCapacity - i - 1U, pool_.numChunksFree());

        buffers[i]->fill(static_cast<uint8_t>(i + 1U));
        for (size_t j{0U}; j < i; ++j) {
            EXPECT_NE(buffers[j].get(), buffers[i].get());
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
    std::array<TestPool::Handle, kCapacity> buffers{};
    for (size_t i{0U}; i < kCapacity; ++i) {
        buffers[i] = pool_.allocate();
        ASSERT_NE(nullptr, buffers[i]);
    }

    EXPECT_EQ(0U, pool_.numChunksFree());
    EXPECT_EQ(nullptr, pool_.allocate());
    EXPECT_EQ(nullptr, pool_.allocate());
    EXPECT_EQ(0U, pool_.numChunksFree());
}

// Verifies destroying a handle restores one slot while preserving another live buffer.
TEST_F(MemoryPoolTest, HandleDestructorRestoresCapacityAndPreservesLiveBuffer) {
    TestPool::Handle first{pool_.allocate()};
    TestPool::Handle second{pool_.allocate()};
    ASSERT_NE(nullptr, first);
    ASSERT_NE(nullptr, second);
    second->fill(0xA5U);

    first.reset();

    EXPECT_EQ(kCapacity - 1U, pool_.numChunksFree());
    for (uint8_t byte : *second) {
        EXPECT_EQ(0xA5U, byte);
    }
}

// Verifies the most recently released storage is available for a later allocation.
TEST_F(MemoryPoolTest, ReleasedBufferCanBeAllocatedAgain) {
    TestPool::Handle original{pool_.allocate()};
    ASSERT_NE(nullptr, original);
    TestPool::Object* const original_ptr{original.get()};

    original.reset();
    TestPool::Handle reused{pool_.allocate()};

    EXPECT_EQ(original_ptr, reused.get());
    EXPECT_EQ(kCapacity - 1U, pool_.numChunksFree());
}

// Verifies destroying all allocated handles restores the pool to full capacity.
TEST_F(MemoryPoolTest, DestroyingAllHandlesRestoresFullCapacity) {
    {
        std::array<TestPool::Handle, kCapacity> buffers{};
        for (TestPool::Handle& buffer : buffers) {
            buffer = pool_.allocate();
            ASSERT_NE(nullptr, buffer);
        }
        EXPECT_EQ(0U, pool_.numChunksFree());
    }

    EXPECT_EQ(kCapacity, pool_.numChunksFree());
}

// Verifies the minimum useful buffer and capacity support exhaustion and reuse.
TEST(MemoryPoolBoundaryTest, SingleByteSingleChunk) {
    using SingleChunkPool = data_structures::MemoryPool<1U, 1U>;
    SingleChunkPool pool;

    SingleChunkPool::Handle buffer{pool.allocate()};
    ASSERT_NE(nullptr, buffer);
    (*buffer)[0] = 0x5AU;
    EXPECT_EQ(0x5AU, (*buffer)[0]);
    EXPECT_EQ(nullptr, pool.allocate());

    buffer.reset();
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
    using Pool = data_structures::MemoryPool<3U, 3U, kAlignment>;
    std::array<Pool::Handle, 3U> buffers{};

    for (size_t i{0U}; i < buffers.size(); ++i) {
        buffers[i] = pool.allocate();
        ASSERT_NE(nullptr, buffers[i]);
        const auto address{reinterpret_cast<std::uintptr_t>(buffers[i].get())};
        EXPECT_EQ(0U, address % kAlignment);
    }
}

// Verifies repeated allocation/free cycles leave the pool count consistent.
TEST(MemoryPoolStressTest, RepeatedAllocateAndFreeCycles) {
    data_structures::MemoryPool<16U, 2U> pool;

    for (size_t cycle{0U}; cycle < 100U; ++cycle) {
        auto first{pool.allocate()};
        auto second{pool.allocate()};
        ASSERT_NE(nullptr, first) << "cycle=" << cycle;
        ASSERT_NE(nullptr, second) << "cycle=" << cycle;
        EXPECT_EQ(nullptr, pool.allocate()) << "cycle=" << cycle;

        first.reset();
        second.reset();
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

// Verifies MemoryPool remains a byte-array specialization of ObjectPool.
TEST(MemoryPoolAliasTest, IsObjectPoolOfByteArray) {
    using Expected =
        data_structures::ObjectPool<std::array<uint8_t, kBufferSize>, kCapacity, sizeof(void*)>;
    static_assert(std::is_same<TestPool, Expected>::value, "");
    EXPECT_TRUE((std::is_same<TestPool::Object, std::array<uint8_t, kBufferSize>>::value));
}

using StaticBufferPool = data_structures::ObjectPool<data_structures::StaticBuffer<16U>, kCapacity>;
static_assert(std::is_trivial<data_structures::StaticBuffer<16U>>::value, "");

// Verifies ObjectPool can store a StaticBuffer with independent payload and length.
TEST(ObjectPoolTest, AllocatesWritableStaticBuffers) {
    StaticBufferPool pool;
    StaticBufferPool::Handle first{pool.allocate()};
    StaticBufferPool::Handle second{pool.allocate()};
    ASSERT_NE(nullptr, first);
    ASSERT_NE(nullptr, second);
    EXPECT_EQ(kCapacity - 2U, pool.numChunksFree());

    first->length = 3U;
    first->bytes[0] = 0x11U;
    first->bytes[1] = 0x22U;
    first->bytes[2] = 0x33U;
    second->length = 1U;
    second->bytes[0] = 0xAAU;

    EXPECT_EQ(3U, first->length);
    EXPECT_EQ(0x11U, first->bytes[0]);
    EXPECT_EQ(0x22U, first->bytes[1]);
    EXPECT_EQ(0x33U, first->bytes[2]);
    EXPECT_EQ(1U, second->length);
    EXPECT_EQ(0xAAU, second->bytes[0]);
}

} // namespace
