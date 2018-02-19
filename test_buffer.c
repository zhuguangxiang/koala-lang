
#include <time.h>
#include "buffer.h"

static int random_string(uint8 *data, int len) {
	static const uint8 char_set[] = "0123456789abcdefghijklmnopqrstuvwxyz" \
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

void test_buffer(void)
{
	Buffer buf;
	Buffer_Init(&buf, 16);
	srand(time(NULL));
	static uint8 saved_data[100];

	int i = 0;
	uint8 data[10];
	while (i < 10) {
		random_string(data, 10);
		memcpy(saved_data + i * 10, data, 10);
		Buffer_Write(&buf, data, 10);
		i++;
	}
	assert(buf.blocks == 7);
	assert(buf.size == 100);

	Block *block;
	int all = 0;
	list_for_each_entry(block, &buf.head, link) {
		all += block->used;
	}
	assert(all == 100);

	struct list_head *last = list_last(&buf.head);
	block = container_of(last, Block, link);
	assert(block->used == (100 % 16));

	uint8 *d = Buffer_RawData(&buf);
	int diff = memcmp(saved_data, d, 100);
	assert(!diff);
	free(d);
	Buffer_Fini(&buf);
}

int main(int argc, char *argv[])
{
	UNUSED_PARAMETER(argc);
	UNUSED_PARAMETER(argv);
	test_buffer();
	return 0;
}
