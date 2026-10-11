---
title: "Lab 1: applying a convolution matrix"
---

## Requirements

Given a matrix F of n x m numbers and a convolution matrix C of k x k (k odd), compute the matrix V of n x m where every element is the sum of the k x k neighbourhood of that element in F, each neighbour multiplied by the matching element of C. Outside F the matrix is bordered with copies of its boundary elements (`f[-1][j] = f[0][j]`, `f[n][m] = f[n-1][m-1]` and so on, also in the corners).

I wrote a sequential program and a parallel one with p threads, created explicitly (no executors), in C++ (matrices allocated statically and dynamically) and in Java. The work is split between the threads by data: horizontal (rows) and vertical (columns) were required. I also did the two extra variants, block and the delta distribution (linear and cyclic).

## Design

The input is read from `MatrixF.txt` and `ConvolutionM.txt`, the result goes to `output.txt`. Each file starts with the sizes (`n m` and `k k`) and then has one matrix row per line. The values of F are random integers from 0 to 255 and the values of C are random integers from -3 to 3, made once by `scripts/gen_data.py`, so all 10 runs of a test use the same files. I used integers so the result of the parallel version can be compared exactly with the sequential one.

For one element V[i][j] I wrote a function `cell(i, j)` that does the sum for any odd k. Inside the matrix it reads the neighbours directly. Near the border it clamps the row and column index to 0..n-1 and 0..m-1, which is the same as the virtual border of the statement. Every distribution calls this function, so they differ only in which cells a thread gets. For p threads and thread t:

- horizontal: rows from `t*n/p` to `(t+1)*n/p`, all columns
- vertical: columns from `t*m/p` to `(t+1)*m/p`, all rows
- block: the threads form a grid of rows x columns blocks (p = 2 gives 1 x 2, 4 gives 2 x 2, 8 gives 2 x 4, 16 gives 4 x 4, with the larger number on the longer side of the matrix), each thread gets one block
- linear: the n*m cells numbered row by row, thread t gets the numbers from `t*n*m/p` to `(t+1)*n*m/p`
- cyclic: thread t gets the cells t, t+p, t+2p, ... in the same numbering, so delta(i,j) = (i*m + j) mod p

The limits are computed as `t*n/p` and not as `n/p` with a remainder, so the sizes of two threads never differ by more than one row (or column, or cell). When p is bigger than the number of rows, some threads get nothing, which matters for the case n = 10.

## Implementation

The C++ program is `cpp/main.cpp`, compiled twice by the Makefile: with `-DSTATIC_ALLOC` it uses `static int f[MAX][MAX]` and `v[MAX][MAX]` with MAX = 10000, without it the matrices are made with `new int*[n]` and `new int[m]` for every row. The rest of the code is the same. The threads are `std::thread`. The Java program is `java/Main.java`, with `int[][]` matrices and `Thread` objects started and joined by hand.

Both programs are started as `main <p> [variant]`. With p = 0 they run the sequential version and write `output_seq.txt`. With p > 0 they run the chosen variant, write `output.txt` and then compare it with `output_seq.txt` in the function `sameFile()`. If the files are different the program exits with code 1. The comparison is done on every run, and it is a byte comparison of the two files.

The time is measured only around the computation: for the parallel versions it starts before the threads are created and stops after the last `join()`. Reading the input, writing the output and the comparison are outside of it. The time in ms is the last line printed by the program. Because the matrices are big (a 10000 x 10000 file has about 350 MB) I read them with my own buffered number parser instead of `>>`, and I wrote the output the same way.

`tools/bench.sh` runs a program 10 times (every run is a new process, for Java a new JVM), takes the time from the last line and saves the mean, min and max in a csv file. If one run fails (wrong output) the test is marked as failed.

## Tests

I used the test cases from the statement: n = m = 10 with k = 3 and p = 4; n = m = 1000 with k = 3; n = 10, m = 10000 with k = 5; n = 10000, m = 10 with k = 5; n = m = 10000 with k = 5. Except for the first one, I ran p = 2, 4, 8 and 16. Every case was run with the sequential version and with the 5 variants, in C++ with static matrices, in C++ with dynamic matrices and in Java, 10 runs each. Machine: {{machine}}.

I checked the sequential result on the 10 x 10 case against `scripts/reference.py`, a short Python program that does the sum from the definition, and the two were the same. The parallel outputs were compared with the sequential one on every run, as described above.

## Results

C++, static matrices:

{{table:cpp_static}}

C++, dynamic matrices:

{{table:cpp_dynamic}}

Java:

{{table:java}}

Best variant for each matrix size:

{{best:cpp_static:cpp_dynamic:java}}

C++ with static matrices against C++ with dynamic matrices (times in ms, the last column is the geometric mean of dynamic / static over all the runs of that test):

{{compare:cpp_static:cpp_dynamic}}

Java against C++ with dynamic matrices (the last column is Java / C++):

{{compare:cpp_dynamic:java}}

## Analysis

