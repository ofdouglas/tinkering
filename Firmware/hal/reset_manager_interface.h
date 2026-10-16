#pragma once

namespace hal {

class ResetManagerInterface {
public:
    virtual ~ResetManagerInterface() = default;

    // Read the reset cause register and store the value.
    virtual void readResetCause() noexcept = 0;

    // Log the reset cause to the console.
    // @pre readResetCause() completed successfully.
    virtual void logResetCause() noexcept = 0;

    // Reset the device.
    virtual void reset() noexcept = 0;
};

} // namespace hal
