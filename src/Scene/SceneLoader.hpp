#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <src/Core.hpp>
#include <src/Scene/Scene.hpp>

#include <src/Graphics/Camera.hpp>

#include <src/Graphics/ILight.hpp>
#include <src/Graphics/IMaterial.hpp>
#include <src/Graphics/ISamplerState.hpp>
#include <src/Graphics/IShape.hpp>
#include <src/Graphics/ITextureView.hpp>

namespace devs_out_of_bounds {
struct Sky {
    ITextureView* skybox_texture = {};
    float lux = 0.0f;
    glm::vec3 skybox_tint = { 1, 1, 1 };
};
struct IData {
    virtual ~IData() = default;
};

// Container to own the heap memory of loaded objects
struct SceneAssets {
    SceneAssets() {}
    ~SceneAssets() {}

    std::vector<std::unique_ptr<IData>> misc_data;

    std::vector<std::unique_ptr<ITextureView>> textures;
    std::vector<std::unique_ptr<ISamplerState>> samplers;

    std::vector<std::unique_ptr<IShape>> shapes;
    std::vector<std::unique_ptr<IMaterial>> materials;
    std::vector<std::unique_ptr<ILight>> lights;

    std::unordered_map<std::string, IMaterial*> material_lookup;
    std::unordered_map<std::string, ITextureView*> texture_lookup;

    Sky sky;
    Camera camera;

    void Clear() {
        misc_data.clear();
        textures.clear();
        samplers.clear();
        shapes.clear();
        materials.clear();
        lights.clear();
        material_lookup.clear();
        texture_lookup.clear();
    }
};

class SceneLoader {
public:
    static bool Load(const std::string& filepath, Scene& scene, SceneAssets& assets);
    static bool LoadGltf(const std::string& gltf_file, Scene& scene, SceneAssets& assets);

private:
    static ITextureView* LoadTexture(const std::string& texturepath, SceneAssets& assets);
};

} // namespace devs_out_of_bounds