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

#include "RenderTarget.h"
#include "Logger.h"
#include "Texture.h"

#include <cmath>
#include <algorithm>

RenderTarget::RenderTarget(GLsizei width, GLsizei height, const std::vector<ColorLayerInfo> & colorLayerInfos)
	: m_width(width)
	, m_height(height)
	, m_colorLayerInfos(colorLayerInfos)
{
	init();
}

void RenderTarget::init() {
	m_framebuffer.bind();

	m_colorTextures.clear();
	m_colorTextures.resize(m_colorLayerInfos.size());
	for (size_t k = 0; k < m_colorLayerInfos.size(); ++k) {
		m_colorTextures[k] = std::make_shared<Texture>(GL_TEXTURE_2D);
		m_colorTextures[k]->storage(1, m_colorLayerInfos[k].format, m_width, m_height);
		m_framebuffer.attachTexture(m_colorLayerInfos[k].attachement, *m_colorTextures[k], 0);
	}

	m_depthTexture = std::make_shared<Texture>(GL_TEXTURE_2D);
	m_depthTexture->storage(1, GL_DEPTH_COMPONENT24, m_width, m_height);
	m_framebuffer.attachDepthTexture(*m_depthTexture, 0);

	if (m_colorLayerInfos.empty()) {
		deactivateColorAttachments();
	}
	else {
		activateColorAttachments();
	}

	if (!m_framebuffer.check()) {
		ERR_LOG << "RenderTarget not complete!";
	}
}

void RenderTarget::bind() const {
	m_framebuffer.bind();
}

void RenderTarget::setResolution(GLsizei width, GLsizei height)
{
	if (width == m_width && height == m_height) return;
	width = std::min(std::max((GLsizei)1, width), (GLsizei)4096);
	height = std::min(std::max((GLsizei)1, height), (GLsizei)4096);
	DEBUG_LOG << "Resizing RenderTarget to (" << width << "x" << height << ")";
	m_width = width;
	m_height = height;
	init();
}

void RenderTarget::deactivateColorAttachments()
{
	m_framebuffer.disableDrawBuffers();
}

void RenderTarget::activateColorAttachments()
{
	std::vector<GLenum> drawBuffers(m_colorLayerInfos.size());
	for (size_t k = 0; k < m_colorLayerInfos.size(); ++k) {
		drawBuffers[k] = m_colorLayerInfos[k].attachement;
	}
	glNamedFramebufferDrawBuffers(m_framebuffer.raw(), static_cast<GLsizei>(drawBuffers.size()), &drawBuffers[0]);
}
