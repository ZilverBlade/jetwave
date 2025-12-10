#include "PathTracer.hpp"
#include <src/Graphics/Lights/PointLight.hpp>
#include <src/Graphics/Shapes/Sphere.hpp>

namespace devs_out_of_bounds {
class DefaultMaterial : public IMaterial {
public:
    DOB_NODISCARD MaterialOutput Evaluate(const MaterialInput& input) const override {
        return {
            .albedo_color = { 1.0f, 0.0f, 0.0f },
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

void PathTracer::OnUpdate(float frame_time) {
    static float accum = 0.0f;

    {
        PointLight* light = static_cast<PointLight*>(m_scene->GetActorById(m_light_actor).GetLight());
        light->m_position = glm::vec3(4.0f * glm::cos(accum), 2.0f, 4.0f * glm::sin(accum));
    }
    accum += frame_time;
}

Pixel PathTracer::Evaluate(int x, int y) const {
    glm::vec2 ndc = glm::vec2(x, y) * m_inv_width_height * 2.0f - 1.0f;
    ndc.x *= m_ar;

    glm::vec3 o = m_camera.GetPosition();
    glm::vec3 r = m_camera.GetRay(ndc);

    glm::vec3 color_accumulated = {};
    m_scene->QueryScene([&color_accumulated, o, r, this](const Actor& actor) {
        if (!actor.GetShape() || !actor.GetMaterial()) {
            return;
        }
        Intersection intersection;
        if (!actor.GetShape()->Intersect(o, r, &intersection)) {
            return;
        }
        const glm::vec3 P = intersection.t * r + o;
        const glm::vec3 N = intersection.normal;
        const glm::vec3 V = o - P;

        MaterialOutput material;
        {
            const MaterialInput input{
                .position = P,
                .normal = N,
                .uv = { 0, 0 },
            };
            material = actor.GetMaterial()->Evaluate(input);
        }

        const LightInput light_input{
            .eye = o,
            .P = P,
            .V = V,
            .N = N,
            .specular_power = material.specular_power,
        };

        glm::vec3 diffuse = {};
        glm::vec3 specular = {};

        m_scene->QueryScene([&](const Actor& actor) {
            if (!actor.GetLight()) {
                return;
            }
            LightOutput light = actor.GetLight()->Evaluate(light_input);
            diffuse += material.albedo_color * light.diffuse;
            specular += material.specular_color * light.specular;
        });
        color_accumulated = diffuse + specular;
    });

    const glm::vec3 tone_mapped = 1.0f - glm::exp(-color_accumulated);
    return DOB_WRITE_PIXEL_F32(tone_mapped.r, tone_mapped.g, tone_mapped.b, 1.0f);
}
void PathTracer::LoadScene() {
    static Sphere sphere1 = Sphere({ 0, 0, 5 }, 1.0f);
    m_sphere_actor = m_scene->NewDrawableActor(&sphere1, &g_default_mat);

    static PointLight light1 = PointLight({ 0, 2, 0 }, { 4.f, 8.f, 8.f });
    m_light_actor = m_scene->NewLightActor(&light1);
}
void PathTracer::Cleanup() {}
} // namespace devs_out_of_bounds