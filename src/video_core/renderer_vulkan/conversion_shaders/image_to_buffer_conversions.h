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

constexpr u32 rg8_to_ia8[]
#include "video_core/renderer_vulkan/conversion_shaders/image_to_buffer/spirv/rg8_to_ia4.h"
    ;

constexpr u32 rg8_to_rg8[]
#include "video_core/renderer_vulkan/conversion_shaders/image_to_buffer/spirv/rg8_to_rg8.h"
    ;

constexpr u32 r8_to_i8[]
#include "video_core/renderer_vulkan/conversion_shaders/image_to_buffer/spirv/r8_to_i8.h"
    ;

constexpr u32 r8_to_a8[]
#include "video_core/renderer_vulkan/conversion_shaders/image_to_buffer/spirv/r8_to_a8.h"
    ;

constexpr u32 rg8_to_ia4[]
#include "video_core/renderer_vulkan/conversion_shaders/image_to_buffer/spirv/rg8_to_ia4.h"
    ;

constexpr u32 r8_to_i4[]
#include "video_core/renderer_vulkan/conversion_shaders/image_to_buffer/spirv/r8_to_i4.h"
    ;

constexpr u32 r8_to_a4[]
#include "video_core/renderer_vulkan/conversion_shaders/image_to_buffer/spirv/r8_to_a4.h"
    ;
} // namespace Vulkan::BufferToImage