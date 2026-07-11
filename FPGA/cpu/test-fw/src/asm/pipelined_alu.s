.section .text
.global _start
_start:
    addi  x1,  x0, 5         /* verify x1  = 0x00000005 */
    addi  x2,  x1, 7         /* verify x2  = 0x0000000c, N+1 forwarding */
    add   x3,  x2, x1        /* verify x3  = 0x00000011, N+1 forwarding */
    sub   x4,  x3, x2        /* verify x4  = 0x00000005, N+1 forwarding */
    sll   x5,  x4, x1        /* verify x5  = 0x000000a0, N+1 forwarding */
    srl   x6,  x5, x1        /* verify x6  = 0x00000005, N+1 forwarding */
    sub   x7,  x2, x3        /* verify x7  = 0xfffffffb */
    addi  x8,  x1, 1         /* verify x8  = 0x00000006, independent gap */
    sra   x9,  x7, x1        /* verify x9  = 0xffffffff, N+2 forwarding */
    xor   x10, x9, x7        /* verify x10 = 0x00000004, N+1 forwarding */
    or    x11, x10, x2       /* verify x11 = 0x0000000c */
    addi  x12, x0, 3         /* verify x12 = 0x00000003, independent gap */
    addi  x13, x0, 4         /* verify x13 = 0x00000004, independent gap */
    and   x14, x11, x3       /* verify x14 = 0x00000000, N+3 forwarding */
    slt   x15, x7, x1        /* verify x15 = 0x00000001 */
    sltu  x16, x1, x7        /* verify x16 = 0x00000001 */
    addi  x17, x16, 31       /* verify x17 = 0x00000020, N+1 forwarding */
    slli  x18, x15, 4        /* verify x18 = 0x00000010, N+3 forwarding */
    srli  x19, x7, 28        /* verify x19 = 0x0000000f */
    srai  x20, x7, 2         /* verify x20 = 0xfffffffe */
    add   x21, x20, x18      /* verify x21 = 0x0000000e, N+1 forwarding */
    xor   x22, x21, x19      /* verify x22 = 0x00000001, N+1 forwarding */
    or    x23, x22, x14      /* verify x23 = 0x00000001, N+1 forwarding */
    and   x24, x23, x15      /* verify x24 = 0x00000001, N+1 forwarding */
    slti  x25, x20, -1       /* verify x25 = 0x00000001 */
    slti  x26, x1, 5         /* verify x26 = 0x00000000 */
    sltiu x27, x7, -1        /* verify x27 = 0x00000001 */
    sltiu x28, x7, 1         /* verify x28 = 0x00000000 */
    add   x29, x27, x25      /* verify x29 = 0x00000002, N+2 forwarding */
    sub   x30, x29, x24      /* verify x30 = 0x00000001, N+1 forwarding */
    xor   x31, x30, x7       /* verify x31 = 0xfffffffa, N+1 forwarding */

    /* Wait for all test results to reach registers */
    nop
    nop
    nop
    nop
