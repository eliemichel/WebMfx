# Copyright (c) 2017 - 2020 -- Élie Michel <elie.michel@exppad.com>
# This is private research work, DO NOT SHARE without the explicit
# consent of the authors.

set(SRC
	utils/debug.h
	utils/debug.cpp
	utils/fileutils.h
	utils/fileutils.cpp
	utils/guiutils.h
	utils/guiutils.cpp
	utils/reflutils.h
	utils/reflutils.cpp
	utils/strutils.h
	utils/strutils.cpp
	utils/streamutils.h
	
	Ui/BaseApp.h
	Ui/BaseApp.cpp
	Ui/Dialog.h
	Ui/Window.h
	Ui/Window.cpp

	Camera.h
	Camera.cpp
	ColorLayerInfo.h
	DrawCall.h
	DrawCall.cpp
	Framebuffer.h
	Framebuffer.cpp
	GlobalTimer.h
	GlobalTimer.cpp
	GroupObject.h
	GroupObject.cpp
	Logger.h
	Logger.cpp
	NonCopyable.h
	Ray.h
	Ray.cpp
	ReflectionAttributes.h
	RenderTarget.h
	RenderTarget.cpp
	RuntimeObject.h
	ScopedFramebufferOverride.h
	Shader.h
	Shader.cpp
	ShaderPool.h
	ShaderPool.cpp
	ShaderPreprocessor.h
	ShaderPreprocessor.cpp
	ShaderProgram.h
	ShaderProgram.cpp
	Texture.h
	Texture.cpp
	TinyTimer.h
	TurntableCamera.h
	TurntableCamera.cpp
)

set(LIBS
	modernglad
	refl-cpp
	glfw
	glm
	imgui
)

if(CMAKE_COMPILER_IS_GNUCC)
	list(APPEND LIBS stdc++fs)
endif(CMAKE_COMPILER_IS_GNUCC)

add_library(Augen STATIC ${SRC})
target_include_directories(Augen PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Augen LINK_PUBLIC ${LIBS})

target_treat_warnings_as_errors(Augen)
if (MSVC)
	target_compile_options(Augen PUBLIC /wd4127) # forces using "if constexpr"
endif (MSVC)

target_compile_definitions(Augen PUBLIC -DNOMINMAX)
if (DEV_MOD)
	target_compile_definitions(Augen PRIVATE -DSHARE_DIR="${PROJECT_SOURCE_DIR}/share")
else (DEV_MOD)
	target_compile_definitions(Augen PRIVATE -DSHARE_DIR="./share")
endif (DEV_MOD)

# For IDE projects
group_source_by_folder(${SRC})
