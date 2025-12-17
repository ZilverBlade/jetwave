#include "SceneLoader.hpp"
#include <fstream>
#include <iostream>

#include <glm/gtc/packing.hpp>
#include <nlohmann/json.hpp>
#include <stb_image.h>

#include <src/Asset/GLTFModelLoader.hpp>
#include <src/Asset/IModelLoader.hpp>

#include <src/Graphics/Shapes/Box.hpp>
#include <src/Graphics/Shapes/Plane.hpp>
#include <src/Graphics/Shapes/Sphere.hpp>
#include <src/Graphics/Shapes/Triangle.hpp>
#include <src/Graphics/Shapes/Bvh.hpp>

#include <src/Graphics/Lights/AreaLight.hpp>
#include <src/Graphics/Lights/DirectionalLight.hpp>
#include <src/Graphics/Lights/PointLight.hpp>

#include <src/Graphics/SamplerStates/LinearWrapSampler.hpp>
#include <src/Graphics/TextureViews/Rgba8TextureView.hpp>
#include <src/Graphics/TextureViews/Rgbe9TextureView.hpp>

#include <src/Graphics/Materials/BasicMaterial.hpp>
#include <src/Graphics/Materials/BasicOrenMaterial.hpp>
#include <src/Graphics/Materials/ClearcoatMaterial.hpp>
#include <src/Graphics/Materials/EmissiveMaterial.hpp>
#include <src/Graphics/Materials/GlassMaterial.hpp>
#include <src/Graphics/Materials/GridCutoutMaterial.hpp>
#include <src/Graphics/Materials/GridMaterial.hpp>
#include <src/Graphics/Materials/MetallicMaterial.hpp>

#include <src/Graphics/Materials/GltfMaterial.hpp>

#ifdef _WIN32
#include <Windows.h>
#else
#define MessageBoxA(...)
#endif
#include <deque>

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

static glm::vec3 ConvertColor(const glm::vec3& color) { return glm::pow(color, glm::vec3(2.2f)); }

static void LoadMaterialBasic(material::BasicMaterial& m, const json& parameters) {
    m.m_albedo = ConvertColor(parameters.value("albedo", glm::vec3(1, 1, 1)));
    m.m_roughness = parameters.value("roughness", 1.0f);
    m.m_specular = parameters.value("specular", 0.5f);
}
static void LoadMaterialBasicOren(material::BasicOrenMaterial& m, const json& parameters) {
    m.m_albedo = ConvertColor(parameters.value("albedo", glm::vec3(1, 1, 1)));
    m.m_diffuse_roughness_angle = parameters.value("diffuseRoughnessAngleRad", glm::half_pi<float>());
    m.m_specular_roughness = parameters.value("specularRoughness", 0.5f);
    m.m_specular = parameters.value("specular", 0.5f);
}

static void LoadMaterialClearcoat(material::ClearcoatMaterial& m, const json& parameters) {
    m.m_albedo = ConvertColor(parameters.value("albedo", glm::vec3(1, 1, 1)));
    m.m_clearcoat = parameters.value("clearcoat", 1.0f);
    m.m_clearcoat_roughness = parameters.value("clearcoatRoughness", 0.01f);
    m.m_roughness = parameters.value("roughness", 0.5f);
}

static void LoadMaterialEmissive(material::EmissiveMaterial& m, const json& parameters) {
    m.m_color = ConvertColor(parameters.value("color", glm::vec3(1, 1, 1)));
    m.m_lumens = parameters.value("lumens", 1000.0f);
}

static void LoadMaterialGlass(material::GlassMaterial& m, const json& parameters) {
    m.m_ior = parameters.value("indexOfRefraction", 1.5f);
    m.m_tint = ConvertColor(parameters.value("tint", glm::vec3(1, 1, 1)));
    m.m_roughness = parameters.value("roughness", 0.0f);
}

static void LoadMaterialGrid(material::GridMaterial& m, const json& parameters) {
    m.m_grid_foreground = ConvertColor(parameters.value("topLayerAlbedo", glm::vec3(.7f, .7f, .7f)));
    m.m_grid_background = ConvertColor(parameters.value("bottomLayerAlbedo", glm::vec3(.4f, .4f, .4f)));
    m.m_grid_size = parameters.value("gridSize", 1.0f);
}

