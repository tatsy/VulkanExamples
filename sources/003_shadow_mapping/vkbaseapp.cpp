#include "vkbaseapp.h"

#include <iostream>
#include <array>

#include "vkutils.h"
#include "vkonetimecommand.h"

const std::vector<const char*> validationLayers = {
    "VK_LAYER_LUNARG_standard_validation"
};

static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDebugUtilsMessengerEXT *pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

static void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks *pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData) {
    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
}

VkBaseApp::VkBaseApp(bool isEnableValidationLayer)
    : isEnableValidationLayer_{isEnableValidationLayer} {
}

VkBaseApp::~VkBaseApp() {
    vkDeviceWaitIdle(device_);

    if (window_) {
        glfwDestroyWindow(window_);
    }
}

void VkBaseApp::run() {
    initializeVkBase();

    while (!glfwWindowShouldClose(window_)) {
        glfwPollEvents();

        paintVk();
    }

    vkDeviceWaitIdle(device_);
}

void VkBaseApp::createWindow(int width, int height, const std::string &title) {
    this->width_ = width;
    this->height_ = height;
    this->title_ = title;

    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    window_ = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);

    glfwSetWindowUserPointer(window_, this);
    glfwSetWindowSizeCallback(window_, VkBaseApp::onWindowResized);
}

#pragma region Protected virtual methods

void VkBaseApp::initializeVk() {
}

void VkBaseApp::resizeVk(int width, int height) {
}

void VkBaseApp::paintVk() {
}

void VkBaseApp::createInstance() {
    // Request validation layers.
    VULKAN_ASSERT(
        !isEnableValidationLayer_ || checkValidationLayerSupport(validationLayers),
        "validation layers requested, but not available"
    );

    // Application info.
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = title_.c_str();
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    // Instance create info.
    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    // Get required extensions (necessary to get OS supports).
    auto extensions = getRequiredExtensions(isEnableValidationLayer_);
    createInfo.enabledExtensionCount = extensions.size();
    createInfo.ppEnabledExtensionNames = extensions.data();

    // Setup debug callback.
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
    if (isEnableValidationLayer_) {
        createInfo.enabledLayerCount = validationLayers.size();
        createInfo.ppEnabledLayerNames = validationLayers.data();

        populateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *) &debugCreateInfo;
    } else {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
    }

    // Create instance.
    CHECK_VULKAN_RUNTIME_ERROR(
            vkCreateInstance(&createInfo, nullptr, &instance_),
            "failed to create instance!"
    );
}

void VkBaseApp::setupDebugMessenger() {
    if (!isEnableValidationLayer_) return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    populateDebugMessengerCreateInfo(createInfo);

    if (CreateDebugUtilsMessengerEXT(instance_, &createInfo, nullptr, &debugMessenger_) != VK_SUCCESS) {
        throw std::runtime_error("failed to set up debug messenger!");
    }
}

void VkBaseApp::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
}

void VkBaseApp::createSurface() {
    CHECK_VULKAN_RUNTIME_ERROR(
        glfwCreateWindowSurface(instance_, window_, nullptr, &surface_),
        "failed to create window surface!"
    );
}

void VkBaseApp::pickPhysicalDevice() {
    // Check number of physical devices.
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance_, &deviceCount, nullptr);
    VULKAN_ASSERT(deviceCount != 0, "failed to find GPUs with Vulkan support!");

    // Get physical device data.
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance_, &deviceCount, devices.data());

    for (const auto& device : devices) {        
        if (isDeviceSuitable(device, surface_)) {
            physicalDevice_ = device;
            break;
        }
    }

    VULKAN_ASSERT(physicalDevice_ != VK_NULL_HANDLE, "failed to find a suitable GPU!");
}

void VkBaseApp::createLogicalDevice() {
    QueueFamilyIndices indices = findQueueFamilies(physicalDevice_, surface_);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<int> uniqueQueueFamilies = {indices.graphicsFamily, indices.presentFamily};

    float queuePriority = 1.0f;
    for (int queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures = {};
    deviceFeatures.samplerAnisotropy = VK_TRUE;

    VkDeviceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.queueCreateInfoCount = (uint32_t) queueCreateInfos.size();

    createInfo.pEnabledFeatures = &deviceFeatures;

    createInfo.enabledExtensionCount = deviceExtensions.size();
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    if (isEnableValidationLayer_) {
        createInfo.enabledLayerCount = validationLayers.size();
        createInfo.ppEnabledLayerNames = validationLayers.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }

    if (vkCreateDevice(physicalDevice_, &createInfo, nullptr, &device_) != VK_SUCCESS) {
        throw std::runtime_error("failed to create logical device!");
    }

    vkGetDeviceQueue(device_, indices.graphicsFamily, 0, &graphicsQueue_);
    vkGetDeviceQueue(device_, indices.presentFamily, 0, &presentQueue_);
}

void VkBaseApp::createSwapChain() {
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice_, surface_);

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities, width_, height_);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface_;

    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices = findQueueFamilies(physicalDevice_, surface_);
    uint32_t queueFamilyIndices[] = {(uint32_t) indices.graphicsFamily, (uint32_t) indices.presentFamily};

    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;

    VkSwapchainKHR oldSwapChain = swapChainHandler_.swapChain;
    createInfo.oldSwapchain = oldSwapChain;

    VkSwapchainKHR newSwapChain;
    CHECK_VULKAN_RUNTIME_ERROR(
        vkCreateSwapchainKHR(device(), &createInfo, nullptr, &newSwapChain),
        "failed to create swap chain!"
    );

    // Update swapchain.
    swapChainHandler_.swapChain = newSwapChain;

    vkGetSwapchainImagesKHR(device_, swapChainHandler_.swapChain, &imageCount, nullptr);
    swapChainHandler_.images.resize(imageCount);
    
    vkGetSwapchainImagesKHR(device(), swapChainHandler_.swapChain, &imageCount, swapChainHandler_.images.data());

    swapChainHandler_.format = surfaceFormat.format;
    swapChainHandler_.extent = extent;

    // Create image views for new swapchain.
    swapChainHandler_.imageViews.resize(swapChainHandler_.images.size());

    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = swapChainHandler_.format;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    for (uint32_t i = 0; i < swapChainHandler_.images.size(); i++) {
        viewInfo.image = swapChainHandler_.images[i];
        CHECK_VULKAN_RUNTIME_ERROR(
            vkCreateImageView(device_, &viewInfo, nullptr, &swapChainHandler_.imageViews[i]),
            "failed to create swapchain image view!"
        );
    }

    // Create depth buffer.
    VkFormat depthFormat = findDepthFormat(physicalDevice_);

    createImage(swapChainHandler_.extent.width, swapChainHandler_.extent.height, depthFormat,
                VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, swapChainHandler_.depthImage, swapChainHandler_.depthImageMemory);

    createImageView(swapChainHandler_.depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, swapChainHandler_.depthImageView);

    transferImageLayout(swapChainHandler_.depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
}

