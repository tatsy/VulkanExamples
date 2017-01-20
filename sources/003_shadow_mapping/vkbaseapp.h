#ifdef _MSC_VER
#pragma once
#endif

#ifndef _VK_BASE_APP_H_
#define _VK_BASE_APP_H_

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>

#include "vksmartptr.h"
#include "vkswapchainutils.h"

// Function prototype declarations.
void destroyDebugReportCallbackEXT(
    VkInstance instance,
    VkDebugReportCallbackEXT callback,
    const VkAllocationCallbacks* pAllocator);

/**
 * VkBaseApp
 */
class VkBaseApp {
public:
    VkBaseApp(bool isEnableValidationLayer = false);
    virtual ~VkBaseApp();

    void run();
    void createWindow(int width, int height, const std::string &title);

    inline GLFWwindow * const window() const { return window_; }

    inline const VkUniquePtr<VkDevice> & device() const { return device_; }
    inline const VkPhysicalDevice & physicalDevice() const { return physicalDevice_; }
    inline const VkUniquePtr<VkCommandPool> & commandPool() const { return commandPool_; }

    inline int width() const { return swapChainHandler_.extent.width; }
    inline int height() const { return swapChainHandler_.extent.height; }

    inline const VkCommandBuffer * const getCommandBufferPtr(int swapChainIndex) const {
        return &commandBuffers_[swapChainIndex];
    }

    inline VkFormat imageFormat() const { return swapChainHandler_.format; }

protected:
    virtual void initializeVk();
    virtual void resizeVk(int width, int height);
    virtual void paintVk();
    virtual uint32_t startPaint(const VkUniquePtr<VkSemaphore> &waitSemaphore);
    virtual void endPaint(int imageIndex, const VkUniquePtr<VkSemaphore> &waitSemaphore);

    virtual void createInstance();
    virtual void createSurface();
    virtual void pickPhysicalDevice();
    virtual void createLogicalDevice();
    virtual void createSwapChain();

    virtual void createCommandPool();

    // Setup framebuffers for swapchains.
    virtual void setupFramebuffers(const std::function<void(VkFramebuffer *, const VkImageView &, const VkImageView &)> &framebufferFn);

    // Setup command buffers for swapchains.
    virtual void setupRenderCommandBuffers(const std::function<void(const VkCommandBuffer &, const VkFramebuffer &)> &commandFn);

    virtual void queueSubmit(const VkSubmitInfo &submitInfo);

    void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
                     VkMemoryPropertyFlags property, VkUniquePtr<VkImage> &image, VkUniquePtr<VkDeviceMemory> &imageMemory);

    void createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, VkUniquePtr<VkImageView>& imageView);
    void transferImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);

private:
    void initializeVkBase();
    void resizeVkBase(int width, int height);
    void paintVkBase();

    static void onWindowResized(GLFWwindow *window, int width, int height);

    int width_;
    int height_;
    std::string title_;

    GLFWwindow *window_;
    bool isEnableValidationLayer_ = false;

    VkUniquePtr<VkInstance> instance_{vkDestroyInstance};
    VkUniquePtr<VkDebugReportCallbackEXT> debugCallback_{instance_, destroyDebugReportCallbackEXT};

    VkUniquePtr<VkSurfaceKHR> surface_{instance_, vkDestroySurfaceKHR};

    VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;
    VkUniquePtr<VkDevice> device_{vkDestroyDevice};

    VkQueue graphicsQueue_;
    VkQueue presentQueue_;

    VkSwapchainHandler swapChainHandler_{device_};

    VkUniquePtr<VkCommandPool> commandPool_{device_, vkDestroyCommandPool};

    std::vector<VkCommandBuffer> commandBuffers_;

    // Friend classes.
    friend class VkMemoryBase;
    friend class VkOneTimeCommand;
};

#endif  // _VK_BASE_APP_H_
