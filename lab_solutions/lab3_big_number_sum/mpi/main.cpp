// sum of two big numbers with MPI
// usage: mpirun -np p main <variant>   variant: 0 sequential, 1, 1.1, 2, 3
// the numbers are in Numar1.txt and Numar2.txt (n, then the digits, least significant first)
// sequential writes output_seq.txt, the others write Numar3.txt and compare it with output_seq.txt
#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <mpi.h>

using namespace std;

typedef unsigned char digit;

// variant 1 uses MPI_Send, with -DSYNC it uses MPI_Ssend to check there is no deadlock
#ifdef SYNC
#define SEND MPI_Ssend
#else
#define SEND MPI_Send
#endif

#define TAG_A 1
#define TAG_B 2
#define TAG_CARRY 3
#define TAG_SUM 4
#define TAG_FINAL 5

// a number in a file, its digits are read in pieces
struct Number {
    ifstream f;
    int n = 0;     // digits in the file
    int left = 0;  // digits not read yet
};

void openNumber(Number& x, const char* name) {
    x.f.open(name);
    if (!x.f) {
        cerr << "cannot open " << name << endl;
        MPI_Abort(MPI_COMM_WORLD, 3);
    }
    x.f >> x.n;
    x.left = x.n;
}

// reads count digits, after the last digit of the number it gives zeros
void readDigits(Number& x, digit* out, int count) {
    for (int i = 0; i < count; i++) {
        out[i] = 0;
        if (x.left == 0) continue;
        char c = 0;
        while (x.f.get(c) && (c < '0' || c > '9')) {}
        if (x.f) out[i] = c - '0';
        x.left--;
    }
}

void writeDigits(ofstream& out, const digit* d, size_t len) {
    string s(len, '0');
    for (size_t i = 0; i < len; i++) s[i] = '0' + d[i];
    out << s;
}

// res = a + b + carry on len digits, returns the carry that goes out
int addDigits(const digit* a, const digit* b, digit* res, int len, int carry) {
    for (int i = 0; i < len; i++) {
        int s = a[i] + b[i] + carry;
        res[i] = s % 10;
        carry = s / 10;
    }
    return carry;
}

// digits that worker w (1..workers) gets when n digits are split between the workers
int chunkSize(int w, int n, int workers) {
    return n / workers + (w - 1 < n % workers ? 1 : 0);
}

bool sameFile(const char* f1, const char* f2) {
    ifstream a(f1, ios::binary), b(f2, ios::binary);
    string s1((istreambuf_iterator<char>(a)), istreambuf_iterator<char>());
    string s2((istreambuf_iterator<char>(b)), istreambuf_iterator<char>());
    return s1 == s2;
}

void sequential() {
    Number x, y;
    openNumber(x, "Numar1.txt");
    openNumber(y, "Numar2.txt");
    int n = max(x.n, y.n);
    vector<digit> a(n), b(n), res(n);
    readDigits(x, a.data(), n);
    readDigits(y, b.data(), n);
    int carry = addDigits(a.data(), b.data(), res.data(), n, 0);

    ofstream out("output_seq.txt");
    writeDigits(out, res.data(), n);
    if (carry) out << 1;
    out << "\n";
}

