#include <src/Graphics/ILight.hpp>

namespace devs_out_of_bounds {
class PointLight : public ILight {
public:
    PointLight(const glm::vec3& position, const glm::vec3& intensity) : m_position(position), m_intensity(intensity) {}

    DOB_NODISCARD LightOutput Evaluate(const LightInput& input) const override {
        const glm::vec3 fragToLight = m_position - input.P;
        const float attenuation = 1.0f / glm::dot(fragToLight, fragToLight);

        const glm::vec3 L = glm::normalize(fragToLight);
        const glm::vec3 H = glm::normalize(L + input.V);
        const float NdH = glm::max(glm::dot(input.N, H), 0.0f);

        const float spec = glm::pow(NdH, input.specular_power);

        glm::vec3 attenuated = attenuation * m_intensity;
        return {
            .diffuse = attenuated,
            .specular = attenuated * spec,
        };
    }

    glm::vec3 m_position;
    glm::vec3 m_intensity;
};
} // namespace devs_out_of_bounds