#include <gtest/gtest.h>

#include "ring_buffer.h"

TEST(RingBuffer, StartsEmpty) {
    RingBuffer<int, 4> queue;
    EXPECT_TRUE(queue.isEmpty());
    EXPECT_FALSE(queue.isFull());
    EXPECT_EQ(queue.size(), 0U);
}

TEST(RingBuffer, EnqueueDequeueFifo) {
    RingBuffer<int, 2> queue;

    EXPECT_TRUE(queue.enqueue(10));
    EXPECT_TRUE(queue.enqueue(20));
    EXPECT_TRUE(queue.isFull());
    EXPECT_FALSE(queue.enqueue(30));

    int value = 0;
    ASSERT_TRUE(queue.dequeue(value));
    EXPECT_EQ(value, 10);
    ASSERT_TRUE(queue.dequeue(value));
    EXPECT_EQ(value, 20);
    EXPECT_TRUE(queue.isEmpty());
    EXPECT_FALSE(queue.dequeue(value));
}

TEST(RingBuffer, PeekDoesNotRemove) {
    RingBuffer<int, 4> queue;
    ASSERT_TRUE(queue.enqueue(42));

    int peeked = 0;
    ASSERT_TRUE(queue.peek(peeked));
    EXPECT_EQ(peeked, 42);
    EXPECT_EQ(queue.size(), 1U);

    int dequeued = 0;
    ASSERT_TRUE(queue.dequeue(dequeued));
    EXPECT_EQ(dequeued, 42);
}

TEST(RingBuffer, ClearEmptiesQueue) {
    RingBuffer<int, 4> queue;
    ASSERT_TRUE(queue.enqueue(1));
    ASSERT_TRUE(queue.enqueue(2));
    queue.clear();
    EXPECT_TRUE(queue.isEmpty());
}
