#!/usr/bin/env python3
import csv
import os
import re
import subprocess
from pathlib import Path

# Try to import matplotlib, but make it optional
try:
    import matplotlib
    matplotlib.use('Agg')  # Use non-interactive backend for SSH - Linux Labs
    import matplotlib.pyplot as plt
    HAS_MATPLOTLIB = True
except ImportError:
    HAS_MATPLOTLIB = False
    print("Warning: matplotlib not installed. CSVs will be generated but no plots.")

ROOT = Path(__file__).parent
BIN = ROOT / "build" / "3_2"
MACHINEFILE = ROOT / "machines"
OUT_DIR = ROOT / "bench_results"
OUT_DIR.mkdir(exist_ok=True)

# Sweeps
PROCESSES = [1, 4, 8, 16, 32]
DIMS_PROCESSES = [1000, 10000]
SPARSITIES = [5, 20, 50, 90]
ITERATIONS_SWEEP = [1, 5, 10, 20]

# Defaults for sweeps
DEFAULT_SPARSITY = 70
DEFAULT_ITERATIONS = 10
DEFAULT_PROCESSES = 16
DEFAULT_DIM = 10000

time_pattern = re.compile(r"(Serial initialization time|Serial multiplication time|Max time to send data|Max parallel multiplication time|Max dense multiplication time): ([0-9.eE+-]+)")

NUM_RUNS = 4  # Number of times to run each benchmark for averaging
WARMUP_RUNS = 1  # Number of warm-up runs before timing


def run_case(dimension: int, sparsity: int, iterations: int, processes: int):
    """Run benchmark NUM_RUNS times (after warm-up) and return average timings."""
    all_times = {
        "Serial initialization time": [],
        "Serial multiplication time": [],
        "Max time to send data": [],
        "Max parallel multiplication time": [],
        "Max dense multiplication time": [],
    }
    
    cmd = ["mpiexec"]
    if MACHINEFILE.exists():
        cmd += ["-f", str(MACHINEFILE)]
    cmd += ["-n", str(processes), str(BIN), str(dimension), str(sparsity), str(iterations)]

    print(f"  Running: n={dimension}, sparsity={sparsity}%, iter={iterations}, procs={processes}")
    
    # Warm-up runs (not timed)
    for w in range(WARMUP_RUNS):
        print(f"    Warm-up {w+1}/{WARMUP_RUNS}...", end=" ", flush=True)
        subprocess.run(cmd, capture_output=True, text=True, cwd=ROOT)
        print("done")
    
    for run in range(NUM_RUNS):
        print(f"    Run {run+1}/{NUM_RUNS}...", end=" ", flush=True)
        result = subprocess.run(cmd, capture_output=True, text=True, cwd=ROOT)
        print("done")
        if result.returncode != 0:
            raise RuntimeError(f"Command failed: {' '.join(cmd)}\nstdout:\n{result.stdout}\nstderr:\n{result.stderr}")
        
        run_times = {}
        for line in result.stdout.splitlines():
            m = time_pattern.search(line)
            if m:
                label, val = m.group(1), float(m.group(2))
                run_times[label] = val
        
        if len(run_times) != 5:
            raise ValueError(f"Parsed times incomplete for command: {' '.join(cmd)}\nParsed: {run_times}\nOutput:\n{result.stdout}")
        
        for label, val in run_times.items():
            all_times[label].append(val)
    
    # Return average of all runs
    return {label: sum(vals) / len(vals) for label, vals in all_times.items()}


def sweep_processes():
    print("\n=== SWEEP: processes ===")
    rows = []
    for dim in DIMS_PROCESSES:
        for proc in PROCESSES:
            times = run_case(dim, DEFAULT_SPARSITY, DEFAULT_ITERATIONS, proc)
            rows.append({
                "sweep": "processes",
                "dimension": dim,
                "sparsity": DEFAULT_SPARSITY,
                "iterations": DEFAULT_ITERATIONS,
                "processes": proc,
                **times,
            })
    path = OUT_DIR / "processes.csv"
    write_csv(path, rows)
    if HAS_MATPLOTLIB:
        plot_processes(path)


def sweep_sparsity():
    print("\n=== SWEEP: sparsity ===")
    rows = []
    dim = 10000
    for sp in SPARSITIES:
        times = run_case(dim, sp, DEFAULT_ITERATIONS, DEFAULT_PROCESSES)
        rows.append({
            "sweep": "sparsity",
            "dimension": dim,
            "sparsity": sp,
            "iterations": DEFAULT_ITERATIONS,
            "processes": DEFAULT_PROCESSES,
            **times,
        })
    path = OUT_DIR / "sparsity.csv"
    write_csv(path, rows)
    if HAS_MATPLOTLIB:
        plot_sparsity(path)


def sweep_iterations():
    print("\n=== SWEEP: iterations ===")
    rows = []
    dim = 10000
    for it in ITERATIONS_SWEEP:
        times = run_case(dim, DEFAULT_SPARSITY, it, DEFAULT_PROCESSES)
        rows.append({
            "sweep": "iterations",
            "dimension": dim,
            "sparsity": DEFAULT_SPARSITY,
            "iterations": it,
            "processes": DEFAULT_PROCESSES,
            **times,
        })
    path = OUT_DIR / "iterations.csv"
    write_csv(path, rows)
    if HAS_MATPLOTLIB:
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


def read_csv_simple(csv_path: Path):
    """Read CSV without pandas - returns dict of lists by column"""
    with open(csv_path, 'r') as f:
        reader = csv.DictReader(f)
        data = {}
        for row in reader:
            for key, val in row.items():
                if key not in data:
                    data[key] = []
                # Try to convert to float
                try:
                    data[key].append(float(val))
                except ValueError:
                    data[key].append(val)
    return data


