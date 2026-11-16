---
title: "Lab 4: final grades with a producer-consumer queue"
---

## Requirements

The final grade of a student is the sum of the grades from 10 project files, divided by 10. The files are `project1.txt` ... `project10.txt`, they have pairs (id, grade), the order of the students is different in every file, and a student can be missing from a file. The result is a file with the final grades of all the students. The program builds a linked list of (id, grade) nodes, ordered by id, with no id twice: a pair whose id is already in the list is added to the grade of that node, otherwise a new node is inserted. I wrote it in C++.

There are two versions. The sequential one reads the files one by one and adds every pair to the list. The parallel one has p threads: p_r readers (the main thread is one of them), which put the pairs in an unbounded queue, and p_w = p - p_r workers, which take the pairs from the queue and add them to the list. The queue has to be written by me, without a built-in synchronized queue, the producer-consumer part has to use semaphores and not condition variables, and the list is thread safe by locking the whole list for every add. Executors are not allowed. The tests are the sequential version and p = 4, 8, 16 with p_r = 2 and p_r = 3, the time includes reading and writing, and the measure is T_sequential / T_parallel.

## Design

A line of a project file is `id grade`, with integer grades from 1 to 10 (10 projects, so the maximum total is 100). The final grade is written as `id grade` in `Results.txt`, ordered by id, where the grade is the total divided by 10 with one decimal. I computed it with integers (`total / 10` and `total % 10`), so there are no real numbers to round and the sums do not depend on the order in which the pairs arrive. This is what makes the output of the parallel version identical to the sequential one on every run.

The list is a singly linked list. `add(id, grade)` locks a mutex for the whole list, goes through the nodes while the id is smaller, adds the grade if it finds the id, and inserts a new node there if it does not. The lock covers the whole list as the statement asks, so two workers never change the list at the same time. This also means that the work inside the list is done by one thread at a time, and only reading the files and taking the pairs out of the queue can really run in parallel.

The queue is also a linked list, with a pointer to the first and to the last node. It has a counting semaphore `items` that holds the number of pairs in the queue, starting from 0, and a mutex for the links. `push` makes the node, links it at the end under the mutex, and then releases the semaphore. `pop` first acquires the semaphore, so a worker waits there as long as the queue is empty, and then takes the first node under the mutex. The mutex is needed because several readers push and several workers pop at the same time. The queue has no maximum size, so a second semaphore for the free places is not needed, and a reader never waits.

An empty queue does not mean that the work is finished, because a reader can still be reading. So the workers do not stop when the queue is empty. The main thread, after reading its own files and waiting (`join`) for the other readers, pushes one pair with id -1 for every worker. A worker that takes a pair with id -1 stops. Since the queue is first in, first out, these pairs come after all the real ones, and since a worker stops after the first one it gets, every worker gets exactly one. The main thread then waits for the workers, so every pair is in the list before the result is written.

The files are split between the readers by number: reader r reads the files r, r + p_r, r + 2 p_r, ... (counting from 0), so for p_r = 2 each reader gets 5 files and for p_r = 3 the readers get 4, 3 and 3 files. The main thread is reader 0, the other p_r - 1 readers and the p_w workers are threads, so in total there are p threads.

## Implementation

The program is `cpp/main.cpp`, compiled with `g++ -std=c++20`, because `std::counting_semaphore` is from C++20. It is started as `main <p> [pr]`, with p = 0 for the sequential version and pr = 2 if it is missing, and it needs 1 <= pr < p. The sequential version writes `output_seq.txt` and uses the same list class, with its lock, but nobody else is using it. The parallel version writes `Results.txt` and compares it with `output_seq.txt` in `sameFile()`, on every run. If the files are different the program exits with code 1.

The functions are `SortedList::add()` and `write()`, `Queue::push()` and `pop()`, `sequential()`, `reader()`, `worker()` and `parallel()`, which creates the threads and joins them. The time is measured from before the threads are created until after `Results.txt` is written, so it includes reading and writing as the statement asks, and the time in ms is the only line printed. `tools/bench.sh` runs the program 10 times, each run is a new process, and saves the mean in a csv.

