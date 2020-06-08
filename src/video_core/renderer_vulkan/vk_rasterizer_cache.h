#pragma once

#include <memory>

#include "video_core/renderer_vulkan/vk_instance.h"

#include "video_core/renderer_opengl/gl_resource_manager.h"
#include "video_core/renderer_opengl/gl_surface_params.h"

#include "video_core/texture/texture_decode.h"

namespace Vulkan {

class Win32SmartHandle : private NonCopyable {
    HANDLE handle = 0;

public:
    Win32SmartHandle() = default;
    Win32SmartHandle(HANDLE&& handle) : handle{handle} {}

    ~Win32SmartHandle() {
        Release();
    }

    void operator=(HANDLE&& handle) {
        Release();
        this->handle = handle;
    }

    operator HANDLE() {
        return handle;
    }

    /// Deletes the internal OpenGL resource
    void Release() {
        if (handle > 0)
            CloseHandle(handle);
    }
};


struct Instance;
struct CachedSurface;
using OpenGL::SurfaceInterval;
using OpenGL::SurfaceParams;
class RasterizerCacheVulkan;
class BufferToImageConverter;
class ImageToBufferConverter;

struct TextureCubeConfig {
    PAddr px;
    PAddr nx;
    PAddr py;
    PAddr ny;
    PAddr pz;
    PAddr nz;
    u32 width;
    Pica::TexturingRegs::TextureFormat format;

    bool operator==(const TextureCubeConfig& rhs) const {
        return std::tie(px, nx, py, ny, pz, nz, width, format) ==
               std::tie(rhs.px, rhs.nx, rhs.py, rhs.ny, rhs.pz, rhs.nz, rhs.width, rhs.format);
    }

    bool operator!=(const TextureCubeConfig& rhs) const {
        return !(*this == rhs);
    }
};

enum class ScaleMatch {
    Exact,   // only accept same res scale
    Upscale, // only allow higher scale than params
    Ignore   // accept every scaled res
};

struct CachedTextureCube {
    OpenGL::OGLTexture texture;
};

struct CachedSurface : SurfaceParams, std::enable_shared_from_this<CachedSurface> {
    CachedSurface(RasterizerCacheVulkan& owner, const SurfaceParams& surface_params);
    CachedSurface(RasterizerCacheVulkan& owner, const GPU::Regs::MemoryFillConfig& config);
    ~CachedSurface();
    RasterizerCacheVulkan& owner;
    friend class RasterizerCacheVulkan;
    vk::UniqueImage image;
    vk::UniqueDeviceMemory image_memory;
    Win32SmartHandle shmem_handle;

    OpenGL::OGLTexture texture;
    OpenGL::OGLMemoryObject gl_memory_object;

    u8 fill_size;
    std::array<u8, 4> fill_data;
};

using Surface = std::shared_ptr<CachedSurface>;

class RasterizerCacheVulkan : NonCopyable {
public:
    RasterizerCacheVulkan(Instance& vk_inst);
    ~RasterizerCacheVulkan();

    Instance& vk_inst;
    struct MemoryRegion {
        vk::UniqueBuffer buffer;
        vk::UniqueDeviceMemory gpu_memory;
        u8* ptr;
    } fcram, vram;

    vk::DeviceSize min_flush;
    std::unique_ptr<BufferToImageConverter> buffer_to_image;
    std::unique_ptr<ImageToBufferConverter> image_to_buffer;

    /// Blit one surface's texture to another
    bool BlitSurfaces(const Surface& src_surface, const Common::Rectangle<u32>& src_rect,
                      const Surface& dst_surface, const Common::Rectangle<u32>& dst_rect);

    /// Copy one surface's region to another
    void CopySurface(const Surface& src_surface, const Surface& dst_surface,
                     SurfaceInterval copy_interval);

    /// Load a texture from 3DS memory to OpenGL and cache it (if not already cached)
    Surface GetSurface(const SurfaceParams& params, ScaleMatch match_res_scale,
                       bool load_if_create);

    /// Attempt to find a subrect (resolution scaled) of a surface, otherwise loads a texture from
    /// 3DS memory to OpenGL and caches it (if not already cached)
    std::tuple<Surface, Common::Rectangle<u32>> GetSurfaceSubRect(const SurfaceParams& params,
                                                                  ScaleMatch match_res_scale,
                                                                  bool load_if_create);

