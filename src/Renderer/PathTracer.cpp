#include "PathTracer.hpp"
#include <src/Graphics/Primitives/Sphere.hpp>
namespace devs_out_of_bounds {
class DefaultMaterial : public IMaterial {
public:
    DOB_NODISCARD MaterialOutput Evaluate(const MaterialInput& input) const override {
        return {
            .albedo_color = { 1.0f, 0.0f, 0.0f },
            .specular_factor = 1.0f,
            .specular_color = { 1.0f, 1.0f, 1.0f },
            .specular_power = 64.0f,
        };
    }

} static g_default_mat;

PathTracer::PathTracer() {
    m_scene = new Scene();
    LoadScene();
}
PathTracer::~PathTracer() {
    Cleanup();
    delete m_scene;
}
void PathTracer::OnResize(int new_width, int new_height) {
    assert(new_width > 0 && new_height > 0 && "Bad width and height!");

    float inv_width = 1.0f / static_cast<float>(new_width);
    float inv_height = 1.0f / static_cast<float>(new_height);
    m_inv_width_height = { inv_width, inv_height };
    m_ar = static_cast<float>(new_width) * inv_height;
}

Pixel PathTracer::Evaluate(int x, int y) const {
    glm::vec2 ndc = glm::vec2(x, y) * m_inv_width_height * 2.0f - 1.0f;
    ndc.x *= m_ar;

    glm::vec3 o = m_camera.GetPosition();
    glm::vec3 r = m_camera.GetRay(ndc);

    glm::vec3 color_accumulated = {};
    m_scene->QueryScene([&color_accumulated, o, r, this](const Actor& actor) {
        Intersection intersection;
        if (!actor.GetShape()->Intersect(o, r, &intersection)) {
            return;
        }
        const glm::vec3 P = intersection.t * r + o;
        const glm::vec3 N = intersection.normal;
        const glm::vec3 V = o - P;

        const MaterialInput input{ .position = P, .normal = N, .uv = { 0, 0 } };

        const MaterialOutput material = actor.GetMaterial()->Evaluate(input);

        static const glm::vec3 LIGHT_POSITION = { 2.0f, 2.0f, 1.0f };
        static const glm::vec3 LIGHT_COLOR = glm::vec3{ 1.0f, 1.0f, 1.0f } * 10.0f;

        const glm::vec3 fragToLight = P - LIGHT_POSITION;
        const float attenuation = 1.0f / glm::dot(fragToLight, fragToLight);

        const glm::vec3 L = glm::normalize(fragToLight);
        const glm::vec3 H = glm::normalize(L + V);
        const float NdH = glm::max(glm::dot(N, H), 0.0f);

        const float spec = material.specular_factor * glm::pow(NdH, material.specular_power);

        glm::vec3 light = attenuation * LIGHT_COLOR;

        glm::vec3 diffuse = {};
        glm::vec3 specular = {};
        diffuse += material.albedo_color * light;
        specular += spec * material.specular_color * light;

        color_accumulated = diffuse + specular;
    });

    const glm::vec3 tone_mapped = 1.0f - glm::exp(-color_accumulated);
    return DOB_WRITE_PIXEL_F32(tone_mapped.r, tone_mapped.g, tone_mapped.b, 1.0f);
}
void PathTracer::LoadScene() {
    static Sphere sphere1 = Sphere({ 0, 0, 5 }, 1.0f);
    ActorId actor1 = m_scene->NewActor(&sphere1, &g_default_mat);

   //static Sphere sphere2 = Sphere({ 0, 2, 5 }, 0.5f);
   //ActorId actor2 = m_scene->NewActor(&sphere1, &g_default_mat);
}
void PathTracer::Cleanup() {}
} // namespace devs_out_of_bounds