def group_by(data, key):
    """Group data by a key column - returns dict of {value: filtered_data}"""
    groups = {}
    unique_vals = sorted(set(data[key]))
    for val in unique_vals:
        groups[val] = {}
        indices = [i for i, v in enumerate(data[key]) if v == val]
        for col, values in data.items():
            groups[val][col] = [values[i] for i in indices]
    return groups


def plot_processes(csv_path: Path):
    df = read_csv_simple(csv_path)
    for dim, g in group_by(df, "dimension").items():
        plt.figure()
        plt.plot(g["processes"], g["Max parallel multiplication time"], marker="o", label="CSR parallel")
        plt.plot(g["processes"], g["Serial multiplication time"], marker="o", label="CSR serial")
        plt.plot(g["processes"], g["Max dense multiplication time"], marker="o", label="Dense parallel")
        plt.xlabel("Processes")
        plt.ylabel("Time (s)")
        plt.title(f"Process scaling (n={int(dim)}, sparsity={DEFAULT_SPARSITY}%)")
        plt.legend()
        plt.grid(True)
        out = OUT_DIR / f"processes_n{int(dim)}.png"
        plt.savefig(out, bbox_inches="tight")
        plt.close()
        print(f"Saved {out}")


def plot_sparsity(csv_path: Path):
    df = read_csv_simple(csv_path)
    for dim, g in group_by(df, "dimension").items():
        plt.figure()
        plt.plot(g["sparsity"], g["Max parallel multiplication time"], marker="o", label="CSR parallel")
        plt.plot(g["sparsity"], g["Serial multiplication time"], marker="o", label="CSR serial")
        plt.plot(g["sparsity"], g["Max dense multiplication time"], marker="o", label="Dense parallel")
        plt.xlabel("Sparsity (% zeros)")
        plt.ylabel("Time (s)")
        plt.title(f"Sparsity sweep (n={int(dim)}, processes={DEFAULT_PROCESSES})")
        plt.legend()
        plt.grid(True)
        out = OUT_DIR / f"sparsity_n{int(dim)}.png"
        plt.savefig(out, bbox_inches="tight")
        plt.close()
        print(f"Saved {out}")


def plot_iterations(csv_path: Path):
    df = read_csv_simple(csv_path)
    for dim, g in group_by(df, "dimension").items():
        plt.figure()
        plt.plot(g["iterations"], g["Max parallel multiplication time"], marker="o", label="CSR parallel")
        plt.plot(g["iterations"], g["Serial multiplication time"], marker="o", label="CSR serial")
        plt.plot(g["iterations"], g["Max dense multiplication time"], marker="o", label="Dense parallel")
        plt.xlabel("Iterations")
        plt.ylabel("Time (s)")
        plt.title(f"Iteration sweep (n={int(dim)}, sparsity={DEFAULT_SPARSITY}%, processes={DEFAULT_PROCESSES})")
        plt.legend()
        plt.grid(True)
        out = OUT_DIR / f"iterations_n{int(dim)}.png"
        plt.savefig(out, bbox_inches="tight")
        plt.close()
        print(f"Saved {out}")


def main():
    print("Building...")
    result = subprocess.run(["make"], cwd=ROOT, capture_output=True, text=True)
    if result.returncode != 0:
        print(f"Build failed:\n{result.stderr}")
        raise RuntimeError("Build failed")
    if not BIN.exists():
        raise FileNotFoundError(f"Binary not found at {BIN}. Build with: mpicc -o 3_2 3_2.c")
    print("Running benchmarks...")
    sweep_processes()
    sweep_sparsity()
    sweep_iterations()
    # Custom plot for 10 iterations, 10000 size
    if HAS_MATPLOTLIB:
        plot_parallel_times_comparison()
    print("Done! Check bench_results/ for CSV and PNG files.")
def plot_parallel_times_comparison():
    """Plot max send time, max parallel multiplication time, and total parallel time for 10 iterations and 10000 size."""
    import matplotlib.pyplot as plt
    csv_path = OUT_DIR / "processes.csv"
    if not csv_path.exists():
        print(f"{csv_path} not found. Run sweep_processes() first.")
        return
    df = read_csv_simple(csv_path)
    # Filter for dimension=10000 and iterations=10
    indices = [i for i, (dim, it) in enumerate(zip(df["dimension"], df["iterations"])) if dim == 10000 and it == 10]
    if not indices:
        print("No data for dimension=10000 and iterations=10 in processes.csv")
        return
    procs = [df["processes"][i] for i in indices]
    max_send = [df["Max time to send data"][i] for i in indices]
    max_parallel = [df["Max parallel multiplication time"][i] for i in indices]
    total_parallel = [max_send[j] + max_parallel[j] + df["Serial initialization time"][indices[j]] for j in range(len(indices))]
    plt.figure()
    plt.plot(procs, max_send, marker="o", label="Max send time")
    plt.plot(procs, max_parallel, marker="o", label="Max parallel multiplication time")
    plt.plot(procs, total_parallel, marker="o", label="Total parallel time")
    plt.xlabel("Processes")
    plt.ylabel("Time (s)")
    plt.title("Parallel Time Breakdown (n=10000, iter=10)")
    plt.legend()
    plt.grid(True)
    out = OUT_DIR / "parallel_time_breakdown_n10000_iter10.png"
    plt.savefig(out, bbox_inches="tight")
    plt.close()
    print(f"Saved {out}")


if __name__ == "__main__":
    main()
