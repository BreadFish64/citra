// Copyright 2018 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <tuple>
#include <glad/glad.h>
#include "common/common_types.h"
#include "video_core/renderer_opengl/gl_resource_manager.h"
#include "video_core/renderer_vulkan/vk_instance.h"

namespace OpenGL {

class OGLStreamBuffer : private NonCopyable {
public:
    OGLStreamBuffer() = default;
    OGLStreamBuffer(const OGLStreamBuffer&) = delete;
    OGLStreamBuffer(OGLStreamBuffer&&) = default;
    explicit OGLStreamBuffer(Vulkan::Instance& device, vk::BufferUsageFlags usage,
                             vk::DeviceSize size, bool prefer_coherent = false);
    ~OGLStreamBuffer();
    OGLStreamBuffer& operator=(const OGLStreamBuffer&) = delete;
    OGLStreamBuffer& operator=(OGLStreamBuffer&&) = default;

    GLuint GetHandle() const;
    vk::DeviceSize GetSize() const;

    /*
     * Allocates a linear chunk of memory in the GPU buffer with at least "size" bytes
     * and the optional alignment requirement.
     * If the buffer is full, the whole buffer is reallocated which invalidates old chunks.
     * The return values are the pointer to the new chunk, the offset within the buffer,
     * and the invalidation flag for previous chunks.
     * The actual used size must be specified on unmapping the chunk.
     */
    std::tuple<u8*, vk::DeviceSize, bool> Map(vk::DeviceSize size, vk::DeviceSize alignment = 0);

    void Unmap(vk::DeviceSize size);

private:
    OGLBuffer gl_buffer;

    vk::UniqueBuffer vk_buffer;
    vk::UniqueDeviceMemory vk_memory;
    Win32SmartHandle shmem_handle;
    OGLMemoryObject gl_memory_object;

    bool coherent = false;
    bool persistent = false;

    vk::DeviceSize buffer_pos = 0;
    vk::DeviceSize buffer_size = 0;
    vk::DeviceSize mapped_offset = 0;
    vk::DeviceSize mapped_size = 0;
    vk::DeviceSize flush_granularity = 0;
    u8* mapped_ptr = nullptr;

    GLenum vkUsageToTarget(vk::BufferUsageFlags usage);
};

} // namespace OpenGL
