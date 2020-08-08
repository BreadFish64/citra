#include <unordered_map>

#include "video_core/pica_state.h"
#include "video_core/renderer_opengl/gl_state.h"
#include "video_core/renderer_vulkan/vk_convert_and_transfer.h"
#include "video_core/renderer_vulkan/vk_rasterizer_cache.h"
#include "video_core/video_core.h"

#include "common/alignment.h"

namespace Vulkan {

std::tuple<Surface, StorageMap::iterator> RasterizerCacheVulkan::SurfaceSearch(
    const SurfaceParams& params) {
    auto set_iter{storage_map.lower_bound(params.GetInterval())};
    for (; set_iter != storage_map.end() && set_iter->first.upper() <= params.end; ++set_iter) {
        // TODO: check if linear search is the best option here
        for (const auto& check_surface : set_iter->second) {
            if (params == static_cast<const SurfaceParams&>(*check_surface)) {
                return std::make_tuple(check_surface, set_iter);
            }
        }
    }
    return std::make_tuple(nullptr, set_iter);
}

void RasterizerCacheVulkan::FillSurface(CacheRecord& record, const CachedSurface& surface,
                                        std::array<u8, 4> fill_buffer,
                                        Common::Rectangle<u32> rect) {
    if (surface.GetScaledRect() != rect)
        // TODO: use vkCmdClearAttachments to clear subrects
        LOG_ERROR(Render_Vulkan, "Partial surface fills not implemented");

    switch (surface.type) {
    case SurfaceParams::SurfaceType::Color:
    case SurfaceParams::SurfaceType::Texture: {
        vk::ImageSubresourceRange image_range;
        image_range.aspectMask = vk::ImageAspectFlagBits::eColor;
        image_range.baseMipLevel = 0;
        image_range.levelCount = 1;
        image_range.baseArrayLayer = 0;
        image_range.layerCount = 1;
        vk::ImageMemoryBarrier barrier;
        barrier.image = *surface.image;
        barrier.subresourceRange = image_range;
        barrier.oldLayout = vk::ImageLayout::eGeneral;
        barrier.newLayout = vk::ImageLayout::eTransferDstOptimal;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

        vk::ClearColorValue color;
        {
            Pica::Texture::TextureInfo tex_info{};
            tex_info.format = static_cast<Pica::TexturingRegs::TextureFormat>(surface.pixel_format);
            Common::Vec4<float> color_vec{
                Pica::Texture::LookupTexture(fill_buffer.data(), 0, 0, tex_info) / 255.0f};
            std::copy_n(std::make_move_iterator(color_vec.AsArray()), 4, color.float32);
        }

        record.command_buffer.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe,
                                              vk::PipelineStageFlagBits::eTransfer, {}, {}, {},
                                              barrier);
        record.command_buffer.clearColorImage(*surface.image, vk::ImageLayout::eTransferDstOptimal,
                                              color, image_range);

        barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
        barrier.newLayout = vk::ImageLayout::eGeneral;
        record.command_buffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                                              vk::PipelineStageFlagBits::eBottomOfPipe, {}, {}, {},
                                              barrier);
        record.signal_tex.emplace(surface.texture.handle);
    } break;
    default:
        LOG_ERROR(Render_Vulkan, "non-color fills not implemented");
    }
}

RasterizerCacheVulkan::RasterizerCacheVulkan(Instance& vk_inst) : vk_inst{vk_inst} {
    min_flush = vk_inst.physical_device.getProperties().limits.nonCoherentAtomSize;

    const auto init_mem_region = [&vk_inst](MemoryRegion& region, vk::DeviceSize size,
                                            PAddr guest_addr) {
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
            memory_requirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible);

        region.gpu_memory = vk_inst.device->allocateMemoryUnique(allocation_info);
        vk_inst.device->bindBufferMemory(*region.buffer, *region.gpu_memory, 0);
        region.ptr = reinterpret_cast<u8*>(vk_inst.device->mapMemory(*region.gpu_memory, 0, size));
        region.guest_addr = guest_addr;
    };
    init_mem_region(vram, Memory::VRAM_SIZE, Memory::VRAM_PADDR);
    init_mem_region(fcram, Memory::FCRAM_N3DS_SIZE, Memory::FCRAM_PADDR);

    buffer_to_image = std::make_unique<ConvertaTron5000>(vk_inst);

    ClearAll(false);
}

RasterizerCacheVulkan::~RasterizerCacheVulkan() {}

