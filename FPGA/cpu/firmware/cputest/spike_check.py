#!/usr/bin/env python3
"""Run cputest ELFs under Spike and compare x-registers to *.expected files."""

from __future__ import annotations

import argparse
import os
import re
import subprocess
import sys
from pathlib import Path

# Spike ABI register names in x0..x31 order (matches Spike "reg 0" output).
XPR_NAMES = (
    "zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
    "s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
    "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
    "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6",
)

DEFAULT_SPIKE = os.environ.get("SPIKE", "")
DEFAULT_GCC_PREFIX = os.environ.get("RISCV_PREFIX", "riscv64-unknown-elf-")

# Spike's debug module occupies the low 4 KiB page; firmware ROM is at 0x1000.
SPIKE_MEMORY = "-m0x1000:0x1000,0x10000:0x1000"
SPIKE_ISA = "RV32I"

# Default skip when tests.mk has no SPIKE_SKIP_TESTS line.
SKIP_DEFAULT = frozenset({"csr_basic", "csr_immediate", "csr_mixed", "jump_lui_auipc", "load_use"})


def parse_tests_mk(tests_mk: Path) -> tuple[list[str], frozenset[str], frozenset[str]]:
    asm_tests: list[str] = []
    skip_regs: set[str] = set()
    spike_skip: set[str] = set()
    for line in tests_mk.read_text().splitlines():
        line = line.strip()
        if line.startswith("ASM_TESTS :="):
            asm_tests = line.split(":=", 1)[1].strip().split()
        elif line.startswith("SKIP_REGS_TESTS :="):
            skip_regs = set(line.split(":=", 1)[1].strip().split())
        elif line.startswith("SPIKE_SKIP_TESTS :="):
            spike_skip = set(line.split(":=", 1)[1].strip().split())
    return asm_tests, frozenset(skip_regs), frozenset(spike_skip)


def discover_asm_tests(cputest_dir: Path) -> list[str]:
    tests = sorted(p.stem for p in cputest_dir.glob("*.s") if p.with_suffix(".expected").exists())
    return tests


def read_expected(path: Path) -> list[int]:
    lines = [ln.strip() for ln in path.read_text().splitlines() if ln.strip()]
    if not lines or not lines[0].startswith("@"):
        raise ValueError(f"{path}: expected @address header")
    values = [int(ln, 16) for ln in lines[1:]]
    if len(values) != 32:
        raise ValueError(f"{path}: want 32 register words, got {len(values)}")
    return values


def find_spike(explicit: str | None) -> str:
    if explicit:
        if not Path(explicit).is_file():
            sys.exit(f"spike not found: {explicit}")
        return explicit
    if DEFAULT_SPIKE and Path(DEFAULT_SPIKE).is_file():
        return DEFAULT_SPIKE
    for candidate in (
        Path.home() / "riscv-spike/bin/spike",
        Path("/usr/local/bin/spike"),
        Path("/usr/bin/spike"),
    ):
        if candidate.is_file():
            return str(candidate)
    found = subprocess.run(["which", "spike"], capture_output=True, text=True)
    if found.returncode == 0 and found.stdout.strip():
        return found.stdout.strip()
    sys.exit("spike not found; set SPIKE=/path/to/spike or pass --spike")


def run(cmd: list[str], *, cwd: Path | None = None) -> subprocess.CompletedProcess[str]:
    return subprocess.run(cmd, cwd=cwd, capture_output=True, text=True, check=False)


def build_spike_elf(
    test: str,
    cputest_dir: Path,
    firmware_dir: Path,
    prefix: str,
    build_dir: Path,
) -> Path:
    arch = ["-march=rv32i", "-mabi=ilp32", "-misa-spec=2.2"]
    asflags = arch + ["-Wa,-march=rv32i"]
    ldflags = arch + ["-T", str(firmware_dir / "linker_spike.ld"), "-nostdlib"]
    out_dir = build_dir / "spike" / test
    out_dir.mkdir(parents=True, exist_ok=True)
    obj = out_dir / f"{test}.o"
    elf = out_dir / f"{test}.elf"

    gcc = f"{prefix}gcc"
    src = cputest_dir / f"{test}.s"
    if not src.is_file():
        raise FileNotFoundError(f"missing {src}")

    steps = [
        [gcc, *asflags, "-c", str(src), "-o", str(obj)],
        [gcc, *ldflags, "-o", str(elf), str(obj), "-lgcc"],
    ]
    for step in steps:
        result = run(step, cwd=cputest_dir)
        if result.returncode != 0:
            raise RuntimeError(
                f"build failed for {test}\n"
                f"cmd: {' '.join(step)}\n"
                f"{result.stderr or result.stdout}"
            )
    return elf


def stop_pc_from_elf(prefix: str, elf: Path) -> int:
    objdump = f"{prefix}objdump"
    result = run([objdump, "-d", str(elf)])
    if result.returncode != 0:
        raise RuntimeError(f"objdump failed for {elf}:\n{result.stderr}")

    last_addr: int | None = None
    for line in result.stdout.splitlines():
        m = re.match(r"^\s*([0-9a-f]+):", line)
        if m:
            last_addr = int(m.group(1), 16)
    if last_addr is None:
        raise RuntimeError(f"no instructions found in {elf}")
    return last_addr + 4


