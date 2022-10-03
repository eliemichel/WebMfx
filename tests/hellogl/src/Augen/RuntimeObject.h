#pragma once

class Camera;

class RuntimeObject {
public:
	virtual void update() {}
	virtual void render(const Camera& camera) const = 0;
};
