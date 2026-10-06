import java.io.*;
import java.nio.file.*;
import java.util.Arrays;
import java.util.Locale;

public class Main {

    static double[] readVector(String path) throws IOException {
        try (BufferedReader in = new BufferedReader(new FileReader(path))) {
            int n = Integer.parseInt(in.readLine().trim());
            double[] v = new double[n];
            for (int i = 0; i < n; i++) v[i] = Double.parseDouble(in.readLine());
            return v;
        }
    }

    static void writeVector(String path, double[] v) throws IOException {
        try (PrintWriter out = new PrintWriter(new BufferedWriter(new FileWriter(path)))) {
            out.print(v.length + "\n");
            for (double x : v) out.print(String.format(Locale.US, "%.6f\n", x));
        }
    }

    static boolean sameFile(String p1, String p2) throws IOException {
        return Arrays.equals(Files.readAllBytes(Paths.get(p1)), Files.readAllBytes(Paths.get(p2)));
    }

    public static void main(String[] args) throws IOException {
        double[] a = readVector("A.txt");
        double[] b = readVector("B.txt");
        int n = a.length;
        double[] c = new double[n];

        long start = System.nanoTime();
        for (int i = 0; i < n; i++) c[i] = a[i] + b[i];
        long end = System.nanoTime();

        writeVector("output.txt", c);

        if (Files.exists(Paths.get("expected.txt")) && !sameFile("output.txt", "expected.txt")) {
            System.err.println("output.txt is different from expected.txt");
            System.exit(1);
        }

        System.out.println((double) (end - start) / 1E6);
    }
}
