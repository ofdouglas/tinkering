/**
  ******************************************************************************
  * @file    deferred_logger.h
  * @brief   Deferred logging API
  ******************************************************************************
  */

#pragma once

#include "static_string.h"
#include "log.hpp"

#include <array>
#include <cstdint>
#include <cstring>

namespace logging {

enum class ItemType : uint8_t {
    kNone = 1U,
    kUint32,
    kInt32,
    kString
};

inline bool isValidType(const ItemType& type)  {
    return (type == ItemType::kUint32) || (type == ItemType::kInt32) || (type == ItemType::kString);
}

struct ItemHeader {
    static constexpr uint8_t kEscapeChar{27}; // 'ESC'
    static constexpr uint8_t kMaxSize{255};

    ItemHeader() = default;
    ItemHeader(ItemType type, uint8_t size) 
        : escape_char_(kEscapeChar), type_(type), size_(size) {}

    bool isValid() const {
        return isValidType(type_) && (escape_char_ == kEscapeChar);
    }

    uint8_t escape_char_{kEscapeChar};
    ItemType type_{ItemType::kNone};
    uint8_t size_{};
    uint8_t reserved_{};    // TODO: implement checksum
};

static_assert(sizeof(ItemHeader) == 4U, "ItemHeader size is incorrect");

template <size_t kBufferSize = 100>
class SerializedLog {
public:
    SerializedLog(const char* file, uint32_t line, logging::Level level) 
        : file_(file), line_(line), level_(level) {}

    SerializedLog& operator<<(uint32_t value) {
        const size_t bytes_needed{sizeof(ItemHeader) + sizeof(uint32_t)};
        if ((size_ + bytes_needed) <= kBufferSize) {
            appendItem(ItemHeader{ItemType::kUint32, sizeof(uint32_t)}, value);
        }
        return *this;
    }

    SerializedLog& operator<<(int32_t value) {
        const size_t bytes_needed{sizeof(ItemHeader) + sizeof(int32_t)};
        if ((size_ + bytes_needed) <= kBufferSize) {
            appendItem(ItemHeader{ItemType::kInt32, sizeof(int32_t)}, value);
        }
        return *this;
    }

    SerializedLog& operator<<(const char* str) {
        return appendString(str, strlen(str));
    }

    SerializedLog& operator<<(StaticString<> str) {
        return appendString(str.data(), str.length());
    }

    SerializedLog& appendString(const char* str, size_t length) {
        // TODO: report truncation
        length = length > ItemHeader::kMaxSize ? ItemHeader::kMaxSize : length;

        const size_t bytes_needed{sizeof(ItemHeader) + length + 1U}; // +1 for null-terminator
        if ((size_ + bytes_needed) <= kBufferSize) {
            appendBytes(ItemHeader{ItemType::kString, length}, str, length);
        }
        return *this;
    }

    void deserialize() {
        logging::LocalLogger output(file_, line_, level_);

        uint8_t* ptr{buffer_.data()};
        uint8_t* const end{ptr + size_};

        while ((ptr + sizeof(ItemHeader)) < end) {
            if (*ptr != ItemHeader::kEscapeChar) {
                ptr++;
                continue;
            }

            ItemHeader header{};
            memcpy(&header, ptr, sizeof(ItemHeader));
            if (!header.isValid()) {
                ptr++;
                continue;
            }

            // TODO: sanity check header.size_?
            ptr += sizeof(ItemHeader);
            if ((ptr + header.size_) > end) {
                break;  // TODO: report error
            }

            switch (header.type_) {
                case ItemType::kUint32:
                {
                    uint32_t value{};
                    memcpy(&value, ptr, sizeof(uint32_t));
                    output << value;
                    break;
                }
                case ItemType::kInt32:
                {
                    int32_t value{};
                    memcpy(&value, ptr, sizeof(int32_t));
                    output << value;
                    break;
                }
                case ItemType::kString:
                {
                    StaticString<> str{reinterpret_cast<const char*>(ptr), header.size_};
                    output << str;
                    break;
                }
                case ItemType::kNone: // intentional fall-through
                default:
                    break;  // TODO: report error
            }

            ptr += header.size_;
        }
    }

private:
    template <typename Item>
    void appendItem(ItemHeader header, Item item) {
        uint8_t* ptr = buffer_.data() + size_;
        memcpy(ptr, &header, sizeof(ItemHeader));
        memcpy(ptr + sizeof(ItemHeader), &item, sizeof(Item));
        size_ += sizeof(ItemHeader) + sizeof(Item);
    }

    void appendBytes(ItemHeader header, const void* bytes, size_t n) {
        uint8_t* ptr = buffer_.data() + size_;
        memcpy(ptr, &header, sizeof(ItemHeader));
        memcpy(ptr + sizeof(ItemHeader), bytes, n);
        size_ += sizeof(ItemHeader) + n;
    }

    const char* file_{};
    uint32_t line_{};
    logging::Level level_{};
    std::array<uint8_t, kBufferSize> buffer_{};
    size_t size_{};
};


class DeferredLogger {
public:
    DeferredLogger(const char* file, uint32_t line, logging::Level level)
        : log_data_(file, line, level) {}

    DeferredLogger& operator<<(uint32_t value) {
        log_data_ << value;
        return *this;
    }

    DeferredLogger& operator<<(int32_t value) {
        log_data_ << value;
        return *this;
    }

    DeferredLogger& operator<<(const char* str) {
        log_data_ << str;
        return *this;
    }

    DeferredLogger& operator<<(const StaticString<>& str) {
        log_data_ << str;
        return *this;
    }

    ~DeferredLogger() {
        // TODO: send the serialized log to the sink
        log_data_.deserialize();
    }

private:
    SerializedLog<> log_data_; 
};

} // namespace logging

#define DEFERRED_LOG_INFO() (::logging::DeferredLogger(__FILE__, __LINE__, ::logging::Level::Info))
#define DEFERRED_LOG_WARN() (::logging::DeferredLogger(__FILE__, __LINE__, ::logging::Level::Warn))
#define DEFERRED_LOG_ERROR() (::logging::DeferredLogger(__FILE__, __LINE__, ::logging::Level::Error))
#define DEFERRED_LOG_FATAL() (::logging::DeferredLogger(__FILE__, __LINE__, ::logging::Level::Fatal))