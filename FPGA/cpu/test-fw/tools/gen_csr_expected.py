#!/usr/bin/env python3
"""Compute final x-registers for CSR asm tests (sparse mie/mip model)."""

from pathlib import Path

MISA = 0x40000080
W_MEI = 1 << 11
W_MTI = 1 << 7
W_MSI = 1 << 3
W_ALL = W_MEI | W_MTI | W_MSI


def write_mie(data):
    return (bool(data & W_MEI), bool(data & W_MTI), bool(data & W_MSI))


def read_mie(fields):
    mei, mti, msi = fields
    v = 0
    if mei:
        v |= W_MEI
    if mti:
        v |= W_MTI
    if msi:
        v |= W_MSI
    return v


write_mip = write_mie
read_mip = read_mie


def csrrw_mie(mie, data):
    return read_mie(mie), write_mie(data)


def csrrs_mie(mie, rs1):
    old = read_mie(mie)
    return old, write_mie(rs1 | old)


def csrrc_mie(mie, rs1):
    old = read_mie(mie)
    return old, write_mie(old & ~rs1)


def csrrc_mip(mip, rs1):
    old = read_mip(mip)
    return old, write_mip(old & ~rs1)


def csrrw_mip(mip, data):
    return read_mip(mip), write_mip(data)


def csrrs_mip(mip, rs1):
    old = read_mip(mip)
    return old, write_mip(rs1 | old)


def csrrw_mtvec(old, val):
    return old, val & ~3


def csrrwi_mie(mie, uimm, do_write):
    old = read_mie(mie)
    if do_write and uimm != 0:
        return old, write_mie(uimm)
    return old, mie


def csrrsi_mie(mie, uimm, do_write):
    old = read_mie(mie)
    if do_write and uimm != 0:
        return old, write_mie(uimm | old)
    return old, mie


def csrrci_mie(mie, uimm, do_write):
    old = read_mie(mie)
    if do_write and uimm != 0:
        return old, write_mie(old & ~uimm)
    return old, mie


class Regs:
    def __init__(self):
        self.x = [0] * 32
        self.mie = (False, False, False)
        self.mip = (False, False, False)
        self.mtvec = 0
        self.ram = {}

    def dump_expected(self, path):
        lines = [
            "// ABI: x0..x31, one 32-bit hex word per line",
            "@00000000",
        ] + [f"{self.x[i]:08x}" for i in range(32)]
        Path(path).write_text("\n".join(lines) + "\n")


def sim_csr_basic():
    r = Regs()
    r.x[1] = MISA
    r.x[3], r.mie = csrrw_mie(r.mie, W_MEI)
    r.x[4] = read_mie(r.mie)
    r.x[5], r.mie = csrrs_mie(r.mie, W_MSI)
    r.x[6] = read_mie(r.mie)
    r.x[7], r.mie = csrrc_mie(r.mie, W_MEI)
    r.x[8] = read_mie(r.mie)
    r.x[9] = r.mtvec
    r.x[10], r.mtvec = csrrw_mtvec(r.mtvec, 0x10000)
    r.x[11] = r.mtvec
    r.x[12] = read_mip(r.mip)
    r.x[13], r.mip = csrrw_mip(r.mip, W_ALL)
    r.x[14] = read_mip(r.mip)
    r.x[15] = MISA
    r.x[16], r.mie = csrrw_mie(r.mie, W_MTI)
    r.x[17] = read_mie(r.mie)
    r.x[18], r.mie = csrrw_mie(r.mie, W_MSI)
    r.x[19], r.mie = csrrw_mie(r.mie, W_MSI)
    r.x[20], r.mie = csrrw_mie(r.mie, W_MEI)
    r.x[21], r.mie = csrrs_mie(r.mie, W_MSI)
    r.x[22] = read_mie(r.mie)
    r.x[23], r.mie = csrrw_mie(r.mie, W_MEI)
    r.x[24], r.mie = csrrs_mie(r.mie, W_MTI)
    r.x[25] = read_mie(r.mie)
    r.x[26], r.mie = csrrw_mie(r.mie, W_MSI)
    r.x[27] = read_mip(r.mip)
    r.x[28], r.mie = csrrc_mie(r.mie, W_MSI)
    r.x[29] = read_mie(r.mie)
    r.x[18], r.mtvec = csrrw_mtvec(r.mtvec, 0x20000)
    r.x[19], r.mtvec = csrrw_mtvec(r.mtvec, 0x20000)
    r.x[20] = r.mtvec
    r.x[30], r.mie = csrrw_mie(r.mie, W_MSI)
    r.x[30] = read_mie(r.mie)
    r.x[31] = r.x[30] + 1
    r.x[16], r.mie = csrrw_mie(r.mie, 0)
    r.x[17] = read_mie(r.mie)
    r.x[18], r.mie = csrrw_mie(r.mie, W_MSI)
    r.x[19], r.mie = csrrs_mie(r.mie, 0)
    r.x[20] = read_mie(r.mie)
    r.x[21], r.mie = csrrc_mie(r.mie, 0)
    r.x[22] = read_mie(r.mie)
    r.x[23], r.mie = csrrc_mie(r.mie, 0)
    r.x[24] = read_mie(r.mie)
    r.x[25] = MISA
    r.x[26] = MISA
    r.x[27] = MISA
    r.x[30] = W_MEI
    return r


