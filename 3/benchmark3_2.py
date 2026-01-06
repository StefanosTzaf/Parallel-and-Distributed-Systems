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
OUT_DIR = ROOT / "bench_results"
OUT_DIR.mkdir(exist_ok=True)

NONUNIFORM_SPARSITIES = [10, 30, 50, 70, 90]
NONUNIFORM_DIM = 10000
NONUNIFORM_ITERATIONS = 20
NONUNIFORM_PROCESSES = 4
NONUNIFORM_DIST_FLAG = "y"

# Sweeps
PROCESSES = [1, 32, 64, 116]
DIMS_PROCESSES = [1000, 10000]
SPARSITIES = [5, 20, 50, 90]
ITERATIONS_SWEEP = [1, 10, 20]

# Defaults for sweeps
DEFAULT_SPARSITY = 70
DEFAULT_ITERATIONS = 10
DEFAULT_PROCESSES = 32
DEFAULT_DIM = 10000

time_pattern = re.compile(r"(Serial initialization time|Serial multiplication time|Max time to send data|Max parallel multiplication time|Max dense multiplication time): ([0-9.eE+-]+)")

NUM_RUNS = 4  # Number of times to run each benchmark for averaging
WARMUP_RUNS = 1  # Number of warm-up runs before timing


def run_case(dimension: int, sparsity: int, iterations: int, processes: int, uneven: bool = False):
    """Run benchmark NUM_RUNS times (after warm-up) and return average timings."""
    all_times = {
        "Serial initialization time": [],
        "Serial multiplication time": [],
        "Max time to send data": [],
        "Max parallel multiplication time": [],
        "Max dense multiplication time": [],
    }
    
    cmd = ["mpiexec", "-n", str(processes), str(BIN), str(dimension), str(sparsity), str(iterations)]
    if uneven:
        cmd.append(NONUNIFORM_DIST_FLAG)
    
    dist = "non-uniform" if uneven else "uniform"
    print(f"  Running: n={dimension}, sparsity={sparsity}%, iter={iterations}, procs={processes}, dist={dist}")
    
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
    for dim in [1000, 10000]:  # Test both dimensions
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
    for dim in [1000, 10000]:  # Test both dimensions
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


def sweep_nonuniform_sparsity():
    print("\n=== SWEEP: nonuniform_sparsity ===")
    rows = []
    for sp in NONUNIFORM_SPARSITIES:
        times = run_case(NONUNIFORM_DIM, sp, NONUNIFORM_ITERATIONS, NONUNIFORM_PROCESSES, uneven=True)
        rows.append({
            "sweep": "nonuniform_sparsity",
            "dimension": NONUNIFORM_DIM,
            "sparsity": sp,
            "iterations": NONUNIFORM_ITERATIONS,
            "processes": NONUNIFORM_PROCESSES,
            "distribution": "non-uniform",
            **times,
        })
    path = OUT_DIR / f"nonuniform_sparsity.csv"
    write_csv(path, rows)
    if HAS_MATPLOTLIB:
        plot_nonuniform_sparsity(path)



def plot_nonuniform_sparsity(csv_path: Path):
    df = read_csv_simple(csv_path)
    plt.figure()
    plt.plot(df["sparsity"], df["Max parallel multiplication time"], marker="o", label="CSR parallel")
    plt.plot(df["sparsity"], df["Serial multiplication time"], marker="o", label="CSR serial")
    plt.plot(df["sparsity"], df["Max dense multiplication time"], marker="o", label="Dense parallel")
    plt.xlabel("Sparsity (% zeros)")
    plt.ylabel("Time (s)")
    plt.title(
        f"Non-uniform zeros (MPI)\n"
        f"n={NONUNIFORM_DIM}, iterations={NONUNIFORM_ITERATIONS}, processes={NONUNIFORM_PROCESSES}"
    )
    plt.legend()
    plt.grid(True)
    out = OUT_DIR / f"nonuniform_sparsity.png"
    plt.savefig(out, bbox_inches="tight")
    plt.close()
    print(f"Saved {out}")


def sweep_uniform_sparsity_high():
    print("\n=== SWEEP: uniform_sparsity ===")
    rows = []
    for sp in NONUNIFORM_SPARSITIES:
        times = run_case(NONUNIFORM_DIM, sp, NONUNIFORM_ITERATIONS, NONUNIFORM_PROCESSES, uneven=False)
        rows.append({
            "sweep": "uniform_sparsity_high",
            "dimension": NONUNIFORM_DIM,
            "sparsity": sp,
            "iterations": NONUNIFORM_ITERATIONS,
            "processes": NONUNIFORM_PROCESSES,
            "distribution": "uniform",
            **times,
        })
    path = OUT_DIR / f"uniform_sparsity.csv"
    write_csv(path, rows)
    if HAS_MATPLOTLIB:
        plot_uniform_sparsity_high(path)


def plot_uniform_sparsity_high(csv_path: Path):
    df = read_csv_simple(csv_path)
    plt.figure()
    plt.plot(df["sparsity"], df["Max parallel multiplication time"], marker="o", label="CSR parallel")
    plt.plot(df["sparsity"], df["Serial multiplication time"], marker="o", label="CSR serial")
    plt.plot(df["sparsity"], df["Max dense multiplication time"], marker="o", label="Dense parallel")
    plt.xlabel("Sparsity (% zeros)")
    plt.ylabel("Time (s)")
    plt.title(
        f"Uniform zeros (MPI)\n"
        f"n={NONUNIFORM_DIM}, iterations={NONUNIFORM_ITERATIONS}, processes={NONUNIFORM_PROCESSES}"
    )
    plt.legend()
    plt.grid(True)
    out = OUT_DIR / f"uniform_sparsity.png"
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
    sweep_nonuniform_sparsity()
    sweep_uniform_sparsity_high()
    print("Done! Check bench_results/ for CSV and PNG files.")


if __name__ == "__main__":
    main()
