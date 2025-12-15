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
        m_right = glm::normalize(glm::cross(up, m_forward));
        m_up = glm::normalize(glm::cross(m_forward, m_right));
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
            .t_min = 1e-6f,
            .direction = glm::normalize((m_right * ndc.x + m_up * ndc.y) + m_forward * m_focal_length),
        };
    }

    DOOB_NODISCARD DOOB_FORCEINLINE glm::vec3 GetPosition() const { return m_position; }
    DOOB_FORCEINLINE void SetPosition(const glm::vec3& position) { m_position = position; }

    DOOB_FORCEINLINE void SetSensor(float aperture, float inv_shutter_speed, float iso) {
        m_aperture = aperture;
        m_shutter_speed = 1.0f / inv_shutter_speed;
        m_iso = iso;
    }

    DOOB_FORCEINLINE void GetSensor(float& aperture, float& inv_shutter_speed, float& iso) {
        aperture = m_aperture;
        inv_shutter_speed = 1.0f / m_shutter_speed;
        iso = m_iso;
    }

    DOOB_NODISCARD DOOB_FORCEINLINE float GetLogExposure() { return m_log_exposure; }
    DOOB_FORCEINLINE void SetLogExposure(float log_exposure) { m_log_exposure = log_exposure; }

    DOOB_NODISCARD DOOB_FORCEINLINE float ComputeExposureFactor() const {
        float ev100 = std::log2((m_aperture * m_aperture) / m_shutter_speed);

        ev100 -= std::log2(m_iso / 100.0f);

        float max_luminance = 1.2f * std::exp2(ev100);

        return std::exp(m_log_exposure) / max_luminance;
    }

private:
    glm::vec3 m_position = { 0, 0, 0 };

    glm::vec3 m_forward = { 0, 0, 1 };
    glm::vec3 m_right = { 1, 0, 0 };
    glm::vec3 m_up = { 0, 1, 0 };

    float m_focal_length = 1.f;

    float m_log_exposure = 0.0f;
    float m_aperture = 16.0f;
    float m_shutter_speed = 1.0f / 125.0f;
    float m_iso = 100.0f;
};
} // namespace devs_out_of_bounds