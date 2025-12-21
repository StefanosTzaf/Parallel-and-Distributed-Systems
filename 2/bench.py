#!/usr/bin/env python3
import csv
import re
import subprocess
from pathlib import Path

import matplotlib
matplotlib.use('Agg')  # Use non-interactive backend for SSH - Linux Labs
import matplotlib.pyplot as plt

ROOT = Path(__file__).parent
BIN = ROOT / "build" / "2_2"
OUT_DIR = ROOT / "bench_results"
OUT_DIR.mkdir(exist_ok=True)

# Sweeps
THREADS = [1, 2, 4, 8]
DIMS_THREADS = [1000, 5000, 10000]
SPARSITIES = [5, 20, 50, 90]
ITERATIONS_SWEEP = [1, 5, 10, 20]

# Defaults for sweeps
DEFAULT_SPARSITY = 70
DEFAULT_ITERATIONS = 10
DEFAULT_THREADS = 4
DEFAULT_DIM = 10000

time_pattern = re.compile(r"(Initialization|Parallel CSR Multiplication|Serial CSR Multiplication|Parallel Dense Multiplication) Time: ([0-9.eE+-]+)")

NUM_RUNS = 4  # Number of times to run each benchmark for averaging


def run_case(dimension: int, sparsity: int, iterations: int, threads: int):
    """Run benchmark NUM_RUNS times and return average timings."""
    all_times = {
        "Initialization": [],
        "Parallel CSR Multiplication": [],
        "Serial CSR Multiplication": [],
        "Parallel Dense Multiplication": [],
    }
    
    for run in range(NUM_RUNS):
        cmd = [str(BIN), str(dimension), str(sparsity), str(iterations), str(threads)]
        result = subprocess.run(cmd, capture_output=True, text=True, cwd=ROOT)
        if result.returncode != 0:
            raise RuntimeError(f"Command failed: {' '.join(cmd)}\nstdout:\n{result.stdout}\nstderr:\n{result.stderr}")
        
        run_times = {}
        for line in result.stdout.splitlines():
            m = time_pattern.search(line)
            if m:
                label, val = m.group(1), float(m.group(2))
                run_times[label] = val
        
        if len(run_times) != 4:
            raise ValueError(f"Parsed times incomplete for command: {' '.join(cmd)}\nParsed: {run_times}\nOutput:\n{result.stdout}")
        
        for label, val in run_times.items():
            all_times[label].append(val)
    
    # Return average of all runs
    return {label: sum(vals) / len(vals) for label, vals in all_times.items()}


def sweep_threads():
    rows = []
    for dim in DIMS_THREADS:
        for th in THREADS:
            times = run_case(dim, DEFAULT_SPARSITY, DEFAULT_ITERATIONS, th)
            times_with_suffix = {f"{k} Time": v for k, v in times.items()}
            rows.append({
                "sweep": "threads",
                "dimension": dim,
                "sparsity": DEFAULT_SPARSITY,
                "iterations": DEFAULT_ITERATIONS,
                "threads": th,
                **times_with_suffix,
            })
    path = OUT_DIR / "threads.csv"
    write_csv(path, rows)
    plot_threads(path)


def sweep_sparsity():
    rows = []
    for sp in SPARSITIES:
        times = run_case(DEFAULT_DIM, sp, DEFAULT_ITERATIONS, DEFAULT_THREADS)
        times_with_suffix = {f"{k} Time": v for k, v in times.items()}
        rows.append({
            "sweep": "sparsity",
            "dimension": DEFAULT_DIM,
            "sparsity": sp,
            "iterations": DEFAULT_ITERATIONS,
            "threads": DEFAULT_THREADS,
            **times_with_suffix,
        })
    path = OUT_DIR / "sparsity.csv"
    write_csv(path, rows)
    plot_sparsity(path)


