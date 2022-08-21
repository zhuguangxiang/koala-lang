
class Fib {
    private short n1;
    private short n2;
    private byte n3;

    Fib() {
        n1 = 100;
        n2 = 200;
        n3 = 2;
    }
    static int fib(int n) {
        if (n <= 2) return 1;
        return fib(n-1) + fib(n-2);
    }
    static short test_short(short a) {
        return a;
    }
    static int not(int n, short b, long l) {
        boolean a = b > 20;
        a = l > 100;
        short num = 10000;
        short n2 = 20000;
        short n3 = (short)(num + 2);
        short n4 = test_short((short)10);
        System.out.println(n2);
        System.out.println(n < 10);
        return ~n;
    }
    public static void main(String[] args) {
        new Fib();
        System.out.println(not(1, (short)20, (long)2000));
        System.out.println(fib(40));
    }
}
