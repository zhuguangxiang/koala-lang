

int atomic_set(int *loc, int value)
{
  int result = value;
  __asm__ __volatile__ (
    "lock xchg %1, %0"
    :"+r" (result), "+m" (*loc)
    : /* no input-only operands */
  );
  return result;
}

void *atomic_set_pointer(void **loc, void *ptr)
{
  void* result = ptr;
  __asm__ __volatile__ (
    "lock xchg %1, %0"
    :"+r" (result), "+m" (*loc)
    : /* no input-only operands */
  );
  return result;
}
