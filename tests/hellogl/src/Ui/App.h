#pragma once

#include <Ui/BaseApp.h>
#include <Ray.h>

#include <glm/vec2.hpp>

#include <memory>
#include <vector>

class RuntimeObject;
class QuadObject;
class Camera;
class TilesetEditorObject;
class TilesetController;
class TilesetRenderer;
class OutputObject;
class MacrosurfaceData;
class MesostructureData;
class MacrosurfaceRenderer;
class MesostructureRenderer;
class TilingSolver;
class TilingSolverRenderer;
class MesostructureExporter;
class MesostructureMvcRenderer;
namespace Model {
struct Tileset;
}
struct ImFont;

class App : public BaseApp
{
public:
	App(std::shared_ptr<Window> window);

	void update() override;
	void render() const override;

protected:
	void setupDialogs() override;

	void onResize(int width, int height) override;
	void onMouseButton(int button, int action, int mods) override;
	void onKey(int key, int scancode, int action, int mods) override;
	void onCursorPosition(double x, double y) override;
	void onScroll(double xoffset, double yoffset) override;

private:
	void afterUpdateMousePosition();
	void saveModel();
	void loadModel();

private:
	static bool hitGround(const Ray& ray, glm::vec3& hit);

private:
	std::shared_ptr<TilesetEditorObject> m_tilesetEditor;

	std::shared_ptr<QuadObject> m_cursorObject;

	std::vector<std::shared_ptr<RuntimeObject>> m_objects;
	std::shared_ptr<Camera> m_camera;

	glm::vec3 m_mousePosition;
	Ray m_mouseRay;

	std::shared_ptr<TilesetController> m_tilesetController;
	std::shared_ptr<Model::Tileset> m_tileset;

	std::shared_ptr<MacrosurfaceData> m_macrosurfaceData;
	std::shared_ptr<MesostructureData> m_mesostructureData;

	std::shared_ptr<MacrosurfaceRenderer> m_macrosurfaceRenderer;
	std::shared_ptr<MesostructureRenderer> m_mesostructureRenderer;

	std::shared_ptr<OutputObject> m_outputSlots;
	std::shared_ptr<TilesetRenderer> m_tilesetRenderer;

	std::shared_ptr<TilingSolver> m_tilingSolver;
	std::shared_ptr<TilingSolverRenderer> m_tilingSolverRenderer;

	std::shared_ptr<MesostructureExporter> m_mesostructureExporter;

	std::shared_ptr<MesostructureMvcRenderer> m_mesostructureMvcRenderer;
};
