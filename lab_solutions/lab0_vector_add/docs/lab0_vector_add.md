---
title: "Lab 0: vector addition"
---

## Requirements

The lab asks for a C++ program that adds two vectors A and B of n real numbers into a vector C, measures the execution time, and a script that runs the program 10 times and gives the average time. I also wrote the same program in Java.

## Design

I used `vector<double>` for the three vectors in C++ and `double[]` in Java. The program is sequential, so there was no data to split between threads. The input is read from `A.txt` and `B.txt` (first line n, then one number per line) and the result is written to `output.txt` in the same format, with 6 decimals.

## Implementation

The C++ program is `cpp/main.cpp`, compiled with `g++ -std=c++17 -O2`. The Java program is `java/Main.java`, with the same structure and the same function names, compiled with `javac`. In both I timed only the loop `c[i] = a[i] + b[i]`, with `high_resolution_clock` in C++ and `System.nanoTime()` in Java. Reading and writing the files is outside the measurement. The time in ms is printed as the last line, because the script takes it from there.

For the repeated runs I wrote `tools/bench.sh`. It runs a program 10 times (for Java, a new JVM each time), reads the last line of each run, and saves the mean, min and max in a csv file. `scripts/gen_data.py` makes the input files for a given n.

To check the result I made `gen_data.py` also write `expected.txt`, the sum computed in Python. After every run each program compares `output.txt` with `expected.txt` using `sameFile()`. If they differ it exits with code 1 and `bench.sh` marks the test as failed. Both sides add IEEE doubles and print 6 decimals, so the files have to be identical.

## Tests

I tested n = 100000, 1000000 and 10000000, with random values between -1000 and 1000 (4 decimals). The 10 runs of one size use the same input files for C++ and for Java, and the output was checked on every run. Machine: {{machine}}.

## Results

C++:

{{table:cpp}}

Java:

{{table:java}}

The time grows about linearly with n, as expected for a loop that touches every element once. Each step of 10 times more elements made the run about 11 or 12 times longer. The small extra could come from the vectors no longer fitting in the cache, but I did not test that.

## How I ran it

I ran everything in WSL (Ubuntu) from VS Code. For each size, `scripts/run_all.sh` generated the files and called `bench.sh` with the compiled C++ program and then with the Java one, and the full run is `bash tools/lab.sh bench lab0_vector_add`. The tables above come straight from `results/cpp.csv` and `results/java.csv`.
