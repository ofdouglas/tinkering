/*
 * Test plan (BasicEventChannel / EventTracer)
 * - BasicEventChannel: initial count 0, recordEvent increments count
 * - hasEventChanged / shouldLogEvent reflect unread events
 * - getCount clears the changed flag until the next recordEvent
 * - EventTracer::processEvents is a no-op when nothing was recorded
 * - EventTracer::processEvents consumes pending events via the logging path
 */

#include "logging/event_trace.h"

#include <array>
#include <gtest/gtest.h>

namespace logging {

class EventTraceTest : public ::testing::Test {
protected:
    // TODO: in order to use virtual functions, we need to use pointers.
    std::array<BasicEventChannel, 3> basic_event_channels_{{
        BasicEventChannel(EventHandle{1, "Test Event 1"}),
        BasicEventChannel(EventHandle{2, "Test Event 2"}),
        BasicEventChannel(EventHandle{3, "Test Event 3"}),
    }};

    EventTracer<10> tracer_{basic_event_channels_};
};

// New channels start with zero occurrences and no pending change.
TEST_F(EventTraceTest, BasicEventChannelStartsAtZero) {
    EXPECT_EQ(basic_event_channels_[0].getCount(), 0U);
    EXPECT_FALSE(basic_event_channels_[0].hasEventChanged());
    EXPECT_FALSE(basic_event_channels_[0].shouldLogEvent());
}

// recordEvent increments the occurrence count.
TEST_F(EventTraceTest, RecordEventIncrementsCount) {
    basic_event_channels_[0].recordEvent();
    basic_event_channels_[0].recordEvent();
    EXPECT_EQ(basic_event_channels_[0].getCount(), 2U);
}

// Reading the count clears the changed flag until the next recordEvent.
TEST_F(EventTraceTest, GetCountClearsChangedFlag) {
    basic_event_channels_[1].recordEvent();
    EXPECT_TRUE(basic_event_channels_[1].hasEventChanged());
    EXPECT_EQ(basic_event_channels_[1].getCount(), 1U);
    EXPECT_FALSE(basic_event_channels_[1].hasEventChanged());
}

// processEvents with no new events leaves all channels unchanged.
TEST_F(EventTraceTest, ProcessEventsNoOpWhenNothingRecorded) {
    tracer_.processEvents();
    for (const auto& channel : basic_event_channels_) {
        EXPECT_EQ(channel.getCount(), 0U);
        EXPECT_FALSE(channel.hasEventChanged());
    }
}

// processEvents consumes pending events so hasEventChanged is false afterward.
TEST_F(EventTraceTest, ProcessEventsConsumesPendingEvents) {
    basic_event_channels_[2].recordEvent();
    EXPECT_TRUE(basic_event_channels_[2].hasEventChanged());
    tracer_.processEvents();
    EXPECT_FALSE(basic_event_channels_[2].hasEventChanged());
    EXPECT_EQ(basic_event_channels_[2].getCount(), 1U);
}

} // namespace logging
