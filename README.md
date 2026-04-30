# High-Performance Computing (HPC) & Parallel Systems

![C](https://img.shields.io/badge/C-00599C?style=for-the-badge&logo=c&logoColor=white)
![C++](https://img.shields.io/badge/C++-00599C?style=for-the-badge&logo=c%2B%2B&logoColor=white)
![OpenMP](https://img.shields.io/badge/OpenMP-000000?style=for-the-badge&logo=openmp&logoColor=white)
![MPI](https://img.shields.io/badge/MPI-007D9C?style=for-the-badge&logo=mpi&logoColor=white)
![NVIDIA](https://img.shields.io/badge/CUDA-76B900?style=for-the-badge&logo=nvidia&logoColor=white)

## Acknowledgments & Academic Context

This repository was created as part of the "Parallel Systems" course at the Department of Informatics and Telecommunications, National and Kapodistrian University of Athens (NKUA).

Teamwork: The projects in 01_Pthreads and 02_OpenMP_Sparse_Matrices were developed collaboratively with Εleftheria Galiatsatou - https://github.com/eeeleftheria.

## 📌 Overview
This repository contains a collection of algorithms and systems built to demonstrate **High-Performance Computing (HPC)** concepts. The projects focus on optimizing performance and execution time through parallel processing, distributed memory architectures, and hardware acceleration.

The implementations cover a range of synchronization techniques, lock management (coarse-grained vs. fine-grained), and memory optimization strategies (such as CSR representation for sparse matrices).

## 🛠️ Technologies & APIs
* **C / C++** (Core logic and memory management)
* **POSIX Threads (Pthreads)** (Shared-memory multi-threading, Mutexes, Condition Variables, Barriers)
* **OpenMP** (Compiler directives for shared-memory parallelism)
* **MPI (Message Passing Interface)** (Distributed-memory parallel programming)
* **CUDA & SIMD** (GPU acceleration and vectorization)

---

## 📂 Repository Structure

### [`01_Pthreads_Multithreading`](./01_Pthreads_Multithreading/README.md)
Explores basic multi-threading and synchronization mechanisms.
* Polynomial multiplication (Serial vs. Parallel).
* Race condition handling using Mutexes, Read-Write locks, and Atomic operations.
* Custom implementations of thread Barriers (Condition Variables & Sense-reversal centralized barriers).

### [`02_OpenMP_Sparse_Matrices`](./02_OpenMP_Sparse_Matrices/README.md)
Focuses on shared-memory parallelism and memory optimization.
* Implementation of the **Compressed Sparse Row (CSR)** format.
* Parallel Sparse Matrix-Vector Multiplication.
* Parallel Merge Sort utilizing OpenMP tasks.

### [`03_MPI_Distributed_Processing`](./03_MPI_Distributed_Processing/README.md)
Scales computation across multiple nodes using distributed memory.
* Distributed Polynomial Multiplication.
* Distributed Sparse Matrix-Vector Multiplication (CSR) across MPI processes.

### [`04_CUDA_SIMD_Acceleration`](./04_CUDA_SIMD_Acceleration/README.md)
Hardware-level acceleration techniques.
* Vectorized operations using SIMD instructions.
* GPU acceleration using CUDA for simulation tasks (Maxwell's Equations simulator).

---

## 🚀 How to Run
Each directory contains its own `Makefile`. Navigate to the specific project folder and compile the code using `make`.

## Extra README Files and reports
In every folder there is a readme file that describes the problem and gives details about the compilation and execution of the code. Additionally, there are reports that analyze the results of the benchmarks and provide insights into the performance of the algorithms.
