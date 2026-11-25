#include "drivers/mcp2515.h"

#include "util/string_utils.h"
#include "drivers/spi.h"
#include "drivers/time.h"

#include <stddef.h>
#include <stdint.h>

namespace {

constexpr uint8_t kInstructionReset{0xc0U};
constexpr uint8_t kInstructionRead{0x03U};
constexpr uint8_t kInstructionWrite{0x02U};
constexpr uint8_t kInstructionBitModify{0x05U};
constexpr uint8_t kInstructionRequestToSendBuffer0{0x81U};

constexpr uint8_t kRegisterCanstat{0x0eU};
constexpr uint8_t kRegisterCanctrl{0x0fU};
constexpr uint8_t kRegisterCnf3{0x28U};
constexpr uint8_t kRegisterCnf2{0x29U};
constexpr uint8_t kRegisterCnf1{0x2aU};
constexpr uint8_t kRegisterCaninte{0x2bU};
constexpr uint8_t kRegisterCanintf{0x2cU};
constexpr uint8_t kRegisterTxb0ctrl{0x30U};
constexpr uint8_t kRegisterTxb0sidh{0x31U};
constexpr uint8_t kRegisterRxb0ctrl{0x60U};
constexpr uint8_t kRegisterRxb0sidh{0x61U};
constexpr uint8_t kRegisterRxb1ctrl{0x70U};
constexpr uint8_t kRegisterRxb1sidh{0x71U};

constexpr uint8_t kModeMask{0xe0U};
constexpr uint8_t kConfigurationMode{0x80U};
constexpr uint8_t kReceiveBuffer0Flag{0x01U};
constexpr uint8_t kReceiveBuffer1Flag{0x02U};
constexpr uint8_t kTransmitRequest{0x08U};
constexpr uint8_t kExtendedIdentifier{0x08U};
constexpr uint8_t kRemoteFrame{0x40U};

constexpr uint32_t kStandardIdentifierMax{0x7ffU};
constexpr uint32_t kExtendedIdentifierMax{0x1fffffffU};
constexpr size_t kIdentifierAndDlcSize{5U};
constexpr size_t kMaximumPayloadSize{8U};

void sendInstruction(uint8_t instruction) {
    static_cast<void>(spi_transfer_byte_blocking(instruction));
    spi_end_transaction();
}

uint8_t readRegister(uint8_t address) {
    static_cast<void>(spi_transfer_byte_blocking(kInstructionRead));
    static_cast<void>(spi_transfer_byte_blocking(address));
    const uint8_t value{spi_transfer_byte_blocking(0x00U)};
    spi_end_transaction();
    return value;
}

void readRegisters(uint8_t address, uint8_t* values, size_t length) {
    static_cast<void>(spi_transfer_byte_blocking(kInstructionRead));
    static_cast<void>(spi_transfer_byte_blocking(address));
    for (size_t i{0U}; i < length; ++i) {
        values[i] = spi_transfer_byte_blocking(0x00U);
    }
    spi_end_transaction();
}

void writeRegister(uint8_t address, uint8_t value) {
    static_cast<void>(spi_transfer_byte_blocking(kInstructionWrite));
    static_cast<void>(spi_transfer_byte_blocking(address));
    static_cast<void>(spi_transfer_byte_blocking(value));
    spi_end_transaction();
}

void writeRegisters(uint8_t address, const uint8_t* values, size_t length) {
    static_cast<void>(spi_transfer_byte_blocking(kInstructionWrite));
    static_cast<void>(spi_transfer_byte_blocking(address));
    for (size_t i{0U}; i < length; ++i) {
        static_cast<void>(spi_transfer_byte_blocking(values[i]));
    }
    spi_end_transaction();
}

void bitModify(uint8_t address, uint8_t mask, uint8_t value) {
    static_cast<void>(spi_transfer_byte_blocking(kInstructionBitModify));
    static_cast<void>(spi_transfer_byte_blocking(address));
    static_cast<void>(spi_transfer_byte_blocking(mask));
    static_cast<void>(spi_transfer_byte_blocking(value));
    spi_end_transaction();
}

void encodeIdentifier(const can_frame_t& frame, uint8_t* registers) {
    if (frame.is_extended_id) {
        registers[0] = static_cast<uint8_t>(frame.can_id >> 21U);
        registers[1] = static_cast<uint8_t>(((frame.can_id >> 13U) & 0xe0U) | kExtendedIdentifier |
                                            ((frame.can_id >> 16U) & 0x03U));
        registers[2] = static_cast<uint8_t>(frame.can_id >> 8U);
        registers[3] = static_cast<uint8_t>(frame.can_id);
    } else {
        registers[0] = static_cast<uint8_t>(frame.can_id >> 3U);
        registers[1] = static_cast<uint8_t>((frame.can_id & 0x07U) << 5U);
        registers[2] = 0U;
        registers[3] = 0U;
    }
}

uint32_t decodeIdentifier(const uint8_t* registers, bool is_extended_id) {
    if (is_extended_id) {
        return (static_cast<uint32_t>(registers[0]) << 21U) |
               (static_cast<uint32_t>(registers[1] & 0xe0U) << 13U) |
               (static_cast<uint32_t>(registers[1] & 0x03U) << 16U) |
               (static_cast<uint32_t>(registers[2]) << 8U) | static_cast<uint32_t>(registers[3]);
    }
    return (static_cast<uint32_t>(registers[0]) << 3U) | (registers[1] >> 5U);
}

}  // namespace

