// convolution with a 3 x 3 kernel done in place: the result ends up in the same matrix, no second matrix
// the rows are split between p threads
// usage: main <p>   p = 0 is the sequential version
#include <algorithm>
#include <barrier>
#include <charconv>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <string>
#include <thread>
#include <vector>
using namespace std;

int** f;
int c[3][3];
int n, m;

struct Reader {
    FILE* fp;
    vector<char> buf;
    size_t pos = 0, len = 0;

    Reader(const char* path) : fp(fopen(path, "rb")), buf(1 << 20) {
        if (!fp) {
            fprintf(stderr, "can't open %s\n", path);
            exit(1);
        }
    }
    ~Reader() { fclose(fp); }

    int next() {
        if (pos == len) {
            len = fread(buf.data(), 1, buf.size(), fp);
            pos = 0;
            if (len == 0) return EOF;
        }
        return buf[pos++];
    }

    int readInt() {
        int ch = next();
        while (ch != '-' && (ch < '0' || ch > '9')) {
            if (ch == EOF) {
                fprintf(stderr, "not enough numbers in the input file\n");
                exit(1);
            }
            ch = next();
        }
        int sign = 1;
        if (ch == '-') {
            sign = -1;
            ch = next();
        }
        int x = 0;
        while (ch >= '0' && ch <= '9') {
            x = x * 10 + (ch - '0');
            ch = next();
        }
        return sign * x;
    }
};

// date.txt: n m, the matrix, then k k and the kernel
void readInput() {
    Reader r("date.txt");
    n = r.readInt();
    m = r.readInt();
    f = new int*[n];
    for (int i = 0; i < n; i++) {
        f[i] = new int[m];
        for (int j = 0; j < m; j++) f[i][j] = r.readInt();
    }
    int k = r.readInt();
    r.readInt();
    if (k != 3) {
        fprintf(stderr, "only the 3 x 3 kernel is supported\n");
        exit(1);
    }
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++) c[i][j] = r.readInt();
}

void writeMatrix(const char* path) {
    FILE* fp = fopen(path, "wb");
    if (!fp) {
        fprintf(stderr, "can't write %s\n", path);
        exit(1);
    }
    string out = to_string(n) + " " + to_string(m) + "\n";
    char tmp[16];
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < m; j++) {
            auto res = to_chars(tmp, tmp + sizeof(tmp), f[i][j]);
            out.append(tmp, res.ptr);
            out += (j + 1 < m) ? ' ' : '\n';
        }
        if (out.size() > (1 << 20)) {
            fwrite(out.data(), 1, out.size(), fp);
            out.clear();
        }
    }
    fwrite(out.data(), 1, out.size(), fp);
    fclose(fp);
}

// compares the parallel output with the sequential one
bool sameFile(const char* p1, const char* p2) {
    FILE* a = fopen(p1, "rb");
    FILE* b = fopen(p2, "rb");
    if (!a || !b) {
        fprintf(stderr, "can't compare %s with %s, run the sequential version first\n", p1, p2);
        return false;
    }
    vector<char> x(1 << 20), y(1 << 20);
    bool same = true;
    while (same) {
        size_t nx = fread(x.data(), 1, x.size(), a);
        size_t ny = fread(y.data(), 1, y.size(), b);
        if (nx != ny || !equal(x.begin(), x.begin() + nx, y.begin())) same = false;
        if (nx == 0) break;
    }
    fclose(a);
    fclose(b);
    return same;
}

vector<int> copyRow(int i) {
    return vector<int>(f[i], f[i] + m);
}

// computes the new rows lo..hi-1 and writes them over the old ones
// prev is a copy of the old row lo-1 and below is a copy of the old row hi (the border is repeated outside the matrix)
// the old row i is saved in cur before it is overwritten, row i+1 is still old because the rows go top to bottom
void process(int lo, int hi, vector<int>& prev, const vector<int>& below) {
    vector<int> cur(m);
    for (int i = lo; i < hi; i++) {
        copy(f[i], f[i] + m, cur.begin());
        const int* next = (i + 1 < hi) ? f[i + 1] : below.data();
        for (int j = 0; j < m; j++) {
            int left = (j > 0) ? j - 1 : 0;
            int right = (j < m - 1) ? j + 1 : m - 1;
            f[i][j] = c[0][0] * prev[left] + c[0][1] * prev[j] + c[0][2] * prev[right] +
                      c[1][0] * cur[left] + c[1][1] * cur[j] + c[1][2] * cur[right] +
                      c[2][0] * next[left] + c[2][1] * next[j] + c[2][2] * next[right];
        }
        swap(prev, cur);  // the old row i is now the previous row
    }
}

void sequential() {
    vector<int> above = copyRow(0);
    vector<int> below = copyRow(n - 1);
    process(0, n, above, below);
}

// every thread gets n / p rows, and the first n % p threads get one more row
void rowsOf(int t, int p, int& lo, int& hi) {
    int base = n / p, rest = n % p;
    lo = t * base + min(t, rest);
    hi = lo + base + (t < rest ? 1 : 0);
}

void worker(int t, int p, barrier<>& sync) {
    int lo, hi;
    rowsOf(t, p, lo, hi);
    // the rows next to the block belong to other threads, copy them before anyone changes anything
    vector<int> above = copyRow(max(lo - 1, 0));
    vector<int> below = copyRow(min(hi, n - 1));
    sync.arrive_and_wait();
    process(lo, hi, above, below);
}

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "usage: main <p>\n");
        return 1;
    }
    int p = atoi(argv[1]);

    readInput();
    barrier<> sync(max(p, 1));

    auto t1 = chrono::high_resolution_clock::now();
    if (p == 0) {
        sequential();
    } else {
        vector<thread> threads;
        for (int t = 0; t < p; t++) threads.emplace_back(worker, t, p, ref(sync));
        for (auto& th : threads) th.join();
    }
    auto t2 = chrono::high_resolution_clock::now();

    writeMatrix(p == 0 ? "output_seq.txt" : "output.txt");
    if (p > 0 && !sameFile("output.txt", "output_seq.txt")) {
        fprintf(stderr, "output.txt is different from output_seq.txt\n");
        return 1;
    }

    chrono::duration<double, milli> ms = t2 - t1;
    printf("%.4f\n", ms.count());
    return 0;
}
