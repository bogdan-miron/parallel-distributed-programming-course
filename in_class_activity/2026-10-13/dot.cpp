#include <chrono>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <thread>

using namespace std;
using namespace std::chrono;

const int N = 100000000;
const int P = 4;

mutex res_mutex;

double dot(double* v1, double* v2) {
    double sum = 0;
    for (int i = 0; i < N; i++) {
        sum += v1[i] * v2[i];
    }
    return sum;
}

void init(double* v) {
    for (int i = 0; i < N; i++) {
        v[i] = rand() % 1000;
    }
}

void worker(double* a, double* b, int start_idx, int end_idx, double* res) {
    double sum = 0;
    for (int i = start_idx; i < end_idx; i++) {
        sum += a[i] * b[i];
    }

    lock_guard<mutex> lock(res_mutex);
    *res += sum;
}

int main() {
    // double* a = (double*) aligned_alloc(64, sizeof(double) * N);
    // double* b = (double*) aligned_alloc(64, sizeof(double) * N);
    double* a = (double*) malloc(sizeof(double) * N);
    double* b = (double*) malloc(sizeof(double) * N);

    init(a);
    init(b);

    auto start = high_resolution_clock::now();
    // double res = dot(a, b);

    thread threads[P];
    int block_size = N / P;
    int r = N % P;

    int start_idx = 0;
    int end_idx = block_size;

    double res = 0;

    for (int tid = 0; tid < P; tid++) {
        if (tid < r) {
            end_idx++;
        }

        threads[tid] = thread(worker, a, b, start_idx, end_idx, &res);

        start_idx = end_idx;
        end_idx = start_idx + block_size;
    }

    for (int tid = 0; tid < P; tid++) {
        threads[tid].join();
    }

    auto end = high_resolution_clock::now();

    double elapsed = duration_cast<milliseconds>(end - start).count();

    cout << "Result: " << res << endl << "Time: " << elapsed << " ms" << endl;
    cout << (res == dot(a, b) ? "Check: OK" : "Check: different from the sequential result") << endl;

    free(a);
    free(b);

    return 0;
}
