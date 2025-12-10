#pragma once
#include <src/Graphics/AABB.hpp>
#include <src/Graphics/Ray.hpp>
namespace devs_out_of_bounds {
struct IShape {
    IShape() = default;
    virtual ~IShape() = default;
    DOOB_NODISCARD virtual bool Intersect(const Ray& ray, Intersection* out_intersection) const = 0;
    DOOB_NODISCARD virtual AABB GetAABB() const = 0;
};
} // namespace devs_out_of_bounds