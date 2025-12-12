#include "PathTracer.hpp"
#include <src/Graphics/Lights/AreaLight.hpp>
#include <src/Graphics/Lights/PointLight.hpp>
#include <src/Graphics/Materials/BasicMaterial.hpp>
#include <src/Graphics/Materials/GltfMaterial.hpp>
#include <src/Graphics/Materials/GridMaterial.hpp>
#include <src/Graphics/Materials/TriangleDebugMaterial.hpp>
#include <src/Graphics/Shapes/Plane.hpp>
#include <src/Graphics/Shapes/Sphere.hpp>

#include <random>
#include <src/Graphics/Shapes/Triangle.hpp>
#include "Tonemapping.hpp"

namespace devs_out_of_bounds {

static BasicMaterial g_default_mat = BasicMaterial({ 1.0f, 1.0f, 1.0f }, { 1, 1, 1 }, 64.0f);
static GltfMaterial g_gold_mat = { .roughness_factor = 0.2f, .metallic_factor = 1.0f };
static TriangleDebugMaterial g_debug_mat = TriangleDebugMaterial();
static GridMaterial g_grid_mat = GridMaterial();
static std::vector<PointLight> g_point_lights;

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
    m_accumulator.resize(static_cast<size_t>(new_width) * new_height);
    m_width = new_width;

    m_accumulation_count = 0;
    m_accumulator.clear();
}

void PathTracer::OnUpdate(float frame_time) {
    static float accum = 0.0f;
    if (!m_parameters.b_accumulate) {
        m_accumulation_count = 1;
        int index = 0;
        for (auto& light : g_point_lights) {
            float phase = index / static_cast<float>(m_light_actors.size()) * glm::two_pi<float>();
            light.m_position = glm::vec3(4.0f * glm::cos(accum + phase), 2.0f, 4.0f * glm::sin(accum + phase) + 4.0f);
            ++index;
        }
        accum += frame_time;
    } else {
        ++m_accumulation_count;
    }
}