// variants 1 and 1.1, process 0: reads, sends, receives and writes, it does not add
void master1(int p) {
    Number x, y;
    openNumber(x, "Numar1.txt");
    openNumber(y, "Numar2.txt");
    int n = max(x.n, y.n);
    MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);
    int workers = p - 1;

    // read a piece from each file and send it to the next process
    vector<digit> a, b;
    for (int w = 1; w <= workers; w++) {
        int len = chunkSize(w, n, workers);
        a.resize(len);
        b.resize(len);
        readDigits(x, a.data(), len);
        readDigits(y, b.data(), len);
        SEND(a.data(), len, MPI_UNSIGNED_CHAR, w, TAG_A, MPI_COMM_WORLD);
        SEND(b.data(), len, MPI_UNSIGNED_CHAR, w, TAG_B, MPI_COMM_WORLD);
    }

    // write every piece of the sum as soon as it arrives
    ofstream out("Numar3.txt");
    vector<digit> sum;
    for (int w = 1; w <= workers; w++) {
        int len = chunkSize(w, n, workers);
        sum.resize(len);
        MPI_Recv(sum.data(), len, MPI_UNSIGNED_CHAR, w, TAG_SUM, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        writeDigits(out, sum.data(), len);
    }

    // the carry that leaves the last process is one more digit
    int last;
    MPI_Recv(&last, 1, MPI_INT, workers, TAG_FINAL, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    if (last) out << 1;
    out << "\n";
}

// variant 1, processes with id > 0: add the digits after the carry from the previous process arrived
void worker1(int rank, int p) {
    int n;
    MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);
    int workers = p - 1;
    int len = chunkSize(rank, n, workers);
    vector<digit> a(len), b(len), sum(len);

    MPI_Recv(a.data(), len, MPI_UNSIGNED_CHAR, 0, TAG_A, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    MPI_Recv(b.data(), len, MPI_UNSIGNED_CHAR, 0, TAG_B, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    int carry = 0;
    if (rank > 1) {
        MPI_Recv(&carry, 1, MPI_INT, rank - 1, TAG_CARRY, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
    carry = addDigits(a.data(), b.data(), sum.data(), len, carry);

    // the carry goes first, so the next process does not wait for the sum to reach process 0
    if (rank < workers) {
        SEND(&carry, 1, MPI_INT, rank + 1, TAG_CARRY, MPI_COMM_WORLD);
    }
    SEND(sum.data(), len, MPI_UNSIGNED_CHAR, 0, TAG_SUM, MPI_COMM_WORLD);
    if (rank == workers) {
        SEND(&carry, 1, MPI_INT, 0, TAG_FINAL, MPI_COMM_WORLD);
    }
}

// variant 1.1, processes with id > 0: add first as if the carry was 0, fix the result when the real carry arrives
void worker11(int rank, int p) {
    int n;
    MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);
    int workers = p - 1;
    int len = chunkSize(rank, n, workers);
    vector<digit> a(len), b(len), sum(len);

    MPI_Recv(a.data(), len, MPI_UNSIGNED_CHAR, 0, TAG_A, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    MPI_Recv(b.data(), len, MPI_UNSIGNED_CHAR, 0, TAG_B, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    int carry0 = addDigits(a.data(), b.data(), sum.data(), len, 0);

    // if the sum is not 99...9 the carry that goes out does not depend on the carry that comes in
    bool allNines = true;
    for (int i = 0; i < len; i++) {
        if (sum[i] != 9) allNines = false;
    }
    if (rank < workers && !allNines) {
        SEND(&carry0, 1, MPI_INT, rank + 1, TAG_CARRY, MPI_COMM_WORLD);
    }

    int carryIn = 0;
    if (rank > 1) {
        MPI_Recv(&carryIn, 1, MPI_INT, rank - 1, TAG_CARRY, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
    if (carryIn) {
        // add 1 to the sum, it stops at the first digit that is not 9
        int i = 0;
        while (i < len && sum[i] == 9) sum[i++] = 0;
        if (i < len) sum[i]++;
    }
    int carryOut = allNines ? carryIn : carry0;

    if (rank < workers && allNines) {
        SEND(&carryOut, 1, MPI_INT, rank + 1, TAG_CARRY, MPI_COMM_WORLD);
    }
    SEND(sum.data(), len, MPI_UNSIGNED_CHAR, 0, TAG_SUM, MPI_COMM_WORLD);
    if (rank == workers) {
        SEND(&carryOut, 1, MPI_INT, 0, TAG_FINAL, MPI_COMM_WORLD);
    }
}

// variant 2, scatter and gather, process 0 adds too
void variant2(int rank, int p) {
    Number x, y;
    int n = 0, total = 0;
    vector<digit> a, b, sum;
    if (rank == 0) {
        openNumber(x, "Numar1.txt");
        openNumber(y, "Numar2.txt");
        n = max(x.n, y.n);
        total = (n + p - 1) / p * p;  // n rounded up to a multiple of p, padded with zeros
        a.resize(total);
        b.resize(total);
        sum.resize(total);
        readDigits(x, a.data(), total);
        readDigits(y, b.data(), total);
    }
    MPI_Bcast(&total, 1, MPI_INT, 0, MPI_COMM_WORLD);
    int len = total / p;

    vector<digit> la(len), lb(len), lsum(len);
    MPI_Scatter(a.data(), len, MPI_UNSIGNED_CHAR, la.data(), len, MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);
    MPI_Scatter(b.data(), len, MPI_UNSIGNED_CHAR, lb.data(), len, MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);

    int carry = 0;
    if (rank > 0) {
        MPI_Recv(&carry, 1, MPI_INT, rank - 1, TAG_CARRY, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
    carry = addDigits(la.data(), lb.data(), lsum.data(), len, carry);
    if (rank < p - 1) {
        SEND(&carry, 1, MPI_INT, rank + 1, TAG_CARRY, MPI_COMM_WORLD);
    }

    MPI_Gather(lsum.data(), len, MPI_UNSIGNED_CHAR, sum.data(), len, MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);

    // the carry of the last process goes after the gather, process 0 is waiting in it for that process
    if (rank == p - 1) {
        SEND(&carry, 1, MPI_INT, 0, TAG_FINAL, MPI_COMM_WORLD);
    }
    if (rank == 0) {
        int last;
        MPI_Recv(&last, 1, MPI_INT, p - 1, TAG_FINAL, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        if (last) sum.push_back(1);
        // the zeros added to reach a multiple of p are not part of the sum
        while ((int)sum.size() > n && sum.back() == 0) sum.pop_back();
        ofstream out("Numar3.txt");
        writeDigits(out, sum.data(), sum.size());
        out << "\n";
    }
}

// variant 3, process 0: Isend the pieces, Irecv the sums
void master3(int p) {
    Number x, y;
    openNumber(x, "Numar1.txt");
    openNumber(y, "Numar2.txt");
    int n = max(x.n, y.n);
    MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);
    int workers = p - 1;

    // the buffers have to stay alive until the sends are done
    vector<vector<digit>> a(workers), b(workers);
    vector<MPI_Request> reqs(3 * workers + 1);
    int k = 0;
    for (int w = 1; w <= workers; w++) {
        int len = chunkSize(w, n, workers);
        a[w - 1].resize(len);
        b[w - 1].resize(len);
        readDigits(x, a[w - 1].data(), len);
        readDigits(y, b[w - 1].data(), len);
        MPI_Isend(a[w - 1].data(), len, MPI_UNSIGNED_CHAR, w, TAG_A, MPI_COMM_WORLD, &reqs[k++]);
        MPI_Isend(b[w - 1].data(), len, MPI_UNSIGNED_CHAR, w, TAG_B, MPI_COMM_WORLD, &reqs[k++]);
    }

    // every sum goes straight to its place in the result, whatever the order they arrive in
    vector<digit> sum(n);
    int start = 0, last = 0;
    for (int w = 1; w <= workers; w++) {
        int len = chunkSize(w, n, workers);
        MPI_Irecv(sum.data() + start, len, MPI_UNSIGNED_CHAR, w, TAG_SUM, MPI_COMM_WORLD, &reqs[k++]);
        start += len;
    }
    MPI_Irecv(&last, 1, MPI_INT, workers, TAG_FINAL, MPI_COMM_WORLD, &reqs[k++]);
    MPI_Waitall(k, reqs.data(), MPI_STATUSES_IGNORE);

    ofstream out("Numar3.txt");
    writeDigits(out, sum.data(), n);
    if (last) out << 1;
    out << "\n";
}

// variant 3, processes with id > 0: the digits and the carry can arrive in any order
void worker3(int rank, int p) {
    int n;
    MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);
    int workers = p - 1;
    int len = chunkSize(rank, n, workers);
    vector<digit> a(len), b(len), sum(len);
    int carry = 0;

    MPI_Request in[3];
    int k = 0;
    MPI_Irecv(a.data(), len, MPI_UNSIGNED_CHAR, 0, TAG_A, MPI_COMM_WORLD, &in[k++]);
    MPI_Irecv(b.data(), len, MPI_UNSIGNED_CHAR, 0, TAG_B, MPI_COMM_WORLD, &in[k++]);
    if (rank > 1) {
        MPI_Irecv(&carry, 1, MPI_INT, rank - 1, TAG_CARRY, MPI_COMM_WORLD, &in[k++]);
    }
    MPI_Waitall(k, in, MPI_STATUSES_IGNORE);

    carry = addDigits(a.data(), b.data(), sum.data(), len, carry);

    MPI_Request out[3];
    k = 0;
    if (rank < workers) {
        MPI_Isend(&carry, 1, MPI_INT, rank + 1, TAG_CARRY, MPI_COMM_WORLD, &out[k++]);
    }
    MPI_Isend(sum.data(), len, MPI_UNSIGNED_CHAR, 0, TAG_SUM, MPI_COMM_WORLD, &out[k++]);
    if (rank == workers) {
        MPI_Isend(&carry, 1, MPI_INT, 0, TAG_FINAL, MPI_COMM_WORLD, &out[k++]);
    }
    MPI_Waitall(k, out, MPI_STATUSES_IGNORE);
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    int p, rank;
    MPI_Comm_size(MPI_COMM_WORLD, &p);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    string variant = (argc > 1) ? argv[1] : "0";
    bool ok = variant == "0" || variant == "1" || variant == "1.1" || variant == "2" || variant == "3";
    if (!ok || (variant != "0" && p < 2)) {
        if (rank == 0) cerr << "usage: mpirun -np p main <0|1|1.1|2|3>, the parallel variants need p >= 2" << endl;
        MPI_Finalize();
        return 2;
    }

    // the time includes reading the numbers and writing the sum
    MPI_Barrier(MPI_COMM_WORLD);
    double t1 = MPI_Wtime();

    if (variant == "0") {
        if (rank == 0) sequential();
    } else if (variant == "1") {
        if (rank == 0) master1(p); else worker1(rank, p);
    } else if (variant == "1.1") {
        if (rank == 0) master1(p); else worker11(rank, p);
    } else if (variant == "2") {
        variant2(rank, p);
    } else {
        if (rank == 0) master3(p); else worker3(rank, p);
    }

    MPI_Barrier(MPI_COMM_WORLD);
    double t2 = MPI_Wtime();

    int code = 0;
    if (rank == 0) {
        if (variant != "0") {
            ifstream ref("output_seq.txt");
            if (!ref) {
                cerr << "output_seq.txt is missing, run the sequential version first" << endl;
                code = 2;
            } else if (!sameFile("Numar3.txt", "output_seq.txt")) {
                cerr << "Numar3.txt is different from output_seq.txt" << endl;
                code = 1;
            }
        }
        cout << fixed << setprecision(4) << (t2 - t1) * 1000 << endl;
    }
    MPI_Finalize();
    return code;
}
