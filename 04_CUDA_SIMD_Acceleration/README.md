Running `make` compiles the programs and creates the executables in the `build` directory. *(Note: The CUDA implementation does not compile, since i have only the files that i changed on the project and not the whole environment).*

### **4.1 SIMD Vectorization (CPU Hardware Acceleration)**
*Objective: Accelerate computational tasks by utilizing CPU-level Single Instruction, Multiple Data (SIMD) vector instructions. This exercise demonstrates how hardware-level vectorization can significantly improve execution time and throughput compared to standard scalar operations by processing multiple data points simultaneously.*
* **To run the program:** `./build/4_1 <degree of polynomials>`
* **To run the script:** `python3 benchmark.py`
  * Results are saved into bench_results_final folder.

### **4.2 GPU Acceleration with CUDA (Maxwell's Equations Simulator)**
*Objective: Port a computationally intensive CPU-based 3D simulation of Maxwell's Equations to the GPU using CUDA C++. The implementation focuses on efficient GPU memory management, avoiding race conditions across thousands of threads, utilizing fancy iterators, and applying cooperative algorithms to coarse the grid.*

---

## ⚠️ NVIDIA DLI Acknowledgments & Context
*The code in section **4.2** represents my changes to the initial implementation and parallelization of the "Maxwell's Equations Simulator" of NVIDIA. The original CPU-only base code, physical simulation logic, and boilerplate structure were provided by the **NVIDIA Deep Learning Institute (DLI)** as part of the official course "Getting Started with Accelerated Computing in Modern CUDA C++".* *My contribution focuses purely on the GPU acceleration aspects, which include porting the CPU execution to the GPU, utilizing fancy iterators, and applying cooperative algorithms to coarse the grid for maximum performance.*