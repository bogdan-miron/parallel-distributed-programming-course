import java.util.Locale;
import java.util.Random;

public class Main {
    static int N = 1000000;
    static int P = 2;

    static final Random random = new Random();

    public static void sum(double[] a, double[] b, double[] c){
        for (int i = 0; i < a.length; i++) {
            c[i] = a[i] + b[i];
        }
    }

    public static void sumCyclic(double[] a, double[] b, double[] c) throws InterruptedException {
        CyclicThread[] threads = new CyclicThread[P];
        for (int t = 0; t < P; t++) {
            threads[t] = new CyclicThread(P, t, a, b, c);
            threads[t].start();
        }
        // the sum is ready only after all the threads finished
        for (int t = 0; t < P; t++) {
            threads[t].join();
        }
    }

    public static void initializeArray(double[] arr){
        for (int i = 0; i < arr.length; i++) {
            arr[i] = random.nextDouble();
        }
    }

    public static boolean validateSum(double[] a, double[] b, double[] c){
        if(a.length != b.length || a.length != c.length)
            return false;
        for (int i = 0; i < a.length; i++) {
            if (c[i] != a[i] + b[i])
                return false;
        }
        return true;
    }

    // prints the first elements, with the thread that computed each one (cyclic: index % P)
    public static void show(double[] a, double[] b, double[] c, int count){
        System.out.println("index  thread  a + b = c");
        for (int i = 0; i < count; i++) {
            System.out.printf(Locale.US, "%5d  %6d  %.4f + %.4f = %.4f%n", i, i % P, a[i], b[i], c[i]);
        }
    }

    public static void main(String[] args) throws InterruptedException {
        double[] a = new double[N];
        double[] b = new double[N];
        double[] c = new double[N];
        initializeArray(a);
        initializeArray(b);

        long start = System.nanoTime();
        sum(a, b, c);
        long end = System.nanoTime();
        System.out.println("sequential: " + (end - start) / 1E6 + " ms, correct: " + validateSum(a, b, c));

        // new result array, so the check does not pass with the values from the sequential sum
        c = new double[N];
        start = System.nanoTime();
        sumCyclic(a, b, c);
        end = System.nanoTime();
        System.out.println("cyclic with " + P + " threads: " + (end - start) / 1E6 + " ms, correct: " + validateSum(a, b, c));

        show(a, b, c, 10);
    }
}
