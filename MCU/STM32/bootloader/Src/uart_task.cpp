class UartTask :  public bare_metal::PeriodicTask {
    public:
        UartTask() : bare_metal::PeriodicTask(std::chrono::milliseconds(100U)) {}
    
        void tick() noexcept override {
            uint8_t next{};
            while (index_ < (buffer_.size() + 1)) {
                if (!uart_rx_buffer.dequeue(next)) {
                    break;
                }
    
                uart_task_count++;
                buffer_[index_++] = static_cast<char>(next);
                
                if (next == '\n') {
                    buffer_[index_] = '\0';
                    LOG_INFO() << reinterpret_cast<char*>(buffer_.data());
                    index_ = 0;
                }
            }
        }
    
    private:
        std::array<char, 64U> buffer_{};
        size_t index_{};
    };