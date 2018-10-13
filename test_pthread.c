
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

int gval = 0;

void *f1(void *arg)
{
  while (gval < 20) {
    sleep(1);
    gval++;
  }
}

void *f2(void *arg)
{
  int count = 0;
  while (count < 100) {
    if (gval == 1)
      break;
    else {
      sleep(1);
      count++;
    }
  }
  printf("count:%d\n", count);
}

int main(int argc, char *argv[])
{
  pthread_t t1, t2;
  pthread_create(&t1, NULL, f1, NULL);
  pthread_create(&t2, NULL, f2, NULL);
  pthread_join(t1, NULL);
  pthread_join(t2, NULL);
  return 0;
}