bool RasterizerCacheVulkan::BlitSurfaces(CacheRecord& record, const Surface& src_surface,
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
    case SurfaceParams::SurfaceType::DepthStencil:
        image_range.aspectMask =
            vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
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

    record.command_buffer.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe,
                                          vk::PipelineStageFlagBits::eTransfer, {}, {}, {},
                                          {src_barrier, dst_barrier});
    record.command_buffer.blitImage(*src_surface->image, vk::ImageLayout::eTransferSrcOptimal,
                                    *dst_surface->image, vk::ImageLayout::eTransferDstOptimal,
                                    {blit_area}, vk::Filter::eNearest);
    std::swap(src_barrier.oldLayout, src_barrier.newLayout);
    std::swap(dst_barrier.oldLayout, dst_barrier.newLayout);
    record.command_buffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                                          vk::PipelineStageFlagBits::eBottomOfPipe, {}, {}, {},
                                          {src_barrier, dst_barrier});
    record.wait_tex.emplace(src_surface->texture.handle);
    record.signal_tex.emplace(dst_surface->texture.handle);
    return true;
}

void RasterizerCacheVulkan::CopySurface(CacheRecord& record, const Surface& src_surface,
                                        const Surface& dst_surface, SurfaceInterval copy_interval) {

    SurfaceParams subrect_params = dst_surface->FromInterval(copy_interval);
    ASSERT(subrect_params.GetInterval() == copy_interval);

    ASSERT(src_surface != dst_surface);

    // This is only called when CanCopy is true, no need to run checks here
    if (src_surface->type == SurfaceParams::SurfaceType::Fill) {
        // FillSurface needs a 4 bytes buffer
        const u32 fill_offset =
            (boost::icl::first(copy_interval) - src_surface->addr) % src_surface->fill_size;
        std::array<u8, 4> fill_buffer;

        u32 fill_buff_pos = fill_offset;
        for (int i : {0, 1, 2, 3})
            fill_buffer[i] = src_surface->fill_data[fill_buff_pos++ % src_surface->fill_size];

        FillSurface(record, *dst_surface, std::move(fill_buffer),
                    dst_surface->GetScaledSubRect(subrect_params));
        return;
    }
    if (src_surface->CanSubRect(subrect_params)) {
        BlitSurfaces(record, src_surface, src_surface->GetScaledSubRect(subrect_params),
                     dst_surface, dst_surface->GetScaledSubRect(subrect_params));
        return;
    }
    UNREACHABLE();
}

Surface RasterizerCacheVulkan::GetSurface(CacheRecord& record, const SurfaceParams& params,
                                          ScaleMatch match_res_scale, bool load_if_create) {
    if (params.addr == 0 || params.height * params.width == 0) {
        return nullptr;
    }
    // Use GetSurfaceSubRect instead
    ASSERT(params.width == params.stride);

    ASSERT(!params.is_tiled || (params.width % 8 == 0 && params.height % 8 == 0));
    auto [surface, iter] = SurfaceSearch(params);

    bool is_new{!surface};
    if (is_new) {
        surface = std::make_shared<CachedSurface>(*this, record.command_buffer, params);
        storage_map.add(iter, std::make_pair(surface->GetInterval(), SurfaceSet{surface}));
    }

    // TODO: figure out load_if_create
    if (load_if_create)
        ValidateSurface(record, *surface, is_new);
    return surface;
}

std::tuple<Surface, Common::Rectangle<u32>> RasterizerCacheVulkan::GetSurfaceSubRect(
    CacheRecord& record, const SurfaceParams& params, ScaleMatch match_res_scale,
    bool load_if_create) {
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
    Surface surface = GetSurface(record, new_params, match_res_scale, load_if_create);
    return std::make_tuple(surface, surface->GetScaledSubRect(params));
}

Surface RasterizerCacheVulkan::GetTextureSurface(
    CacheRecord& record, const Pica::TexturingRegs::FullTextureConfig& config) {
    Pica::Texture::TextureInfo info =
        Pica::Texture::TextureInfo::FromPicaRegister(config.config, config.format);
    return GetTextureSurface(record, info, config.config.lod.max_level);
}

Surface RasterizerCacheVulkan::GetTextureSurface(CacheRecord& record,
                                                 const Pica::Texture::TextureInfo& info,
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
        LOG_CRITICAL(Render_Vulkan, "Texture size ({}x{}) is not multiple of 8", min_width,
                     min_height);
        return nullptr;
    }
    if (info.width != (min_width << max_level) || info.height != (min_height << max_level)) {
        LOG_CRITICAL(Render_Vulkan,
                     "Texture size ({}x{}) does not support required mipmap level ({})",
                     params.width, params.height, max_level);
        return nullptr;
    }

    auto surface = GetSurface(record, params, ScaleMatch::Ignore, true);
    if (!surface)
        return nullptr;

    // TODO: mipmaps

    return surface;
}

