#ifndef RESET_INTERFACE_H
#define RESET_INTERFACE_H

namespace hal {

class ResetInterface {
public:
    // TODO: add reset types (read reason after reset, write reason before reset)
    virtual ~ResetInterface() = default;
    virtual void reset() noexcept = 0;
};

} // namespace hal

#endif // HAL_RESET_INTERFACE_H