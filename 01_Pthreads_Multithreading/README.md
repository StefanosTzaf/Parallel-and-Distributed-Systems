### **Makefile**
* To compile all files: `make`
* To run each executable: `make runX`, where X = 1, 2, 3, 4, 5 for exercise 1, exercise 2, etc., respectively.
* To clean up: `make clean` deletes all `.o` files and executables in the `build` directory.

### **Scripts**
* The results of each script are saved in the corresponding text file (e.g., `1_5_benchmark_results.txt` for exercise 5, and so on).

---

### **1.1 Polynomial Multiplication & Data Partitioning**
*Objective: Implement multithreaded polynomial multiplication, partitioning the workload across threads to optimize cache locality and avoid race conditions.*
* **To run the program:** `make run1`
  * It runs with 4 threads and a polynomial degree of 10^6.
* **To run the script:** `./benchmark1_1.sh [degree] [threads] [runs]`
  * The desired number of runs is optional, as it defaults to 5.

### **1.2 Synchronization Mechanisms & Race Conditions**
*Objective: Manage a shared variable across multiple threads using different synchronization techniques to compare their performance overhead: Mutex, Read/Write locks, and Atomic operations.*
* **To run the program:** `make run2`
  * This uses default values of 4 threads and 10^6 iterations. It runs once for each algorithm, where `1` is the mutex implementation, `2` is the read/write lock implementation, and `3` is the atomic operation implementation.
* **To run the script:** `./benchmark1_2.sh [iterations] [runs]`
  * You can pass the desired arguments; however, the script defaults to 10^6 iterations and 5 runs per algorithm. Each algorithm runs 5 times for 1, 2, 4, and 8 threads.

### **1.3 Struct Management & False Sharing**
*Objective: Update a shared struct containing multiple independent variables concurrently. Demonstrates the performance penalty of "False Sharing" (cache line collisions) and how memory padding resolves it.*
* **To run the program:** `make run3`
  * This uses a default array size of 10^7.
* **To run the script:** `./benchmark1_3.sh [size of array] [runs]`
  * You can pass the desired arguments; however, the script defaults to an array size of 10^7 and 5 runs.

### **1.4 Bank Transactions & Lock Granularity**
*Objective: Simulate bank account money transfers and balance checks. Implements both Coarse-grained and Fine-grained locking mechanisms, utilizing Read-Write locks and ensuring Deadlock avoidance (by locking accounts in ascending ID order).*
* **To run the script:** `./benchmark1_4.sh <number_of_accounts> <transactions_per_thread> <percentage_of_read_transactions> <number_of_threads> [iterations]`
  * The script defaults to 5 iterations (runs), so passing this argument is optional.

### **1.5 Custom Thread Barriers & Synchronization**
*Objective: Implement and compare different barrier synchronization mechanisms for thread execution: the default Pthread barrier, a custom Mutex/Condition Variable barrier, and a Sense-reversal centralized barrier (Busy-waiting).*
* **To run the program:** `make run5`
  * This uses a default of 4 threads and 10^6 iterations. It runs all three programs with these arguments.
* **To run the script:** `./benchmark1_5.sh [iterations] [runs]`
  * It defaults to 10^6 iterations and 5 runs per program. It executes for 1, 2, 4, and 8 threads respectively.