#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#else
#define EMSCRIPTEN_KEEPALIVE
#endif

EMSCRIPTEN_KEEPALIVE
int some_function(int x, int(*callback)(int)) {
	return callback(x) + 10;
}

EMSCRIPTEN_KEEPALIVE
int another_function(int x) {
	return x + 10;
}
