#!/usr/bin/env python3
from __future__ import annotations

import csv
import re
import subprocess
from dataclasses import dataclass
from pathlib import Path
from statistics import mean

import matplotlib

matplotlib.use("Agg")  # Non-interactive backend (SSH/Linux)
import matplotlib.pyplot as plt

ROOT = Path(__file__).parent
BIN = ROOT / "build" / "3_1"
MACHINEFILE = ROOT / "machines"
OUT_DIR = ROOT / "bench_results"
OUT_DIR.mkdir(exist_ok=True)

# Degrees to test
DEGREES = [1_0000, 100_000]

# MPI ranks to test (tweak as needed)
# Tip: multiples of 4 fill your nodes nicely if you have 4 slots/node.
PROCS = [1, 2, 4, 8, 16, 32, 64, 80, 116]

NUM_RUNS = 1  # averaging runs per (n, p)

# Expected lines from 3_1.c
PATTERNS = {
    "serial_time": re.compile(r"^Serial Computation Time: ([0-9.eE+-]+) seconds"),
    "send": re.compile(r"^Max Send Time: ([0-9.eE+-]+) seconds"),
    "parallel": re.compile(r"^Max Parallel Computation Time: ([0-9.eE+-]+) seconds"),
    "gather": re.compile(r"^Max Gather Time: ([0-9.eE+-]+) seconds"),
    "total": re.compile(r"^Max Total Time: ([0-9.eE+-]+) seconds"),
}


@dataclass
class RunResult:
    degree: int
    procs: int
    serial_time: float
    send: float
    parallel: float
    gather: float
    total: float

    @property
    def speedup(self) -> float:
        return self.serial_time / self.total if self.total > 0 else float("nan")

    @property
    def efficiency(self) -> float:
        return self.speedup / self.procs if self.procs > 0 else float("nan")


def build():
    # Prefer make if available in folder 3
    result = subprocess.run(["make"], cwd=ROOT, capture_output=True, text=True)
    if result.returncode != 0:
        raise RuntimeError(f"Build failed\nstdout:\n{result.stdout}\nstderr:\n{result.stderr}")
    if not BIN.exists():
        raise FileNotFoundError(f"Binary not found at {BIN}. Build with: make")


def run_once(degree: int, procs: int) -> RunResult:
    cmd = ["mpiexec"]
    if MACHINEFILE.exists():
        cmd += ["-f", str(MACHINEFILE)]
    cmd += ["-n", str(procs), str(BIN), str(degree)]

    r = subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True)
    if r.returncode != 0:
        raise RuntimeError(
            f"Command failed: {' '.join(cmd)}\nstdout:\n{r.stdout}\nstderr:\n{r.stderr}"
        )

    parsed = {}
    for line in r.stdout.splitlines():
        for key, pat in PATTERNS.items():
            m = pat.match(line.strip())
            if m:
                parsed[key] = float(m.group(1))

    missing = [k for k in PATTERNS.keys() if k not in parsed]
    if missing:
        raise ValueError(
            f"Failed to parse fields: {missing} for degree={degree}, procs={procs}\n"
            f"Output was:\n{r.stdout}"
        )

    return RunResult(
        degree=degree,
        procs=procs,
        serial_time=parsed["serial_time"],
        send=parsed["send"],
        parallel=parsed["parallel"],
        gather=parsed["gather"],
        total=parsed["total"],
    )


def run_case_avg(degree: int, procs: int) -> RunResult:
    runs = [run_once(degree, procs) for _ in range(NUM_RUNS)]
    return RunResult(
        degree=degree,
        procs=procs,
        serial_time=mean(r.serial_time for r in runs),
        send=mean(r.send for r in runs),
        parallel=mean(r.parallel for r in runs),
        gather=mean(r.gather for r in runs),
        total=mean(r.total for r in runs),
    )


def write_csv(path: Path, results: list[RunResult]):
    fieldnames = [
        "degree",
        "procs",
        "serial_time_s",
        "max_send_s",
        "max_parallel_s",
        "max_gather_s",
        "max_total_s",
        "speedup",
        "efficiency",
    ]
    with path.open("w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=fieldnames)
        w.writeheader()
        for r in results:
            w.writerow(
                {
                    "degree": r.degree,
                    "procs": r.procs,
                    "serial_time_s": r.serial_time,
                    "max_send_s": r.send,
                    "max_parallel_s": r.parallel,
                    "max_gather_s": r.gather,
                    "max_total_s": r.total,
                    "speedup": r.speedup,
                    "efficiency": r.efficiency,
                }
            )


def plot_times(results: list[RunResult], degree: int):
    rs = [r for r in results if r.degree == degree]
    rs.sort(key=lambda x: x.procs)

    p = [r.procs for r in rs]
    send = [r.send for r in rs]
    par = [r.parallel for r in rs]
    gath = [r.gather for r in rs]
    tot = [r.total for r in rs]
    serial = [r.serial_time for r in rs]

    plt.figure()
    plt.plot(p, send, marker="o", label="Max send")
    plt.plot(p, par, marker="o", label="Max compute")
    plt.plot(p, gath, marker="o", label="Max gather")
    plt.plot(p, tot, marker="o", label="Max total")
    plt.plot(p, serial, marker="o", label="Serial (verification)")
    plt.xlabel("MPI processes")
    plt.ylabel("Time (s)")
    plt.title(f"MPI polynomial multiplication timing (n={degree})")
    plt.grid(True)
    plt.legend()
    out = OUT_DIR / f"times_n{degree}.png"
    plt.savefig(out, bbox_inches="tight")
    plt.close()


def plot_speedup(results: list[RunResult], degree: int):
    rs = [r for r in results if r.degree == degree]
    rs.sort(key=lambda x: x.procs)

    p = [r.procs for r in rs]
    s = [r.speedup for r in rs]
    e = [r.efficiency for r in rs]

    plt.figure()
    plt.plot(p, s, marker="o", label="Speedup (serial/total)")
    plt.plot(p, p, linestyle="--", label="Ideal")
    plt.xlabel("MPI processes")
    plt.ylabel("Speedup")
    plt.title(f"Speedup (n={degree})")
    plt.grid(True)
    plt.legend()
    out = OUT_DIR / f"speedup_n{degree}.png"
    plt.savefig(out, bbox_inches="tight")
    plt.close()

    plt.figure()
    plt.plot(p, e, marker="o", label="Efficiency")
    plt.xlabel("MPI processes")
    plt.ylabel("Efficiency")
    plt.title(f"Efficiency (n={degree})")
    plt.grid(True)
    plt.legend()
    out = OUT_DIR / f"efficiency_n{degree}.png"
    plt.savefig(out, bbox_inches="tight")
    plt.close()


def main():
    print("Building...")
    build()

    results: list[RunResult] = []
    for n in DEGREES:
        for p in PROCS:
            # Skip impossible cases (more procs than coefficients gives tiny work and can be noisy)
            if p > (2 * n + 1):
                continue
            print(f"Running n={n}, procs={p} ({NUM_RUNS} runs)...")
            results.append(run_case_avg(n, p))

    csv_path = OUT_DIR / "mpi_poly_3_1.csv"
    write_csv(csv_path, results)
    print(f"Wrote {csv_path}")

    for n in DEGREES:
        plot_times(results, n)
        plot_speedup(results, n)

    print(f"Done. Check {OUT_DIR}/ for CSV and PNGs.")


if __name__ == "__main__":
    main()
