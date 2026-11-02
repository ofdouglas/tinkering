#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "drivers/time.h"
#include "drivers/uart.h"
#include "linker/mem_map.h"

/******************************************************************************
 * Constants and type definitions
 *****************************************************************************/

typedef enum state_e {
    STATE_IDLE,
    STATE_DOWNLOAD_IMAGE,
    STATE_DOWNLOAD_CRC,
    STATE_VERIFY,
    STATE_BOOT_APP
} State;

typedef enum result_e { RESULT_PENDING, RESULT_FAILURE, RESULT_SUCCESS } Result;

#define IMAGE_BUFFER ((volatile uint8_t*)SRAM_TEXT_BASE)
static const size_t kMaxImgBytes = SRAM_TEXT_SIZE;
static const uint64_t kAdvertiseIntervalNs = 100ULL * NS_PER_MILLISEC;
static const char kAdvertiseMsg[] = "BL_READY\n";
static const char kBootImgMsg[] = "BOOT_IMG";
static const size_t kBootImgMsgLength = 8;

/******************************************************************************
 * Download / parser state
 *****************************************************************************/

static uint64_t next_advertise_time_ns = 0;
static size_t advertise_index = 0;

static uint8_t crc_buffer[4];
static uint8_t crc_buffer_index = 0;
static size_t img_bytes_received = 0;

static bool hex_byte_started = false;
static uint8_t hex_partial_byte = 0;
static size_t boot_img_match_index = 0;

/******************************************************************************
 * Helpers
 *****************************************************************************/

static uint32_t crc32_bytes(const uint8_t* bytes, size_t length) {
    uint32_t crc = 0xFFFFFFFFu;

    for (size_t i = 0; i < length; i++) {
        crc ^= bytes[i];
        for (int bit = 0; bit < 8; bit++) {
            if ((crc & 1u) != 0u) {
                crc = (crc >> 1) ^ 0xEDB88320u;
            } else {
                crc >>= 1;
            }
        }
    }

    return ~crc;
}

static void send_string_blocking(const char* str) {
    while (*str != '\0') {
        uart_send_byte_blocking((uint8_t)*str);
        str++;
    }
}

static void hang(void) {
    while (true) {
        ;
    }
}

static bool is_whitespace(char c) {
    return (c == ' ') || (c == '\r') || (c == '\n') || (c == '\t');
}

static bool is_hex_digit(char c) {
    return ((c >= '0') && (c <= '9')) || ((c >= 'A') && (c <= 'F')) ||
           ((c >= 'a') && (c <= 'f'));
}

static uint8_t nibble_from_char(char c) {
    if ((c >= '0') && (c <= '9')) {
        return (uint8_t)(c - '0');
    }
    if ((c >= 'A') && (c <= 'F')) {
        return (uint8_t)(c - 'A' + 10);
    }
    return (uint8_t)(c - 'a' + 10);
}

static void reset_hex_parser(void) {
    hex_byte_started = false;
    hex_partial_byte = 0;
}

static void reset_download_state(void) {
    img_bytes_received = 0;
    crc_buffer_index = 0;
    crc_buffer[0] = 0;
    crc_buffer[1] = 0;
    crc_buffer[2] = 0;
    crc_buffer[3] = 0;
    boot_img_match_index = 0;
    reset_hex_parser();
}

/**
 * @brief Decode one ASCII hex byte across two nibble characters.
 *
 * Whitespace is allowed only between complete bytes.
 */
static Result poll_hex_byte(char rx_byte, uint8_t* byte) {
    if (is_whitespace(rx_byte)) {
        if (hex_byte_started) {
            return RESULT_FAILURE;
        }
        return RESULT_PENDING;
    }

    if (!is_hex_digit(rx_byte)) {
        return RESULT_FAILURE;
    }

    const uint8_t nibble = nibble_from_char(rx_byte);
    if (!hex_byte_started) {
        hex_partial_byte = nibble;
        hex_byte_started = true;
        return RESULT_PENDING;
    }

    *byte = (uint8_t)((hex_partial_byte << 4) | nibble);
    reset_hex_parser();
    return RESULT_SUCCESS;
}

/** @brief Match BOOT_IMG; mismatches stay in idle (never an error). */
static Result poll_download_start(char rx_byte) {
    if (rx_byte == kBootImgMsg[boot_img_match_index]) {
        boot_img_match_index++;
        if (boot_img_match_index == kBootImgMsgLength) {
            boot_img_match_index = 0;
            return RESULT_SUCCESS;
        }
        return RESULT_PENDING;
    }

    if (rx_byte == kBootImgMsg[0]) {
        boot_img_match_index = 1;
    } else {
        boot_img_match_index = 0;
    }
    return RESULT_PENDING;
}

