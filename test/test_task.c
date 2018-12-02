
#include "task.h"
#include <stdio.h>
#include <unistd.h>

void *task_entry_func(void *p)
{
	int* value = (int*)p;
	*value += 1;
	task_t *curr = current_task();
	printf("[%d-%lu]: %d\n", current_processor()->id, curr->id, *value);
	task_yield();
	*value += 1;
	curr = current_task();
	printf("[%d-%lu]: %d\n", current_processor()->id, curr->id, *value);
	return NULL;
}

int main(int argc, char *argv[])
{
	init_processors(2);
	int value1 = 100;
	int value2 = 200;
	int value3 = 300;
	task_attr_t attr = {.stacksize = 1 * 1024 * 1024};
	task_create(&attr, task_entry_func, &value1);
	task_create(&attr, task_entry_func, &value2);
	task_create(&attr, task_entry_func, &value3);
	task_yield();
	//task_yield();
	int count = 0;
	while (count++ < 5) {
		sleep(1);
		task_t *curr = current_task();
		printf("main processor...[%d, %lu]\n", current_processor()->id, curr->id);
	}
	return 0;
}
