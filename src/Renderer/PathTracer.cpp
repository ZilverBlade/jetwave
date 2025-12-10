#include "PathTracer.hpp"
#include <src/Graphics/Lights/PointLight.hpp>
#include <src/Graphics/Materials/BasicMaterial.hpp>
#include <src/Graphics/Materials/GridMaterial.hpp>
#include <src/Graphics/Shapes/Plane.hpp>
#include <src/Graphics/Shapes/Sphere.hpp>

#include <random>

namespace devs_out_of_bounds {
static BasicMaterial g_default_mat = BasicMaterial({ 1.0f, 0.0f, 0.0f }, { 1, 1, 1 }, 64.0f);
static GridMaterial g_grid_mat = GridMaterial();


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
    ndc.y /= m_ar;
    ndc.y = -ndc.y; // flip Y

    // debug NDC
    // return DOOB_WRITE_PIXEL_F32(ndc.x * 0.5f + 0.5f, ndc.y * 0.5f + 0.5f, 0.0f, 1.0f);

    const Ray ray = m_camera.GetRay(ndc);

    Intersection intersection;
    DrawableActor actor;

    glm::vec3 color_accumulated = {};
    if (!this->IntersectFirstActor(ray, &intersection, &actor)) {
        // No intersection, sky colour
        color_accumulated = this->SampleSky(ray.direction);
    } else {
        const glm::vec3 P = intersection.t * ray.direction + ray.origin;

        const LightInput light_input{
            .eye = ray.origin,
            .P = P,
            .V = ray.origin - P,
            .N = intersection.normal,
        };
        color_accumulated = this->ShadeActor(actor, light_input);
    }
    const glm::vec3 tone_mapped = 1.0f - glm::exp(-color_accumulated);
    return DOOB_WRITE_PIXEL_F32(tone_mapped.r, tone_mapped.g, tone_mapped.b, 1.0f);
}
void PathTracer::RebuildAccelerationStructures() {}
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
        ActorId id = m_scene->NewDrawableActor(&shape, &material);
    }

    static Plane plane1 = Plane({ 0, 1, 0 }, { 0, -1, 0 });
    m_plane_actor = m_scene->NewDrawableActor(&plane1, &g_grid_mat);

    static PointLight light1 = PointLight({ 0, 10, 0 }, { 4.f, 8.f, 8.f });
    m_light_actor = m_scene->NewLightActor(&light1);

    m_drawable_actors.clear();
    m_light_actors.clear();
    m_scene->QueryScene([this](const Actor& actor) {
        if (actor.GetShape() && actor.GetMaterial()) {
            m_drawable_actors.push_back(DrawableActor{ .shape = actor.GetShape(), .material = actor.GetMaterial() });
        }
        if (actor.GetLight()) {
            m_light_actors.push_back(LightActor{ .light = actor.GetLight() });
        }
    });
}
void PathTracer::Cleanup() {}
bool PathTracer::IntersectFirstActor(const Ray& ray, Intersection* out_intersection, DrawableActor* out_actor) const {
    Intersection intersection{ .t = INFINITY };
    DrawableActor current_actor;

    for (const auto& actor : m_drawable_actors) {
        Intersection curr_intersection;
        if (!actor.shape->Intersect(ray, &curr_intersection)) {
            continue;
        }
        if (curr_intersection.t < intersection.t) {
            intersection = curr_intersection;
            current_actor = actor;
        }
    }
    if (glm::isinf(intersection.t)) {
        return false;
    }
    if (out_actor) {
        *out_actor = current_actor;
    }
    if (out_intersection) {
        *out_intersection = intersection;
    }
    return true;
}

glm::vec3 PathTracer::ShadeActor(const DrawableActor& actor, const LightInput& shading_input) const {
    DrawableActor curr_actor = actor;
    LightInput curr_shading_input = shading_input;
    glm::vec3 total = {};
    glm::vec3 diffuse_absorption = { 1, 1, 1 };
    glm::vec3 specular_absorption = { 1, 1, 1 };
    for (int b = 0; b <= m_parameters.max_light_bounces; ++b) {
        bool b_stop = false;

        glm::vec3 diffuse = {};
        glm::vec3 specular = {};
        MaterialOutput material = curr_actor.material->Evaluate({
            .position = curr_shading_input.P,
            .normal = curr_shading_input.N,
            .uv = { 0, 0 },
        });

        // DIRECT LIGHTING
        {

            for (const auto& [light_obj] : m_light_actors) {
                LightOutput result =
                    light_obj->Evaluate(curr_shading_input, { .specular_power = material.specular_power });
                diffuse += result.diffuse;
                specular += result.specular;
            }
        }
        // SPECULAR LIGHTING (Reflections)
        if (b < m_parameters.max_light_bounces) {
            glm::vec3 R = glm::reflect(-curr_shading_input.V, curr_shading_input.N);
            curr_shading_input.eye = curr_shading_input.P;

            Ray ray{ .origin = curr_shading_input.P, .direction = R };
            // offset ray to avoid bias
            ray.origin += glm::sign(curr_shading_input.N) * glm::abs(ray.origin * 0.0000002f);
            Intersection intersection;
            DrawableActor actor;
            if (!this->IntersectFirstActor(ray, &intersection, &actor)) {
                // reached sky
                specular += this->SampleSky(R);
                b_stop = true;
            } else {
                const glm::vec3 P = intersection.t * ray.direction + ray.origin;
                curr_shading_input.P = P;
                curr_shading_input.V = ray.origin - P;
                curr_shading_input.N = intersection.normal;
                curr_actor = actor;
            }
        }
        total += diffuse_absorption * material.albedo_color * diffuse +
                 specular_absorption * material.specular_color * specular;

        diffuse_absorption *= material.albedo_color;
        specular_absorption *= material.specular_color;

        if (b_stop) {
            break;
        }
    }
    return total;
}
DOOB_NODISCARD glm::vec3 PathTracer::SampleSky(const glm::vec3& R) const {
    // TODO skybox
    static glm::vec3 ambient_color = { 0.21f, 0.23f, 0.24f };
    return ambient_color;
}
} // namespace devs_out_of_bounds