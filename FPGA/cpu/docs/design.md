# RV32 CPU Pipeline Hazards (not including forwarding)



## Global

- external_interrupt
- external_exception
- exception_entry

## Fetch

**Inputs**
- external_interrupt
- external_exception
- decode.decode_trap
- decode.exception_return
- decode.illegal_instruction
- exec.branch_redirect_taken 
- mem.stall

**Outputs**
- fetch.current_pc
- fetch.fetch_pc
- fetch.instruction
- fetch.valid

## Decode

**Internal**
- csr_wb_hazard / stall
- load_use_hazard / stall

**Inputs**
- fetch.valid
- exec.branch_redirect_taken
-

**Outputs**
- decode.decode_flush = load_use_hazard || csr_wb_stall // Pipeline bubble
- decode.decode_trap // ECALL
- decode.invalid_opcode // TODO: make it an internal signal
- decode.illegal_instruction
- decode.exception_return
- decode.valid


## Execute

**Inputs**
- decode.*

**Outputs**
- exec.branch_redirect_taken
- exec.branch_pc
- exec.valid

## Memory

**Internal**
- mem_response_wait (1 cycle delay)

**Inputs**
- exec.*
- bus.wr_ack
- bus.rd_valid

**Outputs**
- mem.stall
- mem.valid

// TODO: mepc instruction may be executed twice?
## Machine Special Regs
- mepc
- mstatus


## Writeback

**Inputs**
- mem.valid
- mem.stall



### Scenarios
                        Detected In             Discards                Writes
IF/ID Exception entry   IF/ID                   Fetch, Decode?          mepc = ?, mie, mpie
Exception return        Decode                  Fetch, (Decode = nop)   mie, mpie
Invalid instruction     Decode                  Fetch, Decode           (exception entry)
Branch misprediction    Execute                 Fetch, Decode
Memory stall            Mem                     NA


Mem Exception Entry     Mem                     



### Potential Bugs

* Exception entry simultaneous with branch_redirect_taken
* Exception exit simultaneous with branch_redirect_taken

* Decode trap simultaneous with branch_redirect_taken

spurious exception
wrong mret pc


interrupts should be delayed if CSR write in progress?







# RV32 CPU Forwarding

## Fetch

## Decode

## Execute

## Memory
- from mem

## Writeback