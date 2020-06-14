#include <unordered_map>

#include "video_core/pica_state.h"
#include "video_core/renderer_opengl/gl_state.h"
#include "video_core/renderer_vulkan/vk_buffer_to_image.h"
#include "video_core/renderer_vulkan/vk_image_to_buffer.h"
#include "video_core/renderer_vulkan/vk_rasterizer_cache.h"
#include "video_core/video_core.h"

#include "common/alignment.h"

namespace Vulkan {

RasterizerCacheVulkan::RasterizerCacheVulkan(Instance& vk_inst) : vk_inst{vk_inst} {
    min_flush = vk_inst.physical_device.getProperties().limits.nonCoherentAtomSize;

    const auto init_mem_region = [&vk_inst](MemoryRegion& region, vk::DeviceSize size) {
        vk::BufferCreateInfo buffer_create_info;
        buffer_create_info.sharingMode = vk::SharingMode::eExclusive;
        buffer_create_info.usage = vk::BufferUsageFlagBits::eTransferSrc |
                                   vk::BufferUsageFlagBits::eTransferDst |
                                   vk::BufferUsageFlagBits::eStorageBuffer;
        buffer_create_info.size = size;
        region.buffer = vk_inst.device->createBufferUnique(buffer_create_info);

        vk::MemoryAllocateInfo allocation_info;
        auto memory_requirements = vk_inst.device->getBufferMemoryRequirements(*region.buffer);
        allocation_info.allocationSize = memory_requirements.size;
        allocation_info.memoryTypeIndex = vk_inst.getMemoryType(
            memory_requirements.memoryTypeBits,
            vk::MemoryPropertyFlagBits::eHostVisible);

        region.gpu_memory = vk_inst.device->allocateMemoryUnique(allocation_info);
        vk_inst.device->bindBufferMemory(*region.buffer, *region.gpu_memory, 0);
        region.ptr = reinterpret_cast<u8*>(vk_inst.device->mapMemory(*region.gpu_memory, 0, size));
    };
    init_mem_region(vram, Memory::VRAM_SIZE);
    init_mem_region(fcram, Memory::FCRAM_N3DS_SIZE);

    buffer_to_image = std::make_unique<BufferToImageConverter>(vk_inst);
    image_to_buffer = std::make_unique<ImageToBufferConverter>(vk_inst);
}

RasterizerCacheVulkan::~RasterizerCacheVulkan() {}

bool RasterizerCacheVulkan::BlitSurfaces(const Surface& src_surface,
                                         const Common::Rectangle<u32>& src_rect,
                                         const Surface& dst_surface,
                                         const Common::Rectangle<u32>& dst_rect) {

    if (!SurfaceParams::CheckFormatsBlittable(src_surface->pixel_format, dst_surface->pixel_format))
        return false;

    vk::ImageSubresourceLayers image_range;
    image_range.baseArrayLayer = 0;
    image_range.layerCount = 1;
    switch (src_surface->type) {
    case SurfaceParams::SurfaceType::Color:
    case SurfaceParams::SurfaceType::Texture:
        image_range.aspectMask = vk::ImageAspectFlagBits::eColor;
        break;
    case SurfaceParams::SurfaceType::Depth:
        image_range.aspectMask = vk::ImageAspectFlagBits::eDepth;
        break;
    default:
        UNIMPLEMENTED();
    }

    vk::ImageMemoryBarrier src_barrier, dst_barrier;
    src_barrier.image = *src_surface->image;
    src_barrier.subresourceRange = SubResourceLayersToRange(image_range);
    src_barrier.oldLayout = vk::ImageLayout::eGeneral;
    src_barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
    src_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    src_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    dst_barrier = src_barrier;
    dst_barrier.image = *dst_surface->image;
    dst_barrier.newLayout = vk::ImageLayout::eTransferDstOptimal;

    vk::ImageBlit blit_area;
    blit_area.srcSubresource = image_range;
    blit_area.srcOffsets[0] = vk::Offset3D(src_rect.left, src_rect.bottom, 0);
    blit_area.srcOffsets[1] = vk::Offset3D(src_rect.right, src_rect.top, 1);
    blit_area.dstSubresource = image_range;
    blit_area.dstOffsets[0] = vk::Offset3D(dst_rect.left, dst_rect.bottom, 0);
    blit_area.dstOffsets[1] = vk::Offset3D(dst_rect.right, dst_rect.top, 1);

    vk::CommandBufferAllocateInfo command_buffer_allocate_info;
    command_buffer_allocate_info.commandBufferCount = 1;
    command_buffer_allocate_info.commandPool = *vk_inst.command_pool;
    command_buffer_allocate_info.level = vk::CommandBufferLevel::ePrimary;

    vk::CommandBufferBeginInfo command_buffer_begin;
    command_buffer_begin.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
    auto command_buffer =
        std::move(vk_inst.device->allocateCommandBuffersUnique(command_buffer_allocate_info)[0]);
    command_buffer->begin(command_buffer_begin);
    command_buffer->pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe,
                                    vk::PipelineStageFlagBits::eTransfer, {}, {}, {},
                                    {src_barrier, dst_barrier});
    command_buffer->blitImage(*src_surface->image, vk::ImageLayout::eTransferSrcOptimal,
                              *dst_surface->image, vk::ImageLayout::eTransferDstOptimal,
                              {blit_area}, vk::Filter::eNearest);
    std::swap(src_barrier.oldLayout, src_barrier.newLayout);
    std::swap(dst_barrier.oldLayout, dst_barrier.newLayout);
    command_buffer->pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                                    vk::PipelineStageFlagBits::eBottomOfPipe, {}, {}, {},
                                    {src_barrier, dst_barrier});
    command_buffer->end();

    vk_inst.SubmitCommandBuffers(*command_buffer);
    vk_inst.device->waitIdle();
    return true;
}

