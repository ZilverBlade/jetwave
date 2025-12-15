#include "SceneLoader.hpp"
#include <fstream>
#include <iostream>

#include <glm/gtc/packing.hpp>
#include <nlohmann/json.hpp>
#include <stb_image.h>

#include <src/Graphics/Shapes/Plane.hpp>
#include <src/Graphics/Shapes/Sphere.hpp>
#include <src/Graphics/Shapes/Triangle.hpp>

#include <src/Graphics/Lights/AreaLight.hpp>
#include <src/Graphics/Lights/DirectionalLight.hpp>
#include <src/Graphics/Lights/PointLight.hpp>

#include <src/Graphics/TextureViews/Rgbe9TextureView.hpp>

#include <src/Graphics/Materials/BasicMaterial.hpp>
#include <src/Graphics/Materials/BasicOrenMaterial.hpp>
#include <src/Graphics/Materials/ClearcoatMaterial.hpp>
#include <src/Graphics/Materials/EmissiveMaterial.hpp>
#include <src/Graphics/Materials/GlassMaterial.hpp>
#include <src/Graphics/Materials/GridCutoutMaterial.hpp>
#include <src/Graphics/Materials/GridMaterial.hpp>

#ifdef _WIN32
#include <Windows.h>
#else
#define MessageBoxA(...)
#endif

using json = nlohmann::json;

