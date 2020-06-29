#pragma once

#include "common/common_types.h"

namespace Vulkan::ImageToBuffer {
constexpr u32 rgba8_to_rgba8[]
#include "video_core/renderer_vulkan/conversion_shaders/image_to_buffer/spirv/rgba8_to_rgba8.h"
    ;

constexpr u32 rgba8_to_rgb8[]
#include "video_core/renderer_vulkan/conversion_shaders/image_to_buffer/spirv/rgba8_to_rgb8.h"
    ;

constexpr u32 rgba8_to_rgb5a1[]
#include "video_core/renderer_vulkan/conversion_shaders/image_to_buffer/spirv/rgba8_to_rgb5a1.h"
    ;

constexpr u32 rgba8_to_rgb565[]
#include "video_core/renderer_vulkan/conversion_shaders/image_to_buffer/spirv/rgba8_to_rgb565.h"
    ;

constexpr u32 rgba8_to_rgba4[]
#include "video_core/renderer_vulkan/conversion_shaders/image_to_buffer/spirv/rgba8_to_rgba4.h"
    ;

constexpr u32 s8_to_d24s8[]
#include "video_core/renderer_vulkan/conversion_shaders/image_to_buffer/spirv/s8_to_d24s8.h"
    ;

constexpr u32 x8d24_to_d24[]
#include "video_core/renderer_vulkan/conversion_shaders/image_to_buffer/spirv/x8d24_to_d24.h"
    ;

constexpr u32 d16_to_d16[]
#include "video_core/renderer_vulkan/conversion_shaders/image_to_buffer/spirv/d16_to_d16.h"
    ;
} // namespace Vulkan::BufferToImage