void RasterizerCacheVulkan::CopySurface(const Surface& src_surface, const Surface& dst_surface,
                                        SurfaceInterval copy_interval){
    LOG_CRITICAL(Render_OpenGL, "CopySurface unimplemented");
}

Surface RasterizerCacheVulkan::GetSurface(const SurfaceParams& params, ScaleMatch match_res_scale,
                                          bool load_if_create) {
    if (params.addr == 0 || params.height * params.width == 0) {
        return nullptr;
    }
    // Use GetSurfaceSubRect instead
    ASSERT(params.width == params.stride);

    ASSERT(!params.is_tiled || (params.width % 8 == 0 && params.height % 8 == 0));
    auto surface = std::make_shared<CachedSurface>(*this, params);
    return surface;
}

std::tuple<Surface, Common::Rectangle<u32>> RasterizerCacheVulkan::GetSurfaceSubRect(
    const SurfaceParams& params, ScaleMatch match_res_scale, bool load_if_create) {
    if (params.addr == 0 || params.height * params.width == 0) {
        return std::make_tuple(nullptr, Common::Rectangle<u32>{});
    }

    SurfaceParams aligned_params = params;
    if (params.is_tiled) {
        aligned_params.height = Common::AlignUp(params.height, 8);
        aligned_params.width = Common::AlignUp(params.width, 8);
        aligned_params.stride = Common::AlignUp(params.stride, 8);
        aligned_params.UpdateParams();
    }

    SurfaceParams new_params = aligned_params;
    // Can't have gaps in a surface
    new_params.width = aligned_params.stride;
    new_params.UpdateParams();
    // GetSurface will create the new surface and possibly adjust res_scale if necessary
    Surface surface = GetSurface(new_params, match_res_scale, load_if_create);
    return {surface, surface->GetScaledSubRect(params)};
}

Surface RasterizerCacheVulkan::GetTextureSurface(
    const Pica::TexturingRegs::FullTextureConfig& config) {
    Pica::Texture::TextureInfo info =
        Pica::Texture::TextureInfo::FromPicaRegister(config.config, config.format);
    return GetTextureSurface(info, config.config.lod.max_level);
}

