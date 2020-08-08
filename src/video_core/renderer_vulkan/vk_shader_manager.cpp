// Copyright 2020 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <algorithm>
#include <thread>
#include <unordered_map>
#include <variant>
#include <boost/functional/hash.hpp>

#include <shaderc/shaderc.hpp>

#include "core/core.h"
#include "video_core/renderer_vulkan/vk_shader_manager.h"
#include "video_core/video_core.h"

#include "video_core/renderer_vulkan/vk_instance.h"

namespace Vulkan {

    static u64 GetUniqueIdentifier(const Pica::Regs& regs, const ProgramCode& code) {
        std::size_t hash = 0;
        u64 regs_uid = Common::ComputeHash64(regs.reg_array.data(), Pica::Regs::NUM_REGS * sizeof(u32));
        boost::hash_combine(hash, regs_uid);
        if (code.size() > 0) {
            u64 code_uid = Common::ComputeHash64(code.data(), code.size() * sizeof(u32));
            boost::hash_combine(hash, code_uid);
        }
        return static_cast<u64>(hash);
    }

    void PicaUniformsData::SetFromRegs(const Pica::ShaderRegs& regs,
        const Pica::Shader::ShaderSetup& setup) {
        std::transform(std::begin(setup.uniforms.b), std::end(setup.uniforms.b), std::begin(bools),
            [](bool value) -> BoolAligned { return { value ? GL_TRUE : GL_FALSE }; });
        std::transform(std::begin(regs.int_uniforms), std::end(regs.int_uniforms), std::begin(i),
            [](const auto& value) -> GLuvec4 {
                return { value.x.Value(), value.y.Value(), value.z.Value(), value.w.Value() };
            });
        std::transform(std::begin(setup.uniforms.f), std::end(setup.uniforms.f), std::begin(f),
            [](const auto& value) -> GLvec4 {
                return { value.x.ToFloat32(), value.y.ToFloat32(), value.z.ToFloat32(),
                        value.w.ToFloat32() };
            });
    }

    class TrivialVertexShader {
    public:
        explicit TrivialVertexShader() {
            auto src = GenerateTrivialVertexShader();
        }
        vk::ShaderModule Get() const {
            return *program;
        }

    private:
        vk::UniqueShaderModule program;
    };

    template <typename KeyConfigType,
        ShaderDecompiler::ProgramResult(*CodeGenerator)(const KeyConfigType&),
        GLenum ShaderType>
        class ShaderCache {
        public:
            explicit ShaderCache() : {}
            std::tuple<GLuint, std::optional<ShaderDecompiler::ProgramResult>> Get(
                const KeyConfigType& config) {
                auto [iter, new_shader] = shaders.emplace(config, OGLShaderStage{ separable });
                OGLShaderStage& cached_shader = iter->second;
                std::optional<ShaderDecompiler::ProgramResult> result{};
                if (new_shader) {
                    result = CodeGenerator(config, separable);
                    cached_shader.Create(result->code.c_str(), ShaderType);
                }
                return { cached_shader.GetHandle(), std::move(result) };
            }

            void Inject(const KeyConfigType& key, OGLProgram&& program) {
                OGLShaderStage stage{ separable };
                stage.Inject(std::move(program));
                shaders.emplace(key, std::move(stage));
            }

        private:
            std::unordered_map<KeyConfigType, vk::ShaderModule> shaders;
    };

