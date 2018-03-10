
#include "properties.h"

int main(int argc, char *argv[])
{
	UNUSED_PARAMETER(argc);
	UNUSED_PARAMETER(argv);
	Properties prop;
	Properties_Init(&prop);
	Properties_Put(&prop, "path", "~/koala-repo");
	Properties_Put(&prop, "path", "./");
	PropEntry *entry = Properties_Get(&prop, "path");
	assert(entry);
	assert(entry->count == 2);
	int i = 0;
	char *val = Properties_Next(&prop, entry, i);
	while (val) {
		printf("path = %s\n", val);
		i++;
		val = Properties_Next(&prop, entry, i);
	}

	Properties_Put(&prop, "config", "~/koala.properties");
	entry = Properties_Get(&prop, "config");
	assert(entry);
	assert(entry->count == 1);
	i = 0;
	val = Properties_Next(&prop, entry, i);
	while (val) {
		assert(!strcmp(val, "~/koala.properties"));
		printf("config = %s\n", val);
		i++;
		val = Properties_Next(&prop, entry, i);
	}

	entry = Properties_Get(&prop, "koala.debug");
	assert(!entry);

	return 0;
}