Parallel against sequential. In the 10 x 10 case (p = 4) every parallel run was much slower than the sequential one. In C++ the sequential run took 0.013 ms (static) and 0.004 ms (dynamic) and the parallel ones between 0.24 and 0.39 ms, so creating four threads costs far more than the work itself. In Java the same case went from 0.11 ms to about 5.3 ms. In the other four cases the C++ programs were faster in parallel. For 1000 x 1000 the best speedup was 2.60 with static and 2.39 with dynamic matrices, for the two long thin matrices it was between 1.75 and 2.61, and for 10000 x 10000 it was 5.45 and 5.15, both with 16 threads. This is far from 16. A cell needs only 25 multiplications and the big case reads a 400 MB matrix and writes another one, so I think the memory limits it more than the cores do. The CPU also has 4 fast and 8 slow cores, so 16 threads are not 16 equal workers. I did not measure the memory bandwidth, so this is only my guess. For the matrices that take only a few ms in total, the times change from one configuration to another without a clear pattern, so I did not draw conclusions from small differences there.

The variants against each other. For 10000 x 10000 there was no clear winner. Block was the fastest with 16 threads in both C++ versions, but in the static one it is only 3% better than vertical and cyclic, and the order of the variants changes with 4, 8 and 16 threads. The clear differences are in other places. On the 10000 x 10 matrix (static) vertical and cyclic were the worst (speedup between 0.48 and 1.39, against 1.5 to 2.6 for horizontal, block and linear). With 10 columns a vertical thread gets one or two columns, and cyclic gives neighbouring cells to different threads, so in both cases threads write cells that are next to each other in memory and I think they fight for the same cache lines (false sharing). I did not test this directly. Cyclic was also the slowest variant on 1000 x 1000 for every number of threads (speedup 1.35 to 1.38 in static, 1.39 to 1.87 in dynamic), and it computes the row and the column of each cell with a division. On the matrices with 10 rows, horizontal can give work to only 10 threads, so with p = 16 six threads had nothing to do and it was slower than with p = 8 (1.28 against 1.96 in static, 1.55 against 1.87 in dynamic). Linear, block and vertical do not have this limit. Between the two required variants, horizontal was never far from the best, and vertical was clearly worse only on the 10000 x 10 matrix.

Java against C++. Java was slower in every case. In all cases except the biggest one the parallel Java runs were also slower than the sequential Java run, and the time grew with the number of threads (on 1000 x 1000 it was 30 ms with 2 threads and 57 ms with 16 for horizontal), while the C++ times went down. Every Java run is a new JVM that runs the code interpreted at the start and compiles it in the background. A sequential run of a few ms is mostly spent before the compilation is done, and the threads start in this cold state and compete with the compiler. I did not test this explanation, and repeating the computation inside the program to warm it up is not allowed by the lab. For 10000 x 10000, where the computation takes seconds, the gap was smaller: Java took 2714 ms sequentially against 1783 ms for C++ with dynamic matrices (1.5 times), 550 ms against 346 ms for the best parallel run (1.6 times), and the speedup was 4.94 against 5.15.

Static against dynamic. The dynamic matrices were faster than the static ones in every case, in the sequential and in the best parallel run, and the gap was largest for 10000 x 10: 17.3 ms against 1.6 ms sequentially and 6.6 ms against 0.9 ms for the best parallel run. In the static version every row has MAX = 10000 elements, even when m = 10, so the rows are 40000 bytes apart and the 10 numbers used from each row are in a different memory page. The result matrix is also written for the first time during the timed computation, so every page of it has to be created by the operating system at that moment. My explanation is that these page faults are the main cause of the gap, and the gap is smaller for the other sizes (for 10000 x 10000, static was 20% slower sequentially). I counted the page faults of the two sequential runs on this case with `scripts/page_faults.sh`: the static program had 21029 and the dynamic one 1303, with times of 19.84 ms and 1.57 ms. The extra faults fit the explanation, but I did not check how much of the time they take, so it is still my explanation and not a proof.

Best variant for each matrix size. The table in the results section has the lowest mean time for each size and implementation. The best variant is not always the same, and for the small cases the first few variants are so close that the winner is probably noise. What the results support is: for the large matrix any of the variants with 16 threads is fine, for the narrow matrices avoid vertical and cyclic on the 10000 x 10 case and avoid horizontal with more threads than rows on the 10 x 10000 case, for the 10 x 10 matrix the sequential program is the fastest, and in Java it is the fastest for every size except 10000 x 10000. I ran every configuration 10 times on one laptop under WSL, so I would not trust differences below 10%.

## How I ran it

I ran everything in WSL (Ubuntu) from VS Code. `scripts/run_all.sh` generated the input files once for each test case (in `~/ppd_data/lab1_convolution`), built the three programs, and for every case ran the sequential version first and then every variant with every number of threads through `bench.sh`. The full run is `bash tools/lab.sh bench lab1_convolution`, and the tables above come from `results/cpp_static.csv`, `results/cpp_dynamic.csv` and `results/java.csv`. The page faults were counted with `scripts/page_faults.sh`. The short run I used to look at the numbers on the 10 x 10 case is `bash tools/lab.sh demo lab1_convolution`.