    // This is a cache designed for shaders translated from PICA shaders. The first cache matches the
    // config structure like a normal cache does. On cache miss, the second cache matches the generated
    // GLSL code. The configuration is like this because there might be leftover code in the PICA shader
    // program buffer from the previous shader, which is hashed into the config, resulting several
    // different config values from the same shader program.
    template <typename KeyConfigType,
        std::optional<ShaderDecompiler::ProgramResult>(*CodeGenerator)(
            const Pica::Shader::ShaderSetup&, const KeyConfigType&),
        GLenum ShaderType>
        class ShaderDoubleCache {
        public:
            explicit ShaderDoubleCache() : {}
            std::tuple<GLuint, std::optional<ShaderDecompiler::ProgramResult>> Get(
                const KeyConfigType& key, const Pica::Shader::ShaderSetup& setup) {
                std::optional<ShaderDecompiler::ProgramResult> result{};
                auto map_it = shader_map.find(key);
                if (map_it == shader_map.end()) {
                    auto program_opt = CodeGenerator(setup, key, separable);
                    if (!program_opt) {
                        shader_map[key] = nullptr;
                        return { 0, std::nullopt };
                    }

                    std::string& program = program_opt->code;
                    auto [iter, new_shader] = shader_cache.emplace(program, OGLShaderStage{ separable });
                    vk::ShaderModule& cached_shader = iter->second;
                    if (new_shader) {
                        result->code = program;
                        cached_shader.Create(program.c_str(), ShaderType);
                    }
                    shader_map[key] = &cached_shader;
                    return { cached_shader.GetHandle(), std::move(result) };
                }

                if (map_it->second == nullptr) {
                    return { 0, std::nullopt };
                }

                return { map_it->second->GetHandle(), std::nullopt };
            }

        private:
            std::unordered_map<KeyConfigType, vk::ShaderModule*> shader_map;
            std::unordered_map<std::string, vk::ShaderModule> shader_cache;
    };

    using ProgrammableVertexShaders =
        ShaderDoubleCache<PicaVSConfig, &GenerateVertexShader, GL_VERTEX_SHADER>;

    using FixedGeometryShaders =
        ShaderCache<PicaFixedGSConfig, &GenerateFixedGeometryShader, GL_GEOMETRY_SHADER>;

    using FragmentShaders = ShaderCache<PicaFSConfig, &GenerateFragmentShader, GL_FRAGMENT_SHADER>;

    class ShaderProgramManager::Impl {
    public:
        explicit Impl()
            : programmable_vertex_shaders(),
            trivial_vertex_shader(), fixed_geometry_shaders(),
            fragment_shaders() {}

        struct ShaderTuple {
            GLuint vs = 0;
            GLuint gs = 0;
            GLuint fs = 0;

            bool operator==(const ShaderTuple& rhs) const {
                return std::tie(vs, gs, fs) == std::tie(rhs.vs, rhs.gs, rhs.fs);
            }

            bool operator!=(const ShaderTuple& rhs) const {
                return std::tie(vs, gs, fs) != std::tie(rhs.vs, rhs.gs, rhs.fs);
            }

            struct Hash {
                std::size_t operator()(const ShaderTuple& tuple) const {
                    std::size_t hash = 0;
                    boost::hash_combine(hash, tuple.vs);
                    boost::hash_combine(hash, tuple.gs);
                    boost::hash_combine(hash, tuple.fs);
                    return hash;
                }
            };
        };

        ShaderTuple current;

        ProgrammableVertexShaders programmable_vertex_shaders;
        TrivialVertexShader trivial_vertex_shader;

        FixedGeometryShaders fixed_geometry_shaders;

        FragmentShaders fragment_shaders;
    };

    ShaderProgramManager::ShaderProgramManager()
        : impl(std::make_unique<Impl>()) {}

    ShaderProgramManager::~ShaderProgramManager() = default;

    bool ShaderProgramManager::UseProgrammableVertexShader(const Pica::Regs& regs,
        Pica::Shader::ShaderSetup& setup) {
        PicaVSConfig config{ regs.vs, setup };
        auto [handle, result] = impl->programmable_vertex_shaders.Get(config, setup);
        if (handle == 0)
            return false;
        impl->current.vs = handle;
        // Save VS to the disk cache if its a new shader
        if (result) {
            ProgramCode program_code{ setup.program_code.begin(), setup.program_code.end() };
            program_code.insert(program_code.end(), setup.swizzle_data.begin(),
                setup.swizzle_data.end());
            const u64 unique_identifier = GetUniqueIdentifier(regs, program_code);
        }
        return true;
    }

    void ShaderProgramManager::UseTrivialVertexShader() {}

    void ShaderProgramManager::UseFixedGeometryShader(const Pica::Regs& regs) {

    }

    void ShaderProgramManager::UseTrivialGeometryShader() {

    }

    void ShaderProgramManager::UseFragmentShader(const Pica::Regs& regs) {

    }

} // namespace Vulkan
