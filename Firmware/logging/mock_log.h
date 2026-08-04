#pragma once

// Host unit tests: link logging/mock_log.cpp instead of logging/log.cpp.

namespace logging::mock {

void reset() noexcept;

} // namespace logging::mock
