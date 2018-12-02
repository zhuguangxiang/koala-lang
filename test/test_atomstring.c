
#include <assert.h>
#include <string.h>
#include "atomstring.h"

void test_atomstring(void)
{
	AtomString_Init();
	String s1 = AtomString_New("hello");
	String s2 = AtomString_New("hello");
	assert(AtomString_Length(s1) == 5);
	assert(AtomString_Equal(s1, s2));
	assert(!strcmp(s1.str, "hello"));
	AtomString_Fini();
}

int main(int argc, char *argv[])
{
	test_atomstring();
	return 0;
}
