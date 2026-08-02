
const char* toString(HAL_StatusTypeDef status) {
    switch (status) {
        case HAL_OK:        return "OK";        break;
        case HAL_ERROR:     return "ERROR";     break;
        case HAL_BUSY:      return "BUSY";      break;
        case HAL_TIMEOUT:   return "TIMEOUT";   break;
        default:            return "UNKNOWN";   break;
    }
    return "UNKNOWN";
}




class Task50Ms : public bare_metal::PeriodicTask {
    public:
        Task50Ms(Bsp& bsp) : bare_metal::PeriodicTask(std::chrono::milliseconds(50U)), bsp_(bsp) {}
    
        void tick() noexcept override {
            std::array<uint8_t, 16U> rx_buffer{};
            Span<uint8_t> rx_span{rx_buffer};
    
            if (bsp_.uartRead(rx_span, 1U)) {
                if ((rx_span[0] == '\n') || (rx_span[0] == '\r')) {
                    buffer_[index_] = '\0';
                    LOG_INFO() << "Received message: " << reinterpret_cast<char*>(buffer_.data());
                    index_ = 0;
                } else {
                    buffer_[index_++] = rx_span[0];
                }
            }
        }
    
    private:
        static constexpr size_t kPayloadBufferSize{64U};
        std::array<uint8_t, kPayloadBufferSize> buffer_{};
        size_t index_{};
        Bsp& bsp_;
    };
    
    
    // class Task5Ms : public bare_metal::PeriodicTask {
    // public:
    //     Task5Ms(Bsp& bsp) : bare_metal::PeriodicTask(std::chrono::milliseconds(5U)), bsp_(bsp) {}
    
    //     void tick() noexcept override {
    //         std::array<uint8_t, 1U> rx_buffer{};
    //         std::array<uint8_t, kPayloadBufferSize> rx_message{};
    
    //         while (bsp_.uartRead(Span<uint8_t>{rx_buffer}, 1U)) {
    //             LOG_INFO() << "Rx byte: " << rx_buffer[0];
    
    //             Span<const uint8_t> rx{rx_buffer.data(), 1U};
    //             bool msg_ready = receiver_.process(rx);
    //             if (msg_ready) {
    //                 size_t msg_size = receiver_.receivePayload(Span<uint8_t>{rx_message});
    //                 LOG_INFO() << "Received " << msg_size << " bytes: " << rx_message[0];
    //             }
    //         }
    //     }
    
    // private:
    //     static constexpr size_t kPayloadBufferSize{64U};
    
    //     Hdlc::Receiver<kPayloadBufferSize> receiver_;
    //     Bsp& bsp_;
    // };
        




void testFlash() noexcept {
    extern uint32_t _TEST_FLASH_START;

    uint32_t start_addr = reinterpret_cast<uint32_t>(&_TEST_FLASH_START);
    uint32_t value = *(reinterpret_cast<uint32_t*>(start_addr));
    LOG_INFO() << logging::Radix::Hexadecimal << "Initial value at " << start_addr << ": " << value;
    // return;

    HAL_StatusTypeDef status{};
    uint32_t sector_error{};

    if ((status = HAL_FLASH_Unlock()) != HAL_OK) {
        LOG_ERROR() << "Flash unlock failed: " << toString(status);
        return;
    }

    LOG_INFO() << "Flash unlocked";

    FLASH_EraseInitTypeDef erase_init = {
        FLASH_TYPEERASE_SECTORS,
        FLASH_SECTOR_3,
        1U,
        FLASH_VOLTAGE_RANGE_3
    };

    if ((status = HAL_FLASHEx_Erase(&erase_init, &sector_error)) != HAL_OK) {
        LOG_ERROR() << "Flash erase failed: " << toString(status) << logging::Radix::Hexadecimal << ", sector_error: " << sector_error;
        return;
    }

    value = *(reinterpret_cast<uint32_t*>(start_addr));
    LOG_INFO() << "Flash erase succesful";
    LOG_INFO() << logging::Radix::Hexadecimal << "Post-erase value at start_addr: " << value;

    uint64_t kData{0xDEADBEEF};
    if ((status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, start_addr, kData)) != HAL_OK) {
        LOG_INFO() << logging::Radix::Hexadecimal << "Flash program failed: " << toString(status);
        return;
    }

    value = *(reinterpret_cast<uint32_t*>(start_addr));
    LOG_INFO() << "Flash program succesful";
    LOG_INFO() << logging::Radix::Hexadecimal << "Post-program value at start_addr: " << value;
}