## Tests

The test from the statement has 100 students and 10 files, and each file has between 70 and 100 grades, in a random order. `scripts/gen_data.py` makes the files once (the same files for all the 10 runs) and also writes `expected.txt`, the final grades computed by Python, and in the demo the sequential result is compared with it. With 100 students the files are very small and the times are fractions of a millisecond, so I added a second test with 2000 students, where each file has between 1400 and 2000 grades. It is not in the statement. Machine: {{machine}}.

I ran the sequential version, and p = 4, 8 and 16 with p_r = 2 and 3. The demo also runs a case with 5 students, small enough to read the files and the result by eye.

## Results and analysis

The Variant column is the number of readers: pr2 has p_r = 2 and pr3 has p_r = 3.

{{table:cpp}}

Every parallel run gave the same file as the sequential one (Check ok in all 14 rows), but the parallel version was slower than the sequential one in every test. With 100 students the speedups are between 0.01 and 0.43, and with 2000 students between 0.31 and 0.68. The best result of both tests was p_r = 3 with 4 threads: 1.05 ms against 0.45 ms sequential for 100 students (0.43), and 95.31 ms against 64.33 ms for 2000 students (0.68).

With 100 students the whole sequential run takes 0.45 ms, so the time of the parallel version is mostly the cost of starting 4 to 16 threads and of the threads waiting for each other, not the work on the grades. With 4 threads the times are stable (1.30 and 1.05 ms). With 8 and 16 threads the means are between 6.07 and 49.32 ms and the runs differ a lot: p_r = 2 with 8 threads went from 1.40 to 32.33 ms, and p_r = 2 with 16 threads from 2.66 to 67.90 ms. In the p_r = 2 tests the mean is decided by a few slow runs, and the fastest runs (1.40 and 2.66 ms) are close to the times with 4 threads. I did not measure where the time goes, so I do not know the reason for the slow runs. My guess is that the threads that wait on the semaphore or on the lock have to be woken up by the system, and that this takes long when there are many of them.

With 2000 students each file has between 1400 and 2000 grades, so between 14000 and 20000 pairs go through the queue, and every add walks the sorted list, which has up to 2000 nodes. The time grows with the number of threads: 154.86, 180.34 and 207.02 ms for p_r = 2, and 95.31, 171.71 and 183.43 ms for p_r = 3. This is what I expected from the lock on the whole list. Only one worker can be inside `add` at a time, so the walk through the list, which is most of the work, is done by one thread at any moment, exactly as in the sequential version. The parallel part is only reading the files, and on top of it come the queue (a mutex and a semaphore operation for every pair) and the workers that wait for the list lock. More workers do not make the adds faster, they only wait for the same lock.

p_r = 3 was faster than p_r = 2 in all 6 pairs of tests with the same number of threads, between 5% and 63% shorter (for 2000 students, 95.31 against 154.86 ms with 4 threads, 171.71 against 180.34 ms with 8 and 183.43 against 207.02 ms with 16). With p_r = 3 there is one worker less, so one thread less to wait for the list lock, and one more thread reading. I think this is the reason, but I did not test other combinations. With 4 threads and p_r = 3 there is a single worker, so the list lock is never contested, and this was the best case.

So with a lock for the whole list this solution does not get a speedup, and the best I could do from the numbers is 0.68. To get a speedup the list would need a finer lock (one for each node, for example), so that several workers can walk it together, or the reading of the files would have to be the bigger part of the work. I did not try either.

## How I ran it

`bash tools/lab.sh demo lab4_grades` for the short run, then `bash tools/lab.sh bench lab4_grades` for the tests above, 10 runs each, and `bash tools/lab.sh report lab4_grades` for the tables and the chart. The numbers are in `results/cpp.csv`.
