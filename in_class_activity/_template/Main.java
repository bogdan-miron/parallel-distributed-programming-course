public class Main {

    public static void main(String[] args) throws InterruptedException {
        int p = args.length > 0 ? Integer.parseInt(args[0]) : 4;   // number of threads

        Thread[] threads = new Thread[p];
        for (int t = 0; t < p; t++) {
            int id = t;
            threads[t] = new Thread(() -> work(id));
            threads[t].start();
        }
        for (Thread th : threads) th.join();
    }

    static void work(int id) {
        System.out.println("thread " + id + " running");
    }
}
