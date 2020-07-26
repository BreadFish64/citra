#pragma once

#include <Windows.h>

namespace vk {
class DispatchLoaderDynamic;
}

namespace Vulkan {
inline vk::DispatchLoaderDynamic& GetDefaultDispatcher();
}

#define VK_USE_PLATFORM_WIN32_KHR
#define VULKAN_HPP_DEFAULT_DISPATCHER_TYPE ::vk::DispatchLoaderDynamic
#define VULKAN_HPP_DEFAULT_DISPATCHER ::Vulkan::GetDefaultDispatcher()
#include <vulkan/vulkan.hpp>

#include "video_core/renderer_opengl/gl_resource_manager.h"

#include "common/logging/log.h"

inline vk::DynamicLoader dl;

class Win32SmartHandle : private NonCopyable {
    HANDLE handle = NULL;

public:
    Win32SmartHandle() = default;
    Win32SmartHandle(HANDLE&& handle) : handle{handle} {}

    ~Win32SmartHandle() {
        Release();
    }

    void operator=(HANDLE&& handle) {
        Release();
        this->handle = handle;
    }

    operator HANDLE() {
        return handle;
    }

    /// Deletes the internal OpenGL resource
    void Release() {
        if (handle > NULL) {
            CloseHandle(handle);
            handle = NULL;
        }
    }
};

static VKAPI_ATTR vk::Bool32 VKAPI_CALL
VulkanDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                    VkDebugUtilsMessageTypeFlagsEXT messageType,
                    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void*) {
    Log::Level log_level{};
    switch (static_cast<vk::DebugUtilsMessageSeverityFlagBitsEXT>(messageSeverity)) {
    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo:
    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose:
        log_level = Log::Level::Info;
        break;
    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning:
        log_level = Log::Level::Warning;
        break;
    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eError:
        log_level = Log::Level::Error;
        break;
    default:
        break;
    }
    LOG_GENERIC(Log::Class::Render_Vulkan, log_level, "{}", pCallbackData->pMessage);
    return VK_FALSE;
}

namespace Vulkan {
vk::DispatchLoaderDynamic& GetDefaultDispatcher() {
    static vk::DispatchLoaderDynamic loader;
    return loader;
}

struct SharedSemaphore {
    vk::UniqueSemaphore vk_handle{};
    GLuint gl_handle{NULL};

    SharedSemaphore& operator=(SharedSemaphore&& rhs) {
        vk_handle = std::move(rhs.vk_handle);
        gl_handle = std::move(rhs.gl_handle);
        rhs.gl_handle = NULL;
        return *this;
    };
    SharedSemaphore() = default;
    SharedSemaphore(SharedSemaphore&& rhs){
        *this = std::move(rhs);
    }
    SharedSemaphore(vk::Device device) {
        {
            vk::ExportSemaphoreCreateInfo export_info;
            export_info.handleTypes = vk::ExternalSemaphoreHandleTypeFlagBits::eOpaqueWin32Kmt;
            vk::SemaphoreCreateInfo semaphore_create_info;
            semaphore_create_info.pNext = &export_info;
            vk_handle = device.createSemaphoreUnique(semaphore_create_info);
        }
        vk::SemaphoreGetWin32HandleInfoKHR semaphore_win_info;
        semaphore_win_info.semaphore = *vk_handle;
        semaphore_win_info.handleType = vk::ExternalSemaphoreHandleTypeFlagBits::eOpaqueWin32Kmt;
        HANDLE win_handle = device.getSemaphoreWin32HandleKHR(semaphore_win_info);
        glGenSemaphoresEXT(1, &gl_handle);
        glImportSemaphoreWin32HandleEXT(gl_handle, GL_HANDLE_TYPE_OPAQUE_WIN32_KMT_EXT, win_handle);
    }

    ~SharedSemaphore() {
        if (gl_handle) {
            LOG_CRITICAL(Render_Vulkan, "meep");
            glDeleteSemaphoresEXT(1, &gl_handle);
        }
    }
};

struct Instance {
    static constexpr std::array<const char*, 1> validation_layers{"VK_LAYER_KHRONOS_validation"};
    static constexpr std::array<const char*, 2> instance_extensions{
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME, VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME};
    static constexpr std::array<const char*, 6> device_extensions{
        VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME,       VK_KHR_EXTERNAL_SEMAPHORE_EXTENSION_NAME,
        VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME, VK_KHR_EXTERNAL_SEMAPHORE_WIN32_EXTENSION_NAME,
        VK_KHR_SHADER_FLOAT16_INT8_EXTENSION_NAME,   VK_KHR_8BIT_STORAGE_EXTENSION_NAME};

    vk::UniqueInstance instance;
    vk::UniqueDebugUtilsMessengerEXT messenger;
    vk::PhysicalDevice physical_device;
    vk::UniqueDevice device;
    vk::Queue queue;
    vk::UniqueCommandPool command_pool;

    f32 queue_priority = 1.0;

    bool enable_validation = true;

