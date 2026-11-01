#include <stdint.h>
#include <stddef.h>

#include "system/intrinsics.h"
#include "drivers/time.h"
#include "drivers/uart.h"

/******************************************************************************
 * Constants and type definitions
 *****************************************************************************/
#define TX_BUFFER_SIZE 32

typedef enum state_e {
    STATE_IDLE,
    STATE_DOWNLOAD_IMAGE,
    STATE_DOWNLOAD_CRC,
    STATE_VERIFY,
    STATE_BOOT_APP,
    STATE_ERROR
} State;

typedef enum result_e { RESULT_PENDING, RESULT_FAILURE, RESULT_SUCCESS } Result;

const uint64_t kNanosecondsPerMillisecond = 1000000ULL;
const uint64_t kAdvertiseIntervalNs = kNanosecondsPerMillisecond * 100ULL;

/******************************************************************************
 * Global objects
 *****************************************************************************/
uint8_t uart_tx_ringbuf_storage[TX_BUFFER_SIZE];
ringbuf_t uart_tx_ringbuf;

uint64_t next_advertise_time_ns = 0;

uint8_t crc_buffer[4] = {0};
uint8_t crc_buffer_index = 0;

size_t img_bytes_received = 0;

/******************************************************************************
 * SRAM sections (linker symbols)
 *****************************************************************************/
extern uint8_t _SRAM_TEXT_START;
extern uint8_t _SRAM_TEXT_END;
extern uint8_t _SRAM_TEXT_LENGTH;

uint8_t* const image_buffer = (uint8_t*)(&_SRAM_TEXT_START);
uint8_t* const image_buffer_end = (uint8_t*)(&_SRAM_TEXT_END);

const size_t kMaxImgBytes = (size_t)(image_buffer_end - image_buffer);

/******************************************************************************
 * Helper functions
 *****************************************************************************/

uint32_t crc32_bytes(const uint8_t* bytes, size_t length) {
    uint32_t crc = 0xFFFFFFFF;

    for (size_t i = 0; i < length; i++) {
        crc ^= bytes[i];
        for (int bit = 7; bit >= 0; bit--) {
            if (crc & 1u) {
                crc = (crc >> 1) ^ 0xEDB88320u;
            } else {
                crc >>= 1;
            }
        }
    }

    return ~crc;
}

// Polled TX queue service (non-blocking)
void run_tx_service_nonblocking() {
    uint8_t tx_byte = 0;
    if (ringbuf_peek(uart_tx_ringbuf, &tx_byte)) {
        if (uart_putchar_nonblocking(tx_byte)) {
            (void)ringbuf_dequeue(uart_tx_ringbuf, &tx_byte);
        }
    }
}

bool send_string_nonblocking(const char* str) {
    while (*str) {
        if (!ringbuf_enqueue(uart_tx_ringbuf, *str)) {
            return false;
        }
        str++;
    }
    return true;
}

void send_string_blocking(const char* str) {
    while (*str) {
        uart_send_byte_blocking(*str);
        str++;
    }
}

uint8_t nibble_from_char(char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    } else if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    } else if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    } else {
        return UINT8_MAX;
    }
}

bool is_hex_digit(char c) {
    return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
}

// Try to receive one ASCII-encoded hex byte from the host.
Result poll_hex_byte(char rx_byte, uint8_t* byte) {
    static bool byte_started = false;
    uint8_t partial_byte = 0;
    Result result = RESULT_PENDING;
    bool do_reset = true;

    if (!is_hex_digit(rx_byte)) {
        result = (rx_byte == ' ') ? RESULT_PENDING : RESULT_FAILURE;
    } else {
        uint8_t nibble = nibble_from_char(rx_byte);
        if (nibble == UINT8_MAX) {
            result = RESULT_FAILURE;
        }

        if (byte_started) {
            partial_byte = (nibble << 4) | partial_byte;
            byte_started = false;
            *byte = partial_byte;
            result = RESULT_SUCCESS;
        } else {
            partial_byte = nibble;
            byte_started = true;
            result = RESULT_PENDING;
            do_reset = false;
        }
    }

    if (do_reset) {
        byte_started = false;
        partial_byte = 0;
    }

    return result;
}

Result poll_message_start(char rx_byte, const char* const message, size_t message_length) {
    static int index = 0;

    if (index >= message_length) {
        return RESULT_FAILURE;
    }

    if (rx_byte == message[index]) {
        index++;
    } else {
        index = 0;
    }

    if (index == message_length) {
        return RESULT_SUCCESS;
    }

    return RESULT_PENDING;
}

