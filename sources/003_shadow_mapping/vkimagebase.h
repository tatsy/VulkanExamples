#ifdef _MSC_VER
#pragma once
#endif

#ifndef _VK_IMAGE_BASE_H_
#define _VK_IMAGE_BASE_H_

#include "vkbaseapp.h"
#include "vkmemorybase.h"

class VkImageBase : public VkMemoryBase {
public:
    VkImageBase(VkBaseApp *parent)
        : VkMemoryBase{parent} {
    }

    virtual ~VkImageBase() {
    }

    virtual void create(int width, int height, VkFormat format,
                        VkImageTiling tiling, VkImageUsageFlags usage,
                        VkMemoryPropertyFlags properties,
                        VkImageAspectFlags aspect) {

        createImage(width, height, format, tiling, usage, properties);
        createImageView(format, aspect);
    }

    const VkUniquePtr<VkImageView> & imageView() const {
        return imageView_;
    }

private:
    void createImage(int width, int height, VkFormat format,
                     VkImageTiling tiling, VkImageUsageFlags usage,
                     VkMemoryPropertyFlags properties) {

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

        CHECK_VULKAN_RUNTIME_ERROR(
            vkCreateImage(parentDevice(), &imageInfo, nullptr, image_.replace()),
            "failed to create image!"
        );

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(parentDevice(), image_, &memRequirements);

        VkMemoryAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

        CHECK_VULKAN_RUNTIME_ERROR(
            vkAllocateMemory(parentDevice(), &allocInfo, nullptr, imageMemory_.replace()),
            "failed to allocate image memory!"
        );

        vkBindImageMemory(parentDevice(), image_, imageMemory_, 0);    
    }

    void createImageView(VkFormat format, VkImageAspectFlags aspectFlags) {
        VkImageViewCreateInfo viewInfo = {};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image_;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = aspectFlags;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        CHECK_VULKAN_RUNTIME_ERROR(
            vkCreateImageView(parentDevice(), &viewInfo, nullptr, imageView_.replace()),
            "failed to create texture image view!"
        );
    }

private:
    VkUniquePtr<VkImage> image_{parentDevice(), vkDestroyImage};
    VkUniquePtr<VkImageView> imageView_{parentDevice(), vkDestroyImageView};
    VkUniquePtr<VkDeviceMemory> imageMemory_{parentDevice(), vkFreeMemory};
};

#endif  // _VK_IMAGE_BASE_H_
