#include "App.h"
#include "QuadObject.h"
#include "TilesetEditorObject.h"
#include "TilesetController.h"
#include "TilesetRenderer.h"
#include "OutputObject.h"
#include "Serialization.h"
#include "TilingSolver.h"
#include "MacrosurfaceData.h"
#include "MesostructureData.h"
#include "MacrosurfaceRenderer.h"
#include "MesostructureRenderer.h"
#include "MesostructureMvcRenderer.h"
#include "TilingSolverRenderer.h"
#include "MesostructureExporter.h"

#include "ModelDialog.h"
#include "OutputDialog.h"
#include "HelpDialog.h"
#include "TilesetRendererDialog.h"
#include "MacrosurfaceRendererDialog.h"
#include "MesostructureRendererDialog.h"
#include "MacrosurfaceDialog.h"
#include "TilingSolverDialog.h"
#include "TilingSolverRendererDialog.h"
#include "MesostructureExporterDialog.h"
#include "TilesetEditorDialog.h"
#include "GlobalTimerDialog.h"

#include <ReflectionAttributes.h>
#include <Logger.h>
#include <TurntableCamera.h>
#include <Ui/Window.h>
#include <GroupObject.h>
#include <ShaderPool.h>
#include <GlobalTimer.h>

#include <imgui.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/quaternion.hpp>
#include <refl.hpp>
#include <algorithm>

constexpr float PI = 3.141592653589793238462643383279502884f;

#define UNUSED(x) (void)x

App::App(std::shared_ptr<Window> window)
	: BaseApp(window)
{
	m_tileset = std::make_shared<Model::Tileset>();
	m_tilesetController = std::make_shared<TilesetController>();
	m_tilesetController->setModel(m_tileset);

	m_macrosurfaceData = std::make_shared<MacrosurfaceData>();
	m_mesostructureData = std::make_shared<MesostructureData>();
	m_mesostructureData->macrosurface = m_macrosurfaceData;

	m_outputSlots = std::make_shared<OutputObject>(m_macrosurfaceData, m_mesostructureData);
	m_outputSlots->setController(m_tilesetController);
	m_objects.push_back(m_outputSlots);

	m_macrosurfaceRenderer = std::make_shared<MacrosurfaceRenderer>();
	m_macrosurfaceRenderer->setData(m_macrosurfaceData);
	m_objects.push_back(m_macrosurfaceRenderer);

	m_mesostructureRenderer = std::make_shared<MesostructureRenderer>();
	m_mesostructureRenderer->setMesostructureData(m_mesostructureData);
	m_objects.push_back(m_mesostructureRenderer);

	m_tilesetController->setMesostructureData(m_mesostructureData);

	m_tilesetRenderer = std::make_shared<TilesetRenderer>();
	m_tilesetRenderer->setController(m_tilesetController);

	m_tilesetEditor = std::make_shared<TilesetEditorObject>();
	m_tilesetEditor->setController(m_tilesetController);
	m_tilesetEditor->setRenderer(m_tilesetRenderer);
	m_objects.push_back(m_tilesetEditor);

	m_cursorObject = std::make_shared<QuadObject>(0.01f);
	m_cursorObject->properties().color = glm::vec4(0, 0, 0, 1);
	m_objects.push_back(m_cursorObject);

	m_tilingSolver = std::make_shared<TilingSolver>();
	m_tilingSolver->setMesostructureData(m_mesostructureData);
	m_tilingSolver->setController(m_tilesetController);

	m_tilingSolverRenderer = std::make_shared<TilingSolverRenderer>(m_tilingSolver->tilingSolverData(), m_mesostructureData);
	m_objects.push_back(m_tilingSolverRenderer);

	m_mesostructureExporter = std::make_shared<MesostructureExporter>(m_mesostructureData, m_mesostructureRenderer);

	m_mesostructureMvcRenderer = std::make_shared<MesostructureMvcRenderer>(m_mesostructureRenderer);
	m_mesostructureRenderer->setMvcRenderer(m_mesostructureMvcRenderer);

	auto camera = std::make_shared<TurntableCamera>();
	camera->setProjectionType(Camera::ProjectionType::Orthographic);
	camera->setOrientation(glm::angleAxis(PI / 2, glm::vec3(1, 0, 0)));
	camera->setOrthographicScale(1.5f);
	m_camera = camera;

	setupDialogs();
	resize();
}

void App::update() {
	BaseApp::update();

	if (m_tilesetController->hasChangedLately()) {
		m_mesostructureRenderer->onModelChange();
	}

	for (const auto& obj : m_objects) {
		obj->update();
	}

	if (m_tilesetController->hasChangedLately()) {
		m_tilesetRenderer->rebuild();
	}

	m_tilesetController->resetChangedEvent();
}