Surface RasterizerCacheVulkan::GetTextureSurface(const Pica::Texture::TextureInfo& info,
                                                 u32 max_level) {
    if (info.physical_address == 0) {
        return nullptr;
    }

    SurfaceParams params;
    params.addr = info.physical_address;
    params.width = info.width;
    params.height = info.height;
    params.is_tiled = true;
    params.pixel_format = SurfaceParams::PixelFormatFromTextureFormat(info.format);
    params.res_scale = 1; // res_scale
    params.UpdateParams();

    if (VulkanPixelFormat(params.pixel_format) == vk::Format::eUndefined)
        return nullptr;

    u32 min_width = info.width >> max_level;
    u32 min_height = info.height >> max_level;
    if (min_width % 8 != 0 || min_height % 8 != 0) {
        LOG_CRITICAL(Render_OpenGL, "Texture size ({}x{}) is not multiple of 8", min_width,
                     min_height);
        return nullptr;
    }
    if (info.width != (min_width << max_level) || info.height != (min_height << max_level)) {
        LOG_CRITICAL(Render_OpenGL,
                     "Texture size ({}x{}) does not support required mipmap level ({})",
                     params.width, params.height, max_level);
        return nullptr;
    }

    auto surface = GetSurface(params, ScaleMatch::Ignore, true);
    if (!surface)
        return nullptr;

    // TODO: mipmaps

    return surface;
}

const CachedTextureCube& RasterizerCacheVulkan::GetTextureCube(const TextureCubeConfig& config) {
    static CachedTextureCube x{};
    return x;
}

std::tuple<Surface, Surface, Common::Rectangle<u32>> RasterizerCacheVulkan::GetFramebufferSurfaces(
    bool using_color_fb, bool using_depth_fb, const Common::Rectangle<s32>& viewport_rect) {
    const auto& regs = Pica::g_state.regs;
    const auto& config = regs.framebuffer.framebuffer;

    Common::Rectangle<u32> viewport_clamped{
        static_cast<u32>(std::clamp(viewport_rect.left, 0, static_cast<s32>(config.GetWidth()))),
        static_cast<u32>(std::clamp(viewport_rect.top, 0, static_cast<s32>(config.GetHeight()))),
        static_cast<u32>(std::clamp(viewport_rect.right, 0, static_cast<s32>(config.GetWidth()))),
        static_cast<u32>(
            std::clamp(viewport_rect.bottom, 0, static_cast<s32>(config.GetHeight())))};

    SurfaceParams color_params;
    color_params.is_tiled = true;
    color_params.res_scale = 1; // res_scale
    color_params.width = config.GetWidth();
    color_params.height = config.GetHeight();
    SurfaceParams depth_params = color_params;

    color_params.addr = config.GetColorBufferPhysicalAddress();
    color_params.pixel_format = SurfaceParams::PixelFormatFromColorFormat(config.color_format);
    color_params.UpdateParams();

    depth_params.addr = config.GetDepthBufferPhysicalAddress();
    depth_params.pixel_format = SurfaceParams::PixelFormatFromDepthFormat(config.depth_format);
    depth_params.UpdateParams();

    auto color_vp_interval = color_params.GetSubRectInterval(viewport_clamped);
    auto depth_vp_interval = depth_params.GetSubRectInterval(viewport_clamped);

    // Make sure that framebuffers don't overlap if both color and depth are being used
    if (using_color_fb && using_depth_fb &&
        boost::icl::length(color_vp_interval & depth_vp_interval)) {
        LOG_CRITICAL(Render_OpenGL, "Color and depth framebuffer memory regions overlap; "
                                    "overlapping framebuffers not supported!");
        using_depth_fb = false;
    }

    Common::Rectangle<u32> color_rect{};
    Surface color_surface = nullptr;
    if (using_color_fb)
        std::tie(color_surface, color_rect) =
            GetSurfaceSubRect(color_params, ScaleMatch::Exact, false);

    Common::Rectangle<u32> depth_rect{};
    Surface depth_surface = nullptr;
    if (using_depth_fb)
        std::tie(depth_surface, depth_rect) =
            GetSurfaceSubRect(depth_params, ScaleMatch::Exact, false);

    Common::Rectangle<u32> fb_rect{};
    if (color_surface != nullptr && depth_surface != nullptr) {
        fb_rect = color_rect;
        // Color and Depth surfaces must have the same dimensions and offsets
        if (color_rect.bottom != depth_rect.bottom || color_rect.top != depth_rect.top ||
            color_rect.left != depth_rect.left || color_rect.right != depth_rect.right) {
            color_surface = GetSurface(color_params, ScaleMatch::Exact, false);
            depth_surface = GetSurface(depth_params, ScaleMatch::Exact, false);
            fb_rect = color_surface->GetScaledRect();
        }
    } else if (color_surface != nullptr) {
        fb_rect = color_rect;
    } else if (depth_surface != nullptr) {
        fb_rect = depth_rect;
    }

    return {color_surface, depth_surface, fb_rect};
}

