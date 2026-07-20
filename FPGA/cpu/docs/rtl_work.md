# RV32I CPU Working Document

## TODOs
* Refine trap handling: invalid fetch, bus errors, unaligned errors, invalid CSR access
* General RTL cleanup
* Periphal addressing cleanup
* Better instruction fetch
* Better load / store unit
* Priviledge levels
* Flush fewer instructions for CSR hazard

### UART
There is also a receiver timing defect in uart.sv: it shifts the stop bit when rx_bit_count == 8, then waits another full bit period before completing at == 9. That should be completed at the stop-bit sample rather than delaying until the following bit.

mepc saved from decode_regs.current_pc (one stage behind)
Handler uses mepc + 8 to resume after ECALL

CSR writes retire in WB
3 nops between csrrw mepc and mret

Handler must not sit at address 0
_start first; handler linked after a halt loop