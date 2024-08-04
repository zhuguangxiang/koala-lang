class Test {
    public static boolean foo(int a, int b) {
        return a > b;
    }

    public static void main(String[] args) {
        boolean b = foo(1, 2);
        System.out.println(b);
    }
}
