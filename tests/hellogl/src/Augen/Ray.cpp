#include "Ray.h"

using glm::vec3;
using glm::vec4;
using glm::mat3;

Ray RayUtils::transform(const Ray& ray, const glm::mat4& matrix) {
	return Ray(
		matrix * vec4(ray.origin, 1.0f),
		mat3(matrix) * ray.direction
	);
}

bool RayUtils::intersectPlane(
	const Ray& ray,
	const glm::vec3& planeNormal,
	float planeOrigin,
	glm::vec3& hitPoint,
	float& hitLambda
) {
	float denom = dot(ray.direction, planeNormal);
	if (std::abs(denom) < 1e-6) return false;
	hitLambda = (planeOrigin - dot(ray.origin, planeNormal)) / denom;
	hitPoint = ray.origin + hitLambda * ray.direction;
	return hitLambda >= 0;
}

float RayUtils::intersectLine(
	const Ray& ray,
	const glm::vec3& a,
	const glm::vec3& b,
	float& hitLambda,
	float& hitGamma
) {
	Ray line(a, b - a);
	vec3 n = cross(ray.direction, line.direction);
	float ln2 = dot(n, n);
	float ln = std::sqrt(ln2);
	hitLambda = dot(cross(line.origin - ray.origin, line.direction), n) / ln2;
	hitGamma = dot(cross(line.origin - ray.origin, ray.direction), n) / ln2;
	float distance = abs(dot(ray.origin - line.origin, n) / ln);
	return distance;
}