void poll_advertise(void) {
    uint64_t now_ns = get_time_ns();

    if (now_ns >= next_advertise_time_ns) {
        next_advertise_time_ns = now_ns + kAdvertiseIntervalNs;
        send_string_nonblocking("BL_READY\n");
    }
}

Result poll_download_start(char rx_byte) {
    static const char* const kBootImgMsg = "BOOT_IMG";
    static const size_t kBootImgMsgLength = 8;
    return poll_message_start(rx_byte, kBootImgMsg, kBootImgMsgLength);
}

Result download_app(int rx_byte) {
    if (img_bytes_received >= kMaxImgBytes) {
        return RESULT_FAILURE;
    }

    if (rx_byte == '$') {
        return RESULT_SUCCESS;
    }

    uint8_t decoded_byte = 0;
    Result result = poll_hex_byte(rx_byte, &decoded_byte);
    if (result == RESULT_SUCCESS) {
        image_buffer[img_bytes_received++] = decoded_byte;
    }

    return RESULT_PENDING;
}

Result download_crc(int rx_byte) {
    if (crc_buffer_index > 4) {
        return RESULT_FAILURE;
    }

    if (rx_byte == ';') {
        return (crc_buffer_index == 4) ? RESULT_SUCCESS : RESULT_FAILURE;
    }

    uint8_t decoded_byte = 0;
    Result result = poll_hex_byte(rx_byte, &decoded_byte);
    if (result == RESULT_SUCCESS) {
        crc_buffer[crc_buffer_index++] = decoded_byte;
    }

    return RESULT_PENDING;
}

Result verify_app(void) {
    if (crc_buffer_index != 4) {
        return RESULT_FAILURE;
    }

    uint32_t computed_crc = crc32_bytes(image_buffer, img_bytes_received);
    uint32_t received_crc =
        (crc_buffer[0] << 24) | (crc_buffer[1] << 16) | (crc_buffer[2] << 8) | crc_buffer[3];
    return (computed_crc == received_crc) ? RESULT_SUCCESS : RESULT_FAILURE;
}

void infinite_loop(void) {
    while (true) {
        ;
    }
}

void boot_app(void) {
    void*(application_entry_point)() = (void (*)())image_buffer;
    application_entry_point();

    infinite_loop();  // should not get here
}

/******************************************************************************
 * Bootloader main loop
 *****************************************************************************/

int main(void) {
    State state = STATE_IDLE;
    Result result = RESULT_PENDING;
    int rx_byte = -1;

    ringbuf_init(&uart_tx_ringbuf, uart_tx_ringbuf_storage, sizeof(uart_tx_ringbuf_storage));

    do {
        run_tx_service_nonblocking();
        rx_byte = uart_getchar_nonblocking();

        switch (state) {
            case STATE_IDLE:
                if (rx_byte < 0) {
                    continue;
                }
                result = poll_download_start(rx_byte);
                if (result == RESULT_SUCCESS) {
                    state = STATE_DOWNLOAD_IMAGE;
                    send_string_nonblocking("DL_START\n");
                } else if (result == RESULT_FAILURE) {
                    state = STATE_ERROR;
                    send_string_nonblocking("ERR_START\n");
                }
                break;

            case STATE_DOWNLOAD_IMAGE:
                if (rx_byte < 0) {
                    continue;
                }
                result = download_app(rx_byte);
                if (result == RESULT_SUCCESS) {
                    state = STATE_DOWNLOAD_CRC;
                    send_string_nonblocking("DL_CRC\n");
                } else if (result == RESULT_FAILURE) {
                    state = STATE_ERROR;
                    send_string_nonblocking("ERR_DL_IMG\n");
                }
                break;

            case STATE_DOWNLOAD_CRC:
                if (rx_byte < 0) {
                    continue;
                }
                result = download_crc(rx_byte);
                if (result == RESULT_SUCCESS) {
                    state = STATE_VERIFY;
                    send_string_nonblocking("CHECK_CRC\n");
                } else if (result == RESULT_FAILURE) {
                    state = STATE_ERROR;
                    send_string_nonblocking("ERR_DL_CRC\n");
                }
                break;

            case STATE_VERIFY:
                result = verify_app();
                if (result == RESULT_SUCCESS) {
                    state = STATE_BOOT_APP;
                    // Block so the message gets logged before the app boots
                    send_string_blocking("BOOT_APP\n");
                } else if (result == RESULT_FAILURE) {
                    state = STATE_ERROR;
                    send_string_nonblocking("ERR_VERIFY\n");
                }
                break;

            case STATE_BOOT_APP:
                boot_app();
                break;

            case STATE_ERROR:
            default:  // intentional fall through
                infinite_loop();
                break;
        };

    } while (true);  // Exits through boot_app()

    infinite_loop();
}