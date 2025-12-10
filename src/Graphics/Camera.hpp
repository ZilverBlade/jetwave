#pragma once
#include <glm/glm.hpp>
#include <src/Core.hpp>
#include <src/Graphics/Ray.hpp>

namespace devs_out_of_bounds {
class Camera {
public:
    Camera() {}
    ~Camera() {}

    DOOB_FORCEINLINE void LookDir(const glm::vec3& ray, float fov_degrees, const glm::vec3& up = { 0, 1, 0 }) {
        assert(glm::distance(glm::normalize(ray), ray) < std::numeric_limits<float>::epsilon() &&
               "Ray should be normalised!");

        float camera_fov = glm::radians(fov_degrees);
        m_focal_length = 1.0f / glm::tan(camera_fov * 0.5f);

        m_forward = ray;
        m_right = glm::cross(m_forward, up);
        m_up = glm::cross(m_right, m_forward);
    }
    DOOB_FORCEINLINE void LookAt(
        const glm::vec3& origin, const glm::vec3& target, float fov_degrees, const glm::vec3& up = { 0, 1, 0 }) {
        assert(glm::distance(target, origin) > std::numeric_limits<float>::epsilon() &&
               "Target must not coincide with origin!");
        LookDir(glm::normalize(target - origin), fov_degrees, up);
    }

    DOOB_NODISCARD DOOB_FORCEINLINE Ray GetRay(glm::vec2 ndc) const {
        return {
            .origin = m_position,
            .direction = glm::normalize((m_right * ndc.x + m_up * ndc.y) + m_forward * m_focal_length),
        };
    }

    DOOB_NODISCARD DOOB_FORCEINLINE glm::vec3 GetPosition() const { return m_position; }
    DOOB_FORCEINLINE void SetPosition(const glm::vec3& position) { m_position = position; }

private:
    glm::vec3 m_position = { 0, 0, 0 };

    glm::vec3 m_forward = { 0, 0, 1 };
    glm::vec3 m_right = { 1, 0, 0 };
    glm::vec3 m_up = { 0, 1, 0 };

    float m_focal_length = 1.f;
};
} // namespace devs_out_of_bounds