const CachedTextureCube& RasterizerCacheVulkan::GetTextureCube(CacheRecord& record,
                                                               const TextureCubeConfig& config) {
    static CachedTextureCube x{};
    return x;
}

std::tuple<Surface, Surface, Common::Rectangle<u32>> RasterizerCacheVulkan::GetFramebufferSurfaces(
    CacheRecord& record, bool using_color_fb, bool using_depth_fb,
    const Common::Rectangle<s32>& viewport_rect) {
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
        boost::icl::intersects(color_vp_interval, depth_vp_interval)) {
        LOG_CRITICAL(Render_Vulkan, "Color and depth framebuffer memory regions overlap; "
                                    "overlapping framebuffers not supported!");
        using_depth_fb = false;
    }

    Common::Rectangle<u32> color_rect{};
    Surface color_surface = nullptr;
    if (using_color_fb)
        std::tie(color_surface, color_rect) =
            GetSurfaceSubRect(record, color_params, ScaleMatch::Exact, false);

    Common::Rectangle<u32> depth_rect{};
    Surface depth_surface = nullptr;
    if (using_depth_fb)
        std::tie(depth_surface, depth_rect) =
            GetSurfaceSubRect(record, depth_params, ScaleMatch::Exact, false);

    Common::Rectangle<u32> fb_rect{};
    if (color_surface != nullptr && depth_surface != nullptr) {
        fb_rect = color_rect;
        // Color and Depth surfaces must have the same dimensions and offsets
        if (color_rect != depth_rect) {
            color_surface = GetSurface(record, color_params, ScaleMatch::Exact, false);
            depth_surface = GetSurface(record, depth_params, ScaleMatch::Exact, false);
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
    SurfaceParams params;
    params.addr = config.GetStartAddress();
    params.end = config.GetEndAddress();
    params.size = params.end - params.addr;
    params.type = SurfaceParams::SurfaceType::Fill;
    params.res_scale = std::numeric_limits<u16>::max();

    auto [surface, iter] = SurfaceSearch(params);
    bool is_new{!surface};
    if (is_new) {
        surface = std::make_shared<CachedSurface>(*this, config);
        storage_map.add(iter, std::make_pair(surface->GetInterval(), SurfaceSet{surface}));
    }

    auto start_page = Common::AlignDown(surface->addr, Memory::PAGE_SIZE);
    auto end_page = Common::AlignUp(surface->addr + surface->size, Memory::PAGE_SIZE);
    VideoCore::g_memory->RasterizerMarkRegionCached(start_page, end_page - start_page, true);
    return surface;
}

std::tuple<Surface, Common::Rectangle<u32>> RasterizerCacheVulkan::GetTexCopySurface(
    CacheRecord& record, const SurfaceParams& params) {
    Common::Rectangle<u32> rect{};

    auto [match_surface, _] = SurfaceSearch(params);

    if (match_surface) {
        ValidateSurface(record, *match_surface, false);

        SurfaceParams match_subrect;
        if (params.width != params.stride) {
            const u32 tiled_size = match_surface->is_tiled ? 8 : 1;
            match_subrect = params;
            match_subrect.width = match_surface->PixelsInBytes(params.width) / tiled_size;
            match_subrect.stride = match_surface->PixelsInBytes(params.stride) / tiled_size;
            match_subrect.height *= tiled_size;
        } else {
            match_subrect = match_surface->FromInterval(params.GetInterval());
            ASSERT(match_subrect.GetInterval() == params.GetInterval());
        }

        rect = match_surface->GetScaledSubRect(match_subrect);
    }

    return std::make_tuple(match_surface, rect);
}

void RasterizerCacheVulkan::FlushRegion(PAddr addr, u32 size) {
    SurfaceInterval region_interval{addr, addr + size};
    auto [range_begin, range_end] = validity_map.equal_range(region_interval);
    if (range_begin == validity_map.end())
        return;

    auto [region, offset] = GetBufferOffset(addr);
    auto cpu_ptr = VideoCore::g_memory->GetPhysicalPointer(region.guest_addr);
    std::vector<vk::MappedMemoryRange> flush_regions;
    boost::icl::interval_set<PAddr, std::less, SurfaceInterval> erase_regions;
    MiniSet<CachedSurface*, 1> flush_surfaces;

    for (auto it{range_begin}; it != range_end; ++it) {
        if (it->second) {
            auto interval = it->first & region_interval;
            auto lower = Common::AlignDown(interval.lower(), min_flush);
            auto upper = Common::AlignUp(interval.upper(), min_flush);
            flush_regions.emplace_back(*region.gpu_memory, lower - region.guest_addr,
                                       upper - lower);
            LOG_TRACE(Render_Vulkan, "Flushing {:#08X} to {:#08X}", lower, upper);
            erase_regions += interval;
            if (const auto flush_surface = *it->second) {
                flush_surfaces.emplace(flush_surface);
                it->second = nullptr;
            }
        }
    }

    if (flush_regions.empty())
        return;

    if (!flush_surfaces.empty()) {
        vk::UniqueCommandBuffer cmd_buff;
        CacheRecord cache_record;
        {
            vk::CommandBufferAllocateInfo cmd_buff_info;
            cmd_buff_info.commandBufferCount = 1;
            cmd_buff_info.commandPool = *vk_inst.command_pool;
            cmd_buff_info.level = vk::CommandBufferLevel::ePrimary;
            cmd_buff = std::move(vk_inst.device->allocateCommandBuffersUnique(cmd_buff_info)[0]);
        }
        {
            vk::CommandBufferBeginInfo cmd_begin;
            cmd_begin.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
            cmd_buff->begin(cmd_begin);
        }
        // TODO: make this more fine-grained
        for (auto flush_surface : flush_surfaces) {
            cache_record.wait_tex.emplace_hint_unique(cache_record.wait_tex.end(),
                                                      flush_surface->texture.handle);
            buffer_to_image->BufferFromImage(
                *cmd_buff, *region.buffer, flush_surface->addr - region.guest_addr, *flush_surface);
        }
        cmd_buff->end();
        vk::UniqueFence flush_fence = vk_inst.device->createFenceUnique({});
        cache_record.command_buffer = *cmd_buff;
        vk_inst.CrossSubmit(cache_record, *flush_fence);
        vk_inst.device->waitForFences(*flush_fence, true, std::numeric_limits<u64>::max());
    }

    vk_inst.device->invalidateMappedMemoryRanges(flush_regions);
    for (const auto& interval : erase_regions) {
        auto offset = interval.lower() - region.guest_addr;
        auto size = interval.upper() - interval.lower();
        std::memcpy(cpu_ptr + offset, region.ptr + offset, size);
    }

    validity_map -= std::move(erase_regions);
}

void RasterizerCacheVulkan::InvalidateRegion(PAddr addr, u32 size, Surface region_owner) {
    SurfaceInterval interval{addr, addr + size};
    if (region_owner) {
        LOG_TRACE(Render_Vulkan, "Region overwritten by surface{}", region_owner->PrintParams());
        // Set 'region_owner' as the most recent write
        validity_map += std::make_pair(interval, std::make_optional(region_owner.get()));
        // Destroy all other surface references in interval
        storage_map -= interval;
        storage_map += std::make_pair(interval, SurfaceSet{std::move(region_owner)});
    } else {
        LOG_TRACE(Render_Vulkan, "Invalidating {:#08X} to {:#08X} by CPU", addr, addr + size);
        // Destroy all surface references in interval
        storage_map -= interval;
        // Set CPU as most recent write
        validity_map +=
            std::pair<SurfaceInterval, std::optional<CachedSurface*>>(interval, std::nullopt);
        // If the invalidation covers an entire page, un-cache it
        // If it becomes re-validated then it will be marked cached again
        auto start_page = Common::AlignUp(addr, Memory::PAGE_SIZE);
        auto end_page = Common::AlignDown(addr + size, Memory::PAGE_SIZE);
        if (start_page < end_page)
            VideoCore::g_memory->RasterizerMarkRegionCached(start_page, end_page - start_page,
                                                            false);
    }
}

void RasterizerCacheVulkan::ValidateSurface(CacheRecord& record, const CachedSurface& surface,
                                            bool is_new) {
    {
        // mark all pages touched by this surface as cached
        auto start_page{Common::AlignDown(surface.addr, Memory::PAGE_SIZE)};
        auto end_page{Common::AlignUp(surface.end, Memory::PAGE_SIZE)};
        VideoCore::g_memory->RasterizerMarkRegionCached(start_page, end_page - start_page, true);
    }

    SurfaceInterval region_interval{surface.addr, surface.addr + surface.size};
    auto [range_begin, range_end] = validity_map.equal_range(region_interval);
    if (range_begin == validity_map.end())
        return;

    auto [region, offset] = GetBufferOffset(surface.addr);
    auto cpu_ptr = VideoCore::g_memory->GetPhysicalPointer(region.guest_addr);
    std::vector<vk::MappedMemoryRange> flush_regions;
    boost::icl::interval_set<PAddr, std::less, SurfaceInterval> erase_regions;
    MiniSet<CachedSurface*, 1> flush_surfaces{};

    for (auto it{range_begin}; it != range_end; ++it) {
        if (!it->second) {
            auto interval = it->first & region_interval;
            auto lower = Common::AlignDown(interval.lower(), min_flush);
            auto upper = Common::AlignUp(interval.upper(), min_flush);
            flush_regions.emplace_back(*region.gpu_memory, lower - region.guest_addr,
                                       upper - lower);
            LOG_TRACE(Render_Vulkan, "Revalidating {:#08X} to {:#08X}", interval.lower(),
                      interval.upper());
            erase_regions += interval;
        } else if (const auto flush_surface = *it->second;
                   flush_surface && flush_surface != &surface) {
            ASSERT(boost::icl::intersects(flush_surface->GetInterval(), region_interval));
            it->second = nullptr;
            flush_surfaces.emplace(flush_surface);
        }
    }

    for (auto flush_surface : flush_surfaces) {
        if (flush_surface->type != SurfaceParams::SurfaceType::Fill) {
            buffer_to_image->BufferFromImage(record.command_buffer, *region.buffer,
                                             flush_surface->addr - region.guest_addr,
                                             *flush_surface);
            record.wait_tex.emplace(flush_surface->texture.handle);
        }
        // TODO, flush fill surfaces
    }

    for (const auto& interval : erase_regions) {
        auto offset = interval.lower() - region.guest_addr;
        std::memcpy(region.ptr + offset, cpu_ptr + offset, boost::icl::length(interval));
    }

    if (!flush_regions.empty())
        vk_inst.device->flushMappedMemoryRanges(flush_regions);

    if (!flush_surfaces.empty() || !erase_regions.empty() || is_new) {
        buffer_to_image->ImageFromBuffer(record.command_buffer, *region.buffer, offset, surface);
        record.signal_tex.emplace(surface.texture.handle);
    }

    validity_map -= std::move(erase_regions);
}

void RasterizerCacheVulkan::FlushAll() {
    FlushRegion(0, 0xFFFFFFFF);
}

void RasterizerCacheVulkan::ClearAll(bool flush) {
    if (flush)
        FlushAll();
    validity_map.clear();
    validity_map.set(
        std::make_pair(SurfaceInterval{Memory::VRAM_PADDR, Memory::VRAM_PADDR_END}, std::nullopt));
    validity_map.set(std::make_pair(
        SurfaceInterval{Memory::FCRAM_PADDR, Memory::FCRAM_N3DS_PADDR_END}, std::nullopt));
    storage_map.clear();
}

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

CachedSurface::CachedSurface(RasterizerCacheVulkan& owner, vk::CommandBuffer cmd_buff,
                             const SurfaceParams& surface_params)
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
        image_create_info.usage |= vk::ImageUsageFlagBits::eStorage;
        image_create_info.flags |= vk::ImageCreateFlagBits::eMutableFormat;
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

    vk::ImageMemoryBarrier barrier;
    barrier.image = *image;
    switch (type) {
    case OpenGL::SurfaceParams::SurfaceType::Color:
    case OpenGL::SurfaceParams::SurfaceType::Texture:
        barrier.subresourceRange.aspectMask |= vk::ImageAspectFlagBits::eColor;
        break;
    case OpenGL::SurfaceParams::SurfaceType::DepthStencil:
        barrier.subresourceRange.aspectMask |= vk::ImageAspectFlagBits::eStencil;
    case OpenGL::SurfaceParams::SurfaceType::Depth:
        barrier.subresourceRange.aspectMask |= vk::ImageAspectFlagBits::eDepth;
        break;
    }
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.oldLayout = vk::ImageLayout::eUndefined;
    barrier.newLayout = vk::ImageLayout::eGeneral;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

    cmd_buff.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe,
                             vk::PipelineStageFlagBits::eBottomOfPipe, {}, {}, {}, barrier);

    if (type <= SurfaceParams::SurfaceType::Texture) {
        vk::ImageViewCreateInfo image_view_info;
        image_view_info.viewType = vk::ImageViewType::e2D;
        image_view_info.format = VulkanIntFormat(image_create_info.format);
        image_view_info.image = *image;
        image_view_info.subresourceRange = barrier.subresourceRange;
        uint_view = owner.vk_inst.device->createImageViewUnique(image_view_info);
    }

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
    owner.buffer_to_image->AssignConversionDescriptor(*this, *region.buffer, offset);

    LOG_TRACE(Render_Vulkan, "Surface{}", PrintParams());
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

CachedSurface::~CachedSurface() {}

} // namespace Vulkan