void VkBaseApp::setupFramebuffers(const std::function<void(VkFramebuffer *, const VkImageView &, const VkImageView &)> &framebufferFn) {
    swapChainHandler_.framebuffers.resize(swapChainHandler_.imageViews.size());

    for (size_t i = 0; i < swapChainHandler_.imageViews.size(); i++) {
        framebufferFn(&swapChainHandler_.framebuffers[i],
                      swapChainHandler_.imageViews[i],
                      swapChainHandler_.depthImageView);
    }
}

void VkBaseApp::createCommandPool() {
    QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice_, surface_);

    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;

    CHECK_VULKAN_RUNTIME_ERROR(
        vkCreateCommandPool(device(), &poolInfo, nullptr, &commandPool_),
        "failed to create graphics command pool!"
    );
}

void VkBaseApp::setupRenderCommandBuffers(const std::function<void(const VkCommandBuffer&, const VkFramebuffer&)> &commandFn) {
    // Reset command buffers.
    if (commandBuffers_.size() > 0) {
        vkFreeCommandBuffers(device(), commandPool_, commandBuffers_.size(), commandBuffers_.data());
    }

    // Re-allocate command buffers.
    commandBuffers_.resize(swapChainHandler_.framebuffers.size());

    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool_;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t)commandBuffers_.size();

    CHECK_VULKAN_RUNTIME_ERROR(
        vkAllocateCommandBuffers(device(), &allocInfo, commandBuffers_.data()),
        "failed to allocate command buffers!"
    );

    // Setup commands for swapchains.
    for (size_t i = 0; i < commandBuffers_.size(); i++) {
        commandFn(commandBuffers_[i], swapChainHandler_.framebuffers[i]);    
    }
}

void VkBaseApp::queueSubmit(const VkSubmitInfo &submitInfo) {
    CHECK_VULKAN_RUNTIME_ERROR(
        vkQueueSubmit(graphicsQueue_, 1, &submitInfo, VK_NULL_HANDLE),
        "failed to submit draw command buffer!"
    );
}

#pragma endregion

#pragma region Protected methods

void VkBaseApp::createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) {
    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(device(), &imageInfo, nullptr, &image) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image!");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device(), image, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(physicalDevice_, memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(device(), &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate image memory!");
    }

    vkBindImageMemory(device(), image, imageMemory, 0);
}

void VkBaseApp::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, VkImageView& imageView) {
    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(device(), &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture image view!");
    }
}

void VkBaseApp::transferImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
    VkOneTimeCommand oneTimeCommand(this);
    oneTimeCommand.begin();
        
    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;

    if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

        if (hasStencilComponent(format)) {
            barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    } else {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    } else {
        throw std::invalid_argument("unsupported layout transition!");
    }

    vkCmdPipelineBarrier(
        oneTimeCommand.command(),
        sourceStage, destinationStage,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    oneTimeCommand.end();
}


#pragma endregion

#pragma region Private methods

void VkBaseApp::initializeVkBase() {
    createInstance();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createCommandPool();
    createSwapChain();

    initializeVk();
}

void VkBaseApp::resizeVkBase(int width, int height) {
    vkDeviceWaitIdle(device_);
    createSwapChain();

    resizeVk(width, height);
}

int VkBaseApp::startPaint(const VkSemaphore &waitSemaphore) {
    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(device(), swapChainHandler_.swapChain,
                                            std::numeric_limits<uint64_t>::max(),
                                            waitSemaphore, VK_NULL_HANDLE, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        resizeVkBase(-1, -1);
        return -1;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    return imageIndex;
}

void VkBaseApp::endPaint(int imageIndex, const VkSemaphore &waitSemaphore) {
    // Presentation
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &waitSemaphore;

    VkSwapchainKHR swapChains[] = { swapChainHandler_.swapChain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;

    std::array<uint32_t, 1> imageIndices = { (uint32_t)imageIndex };
    presentInfo.pImageIndices = imageIndices.data();

    VkResult result = vkQueuePresentKHR(presentQueue_, &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        resizeVkBase(-1, -1);
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image!");
    }
}

void VkBaseApp::onWindowResized(GLFWwindow *window, int width, int height) {
    if (width == 0 || height == 0) return;

    VkBaseApp *app = reinterpret_cast<VkBaseApp *>(glfwGetWindowUserPointer(window));
    app->resizeVkBase(width, height);    
}

#pragma endregion

