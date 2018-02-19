
#include "list.h"
#include <stdlib.h>
#include <assert.h>

/*
 gcc -g -std=c99 test_list.c
 valgrind ./a.out
 */
struct integer_struct {
	int value;
	struct list_head node;
};

void test_list(void)
{
	int i;
	struct integer_struct *int_ptr;
	LIST_HEAD(integer_list);

	assert(list_empty(&integer_list));

	for (i = 1; i <= 100; i++) {
		int_ptr = malloc(sizeof(*int_ptr));
		init_list_head(&int_ptr->node);
		int_ptr->value = i;

		list_add_tail(&int_ptr->node, &integer_list);
	}

	struct list_head *pos, *nxt;
	i = 1;
	list_for_each_safe(pos, nxt, &integer_list) {
		int_ptr = list_entry(pos, struct integer_struct, node);
		assert(int_ptr->value == i);
		list_del(pos);
		free(int_ptr);
		i++;
	}

	assert(list_empty(&integer_list));

	for (i = 1; i <= 100; i++) {
		int_ptr = malloc(sizeof(*int_ptr));
		init_list_head(&int_ptr->node);
		int_ptr->value = i;

		list_add(&int_ptr->node, &integer_list);
	}

	i = 1;
	list_for_each_prev_safe(pos, nxt, &integer_list) {
		int_ptr = list_entry(pos, struct integer_struct, node);
		assert(int_ptr->value == i);
		list_del(pos);
		free(int_ptr);
		i++;
	}

	assert(list_empty(&integer_list));
}

struct integer_struct2 {
	int value;
	struct hlist_node hnode;
};

void test_hlist(void)
{
	int i;
	struct integer_struct2 *int_ptr;
	HLIST_HEAD(integer_list);

	assert(hlist_empty(&integer_list));

	for (i = 1; i <= 100; i++) {
		int_ptr = malloc(sizeof(*int_ptr));
		init_hlist_node(&int_ptr->hnode);
		int_ptr->value = i;

		assert(hlist_unhashed(&int_ptr->hnode));

		hlist_add_head(&int_ptr->hnode, &integer_list);
	}

	struct hlist_node *pos, *nxt;
	i = 100;
	hlist_for_each_safe(pos, nxt, &integer_list) {
		int_ptr = hlist_entry(pos, struct integer_struct2, hnode);
		assert(int_ptr->value == i);
		assert(!hlist_unhashed(pos));
		hlist_del(pos);
		assert(hlist_unhashed(pos));
		free(int_ptr);
		i--;
	}

	assert(hlist_empty(&integer_list));
}

int main(int argc, char *argv[])
{
	UNUSED_PARAMETER(argc);
	UNUSED_PARAMETER(argv);
	test_list();
	test_hlist();
	return 0;
}
