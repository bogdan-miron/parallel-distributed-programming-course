// usage: main <p> [variant]   p = 0 is the sequential version
// variants: horizontal, vertical, block, linear, cyclic
// built with -DSTATIC_ALLOC the matrices are static arrays, otherwise they are allocated with new
#include <algorithm>
#include <charconv>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <string>
#include <thread>
#include <vector>
using namespace std;

const int MAXK = 15;
#ifdef STATIC_ALLOC
const int MAX = 10000;
static int f[MAX][MAX];
static int v[MAX][MAX];
#else
int** f;
int** v;
#endif
int c[MAXK][MAXK];
int n, m, k;
int blockRows, blockCols;  // how the threads are laid out for the block variant

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

void readInput() {
    Reader rc("ConvolutionM.txt");
    k = rc.readInt();
    rc.readInt();
    if (k % 2 == 0 || k > MAXK) {
        fprintf(stderr, "k has to be odd and at most %d\n", MAXK);
        exit(1);
    }
    for (int i = 0; i < k; i++)
        for (int j = 0; j < k; j++) c[i][j] = rc.readInt();

    Reader rf("MatrixF.txt");
    n = rf.readInt();
    m = rf.readInt();
#ifdef STATIC_ALLOC
    if (n > MAX || m > MAX) {
        fprintf(stderr, "the matrix is bigger than MAX = %d\n", MAX);
        exit(1);
    }
#else
    f = new int*[n];
    v = new int*[n];
    for (int i = 0; i < n; i++) {
        f[i] = new int[m];
        v[i] = new int[m];
    }
#endif
    for (int i = 0; i < n; i++)
        for (int j = 0; j < m; j++) f[i][j] = rf.readInt();
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
            auto r = to_chars(tmp, tmp + sizeof(tmp), v[i][j]);
            out.append(tmp, r.ptr);
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

// value of V[i][j], the border of F is repeated outside the matrix
int cell(int i, int j) {
    int r = k / 2, s = 0;
    if (i >= r && i < n - r && j >= r && j < m - r) {
        for (int a = 0; a < k; a++)
            for (int b = 0; b < k; b++) s += f[i - r + a][j - r + b] * c[a][b];
    } else {
        for (int a = 0; a < k; a++) {
            int ii = min(max(i - r + a, 0), n - 1);
            for (int b = 0; b < k; b++) {
                int jj = min(max(j - r + b, 0), m - 1);
                s += f[ii][jj] * c[a][b];
            }
        }
    }
    return s;
}

void rectangle(int i0, int i1, int j0, int j1) {
    for (int i = i0; i < i1; i++)
        for (int j = j0; j < j1; j++) v[i][j] = cell(i, j);
}

// the cells numbered from, from + step, ... (row by row), up to to
void indexes(long long from, long long to, long long step) {
    for (long long x = from; x < to; x += step) {
        int i = x / m, j = x % m;
        v[i][j] = cell(i, j);
    }
}

void work(int t, int p, const string& variant) {
    long long total = (long long)n * m;
    if (variant == "horizontal") {
        rectangle(t * n / p, (t + 1) * n / p, 0, m);
    } else if (variant == "vertical") {
        rectangle(0, n, t * m / p, (t + 1) * m / p);
    } else if (variant == "block") {
        int bi = t / blockCols, bj = t % blockCols;
        rectangle(bi * n / blockRows, (bi + 1) * n / blockRows, bj * m / blockCols, (bj + 1) * m / blockCols);
    } else if (variant == "linear") {
        indexes(t * total / p, (t + 1) * total / p, 1);
    } else {
        indexes(t, total, p);  // cyclic
    }
}

// p threads as a blockRows x blockCols grid, more blocks along the longer side
void chooseGrid(int p) {
    int a = 1;
    for (int d = 1; d * d <= p; d++)
        if (p % d == 0) a = d;
    int b = p / a;
    blockRows = n > m ? b : a;
    blockCols = n > m ? a : b;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "usage: main <p> [horizontal|vertical|block|linear|cyclic]\n");
        return 1;
    }
    int p = atoi(argv[1]);
    string variant = argc > 2 ? argv[2] : "horizontal";
    if (p > 0 && variant != "horizontal" && variant != "vertical" && variant != "block" && variant != "linear" &&
        variant != "cyclic") {
        fprintf(stderr, "unknown variant %s\n", variant.c_str());
        return 1;
    }

    readInput();
    if (p > 0) chooseGrid(p);

    auto t1 = chrono::high_resolution_clock::now();
    if (p == 0) {
        rectangle(0, n, 0, m);
    } else {
        vector<thread> threads;
        for (int t = 0; t < p; t++) threads.emplace_back(work, t, p, ref(variant));
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
