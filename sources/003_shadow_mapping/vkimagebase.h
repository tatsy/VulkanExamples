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

        this->width_ = width;
        this->height_ = height;
        this->format_ = format;
        this->tiling_ = tiling;
        this->aspect_ = aspect;

        createImage(width, height, format, tiling, usage, properties, image_, imageMemory_);
        createImageView(format, aspect, imageView_);
    }

    void setData(uint8_t *bytes, size_t imageSize) {
        VkImage stagingImage;
        VkDeviceMemory stagingImageMemory;
        createImage(width(), height(), format(), VK_IMAGE_TILING_LINEAR,
                    VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                    stagingImage, stagingImageMemory);
        
        VkImageSubresource subresource = {};
        subresource.aspectMask = aspect();
        subresource.mipLevel = 0;
        subresource.arrayLayer = 0;
        
        VkSubresourceLayout stagingImageLayout;
        vkGetImageSubresourceLayout(parentDevice(), stagingImage, &subresource, &stagingImageLayout);
        
        void* data;
        vkMapMemory(parentDevice(), stagingImageMemory, 0, imageSize, 0, &data);
        
        if (stagingImageLayout.rowPitch == width() * 4) {
            memcpy(data, bytes, (size_t) imageSize);
        } else {
            uint8_t* dataBytes = reinterpret_cast<uint8_t*>(data);
            
            for (int y = 0; y < height(); y++) {
                memcpy(&dataBytes[y * stagingImageLayout.rowPitch], &bytes[y * width() * 4], width() * 4);
            }
        }
        
        vkUnmapMemory(parentDevice(), stagingImageMemory);
        
        changeImageLayout(stagingImage, format_, VK_IMAGE_LAYOUT_PREINITIALIZED,
                          VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
        changeImageLayout(image_, format_, VK_IMAGE_LAYOUT_PREINITIALIZED,
                              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        
        copyImage(stagingImage, image_, width_, height_);
        
        changeImageLayout(image_, format_, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    const VkImageView & imageView() const {
        return imageView_;
    }
    
    inline int width() const { return width_; }
    inline int height() const { return height_; }
    inline VkFormat format() const { return format_; }
    inline VkImageTiling tiling() const { return tiling_; }
    inline VkImageAspectFlags aspect() const { return aspect_; }

protected:
    void changeImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout,
                               VkImageLayout newLayout) {
        oneTimeCommand().begin();
        
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
        
        if (oldLayout == VK_IMAGE_LAYOUT_PREINITIALIZED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        } else if (oldLayout == VK_IMAGE_LAYOUT_PREINITIALIZED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        } else {
            throw std::invalid_argument("unsupported layout transition!");
        }
        
        vkCmdPipelineBarrier(
            oneTimeCommand().command(),
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier);
        
        oneTimeCommand().end();
    }
    
    void copyImage(VkImage srcImage, VkImage dstImage, uint32_t width, uint32_t height) {
        oneTimeCommand().begin();
        
        VkImageSubresourceLayers subResource = {};
        subResource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        subResource.baseArrayLayer = 0;
        subResource.mipLevel = 0;
        subResource.layerCount = 1;
        
        VkImageCopy region = {};
        region.srcSubresource = subResource;
        region.dstSubresource = subResource;
        region.srcOffset = {0, 0, 0};
        region.dstOffset = {0, 0, 0};
        region.extent.width = width;
        region.extent.height = height;
        region.extent.depth = 1;
        
        vkCmdCopyImage(
                       oneTimeCommand().command(),
                       srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                       dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                       1, &region
                       );
        
        oneTimeCommand().end();
    }

private:
    void createImage(int width, int height, VkFormat format,
                     VkImageTiling tiling, VkImageUsageFlags usage,
                     VkMemoryPropertyFlags properties,
                     VkImage &image, VkDeviceMemory &imageMemory) {

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
            vkCreateImage(parentDevice(), &imageInfo, nullptr, &image),
            "failed to create image!"
        );

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(parentDevice(), image, &memRequirements);

        VkMemoryAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

        CHECK_VULKAN_RUNTIME_ERROR(
            vkAllocateMemory(parentDevice(), &allocInfo, nullptr, &imageMemory),
            "failed to allocate image memory!"
        );

        vkBindImageMemory(parentDevice(), image, imageMemory, 0);
    }

    void createImageView(VkFormat format, VkImageAspectFlags aspectFlags, VkImageView &imageView) const {
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
            vkCreateImageView(parentDevice(), &viewInfo, nullptr, &imageView),
            "failed to create texture image view!"
        );
    }

private:
    int width_ = 0;
    int height_ = 0;
    VkFormat format_;
    VkImageTiling tiling_;
    VkImageAspectFlags aspect_;

    VkImage image_;
    VkImageView imageView_;
    VkDeviceMemory imageMemory_;
};

#endif  // _VK_IMAGE_BASE_H_
