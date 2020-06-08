#pragma once

#include <unordered_map>

#include <glad/glad.h>

#include "video_core/renderer_opengl/gl_surface_params.h"
#include "video_core/renderer_vulkan/vk_instance.h"

namespace Vulkan {

class ImageToBufferConverter : NonCopyable {
public:
    ImageToBufferConverter(Instance& vk_inst);
    ~ImageToBufferConverter();

    Instance& vk_inst;

    vk::UniquePipelineLayout conversion_pipeline_layout;
    std::unordered_map<OpenGL::SurfaceParams::PixelFormat,
                       std::pair<vk::Format, vk::UniquePipeline>>
        conversion_pipelines;
    vk::UniqueDescriptorPool descriptor_pool;
    vk::UniqueDescriptorSet conversion_descriptor_set;
    vk::UniqueDescriptorSetLayout conversion_descriptor_layout;

    void BufferFromImage(vk::Buffer buffer, vk::Image image,
                         OpenGL::SurfaceParams::PixelFormat pixel_format, vk::DeviceSize offset,
                         u32 width, u32 height, u32 stride, bool tiled);
};
} // namespace Vulkan