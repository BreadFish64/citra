#include <fstream>

#include <lodepng.h>

#include "common/logging/backend.h"

#include "video_core/renderer_vulkan/vk_instance.h"
#include "video_core/renderer_vulkan/vk_buffer_to_image.h"

#ifdef _WIN32
extern "C" {
// tells Nvidia drivers to use the dedicated GPU by default on laptops with switchable graphics
__declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;
}
#endif

int main() {
    Log::AddBackend(std::make_unique<Log::DebuggerBackend>());

    u32 width = 512, height = 128, stride = width;
    OpenGL::SurfaceParams::PixelFormat format = OpenGL::SurfaceParams::PixelFormat::RGBA8;
    vk::DeviceSize size = OpenGL::SurfaceParams::GetFormatBpp(format) * width * height / 8;
    std::string_view file = "E:\\citra\\out\\build\\x64-Release\\bin\\pw_logo.bin";

    Vulkan::Instance vk_inst{"Conversion Test"};

    vk::UniqueBuffer img_buf;
    {
        vk::BufferCreateInfo img_buf_info;
        img_buf_info.size = size;
        img_buf_info.sharingMode = vk::SharingMode::eExclusive;
        img_buf_info.usage = vk::BufferUsageFlagBits::eStorageBuffer |
                             vk::BufferUsageFlagBits::eTransferSrc |
                             vk::BufferUsageFlagBits::eTransferDst;
        img_buf = vk_inst.device->createBufferUnique(img_buf_info);
    }

    vk::UniqueDeviceMemory img_buf_mem;
    {
        auto img_buf_mem_req = vk_inst.device->getBufferMemoryRequirements(*img_buf);

        vk::MemoryAllocateInfo img_buf_alloc_info;
        img_buf_alloc_info.allocationSize = img_buf_mem_req.size;
        img_buf_alloc_info.memoryTypeIndex = vk_inst.getMemoryType(
            img_buf_mem_req.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible);
        img_buf_mem = vk_inst.device->allocateMemoryUnique(img_buf_alloc_info);
    }
    vk_inst.device->bindBufferMemory(*img_buf, *img_buf_mem, 0);

    u8* mapped_img_buf_mem;
    {
        std::basic_ifstream<u8> img_bin{file.data(), std::ios::binary};
        mapped_img_buf_mem =
            reinterpret_cast<u8*>(vk_inst.device->mapMemory(*img_buf_mem, 0, size));
        std::istreambuf_iterator<u8> iter = img_bin;
        std::copy_n(iter, size, mapped_img_buf_mem);
    }
    vk_inst.device->flushMappedMemoryRanges(vk::MappedMemoryRange{*img_buf_mem, 0, size});

    lodepng::encode("before_img.png", mapped_img_buf_mem, width, height, LodePNGColorType::LCT_RGBA,
                    8U);

    Vulkan::FormatConverter format_converter{vk_inst};



    /*{
        vk::UniqueCommandBuffer cmd_buf;
        {
            vk::CommandBufferAllocateInfo cmd_alloc_info;
            cmd_alloc_info.commandPool = *vk_inst.command_pool;
            cmd_alloc_info.level = vk::CommandBufferLevel::ePrimary;
            cmd_alloc_info.commandBufferCount = 1;
            cmd_buf = std::move(vk_inst.device->allocateCommandBuffersUnique(cmd_alloc_info)[0]);
        }
        {
            vk::CommandBufferBeginInfo begin_info{};
            cmd_buf->begin(begin_info);
        }
        {
            vk::BufferImageCopy buf_img_copy;
            buf_img_copy.bufferRowLength = stride;
            buf_img_copy.imageExtent = vk::Extent3D{width, height, 1};
            buf_img_copy.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
            buf_img_copy.imageSubresource.mipLevel = 0;
            buf_img_copy.imageSubresource.baseArrayLayer = 0;
            buf_img_copy.imageSubresource.layerCount = 1;

            vk::ImageMemoryBarrier barrier;
            barrier.image = *img;
            barrier.oldLayout = vk::ImageLayout::eGeneral;
            barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.subresourceRange = SubResourceLayersToRange(buf_img_copy.imageSubresource);

            cmd_buf->pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe,
                                     vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, barrier);
            cmd_buf->copyImageToBuffer(*img, vk::ImageLayout::eTransferSrcOptimal, *img_buf,
                                       buf_img_copy);
            barrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
            barrier.newLayout = vk::ImageLayout::eGeneral;
            cmd_buf->pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                                     vk::PipelineStageFlagBits::eBottomOfPipe, {}, {}, {}, barrier);
        }
        cmd_buf->end();
        vk_inst.SubmitCommandBuffers(*cmd_buf);

        vk_inst.queue.waitIdle();
    }
    vk_inst.device->invalidateMappedMemoryRanges(vk::MappedMemoryRange{*img_buf_mem, 0, size});

    lodepng::encode("img.png", mapped_img_buf_mem, width, height, LodePNGColorType::LCT_RGBA, 8U);*/
}