    Instance(std::string_view app_name = "Citra") {
        {
            PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr =
                dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
            VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);
        }
        // Initialize Vulkan instance
        {
            vk::ApplicationInfo info;
            info.setApiVersion(VK_API_VERSION_1_1);
            info.setPApplicationName(app_name.data());
            vk::InstanceCreateInfo create_info;
            create_info.setPApplicationInfo(&info);
            if (enable_validation) {
                create_info.setEnabledLayerCount(validation_layers.size());
                create_info.setPpEnabledLayerNames(validation_layers.data());
            }
            create_info.setEnabledExtensionCount(instance_extensions.size());
            create_info.setPpEnabledExtensionNames(instance_extensions.data());

            instance = vk::createInstanceUnique(create_info);
            VULKAN_HPP_DEFAULT_DISPATCHER.init(*instance);
        }
        // Setup debug callback
        if (enable_validation) {
            vk::DebugUtilsMessengerCreateInfoEXT messenger_info;
            messenger_info.setPfnUserCallback(VulkanDebugCallback);
            constexpr auto severity_flags{vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
                                          vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                                          vk::DebugUtilsMessageSeverityFlagBitsEXT::eError};
            constexpr auto type_flags{vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                                      vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
                                      vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation};
            messenger_info.setMessageSeverity(severity_flags);
            messenger_info.setMessageType(type_flags);
            messenger = instance->createDebugUtilsMessengerEXTUnique(messenger_info);
        }
        {
            const auto physical_devices = instance->enumeratePhysicalDevices();
            physical_device = physical_devices[0];
        }
        {
            LOG_INFO(Render_Vulkan, "{}", physical_device.getProperties().deviceName);
            LOG_INFO(Render_Vulkan, "Vulkan {}.{}.{}",
                     VK_VERSION_MAJOR(physical_device.getProperties().apiVersion),
                     VK_VERSION_MINOR(physical_device.getProperties().apiVersion),
                     VK_VERSION_PATCH(physical_device.getProperties().apiVersion));
            for (auto format : {
                     vk::Format::eR8G8B8A8Unorm,
                     vk::Format::eB8G8R8A8Unorm,
                     vk::Format::eR8G8B8Unorm,
                     vk::Format::eB8G8R8Unorm,
                     vk::Format::eR5G5B5A1UnormPack16,
                     vk::Format::eR5G6B5UnormPack16,
                     vk::Format::eR4G4B4A4UnormPack16,
                     vk::Format::eR8G8Unorm,
                     vk::Format::eR8Unorm,
                     vk::Format::eR4G4UnormPack8,
                     vk::Format::eEtc2R8G8B8UnormBlock,
                     vk::Format::eEtc2R8G8B8A8UnormBlock,
                     vk::Format::eD16Unorm,
                     vk::Format::eD24UnormS8Uint,
                     vk::Format::eX8D24UnormPack32,
                }) {
                auto format_properties = physical_device.getFormatProperties(format);
                LOG_INFO(Render_Vulkan, "{} linear: {}, tiling: {}", vk::to_string(format),
                         vk::to_string(format_properties.linearTilingFeatures),
                         vk::to_string(format_properties.optimalTilingFeatures));
            }
        }
        u32 queue_family_index{0};
        {
            auto queue_families = physical_device.getQueueFamilyProperties();
            constexpr auto queue_flags{vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eTransfer |
                                       vk::QueueFlagBits::eCompute};
            for (; queue_family_index != queue_families.size(); ++queue_family_index) {
                const auto& queue_family = queue_families[queue_family_index];
                if (queue_family.queueFlags & queue_flags)
                    break;
            }

            vk::DeviceQueueCreateInfo queue_info;
            queue_info.setQueueCount(1);
            queue_info.setQueueFamilyIndex(queue_family_index);
            queue_info.setPQueuePriorities(&queue_priority);

            vk::PhysicalDeviceFloat16Int8FeaturesKHR shader_int8_feature;
            shader_int8_feature.shaderInt8 = true;
            vk::PhysicalDevice8BitStorageFeatures storage_8bit_feature;
            storage_8bit_feature.storageBuffer8BitAccess = true;

            vk::PhysicalDeviceFeatures2 feature_chain;
            feature_chain.features.shaderInt16 = true;
            feature_chain.pNext = &shader_int8_feature;
            shader_int8_feature.pNext = &storage_8bit_feature;

            vk::DeviceCreateInfo device_info;
            device_info.setQueueCreateInfoCount(1);
            device_info.setPQueueCreateInfos(&queue_info);
            device_info.enabledExtensionCount = device_extensions.size();
            device_info.ppEnabledExtensionNames = device_extensions.data();
            device_info.pNext = &feature_chain;

            device = physical_device.createDeviceUnique(device_info);
            queue = device->getQueue(queue_family_index, 0);
        }
        {
            vk::CommandPoolCreateInfo command_pool_info;
            command_pool_info.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
            command_pool_info.queueFamilyIndex = queue_family_index;
            command_pool = device->createCommandPoolUnique(command_pool_info);
        }
    }

    u32 getMemoryType(u32 type_filter, vk::MemoryPropertyFlags properties) {
        const auto physical_memory_properties = physical_device.getMemoryProperties();
        for (u32 idx = 0; idx < physical_memory_properties.memoryTypeCount; idx++) {
            if ((type_filter & (1 << idx)) &&
                (physical_memory_properties.memoryTypes[idx].propertyFlags & properties) ==
                    properties) {
                return idx;
            }
        }
        throw std::runtime_error("failed to find suitable memory type!");
    }

    void SubmitCommandBuffers(vk::ArrayProxy<vk::CommandBuffer> cmd_buffs,
                              vk::Fence fence = nullptr) {
        vk::SubmitInfo submit_info;
        submit_info.pCommandBuffers = cmd_buffs.data();
        submit_info.commandBufferCount = cmd_buffs.size();
        queue.submit(submit_info, nullptr);
    }
};

inline vk::ImageSubresourceRange SubResourceLayersToRange(const vk::ImageSubresourceLayers& in) {
    vk::ImageSubresourceRange out;
    out.aspectMask = in.aspectMask;
    out.baseArrayLayer = in.baseArrayLayer;
    out.layerCount = in.layerCount;
    out.baseMipLevel = in.mipLevel;
    out.levelCount = 1;
    return out;
}

} // namespace Vulkan
