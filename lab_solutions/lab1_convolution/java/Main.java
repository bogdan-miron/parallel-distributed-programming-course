import java.io.*;
import java.util.Locale;

// usage: java Main <p> [variant]   p = 0 is the sequential version
// horizontal, vertical, block, linear, cyclic
public class Main {
    static int n, m, k;
    static int[][] f, v, c;
    static int blockRows, blockCols;  // how the threads are laid out for the block variant

    static class Reader {
        InputStream in;
        byte[] buf = new byte[1 << 20];
        int pos = 0, len = 0;

        Reader(String path) throws IOException {
            in = new FileInputStream(path);
        }

        int next() throws IOException {
            if (pos == len) {
                len = in.read(buf, 0, buf.length);
                pos = 0;
                if (len <= 0) return -1;
            }
            return buf[pos++];
        }

        int readInt() throws IOException {
            int ch = next();
            while (ch != '-' && (ch < '0' || ch > '9')) {
                if (ch == -1) throw new IOException("not enough numbers in the input file");
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

        void close() throws IOException {
            in.close();
        }
    }

    static void readInput() throws IOException {
        Reader rc = new Reader("ConvolutionM.txt");
        k = rc.readInt();
        rc.readInt();
        if (k % 2 == 0) throw new IOException("k has to be odd");
        c = new int[k][k];
        for (int i = 0; i < k; i++)
            for (int j = 0; j < k; j++) c[i][j] = rc.readInt();
        rc.close();

        Reader rf = new Reader("MatrixF.txt");
        n = rf.readInt();
        m = rf.readInt();
        f = new int[n][m];
        v = new int[n][m];
        for (int i = 0; i < n; i++)
            for (int j = 0; j < m; j++) f[i][j] = rf.readInt();
        rf.close();
    }

    static void writeMatrix(String path) throws IOException {
        try (OutputStream out = new BufferedOutputStream(new FileOutputStream(path), 1 << 20)) {
            out.write((n + " " + m + "\n").getBytes());
            byte[] line = new byte[m * 12 + 1];
            byte[] digits = new byte[12];
            for (int i = 0; i < n; i++) {
                int len = 0;
                for (int j = 0; j < m; j++) {
                    int x = v[i][j];
                    if (x < 0) {
                        line[len++] = '-';
                        x = -x;
                    }
                    int d = 0;
                    do {
                        digits[d++] = (byte) ('0' + x % 10);
                        x /= 10;
                    } while (x > 0);
                    while (d > 0) line[len++] = digits[--d];
                    line[len++] = (byte) (j + 1 < m ? ' ' : '\n');
                }
                out.write(line, 0, len);
            }
        }
    }

    // compares the parallel output with the sequential one
    static boolean sameFile(String p1, String p2) throws IOException {
        File a = new File(p1), b = new File(p2);
        if (!a.exists() || !b.exists()) {
            System.err.println("can't compare " + p1 + " with " + p2 + ", run the sequential version first");
            return false;
        }
        if (a.length() != b.length()) return false;
        try (InputStream x = new FileInputStream(a); InputStream y = new FileInputStream(b)) {
            byte[] bx = new byte[1 << 20], by = new byte[1 << 20];
            int nx;
            while ((nx = x.readNBytes(bx, 0, bx.length)) > 0) {
                int ny = y.readNBytes(by, 0, nx);
                if (ny != nx) return false;
                for (int i = 0; i < nx; i++)
                    if (bx[i] != by[i]) return false;
            }
        }
        return true;
    }

    // value of V[i][j], the border of F is repeated outside the matrix
    static int cell(int i, int j) {
        int r = k / 2, s = 0;
        if (i >= r && i < n - r && j >= r && j < m - r) {
            for (int a = 0; a < k; a++)
                for (int b = 0; b < k; b++) s += f[i - r + a][j - r + b] * c[a][b];
        } else {
            for (int a = 0; a < k; a++) {
                int ii = Math.min(Math.max(i - r + a, 0), n - 1);
                for (int b = 0; b < k; b++) {
                    int jj = Math.min(Math.max(j - r + b, 0), m - 1);
                    s += f[ii][jj] * c[a][b];
                }
            }
        }
        return s;
    }

    static void rectangle(int i0, int i1, int j0, int j1) {
        for (int i = i0; i < i1; i++)
            for (int j = j0; j < j1; j++) v[i][j] = cell(i, j);
    }

    // the cells numbered from, from + step, ... (row by row), up to to
    static void indexes(long from, long to, long step) {
        for (long x = from; x < to; x += step) {
            int i = (int) (x / m), j = (int) (x % m);
            v[i][j] = cell(i, j);
        }
    }

    static void work(int t, int p, String variant) {
        long total = (long) n * m;
        switch (variant) {
            case "horizontal":
                rectangle(t * n / p, (t + 1) * n / p, 0, m);
                break;
            case "vertical":
                rectangle(0, n, t * m / p, (t + 1) * m / p);
                break;
            case "block":
                int bi = t / blockCols, bj = t % blockCols;
                rectangle(bi * n / blockRows, (bi + 1) * n / blockRows, bj * m / blockCols, (bj + 1) * m / blockCols);
                break;
            case "linear":
                indexes(t * total / p, (t + 1) * total / p, 1);
                break;
            default:  // cyclic
                indexes(t, total, p);
        }
    }

    // p threads as a blockRows x blockCols grid, more blocks along the longer side
    static void chooseGrid(int p) {
        int a = 1;
        for (int d = 1; d * d <= p; d++)
            if (p % d == 0) a = d;
        int b = p / a;
        blockRows = n > m ? b : a;
        blockCols = n > m ? a : b;
    }

    public static void main(String[] args) throws Exception {
        if (args.length < 1) {
            System.err.println("usage: java Main <p> [horizontal|vertical|block|linear|cyclic]");
            System.exit(1);
        }
        int p = Integer.parseInt(args[0]);
        String variant = args.length > 1 ? args[1] : "horizontal";
        if (p > 0 && !variant.matches("horizontal|vertical|block|linear|cyclic")) {
            System.err.println("unknown variant " + variant);
            System.exit(1);
        }

        readInput();
        if (p > 0) chooseGrid(p);

        long t1 = System.nanoTime();
        if (p == 0) {
            rectangle(0, n, 0, m);
        } else {
            Thread[] threads = new Thread[p];
            for (int t = 0; t < p; t++) {
                final int id = t;
                threads[t] = new Thread(() -> work(id, p, variant));
                threads[t].start();
            }
            for (Thread th : threads) th.join();
        }
        long t2 = System.nanoTime();

        writeMatrix(p == 0 ? "output_seq.txt" : "output.txt");
        if (p > 0 && !sameFile("output.txt", "output_seq.txt")) {
            System.err.println("output.txt is different from output_seq.txt");
            System.exit(1);
        }

        System.out.println(String.format(Locale.US, "%.4f", (t2 - t1) / 1e6));
    }
}
