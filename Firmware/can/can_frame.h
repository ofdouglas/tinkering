#ifndef CAN_ID_H
#define CAN_ID_H

#include <stdint.h>
#include <array>
#include <cstring>
#include <type_traits>
#include "util/span.h"

namespace Can {

enum class Rv32SocCanId : uint16_t {
    kBootloaderCommand  = 0x700,
    kBootloaderResponse = 0x701
};

template <typename CanIdEnumType>
class CanId {
public:
    static_assert(std::is_enum<CanIdEnumType>::value, "CanIdEnumType must be an enum type");
    static_assert(std::is_same<typename std::underlying_type<CanIdEnumType>::type, uint16_t>::value,
                  "CanIdEnumType must be an unsigned 16-bit enum type");

    CanId() = default;
    CanId(CanIdEnumType can_id) : can_id_(can_id) {}
    CanId(uint16_t can_id) : can_id_(static_cast<CanIdEnumType>(can_id)) {}

    uint16_t raw_id() const { return static_cast<uint16_t>(can_id_); }
    CanIdEnumType enum_id() const { return can_id_; }

private:
    CanIdEnumType can_id_{};
};

template <typename IdEnumT>
struct CanFrameWireData {
    Can::CanId<IdEnumT> can_id_;
    uint8_t data_len_;
    std::array<uint8_t, 8U> data_;
    uint32_t crc_;
};

template <typename IdEnumT>
class CanFrame {
public:
    enum : size_t {
        kHdlcPayloadSize = 1U + sizeof(uint16_t) + sizeof(uint8_t) + 8U + sizeof(uint32_t)
    };

    CanFrame() = default;
    CanFrame(CanId<IdEnumT> can_id) : can_id_(can_id) {}

    CanFrame(CanId<IdEnumT> can_id, uint64_t data) : can_id_(can_id) {
        setData(data);
    }

    CanFrame(CanId<IdEnumT> can_id, util::Span<const uint8_t> data) : can_id_(can_id) {
        setData(data);
    }

    void setData(uint64_t data) {
        setData(util::Span<const uint8_t>(reinterpret_cast<const uint8_t*>(&data), sizeof(data)));
    }

    bool setData(util::Span<const uint8_t> data) {
        if (data.size() > data_.size()) {
            return false;
        }
        data_.fill(0U);
        std::memcpy(data_.data(), data.data(), data.size());
        data_len_ = static_cast<uint8_t>(data.size());
        return true;
    }

    void setCrc(uint32_t crc) {
        crc_ = crc;
    }

    size_t storeHdlcPayload(uint8_t protocol_type, util::Span<uint8_t> buffer) const {
        if (buffer.size() < kHdlcPayloadSize) {
            return 0U;
        }
        size_t index = 0U;
        buffer[index++] = protocol_type;
        const uint16_t id = can_id_.raw_id();
        std::memcpy(buffer.data() + index, &id, sizeof(id));
        index += sizeof(id);
        buffer[index++] = data_len_;
        std::memcpy(buffer.data() + index, data_.data(), data_.size());
        index += data_.size();
        std::memcpy(buffer.data() + index, &crc_, sizeof(crc_));
        index += sizeof(crc_);
        return index;
    }

    bool loadHdlcPayload(uint8_t expected_protocol_type, util::Span<const uint8_t> buffer) {
        if (buffer.size() < kHdlcPayloadSize) {
            return false;
        }
        if (buffer[0] != expected_protocol_type) {
            return false;
        }
        size_t index = 1U;
        uint16_t id = 0U;
        std::memcpy(&id, buffer.data() + index, sizeof(id));
        index += sizeof(id);
        const uint8_t data_len = buffer[index++];
        can_id_ = CanId<IdEnumT>(id);
        if (data_len > data_.size()) {
            return false;
        }
        data_.fill(0U);
        std::memcpy(data_.data(), buffer.data() + index, data_.size());
        index += data_.size();
        data_len_ = data_len;
        std::memcpy(&crc_, buffer.data() + index, sizeof(crc_));
        return true;
    }

    CanId<IdEnumT> can_id() const { return can_id_; }
    uint8_t data_len() const { return data_len_; }
    util::Span<const uint8_t> data() const { return util::Span<const uint8_t>(data_.data(), data_len_); }
    uint64_t data64() const {
        uint64_t value = 0U;
        std::memcpy(&value, data_.data(), sizeof(value));
        return value;
    }
    uint32_t crc() const { return crc_; }

private:
    CanId<IdEnumT> can_id_{};
    uint8_t data_len_{0};
    std::array<uint8_t, 8U> data_{};
    uint32_t crc_{0};
};

} // namespace Can
#endif // CAN_ID_H
