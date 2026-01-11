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
DEGREES = [10_000, 100_000, 500_000]

# skip serial computation
SKIP_SERIAL = {500_000}

PROCS = [1, 4, 16, 32, 64, 80, 116]

NUM_RUNS = 4

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
    
    # Skip serial for large degrees
    if degree in SKIP_SERIAL:
        cmd.append("No")

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

    if "serial_time" not in parsed:
        parsed["serial_time"] = 0.0
    
    required = set(PATTERNS.keys()) - {"serial_time"}
    missing = [k for k in required if k not in parsed]
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
    # Warm-up run (not counted). Errors should propagate.
    print(f"  Warm-up run...")
    _ = run_once(degree, procs)

    # Measured runs
    runs = []
    for i in range(NUM_RUNS):
        print(f"  Run {i+1}/{NUM_RUNS}...")
        runs.append(run_once(degree, procs))
    
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


def plot_parallel_times(results: list[RunResult], degree: int):
    """Plot send, compute, gather times (parallelism-related) for a given degree."""
    rs = [r for r in results if r.degree == degree]
    rs.sort(key=lambda x: x.procs)

    p = [r.procs for r in rs]
    send = [r.send for r in rs]
    par = [r.parallel for r in rs]
    gath = [r.gather for r in rs]

    plt.figure()
    plt.plot(p, send, marker="o", label="Max send")
    plt.plot(p, par, marker="o", label="Max compute")
    plt.plot(p, gath, marker="o", label="Max gather")
    plt.xlabel("MPI processes")
    plt.ylabel("Time (s)")
    plt.title(f"Parallel operation times (n={degree})")
    plt.grid(True)
    plt.legend()
    out = OUT_DIR / f"parallel_times_n{degree}.png"
    plt.savefig(out, bbox_inches="tight")
    plt.close()


def plot_total_vs_serial(results: list[RunResult], degree: int):
    """Plot total parallel time vs serial time for a given degree."""
    rs = [r for r in results if r.degree == degree]
    rs.sort(key=lambda x: x.procs)

    p = [r.procs for r in rs]
    tot = [r.total for r in rs]
    serial = [r.serial_time for r in rs]

    plt.figure()
    plt.plot(p, tot, marker="o", label="Max total (parallel)")
    if serial[0] > 0:  # Only plot if serial was run
        plt.plot(p, serial, marker="s", label="Serial")
    plt.xlabel("MPI processes")
    plt.ylabel("Time (s)")
    plt.title(f"Total parallel vs serial time (n={degree})")
    plt.grid(True)
    plt.legend()
    out = OUT_DIR / f"total_vs_serial_n{degree}.png"
    plt.savefig(out, bbox_inches="tight")
    plt.close()


def plot_speedup(results: list[RunResult], degree: int):
    rs = [r for r in results if r.degree == degree]
    rs.sort(key=lambda x: x.procs)

    p = [r.procs for r in rs]
    s = [r.speedup for r in rs]

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


def plot_efficiency(results: list[RunResult], degree: int):
    rs = [r for r in results if r.degree == degree]
    rs.sort(key=lambda x: x.procs)

    p = [r.procs for r in rs]
    e = [r.efficiency for r in rs]

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
        # For 500k, use only large process counts
        if n == 500_000:
            procs_list = [32, 64, 80, 116]
        else:
            procs_list = PROCS
        
        for p in procs_list:
            # Skip impossible cases (more procs than coefficients gives tiny work and can be noisy)
            if p > (2 * n + 1):
                continue
            print(f"Running n={n}, procs={p} ({NUM_RUNS} runs)...")
            results.append(run_case_avg(n, p))

    csv_path = OUT_DIR / "mpi_poly_3_1.csv"
    write_csv(csv_path, results)
    print(f"Wrote {csv_path}")

    # Plot 1: Parallel operation times (send, compute, gather) for each degree
    for n in DEGREES:
        plot_parallel_times(results, n)

    # Plot 2: Total vs serial comparison (only for 10^5 and 10^6)
    for n in [100_000, 10_000]:
        plot_total_vs_serial(results, n)

    # Additional plots: speedup & efficiency for 10^4 and 10^5
    for n in [10_000, 100_000]:
        plot_speedup(results, n)
        plot_efficiency(results, n)

    print(f"Done. Check {OUT_DIR}/ for CSV and PNGs.")


if __name__ == "__main__":
    main()