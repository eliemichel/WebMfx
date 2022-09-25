call emcc plugin.c ^
	-sSIDE_MODULE ^
	-sEXPORTED_RUNTIME_METHODS=cwrap ^
	-o build/plugin.wasm

call emcc main.c ^
	-sMAIN_MODULE ^
	--preload-file build/plugin.wasm ^
	-o build/index.html

call emcc main.c ^
	-sMAIN_MODULE=2 ^
	--preload-file build/plugin.wasm ^
	-g -gsource-map ^
	--source-map-base http://127.0.0.1:8888/build/ ^
	-o build/index_lite.html