static void LoadMaterialGridCutout(material::GridCutoutMaterial& m, const json& parameters) {
    m.m_grid_foreground = ConvertColor(parameters.value("albedo", glm::vec3(.7f, .7f, .7f)));
    m.m_grid_size = parameters.value("gridSize", 1.0f);
}
static void LoadMaterialMetallic(material::MetallicMaterial& m, const json& parameters) {
    m.m_albedo = ConvertColor(parameters.value("albedo", glm::vec3(1, 1, 1)));
    m.m_roughness = parameters.value("roughness", 0.5f);
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
            } else if (type == "metallic") {
                auto mat = std::make_unique<material::MetallicMaterial>();
                LoadMaterialMetallic(*mat, j_mat["parameters"]);
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
            } else if (shape_type == "box") {
                glm::vec3 extent = j_actor.value("extent", glm::vec3(1, 1, 1));
                glm::vec3 position = j_actor.value("position", glm::vec3(0, 0, 0));

                auto box = std::make_unique<shape::Box>(position, extent);
                shape_ptr = box.get();
                assets.shapes.push_back(std::move(box));
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


    if (j.contains("gltf")) {
        for (const auto& j_gltf : j["gltf"]) {
            std::string path = j_gltf.value("file", "");
            glm::mat4 transform(1.f);
            if (j_gltf.contains("offset")) {
                const auto& j_off = j_gltf["offset"];
                glm::vec3 position = j_off.value("position", glm::vec3(0, 0, 0));
                glm::vec3 rotationXYZ = j_off.value("position", glm::vec3(0, 0, 0));
                glm::vec3 scale = j_off.value("scale", glm::vec3(1, 1, 1));
                transform = glm::translate(glm::scale(glm::mat4(1), scale), position);
            }
            LoadGltf(path, scene, assets, transform);
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
                ActorId id = scene.NewLightActor(light_ptr);
            }
        }
    }

    return true;
}

