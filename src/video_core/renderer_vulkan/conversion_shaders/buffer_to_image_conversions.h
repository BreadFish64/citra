#pragma once

#include "common/common_types.h"

namespace Vulkan::BufferToImage {
constexpr u32 rgba8_to_rgba8[]
#include "video_core/renderer_vulkan/conversion_shaders/buffer_to_image/spirv/rgba8_to_rgba8.h"
    ;

constexpr u32 rgb8_to_rgba8[]
#include "video_core/renderer_vulkan/conversion_shaders/buffer_to_image/spirv/rgb8_to_rgba8.h"
    ;

constexpr u32 rgb5a1_to_rgba8[]
#include "video_core/renderer_vulkan/conversion_shaders/buffer_to_image/spirv/rgb5a1_to_rgba8.h"
    ;

constexpr u32 rgb565_to_rgba8[]
#include "video_core/renderer_vulkan/conversion_shaders/buffer_to_image/spirv/rgb565_to_rgba8.h"
    ;

constexpr u32 rgba4_to_rgba8[]
#include "video_core/renderer_vulkan/conversion_shaders/buffer_to_image/spirv/rgba4_to_rgba8.h"
    ;

constexpr u32 ia8_to_rg8[]
#include "video_core/renderer_vulkan/conversion_shaders/buffer_to_image/spirv/ia8_to_rg8.h"
    ;

constexpr u32 rg8_to_rg8[]
#include "video_core/renderer_vulkan/conversion_shaders/buffer_to_image/spirv/rg8_to_rg8.h"
    ;

constexpr u32 i8_to_r8[]
#include "video_core/renderer_vulkan/conversion_shaders/buffer_to_image/spirv/i8_to_r8.h"
    ;

constexpr u32 a8_to_r8[]
#include "video_core/renderer_vulkan/conversion_shaders/buffer_to_image/spirv/a8_to_r8.h"
    ;

constexpr u32 ia4_to_rg8[]
#include "video_core/renderer_vulkan/conversion_shaders/buffer_to_image/spirv/ia4_to_rg8.h"
    ;

constexpr u32 i4_to_r8[]
#include "video_core/renderer_vulkan/conversion_shaders/buffer_to_image/spirv/i4_to_r8.h"
    ;

constexpr u32 a4_to_r8[]
#include "video_core/renderer_vulkan/conversion_shaders/buffer_to_image/spirv/a4_to_r8.h"
    ;

constexpr u32 etc1_to_rgba8[]
#include "video_core/renderer_vulkan/conversion_shaders/buffer_to_image/spirv/etc1_to_rgba8.h"
    ;

constexpr u32 etc1a4_to_rgba8[]
#include "video_core/renderer_vulkan/conversion_shaders/buffer_to_image/spirv/etc1a4_to_rgba8.h"
    ;
} // namespace Vulkan::BufferToImage