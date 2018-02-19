
#include <unistd.h>
#include <time.h>
#include "common.h"
#include "vector.h"
/*
 ./build_lib.sh
 gcc -g -std=c99 test_vec.c -lkoala -L.
 valgrind ./a.out
 */
int random_string(char *data, int len)
{
	static const char char_set[] = "0123456789abcdefghijklmnopqrstuvwxyz" \
																 "ABCDEFGHIJKLMNOPQRSTUWXYZ";
	int i;
	int idx;

	for (i = 0; i < len; i++) {
		idx = rand() % (nr_elts(char_set) - 1);
		data[i] = char_set[idx];
	}

	data[i] = 0;

	return 0;
}

char mem[40][10];
char *strings[40];

void string_check(char *s, int index)
{
	ASSERT(strcmp(s, strings[index]) == 0);
}

void test_vector_append(void)
{
	Vector *vec = Vector_New();
	ASSERT_PTR(vec);
	int res;
	int i;
	for (i = 0; i < 30; i++) {
		res = Vector_Append(vec, strings[i]);
		ASSERT(res >= 0);
	}

	for (i = 0; i < Vector_Size(vec); i++) {
		char *s = Vector_Get(vec, i);
		if (s) printf("%s\n", s);
		string_check(s, i);
	}

	Vector_Free(vec, NULL, NULL);
}

void test_vector_insert(void)
{
	Vector *vec = Vector_New();
	ASSERT_PTR(vec);
	int res;
	int i;
	for (i = 0; i < 30; i++) {
		res = Vector_Insert(vec, i, strings[i]);
		ASSERT(res >= 0);
	}

	for (i = 0; i < Vector_Size(vec); i++) {
		char *s = Vector_Get(vec, i);
		string_check(s, i);
	}

	ASSERT(Vector_Size(vec) == 30);

	res = Vector_Insert(vec, -1, strings[30]);
	ASSERT(res >= 0);

	ASSERT(Vector_Size(vec) == 31);

	res = Vector_Insert(vec, 10, strings[31]);
	ASSERT(res >= 0);

	ASSERT(Vector_Size(vec) == 32);

	res = Vector_Insert(vec, 20, strings[32]);
	ASSERT(res >= 0);

	ASSERT(Vector_Size(vec) == 33);

	for (i = 0; i < Vector_Size(vec); i++) {
		char *s = Vector_Get(vec, i);
		if (i == 0) string_check(s, 30);
		else if (i == 10) string_check(s, 31);
		else if (i == 20) string_check(s, 32);
		else {
			int j = i - 1;
			if (i > 10) j--;
			if (i > 20) j--;
			string_check(s, j);
		}
	}

	Vector_Free(vec, NULL, NULL);
}

void test_vector_concat(void)
{
	Vector *vec = Vector_New();
	ASSERT_PTR(vec);
	int res;
	int i;
	for (i = 0; i < 30; i++) {
		res = Vector_Insert(vec, i, strings[i]);
		ASSERT(res >= 0);
	}

	for (i = 0; i < Vector_Size(vec); i++) {
		char *s = Vector_Get(vec, i);
		string_check(s, i);
	}

	Vector *vec2 = Vector_New();
	ASSERT_PTR(vec2);
	for (i = 30; i < 40; i++) {
		res = Vector_Insert(vec2, i, strings[i]);
		ASSERT(res >= 0);
	}

	Vector_Concat(vec, vec2);

	for (i = 0; i < Vector_Size(vec); i++) {
		char *s = Vector_Get(vec, i);
		string_check(s, i);
	}

	Vector_Free(vec, NULL, NULL);
	Vector_Free(vec2, NULL, NULL);
}

void init_strings(void)
{
	srand(time(NULL));
	for (int i = 0; i < 40; i++) {
		strings[i] = mem[i];
		random_string(strings[i], 9);
	}
}

int main(int argc, char *argv[])
{
	UNUSED_PARAMETER(argc);
	UNUSED_PARAMETER(argv);
	init_strings();
	test_vector_append();
	test_vector_insert();
	test_vector_concat();
	return 0;
}