namespace nlohmann {
inline void from_json(const json& j, glm::vec3& v) {
    if (j.size() >= 3) {
        v.x = j[0].get<float>();
        v.y = j[1].get<float>();
        v.z = j[2].get<float>();
    }
}
inline void from_json(const json& j, glm::vec2& v) {
    if (j.size() >= 2) {
        v.x = j[0].get<float>();
        v.y = j[1].get<float>();
    }
}
} // namespace nlohmann
namespace devs_out_of_bounds {
static void LoadMaterialBasic(material::BasicMaterial& m, const json& parameters) {
    m.m_albedo = parameters.value("albedo", glm::vec3(1, 1, 1));
    m.m_roughness = parameters.value("roughness", 1.0f);
    m.m_specular = parameters.value("specular", 0.5f);
}
static void LoadMaterialBasicOren(material::BasicOrenMaterial& m, const json& parameters) {
    m.m_albedo = parameters.value("albedo", glm::vec3(1, 1, 1));
    m.m_diffuse_roughness_angle = parameters.value("diffuseRoughnessAngleRad", glm::half_pi<float>());
    m.m_specular_roughness = parameters.value("specularRoughness", 0.5f);
    m.m_specular = parameters.value("specular", 0.5f);
}

static void LoadMaterialClearcoat(material::ClearcoatMaterial& m, const json& parameters) {
    m.m_albedo = parameters.value("albedo", glm::vec3(1, 1, 1));
    m.m_clearcoat = parameters.value("clearcoat", 1.0f);
    m.m_clearcoat_roughness = parameters.value("clearcoatRoughness", 0.01f);
    m.m_roughness = parameters.value("roughness", 0.5f);
}

static void LoadMaterialEmissive(material::EmissiveMaterial& m, const json& parameters) {
    m.m_color = parameters.value("color", glm::vec3(1, 1, 1));
    m.m_lumens = parameters.value("lumens", 1000.0f);
}

static void LoadMaterialGlass(material::GlassMaterial& m, const json& parameters) {
    m.m_ior = parameters.value("indexOfRefraction", 1.5f);
    m.m_tint = parameters.value("tint", glm::vec3(1, 1, 1));
    m.m_roughness = parameters.value("roughness", 0.0f);
}

static void LoadMaterialGrid(material::GridMaterial& m, const json& parameters) {
    m.m_grid_foreground = parameters.value("topLayerAlbedo", glm::vec3(.7f, .7f, .7f));
    m.m_grid_background = parameters.value("bottomLayerAlbedo", glm::vec3(.4f, .4f, .4f));
    m.m_grid_size = parameters.value("gridSize", 1.0f);
}

static void LoadMaterialGridCutout(material::GridCutoutMaterial& m, const json& parameters) {
    m.m_grid_foreground = parameters.value("albedo", glm::vec3(.7f, .7f, .7f));
    m.m_grid_size = parameters.value("gridSize", 1.0f);
}


bool SceneLoader::Load(const std::string& filepath, Scene& scene, SceneAssets& assets) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        MessageBoxA(
            nullptr, std::string("Failed to open scene file: " + filepath).c_str(), "File error!", MB_ICONERROR);
        return false;
    }
    json j;
    try {
        j << file;
    } catch (const json::parse_error& e) {
        MessageBoxA(nullptr, (std::string("JSON Parse Error: ") + e.what()).c_str(), "Json error!", MB_ICONERROR);
        return false;
    }

    assets.Clear();

    if (j.contains("camera")) {
        const auto& j_camera = j["camera"];

        glm::vec3 position = j_camera.value("position", glm::vec3(0, 0, 0));
        glm::vec3 direction = j_camera.value("direction", glm::vec3(0, 0, 1));

        float fov = j_camera.value("fieldOfView", 90.0f);
        float aperture = j_camera.value("aperture", 16.0f);
        float inv_shutter_speed = j_camera.value("invShutterSpeed", 125.0f);
        float iso = j_camera.value("iso", 100.0f);
        float log_exposure = j_camera.value("logExposure", 0.0f);

        assets.camera.LookDir(glm::normalize(direction), fov);
        assets.camera.SetPosition(position);
        assets.camera.SetSensor(aperture, inv_shutter_speed, iso);
        assets.camera.SetLogExposure(log_exposure);
    }

    if (j.contains("sky")) {
        const auto& j_sky = j["sky"];

        assets.sky.lux = j_sky.value("lux", 0.0f);
        if (j_sky.contains("skybox")) {
            assets.sky.skybox_texture = LoadTexture(j_sky["skybox"], assets);
        }
        if (j_sky.contains("tint")) {
            assets.sky.skybox_tint = j_sky.value("tint", glm::vec3(1, 1, 1));
        }
    }

    if (j.contains("materials")) {
        for (const auto& j_mat : j["materials"]) {
            std::string type = j_mat.value("type", "basic");
            std::string name = j_mat.value("name", "unnamed");

            IMaterial* raw_ptr = nullptr;

            if (type == "basic") {
                auto mat = std::make_unique<material::BasicMaterial>();
                LoadMaterialBasic(*mat, j_mat["parameters"]);
                raw_ptr = mat.get();
                assets.materials.push_back(std::move(mat));
            } else if (type == "basic_oren") {
                auto mat = std::make_unique<material::BasicOrenMaterial>();
                LoadMaterialBasicOren(*mat, j_mat["parameters"]);
                raw_ptr = mat.get();
                assets.materials.push_back(std::move(mat));
            } else if (type == "clearcoat") {
                auto mat = std::make_unique<material::ClearcoatMaterial>();
                LoadMaterialClearcoat(*mat, j_mat["parameters"]);
                raw_ptr = mat.get();
                assets.materials.push_back(std::move(mat));
            } else if (type == "emissive") {
                auto mat = std::make_unique<material::EmissiveMaterial>();
                LoadMaterialEmissive(*mat, j_mat["parameters"]);
                raw_ptr = mat.get();
                assets.materials.push_back(std::move(mat));
            } else if (type == "glass") {
                auto mat = std::make_unique<material::GlassMaterial>();
                LoadMaterialGlass(*mat, j_mat["parameters"]);
                raw_ptr = mat.get();
                assets.materials.push_back(std::move(mat));
            } else if (type == "gridMaterial") {
                auto mat = std::make_unique<material::GridMaterial>();
                LoadMaterialGrid(*mat, j_mat["parameters"]);
                raw_ptr = mat.get();
                assets.materials.push_back(std::move(mat));
            } else if (type == "gridCutoutMaterial") {
                auto mat = std::make_unique<material::GridCutoutMaterial>();
                LoadMaterialGridCutout(*mat, j_mat["parameters"]);
                raw_ptr = mat.get();
                assets.materials.push_back(std::move(mat));
            }

            if (raw_ptr) {
                assets.material_lookup[name] = raw_ptr;
            }
        }
    }

    if (j.contains("actors")) {
        for (const auto& j_actor : j["actors"]) {
            std::string shape_type = j_actor.value("shape", "none");
            IShape* shape_ptr = nullptr;

            // --- Instantiate Shape ---
            if (shape_type == "sphere") {
                glm::vec3 center = j_actor.value("center", glm::vec3(0.0f));
                float radius = j_actor.value("radius", 1.0f);

                auto sphere = std::make_unique<shape::Sphere>(center, radius);
                shape_ptr = sphere.get();
                assets.shapes.push_back(std::move(sphere));
            } else if (shape_type == "plane") {
                glm::vec3 normal = j_actor.value("normal", glm::vec3(0, 1, 0));
                glm::vec3 position = j_actor.value("position", glm::vec3(0, 0, 0));

                auto plane = std::make_unique<shape::Plane>(normal, position);
                shape_ptr = plane.get();
                assets.shapes.push_back(std::move(plane));
            } else if (shape_type == "triangle") {
                glm::vec3 a = j_actor.value("a", glm::vec3(0, 0, 0));
                glm::vec3 b = j_actor.value("b", glm::vec3(0, 0, 0));
                glm::vec3 c = j_actor.value("c", glm::vec3(0, 0, 0));

                auto tri = std::make_unique<shape::Triangle>(a, b, c);
                shape_ptr = tri.get();
                assets.shapes.push_back(std::move(tri));
            }

            // --- Instantiate Actor ---
            if (shape_ptr) {
                std::string mat_name = j_actor.value("material", "");
                IMaterial* mat_ptr = nullptr;

                // Find material by name
                if (assets.material_lookup.count(mat_name)) {
                    mat_ptr = assets.material_lookup[mat_name];
                }

                if (mat_ptr) {
                    ActorId id = scene.NewDrawableActor(shape_ptr, mat_ptr);
                }
            }
        }
    }

    if (j.contains("lights")) {
        for (const auto& j_light : j["lights"]) {
            std::string type = j_light.value("type", "point");
            ILight* light_ptr = nullptr;
            float intensity = 1000.0f;
            if (j_light.contains("lumens")) {
                intensity = j_light.value("lumens", 0.0f) / (4.0f * glm::pi<float>());
            } else if (j_light.contains("candelas")) {
                intensity = j_light.value("candelas", 0.0f);
            } else if (j_light.contains("lux")) {
                intensity = j_light.value("lux", 0.0f);
            }
            glm::vec3 lux = intensity * j_light.value("color", glm::vec3(1, 1, 1));
            if (type == "point") {
                glm::vec3 pos = j_light.value("position", glm::vec3(0, 0, 0));
                auto l = std::make_unique<light::PointLight>(pos, lux);
                light_ptr = l.get();
                assets.lights.push_back(std::move(l));
            } else if (type == "directional") {
                glm::vec3 dir = j_light.value("direction", glm::vec3(0, -1, 0));
                float angle = j_light.value("angle", 2.0f);
                auto l = std::make_unique<light::DirectionalLight>(glm::normalize(dir), lux, angle);
                light_ptr = l.get();
                assets.lights.push_back(std::move(l));
            } else if (type == "area") {
                glm::vec3 center = j_light.value("center", glm::vec3(0, 0, 0));
                glm::vec2 extent = j_light.value("extent", glm::vec2(1, 1));
                auto l = std::make_unique<light::AreaLight>(center, extent, lux);
                light_ptr = l.get();
                assets.lights.push_back(std::move(l));
            }

            if (light_ptr) {
                scene.NewLightActor(light_ptr);
            }
        }
    }

    return true;
}

