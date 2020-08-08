// Copyright 2018 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/alignment.h"
#include "common/assert.h"
#include "video_core/renderer_opengl/gl_stream_buffer.h"

namespace OpenGL {

OGLStreamBuffer::OGLStreamBuffer(Vulkan::Instance& vk_inst, vk::BufferUsageFlags usage,
                                 vk::DeviceSize size, bool prefer_coherent)
    : buffer_size(size) {
    flush_granularity = vk_inst.physical_device.getProperties().limits.nonCoherentAtomSize;
    {
        vk::BufferCreateInfo buffer_create_info;
        buffer_create_info.size = buffer_size;
        buffer_create_info.usage = usage;
        vk_buffer = vk_inst.device->createBufferUnique(buffer_create_info);
    }
    {
        auto buffer_memory_requirements = vk_inst.device->getBufferMemoryRequirements(*vk_buffer);
        vk::MemoryAllocateInfo buffer_allocate_info;
        buffer_allocate_info.allocationSize = buffer_memory_requirements.size;

        vk::ExportMemoryAllocateInfo buffer_export_memory_allocation_info;
        buffer_export_memory_allocation_info.handleTypes =
            vk::ExternalMemoryHandleTypeFlagBits::eOpaqueWin32;
        buffer_allocate_info.pNext = &buffer_export_memory_allocation_info;

        vk::MemoryPropertyFlags flags = vk::MemoryPropertyFlagBits::eHostVisible;
        if (coherent)
            flags |= vk::MemoryPropertyFlagBits::eHostCoherent;
        buffer_allocate_info.memoryTypeIndex =
            vk_inst.getMemoryType(buffer_memory_requirements.memoryTypeBits, flags);

        vk_memory = vk_inst.device->allocateMemoryUnique(buffer_allocate_info);
    }
    vk_inst.device->bindBufferMemory(*vk_buffer, *vk_memory, 0);

    gl_buffer.Create();
    volatile auto target = vkUsageToTarget(usage);
    glBindBuffer(target, gl_buffer.handle);
    shmem_handle = vk_inst.device->getMemoryWin32HandleKHR(
        {*vk_memory, vk::ExternalMemoryHandleTypeFlagBits::eOpaqueWin32});
    gl_memory_object.Create();
    glImportMemoryWin32HandleEXT(gl_memory_object.handle, buffer_size,
                                 GL_HANDLE_TYPE_OPAQUE_WIN32_EXT, shmem_handle);
    glBufferStorageMemEXT(target, buffer_size, gl_memory_object.handle, 0);

    mapped_ptr = static_cast<u8*>(vk_inst.device->mapMemory(*vk_memory, 0, buffer_size));
}

OGLStreamBuffer::~OGLStreamBuffer() {
    if (vk_memory)
        vk_memory.getOwner().unmapMemory(*vk_memory);
}

GLuint OGLStreamBuffer::GetHandle() const {
    return gl_buffer.handle;
}

vk::DeviceSize OGLStreamBuffer::GetSize() const {
    return buffer_size;
}

std::tuple<u8*, vk::DeviceSize, bool> OGLStreamBuffer::Map(vk::DeviceSize size,
                                                           vk::DeviceSize alignment) {
    ASSERT(size <= buffer_size);
    ASSERT(alignment <= buffer_size);
    mapped_size = size;

    if (alignment > 0) {
        buffer_pos = Common::AlignUp<std::size_t>(buffer_pos, alignment);
    }

    bool invalidate = false;
    if (buffer_pos + size > buffer_size) {
        buffer_pos = 0;
        invalidate = true;

        vk_memory.getOwner().unmapMemory(*vk_memory);
    }

    if (invalidate) {
        mapped_ptr = static_cast<u8*>(
            vk_memory.getOwner().mapMemory(*vk_memory, buffer_pos, buffer_size - buffer_pos));
        mapped_offset = buffer_pos;
    }

    return std::make_tuple(mapped_ptr + buffer_pos - mapped_offset, buffer_pos, invalidate);
}

void OGLStreamBuffer::Unmap(vk::DeviceSize size) {
    ASSERT(size <= mapped_size);

    if (!coherent && size > 0) {
        auto pos = buffer_pos - mapped_offset;
        auto aligned_pos = Common::AlignDown(pos, flush_granularity);
        auto length = Common::AlignUp(pos - aligned_pos + size, flush_granularity);
        vk_memory.getOwner().flushMappedMemoryRanges(
            vk::MappedMemoryRange{*vk_memory, aligned_pos, length});
    }

    buffer_pos += size;
}

__declspec(noinline)
GLenum OGLStreamBuffer::vkUsageToTarget(vk::BufferUsageFlags usage) {
    if (usage == vk::BufferUsageFlagBits::eVertexBuffer)
        return GL_ARRAY_BUFFER;
    if (usage == vk::BufferUsageFlagBits::eUniformBuffer)
        return GL_UNIFORM_BUFFER;
    if (usage == vk::BufferUsageFlagBits::eIndexBuffer)
        return GL_ELEMENT_ARRAY_BUFFER;
    if (usage == vk::BufferUsageFlagBits::eStorageTexelBuffer)
        return GL_TEXTURE_BUFFER;
    return {};
}

} // namespace OpenGL
