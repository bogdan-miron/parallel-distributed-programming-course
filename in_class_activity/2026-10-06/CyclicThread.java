public class CyclicThread extends Thread {
    private int p;
    private int th_id;
    private double[] a,b,c;

    public CyclicThread(int p, int th_id, double[] a, double[] b, double[] c){
        this.p = p;
        this.th_id = th_id;
        this.a = a;
        this.b = b;
        this.c = c;
    }

    @Override
    public void run(){
        // thread th_id takes the indexes th_id, th_id + p, th_id + 2p, ...
        for(int i=th_id; i<a.length; i+=p){
            c[i] = a[i] + b[i];
        }
    }

}
