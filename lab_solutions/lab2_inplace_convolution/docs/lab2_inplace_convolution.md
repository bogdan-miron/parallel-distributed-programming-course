---
title: "Lab 2: convolution in place"
---

## Requirements

The problem is the one from Lab 1 (a convolution of a matrix with a kernel), with one more condition: the filtered image has to end up in the initial matrix. No result matrix and no temporary matrix may be allocated, only temporary vectors with the space complexity O(n). This applies to the sequential version too. I wrote it in C++ (dynamic allocation) and in Java. Compared to Lab 1 only the row-based distribution and the kernel of size 3 x 3 were required.

## Design

The input is `date.txt`: on the first line the size `n m`, then the n rows of the matrix, then `3 3` and the 3 rows of the kernel. The result is written to `output.txt`.

The problem with working in place is that a new value needs the old values of the three rows above, on and below it. If I write the new row i over the old row i, the next row loses the old row i it still needs. So I go through the rows from top to bottom and keep two vectors of m elements: `prev`, a copy of the old row i-1, and `cur`, a copy of the old row i, made before the row is overwritten. The old row i+1 is still in the matrix, because it was not reached yet. With `prev`, `cur` and the row i+1 I can compute the new row i and write it straight into the matrix. Then `prev` and `cur` are swapped (the old row i becomes the previous row, nothing is copied). The border is handled like in Lab 1: outside the matrix the border elements are repeated, so for the first row the previous row is the first row itself, for the last row the next row is the last row itself, and for the columns I use `left = j-1` or 0 and `right = j+1` or m-1.

The sequential version is this same procedure over all the rows. The parallel version gives each thread a block of consecutive rows. The problem is only at the edges of a block: the first row of a block needs the last row of the block above, and the last row needs the first row of the block below, and those rows are changed by the other threads. I chose the variant 1b from the utils file:

1. each thread copies into two auxiliary vectors the old rows next to its block (the row before the first one and the row after the last one);
2. synchronization barrier, so that all the copies are done before any thread starts to write;
3. each thread processes its own rows in place, with the procedure above, starting with the copied row as `prev` and ending with the copied row as the next row.

The rows are split so that every thread gets n / p rows, and the first n % p threads get one more row. For 10 rows and 4 threads this gives 3, 3, 2 and 2 rows, so all the rows are covered and two threads never differ by more than one row. If there are more threads than rows, the extra threads get no rows, only copy the edges and wait at the barrier.

I did not implement the other variants (1f with flags, 2b and 2f). The variants with flags would remove the barrier and let a thread continue as soon as its neighbours have copied their edges, which could be a little faster, but the code is longer and harder to explain.

## Implementation

The C++ program is `cpp/main.cpp`, compiled with `g++ -std=c++20`, because the barrier is `std::barrier` from C++20. The matrix is `int**`, with every row allocated with `new`. In Java (`java/Main.java`) the matrix is an `int[][]` and the barrier is a `CyclicBarrier`. The threads are created explicitly in both. The two programs have the same structure and the same function names: `process()` for the in-place procedure, `sequential()`, `worker()` for the code of a thread, `copyRow()` for the auxiliary vectors.

Both are started as `main <p>`, with p = 0 for the sequential version. The sequential version writes `output_seq.txt`, and the parallel ones write `output.txt` and compare it with `output_seq.txt` in `sameFile()`, on every run. If the files are different the program exits with code 1. The time is measured around the computation only, from before the threads are created until after the last `join()`, so it includes copying the edges and waiting at the barrier but not reading or writing the files. The time in ms is the last line printed. Like in Lab 1, `tools/bench.sh` runs a program 10 times, every run is a new process, and saves the mean in a csv.

## Tests

I used the cases from the statement: 10 x 10 with p = 2, 1000 x 1000 and 10000 x 10000 with p = 2, 4, 8 and 16, and the sequential version for each. The matrix has random integers from 0 to 255 and the kernel integers from -3 to 3, made once by `scripts/gen_data.py`, so all 10 runs of a test use the same `date.txt`, in C++ and in Java. Machine: {{machine}}.

I checked the sequential result of the 10 x 10 case against `scripts/reference.py`, a short Python program that makes the sum from the definition, and they were the same. To see the uneven split working I also ran the 10 x 10 matrix with p = 3, 4 and 7, which do not divide 10, and the check against the sequential output passed.

## Results

C++:

{{table:cpp}}

Java:

{{table:java}}

Java against C++ (times in ms, the last column is Java / C++ over all the runs of that test):

{{compare:cpp:java}}

## Space complexity