Surface RasterizerCacheVulkan::GetFillSurface(const GPU::Regs::MemoryFillConfig& config) {
    return std::make_shared<CachedSurface>(*this, config);
}

std::tuple<Surface, Common::Rectangle<u32>> RasterizerCacheVulkan::GetTexCopySurface(
    const SurfaceParams& params) {
    return {nullptr, {}};
}

void RasterizerCacheVulkan::FlushRegion(PAddr addr, u32 size, Surface flush_surface) {}

void RasterizerCacheVulkan::InvalidateRegion(PAddr addr, u32 size, const Surface& region_owner) {}

void RasterizerCacheVulkan::FlushAll() {}

void RasterizerCacheVulkan::ClearAll(bool flush) {}

std::tuple<RasterizerCacheVulkan::MemoryRegion&, vk::DeviceSize>
RasterizerCacheVulkan::GetBufferOffset(PAddr addr) {
    if (boost::icl::contains(boost::icl::right_open_interval<PAddr>{Memory::FCRAM_PADDR,
                                                                    Memory::FCRAM_N3DS_PADDR_END},
                             addr)) {
        return {fcram, addr - Memory::FCRAM_PADDR};
    } else if (boost::icl::contains(boost::icl::right_open_interval<PAddr>{Memory::VRAM_PADDR,
                                                                           Memory::VRAM_PADDR_END},
                                    addr)) {
        return {vram, addr - Memory::VRAM_PADDR};
    } else {
        UNREACHABLE();
    }
}