Pixel PathTracer::Evaluate(int x, int y, uint32_t seed) const {
    float jitter_x = RandomFloatAdv(seed) - 0.5f;
    float jitter_y = RandomFloatAdv(seed) - 0.5f;

    // 2. Add to pixel coordinate FIRST
    // (x + 0.5) centers us in the pixel.
    // (+ jitter) moves us randomly within it.
    float px = (float)x + 0.5f + jitter_x;
    float py = (float)y + 0.5f + jitter_y;

    // 3. Convert to NDC (0..1 -> -1..1)
    // Note: We use the jittered 'px' and 'py' here
    glm::vec2 ndc;
    ndc.x = (px * m_inv_width_height.x) * 2.0f - 1.0f;
    ndc.y = (py * m_inv_width_height.y) * 2.0f - 1.0f;

    // 4. Apply Aspect Ratio & Flip (Now safe to do!)
    ndc.x *= m_ar; // usually we scale X by AR, but your code scales Y. Adapt as needed.
    // ndc.y /= m_ar; // (If you stick to your previous scaling method)
    ndc.y = -ndc.y; // Flip Y

    // debug NDC
    // return DOOB_WRITE_PIXEL_F32(ndc.x * 0.5f + 0.5f, ndc.y * 0.5f + 0.5f, 0.0f, 1.0f);

    const Ray ray = m_camera.GetRay(ndc);

    Intersection intersection;
    DrawableActor actor;

    glm::vec3 color = {};
    if (!this->IntersectFirstActor(ray, &intersection, &actor)) {
        // No intersection, sky colour
        color = this->SampleSky(ray.direction);
    } else {
        const glm::vec3 P = intersection.t * ray.direction + ray.origin;

        const LightInput light_input{
            .eye = ray.origin,
            .P = P,
            .V = glm::normalize(ray.origin - P),
            .N = intersection.flat_normal,
        };
        color = this->ShadeActor(actor, light_input, intersection, seed);
    }
    glm::dvec3& summed = m_accumulator[static_cast<size_t>(x) + y * m_width];
    if (m_parameters.b_accumulate) {
        summed += static_cast<glm::dvec3>(color);
    } else {
        summed = static_cast<glm::dvec3>(color);
    }

    glm::vec3 color_accumulated = static_cast<glm::vec3>(summed / static_cast<double>(m_accumulation_count));
    if (m_parameters.b_gt7_tonemapper) {
        GT7ToneMapping TM;
        initializeAsSDR(TM);
        applyToneMapping(color_accumulated, TM);
    } else {
        color_accumulated = 1.0f - exp(-color_accumulated);
    }

    return DOOB_WRITE_PIXEL_F32(color_accumulated.r, color_accumulated.g, color_accumulated.b, 1.0f);
}
void PathTracer::RebuildAccelerationStructures() {}
void PathTracer::LoadScene() {

    static Plane plane1 = Plane({ 0, 1, 0 }, { 0, -1, 0 });
    m_plane_actor = m_scene->NewDrawableActor(&plane1, &g_grid_mat);


    // static std::vector<std::pair<Sphere, BasicMaterial>> objects;
    // objects.clear();
    // objects.reserve(100);

    // std::random_device rd;
    // std::mt19937 gen(rd());
    // std::uniform_real_distribution<float> posDist(-10.0f, 10.0f); // Range -10 to 10
    // std::uniform_real_distribution<float> radDist(0.5f, 1.5f);    // Radius 0.5 to 1.5
    // std::uniform_real_distribution<float> colDist(0.0f, 1.0f);    // Colour 0.0 to 1.0

    // for (int i = 0; i < 100; ++i) {
    //     float x = posDist(gen);
    //     float y = posDist(gen);
    //     float z = posDist(gen) + 5.0f; // Offset Z so they are in front of camera
    //     float radius = radDist(gen);

    //    float r = colDist(gen);
    //    float g = colDist(gen);
    //    float b = colDist(gen);

    //    objects.emplace_back(Sphere({ x, y, z }, radius), BasicMaterial({ r, g, b }, { 1, 1, 1 }, 64.f));
    //}
    //
    // for (auto& [shape, material] : objects) {
    //    ActorId id = m_scene->NewDrawableActor(&shape, &material);
    //}

    static Sphere sphere1 = Sphere({ 0.0f, 0.0f, 5.0f }, 1.0f);
    ActorId sphere_actor = m_scene->NewDrawableActor(&sphere1, &g_default_mat);

    static Triangle triangle1 = Triangle({ 0, 0, 3 }, { 0, 3, 3 }, { 1, 1, 4 });
    // ActorId triangle_actor = m_scene->NewDrawableActor(&triangle1, &g_debug_mat);

    std::vector<glm::vec3> light_colors{ { 1.f, 1.f, 1.f }, { 1.f, .1f, .1f }, { .1f, .1f, 1.f }, { .1f, 1.f, .1f },
        { 1.f, 1.f, .1f }, { .1f, 1.f, 1.f } };
    g_point_lights.clear();
    g_point_lights.reserve(light_colors.size());
    for (int i = 0; i < light_colors.size(); ++i) {
        g_point_lights.push_back(PointLight({ 0, 10, 0 }, light_colors[i] * 10.0f));
        m_light_actor = m_scene->NewLightActor(&g_point_lights.back());
    }

    static AreaLight area_light1 = AreaLight({ 0, 4.0f, 8 }, { 1, 2 }, { 60, 60, 60 });
    ActorId light_actor1 = m_scene->NewLightActor(&area_light1);

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

DOOB_NODISCARD bool PathTracer::IntersectAnyActor(const Ray& ray) const {
    for (const auto& actor : m_drawable_actors) {
        if (actor.shape->Intersect(ray, nullptr)) {
            return true;
        }
    }
    return false;
}

glm::vec3 PathTracer::ShadeActor(const DrawableActor& actor, const LightInput& shading_input,
    const Intersection& intersection, uint32_t& seed) const {
    DrawableActor curr_actor = actor;
    LightInput curr_shading_input = shading_input;
    Intersection curr_intersection = intersection;
    glm::vec3 total = {};
    glm::vec3 diffuse_absorption = { 1, 1, 1 };
    glm::vec3 specular_absorption = { 1, 1, 1 };
    for (int b = 0; b <= m_parameters.max_light_bounces; ++b) {
        bool b_stop = false;

        glm::vec3 diffuse = {};
        glm::vec3 specular = {};
        Fragment fragment = curr_actor.shape->SampleFragment(curr_intersection);

        MaterialOutput material = curr_actor.material->Evaluate(fragment);
        curr_shading_input.N = material.world_normal;
        // DIRECT LIGHTING
        {
            for (const auto& [light_obj] : m_light_actors) {
                LightOutput result =
                    light_obj->Evaluate(curr_shading_input, { .specular_power = material.specular_power }, seed,
                        {
                            .userdata = reinterpret_cast<const void*>(this),
                            .fn_shadow_check =
                                [](const Ray& ray, const void* userdata) {
                                    return !reinterpret_cast<const PathTracer*>(userdata)->IntersectAnyActor(ray);
                                },
                        });
                diffuse += result.diffuse;
                specular += result.specular;
            }
        }

        float fresnel = glm::max(glm::dot(curr_shading_input.V, curr_shading_input.N), 0.0f);
        float f2 = fresnel * fresnel;
        float f4 = f2 * f2;
        float f5 = f4 * fresnel;
        float kD = fresnel;

        // SPECULAR LIGHTING (Reflections)
        if (b < m_parameters.max_light_bounces) {
            glm::vec3 R = glm::reflect(-curr_shading_input.V, curr_shading_input.N);
            curr_shading_input.eye = curr_shading_input.P;

            Ray ray{ .origin = curr_shading_input.P, .direction = R };
            // offset ray to avoid bias
            ray.origin += glm::sign(curr_shading_input.N) * glm::abs(ray.origin * 0.0000002f);
            DrawableActor actor;
            if (!this->IntersectFirstActor(ray, &curr_intersection, &actor)) {
                // reached sky
                specular += this->SampleSky(R);
                b_stop = true;
            } else {
                curr_shading_input.P = curr_intersection.position;
                curr_shading_input.V = glm::normalize(ray.origin - curr_intersection.position);
                curr_actor = actor;
            }
        }

        total += kD * diffuse_absorption * material.albedo_color * diffuse +
                 (1.0f - kD) * specular_absorption * material.specular_color * specular;

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
    static glm::vec3 ambient_color = { 0.51f, 0.53f, 0.54f };
    return ambient_color;
}
} // namespace devs_out_of_bounds