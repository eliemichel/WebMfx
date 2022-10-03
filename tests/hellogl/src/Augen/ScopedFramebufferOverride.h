/**
 * This file is part of GrainViewer
 *
 * Copyright (c) 2017 - 2020 -- Télécom Paris (Élie Michel <elie.michel@telecom-paris.fr>)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the “Software”), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * The Software is provided “as is”, without warranty of any kind, express or
 * implied, including but not limited to the warranties of merchantability,
 * fitness for a particular purpose and non-infringement. In no event shall the
 * authors or copyright holders be liable for any claim, damages or other
 * liability, whether in an action of contract, tort or otherwise, arising
 * from, out of or in connection with the software or the use or other dealings
 * in the Software.
 */

#pragma once

#include <OpenGL>

/**
 * Restore framebuffer bindings upon destruction
 * (also save viewport)
 */
class ScopedFramebufferOverride {
public:
	ScopedFramebufferOverride() {
		glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &m_drawFboId);
		glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &m_readFboId);
		glGetIntegerv(GL_VIEWPORT, m_viewport);
	}
	~ScopedFramebufferOverride() {
		restore();
	}
	void restore() {
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_drawFboId);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, m_readFboId);
		glViewport(m_viewport[0], m_viewport[1], m_viewport[2], m_viewport[3]);
	}

private:
	GLint m_drawFboId = 0;
	GLint m_readFboId = 0;
	GLint m_viewport[4];
};