#pragma once
#include "RuntimeObject.h"

#include <vector>
#include <memory>

/**
 * A group object is a runtime object that contains other runtime objects
 */
class GroupObject : public RuntimeObject {
public:
	void update() override;
	void render(const Camera& camera) const override;

	void add(std::shared_ptr<RuntimeObject> object);

private:
	std::vector<std::shared_ptr<RuntimeObject>> m_objects;
};
