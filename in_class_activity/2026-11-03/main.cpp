#include <iostream>
#include <cmath>
#include <cstdlib>
#include <mpi.h>

using namespace std;

#define N 1024

void init_rand(double *vec, int len) {
    for (int i = 0; i < len; i++) {
        vec[i] = (double)(rand() % 1000) / 100  - 5;
    }
}

double dot(double *a, double *b, int len) {
    double res = 0.0;
    for (int i = 0; i < len; i++) {
        res += a[i] * b[i];
    }
    return res;
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int size = 0; // P - numarul de procese
    int rank = 0; // pid - id-ul procesului curent

    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    cerr << "Hello from process " << rank << " out of " << size << endl;

    if (N % size != 0) {
        if (rank == 0) {
            cerr << "N = " << N << " is not divisible by " << size << " processes" << endl;
        }
        MPI_Finalize();
        return 1;
    }

    MPI_Barrier(MPI_COMM_WORLD);

    double *a = nullptr;
    double *b = nullptr;
    double secv_res = 0.0;
    double par_res = 0.0;
    double secv_time = 0.0;

    if (rank == 0) {
        a = new double[N];
        b = new double[N];
        init_rand(a, N);
        init_rand(b, N);

        double s1 = MPI_Wtime();
        secv_res = dot(a, b, N);
        secv_time = MPI_Wtime() - s1;
    }

    // toate procesele executa asta
    int local_N = N / size; // N se imparte exact la size, verificat mai sus
    double *local_a = new double[local_N];
    double *local_b = new double[local_N];
    double local_res = 0.0;

    double t1 = MPI_Wtime();

    MPI_Scatter(a, local_N, MPI_DOUBLE, local_a, local_N, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    MPI_Scatter(b, local_N, MPI_DOUBLE, local_b, local_N, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    local_res = dot(local_a, local_b, local_N);

    // adunam rezultatele partiale in procesul 0
    MPI_Reduce(&local_res, &par_res, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

    MPI_Barrier(MPI_COMM_WORLD);

    double t2 = MPI_Wtime();


    if (rank == 0) {
        cout << "===" << endl;
        cout << "Secv: " << secv_res << endl;
        cout << "Par:  " << par_res << endl;
        cout << "Check: " << (fabs(secv_res - par_res) < 1e-6 ? "OK" : "WRONG") << endl;
        cout << "Secv time: " << secv_time * 1e6 << "us" << endl;
        cout << "Time: " << (t2 - t1) * 1e6 << "us" << endl;
    }

    if (a != nullptr) {
        delete [] a;
    }

    if (b != nullptr) {
        delete [] b;
    }

    if (local_a != nullptr) {
        delete [] local_a;
    }
    if (local_b != nullptr) {
        delete [] local_b;
    }
    MPI_Finalize();
    return 0;
}
