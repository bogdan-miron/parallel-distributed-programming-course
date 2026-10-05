# How my lab setup works

## What I use

- **WSL (Ubuntu)** on Windows. It gives me g++, Java, make and bash. I picked it because the teacher's example script for Linux (`run_process.sh`) is a bash script, and the same setup works later for OpenMP, MPI and CUDA.
- **VS Code**, connected to WSL ("WSL: Ubuntu" in the bottom left). I edit, run the terminal and debug from it.
- **g++ and javac** to compile, with `make` calling them. C++ is compiled with `-std=c++17 -O2 -pthread`.
- **Python 3** for the small scripts: generating the input files, making the tables and the chart.
- **Four scripts of mine** in `tools/`: `lab.sh`, `bench.sh`, `report.py`, `build_doc.py`.
- **pandoc** to turn my markdown report into the Word document I upload.

## Folders

```
paralel_distributed_programming/
  teacher_materials/        what the teacher gave (statements, scripts, lectures)
  tools/                    my scripts, same for every lab
  lab_solutions/
    lab1_convolution/
      cpp/  java/           source code and Makefile for each language
      scripts/              gen_data.py, run_all.sh, demo.sh
      results/              csv files with the times, chart, machine.txt
      docs/                 the documentation (.md and .docx)
  .vscode/                  tasks and debug settings
~/ppd_data/<lab>/<case>/    generated input and output files (outside the repo, they are big)
```

## Where the data files are

The input and output files (`A.txt`, `B.txt`, `MatrixF.txt`, `output.txt` and so on) are not in the course folder. `gen_data.py` writes them to `~/ppd_data/<lab>/<case>/` in my WSL home, one folder per test case (for example `~/ppd_data/lab0_vector_add/n1000000/`). They are big, so I keep them out of git and OneDrive, and only the code and the documentation are uploaded.

- Look at them from WSL: `ls -lh ~/ppd_data/<lab>/<case>` and `head` or `code <file>`.
- Look at them from Windows: `explorer.exe ~/ppd_data` in WSL, or `\\wsl$\Ubuntu-24.04\home\<user>\ppd_data` in the Explorer address bar.
- Each case folder also has `expected.txt` or `output_seq.txt` (the reference result), `output.txt` (the last parallel run) and a `.ready` marker so the files are not generated twice.
- To keep them elsewhere, set `PPD_DATA_ROOT` before running, for example `export PPD_DATA_ROOT=/mnt/c/Users/alexb/Desktop/school_work/paralel_distributed_programming/data`. That folder is in `.gitignore`.
- Every lab has its own `scripts/gen_data.py`, written for what that statement needs (vectors, matrices, anything else). `tools/gen_lib.py` has helpers for vectors and matrices. The rest of the pipeline does not care what the data is, it only passes the case folder to the program.
- Any file can be recreated, `gen_data.py` is seeded and gives the same files for the same arguments.

## Showing a lab to the teacher

