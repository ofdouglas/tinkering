#ifndef MEM_MAP_H
#define MEM_MAP_H

#define MEMORY_ADDR_MSB 17
#define PERIPH_ADDR_MSB 7

#define ROM_BASE   0x00010000
#define ROM_SIZE   0x00001000
#define ROM_END    (ROM_BASE + ROM_SIZE)

#define RAM_BASE   0x10000000
#define RAM_SIZE   0x00001000
#define RAM_END    (RAM_BASE + RAM_SIZE)

#define LED_BASE   0x80000000
#define UART_BASE  0x80000100

#endif