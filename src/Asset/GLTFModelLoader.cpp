#include "GLTFModelLoader.hpp"

#include <fastgltf/core.hpp>
#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/types.hpp>
#include <libassert/assert.hpp>
#include <print>

#include <glm/gtx/matrix_decompose.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace devs_out_of_bounds {
        struct GltfAsset : public Asset {
            GltfAsset(fastgltf::Asset&& a) : asset(std::move(a)) {}
            fastgltf::Asset asset;
        };

        ModelData GLTFModelLoader::Load(const std::filesystem::path& filepath) {
            if (!std::filesystem::exists(filepath)) {
                std::println("Failed to find: {}", filepath.string());
            }

            auto gltfFile = fastgltf::MappedGltfFile::FromPath(filepath);

            if (!bool(gltfFile)) {
                std::println("Failed to open glTF file: {}", fastgltf::getErrorMessage(gltfFile.error()));
            }

            static constexpr fastgltf::Extensions supportedExtensions = {};
            fastgltf::Parser parser(supportedExtensions);

            static constexpr fastgltf::Options gltfOptions = {};
            // static constexpr fastgltf::Options gltfOptions = fastgltf::Options::LoadExternalBuffers;

            fastgltf::Asset asset;
            {
                auto expectedAsset = parser.loadGltf(gltfFile.get(), filepath.parent_path(), gltfOptions);

                if (expectedAsset.error() != fastgltf::Error::None) {
                    std::println("Failed to load glTF file: {}", fastgltf::getErrorMessage(gltfFile.error()));
                }

                asset = std::move(expectedAsset.get());
            }


            auto castOptionalType = [](const auto& optional, const auto& fn) -> auto {
                return optional.has_value() ? std::make_optional(fn(optional.value())) : std::nullopt;
            };

            auto castOptionalU32 = [](const auto& optional) -> auto {
                return optional.has_value() ? std::make_optional(static_cast<uint32_t>(optional.value())) : std::nullopt;
            };

            auto castOptionalTexture = [](const auto& optional) -> auto {
                return optional.has_value() ? std::make_optional(static_cast<uint32_t>(optional.value().textureIndex)) : std::nullopt;
            };

            ModelData modelData = {};
            modelData.scenes.reserve(asset.scenes.size());
            for (const fastgltf::Scene& gltfScene : asset.scenes) {
                Scene scene = {};
                scene.name = gltfScene.name.c_str();

                scene.nodeIndices.reserve(gltfScene.nodeIndices.size());
                for (usize value : gltfScene.nodeIndices) {
                    scene.nodeIndices.push_back(value);
                }

                modelData.scenes.push_back(scene);
            }

            modelData.nodes.reserve(asset.nodes.size());
            for (fastgltf::Node& gltfNode : asset.nodes) {
                Node node = {};
                node.name = gltfNode.name.c_str();

                node.meshIndex = castOptionalU32(gltfNode.meshIndex);
                node.lightIndex = castOptionalU32(gltfNode.lightIndex);

                for (float value : gltfNode.weights) {
                    node.weights.push_back(value);
                }

                for (usize value : gltfNode.children) {
                    node.children.push_back(value);
                }

                node.transform = std::visit(
                    fastgltf::visitor{
                        [](fastgltf::TRS trs) {
                            Transform transform = {};
                            transform.position = { trs.translation.x(), trs.translation.y(), trs.translation.z() };

                            transform.rotation.x = trs.rotation[0];
                            transform.rotation.y = trs.rotation[1];
                            transform.rotation.z = trs.rotation[2];
                            transform.rotation.w = trs.rotation[3];

                            transform.scale = { trs.scale.x(), trs.scale.y(), trs.scale.z() };
                            return transform;
                        },
                        [](fastgltf::glm::mat4x4 trs) {
                            auto decompose_transform = [](const glm::mat4& transform, glm::vec3& translation, glm::vec3& rotation, glm::vec3& scale) -> bool {
                                glm::mat4 local_matrix{ transform };

                                if (glm::epsilonEqual(local_matrix[3][3], static_cast<float>(0), glm::epsilon<float>())) {
                                    return false;
                                }

                                if (glm::epsilonNotEqual(local_matrix[0][3], static_cast<float>(0), glm::epsilon<float>()) ||
                                    glm::epsilonNotEqual(local_matrix[1][3], static_cast<float>(0), glm::epsilon<float>()) ||
                                    glm::epsilonNotEqual(local_matrix[2][3], static_cast<float>(0), glm::epsilon<float>())) {
                                    local_matrix[0][3] = local_matrix[1][3] = local_matrix[2][3] = static_cast<float>(0);
                                    local_matrix[3][3] = static_cast<float>(1);
                                }

                                translation = glm::vec3(local_matrix[3]);
                                local_matrix[3] = glm::vec4(0, 0, 0, local_matrix[3].w);

                                glm::vec3 row[3];

                                for (i32 i = 0; i < 3; i++) {
                                    for (i32 j = 0; j < 3; j++) {
                                        row[i][j] = local_matrix[i][j];
                                    }
                                }

                                scale.x = length(row[0]);
                                row[0] = glm::detail::scale(row[0], static_cast<float>(1));
                                scale.y = length(row[1]);
                                row[1] = glm::detail::scale(row[1], static_cast<float>(1));
                                scale.z = length(row[2]);
                                row[2] = glm::detail::scale(row[2], static_cast<float>(1));

                                rotation.y = asin(-row[0][2]);
                                if (cos(rotation.y) != 0) {
                                    rotation.x = atan2(row[1][2], row[2][2]);
                                    rotation.z = atan2(row[0][1], row[0][0]);
                                } else {
                                    rotation.x = atan2(-row[2][0], row[1][1]);
                                    rotation.z = 0;
                                }

                                rotation = glm::degrees(rotation);

                                return true;
                            };

                            Transform transform = {};
                            glm::vec3 rotation = {};
                            decompose_transform(*reinterpret_cast<glm::mat4*>(&trs), transform.position, rotation, transform.scale);
                            transform.rotation = glm::quat(glm::radians(rotation));
                            return transform;
                        } },
                    gltfNode.transform);

                modelData.nodes.push_back(node);
            }

            modelData.images.reserve(asset.images.size());
            for (const fastgltf::Image& gltfImage : asset.images) {
                modelData.images.emplace_back(gltfImage.name.c_str(), modelData.images.size());
            }

            modelData.samplers.reserve(asset.samplers.size());
            for (const fastgltf::Sampler& gltfSampler : asset.samplers) {
                Sampler sampler = {};
                sampler.name = gltfSampler.name.c_str();

                auto fastgltfFilterToFilter = [](fastgltf::Filter fastgltfFilter) -> Filter {
                    switch (fastgltfFilter) {
                    case fastgltf::Filter::Nearest:
                        return Filter::Nearest;
                    case fastgltf::Filter::Linear:
                        return Filter::Linear;
                    case fastgltf::Filter::NearestMipMapNearest:
                        return Filter::NearestMipMapNearest;
                    case fastgltf::Filter::LinearMipMapNearest:
                        return Filter::LinearMipMapNearest;
                    case fastgltf::Filter::NearestMipMapLinear:
                        return Filter::NearestMipMapLinear;
                    case fastgltf::Filter::LinearMipMapLinear:
                        return Filter::LinearMipMapLinear;
                    default:
                        return Filter::LinearMipMapLinear;
                    }
                };

                sampler.magnificationFilter = castOptionalType(gltfSampler.magFilter, fastgltfFilterToFilter);
                sampler.minificationFilter = castOptionalType(gltfSampler.minFilter, fastgltfFilterToFilter);

                auto fastgltfWrapToWrapMode = [](fastgltf::Wrap fastgltfWrap) -> WrapMode {
                    switch (fastgltfWrap) {
                    case fastgltf::Wrap::ClampToEdge:
                        return WrapMode::ClampToEdge;
                    case fastgltf::Wrap::MirroredRepeat:
                        return WrapMode::MirroredRepeat;
                    case fastgltf::Wrap::Repeat:
                        return WrapMode::Repeat;
                    default:
                        return WrapMode::Repeat;
                    }
                };

                sampler.wrapS = fastgltfWrapToWrapMode(gltfSampler.wrapS);
                sampler.wrapT = fastgltfWrapToWrapMode(gltfSampler.wrapT);

                modelData.samplers.push_back(sampler);
            }

            modelData.textures.reserve(asset.textures.size());
            for (const fastgltf::Texture& gltfTexture : asset.textures) {
                Texture texture = {};
                texture.name = gltfTexture.name.c_str();
                texture.samplerIndex = castOptionalU32(gltfTexture.samplerIndex);

                if (gltfTexture.imageIndex.has_value()) {
                    texture.imageIndex = static_cast<uint32_t>(gltfTexture.imageIndex.value());
                } else if (gltfTexture.basisuImageIndex.has_value()) {
                    texture.imageIndex = static_cast<uint32_t>(gltfTexture.basisuImageIndex.value());
                } else if (gltfTexture.ddsImageIndex.has_value()) {
                    texture.imageIndex = static_cast<uint32_t>(gltfTexture.ddsImageIndex.value());
                } else if (gltfTexture.webpImageIndex.has_value()) {
                    texture.imageIndex = static_cast<uint32_t>(gltfTexture.webpImageIndex.value());
                }
                modelData.textures.push_back(texture);
            }

            modelData.meshGroups.reserve(asset.meshes.size());
            for (const fastgltf::Mesh& gltfMesh : asset.meshes) {
                MeshGroup meshGroup = {};
                meshGroup.name = gltfMesh.name.c_str();

                meshGroup.weights.reserve(gltfMesh.weights.size());
                for (float value : gltfMesh.weights) {
                    meshGroup.weights.push_back(value);
                }

                meshGroup.meshes.reserve(gltfMesh.primitives.size());
                for (const fastgltf::Primitive& primitive : gltfMesh.primitives) {
                    meshGroup.meshes.emplace_back(castOptionalU32(primitive.materialIndex), modelData.meshGroups.size(), meshGroup.meshes.size());
                }

                modelData.meshGroups.push_back(meshGroup);
            }

            modelData.materials.reserve(asset.materials.size());
            for (const fastgltf::Material& gltfMaterial : asset.materials) {
                Material material = {};
                material.name = gltfMaterial.name.c_str();

                material.albedoTexture = castOptionalTexture(gltfMaterial.pbrData.baseColorTexture);
                material.albedoColor = { gltfMaterial.pbrData.baseColorFactor.x(), gltfMaterial.pbrData.baseColorFactor.y(), gltfMaterial.pbrData.baseColorFactor.z(), gltfMaterial.pbrData.baseColorFactor.w() };

                material.metallicRoughnessTexture = castOptionalTexture(gltfMaterial.pbrData.metallicRoughnessTexture);
                material.metallicFactor = gltfMaterial.pbrData.metallicFactor;
                material.roughnessFactor = gltfMaterial.pbrData.roughnessFactor;

                material.normalTexture = castOptionalTexture(gltfMaterial.normalTexture);

                material.emissiveTexture = castOptionalTexture(gltfMaterial.emissiveTexture);
                material.emissiveFactor = { gltfMaterial.emissiveFactor.x(), gltfMaterial.emissiveFactor.y(), gltfMaterial.emissiveFactor.z() };
                material.emissiveStrength = gltfMaterial.emissiveStrength;

                material.alphaMode = [](fastgltf::AlphaMode alphaMode) {
                    switch (alphaMode) {
                    case fastgltf::AlphaMode::Opaque:
                        return AlphaMode::Opaque;
                    case fastgltf::AlphaMode::Mask:
                        return AlphaMode::Mask;
                    case fastgltf::AlphaMode::Blend:
                        return AlphaMode::Blend;
                    default:
                        return AlphaMode::Opaque;
                    }
                }(gltfMaterial.alphaMode);
                material.alphaCutoff = gltfMaterial.alphaCutoff;

                material.doubleSided = gltfMaterial.doubleSided;

                modelData.materials.push_back(material);
            }

            modelData.lights.reserve(asset.lights.size());
            for (const fastgltf::Light& gltfLight : asset.lights) {
                Light light = {};
                light.name = gltfLight.name.c_str();

                light.type = [](fastgltf::LightType type) {
                    switch (type) {
                    case fastgltf::LightType::Directional: 
                        return LightType::Directional;
                    case fastgltf::LightType::Spot: 
                        return LightType::Spot;
                    case fastgltf::LightType::Point:
                        return LightType::Point;
                    default:
                        return LightType{};
                    }
                    return LightType{};
                }(gltfLight.type);

                light.color = { gltfLight.color.x(), gltfLight.color.y(), gltfLight.color.z() };
                light.intensity = gltfLight.intensity;
                light.range = castOptionalType(light.range, [](auto value) { return static_cast<float>(value); });
                if (gltfLight.innerConeAngle.has_value()) {
                    light.innerConeAngle = gltfLight.innerConeAngle.value();
                }
                if (gltfLight.outerConeAngle.has_value()) {
                    light.outerConeAngle = gltfLight.outerConeAngle.value();
                }

                modelData.lights.push_back(light);
            }

            modelData.cameras.reserve(asset.cameras.size());
            for (const fastgltf::Camera& gltfCamera : asset.cameras) {
                Camera camera = {};
                camera.name = gltfCamera.name.c_str();

                if (std::holds_alternative<fastgltf::Camera::Perspective>(gltfCamera.camera)) {
                    const fastgltf::Camera::Perspective perspective = std::get<fastgltf::Camera::Perspective>(gltfCamera.camera);
                    camera.camera = Camera::Perspective{
                        .aspectRatio = castOptionalType(perspective.aspectRatio, [](auto value) { return static_cast<float>(value); }),
                        .yfov = static_cast<float>(perspective.yfov),
                        .zfar = castOptionalType(perspective.zfar, [](auto value) { return static_cast<float>(value); }),
                        .znear = static_cast<float>(perspective.znear),
                    };
                } else if (std::holds_alternative<fastgltf::Camera::Orthographic>(gltfCamera.camera)) {
                    const fastgltf::Camera::Orthographic orthographic = std::get<fastgltf::Camera::Orthographic>(gltfCamera.camera);
                    camera.camera = Camera::Orthographic{
                        .xmag = static_cast<float>(orthographic.xmag),
                        .ymag = static_cast<float>(orthographic.ymag),
                        .zfar = static_cast<float>(orthographic.zfar),
                        .znear = static_cast<float>(orthographic.znear),
                    };
                }

                modelData.cameras.push_back(camera);
            }

            // TODO: replace
            std::println("scene count: {}", modelData.scenes.size());
            std::println("node count: {}", modelData.nodes.size());
            std::println("image count: {}", modelData.images.size());
            std::println("sampler count: {}", modelData.samplers.size());
            std::println("texture count: {}", modelData.textures.size());
            std::println("meshGroup count: {}", modelData.meshGroups.size());
            std::println("material count: {}", modelData.materials.size());
            std::println("light count: {}", modelData.lights.size());
            std::println("camera count: {}", modelData.cameras.size());

            usize meshCount = 0;
            for (const MeshGroup& meshGroup : modelData.meshGroups) {
                meshCount += meshGroup.meshes.size();
            }
            std::println("mesh count: {}", meshCount);

            modelData.modelFilepath = filepath;
            modelData.asset = std::make_unique<GltfAsset>(std::move(asset));
            return modelData;
        }

        auto read_file_to_bytes(const std::filesystem::path& file_path) -> std::vector<std::byte> {
            if (!std::filesystem::exists(file_path)) {
                throw std::runtime_error("file hasnt been found: " + file_path.string());
            }

            std::ifstream file(file_path, std::ios::binary);
            auto size = std::filesystem::file_size(file_path);
            std::vector<std::byte> data = {};
            data.resize(size);
            file.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(size));

            return data;
        }

        struct BufferDataAdapter {
            std::filesystem::path root_path = {};
            std::vector<std::byte> operator()(const fastgltf::Asset& asset, const std::size_t bufferViewIdx) const {
                const auto& bufferView = asset.bufferViews[bufferViewIdx];
                fastgltf::Buffer const& gltfBuffer = asset.buffers.at(bufferView.bufferIndex);

                return {};
            }
        };

        ImageData GLTFModelLoader::ReadImageData(const ModelData& modelData, const Image& image) {
            const fastgltf::Asset& asset = static_cast<const GltfAsset*>(modelData.asset.get())->asset;
            const fastgltf::Image& gltfImage = asset.images[image.imageIndex];

            auto getGltfData = [&](this auto&& self, const fastgltf::DataSource& data) -> std::vector<std::byte> {
                return std::visit(
                    fastgltf::visitor{
                        [&](const std::monostate&) -> std::vector<std::byte> {
                            ASSERT(false, "std::monostate should never happen");
                            return {};
                        },
                        [&](const fastgltf::sources::BufferView& source) -> std::vector<std::byte> {
                            const fastgltf::BufferView& bufferView = asset.bufferViews[source.bufferViewIndex];
                            const fastgltf::Buffer& buffer = asset.buffers[bufferView.bufferIndex];
                            std::vector<std::byte> bufferData = self(buffer.data);

                            std::vector<std::byte> ret = {};
                            ret.resize(bufferView.byteLength);
                            std::memcpy(ret.data(), bufferData.data() + bufferView.byteOffset, bufferView.byteLength);
                            return ret;
                        },
                        [&](const fastgltf::sources::URI& source) -> std::vector<std::byte> {
                            std::filesystem::path path{ source.uri.path().begin(), source.uri.path().end() };
                            if (source.uri.isLocalPath()) {
                                path = modelData.modelFilepath.parent_path() / path;
                            }
                            return read_file_to_bytes(path);
                        },
                        [&](const fastgltf::sources::Array& source) -> std::vector<std::byte> {
                            std::vector<std::byte> ret = {};
                            ret.resize(source.bytes.size_bytes());
                            std::memcpy(ret.data(), source.bytes.data(), source.bytes.size_bytes());
                            return ret;
                        },
                        [&](const fastgltf::sources::Vector& source) -> std::vector<std::byte> {
                            std::vector<std::byte> ret = {};
                            ret.resize(source.bytes.size());
                            std::memcpy(ret.data(), source.bytes.data(), source.bytes.size());
                            return ret;
                        },
                        [&](const fastgltf::sources::CustomBuffer& /* source */) -> std::vector<std::byte> {
                            ASSERT(false, "fastgltf::sources::CustomBuffer isnt handled");
                            return {};
                        },
                        [&](const fastgltf::sources::ByteView& source) -> std::vector<std::byte> {
                            std::vector<std::byte> ret = {};
                            ret.resize(source.bytes.size());
                            std::memcpy(ret.data(), source.bytes.data(), source.bytes.size());
                            return ret;
                        },
                        [&](const fastgltf::sources::Fallback& /* source */) -> std::vector<std::byte> {
                            ASSERT(false, "fastgltf::sources::Fallback isnt handled");
                            return {};
                        },
                    },
                    data);
            };

            ImageData imageData = {};
            i32 numChannels = 0;
            auto gltfImageData = getGltfData(gltfImage.data);
            uint8_t* data = stbi_load_from_memory(reinterpret_cast<const uint8_t*>(gltfImageData.data()), static_cast<i32>(gltfImageData.size()), reinterpret_cast<i32*>(&imageData.width), reinterpret_cast<i32*>(&imageData.height), &numChannels, 4);

            if (!data) {
                std::println("Failed to open image file");
            }

            imageData.data.resize(static_cast<u64>(imageData.width * imageData.height * 4));
            std::memcpy(imageData.data.data(), data, static_cast<u64>(imageData.width * imageData.height * 4));
            stbi_image_free(data);

            return imageData;
        }

        MeshData GLTFModelLoader::ReadMeshData(const ModelData& modelData, const Mesh& mesh) {
            const fastgltf::Asset& asset = static_cast<const GltfAsset*>(modelData.asset.get())->asset;
            const fastgltf::Mesh& gltfMesh = asset.meshes[mesh.meshGroupIndex];
            const fastgltf::Primitive& gltfPrimitive = gltfMesh.primitives[mesh.meshIndex];

            // BufferDataAdapter adapter = { .root_path = modelData.modelFilepath.root_path() };
            fastgltf::DefaultBufferDataAdapter adapter = {};

            MeshData meshData = {};
            if (const fastgltf::Attribute* attribute = gltfPrimitive.findAttribute("POSITION"); attribute != gltfPrimitive.attributes.end()) {
                const fastgltf::Accessor& accessor = asset.accessors[attribute->accessorIndex];

                meshData.positions.resize(accessor.count);
                fastgltf::copyFromAccessor<glm::vec3>(asset, accessor, meshData.positions.data(), adapter);
            }

            if (const fastgltf::Attribute* attribute = gltfPrimitive.findAttribute("NORMAL"); attribute != gltfPrimitive.attributes.end()) {
                const fastgltf::Accessor& accessor = asset.accessors[attribute->accessorIndex];

                meshData.normals.resize(accessor.count);
                fastgltf::copyFromAccessor<glm::vec3>(asset, accessor, meshData.normals.data(), adapter);
            }

            if (const fastgltf::Attribute* attribute = gltfPrimitive.findAttribute("TANGENT"); attribute != gltfPrimitive.attributes.end()) {
                const fastgltf::Accessor& accessor = asset.accessors[attribute->accessorIndex];

                meshData.tangents.resize(accessor.count);
                fastgltf::copyFromAccessor<glm::vec4>(asset, accessor, meshData.tangents.data(), adapter);
            }

            {
                uint32_t count = 0;
                while (true) {
                    const fastgltf::Attribute* attribute = gltfPrimitive.findAttribute("TEXCOORD_" + std::to_string(count));
                    if (attribute == gltfPrimitive.attributes.end() || attribute == nullptr) {
                        break;
                    }
                    const fastgltf::Accessor& accessor = asset.accessors[attribute->accessorIndex];

                    std::vector<glm::vec2> uvs = {};
                    uvs.resize(accessor.count);
                    ASSERT(accessor.componentType == fastgltf::ComponentType::Float, "only supporting floats");
                    fastgltf::copyFromAccessor<glm::vec2>(asset, accessor, uvs.data(), adapter);
                    meshData.uvs.push_back(uvs);
                    count++;
                }
            }

            ASSERT(gltfPrimitive.findAttribute("COLOR_0") == gltfPrimitive.attributes.end(), "todo vertex color");
            ASSERT(gltfPrimitive.findAttribute("JOINTS_0") == gltfPrimitive.attributes.end(), "todo joint");
            ASSERT(gltfPrimitive.findAttribute("WEIGHTS_0") == gltfPrimitive.attributes.end(), "todo weight");

            if (gltfPrimitive.indicesAccessor.has_value()) {
                const fastgltf::Accessor& accessor = asset.accessors[gltfPrimitive.indicesAccessor.value()];
                meshData.indices.resize(accessor.count);

                if (accessor.componentType == fastgltf::ComponentType::UnsignedInt) {
                    fastgltf::iterateAccessorWithIndex<uint32_t>(asset, accessor, [&](uint32_t value, usize index) { meshData.indices[index] = value; }, adapter);
                } else if (accessor.componentType == fastgltf::ComponentType::UnsignedShort) {
                    fastgltf::iterateAccessorWithIndex<u16>(asset, accessor, [&](u16 value, usize index) { meshData.indices[index] = static_cast<uint32_t>(value); }, adapter);
                } else if (accessor.componentType == fastgltf::ComponentType::UnsignedByte) {
                    fastgltf::iterateAccessorWithIndex<uint8_t>(asset, accessor, [&](uint8_t value, usize index) { meshData.indices[index] = static_cast<uint32_t>(value); }, adapter);
                }
            }

            std::println("positions count: {}", meshData.positions.size());
            std::println("normals count: {}", meshData.normals.size());
            std::println("tangents count: {}", meshData.tangents.size());
            std::println("uv sets: {} uv count: {}", meshData.uvs.size(), meshData.uvs.size() != 0 ? meshData.uvs[0].size() : 0);
            std::println("colors count: {}", meshData.colors.size());
            std::println("joints count: {}", meshData.joints.size());
            std::println("weights count: {}", meshData.weights.size());
            std::println("index count: {}", meshData.indices.size());

            return meshData;
        }
} // namespace devs_out_of_bounds
