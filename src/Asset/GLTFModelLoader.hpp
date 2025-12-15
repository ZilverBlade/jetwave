#pragma once

#include "IModelLoader.hpp"

namespace devs_out_of_bounds {
namespace model_loader {
    class GLTFModelLoader : public IModelLoader {
    public:
        GLTFModelLoader() = default;
        virtual ~GLTFModelLoader() = default;

        virtual ModelData Load(const std::filesystem::path& filepath) override;

        virtual ImageData ReadImageData(const ModelData& modelData, const Image& image) override;
        virtual MeshData ReadMeshData(const ModelData& modelData, const Mesh& mesh) override;
    };
} // namespace model_loader
} // namespace devs_out_of_bounds