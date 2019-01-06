
#include "stringbuf.h"

void test_stringbuf(void)
{
  DeclareStringBuf(buf);
  String s = {"hello, worldhello, worldhello, w"};
  StringBuf_Append(buf, s);
  s.str = NULL;
  StringBuf_Append(buf, s);
  assert(buf.length == 32 && buf.size == 64);
  puts(buf.data);
  printf("length:%d, size:%d\n", buf.length, buf.size);
  s.str = "Change'e-4";
  StringBuf_Append(buf, s);
  StringBuf_Append(buf, s);
  assert(buf.length == 52 && buf.size == 64);
  puts(buf.data);
  printf("length:%d, size:%d\n", buf.length, buf.size);
  String s1 = {"Llang.Tuple;"};
  String s2 = {"s"};
  String s3 = {"e"};
  StringBuf_Format(buf, "P##:#;", s1, s2, s3);
  assert(buf.length == 69 && buf.size == 96);
  puts(buf.data);
  printf("length:%d, size:%d\n", buf.length, buf.size);
  StringBuf_Format_CStr(buf, "M#:#", NULL, "s");
  assert(buf.length == 72 && buf.size == 96);
  puts(buf.data);
  printf("length:%d, size:%d\n", buf.length, buf.size);
  FiniStringBuf(buf);
}

int main(int argc, char *argv[])
{
  test_stringbuf();
  return 0;
}