static void poll_advertise(void) {
    if (boot_img_match_index != 0) {
        return;
    }

    if (advertise_index == 0) {
        if (mtim_read_nanosec() < next_advertise_time_ns) {
            return;
        }
    }

    if (!uart_putchar_nonblocking(kAdvertiseMsg[advertise_index])) {
        return;
    }

    advertise_index++;
    if (kAdvertiseMsg[advertise_index] == '\0') {
        advertise_index = 0;
        next_advertise_time_ns = mtim_read_nanosec() + kAdvertiseIntervalNs;
    }
}

static Result download_app(int rx_byte) {
    if (rx_byte == '$') {
        if (hex_byte_started || (img_bytes_received == 0)) {
            return RESULT_FAILURE;
        }
        return RESULT_SUCCESS;
    }

    if (img_bytes_received >= kMaxImgBytes) {
        return RESULT_FAILURE;
    }

    uint8_t decoded_byte = 0;
    const Result result = poll_hex_byte((char)rx_byte, &decoded_byte);
    if (result == RESULT_SUCCESS) {
        IMAGE_BUFFER[img_bytes_received] = decoded_byte;
        img_bytes_received++;
        return RESULT_PENDING;
    }
    return result;
}

static Result download_crc(int rx_byte) {
    if (rx_byte == ';') {
        if (hex_byte_started) {
            return RESULT_FAILURE;
        }
        return (crc_buffer_index == 4) ? RESULT_SUCCESS : RESULT_FAILURE;
    }

    if (crc_buffer_index >= 4) {
        return RESULT_FAILURE;
    }

    uint8_t decoded_byte = 0;
    const Result result = poll_hex_byte((char)rx_byte, &decoded_byte);
    if (result == RESULT_SUCCESS) {
        crc_buffer[crc_buffer_index] = decoded_byte;
        crc_buffer_index++;
        return RESULT_PENDING;
    }
    return result;
}

static Result verify_app(void) {
    if ((crc_buffer_index != 4) || (img_bytes_received == 0)) {
        return RESULT_FAILURE;
    }

    const uint32_t computed_crc =
        crc32_bytes((const uint8_t*)IMAGE_BUFFER, img_bytes_received);
    const uint32_t received_crc = ((uint32_t)crc_buffer[0] << 24) |
                                  ((uint32_t)crc_buffer[1] << 16) |
                                  ((uint32_t)crc_buffer[2] << 8) |
                                  (uint32_t)crc_buffer[3];
    return (computed_crc == received_crc) ? RESULT_SUCCESS : RESULT_FAILURE;
}

static void fail_download(const char* msg, State* state) {
    send_string_blocking(msg);
    reset_download_state();
    *state = STATE_IDLE;
}

static void boot_app(void) {
    void (*entry)(void) = (void (*)(void))(uintptr_t)IMAGE_BUFFER;
    __asm__ volatile("" ::: "memory");
    entry();
    hang();
}

/******************************************************************************
 * Bootloader main loop
 *****************************************************************************/

int main(void) {
    State state = STATE_IDLE;
    reset_download_state();

    while (true) {
        const int rx_byte = uart_getchar_nonblocking();

        switch (state) {
            case STATE_IDLE:
                if (rx_byte < 0) {
                    poll_advertise();
                    break;
                }
                if (poll_download_start((char)rx_byte) == RESULT_SUCCESS) {
                    reset_download_state();
                    send_string_blocking("DL_START\n");
                    state = STATE_DOWNLOAD_IMAGE;
                }
                break;

            case STATE_DOWNLOAD_IMAGE:
                if (rx_byte < 0) {
                    break;
                }
                {
                    const Result result = download_app(rx_byte);
                    if (result == RESULT_SUCCESS) {
                        reset_hex_parser();
                        state = STATE_DOWNLOAD_CRC;
                    } else if (result == RESULT_FAILURE) {
                        fail_download("DL_ERROR\n", &state);
                    }
                }
                break;

            case STATE_DOWNLOAD_CRC:
                if (rx_byte < 0) {
                    break;
                }
                {
                    const Result result = download_crc(rx_byte);
                    if (result == RESULT_SUCCESS) {
                        state = STATE_VERIFY;
                    } else if (result == RESULT_FAILURE) {
                        fail_download("DL_ERROR\n", &state);
                    }
                }
                break;

            case STATE_VERIFY:
                if (verify_app() == RESULT_SUCCESS) {
                    send_string_blocking("BOOT_APP\n");
                    state = STATE_BOOT_APP;
                } else {
                    fail_download("CRC_ERROR\n", &state);
                }
                break;

            case STATE_BOOT_APP:
                boot_app();
                break;

            default:
                hang();
                break;
        }
    }
}
