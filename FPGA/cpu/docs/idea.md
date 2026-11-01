# "Archipelago" (WIP title) federated / isolated MCU architecture

Uses a static, range-based MMU to isolate the data memories of different processes. Accesses to flash and peripherals use physical addresses. Accesses to SRAM (and thus all stack, heap, and .bss accesses) use a virtual address which is translated by applying a fixed offset.

Only the lower 24 bits of virtual addresses are translated. The upper 8 bits select different address spaces, only some of which are virtual. An example address mapping might be:

```
256 address ranges, each is 24 bits = 16 MiB

0x00..'.... Boot ROM (physical, never translated)
0x01..'.... Reserved / invalid
0x02..'.... System peripheral bus (physical, never translated)
...
0x30..'.... Physical SRAM0 [512k main RAM] (only kernel may directly access)
0x31..'.... Physical SRAM1 [64k DTCM RAM] (only kernel may directly access)
0x32..'.... Physical SRAM2 [32k ITCM RAM] (not translated, anyone can read, only kernel can write)
..
0x50..'.... Internal flash (physical, never translated, anyone can read it)
..
0x70..'.... Process-private virtual addresses (always translated / permission checked)
...
0xA0..'.... Inter-process virtual shared memory (always translated / permission checked)
...
0xFF..'.... Physical SRAM2 [External SRAM / DRAM interface] (only kernel may directly access)
```

The ISA has a notion of "process ID". This is an 8-bit identifier which is used to lookup process memory mappings among other things. Implementations are allowed to implement fewer ID bits. The CPU has a PID register which is updated by HW on trap / kernel entry (and maybe ISRs too), and an OS will update the PID on a context switch.

## MMU Private RAM Mapping (0x70..'....)

The MMU has a private internal SRAM which contains the mappings and privileges of each process. Since arbitrary virtual - physical translation is **not** required, the MMU data (page table) is very compact.

One potential implementation of the private mapping is:
* 4x 32-bit pointers (global) identify the base of the physical SRAMs (for up to 4 non-contiguous SRAMs)
* Each process stores the following mapping information:
  - 2-bit index identifying the base of the physical SRAM (which global pointer to use?)
  - 14-bit page count (how many contiguous pages are allocated to this process?)
  - 16-bit page offset (at what page in the SRAM does this processes' private memory start?)
* Process page size is globally selectable from: {256B, 1024B, 4096B}

So for a table of 256 processes, you need 4 * 4B global pointers + 256 * 4B mapping words == 1040B (plus a few bits to select global page size). And a virtual -> physical address translation is done as follows:

```
function translate_private(virtual_addr [31:0], PID [7:0]) -> {status_bit, 32-bit physical address}
    if (virtual_addr[31:24] != 0x70):
        return {STATUS_FAULT, '0};

    mapping [31:0]     = mmu.table[PID];
    gptr_index [1:0]   = mapping[31:30];
    page_count [13:0]  = mapping[29:16];
    page_offset [15:0] = mapping[15:0];

    page_sz_lg2 [3:0]  = mmu.page_size_log2;
    sram_base [31:0]   = mmu.global_ptrs[gptr_index];
    proc_base [31:0]   = sram_base + (page_offset << page_sz_lg2);
    proc_end  [31:0]   = proc_base + (page_count << page_sz_lg2);

    physical_addr = proc_base + virtual_addr[23:0];
    status_bit = (physical_addr >= proc_end) ? STATUS_FAULT : STATUS_OK;
    return {status_bit, physical_addr};
```

## MMU Shared RAM Mapping (0xA0..'....)

The MMU also maps the virtual shared RAM space, using a modified version of the private RAM mapping approach.

One potential implementation of the shared mapping is:
* Re-uses the same 4x 32-bit global physical SRAM base pointers described in the private RAM section.
* MMU supports up to 32 shared RAM regions, each has a 24-bit descriptor (96B total):
  - 2-bit index identifying the base of the physical SRAM (which global pointer to use?)
  - 6-bit page count (how many contiguous pages are allocated to this region?)
  - 2-bit page size {16B, 64B, 256B, 1024B}
  - 14-bit page offset (at what page in the SRAM does this resource's memory start?)
  - note: shared region page sizes are region-specific, unlike process-private pages, which are global.
* MMU has a 2kB SRAM which stores a 64 bit descriptor per process:
  - 32-bit bitmap of which shared RAM regions this process has consumer (read) access to.
  - 32-bit bitmap of which shared RAM regions this process has producer (write) access to.
* Processes access a specific shared region by setting the upper 5 bits of the 24-bit virtual address to shared_region[4:0].
  - The maximum size of a shared region is therefore 512 kB.

Address translation is similar to the private mapping, except that the upper 5 bits index the 96B region translation memory inside the MMU, and task R/W permissions are checked.

For both types of translation, the permission bitmaps or private mapping information of the currently executing process can be cached values, so that reading from the 1/2kB SRAMs embedded inside the MMU only needs to happen on context switches. For most accesses, the MMU only needs to read the cached process descriptor, global register bits, and potentially the 32-entry shared RAM descriptor table. 