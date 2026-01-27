#!/usr/bin/env python3
import csv
import re
import subprocess
from dataclasses import dataclass
from pathlib import Path
from statistics import mean
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np

ROOT = Path(__file__).parent
BIN_NO_O2 = ROOT / "build" / "4_1"
BIN_O2    = ROOT / "build" / "4_1_O2"
OUT_DIR   = ROOT / "bench_results_final"
OUT_DIR.mkdir(exist_ok=True)

DEGREES = [1000,10000,20000, 50_000,100_000,200_000]
NUM_RUNS = 4

PATTERNS = {
    "serial": re.compile(r"Serial algorithm time: ([0-9.]+) s"),
    "simd":   re.compile(r"Parallel algorithm time: ([0-9.]+) s"),
}

@dataclass
class ComparisonResult:
    degree: int
    serial_no_O2: float
    simd_no_O2: float
    serial_O2: float
    simd_O2: float

def build():
    print("Running make...")
    result = subprocess.run(["make"], cwd=ROOT)
    if result.returncode != 0:
        raise RuntimeError("Make failed.")
    
def run_bench(bin_path, degree):
    s_times, p_times = [], []
    for _ in range(NUM_RUNS):
        r = subprocess.run([str(bin_path), str(degree)], capture_output=True, text=True)
        s_match = PATTERNS["serial"].search(r.stdout)
        p_match = PATTERNS["simd"].search(r.stdout)
        if s_match: s_times.append(float(s_match.group(1)))
        if p_match: p_times.append(float(p_match.group(1)))
    return mean(s_times), mean(p_times)

def main():
    build()
    results = []

    for n in DEGREES:
        print(f"Testing Degree: {n}...")
        s_no, p_no = run_bench(BIN_NO_O2, n)
        s_o2, p_o2 = run_bench(BIN_O2, n)
        results.append(ComparisonResult(n, s_no, p_no, s_o2, p_o2))

    # --- Plotting ---
    x_labels = [f"{d:,}" for d in DEGREES] # Formatting με κόμμα για χιλιάδες
    indices = np.arange(len(DEGREES)) # Χρήση indices για ίση απόσταση

    # --- Plot 1: Execution Times ---
    plt.figure(figsize=(12, 7))
    
    # Χρήση διαφορετικών χρωμάτων (Set1 palette style)
    plt.plot(indices, [r.serial_no_O2 for r in results], color='#E41A1C', linestyle='--', marker='o', linewidth=2, label="Serial (No O2)")
    plt.plot(indices, [r.simd_no_O2 for r in results],   color='#377EB8', linestyle='-',  marker='s', linewidth=2, label="SIMD (No O2)")
    plt.plot(indices, [r.serial_O2 for r in results],    color='#4DAF4A', linestyle='--', marker='D', linewidth=2, label="Serial (-O2)")
    plt.plot(indices, [r.simd_O2 for r in results],      color='#984EA3', linestyle='-',  marker='^', linewidth=2, label="SIMD (-O2)")
    
    plt.xticks(indices, x_labels) # Εδώ επιβάλλουμε την ίση απόσταση
    plt.yscale('log')
    plt.xlabel("Polynomial Degree (n)")
    plt.ylabel("Execution Time (s) - Log Scale")
    plt.title("Performance Benchmark: Serial vs SIMD (AVX2)")
    plt.legend(loc='best', frameon=True)
    plt.grid(True, which="both", alpha=0.3)
    plt.savefig(OUT_DIR / "times_fixed.png", dpi=300)

    # --- Plot 2: Speedup Comparison ---
    plt.figure(figsize=(10, 6))
    speedup_no = [r.serial_no_O2 / r.simd_no_O2 for r in results]
    speedup_o2 = [r.serial_O2 / r.simd_O2 for r in results]
    
    width = 0.35
    plt.bar(indices - width/2, speedup_no, width, label='Speedup (No O2)', color='#FF7F00', edgecolor='black', alpha=0.8)
    plt.bar(indices + width/2, speedup_o2, width, label='Speedup (-O2)', color='#F781BF', edgecolor='black', alpha=0.8)
    
    plt.xticks(indices, x_labels)
    plt.ylabel("Speedup Factor (Serial Time / SIMD Time)")
    plt.title("SIMD Speedup Comparison")
    plt.axhline(y=1, color='black', linestyle='-', linewidth=0.8, alpha=0.5) # Baseline 1x
    plt.legend()
    plt.grid(axis='y', linestyle='--', alpha=0.6)
    plt.savefig(OUT_DIR / "speedup_fixed.png", dpi=300)

    print(f"\nDone! Plots generated in: {OUT_DIR}")

if __name__ == "__main__":
    main()