#ifdef _MSC_VER
#pragma once
#endif

#ifndef _VK_BASE_APP_H_
#define _VK_BASE_APP_H_

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>

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

    inline const VkDevice & device() const { return device_; }
    inline const VkPhysicalDevice & physicalDevice() const { return physicalDevice_; }
    inline const VkCommandPool & commandPool() const { return commandPool_; }

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
    virtual int  startPaint(const VkSemaphore &waitSemaphore);
    virtual void endPaint(int imageIndex, const VkSemaphore &waitSemaphore);

    virtual void createInstance();
    virtual void setupDebugMessenger();
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

    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo);
    void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
                     VkMemoryPropertyFlags property, VkImage &image, VkDeviceMemory &imageMemory);

    void createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, VkImageView& imageView);
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

    VkInstance instance_;
    VkDebugUtilsMessengerEXT debugMessenger_;
    VkSurfaceKHR surface_;

    VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;
    VkDevice device_;

    VkQueue graphicsQueue_;
    VkQueue presentQueue_;

    VkSwapchainHandler swapChainHandler_;

    VkCommandPool commandPool_;

    std::vector<VkCommandBuffer> commandBuffers_;

    // Friend classes.
    friend class VkMemoryBase;
    friend class VkOneTimeCommand;
};

#endif  // _VK_BASE_APP_H_
