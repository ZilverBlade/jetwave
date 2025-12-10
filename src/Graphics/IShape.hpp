#pragma once
#include <glm/glm.hpp>
#include <src/Core.hpp>
namespace devs_out_of_bounds {
struct AABB {
    glm::vec3 min = {};
    glm::vec3 max = {};
};
struct Intersection {
    float t = {};
    glm::vec3 normal = {};
    glm::vec2 uv = {};
};
struct IShape {
    DOB_NODISCARD virtual bool Intersect(
        const glm::vec3& ray_origin, const glm::vec3& ray_direction, Intersection* out_intersection) const = 0;
    DOB_NODISCARD virtual AABB GetAABB() const = 0;
};
} // namespace devs_out_of_bounds