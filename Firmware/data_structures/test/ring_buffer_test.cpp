#include <array>
#include <vector>

#include <gtest/gtest.h>

#include "data_structures/ring_buffer.h"

namespace {

/*
 * Test plan (RingBuffer<T, kCapacity>)
 *
 * Happy path:
 *   - Default construction: empty, not full, size 0
 *   - Single enqueue / dequeue preserves value and returns true
 *   - FIFO ordering for multiple items
 *   - Fill to kCapacity items: isFull, size == kCapacity
 *   - Drain all items after fill
 *   - enqueue(Span): all items when space available
 *   - dequeue(Span): fills output span, returns count
 *   - peek: reads head without consuming; repeated peek unchanged
 *   - clear: removes all elements; idempotent on empty
 *   - Index wrap: enqueue/dequeue across physical buffer boundary
 *
 * Expected errors / misuse (API contract):
 *   - enqueue on full -> false, queue unchanged (no overwrite)
 *   - dequeue on empty -> false, output unchanged
 *   - peek on empty -> false
 *   - enqueue(Span) when insufficient space -> false; items before failure remain
 *   - dequeue(Span) on empty -> 0
 *
 * Edge cases / boundary values:
 *   - kCapacity == 1 (minimum): single slot fill, full, drain
 *   - size() at 0, 1, kCapacity-1, kCapacity
 *   - dequeue(Span) with span larger than available data
 *   - enqueue(Span) with empty span -> true
 *   - dequeue(Span) with zero-length span -> 0
 *
 * Adversarial / stress-style (single-threaded):
 *   - Many alternating enqueue/dequeue cycles
 *   - Large span enqueue attempt against nearly-full buffer
 *   - Non-trivial T (struct) copy in/out
 */

constexpr size_t kDefaultCapacity{10U};

class RingBufferCapacity10Test : public ::testing::Test {
protected:
    RingBuffer<int, kDefaultCapacity> buffer_;
};

// Verifies the initial state of the buffer is empty.
TEST_F(RingBufferCapacity10Test, InitiallyEmpty) {
    EXPECT_TRUE(buffer_.isEmpty());
    EXPECT_FALSE(buffer_.isFull());
    EXPECT_EQ(0U, buffer_.size());
}

// Verifies one enqueue updates size and a dequeue returns the same value.
TEST_F(RingBufferCapacity10Test, EnqueueDequeueSingle) {
    ASSERT_TRUE(buffer_.enqueue(42));
    EXPECT_FALSE(buffer_.isEmpty());
    EXPECT_EQ(1U, buffer_.size());

    int out{-1};
    ASSERT_TRUE(buffer_.dequeue(out));
    EXPECT_EQ(42, out);
    EXPECT_TRUE(buffer_.isEmpty());
}

// Verifies items are dequeued in the order they were enqueued.
TEST_F(RingBufferCapacity10Test, FifoOrdering) {
    for (int i = 0; i < 5; ++i) {
        ASSERT_TRUE(buffer_.enqueue(i));
    }
    for (int i = 0; i < 5; ++i) {
        int out{-1};
        ASSERT_TRUE(buffer_.dequeue(out));
        EXPECT_EQ(i, out);
    }
}

// Verifies the buffer reports full with size equal to kCapacity when every slot is used.
TEST_F(RingBufferCapacity10Test, FillToCapacity) {
    for (size_t i = 0U; i < kDefaultCapacity; ++i) {
        ASSERT_TRUE(buffer_.enqueue(static_cast<int>(i))) << "i=" << i;
    }
    EXPECT_TRUE(buffer_.isFull());
    EXPECT_FALSE(buffer_.isEmpty());
    EXPECT_EQ(kDefaultCapacity, buffer_.size());
}

// Verifies enqueue on a full buffer fails without dropping the oldest element.
TEST_F(RingBufferCapacity10Test, EnqueueWhenFullFails) {
    for (size_t i = 0U; i < kDefaultCapacity; ++i) {
        ASSERT_TRUE(buffer_.enqueue(static_cast<int>(i)));
    }
    EXPECT_FALSE(buffer_.enqueue(999));
    EXPECT_EQ(kDefaultCapacity, buffer_.size());

    int head{-1};
    ASSERT_TRUE(buffer_.dequeue(head));
    EXPECT_EQ(0, head);
}

// Verifies dequeue on an empty buffer fails and does not modify the output reference.
TEST_F(RingBufferCapacity10Test, DequeueWhenEmptyFails) {
    int out{77};
    EXPECT_FALSE(buffer_.dequeue(out));
    EXPECT_EQ(77, out);
}

// Verifies peek on an empty buffer fails and does not modify the output reference.
TEST_F(RingBufferCapacity10Test, PeekWhenEmptyFails) {
    int out{88};
    EXPECT_FALSE(buffer_.peek(out));
    EXPECT_EQ(88, out);
}

// Verifies peek reads the head element without changing size or subsequent peek results.
TEST_F(RingBufferCapacity10Test, PeekDoesNotConsume) {
    ASSERT_TRUE(buffer_.enqueue(5));
    ASSERT_TRUE(buffer_.enqueue(6));

    int first{-1};
    int second{-1};
    ASSERT_TRUE(buffer_.peek(first));
    EXPECT_EQ(5, first);
    EXPECT_EQ(2U, buffer_.size());
    ASSERT_TRUE(buffer_.peek(second));
    EXPECT_EQ(5, second);

    ASSERT_TRUE(buffer_.dequeue(first));
    EXPECT_EQ(5, first);
}

// Verifies clear removes all queued elements.
TEST_F(RingBufferCapacity10Test, ClearEmptiesBuffer) {
    for (int i = 0; i < 4; ++i) {
        ASSERT_TRUE(buffer_.enqueue(i));
    }
    buffer_.clear();
    EXPECT_TRUE(buffer_.isEmpty());
    EXPECT_EQ(0U, buffer_.size());
}

// Verifies clear on an already empty buffer leaves it empty.
TEST_F(RingBufferCapacity10Test, ClearOnEmptyIsNoOp) {
    buffer_.clear();
    EXPECT_TRUE(buffer_.isEmpty());
}

// Verifies span enqueue places all items when enough free space exists.
TEST_F(RingBufferCapacity10Test, EnqueueSpanAllSucceed) {
    const std::array<int, 4> items{{10, 11, 12, 13}};
    ASSERT_TRUE(buffer_.enqueue(util::Span<const int>(items.data(), items.size())));
    EXPECT_EQ(4U, buffer_.size());
    for (int expected = 10; expected <= 13; ++expected) {
        int out{-1};
        ASSERT_TRUE(buffer_.dequeue(out));
        EXPECT_EQ(expected, out);
    }
}

// Verifies span enqueue stops on first failure but keeps items already enqueued.
TEST_F(RingBufferCapacity10Test, EnqueueSpanPartialFailureRetainsPrefix) {
    for (size_t i = 0U; i < kDefaultCapacity - 1U; ++i) {
        ASSERT_TRUE(buffer_.enqueue(0));
    }
    const std::array<int, 3> items{{1, 2, 3}};
    EXPECT_FALSE(buffer_.enqueue(util::Span<const int>(items.data(), items.size())));
    EXPECT_TRUE(buffer_.isFull());
    EXPECT_EQ(kDefaultCapacity, buffer_.size());

    for (size_t i = 0U; i < kDefaultCapacity - 1U; ++i) {
        int out{-1};
        ASSERT_TRUE(buffer_.dequeue(out));
        EXPECT_EQ(0, out);
    }
    int tail{-1};
    ASSERT_TRUE(buffer_.dequeue(tail));
    EXPECT_EQ(1, tail);
    EXPECT_TRUE(buffer_.isEmpty());
}

// Verifies span dequeue returns the number of elements written and preserves FIFO order.
TEST_F(RingBufferCapacity10Test, DequeueSpanReturnsCount) {
    for (int i = 0; i < 3; ++i) {
        ASSERT_TRUE(buffer_.enqueue(i * 10));
    }
    std::array<int, 5> out{};
    const size_t n = buffer_.dequeue(util::Span<int>(out));
    EXPECT_EQ(3U, n);
    EXPECT_EQ(0, out[0]);
    EXPECT_EQ(10, out[1]);
    EXPECT_EQ(20, out[2]);
    EXPECT_TRUE(buffer_.isEmpty());
}

// Verifies span dequeue on an empty buffer returns zero.
TEST_F(RingBufferCapacity10Test, DequeueSpanEmptyReturnsZero) {
    std::array<int, 4> out{{1, 2, 3, 4}};
    EXPECT_EQ(0U, buffer_.dequeue(util::Span<int>(out)));
}

// Verifies span dequeue stops when the buffer is drained even if the span is larger.
TEST_F(RingBufferCapacity10Test, DequeueSpanPartialFill) {
    ASSERT_TRUE(buffer_.enqueue(7));
    ASSERT_TRUE(buffer_.enqueue(8));
    std::array<int, 10> out{};
    EXPECT_EQ(2U, buffer_.dequeue(util::Span<int>(out)));
    EXPECT_EQ(7, out[0]);
    EXPECT_EQ(8, out[1]);
    EXPECT_TRUE(buffer_.isEmpty());
}

// Verifies enqueue of a zero-length span succeeds and leaves the buffer empty.
TEST_F(RingBufferCapacity10Test, EnqueueEmptySpan) {
    const std::array<int, 0> items{};
    EXPECT_TRUE(buffer_.enqueue(util::Span<const int>(items.data(), 0U)));
    EXPECT_TRUE(buffer_.isEmpty());
}

// Verifies zero-length span dequeue is a no-op and does not consume elements.
TEST_F(RingBufferCapacity10Test, DequeueZeroLengthSpan) {
    ASSERT_TRUE(buffer_.enqueue(1));
    std::array<int, 1> out{};
    EXPECT_EQ(0U, buffer_.dequeue(util::Span<int>(out.data(), 0U)));
    EXPECT_EQ(1U, buffer_.size());
}

// Verifies repeated fill-and-drain cycles work when indices wrap the backing array.
TEST_F(RingBufferCapacity10Test, WrapAroundIndices) {
    for (int round = 0; round < 3; ++round) {
        for (size_t i = 0U; i < kDefaultCapacity; ++i) {
            const int value = static_cast<int>(round * 100 + i);
            ASSERT_TRUE(buffer_.enqueue(value)) << "round=" << round << " i=" << i;
        }
        for (size_t i = 0U; i < kDefaultCapacity; ++i) {
            int out{-1};
            const int expected = static_cast<int>(round * 100 + i);
            ASSERT_TRUE(buffer_.dequeue(out));
            EXPECT_EQ(expected, out);
        }
        EXPECT_TRUE(buffer_.isEmpty());
    }
}

// Verifies size() matches the number of elements from empty through full.
TEST_F(RingBufferCapacity10Test, SizeTracksPartialFill) {
    EXPECT_EQ(0U, buffer_.size());
    ASSERT_TRUE(buffer_.enqueue(1));
    EXPECT_EQ(1U, buffer_.size());
    for (size_t i = 2U; i < kDefaultCapacity; ++i) {
        ASSERT_TRUE(buffer_.enqueue(static_cast<int>(i)));
    }
    EXPECT_EQ(kDefaultCapacity - 1U, buffer_.size());
    ASSERT_TRUE(buffer_.enqueue(static_cast<int>(kDefaultCapacity)));
    EXPECT_EQ(kDefaultCapacity, buffer_.size());
}

// Verifies many single-element enqueue/dequeue cycles do not corrupt state.
TEST_F(RingBufferCapacity10Test, AlternatingEnqueueDequeue) {
    for (int cycle = 0; cycle < 50; ++cycle) {
        ASSERT_TRUE(buffer_.enqueue(cycle));
        int out{-1};
        ASSERT_TRUE(buffer_.dequeue(out));
        EXPECT_EQ(cycle, out);
    }
    EXPECT_TRUE(buffer_.isEmpty());
}

struct Payload {
    int a{0};
    int b{0};
    bool operator==(const Payload& other) const {
        return a == other.a && b == other.b;
    }
};

// Verifies a non-scalar T is copied correctly through enqueue and dequeue.
TEST(RingBufferStructType, CopyInOut) {
    RingBuffer<Payload, 4> buffer;
    const Payload in{3, 4};
    ASSERT_TRUE(buffer.enqueue(in));
    Payload out{};
    ASSERT_TRUE(buffer.dequeue(out));
    EXPECT_EQ(in, out);
}

// Verifies minimum kCapacity of 1: one element, full/empty transitions, and overflow rejection.
TEST(RingBufferMinCapacity, SingleElementSlot) {
    RingBuffer<int, 1> buffer;
    EXPECT_TRUE(buffer.isEmpty());
    EXPECT_FALSE(buffer.isFull());
    ASSERT_TRUE(buffer.enqueue(100));
    EXPECT_TRUE(buffer.isFull());
    EXPECT_FALSE(buffer.isEmpty());
    EXPECT_EQ(1U, buffer.size());
    EXPECT_FALSE(buffer.enqueue(101));

    int out{-1};
    ASSERT_TRUE(buffer.dequeue(out));
    EXPECT_EQ(100, out);
    EXPECT_TRUE(buffer.isEmpty());
}

} // namespace