void App::render() const {
	GlobalTimer::StartFrame();
	glClearColor(1, 1, 1, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	for (const auto& obj : m_objects) {
		obj->render(*m_camera);
	}
	GlobalTimer::StopFrame();

	renderGui();
}

//-----------------------------------------------------------------------------

void App::setupDialogs() {
	clearDialogs();
	addDialogGroup<ModelDialog>("Model", m_tilesetController);
	addDialogGroup<TilesetEditorDialog>("Interface Editor", m_tilesetEditor);
	addDialogGroup<MacrosurfaceDialog>("Macrosurface", m_macrosurfaceData);
	addDialogGroup<TilingSolverDialog>("Generator", m_tilingSolver);
	addDialogGroup<MacrosurfaceRendererDialog>("Macrosurface Renderer", m_macrosurfaceRenderer);
	addDialogGroup<MesostructureRendererDialog>("Mesostructure Renderer", m_mesostructureRenderer);
	//addDialogGroup<TilingSolverRendererDialog>("Generator Debug Renderer", m_tilingSolverRenderer);
	addDialogGroup<MesostructureExporterDialog>("Export", m_mesostructureExporter);
	addDialogGroup<GlobalTimerDialog>("Timing", GlobalTimer::GetInstance());
	addDialogGroup<HelpDialog>("Help", nullptr);
}

void App::onResize(int width, int height) {
	m_camera->setResolution(width, height);
}

void App::onMouseButton(int button, int action, int mods) {
	if (guiHasFocus()) return;

	switch (button) {
	case GLFW_MOUSE_BUTTON_LEFT:
		if ((mods & GLFW_MOD_ALT) != 0) {
			if (action == GLFW_PRESS) {
				m_camera->startMousePanning();
			}
			else if (action == GLFW_RELEASE) {
				m_camera->stopMousePanning();
			}
			break;
		}
		else if ((mods & GLFW_MOD_SHIFT) != 0) {
			if (action == GLFW_PRESS) {
				m_outputSlots->onMousePress(true /* force hover */);
			}
			else if (action == GLFW_RELEASE) {
				m_outputSlots->onMouseRelease();
			}
			break;
		}
		else {
			if (action == GLFW_PRESS) {
				if (!m_outputSlots->onMousePress()) {
					m_tilesetEditor->onMousePress();
				}
			}
			else if (action == GLFW_RELEASE) {
				m_outputSlots->onMouseRelease();
				m_tilesetEditor->onMouseRelease();
			}
		}
		break;
	case GLFW_MOUSE_BUTTON_MIDDLE:
		if (action == GLFW_PRESS) {
			m_camera->startMousePanning();
		}
		else if (action == GLFW_RELEASE) {
			m_camera->stopMousePanning();
		}
		break;
	case GLFW_MOUSE_BUTTON_RIGHT:
		if (action == GLFW_PRESS) {
			m_camera->startMouseZoom();
		}
		else if (action == GLFW_RELEASE) {
			m_camera->stopMouseZoom();
		}
		break;
	}
}

void App::onCursorPosition(double x, double y) {
	if (guiHasFocus()) return;

	glm::vec2 mouseScreenPosition(
		static_cast<float>(x),
		static_cast<float>(y)
	);

	m_camera->updateMousePosition(mouseScreenPosition.x, mouseScreenPosition.y);
	m_outputSlots->onCursorPosition(mouseScreenPosition.x, mouseScreenPosition.y, *m_camera);
	m_tilesetEditor->onCursorPosition(mouseScreenPosition.x, mouseScreenPosition.y, *m_camera);
	
	m_mouseRay = m_camera->mouseRay(mouseScreenPosition);

	if (hitGround(m_mouseRay, m_mousePosition)) {
		m_cursorObject->setPosition(m_mousePosition);

		afterUpdateMousePosition();
	}
}

void App::onScroll(double xoffset, double yoffset) {
	if (guiHasFocus()) return;

	UNUSED(xoffset);

	bool holdingControl = false;
	if (auto window = getWindow().lock()) {
		holdingControl = (
			glfwGetKey(window->glfw(), GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
			glfwGetKey(window->glfw(), GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS
		);
	}

	if (holdingControl) {
		m_tilesetEditor->onMouseScroll(static_cast<float>(yoffset));
	}
	else {
		m_camera->zoom(-2 * static_cast<float>(yoffset));
	}

	afterUpdateMousePosition();
}

void App::onKey(int key, int scancode, int action, int mods) {
	if (action == GLFW_PRESS && key == GLFW_KEY_ESCAPE) {
		if (auto window = getWindow().lock()) {
			glfwSetWindowShouldClose(window->glfw(), GL_TRUE);
		}
	}

	if (guiHasFocus()) return;

	UNUSED(scancode);
	UNUSED(mods);

	if (action == GLFW_PRESS) {
		switch (key) {
		case GLFW_KEY_P:
			setShowPanel(!showPanel());
			break;

		case GLFW_KEY_F5:
			ShaderPool::ReloadShaders();
			break;
		}
	}
}

//-----------------------------------------------------------------------------

void App::afterUpdateMousePosition() {
	//m_tileEditor->onMouseMove(m_mousePosition);
}

//-----------------------------------------------------------------------------

bool App::hitGround(const Ray& ray, glm::vec3& hit) {
	if (std::abs(ray.direction.z) < 1e-5) return false;

	float t = -ray.origin.z / ray.direction.z;
	if (t < 0) return false;

	hit = ray.origin + t * ray.direction;
	return true;
}

void App::saveModel() {
	Serialization::serializeTo(*m_tileset, "tileset.json");
}

void App::loadModel() {
	Serialization::deserializeFrom(*m_tileset, "tileset.json");
	m_tilesetController->setDirty();
}