The matrix itself takes n x m elements and it is the only matrix, so the result needs no extra matrix. Besides it, every thread has three vectors of m elements (the two copied edge rows and `cur`), so the auxiliary space is 3 x m per thread, 3 x p x m in total, which is O(m) for a fixed number of threads. The sequential version has 3 vectors of m elements too. In Lab 1 the program kept two matrices (F and V), so it needed 2 x n x m elements. Here it needs n x m + 3 x p x m, which is about half for a big matrix.

I measured the peak memory of the sequential run on the 10000 x 10000 matrix with `scripts/peak_memory.sh`, for this lab and for the Lab 1 programs (C++ with `new`, and Java):

| Program | Lab 1 (two matrices) | Lab 2 (in place) |
|---|---|---|
| C++ | 770 MB | 388 MB |
| Java | 1039 MB | 520 MB |

A 10000 x 10000 matrix of ints has 400,000,000 bytes, which is about 381 MB in the units of the script, so the 388 MB of the C++ program is one matrix and a few MB. The memory was cut to half in both languages, as expected. Java uses more than C++ in both labs because of the JVM itself.

## Analysis

Case by case. For 10 x 10 the parallel version was much slower in both languages: in C++ the sequential run took 0.0014 ms and the run with 2 threads 0.35 ms, in Java 0.019 ms and 5.7 ms. The matrix has only 100 elements, so creating the threads costs much more than the work.

For 1000 x 1000 the C++ version was faster in parallel up to 8 threads, with the best time with 4 threads (1.65 ms against 3.22 ms, speedup 1.96). With 16 threads the speedup went down to 1.17, because the whole sequential run is only about 3 ms and the cost of starting 16 threads and waiting at the barrier gets close to the work each thread has. The Java version was slower in parallel than sequentially for every number of threads (14.7 ms sequentially, 22 ms with 2 threads and about 55 ms with 8 and 16 threads). My explanation is the same as in Lab 1: every Java run starts a new JVM that runs the code interpreted at first and compiles it later, and for a run of 15 ms this warm-up is most of the time. I did not test it.

For 10000 x 10000 both versions were faster in parallel. In C++ the time went from 310 ms to 147 ms with 2 threads (speedup 2.10) and to about 94 ms with 4 threads (3.30), and then it stayed almost the same: 3.21 with 8 threads and 3.46 with 16, the best one. In Java the best was 4 threads, with 163 ms against 421 ms sequentially (speedup 2.58), and with 8 and 16 threads it went back to 2.46 and 2.13. The speedup does not grow after 4 threads, so I think the memory limits the program: every element needs only 9 multiplications, and the matrix has 400 MB that are read and written in about 90 ms, which is roughly 9 GB/s by my estimate. I did not measure the memory bandwidth, so this is a guess. The laptop has 16 threads but they are not 16 equal cores, which could also play a role.

The speedups are lower than in Lab 1, where the best speedup for 10000 x 10000 was about 5 with 16 threads. The two labs are not directly comparable, because Lab 1 used a 5 x 5 kernel (25 multiplications per element) and this one a 3 x 3 kernel (9), so here there is less work per element for the same amount of memory. In this lab every thread also copies two rows and waits at the barrier, but these are only two rows of 10000 elements for each thread, and I did not measure their cost separately.

Java against C++. C++ was faster in every case. Sequentially Java was 1.4 times slower on 10000 x 10000 (421 ms against 310 ms) and 4.5 times slower on 1000 x 1000. For the best parallel runs on 10000 x 10000 it was 1.8 times slower (163 ms against 90 ms). The gap is the biggest for the small matrices, where the JVM warm-up is most of the time, and it gets smaller when the computation is longer, as in Lab 1.

Space. The results confirm the analysis above: the in place version needs one matrix plus a few vectors, and its peak memory was half of the one from Lab 1 in both languages. The price is that a row can be computed only after the previous one is saved, and the threads have to wait for each other at the barrier before they start to write. Even so, the program kept a speedup of 3.46 (C++) and 2.58 (Java) on the biggest matrix.

## How I ran it

I ran everything in WSL (Ubuntu) from VS Code. `scripts/run_all.sh` generated `date.txt` once for each test case (in `~/ppd_data/lab2_inplace_convolution`), built the programs, and for every case ran the sequential version first and then every number of threads through `bench.sh`. The full run is `bash tools/lab.sh bench lab2_inplace_convolution`, and the tables above come from `results/cpp.csv` and `results/java.csv`. The short run I used to look at the numbers on the 10 x 10 case is `bash tools/lab.sh demo lab2_inplace_convolution`, and the peak memory was measured with `scripts/peak_memory.sh`.