ITextureView* SceneLoader::LoadTexture(const std::string& texturepath, SceneAssets& assets) {
    ITextureView* raw_ptr = nullptr;
    if (assets.texture_lookup.count(texturepath)) {
        return assets.texture_lookup[texturepath];
    }
    {
        // FIXME: multiple formats!
        int x, y, c;
        float* data = stbi_loadf(texturepath.c_str(), &x, &y, &c, 3);

        assets.misc_data.push_back(std::make_unique<std::vector<uint8_t>>());
        std::vector<uint8_t>& texture_data = *assets.misc_data.back();
        texture_data.resize(x * y * 4);
        for (int i = 0; i < x * y; ++i) {
            float* src_ptr = data + i * c;
            uint8_t* dst_ptr = texture_data.data() + i * 4;

            float r = src_ptr[0];
            float g = src_ptr[1];
            float b = src_ptr[2];

            *reinterpret_cast<uint32_t*>(dst_ptr) = glm::packF3x9_E1x5({ r, g, b });
        }
        stbi_image_free(data);

        auto tex = std::make_unique<texture_view::Rgbe9TextureView>(texture_data.data(), x, y, x * 4);
        raw_ptr = tex.get();
        assets.textures.push_back(std::move(tex));
    }


    if (raw_ptr) {
        assets.texture_lookup[texturepath] = raw_ptr;
    }
    return raw_ptr;
}

} // namespace devs_out_of_bounds