call emcc -c src/plugin.c ^
	-sSIDE_MODULE ^
	-I src/openmfx ^
	-g -gsource-map ^
	--source-map-base http://127.0.0.1:8888/build/ ^
	-o build/plugin.c.o
call emcc -c src/openmfx-sdk/c/common/common.c ^
	-sSIDE_MODULE ^
	-I src/openmfx ^
	-g -gsource-map ^
	--source-map-base http://127.0.0.1:8888/build/ ^
	-o build/common.c.o
call emcc build/plugin.c.o build/common.c.o ^
	-sSIDE_MODULE ^
	-sEXPORTED_RUNTIME_METHODS=cwrap ^
	-sEXPORTED_FUNCTIONS=OfxGetNumberOfPlugins,OfxGetPlugin ^
	-o build/plugin.wasm

EXIT /B

REM call emcc src/plugin2.c ^
REM	-sSIDE_MODULE ^
REM	-o build/plugin2.wasm

call %EMSDK%/upstream/emscripten/tools/webidl_binder ^
	src/binding.idl ^
	build/bindings/binding

call emcc src/webmfx.cpp ^
	src/openmfx-sdk/c/common/common.c ^
	src/openmfx-sdk/c/host/types.c ^
	src/openmfx-sdk/c/host/meshEffectSuite.c ^
	src/openmfx-sdk/c/host/propertySuite.c ^
	src/openmfx-sdk/c/host/parameterSuite.c ^
	src/openmfx-sdk/c/host/host.c ^
	-sMAIN_MODULE ^
	-sEXPORTED_RUNTIME_METHODS=cwrap,FS ^
	-sMIN_WEBGL_VERSION=2 ^
	-sMAX_WEBGL_VERSION=2 ^
	--shell-file src/html_templates/index.html ^
	-I src/openmfx ^
	-I src/openmfx-sdk/c ^
	-I build/bindings ^
	--post-js build/bindings/binding.js ^
	-g -gsource-map ^
	--source-map-base http://127.0.0.1:8888/build/ ^
	-o build/index.html

REM call emcc src/empty.c ^
REM	-sMAIN_MODULE=2 ^
REM	-sEXPORTED_RUNTIME_METHODS=cwrap ^
REM	-o build/main.html

REM copy src\test.html build\test.html
REM mklink build\js js
