
#include "properties.h"

void test_properties(void)
{
	Properties properties;
	Properties_Init(&properties);
	Properties_Put(&properties, "foo", "foo1");
	char *v = Properties_Get(&properties, "foo");
	assert(!strcmp("foo1", v));
	Properties_Put(&properties, "foo", "foo2");
	Properties_Put(&properties, "bar", "bar1");
	v = Properties_Get(&properties, "bar");
	assert(!strcmp("bar1", v));
	struct list_head *list = Properties_Get_List(&properties, "foo");
	assert(list);
	struct list_head *pos;
	Property *property;
	int count = 0;
	list_for_each(pos, list) {
		property = container_of(pos, Property, link);
		v = Property_Value(property);
		if (count == 0) {
			assert(!strcmp("foo1", v));
		} else if (count == 1) {
			assert(!strcmp("foo2", v));
		} else {
			assert(0);
		}
		count++;
	}
	assert(count == 2);
	Properties_Fini(&properties);
}

int main(int argc, char *argv[])
{
	AtomString_Init();
	test_properties();
	AtomString_Fini();
	return 0;
}
