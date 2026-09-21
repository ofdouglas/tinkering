#include <avr/io.h>

.section .bss

.global uptime_ms
uptime_ms:
    .skip 4


.section .vectors, "ax", @progbits

.global _vectors
_vectors:
    rjmp    _init  /* Reset */
    rjmp    _trap  /* INT0  */
    rjmp    _trap  /* INT1  */
    rjmp    _trap  /* TIM1_CAPT  */
    rjmp    _trap  /* TIM1_COMPA */
    rjmp    _trap  /* TIM1_OVF  */
    rjmp    _trap  /* TIM0_OVF */
    rjmp    _trap  /* USART0_RXC  */
    rjmp    _trap  /* USART0_DRE */
    rjmp    _trap  /* USART0_TXC  */
    rjmp    _trap  /* ANA_COMP  */
    rjmp    _trap  /* PCINT0  */
    rjmp    _trap  /* TIMER1_COMPB */
    rjmp    ms_tick_isr  /* TIMER0_COMPA  */
    rjmp    _trap  /* TIMER0_COMPB */
    rjmp    _trap  /* USI_START  */
    rjmp    _trap  /* USI_OVERFLOW */
    rjmp    _trap  /* EE_READY  */
    rjmp    _trap  /* WDT_OVERFLOW  */
    rjmp    _trap  /* PCINT1  */
    rjmp    _trap  /* PCINT2  */


.section .text

_trap:
    rjmp    _trap

ms_tick_isr:
    push    r23
    push    r24
    push    r25
    push    r26
    push    r27
    push    r28
    push    r29
    ldi     r28, lo8(uptime_ms)
    ldi     r29, hi8(uptime_ms)
    ld      r24, Y+
    ld      r25, Y+
    ld      r26, Y+
    ld      r27, Y+
    in      r23, _SFR_IO_ADDR(SREG)
    adiw    r24, 1
    adc     r26, r1
    adc     r27, r1
    out     _SFR_IO_ADDR(SREG), r23
    st      -Y, r27
    st      -Y, r26
    st      -Y, r25
    st      -Y, r24
    pop     r29
    pop     r28
    pop     r27
    pop     r26
    pop     r25
    pop     r24
    pop     r23
    reti

_init:
    /*  r1 = constant zero register */
    clr     r1
    ldi     r16, lo8(RAMEND)
    out     AVR_STACK_POINTER_LO_ADDR, r16

    /*  uptime_ms = 0*/
    ldi     r26, lo8(uptime_ms)
    ldi     r27, hi8(uptime_ms)
    st      X+, r1
    st      X+, r1
    st      X+, r1
    st      X+, r1

.global main
main:
    /*  Init LED and turn it on */
    sbi     _SFR_IO_ADDR(DDRD), PD6
    sbi     _SFR_IO_ADDR(PORTD), PD6

    /*  Init millisecond timer */
    ldi     r18, _BV(CS01) | _BV(CS00)
    out     _SFR_IO_ADDR(TCCR0B), r18   /* 16 MHz / 64 = 250 kHz tick */
    ldi     r18, 249
    out     _SFR_IO_ADDR(OCR0A), r18    /* 1 kHz TIMER0_COMPA IRQ */
    ldi     r18, _BV(OCIE0A)
    out     _SFR_IO_ADDR(TIMSK), r18    /* OCIE0A IRQ enable */
    ldi     r18, _BV(WGM01)
    out     _SFR_IO_ADDR(TCCR0A), r18   /* CTC mode */

    /*  Init USART, TX-only in polling mode */
    /*  Baud rate = 76.8k */
    ldi     r16, 12
    out     _SFR_IO_ADDR(UBRRL), r16
    ldi     r16, _BV(TXEN)
    out     _SFR_IO_ADDR(UCSRB), r16

    /*  Enable interrupts */
    sei

    /*  const r3:r2 = 500 ms */
    ldi     r16, lo8(500)
    mov     r2, r16
    ldi     r16, hi8(500)
    mov     r3, r16

    /*  r7:r4 = next_tick_time_ms = 0 */
    clr     r4
    clr     r5
    clr     r6
    clr     r7

loop500ms:
    /*  X = &uptime_ms */
    ldi     r26, lo8(uptime_ms)
    ldi     r27, hi8(uptime_ms)

    /*  r21:r18 = current_time_ms */
    /*  Critical section for ISR-safe read */
    cli
    ld      r18, X+
    ld      r19, X+
    ld      r20, X+
    ld      r21, X+
    sei

    /* while (current_time_ms < next_tick_time_ms) ; wait */
    cp      r18, r4
    cpc     r19, r5
    cpc     r20, r6
    cpc     r21, r7
    brmi    loop500ms

    /* Run 500 ms tick */
    rcall   toggle_led
    ldi     r30, lo8(hello_str)
    ldi     r31, hi8(hello_str)
    rcall   putstr

    /* next_tick_time_ms += 500 ms */
    add     r4, r2
    adc     r5, r3
    adc     r6, r1
    adc     r7, r1

    rjmp    loop500ms

toggle_led:
    in      r18, _SFR_IO_ADDR(PORTD)
    ldi     r19, _BV(PD6)
    eor     r18, r19
    out     _SFR_IO_ADDR(PORTD), r18
    ret

/* arguments: char c = r24 */
putchar:
    sbis    _SFR_IO_ADDR(UCSRA), UDRE
    rjmp    putchar
    out     _SFR_IO_ADDR(UDR), r24
    ret

/* arguments: char* str = Z register */
putstr:
    lpm     r24, Z+
    tst     r24
    breq    null_found
    rcall   putchar
    rjmp    putstr
null_found:
    ret


.section .progmem.data, "a", @progbits

hello_str:
    .asciz "hello, world!\n"