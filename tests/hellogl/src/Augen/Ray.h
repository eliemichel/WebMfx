#pragma once

#include <glm/glm.hpp>

namespace RayUtils {

struct Ray {
	Ray() {}
	Ray(const glm::vec3& origin, const glm::vec3& direction)
		: origin(origin)
		, direction(direction)
	{}

	glm::vec3 origin = glm::vec3(0);
	glm::vec3 direction = glm::vec3(0, 0, 1);
};

Ray transform(const Ray& ray, const glm::mat4& matrix);

bool intersectPlane(
	const Ray& ray,
	const glm::vec3& planeNormal,
	float planeOrigin,
	glm::vec3& hitPoint,
	float& hitLambda
);

// return distance to line
float intersectLine(
	const Ray& ray,
	const glm::vec3& a,
	const glm::vec3& b,
	float& hitLambda,
	float& hitGamma
);

}

using Ray = RayUtils::Ray;