    /// Get a surface based on the texture configuration
    Surface GetTextureSurface(const Pica::TexturingRegs::FullTextureConfig& config);
    Surface GetTextureSurface(const Pica::Texture::TextureInfo& info, u32 max_level = 0);

    /// Get a texture cube based on the texture configuration
    const CachedTextureCube& GetTextureCube(const TextureCubeConfig& config);

    /// Get the color and depth surfaces based on the framebuffer configuration
    std::tuple<Surface, Surface, Common::Rectangle<u32>> GetFramebufferSurfaces(
        bool using_color_fb, bool using_depth_fb, const Common::Rectangle<s32>& viewport_rect);

    /// Get a surface that matches the fill config
    Surface GetFillSurface(const GPU::Regs::MemoryFillConfig& config);

    /// Get a surface that matches a "texture copy" display transfer config
    std::tuple<Surface, Common::Rectangle<u32>> GetTexCopySurface(const SurfaceParams& params);

    /// Write any cached resources overlapping the region back to memory (if dirty)
    void FlushRegion(PAddr addr, u32 size, Surface flush_surface = nullptr);

    /// Mark region as being invalidated by region_owner (nullptr if 3DS memory)
    void InvalidateRegion(PAddr addr, u32 size, const Surface& region_owner);

    /// Flush all cached resources tracked by this cache manager
    void FlushAll();

    /// Clear all cached resources tracked by this cache manager
    void ClearAll(bool flush);
    std::tuple<MemoryRegion&, vk::DeviceSize> GetBufferOffset(PAddr addr);
};

inline GLenum OpenGLPixelFormat(OpenGL::SurfaceParams::PixelFormat format) {
    using PX = OpenGL::SurfaceParams::PixelFormat;
    switch (format) {
    case PX::RGBA8:
    // needs conversion
    case PX::RGB8:
    case PX::RGBA4:
    case PX::RGB5A1:
    case PX::RGB565:
    case PX::ETC1:
    case PX::ETC1A4:
        return GL_RGBA8;
    case PX::IA8:
    case PX::RG8:
    case PX::IA4:
        return GL_RG8;
    case PX::I8:
    case PX::A8:
    case PX::A4:
    case PX::I4:
        return GL_R8;
    case PX::D16:
        return GL_DEPTH_COMPONENT16;
    case PX::D24:
        return GL_DEPTH_COMPONENT24;
    case PX::D24S8:
        return GL_DEPTH24_STENCIL8;
    default:
        return NULL;
    }
}

inline vk::Format VulkanIntFormat(vk::Format fmt) {
    switch (fmt) {
    case vk::Format::eR8G8B8A8Unorm:
        return vk::Format::eR8G8B8A8Uint;
    case vk::Format::eR8G8Unorm:
        return vk::Format::eR8G8Uint;
    case vk::Format::eR8Unorm:
        return vk::Format::eR8Uint;
    default:
        UNREACHABLE();
    }
}

inline vk::Format VulkanPixelFormat(OpenGL::SurfaceParams::PixelFormat format) {
    using PX = OpenGL::SurfaceParams::PixelFormat;
    switch (format) {
    case PX::RGBA8:
        // needs conversion
    case PX::RGB8:
    case PX::RGBA4:
    case PX::RGB5A1:
    case PX::RGB565:
    case PX::ETC1:
    case PX::ETC1A4:
        return vk::Format::eR8G8B8A8Unorm;
    case PX::IA8:
    case PX::RG8:
    case PX::IA4:
        return vk::Format::eR8G8Unorm;
    case PX::I8:
    case PX::A8:
    case PX::A4:
    case PX::I4:
        return vk::Format::eR8Unorm;
    case PX::D16:
        return vk::Format::eD16Unorm;
    case PX::D24:
        return vk::Format::eD24UnormS8Uint;
    case PX::D24S8:
        return vk::Format::eD24UnormS8Uint;
    default:
        LOG_CRITICAL(Render_OpenGL, "Unimplemented pixel format {}",
                     OpenGL::SurfaceParams::PixelFormatAsString(format));
        return vk::Format::eUndefined;
    }
}
} // namespace Vulkan
