#pragma once

#include <unordered_map>

#include <glad/glad.h>

#include "video_core/renderer_opengl/gl_surface_params.h"
#include "video_core/renderer_vulkan/vk_instance.h"

namespace Vulkan {

// TODO: make threadsafe
class ConvertaTron5000 : NonCopyable {
public:
    ConvertaTron5000(Instance& vk_inst);
    ~ConvertaTron5000();

    void ImageFromBuffer(vk::Buffer buffer, vk::Image image,
                         OpenGL::SurfaceParams::PixelFormat pixel_format, vk::DeviceSize offset,
                         u32 width, u32 height, u32 stride, bool tiled);
    void BufferFromImage(vk::Buffer buffer, vk::Image image,
                         OpenGL::SurfaceParams::PixelFormat pixel_format, vk::DeviceSize offset,
                         u32 width, u32 height, u32 stride, bool tiled);

private:
    using PX = OpenGL::SurfaceParams::PixelFormat;
    enum class Direction : u8 { BufferToImage, ImageToBuffer };

    Instance& vk_inst;

    std::unordered_map<OpenGL::SurfaceParams::PixelFormat, vk::UniquePipeline> buffer_to_image_pipelines;
    std::unordered_map<OpenGL::SurfaceParams::PixelFormat, vk::UniquePipeline>
        image_to_buffer_pipelines;
    vk::UniquePipeline d24s8_buffer_to_image_pipeline;
    vk::UniquePipeline d24s8_image_to_buffer_pipeline;
    vk::UniqueDescriptorPool descriptor_pool;
    vk::UniqueDescriptorSetLayout buffer_to_image_set_layout;
    vk::UniqueDescriptorSetLayout buffer_to_buffer_set_layout;
    vk::UniqueDescriptorSet buffer_to_image_descriptor_set;
    vk::UniqueDescriptorSet buffer_to_buffer_descriptor_set;
    vk::UniquePipelineLayout buffer_to_image_pipeline_layout;
    vk::UniquePipelineLayout buffer_to_buffer_pipeline_layout;
    vk::UniqueCommandBuffer command_buffer;
    vk::UniqueBuffer stencil_temp;
    vk::UniqueBuffer depth_temp;
    vk::UniqueDeviceMemory temp_buf_mem;

    void BufferColorConvert(Direction direction, vk::Buffer buffer, vk::Image image,
                       OpenGL::SurfaceParams::PixelFormat pixel_format, vk::DeviceSize offset,
                       u32 width, u32 height, u32 stride, bool tiled);
    void D24S8Convert(Direction direction, vk::Buffer buffer, vk::Image image, vk::DeviceSize offset,
                      u32 width,
                      u32 height, u32 stride, bool tiled);
};
} // namespace Vulkan