
call emcc ^
	-I src/third_party/glm ^
	^
	-I src/third_party/refl-cpp/include ^
	^
	src/third_party/imgui/imgui.cpp ^
	src/third_party/imgui/imgui_draw.cpp ^
	src/third_party/imgui/imgui_tables.cpp ^
	src/third_party/imgui/imgui_widgets.cpp ^
	src/third_party/imgui/backends/imgui_impl_glfw.cpp ^
	src/third_party/imgui/backends/imgui_impl_opengl3.cpp ^
	-I src/third_party/imgui ^
	-I src/third_party/imgui/backends ^
	^
	src/Augen/utils/debug.cpp ^
	src/Augen/utils/fileutils.cpp ^
	src/Augen/utils/guiutils.cpp ^
	src/Augen/utils/reflutils.cpp ^
	src/Augen/utils/strutils.cpp ^
	src/Augen/Ui/BaseApp.cpp ^
	src/Augen/Ui/Window.cpp ^
	src/Augen/Camera.cpp ^
	src/Augen/DrawCall.cpp ^
	src/Augen/Framebuffer.cpp ^
	src/Augen/GlobalTimer.cpp ^
	src/Augen/GroupObject.cpp ^
	src/Augen/Logger.cpp ^
	src/Augen/Ray.cpp ^
	src/Augen/RenderTarget.cpp ^
	src/Augen/Shader.cpp ^
	src/Augen/ShaderPool.cpp ^
	src/Augen/ShaderPreprocessor.cpp ^
	src/Augen/ShaderProgram.cpp ^
	src/Augen/Texture.cpp ^
	src/Augen/TurntableCamera.cpp ^
	-I src/Augen ^
	^
	src/Ui/App.cpp ^
	src/main.cpp ^
	^
	--shell-file ../../src/html_templates/index.html ^
	-o build/index.html
