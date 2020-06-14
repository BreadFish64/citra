#include "video_core/renderer_vulkan/vk_rasterizer_cache.h"
#include "video_core/renderer_vulkan/conversion_shaders/image_to_buffer_conversions.h"
#include "video_core/renderer_vulkan/vk_image_to_buffer.h"

namespace Vulkan {
ImageToBufferConverter::ImageToBufferConverter(Instance& vk_inst) : vk_inst{vk_inst} {
    using PX = OpenGL::SurfaceParams::PixelFormat;
    struct PXConversion {
        PX src_fmt;
        vk::Format dst_fmt;
        const u32* shader_bin;
        vk::DeviceSize shader_size;
    };
#define PX_CONVERSION(src, dst, shader)                                                            \
    PXConversion{PX::src, vk::Format::dst, ImageToBuffer::shader, sizeof(ImageToBuffer::shader)}
    std::initializer_list<PXConversion> conversion_pipeline_specs{
        PX_CONVERSION(RGBA8, eR8G8B8A8Unorm, rgba8_to_rgba8),
        PX_CONVERSION(RGB8, eR8G8B8A8Unorm, rgba8_to_rgb8),
        PX_CONVERSION(RGB5A1, eR8G8B8A8Unorm, rgba8_to_rgb5a1),
        PX_CONVERSION(RGB565, eR8G8B8A8Unorm, rgba8_to_rgb565),
        PX_CONVERSION(RGBA4, eR8G8B8A8Unorm, rgba8_to_rgba4),
        PX_CONVERSION(IA8, eR8G8Unorm, rg8_to_ia8),
        PX_CONVERSION(RG8, eR8G8Unorm, rg8_to_rg8),
        PX_CONVERSION(I8, eR8Unorm, r8_to_i8),
        PX_CONVERSION(A8, eR8Unorm, r8_to_a8),
        PX_CONVERSION(IA4, eR8G8Unorm, rg8_to_ia4),
        PX_CONVERSION(I4, eR8Unorm, r8_to_i4),
        PX_CONVERSION(A4, eR8Unorm, r8_to_a4),
    };
#undef PX_CONVERSION

    std::array<vk::DescriptorPoolSize, 2> pool_sizes;
    pool_sizes[0].descriptorCount = 1;
    pool_sizes[0].type = vk::DescriptorType::eStorageBuffer;
    pool_sizes[1].descriptorCount = 1;
    pool_sizes[1].type = vk::DescriptorType::eStorageImage;

    vk::DescriptorPoolCreateInfo descriptor_pool_info;
    descriptor_pool_info.pPoolSizes = pool_sizes.data();
    descriptor_pool_info.poolSizeCount = 2;
    descriptor_pool_info.maxSets = 1;
    descriptor_pool = vk_inst.device->createDescriptorPoolUnique(descriptor_pool_info);

    std::array<vk::DescriptorSetLayoutBinding, 2> descriptors;
    auto& src_buffer_descriptor = descriptors[0];
    src_buffer_descriptor.binding = 0;
    // TODO: some conversion occur because
    // rendering to packed formats (such as RGBA4)
    // is not widely supported but sampling them is possible.
    // In this case we could try copying from a texel buffer
    // instead of doing the conversion ourselves.
    src_buffer_descriptor.descriptorType = vk::DescriptorType::eStorageBuffer;
    src_buffer_descriptor.descriptorCount = 1;
    src_buffer_descriptor.stageFlags = vk::ShaderStageFlagBits::eCompute;

    auto& dst_image_descriptor = descriptors[1];
    dst_image_descriptor.binding = 1;
    dst_image_descriptor.descriptorType = vk::DescriptorType::eStorageImage;
    dst_image_descriptor.descriptorCount = 1;
    dst_image_descriptor.stageFlags = vk::ShaderStageFlagBits::eCompute;

    vk::DescriptorSetLayoutCreateInfo descriptor_layout_info;
    descriptor_layout_info.pBindings = descriptors.data();
    descriptor_layout_info.bindingCount = descriptors.size();
    conversion_descriptor_layout =
        vk_inst.device->createDescriptorSetLayoutUnique(descriptor_layout_info);

    vk::DescriptorSetAllocateInfo descriptor_set_allocate_info;
    descriptor_set_allocate_info.descriptorPool = *descriptor_pool;
    descriptor_set_allocate_info.descriptorSetCount = 1;
    descriptor_set_allocate_info.pSetLayouts = &*conversion_descriptor_layout;
    conversion_descriptor_set =
        std::move(vk_inst.device->allocateDescriptorSetsUnique(descriptor_set_allocate_info)[0]);

    vk::PushConstantRange push_constant_range;
    push_constant_range.stageFlags = vk::ShaderStageFlagBits::eCompute;
    push_constant_range.offset = 0;
    push_constant_range.size = 8;

    vk::PipelineLayoutCreateInfo pipeline_layout_info;
    pipeline_layout_info.pSetLayouts = &*conversion_descriptor_layout;
    pipeline_layout_info.setLayoutCount = 1;
    pipeline_layout_info.pPushConstantRanges = &push_constant_range;
    pipeline_layout_info.pushConstantRangeCount = 1;

    conversion_pipeline_layout = vk_inst.device->createPipelineLayoutUnique(pipeline_layout_info);
    for (auto [src_fmt, dst_fmt, shader_bin, shader_size] : conversion_pipeline_specs) {
        vk::ShaderModuleCreateInfo shader_info;
        shader_info.pCode = shader_bin;
        shader_info.codeSize = shader_size;
        auto shader_module = vk_inst.device->createShaderModuleUnique(shader_info);
        vk::ComputePipelineCreateInfo pipeline_info;
        pipeline_info.stage.stage = vk::ShaderStageFlagBits::eCompute;
        pipeline_info.stage.module = *shader_module;
        pipeline_info.stage.pName = "main";
        pipeline_info.layout = *conversion_pipeline_layout;

        auto pipeline = vk_inst.device->createComputePipelineUnique({}, pipeline_info);
        conversion_pipelines.emplace(src_fmt, std::make_pair(dst_fmt, std::move(pipeline)));
    }
}

ImageToBufferConverter::~ImageToBufferConverter() {}

void ImageToBufferConverter::BufferFromImage(vk::Buffer buffer, vk::Image image,
                                             OpenGL::SurfaceParams::PixelFormat pixel_format,
                                             vk::DeviceSize offset, u32 width, u32 height,
                                             u32 stride, bool tiled) {
    using OpenGL::SurfaceParams;
    using PX = SurfaceParams::PixelFormat;

    switch (pixel_format) {
    case PX::RGBA8:
    case PX::RGB8:
    case PX::RGBA4:
    case PX::RGB5A1:
    case PX::RGB565:
    case PX::IA8:
    case PX::RG8:
    case PX::IA4:
    case PX::I8:
    case PX::A8:
    case PX::A4:
    case PX::I4: {
        vk::CommandBufferAllocateInfo command_buffer_allocate_info;
        command_buffer_allocate_info.commandBufferCount = 1;
        command_buffer_allocate_info.commandPool = *vk_inst.command_pool;
        command_buffer_allocate_info.level = vk::CommandBufferLevel::ePrimary;
        auto command_buffer = std::move(
            vk_inst.device->allocateCommandBuffersUnique(command_buffer_allocate_info)[0]);

        vk::CommandBufferBeginInfo command_buffer_begin;
        command_buffer_begin.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
        command_buffer->begin(command_buffer_begin);
        vk::ImageSubresourceRange image_range;
        image_range.aspectMask = vk::ImageAspectFlagBits::eColor;
        image_range.baseMipLevel = 0;
        image_range.levelCount = 1;
        image_range.baseArrayLayer = 0;
        image_range.layerCount = 1;
        vk::ImageMemoryBarrier barrier;
        barrier.image = image;
        barrier.subresourceRange = image_range;
        barrier.oldLayout = vk::ImageLayout::eUndefined;
        barrier.newLayout = vk::ImageLayout::eGeneral;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

        std::array<vk::WriteDescriptorSet, 2> desc_set_writes;

        vk::SamplerCreateInfo sampler_info;
        sampler_info.addressModeU = vk::SamplerAddressMode::eClampToEdge;
        sampler_info.addressModeV = vk::SamplerAddressMode::eClampToEdge;
        sampler_info.addressModeW = vk::SamplerAddressMode::eClampToEdge;
        sampler_info.magFilter = vk::Filter::eNearest;
        sampler_info.minFilter = vk::Filter::eNearest;
        auto sampler = vk_inst.device->createSamplerUnique(sampler_info);
        vk::ImageViewCreateInfo image_view_info;
        image_view_info.viewType = vk::ImageViewType::e2D;
        image_view_info.format = VulkanIntFormat(VulkanPixelFormat(pixel_format));
        image_view_info.image = image;
        image_view_info.subresourceRange = image_range;
        auto image_view = vk_inst.device->createImageViewUnique(image_view_info);
        vk::DescriptorImageInfo image_info;
        image_info.imageLayout = vk::ImageLayout::eGeneral;
        image_info.imageView = *image_view;
        image_info.sampler = *sampler;

        desc_set_writes[0].descriptorCount = 1;
        desc_set_writes[0].descriptorType = vk::DescriptorType::eStorageImage;
        desc_set_writes[0].dstArrayElement = 0;
        desc_set_writes[0].dstBinding = 1;
        desc_set_writes[0].dstSet = *conversion_descriptor_set;
        desc_set_writes[0].pImageInfo = &image_info;

        vk::DeviceSize size = SurfaceParams::GetFormatBpp(pixel_format) * width * height / 8;
        vk::DescriptorBufferInfo buffer_info;
        buffer_info.buffer = buffer;
        buffer_info.offset = offset;
        buffer_info.range = size;

        desc_set_writes[1].descriptorCount = 1;
        desc_set_writes[1].descriptorType = vk::DescriptorType::eStorageBuffer;
        desc_set_writes[1].dstArrayElement = 0;
        desc_set_writes[1].dstBinding = 0;
        desc_set_writes[1].dstSet = *conversion_descriptor_set;
        desc_set_writes[1].pBufferInfo = &buffer_info;

        vk_inst.device->updateDescriptorSets(desc_set_writes, {});

        command_buffer->pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe,
                                        vk::PipelineStageFlagBits::eComputeShader, {}, {}, {},
                                        barrier);
        auto& pipeline = conversion_pipelines.at(pixel_format);
        command_buffer->bindPipeline(vk::PipelineBindPoint::eCompute, *pipeline.second);
        struct {
            u32 stride{};
            u8 pad[3]{};
            bool tiled{};
        } push_values{stride, tiled};
        command_buffer->pushConstants(*conversion_pipeline_layout,
                                      vk::ShaderStageFlagBits::eCompute, 0, sizeof(push_values),
                                      &push_values);
        command_buffer->bindDescriptorSets(vk::PipelineBindPoint::eCompute,
                                           *conversion_pipeline_layout, 0,
                                           *conversion_descriptor_set, {});
        command_buffer->dispatch(width / 8, height / 8, 1);
        command_buffer->end();
        vk_inst.SubmitCommandBuffers(*command_buffer);
        vk_inst.device->waitIdle();
    } break;
    case PX::D16:
    case PX::D24:
    case PX::D24S8: {
        if (tiled) {
            LOG_DEBUG(Render_OpenGL, "Unimplemented tiled Depth/Stencil");
        }
        vk::CommandBufferAllocateInfo command_buffer_allocate_info;
        command_buffer_allocate_info.commandBufferCount = 1;
        command_buffer_allocate_info.commandPool = *vk_inst.command_pool;
        command_buffer_allocate_info.level = vk::CommandBufferLevel::ePrimary;
        auto command_buffer = std::move(
            vk_inst.device->allocateCommandBuffersUnique(command_buffer_allocate_info)[0]);

        vk::CommandBufferBeginInfo command_buffer_begin;
        command_buffer_begin.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
        command_buffer->begin(command_buffer_begin);

        vk::ImageSubresourceRange image_range;
        image_range.aspectMask = vk::ImageAspectFlagBits::eDepth;
        if (pixel_format == PX::D24S8 || pixel_format == PX::D24) {
            image_range.aspectMask |= vk::ImageAspectFlagBits::eStencil;
        }
        image_range.baseMipLevel = 0;
        image_range.levelCount = 1;
        image_range.baseArrayLayer = 0;
        image_range.layerCount = 1;
        vk::ImageMemoryBarrier barrier;
        barrier.image = image;
        barrier.subresourceRange = image_range;
        barrier.oldLayout = vk::ImageLayout::eGeneral;
        barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

        command_buffer->pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe,
                                        vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, barrier);
        std::array<vk::BufferImageCopy, 2> copy;
        auto& depth_copy = copy[0];
        depth_copy.bufferRowLength = stride;
        depth_copy.bufferOffset = offset;
        depth_copy.imageExtent = vk::Extent3D{width, height, 1};
        depth_copy.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eDepth;
        depth_copy.imageSubresource.baseArrayLayer = 0;
        depth_copy.imageSubresource.layerCount = 1;
        depth_copy.imageSubresource.mipLevel = 0;
        copy[1] = copy[0];
        copy[1].imageSubresource.aspectMask = vk::ImageAspectFlagBits::eStencil;
        command_buffer->copyImageToBuffer(image, vk::ImageLayout::eTransferSrcOptimal, buffer,
                                          {pixel_format == PX::D24S8 ? 2u : 1u, copy.data()});

        barrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
        barrier.newLayout = vk::ImageLayout::eGeneral;
        command_buffer->pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                                        vk::PipelineStageFlagBits::eBottomOfPipe, {}, {}, {},
                                        barrier);

        command_buffer->end();
        vk_inst.SubmitCommandBuffers(*command_buffer);
        vk_inst.device->waitIdle();
    } break;
    default:
        break;
    }
}

} // namespace Vulkan