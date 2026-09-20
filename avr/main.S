#include <avr/io.h>

.section .bss

.global uptime_ms
uptime_ms:
    .skip 2


.section .text

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

_trap:
    rjmp    _trap

ms_tick_isr:
    push    r24
    push    r25
    push    r26
    push    r27
    ldi     r26, lo8(uptime_ms)
    ldi     r27, hi8(uptime_ms)
    ld      r24, X+
    ld      r25, X+
    adiw    r24, 1
    st      -X, r25
    st      -X, r24
    pop     r27
    pop     r26
    pop     r25
    pop     r24
    reti

_init:
    ldi     r16, lo8(RAMEND)
    out     AVR_STACK_POINTER_LO_ADDR, r16

.global main
main:
    /* Init LED and turn it on */
    sbi     _SFR_IO_ADDR(DDRD), PD6
    sbi     _SFR_IO_ADDR(PORTD), PD6

    /* Init millisecond timer */
    ldi     r18, _BV(CS01) | _BV(CS00)
    out     _SFR_IO_ADDR(TCCR0B), r18   /* 16 MHz / 64 = 250 kHz tick */
    ldi     r18, 250
    out     _SFR_IO_ADDR(OCR0A), r18    /* 1 kHz TIMER0_COMPA IRQ */
    ldi     r18, _BV(OCIE0A)
    out     _SFR_IO_ADDR(TIMSK), r18    /* OCIE0A IRQ enable */
    ldi     r18, _BV(WGM01)
    out     _SFR_IO_ADDR(TCCR0A), r18   /* CTC mode */

    /* Enable interrupts */
    sei

    ldi     r26, lo8(uptime_ms)
    ldi     r27, hi8(uptime_ms)
loop:
    ld      r15, X
    ldi     r16, 250
    sub     r15, r16
    brne    loop

    in      r15, _SFR_IO_ADDR(PORTD)
    ldi     r16, _BV(PD6)
    eor     r15, r16
    out     _SFR_IO_ADDR(PORTD), r15

loop2:
    ld      r15, X
    ldi     r16, 251
    sub     r15, r16
    brne    loop2

    rjmp    loop
