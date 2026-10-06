public class BlockThread extends Thread{
    private int startId;
    private int endId;
    private double[] a;
    private double[] b;
    private double[] c;

    public BlockThread(int startId, int endId, double[] a, double[] b, double[] c){
        this.startId = startId;
        this.endId = endId;
        this.a = a;
        this.b = b;
        this.c = c;
    }

    @Override
    public void run(){
        // the thread does one block of indexes: startId (included) to endId (not included)
        for(int i = startId; i < endId; i++){
            c[i] = a[i] + b[i];
        }
    }
}
