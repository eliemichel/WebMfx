#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#else
#define EMSCRIPTEN_KEEPALIVE
#endif

#include <string.h>

EMSCRIPTEN_KEEPALIVE
int is_valid(const char *key) {
	if (0 == strcmp(key, "foo")) {
		return 1;
	} else {
		return 0;
	}
}
