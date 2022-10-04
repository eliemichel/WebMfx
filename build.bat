call emcc src/plugin.c ^
	-sSIDE_MODULE ^
	-sEXPORTED_RUNTIME_METHODS=cwrap ^
	-I src/openmfx ^
	-g -gsource-map ^
	--source-map-base http://127.0.0.1:8888/build/ ^
	-o build/plugin.wasm

REM call emcc src/plugin2.c ^
	-sSIDE_MODULE ^
	-o build/plugin2.wasm

call emcc src/main.c ^
	src/openmfx-sdk/c/common/common.c ^
	-sMAIN_MODULE ^
	-sEXPORTED_RUNTIME_METHODS=cwrap,FS ^
	-sMIN_WEBGL_VERSION=2 ^
	-sMAX_WEBGL_VERSION=2 ^
	--shell-file src/html_templates/index.html ^
	-I src/openmfx ^
	-g -gsource-map ^
	--source-map-base http://127.0.0.1:8888/build/ ^
	-o build/index.html

REM call emcc src/empty.c ^
	-sMAIN_MODULE=2 ^
	-sEXPORTED_RUNTIME_METHODS=cwrap ^
	-o build/main.html

REM copy src\test.html build\test.html
REM mklink build\js js
