

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
