#include "GroupObject.h"

void GroupObject::update() {
	for (auto& obj : m_objects) {
		obj->update();
	}
}

void GroupObject::render(const Camera& camera) const {
	for (const auto& obj : m_objects) {
		obj->render(camera);
	}
}

void GroupObject::add(std::shared_ptr<RuntimeObject> object) {
	m_objects.push_back(object);
}
