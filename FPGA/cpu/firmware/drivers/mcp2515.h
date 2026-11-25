#pragma once
/*
 * @file mcp2515.h
 * @brief Basic polled Classical CAN driver for an MCP2515.
 */

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint8_t data[8];
    uint32_t can_id;
    uint8_t dlc;
    bool is_extended_id;
} can_frame_t;

/**
 * @brief Configure an MCP2515 with an 8 MHz crystal for 500 kbit/s.
 *
 * The controller accepts standard and extended data frames into either receive
 * buffer. Interrupts, remote frames, and acceptance filtering are not used.
 *
 * @return true when the controller enters Normal mode.
 */
bool mcp2515_init_500kbps_8mhz(void);

/**
 * @brief Queue a standard or extended data frame in transmit buffer 0.
 *
 * @return false for an invalid frame or while transmit buffer 0 is busy.
 */
bool mcp2515_send(const can_frame_t* frame);

/**
 * @brief Poll both receive buffers for one standard or extended data frame.
 *
 * Remote frames are discarded because can_frame_t represents data frames.
 *
 * @return true when frame was populated.
 */
bool mcp2515_receive(can_frame_t* frame);
