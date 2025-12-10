#include "PathTracer.hpp"
#include <src/Graphics/Lights/PointLight.hpp>
#include <src/Graphics/Materials/BasicMaterial.hpp>
#include <src/Graphics/Shapes/Sphere.hpp>

#include <random>

namespace devs_out_of_bounds {
static BasicMaterial g_default_mat = BasicMaterial({ 1.0f, 0.0f, 0.0f }, { 1, 1, 1 }, 64.0f);

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

    const glm::vec3 o = m_camera.GetPosition();
    const glm::vec3 r = m_camera.GetRay(ndc);

    Intersection intersection{ .t = INFINITY };
    IMaterial* current_material = nullptr;
    m_scene->QueryScene([&](const Actor& actor) {
        if (!actor.GetShape() || !actor.GetMaterial()) {
            return;
        }
        Intersection curr_intersection;
        if (!actor.GetShape()->Intersect(o, r, &curr_intersection)) {
            return;
        }
        if (curr_intersection.t < intersection.t) {
            intersection = curr_intersection;
            current_material = actor.GetMaterial();
        }
    });
    if (!current_material) {
        // No intersection, background colour
        return DOB_WRITE_PIXEL(205, 245, 255, 255);
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
        material = current_material->Evaluate(input);
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
    const glm::vec3 color_accumulated = diffuse + specular;
    const glm::vec3 tone_mapped = 1.0f - glm::exp(-color_accumulated);
    return DOB_WRITE_PIXEL_F32(tone_mapped.r, tone_mapped.g, tone_mapped.b, 1.0f);
}
void PathTracer::LoadScene() {
    static std::vector<std::pair<Sphere, BasicMaterial>> objects;
    objects.clear();
    objects.reserve(100);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> posDist(-10.0f, 10.0f); // Range -10 to 10
    std::uniform_real_distribution<float> radDist(0.5f, 1.5f);    // Radius 0.5 to 1.5
    std::uniform_real_distribution<float> colDist(0.0f, 1.0f);    // Colour 0.0 to 1.0

    for (int i = 0; i < 100; ++i) {
        float x = posDist(gen);
        float y = posDist(gen);
        float z = posDist(gen) + 5.0f; // Offset Z so they are in front of camera
        float radius = radDist(gen);

        float r = colDist(gen);
        float g = colDist(gen);
        float b = colDist(gen);


        objects.emplace_back(Sphere({ x, y, z }, radius), BasicMaterial({ r, g, b }, { 1, 1, 1 }, 64.f));
    }

    for (auto& [shape, material] : objects) {
        m_scene->NewDrawableActor(&shape, &material);
    }

    // --- Light Setup (Kept from your original code) ---
    static PointLight light1 = PointLight({ 0, 10, 0 }, { 4.f, 8.f, 8.f });
    m_light_actor = m_scene->NewLightActor(&light1);
}
void PathTracer::Cleanup() {}
} // namespace devs_out_of_bounds