1. In WSL: `cd /mnt/c/Users/alexb/Desktop/school_work/paralel_distributed_programming`, then `code .`. VS Code opens in the WSL window.
2. Open `lab_solutions/<lab>/cpp/main.cpp` and `java/Main.java` in two tabs, and `docs/<lab>.docx` or the csv files if asked.
3. Open the terminal (Ctrl+`) and run `bash tools/lab.sh demo <lab>`. It builds both programs, runs them on a tiny case and on a bigger one, and prints the check result.
4. The input files are in `~/ppd_data/<lab>/<case>/`. Open them with `code ~/ppd_data/<lab>/demo_small/A.txt` (tiny) or `n100000` (bigger), or with `ls -lh` and `head` in the terminal.
5. To run one program by hand: `cd ~/ppd_data/<lab>/demo_small`, then `/mnt/c/Users/alexb/Desktop/school_work/paralel_distributed_programming/lab_solutions/<lab>/cpp/build/main` for C++ or `java -cp /mnt/c/Users/alexb/Desktop/school_work/paralel_distributed_programming/lab_solutions/<lab>/java/build Main` for Java. It prints the time in ms, and nothing else when the output is correct.

## What the teacher asked and where it is done

| Teacher's rule | Where it is handled |
|---|---|
| Data always read from an input file | `gen_data.py` makes the files, the program reads them from its current folder |
| Output compared with the sequential output on every one of the 10 runs | the program does it after writing its output (function `sameFile`), exit code 1 if different |
| Each variant run at least 10 times, mean time | `bench.sh`, 10 runs by default |
| A sequential version as baseline, speedup Tseq/Tpar | the program with `p = 0`, `report.py` computes the speedup |
| Do not call a function 10 times inside the program | the program runs once and exits, `bench.sh` starts it 10 times as a new process |
| A script that runs the program and saves the times in a csv | `bench.sh`, writes `results/*.csv` |
| Measure only the computation | the timer in the program starts after reading the input and stops before writing the output |
| Tables with time and speedup in the documentation | `report.py` makes them from the csv, `build_doc.py` puts them in the document |
| Documentation with requirements, design, implementation, tests, results | `docs/<lab>.md`, same headings every time |

## The path of one test

```
gen_data.py ──> input files in ~/ppd_data/<lab>/<case>/
                        │
run_all.sh ──> bench.sh ──> runs  main 0            (sequential)   ──> output_seq.txt, time
                        │   runs  main p variant    (parallel)     ──> output.txt, time
                        │                                              │
                        │            main compares output.txt with output_seq.txt
                        │            (different: message and exit code 1)
                        ▼
        reads the last line of each run (the time in ms), repeats 10 times,
        saves mean, min, max in results/cpp.csv
                        │
report.py ──> tables with speedup + chart ──> build_doc.py ──> docs/<lab>.docx
```

## How the pieces talk to each other

The program and the scripts only agree on four things, which is also what the teacher's own scripts expect (`scriptC.ps1`, `scriptJ.ps1`, `run_process.sh`):

1. **Arguments.** The program gets the number of threads first (0 means sequential), then the variant name when the lab has several (horizontal, vertical, and so on).
2. **Files.** The input files have the names from the statement and are read from the current folder. `bench.sh` runs the program inside the folder of the test case.
3. **Standard output.** The last line printed is the execution time in ms and nothing else. This is what the teacher's scripts read.
4. **Exit code.** 0 when the output matches the sequential one, different from 0 when it does not. `bench.sh` stops and writes `ok = 0` in the csv when this happens.

Because of this the same `bench.sh` works for C++, Java and later OpenMP or CUDA programs.

## Using it in VS Code

- Open the folder in WSL: in the Ubuntu terminal go to the course folder and type `code .`
- Open any file of the lab, then Ctrl+Shift+P, "Tasks: Run Task". The tasks work on the lab of the open file:
  - `lab: build` compiles every language folder
  - `lab: demo` short run with the check, for showing the teacher
  - `lab: full benchmark` the whole test matrix, 10 runs for each case
  - `lab: report` tables and chart from the csv files
  - `lab: build documentation` makes the .docx
- The same things from the terminal: `bash tools/lab.sh <build|demo|bench|report|doc> <lab_name>`. `bash tools/lab.sh status` lists the labs.
- Debugging: Run panel, "C++: debug active lab" or "Java: debug active lab". It asks for the thread count, the variant and the folder with the input files.

## The scripts

- `lab.sh`: one entry point, calls the others.
- `bench.sh`: the repeat-and-average script. It is my version of the teacher's `run_process.sh`. The differences are that it keeps decimals in the mean (his uses integer division), saves min and max too, and stops when a run fails the check.
- `report.py`: reads a csv and prints a table per test with the speedup, the best variant, and a chart.
- `build_doc.py`: fills the tables, the chart and the machine description into the markdown report and calls pandoc.

## Things the teacher may ask

- **Why is the time not measured around 10 calls inside the program?** The rules say it is unacceptable. Each run is a new process, started by the script.
- **How do you know the parallel result is right?** Every run writes its output and compares it with the sequential output of the same input. A difference ends the run with an error.
- **Why is reading and writing not in the time?** The statement asks to measure only the computation.
- **Java warm-up?** Each run starts a new JVM, the same as in the teacher's `scriptJ.ps1`, so the 10 runs include it equally.
- **Why WSL and not Windows?** The teacher's Linux script runs as it is. The PowerShell scripts in `teacher_materials` follow the same protocol, so they work with my programs too.
- **Where are the numbers from?** `results/*.csv`, written by `bench.sh`. The machine description is in `results/machine.txt`.
