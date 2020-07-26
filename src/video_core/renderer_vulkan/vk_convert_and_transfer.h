#pragma once

#include <unordered_map>

#include <glad/glad.h>

#include "video_core/renderer_opengl/gl_surface_params.h"
#include "video_core/renderer_vulkan/vk_instance.h"

namespace Vulkan {

class CachedSurface;

// TODO: make threadsafe
class ConvertaTron5000 : NonCopyable {
public:
    ConvertaTron5000(Instance& vk_inst);
    ~ConvertaTron5000();

    void ImageFromBuffer(vk::CommandBuffer cmd_buff, vk::Buffer buffer, vk::DeviceSize offset,
                         const CachedSurface& surface);
    void BufferFromImage(vk::CommandBuffer cmd_buff, vk::Buffer buffer, vk::DeviceSize offset,
                         const CachedSurface& surface);
    void AssignConversionDescriptor(CachedSurface& surface, vk::Buffer buffer, vk::DeviceSize offset);

private:
    using PX = OpenGL::SurfaceParams::PixelFormat;
    enum class Direction : u8 { BufferToImage, ImageToBuffer };

    Instance& vk_inst;

    std::unordered_map<OpenGL::SurfaceParams::PixelFormat, vk::UniquePipeline>
        buffer_to_image_pipelines;
    std::unordered_map<OpenGL::SurfaceParams::PixelFormat, vk::UniquePipeline>
        image_to_buffer_pipelines;
    vk::UniqueDescriptorPool descriptor_pool;
    vk::UniqueDescriptorSetLayout buffer_to_image_set_layout;
    vk::UniqueDescriptorSetLayout buffer_to_buffer_set_layout;
    vk::UniquePipelineLayout buffer_to_image_pipeline_layout;
    vk::UniquePipelineLayout buffer_to_buffer_pipeline_layout;
    vk::UniqueBuffer depth_stencil_temp;
    vk::UniqueDeviceMemory temp_buf_mem;
    vk::UniqueSampler nearest_sampler;

    // eventually the command buffer should be started outside the function so this won't matter
    void BufferColorConvert(vk::CommandBuffer cmd_buff, Direction direction, vk::Buffer buffer,
                            vk::DeviceSize offset, const CachedSurface& surface);
    void D24S8Convert(vk::CommandBuffer cmd_buff, Direction direction, vk::Buffer buffer,
                      vk::DeviceSize offset,
                      const CachedSurface& surface);
};
} // namespace Vulkan