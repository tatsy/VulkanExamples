#ifdef _MSC_VER
#pragma once
#endif

#ifndef _VK_UTILS_H_
#define _VK_UTILS_H_

#include <sstream>
#include <vector>
#include <set>
#include <stdexcept>
#include <cassert>

#include <vulkan/vulkan.h>

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

#ifndef __FUNCTION_NAME__
    #if defined(_WIN32) || defined(__WIN32__)
        #define __FUNCTION_NAME__ __FUNCTION__
    #else
        #define __FUNCTION_NAME__ __func__
    #endif
#endif

// Check predicate and throw runtime_error if failed.
#define VULKAN_ASSERT(PREDICATE, MESSAGE) \
do { \
    if (!(PREDICATE)) { \
        std::stringstream ss; \
        ss << "Exception thrown from " << __FILE__ \
        << " line " << __LINE__ << " in function " << (__FUNCTION_NAME__) \
        << " : " << (MESSAGE); \
        throw std::runtime_error(ss.str()); \
    } \
} while (false)

// Check Vulkan operation errors.
#ifndef NDEBUG
#define CHECK_VULKAN_RUNTIME_ERROR(RESULT, MESSAGE) \
do { \
    if ((RESULT) != VK_SUCCESS) { \
        std::cerr << "Exception thrown from " << __FILE__ \
        << " line " << __LINE__ << " in function " << (__FUNCTION_NAME__) \
        << " : " << (MESSAGE); \
        std::abort(); \
    } \
} while (false)
#else
#define CHECK_VULKAN_RUNTIME_ERROR(RESULT, MESSAGE) \
do { \
    if ((RESULT) != VK_SUCCESS) { \
        std::stringstream ss; \
        ss << "Exception thrown from " << __FILE__ \
        << " line " << __LINE__ << " in function " << (__FUNCTION_NAME__) \
        << " : " << (MESSAGE); \
        throw std::runtime_error(ss.str()); \
    } \
} while (false)
#endif

// An object to manage queue families.
struct QueueFamilyIndices {
    int graphicsFamily = -1;
    int presentFamily = -1;

    bool isComplete() {
        return graphicsFamily >= 0 && presentFamily >= 0;
    }
};

bool checkValidationLayerSupport(const std::vector<const char *>& validationLayers);

std::vector<const char*> getRequiredExtensions(bool isEnableValidationLayers);

QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, const VkSurfaceKHR &surface);

bool checkDeviceExtensionSupport(VkPhysicalDevice device);

bool isDeviceSuitable(VkPhysicalDevice device, const VkSurfaceKHR &surface);

inline bool hasStencilComponent(VkFormat format) {
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

VkFormat findSupportedFormat(
    const VkPhysicalDevice physicalDevice,
    const std::vector<VkFormat>& candidates,
    VkImageTiling tiling,
    VkFormatFeatureFlags features);

uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);

VkFormat findDepthFormat(const VkPhysicalDevice &physicalDevice);

#endif  // _VK_UTILS_H_