def sweep_iterations():
    rows = []
    for it in ITERATIONS_SWEEP:
        times = run_case(DEFAULT_DIM, DEFAULT_SPARSITY, it, DEFAULT_THREADS)
        times_with_suffix = {f"{k} Time": v for k, v in times.items()}
        rows.append({
            "sweep": "iterations",
            "dimension": DEFAULT_DIM,
            "sparsity": DEFAULT_SPARSITY,
            "iterations": it,
            "threads": DEFAULT_THREADS,
            **times_with_suffix,
        })
    path = OUT_DIR / "iterations.csv"
    write_csv(path, rows)
    plot_iterations(path)


def write_csv(path: Path, rows):
    if not rows:
        return
    fieldnames = list(rows[0].keys())
    with path.open("w", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)
    print(f"Wrote {path}")


def plot_threads(csv_path: Path):
    import pandas as pd
    df = pd.read_csv(csv_path)
    for dim, g in df.groupby("dimension"):
        plt.figure()
        plt.plot(g["threads"], g["Parallel CSR Multiplication Time"], marker="o", label="CSR parallel")
        plt.plot(g["threads"], g["Serial CSR Multiplication Time"], marker="o", label="CSR serial")
        plt.plot(g["threads"], g["Parallel Dense Multiplication Time"], marker="o", label="Dense parallel")
        plt.xlabel("Threads")
        plt.ylabel("Time (s)")
        plt.title(f"Threads scaling (n={dim}, sparsity={DEFAULT_SPARSITY}%)")
        plt.legend()
        plt.grid(True)
        out = OUT_DIR / f"threads_n{dim}.png"
        plt.savefig(out, bbox_inches="tight")
        plt.close()
        print(f"Saved {out}")


def plot_sparsity(csv_path: Path):
    import pandas as pd
    df = pd.read_csv(csv_path)
    plt.figure()
    plt.plot(df["sparsity"], df["Parallel CSR Multiplication Time"], marker="o", label="CSR parallel")
    plt.plot(df["sparsity"], df["Serial CSR Multiplication Time"], marker="o", label="CSR serial")
    plt.plot(df["sparsity"], df["Parallel Dense Multiplication Time"], marker="o", label="Dense parallel")
    plt.xlabel("Sparsity (% zeros)")
    plt.ylabel("Time (s)")
    plt.title(f"Sparsity sweep (n={DEFAULT_DIM}, threads={DEFAULT_THREADS})")
    plt.legend()
    plt.grid(True)
    out = OUT_DIR / "sparsity.png"
    plt.savefig(out, bbox_inches="tight")
    plt.close()
    print(f"Saved {out}")


def plot_iterations(csv_path: Path):
    import pandas as pd
    df = pd.read_csv(csv_path)
    plt.figure()
    plt.plot(df["iterations"], df["Parallel CSR Multiplication Time"], marker="o", label="CSR parallel")
    plt.plot(df["iterations"], df["Serial CSR Multiplication Time"], marker="o", label="CSR serial")
    plt.plot(df["iterations"], df["Parallel Dense Multiplication Time"], marker="o", label="Dense parallel")
    plt.xlabel("Iterations")
    plt.ylabel("Time (s)")
    plt.title(f"Iteration sweep (n={DEFAULT_DIM}, sparsity={DEFAULT_SPARSITY}%, threads={DEFAULT_THREADS})")
    plt.legend()
    plt.grid(True)
    out = OUT_DIR / "iterations.png"
    plt.savefig(out, bbox_inches="tight")
    plt.close()
    print(f"Saved {out}")


def main():
    print("Building...")
    result = subprocess.run(["make"], cwd=ROOT, capture_output=True, text=True)
    if result.returncode != 0:
        print(f"Make failed:\n{result.stderr}")
        raise RuntimeError("Build failed")
    if not BIN.exists():
        raise FileNotFoundError(f"Binary not found at {BIN}. Build with: make")
    print("Running benchmarks...")
    sweep_threads()
    sweep_sparsity()
    sweep_iterations()
    print("Done! Check bench_results/ for CSV and PNG files.")


if __name__ == "__main__":
    main()