CachedSurface::CachedSurface(RasterizerCacheVulkan& owner, const SurfaceParams& surface_params)
    : owner{owner} {
    static_cast<SurfaceParams&>(*this) = surface_params;

    using PX = OpenGL::SurfaceParams::PixelFormat;

    vk::ImageCreateInfo image_create_info;
    image_create_info.imageType = vk::ImageType::e2D;
    image_create_info.mipLevels = 1;
    image_create_info.arrayLayers = 1;
    image_create_info.format = VulkanPixelFormat(pixel_format);
    image_create_info.tiling = vk::ImageTiling::eOptimal;
    image_create_info.initialLayout = vk::ImageLayout::eUndefined;
    using SurfaceType = SurfaceParams::SurfaceType;
    switch (type) {
    case SurfaceType::Color:
        image_create_info.usage |= vk::ImageUsageFlagBits::eColorAttachment;
    case SurfaceType::Texture:
        image_create_info.usage |= vk::ImageUsageFlagBits::eSampled;
        image_create_info.flags = vk::ImageCreateFlagBits::eMutableFormat;
        image_create_info.usage |= vk::ImageUsageFlagBits::eStorage;
        break;
    case SurfaceType::Depth:
    case SurfaceType::DepthStencil:
        image_create_info.usage |= vk::ImageUsageFlagBits::eDepthStencilAttachment;
        break;
    default:
        UNIMPLEMENTED();
    }
    image_create_info.usage |= vk::ImageUsageFlagBits::eTransferSrc;
    image_create_info.usage |= vk::ImageUsageFlagBits::eTransferDst;
    image_create_info.sharingMode = vk::SharingMode::eExclusive;
    image_create_info.samples = vk::SampleCountFlagBits::e1;
    image_create_info.extent = vk::Extent3D{width, height, 1};
    image = owner.vk_inst.device->createImageUnique(image_create_info);

    auto default_image_memory_requirements =
        owner.vk_inst.device->getImageMemoryRequirements(*image);
    vk::MemoryAllocateInfo image_memory_allocation_info;
    image_memory_allocation_info.allocationSize = default_image_memory_requirements.size;
    image_memory_allocation_info.memoryTypeIndex = owner.vk_inst.getMemoryType(
        default_image_memory_requirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal);

    vk::ExportMemoryAllocateInfo image_export_memory_allocation_info;
    image_export_memory_allocation_info.handleTypes =
        vk::ExternalMemoryHandleTypeFlagBits::eOpaqueWin32;
    image_memory_allocation_info.pNext = &image_export_memory_allocation_info;

    image_memory = owner.vk_inst.device->allocateMemoryUnique(image_memory_allocation_info);
    owner.vk_inst.device->bindImageMemory(*image, *image_memory, 0);

    shmem_handle = owner.vk_inst.device->getMemoryWin32HandleKHR(
        {*image_memory, vk::ExternalMemoryHandleTypeFlagBits::eOpaqueWin32});

    {
        auto cur_state = OpenGL::OpenGLState::GetCurState();
        auto state = cur_state;

        gl_memory_object.Create();
        glImportMemoryWin32HandleEXT(gl_memory_object.handle,
                                     image_memory_allocation_info.allocationSize,
                                     GL_HANDLE_TYPE_OPAQUE_WIN32_EXT, shmem_handle);

        texture.Create();
        cur_state.texture_units[0].texture_2d = texture.handle;
        cur_state.Apply();
        glActiveTexture(GL_TEXTURE0);
        glTexStorageMem2DEXT(GL_TEXTURE_2D, 1, OpenGLPixelFormat(pixel_format), width, height,
                             gl_memory_object.handle, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

        // TODO: will need to port this to the vulkan equivalent when I get that far
        std::array<GLint, 4> swizzle_mask;
        switch (pixel_format) {
        case PX::RGB8:
        case PX::RGB565:
            swizzle_mask = {GL_RED, GL_GREEN, GL_BLUE, GL_ONE};
            break;
        case PX::IA8:
        case PX::IA4:
            swizzle_mask = {GL_RED, GL_RED, GL_RED, GL_GREEN};
            break;
        case PX::I8:
        case PX::I4:
            swizzle_mask = {GL_RED, GL_RED, GL_RED, GL_ONE};
            break;
        case PX::A8:
        case PX::A4:
            swizzle_mask = {GL_ZERO, GL_ZERO, GL_ZERO, GL_RED};
            break;
        default:
            swizzle_mask = {GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA};
            break;
        }
        glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzle_mask.data());
        cur_state.Apply();
    }
    auto [region, offset] = owner.GetBufferOffset(addr);
    std::memcpy(region.ptr + offset, VideoCore::g_memory->GetPhysicalPointer(addr), size);
    vk::MappedMemoryRange range{*region.gpu_memory, Common::AlignDown(offset, owner.min_flush),
                                Common::AlignUp(size, owner.min_flush)};
    owner.vk_inst.device->flushMappedMemoryRanges(range);

    owner.buffer_to_image->ImageFromBuffer(*region.buffer, *image, pixel_format, offset, width,
                                           height, stride, is_tiled);

    LOG_DEBUG(Render_OpenGL, "Surface{}", PrintParams());
}

CachedSurface::CachedSurface(RasterizerCacheVulkan& owner,
                             const GPU::Regs::MemoryFillConfig& config)
    : owner{owner} {
    addr = config.GetStartAddress();
    end = config.GetEndAddress();
    size = end - addr;
    type = SurfaceType::Fill;
    res_scale = std::numeric_limits<u16>::max();

    std::memcpy(fill_data.data(), &config.value_32bit, 4);
    if (config.fill_32bit) {
        fill_size = 4;
    } else if (config.fill_24bit) {
        fill_size = 3;
    } else {
        fill_size = 2;
    }
}

CachedSurface::~CachedSurface() {
    if (type == SurfaceType::Fill || type == SurfaceType::Texture)
        return;
    auto [region, offset] = owner.GetBufferOffset(addr);
    owner.image_to_buffer->BufferFromImage(*region.buffer, *image, pixel_format, offset, width,
                                           height, stride, is_tiled);
    vk::MappedMemoryRange range{*region.gpu_memory, Common::AlignDown(offset, owner.min_flush),
                                Common::AlignUp(size, owner.min_flush)};
    owner.vk_inst.device->invalidateMappedMemoryRanges(range);
    std::memcpy(VideoCore::g_memory->GetPhysicalPointer(addr), region.ptr + offset, size);
}
} // namespace Vulkan
