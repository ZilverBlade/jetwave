#include <src/Graphics/ILight.hpp>

namespace devs_out_of_bounds {
class PointLight : public ILight {
public:
    PointLight(const glm::vec3& center, const glm::vec3& intensity) : m_center(center), m_intensity(intensity) {}

    DOB_NODISCARD LightOutput Evaluate(const LightInput& input) const override {

        const glm::vec3 fragToLight = input.camera_position - input.fragment_position;
        const float attenuation = 1.0f / glm::dot(fragToLight, fragToLight);

        const glm::vec3 L = glm::normalize(fragToLight);
        const glm::vec3 H = glm::normalize(L + V);
        const float NdH = glm::max(glm::dot(N, H), 0.0f);

        const float spec = material.specular_factor * glm::pow(NdH, material.specular_power);

    }

private:
    glm::vec3 m_position ;
    glm::vec3 m_intensity;
};
} // namespace devs_out_of_bounds