class Test {
    public static boolean foo(int a, int b) { return a > b; }

    public static void main(String[] args)
    {
        boolean b = foo(1, 2);
        System.out.println(b);
        int a = 100;
        System.out.println(a);
        Integer i = new Integer(100);
        System.out.println(i.toString());
    }
}

/*
Test();
    descriptor: ()V
    flags: (0x0000)
    Code:
      stack=1, locals=1, args_size=1
         0: aload_0
         1: invokespecial #1                  // Method java/lang/Object."<init>":()V
         4: return
      LineNumberTable:
        line 1: 0

  public static boolean foo(int, int);
    descriptor: (II)Z
    flags: (0x0009) ACC_PUBLIC, ACC_STATIC
    Code:
      stack=2, locals=2, args_size=2
         0: iload_0
         1: iload_1
         2: if_icmple     9
         5: iconst_1
         6: goto          10
         9: iconst_0
        10: ireturn
      LineNumberTable:
        line 3: 0
      StackMapTable: number_of_entries = 2
        frame_type = 9 // same
        frame_type = 64 // same_locals_1_stack_item
          stack = [ int ]

  public static void main(java.lang.String[]);
    descriptor: ([Ljava/lang/String;)V
    flags: (0x0009) ACC_PUBLIC, ACC_STATIC
    Code:
      stack=2, locals=2, args_size=1
         0: iconst_1
         1: iconst_2
         2: invokestatic  #7                  // Method foo:(II)Z
         5: istore_1
         6: getstatic     #13                 // Field
java/lang/System.out:Ljava/io/PrintStream; 9: iload_1 10: invokevirtual #19 // Method
java/io/PrintStream.println:(Z)V 13: return LineNumberTable: line 7: 0 line 8: 6 line 9:
13
}
*/