def sim_csr_immediate():
    r = Regs()
    r.x[1] = MISA
    r.x[3], r.mie = csrrwi_mie(r.mie, 0, True)
    r.x[4], r.mie = csrrwi_mie(r.mie, 8, True)
    r.x[5] = read_mie(r.mie)
    r.x[6], r.mie = csrrw_mie(r.mie, W_MEI)
    r.x[7] = read_mie(r.mie)
    r.x[8], r.mie = csrrci_mie(r.mie, 8, True)
    r.x[9] = read_mie(r.mie)
    r.x[10] = r.mtvec
    r.x[11], r.mtvec = csrrw_mtvec(r.mtvec, 0x10)
    r.x[12] = r.mtvec
    r.x[13] = read_mip(r.mip)
    r.x[14], r.mip = csrrw_mip(r.mip, 8)
    r.x[15] = read_mip(r.mip)
    r.x[16], r.mie = csrrwi_mie(r.mie, 0, True)
    r.x[17] = read_mie(r.mie)
    r.x[18] = MISA
    r.x[19] = MISA
    r.x[20] = MISA
    r.x[21], r.mie = csrrwi_mie(r.mie, 8, True)
    r.x[22] = read_mie(r.mie)
    r.x[23], r.mie = csrrci_mie(r.mie, 8, True)
    r.x[24] = read_mie(r.mie)
    return r


def sim_csr_mixed():
    r = Regs()
    r.x[1] = 0x10000000
    r.x[2], r.mie = csrrw_mie(r.mie, 0)
    r.x[11] = W_MEI
    r.x[12], r.mie = csrrw_mie(r.mie, W_MEI)
    r.x[13] = read_mie(r.mie)
    r.x[14] = W_MSI
    r.x[15], r.mie = csrrs_mie(r.mie, W_MSI)
    r.x[16] = W_MTI
    r.x[17], r.mie = csrrc_mie(r.mie, W_MTI)
    r.x[18] = W_MTI
    r.x[19], r.mie = csrrw_mie(r.mie, W_MTI)
    r.x[20] = W_MEI
    r.x[21], r.mie = csrrs_mie(r.mie, W_MEI)
    r.x[22] = read_mie(r.mie)
    r.x[23] = W_ALL
    r.x[24] = 0xBE
    r.x[25] = read_mie(r.mie)
    r.x[26] = r.x[25]
    r.x[27] = W_MSI
    r.x[28], r.mie = csrrw_mie(r.mie, W_MSI)
    r.x[29], r.mie = csrrs_mie(r.mie, W_MEI)
    r.x[31] = W_MTI
    r.x[30], r.mie = csrrw_mie(r.mie, W_MSI)
    r.x[2], r.mie = csrrw_mie(r.mie, W_MTI)
    r.x[3], r.mie = csrrwi_mie(r.mie, 8, True)
    r.x[4] = read_mie(r.mie)
    r.x[5], r.mie = csrrwi_mie(r.mie, 8, True)
    r.x[6] = read_mie(r.mie)
    r.x[7] = W_MTI
    r.x[8], r.mie = csrrs_mie(r.mie, W_MTI)
    r.x[9], r.mie = csrrsi_mie(r.mie, 8, True)
    r.x[10] = read_mie(r.mie)
    r.x[11] = read_mie(r.mie)
    r.x[12] = W_ALL
    r.x[13] = 0xC2
    r.x[14] = W_MSI
    r.x[15], r.mie = csrrc_mie(r.mie, W_MSI)
    r.x[16] = W_MEI
    r.x[17], r.mie = csrrw_mie(r.mie, W_MEI)
    r.x[18] = W_MTI
    r.x[19], r.mie = csrrw_mie(r.mie, W_MTI)
    r.x[20], r.mie = csrrc_mie(r.mie, r.x[19])
    r.x[21] = read_mie(r.mie)
    return r


if __name__ == "__main__":
    root = Path(__file__).resolve().parent.parent / "expected"
    sim_csr_basic().dump_expected(root / "csr_basic.regs")
    sim_csr_immediate().dump_expected(root / "csr_immediate.regs")
    sim_csr_mixed().dump_expected(root / "csr_mixed.regs")
    print("wrote expected/csr_*.regs")
