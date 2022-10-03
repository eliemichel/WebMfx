#pragma once

#include <OpenGL>

struct ColorLayerInfo {
	GLenum format;  // GL_RGBA32F, GL_RGBA32UI...
	GLenum attachement; // GL_COLOR_ATTACHMENT0, ...
};