def parse_spike_regs(stderr: str) -> list[int] | None:
    by_name: dict[str, int] = {}
    for match in re.finditer(r"(\w+):\s+0x([0-9a-fA-F]+)", stderr):
        by_name[match.group(1)] = int(match.group(2), 16)
    if len(by_name) < 32:
        return None
    return [by_name[name] for name in XPR_NAMES]


def run_spike(spike: str, elf: Path, stop_pc: int, cmd_file: Path) -> subprocess.CompletedProcess[str]:
    cmd_file.write_text(f"until pc 0 {stop_pc:#x}\nreg 0\nquit\n")
    return run([
        spike,
        "-d",
        f"--isa={SPIKE_ISA}",
        "--priv=m",
        "--disable-dtb",
        f"--debug-cmd={cmd_file}",
        SPIKE_MEMORY,
        str(elf),
    ])


def format_diff(expected: list[int], actual: list[int]) -> str:
    lines = ["  reg      expected    actual"]
    for i in range(32):
        if expected[i] == actual[i]:
            continue
        lines.append(f"  x{i:<2}  0x{expected[i]:08x}  0x{actual[i]:08x}")
    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("tests", nargs="*", help="Test names (default: all *.s with .expected)")
    parser.add_argument("--spike", help="Path to spike binary")
    parser.add_argument("--prefix", default=DEFAULT_GCC_PREFIX, help="riscv toolchain prefix")
    parser.add_argument("--include-skipped", action="store_true",
                        help="Run CSR / PC-layout tests too (usually mismatch RTL .expected)")
    parser.add_argument("--verbose", "-v", action="store_true")
    args = parser.parse_args()

    cputest_dir = Path(__file__).resolve().parent
    firmware_dir = cputest_dir.parent
    build_dir = cputest_dir / "build"
    spike = find_spike(args.spike)

    if args.tests:
        tests = args.tests
    else:
        tests_mk, skip_regs, spike_skip = parse_tests_mk(cputest_dir / "tests.mk")
        tests = tests_mk or discover_asm_tests(cputest_dir)
        skip = spike_skip or SKIP_DEFAULT
        if not args.include_skipped:
            skip |= skip_regs
            tests = [t for t in tests if t not in skip]
        elif skip_regs:
            tests = [t for t in tests if t not in skip_regs]

    passed = 0
    failed = 0
    skipped = 0
    fail_log: list[str] = []
    cmd_file = build_dir / "spike" / "cmds.txt"
    cmd_file.parent.mkdir(parents=True, exist_ok=True)

    for test in tests:
        expected_path = cputest_dir / f"{test}.expected"
        if not expected_path.is_file():
            print(f"  {test:<24} SKIP (no .expected)")
            skipped += 1
            continue

        try:
            expected = read_expected(expected_path)
            elf = build_spike_elf(test, cputest_dir, firmware_dir, args.prefix, build_dir)
            stop_pc = stop_pc_from_elf(args.prefix, elf)
            result = run_spike(spike, elf, stop_pc, cmd_file)
        except (OSError, RuntimeError, ValueError) as exc:
            print(f"  {test:<24} FAIL ({exc})")
            failed += 1
            fail_log.append(f"  {test}: {exc}")
            continue

        if result.returncode != 0:
            print(f"  {test:<24} FAIL (spike exit {result.returncode})")
            failed += 1
            tail = (result.stderr or result.stdout).strip().splitlines()[-3:]
            fail_log.append(f"  {test}: spike exit {result.returncode}: {' | '.join(tail)}")
            if args.verbose:
                print(result.stderr)
            continue

        actual = parse_spike_regs(result.stderr)
        if actual is None:
            print(f"  {test:<24} FAIL (could not parse spike register dump)")
            failed += 1
            if args.verbose:
                print(result.stderr)
            fail_log.append(f"  {test}: register parse failed")
            continue

        mismatches = [i for i in range(32) if expected[i] != actual[i]]
        if mismatches:
            print(f"  {test:<24} FAIL ({len(mismatches)} reg mismatch)")
            failed += 1
            fail_log.append(f"  {test}: {len(mismatches)} register mismatch(es)")
            if args.verbose:
                print(format_diff(expected, actual))
            else:
                i = mismatches[0]
                fail_log[-1] += f" (first: x{i} expected 0x{expected[i]:08x} got 0x{actual[i]:08x})"
        else:
            print(f"  {test:<24} ok")
            passed += 1

    total = passed + failed + skipped
    print()
    if skipped:
        print(f"{passed}/{total - skipped} passed, {failed} failed, {skipped} skipped")
    elif failed:
        print(f"{passed}/{total} passed, {failed} failed:")
        print("\n".join(fail_log))
    else:
        print(f"{passed}/{total} passed")

    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
