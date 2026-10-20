import java.io.*;
import java.util.Locale;
import java.util.concurrent.BrokenBarrierException;
import java.util.concurrent.CyclicBarrier;

// convolution with a 3 x 3 kernel done in place: the result ends up in the same matrix, no second matrix
// the rows are split between p threads
// usage: java Main <p>   p = 0 is the sequential version
public class Main {
    static int[][] f;
    static int[][] c = new int[3][3];
    static int n, m;

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

    // date.txt: n m, the matrix, then k k and the kernel
    static void readInput() throws IOException {
        Reader r = new Reader("date.txt");
        n = r.readInt();
        m = r.readInt();
        f = new int[n][m];
        for (int i = 0; i < n; i++)
            for (int j = 0; j < m; j++) f[i][j] = r.readInt();
        int k = r.readInt();
        r.readInt();
        if (k != 3) throw new IOException("only the 3 x 3 kernel is supported");
        for (int i = 0; i < 3; i++)
            for (int j = 0; j < 3; j++) c[i][j] = r.readInt();
        r.close();
    }

    static void writeMatrix(String path) throws IOException {
        try (OutputStream out = new BufferedOutputStream(new FileOutputStream(path), 1 << 20)) {
            out.write((n + " " + m + "\n").getBytes());
            byte[] line = new byte[m * 12 + 1];
            byte[] digits = new byte[12];
            for (int i = 0; i < n; i++) {
                int len = 0;
                for (int j = 0; j < m; j++) {
                    int x = f[i][j];
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

    static int[] copyRow(int i) {
        return f[i].clone();
    }

    // computes the new rows lo..hi-1 and writes them over the old ones
    // prev is a copy of the old row lo-1 and below is a copy of the old row hi (the border is repeated outside the matrix)
    // the old row i is saved in cur before it is overwritten, row i+1 is still old because the rows go top to bottom
    static void process(int lo, int hi, int[] prev, int[] below) {
        int[] cur = new int[m];
        for (int i = lo; i < hi; i++) {
            System.arraycopy(f[i], 0, cur, 0, m);
            int[] next = (i + 1 < hi) ? f[i + 1] : below;
            int[] row = f[i];
            for (int j = 0; j < m; j++) {
                int left = (j > 0) ? j - 1 : 0;
                int right = (j < m - 1) ? j + 1 : m - 1;
                row[j] = c[0][0] * prev[left] + c[0][1] * prev[j] + c[0][2] * prev[right] +
                         c[1][0] * cur[left] + c[1][1] * cur[j] + c[1][2] * cur[right] +
                         c[2][0] * next[left] + c[2][1] * next[j] + c[2][2] * next[right];
            }
            int[] tmp = prev;  // the old row i is now the previous row
            prev = cur;
            cur = tmp;
        }
    }

    static void sequential() {
        process(0, n, copyRow(0), copyRow(n - 1));
    }

    // every thread gets n / p rows, and the first n % p threads get one more row
    static void worker(int t, int p, CyclicBarrier sync) throws InterruptedException, BrokenBarrierException {
        int base = n / p, rest = n % p;
        int lo = t * base + Math.min(t, rest);
        int hi = lo + base + (t < rest ? 1 : 0);
        // the rows next to the block belong to other threads, copy them before anyone changes anything
        int[] above = copyRow(Math.max(lo - 1, 0));
        int[] below = copyRow(Math.min(hi, n - 1));
        sync.await();
        process(lo, hi, above, below);
    }

    public static void main(String[] args) throws Exception {
        if (args.length < 1) {
            System.err.println("usage: java Main <p>");
            System.exit(1);
        }
        int p = Integer.parseInt(args[0]);

        readInput();
        CyclicBarrier sync = new CyclicBarrier(Math.max(p, 1));

        long t1 = System.nanoTime();
        if (p == 0) {
            sequential();
        } else {
            Thread[] threads = new Thread[p];
            for (int t = 0; t < p; t++) {
                final int id = t;
                threads[t] = new Thread(() -> {
                    try {
                        worker(id, p, sync);
                    } catch (InterruptedException | BrokenBarrierException e) {
                        throw new RuntimeException(e);
                    }
                });
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
