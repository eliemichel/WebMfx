#pragma once

#include <OpenGL>

#include "ShaderProgram.h"
#include "ShaderPool.h"

#include <limits>
#include <memory>

class Camera;

struct DrawCall {
	GLsizei elementCount = 0;
	GLuint vao = std::numeric_limits<GLuint>::max();
	GLuint vbo = std::numeric_limits<GLuint>::max();
	GLuint ebo = std::numeric_limits<GLuint>::max();
	std::shared_ptr<ShaderProgram> shader;
	GLenum topology = GL_TRIANGLES;
	bool indexed = true;
	GLenum elementType = GL_UNSIGNED_INT;
	bool restart = false;
	GLuint restartIndex = 0;

	DrawCall(
		const std::string& shaderName,
		GLenum topology = GL_TRIANGLES,
		bool indexed = true,
		GLenum elementType = GL_UNSIGNED_INT
	);
	~DrawCall();
	DrawCall(const DrawCall&) = delete;
	DrawCall& operator=(const DrawCall&) = delete;

	/**
	 * Destroy OpenGL objects if the draw call is valid (non null element count)
	 */
	void destroy();

	void render(const Camera& camera, GLint first = 0, GLsizei size = 0, GLsizei instanceCount = 1) const;
	void render(GLint first = 0, GLsizei size = 0, GLsizei instanceCount = 1) const;

	// Functions that populate the draw call
	// (caller must then manually call destroy() to clean up gl objects)
	void loadFromVectors(const std::vector<GLfloat>& pointData, bool dynamic = false);
	void loadFromVectors(const std::vector<GLfloat>& pointData, const std::vector<GLuint> & elementData, bool dynamic = false);
	void loadFromVectorsWithNormal(const std::vector<GLfloat>& pointData, const std::vector<GLfloat>& normalData, bool dynamic = false);
	void loadFromVectorsWithNormal(const std::vector<GLfloat>& pointData, const std::vector<GLfloat>& normalData, const std::vector<GLuint>& elementData, bool dynamic = false);
	void loadFromVectorsWithNormalAndUVs(const std::vector<GLfloat>& pointData, const std::vector<GLfloat>& normalData, const std::vector<GLfloat>& uvData, bool dynamic = false);
	void loadFromVectorsWithNormalAndUVs(const std::vector<GLfloat>& pointData, const std::vector<GLfloat>& normalData, const std::vector<GLfloat>& uvData, const std::vector<GLuint>& elementData, bool dynamic = false);
	void loadQuad();

	/**
	 * Check that the element buffer is consistent with the vbo size
	 */
	bool checkIntegrity(GLuint vertexCount) const;
};