bool mcp2515_init_500kbps_8mhz(void) {
    spi_init();
    sendInstruction(kInstructionReset);
    mtim_delay_ns(NS_PER_MILLISEC);

    const uint8_t mode = readRegister(kRegisterCanstat) & kModeMask;
    if (mode != kConfigurationMode) {
        log_value("ERROR: MCP2515 is not in configuration mode: ", mode);
        return false;
    }

    // Fosc=8 MHz, BRP=0: 8 time quanta per 2 us bit. The sample point is
    // 75%: Sync=1, PropSeg=1, PhaseSeg1=4, PhaseSeg2=2, SJW=1.
    writeRegister(kRegisterCnf1, 0x00U);
    writeRegister(kRegisterCnf2, 0x98U);
    writeRegister(kRegisterCnf3, 0x01U);

    // Accept all valid standard and extended frames. RXB0 rollover allows RXB1
    // to hold another frame while the polling application services RXB0.
    writeRegister(kRegisterRxb0ctrl, 0x64U);
    writeRegister(kRegisterRxb1ctrl, 0x60U);
    writeRegister(kRegisterCaninte, 0x00U);
    writeRegister(kRegisterCanintf, 0x00U);

    bitModify(kRegisterCanctrl, kModeMask, 0x00U);
    for (uint16_t attempt{0U}; attempt < 1000U; ++attempt) {
        if ((readRegister(kRegisterCanstat) & kModeMask) == 0U) {
            return true;
        }
    }
    return false;
}

bool mcp2515_send(const can_frame_t* frame) {
    if ((frame == nullptr) || (frame->dlc > kMaximumPayloadSize) ||
        (frame->is_extended_id && (frame->can_id > kExtendedIdentifierMax)) ||
        (!frame->is_extended_id && (frame->can_id > kStandardIdentifierMax)) ||
        ((readRegister(kRegisterTxb0ctrl) & kTransmitRequest) != 0U)) {
        return false;
    }

    uint8_t tx_buffer[kIdentifierAndDlcSize + kMaximumPayloadSize]{};
    encodeIdentifier(*frame, tx_buffer);
    tx_buffer[4] = frame->dlc;
    for (uint8_t i{0U}; i < frame->dlc; ++i) {
        tx_buffer[kIdentifierAndDlcSize + i] = frame->data[i];
    }
    writeRegisters(kRegisterTxb0sidh, tx_buffer, kIdentifierAndDlcSize + frame->dlc);
    sendInstruction(kInstructionRequestToSendBuffer0);
    return true;
}

bool mcp2515_receive(can_frame_t* frame) {
    if (frame == nullptr) {
        return false;
    }

    const uint8_t flags{readRegister(kRegisterCanintf)};
    uint8_t receive_flag{0U};
    uint8_t receive_buffer_address{0U};
    if ((flags & kReceiveBuffer0Flag) != 0U) {
        receive_flag = kReceiveBuffer0Flag;
        receive_buffer_address = kRegisterRxb0sidh;
    } else if ((flags & kReceiveBuffer1Flag) != 0U) {
        receive_flag = kReceiveBuffer1Flag;
        receive_buffer_address = kRegisterRxb1sidh;
    } else {
        return false;
    }

    uint8_t rx_buffer[kIdentifierAndDlcSize + kMaximumPayloadSize]{};
    readRegisters(receive_buffer_address, rx_buffer, sizeof(rx_buffer));

    if ((rx_buffer[4] & kRemoteFrame) != 0U) {
        bitModify(kRegisterCanintf, receive_flag, 0x00U);
        return false;
    }

    frame->is_extended_id = (rx_buffer[1] & kExtendedIdentifier) != 0U;
    frame->can_id = decodeIdentifier(rx_buffer, frame->is_extended_id);
    frame->dlc = static_cast<uint8_t>(rx_buffer[4] & 0x0fU);
    if (frame->dlc > kMaximumPayloadSize) {
        frame->dlc = kMaximumPayloadSize;
    }
    for (uint8_t i{0U}; i < frame->dlc; ++i) {
        frame->data[i] = rx_buffer[kIdentifierAndDlcSize + i];
    }

    bitModify(kRegisterCanintf, receive_flag, 0x00U);
    return true;
}
