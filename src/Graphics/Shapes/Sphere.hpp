#include <src/Graphics/IShape.hpp>

namespace devs_out_of_bounds {
class Sphere : public IShape {
public:
    Sphere(const glm::vec3& center, float radius) : m_center(center), m_radius2(radius * radius) {}

    DOB_NODISCARD bool Intersect(
        const glm::vec3& ray_origin, const glm::vec3& ray_direction, Intersection* out_intersection) const override {

        const glm::vec3 origin = ray_origin - m_center;
        // const float A = glm::dot(ray_direction, ray_direction);
        const float B = 2.0f * glm::dot(ray_direction, origin);
        const float C = glm::dot(origin, origin) - m_radius2;

        float discriminant = B * B - 4.0f * C;
        if (discriminant < 0.0f) {
            return false;
        }
        if (out_intersection) {
            const float t0 = (-B - glm::sqrt(discriminant)) * 0.5;
            out_intersection->t = t0;
            out_intersection->normal = glm::normalize(ray_direction * t0 + origin);
            out_intersection->uv = { 0.0f, 0.0f };
        }

        return true;
    }
    DOB_NODISCARD AABB GetAABB() const override {
        float r = glm::sqrt(m_radius2);
        return AABB{
            .min = m_center - r,
            .max = m_center + r,
        };
    }
    DOB_NODISCARD glm::vec3 GetPosition() const override {
        return m_center;
    }

private:
    glm::vec3 m_center;
    float m_radius2;
};
} // namespace devs_out_of_bounds