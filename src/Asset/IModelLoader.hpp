#pragma once

#include <filesystem>
#include <memory>
#include <numbers>
#include <optional>
#include <src/Asset/Common.hpp>
#include <string>
#include <variant>
#include <vector>


namespace devs_out_of_bounds {

    struct Image {
        std::string name = {};
        const uint32_t imageIndex = {};
    };

    enum struct Filter : uint16_t {
        Nearest,
        Linear,
        NearestMipMapNearest,
        LinearMipMapNearest,
        NearestMipMapLinear,
        LinearMipMapLinear,
    };

    enum struct WrapMode : uint16_t { ClampToEdge, MirroredRepeat, Repeat };

    struct Sampler {
        std::optional<Filter> magnificationFilter = {};
        std::optional<Filter> minificationFilter = {};
        WrapMode wrapS = WrapMode::Repeat;
        WrapMode wrapT = WrapMode::Repeat;
        std::string name = {};
    };

    struct Texture {
        uint32_t imageIndex = {};
        std::optional<uint32_t> samplerIndex = {};
        std::string name = {};
    };

    struct Transform {
        glm::vec3 position = {};
        glm::quat rotation = {};
        glm::vec3 scale = glm::vec3{ 1.0f };
    };

    struct Node {
        std::optional<uint32_t> meshIndex = {};
        std::optional<uint32_t> lightIndex = {};

        std::vector<float> weights = {};

        Transform transform = {};

        std::vector<uint32_t> children = {};
        std::string name = {};
    };

    struct Scene {
        std::vector<uint32_t> nodeIndices = {};
        std::string name = {};
    };

    struct Mesh {
        std::optional<uint32_t> materialIndex = {};
        const uint32_t meshGroupIndex = {};
        const uint32_t meshIndex = {};
    };

    struct MeshGroup {
        std::vector<Mesh> meshes = {};
        std::vector<float> weights = {};
        std::string name = {};
    };

    enum struct AlphaMode : uint8_t {
        Opaque,
        Mask,
        Blend,
    };

    struct Material {
        std::optional<uint32_t> albedoTexture = {};
        glm::vec4 albedoColor = glm::vec4{ 1.0f };

        std::optional<uint32_t> metallicRoughnessTexture = {};
        float metallicFactor = 1.0f;
        float roughnessFactor = 1.0f;

        std::optional<uint32_t> normalTexture = {};

        std::optional<uint32_t> emissiveTexture = {};
        glm::vec3 emissiveFactor = glm::vec3{ 0.0f };
        float emissiveStrength = 1.0f;

        AlphaMode alphaMode = AlphaMode::Opaque;
        float alphaCutoff = 1.0f;

        bool doubleSided = false;

        std::string name = {};
    };

    enum class LightType : uint8_t {
        Directional,
        Spot,
        Point,
    };

    struct Light {
        LightType type;
        glm::vec3 color = glm::vec3{ 1.0f };

        float intensity = {}; // point and spot lights use candela and directional light uses lux
        std::optional<float> range = {};

        float innerConeAngle = 0.0f;
        float outerConeAngle = std::numbers::pi_v<float> / 4.0f;

        std::string name = {};
    };

    struct Camera {
        struct Orthographic {
            float xmag;
            float ymag;
            float zfar;
            float znear;
        };

        struct Perspective {
            std::optional<float> aspectRatio;
            float yfov;
            std::optional<float> zfar;
            float znear;
        };

        std::variant<Perspective, Orthographic> camera = {};
        std::string name = {};
    };

    struct Asset {
        virtual ~Asset() = default;
    };

    struct ModelData {
        std::filesystem::path modelFilepath = {};
        std::vector<Scene> scenes = {};
        std::vector<Node> nodes = {};
        std::vector<Image> images = {};
        std::vector<Sampler> samplers = {};
        std::vector<Texture> textures = {};
        std::vector<MeshGroup> meshGroups = {};
        std::vector<Material> materials = {};
        std::vector<Light> lights = {};
        std::vector<Camera> cameras = {};
        std::unique_ptr<Asset> asset;
    };

    struct IModelLoader {
    public:
        IModelLoader() = default;
        virtual ~IModelLoader() = default;

        virtual ModelData Load(const std::filesystem::path& filepath) = 0;
        virtual ImageData ReadImageData(const ModelData& modelData, const Image& image) = 0;
        virtual MeshData ReadMeshData(const ModelData& modelData, const Mesh& mesh) = 0;
    };
} // namespace Editor
