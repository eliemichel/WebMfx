#include <stdio.h>
#include <dlfcn.h>
#include <assert.h>

const char *PLUGIN_PATH = "build/plugin.wasm";

int main(int argc, char** argv) {
	printf("Checking plugin file\n");
	FILE *file = fopen(PLUGIN_PATH, "rb");
	if (!file) {
		printf("cannot open file\n");
		return 1;
	} else {
		printf("can open file\n");
	}
	fclose(file);

	printf("Loading plugin\n");
	void* handle = dlopen(PLUGIN_PATH, RTLD_LAZY);
	if (NULL == handle) {
		printf("Error while loading plugin: %s\n", dlerror());
		return 1;
	}

	int (*is_valid)(const char*) = dlsym(handle, "is_valid");
	if (NULL == is_valid) {
		printf("Error while loading symbol 'is_valid': %s\n", dlerror());
		return 1;
	}

	int is_foo_valid = is_valid("foo");
	printf("is_foo_valid? %d\n", is_foo_valid);
	assert(is_foo_valid);

	int is_bar_valid = is_valid("bar");
	printf("is_bar_valid? %d\n", is_bar_valid);
	assert(!is_bar_valid);

	int status = dlclose(handle);
	if (status != 0) {
		printf("Error while closing plugin: %s (status=%d)\n", dlerror(), status);
		return 1;
	}

	printf("Test passed.\n");

	return 0;
}
