call emcc src/plugin.c -o build/plugin.wasm -sSIDE_MODULE -sEXPORTED_RUNTIME_METHODS=cwrap -I src/openmfx
call emcc src/plugin2.c -o build/plugin2.wasm -sSIDE_MODULE
call emcc src/main.c -o build/index.html -sMAIN_MODULE=2 -sEXPORTED_RUNTIME_METHODS=cwrap,FS --shell-file src/html_templates/index.html -I src/openmfx
call emcc src/empty.c -o build/main.html -sMAIN_MODULE=2 -sEXPORTED_RUNTIME_METHODS=cwrap

REM copy src\test.html build\test.html