bool SceneLoader::LoadGltf(
    const std::string& gltf_file, Scene& scene, SceneAssets& assets, const glm::mat4& base_Transform) {
    model_loader::GLTFModelLoader loader;
    model_loader::ModelData data = loader.Load(gltf_file);

    // gltf_meshes gltf_mesh_instances MEMORY LEAK TODO:
    std::vector<std::vector<Mesh*>> gltf_meshes = {};
    std::vector<MeshInstance*> gltf_mesh_instances = {};

    std::vector<ITextureView*> gltf_texture_indices = {};
    std::vector<IMaterial*> gltf_material_indices = {};

    for (auto& mesh_group : data.meshGroups) {
        std::vector<Mesh*> mesh_shapes = {};
        for (auto& mesh : mesh_group.meshes) {
            model_loader::MeshData result = loader.ReadMeshData(data, mesh);

            Mesh* m = new Mesh();
            m->m_indices = result.indices;
            for (int i = 0; i < result.positions.size(); ++i) {
                m->m_vertices.push_back(Vertex{
                    .position = result.positions[i],
                    .normal = result.normals.empty() ? glm::vec3{} : result.normals[i],
                    .tangent = result.tangents.empty() ? glm::vec3{} : result.tangents[i],
                    .uv = (result.uvs.empty() || result.uvs[0].empty()) ? glm::vec2{} : result.uvs[0][i],
                });
            }
            mesh_shapes.push_back(m);
        }
        gltf_meshes.push_back(mesh_shapes);
    }

    for (auto& image : data.images) {
        model_loader::ImageData result = loader.ReadImageData(data, image);
        class GltfTexture : public IData {
        public:
            GltfTexture(model_loader::ImageData&& data) : m_data(data) {}
            ~GltfTexture() override {}
            model_loader::ImageData m_data;
        };
        assets.misc_data.push_back(std::make_unique<GltfTexture>(std::move(result)));
        auto* t = reinterpret_cast<GltfTexture*>(assets.misc_data.back().get());
        assets.textures.push_back(std::make_unique<texture_view::Rgba8TextureView>(
            t->m_data.data.data(), t->m_data.width, t->m_data.height, t->m_data.width * 4));
        auto* tv = assets.textures.back().get();
        gltf_texture_indices.push_back(tv);
    }


    for (auto& material : data.materials) {
        material::GltfMaterial mat;
        mat.base_color_factor = material.albedoColor;
        mat.base_color_texture = material.albedoTexture ? gltf_texture_indices[*material.albedoTexture] : nullptr;

        mat.roughness_factor = material.roughnessFactor;
        mat.metallic_factor = material.metallicFactor;
        mat.metallic_roughness_texture =
            material.metallicRoughnessTexture ? gltf_texture_indices[*material.metallicRoughnessTexture] : nullptr;

        mat.emissive_factor = material.emissiveFactor * material.emissiveStrength * 10000.0f;
        mat.emissive_texture = material.emissiveTexture ? gltf_texture_indices[*material.emissiveTexture] : nullptr;
        mat.normal_strength = 1.0f;
        mat.normal_texture = material.normalTexture ? gltf_texture_indices[*material.normalTexture] : nullptr;

        class SamplerDeleter : public IData {
        public:
            SamplerDeleter(ISamplerState* s) : m_s(s) {}
            ~SamplerDeleter() override { delete m_s; }
            ISamplerState* m_s;
        };

        mat.sampler_state = new sampler::LinearWrapSampler;
        assets.misc_data.push_back(std::make_unique<SamplerDeleter>(mat.sampler_state));
        assets.materials.push_back(std::make_unique<material::GltfMaterial>(mat));
        gltf_material_indices.push_back(assets.materials.back().get());
    }

    for (auto& s : data.scenes) {
        std::deque<int> remaining_children;
        remaining_children.append_range(s.nodeIndices);
        std::deque<glm::mat4> remaining_children_transforms = {};
        for (auto x : remaining_children) {
            remaining_children_transforms.push_back(base_Transform);
        }
        while (!remaining_children.empty()) {
            int child = remaining_children.front();
            remaining_children.pop_front();
            glm::mat4 transform = remaining_children_transforms.front();
            remaining_children_transforms.pop_front();

            const model_loader::Node& node = data.nodes[child];

            transform *=
                glm::translate(glm::toMat4(node.transform.rotation) * glm::scale(glm::mat4(1.f), node.transform.scale),
                    node.transform.position);

            remaining_children.append_range(node.children);
            for (auto x : node.children) {
                remaining_children_transforms.push_back(transform);
            }

            if (node.meshIndex) {
                int i = 0;
                for (auto w : data.meshGroups) {
                    if (i == *node.meshIndex) {
                        for (auto z : w.meshes) {
                            Mesh* mesh = gltf_meshes[i][z.meshIndex];
                            IMaterial* material = gltf_material_indices[*z.materialIndex];

                            MeshInstance* mesh_instance = new MeshInstance(mesh, transform);
                            gltf_mesh_instances.push_back(mesh_instance);

                            assets.shapes.push_back(std::make_unique<shape::BVH>(mesh_instance));

                            ActorId id = scene.NewDrawableActor(assets.shapes.back().get(), material);
                        }
                        break;
                    }
                    ++i;
                }
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

        class Deleter : public IData {
        public:
            Deleter(uint8_t* p) : m_ptr(p) {}
            ~Deleter() override { delete[] m_ptr; }
            uint8_t* m_ptr;
        };

        uint8_t* p_texture_data = new uint8_t[x * y * 4];
        assets.misc_data.push_back(std::make_unique<Deleter>(p_texture_data));
        for (int i = 0; i < x * y; ++i) {
            float* src_ptr = data + i * c;
            uint8_t* dst_ptr = p_texture_data + i * 4;

            float r = src_ptr[0];
            float g = src_ptr[1];
            float b = src_ptr[2];

            *reinterpret_cast<uint32_t*>(dst_ptr) = glm::packF3x9_E1x5({ r, g, b });
        }
        stbi_image_free(data);

        auto tex = std::make_unique<texture_view::Rgbe9TextureView>(p_texture_data, x, y, x * 4);
        raw_ptr = tex.get();
        assets.textures.push_back(std::move(tex));
    }

    if (raw_ptr) {
        assets.texture_lookup[texturepath] = raw_ptr;
    }
    return raw_ptr;
}

} // namespace devs_out_of_bounds