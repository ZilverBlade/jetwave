#pragma once
#include <src/Graphics/IShape.hpp>

namespace devs_out_of_bounds {
namespace shape {
    class Sphere : public IShape {
    public:
        Sphere(const glm::vec3& center, float radius) : m_center(center), m_radius2(radius * radius) {}

        DOOB_NODISCARD bool Intersect(const Ray& ray, Intersection* out_intersection) const override {
            const glm::vec3 oc = ray.origin - m_center;

            // const float a = glm::dot(ray.direction, ray.direction);
            const float b = 2.0f * glm::dot(ray.direction, oc);
            const float c = glm::dot(oc, oc) - m_radius2;

            const float discriminant = b * b - 4.0f * c;

            if (discriminant < 0.0f) {
                return false;
            }
            float sqrt_disc = glm::sqrt(discriminant);
            float t = (-b - sqrt_disc) * 0.5f;
            bool b_front_facing = true;

            // If t0 is invalid (behind us or too far), try t1 (the exit point, backface)
            if (t > ray.t_max) {
                return false;
            } else if (t < ray.t_min) {
                t = (-b + sqrt_disc) * 0.5f;
                if (t < ray.t_min || t > ray.t_max) {
                    t = (-b + sqrt_disc) * 0.5f;
                    return false;
                }
                b_front_facing = false;
            }

            if (out_intersection) {
                glm::vec3 target = ray.direction * t;

                out_intersection->num_intersections = 1;
                out_intersection->position = ray.origin + target;
                out_intersection->t = t;
                out_intersection->barycentric = { 0.0f, 0.0f }; // Spheres don't use barycentrics
                out_intersection->primitive = 0;
                out_intersection->flat_normal = glm::normalize(oc + target) * (b_front_facing ? 1.f : -1.f);
                out_intersection->b_front_facing = b_front_facing;
            }

            return true;
        }
        DOOB_NODISCARD AABB GetAABB() const override {
            float r = glm::sqrt(m_radius2);
            return AABB{
                .min = m_center - r,
                .max = m_center + r,
            };
        }
        DOOB_NODISCARD Fragment SampleFragment(const Intersection& intersection) const override {
            return {
                .position = intersection.position,
                .normal = intersection.flat_normal,
                .tangent = {},
                .uv = {},
            };
        }


        glm::vec3 m_center;
        float m_radius2;
    };
} // namespace shape
} // namespace devs_out_of_bounds