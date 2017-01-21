#ifdef _MSC_VER
#pragma once
#endif

#ifndef _VK_TEXTURE_H_
#define _VK_TEXTURE_H_

#include "vkimagebase.h"

class VkTexture : public VkImageBase {
public:
    VkTexture(VkBaseApp *parent)
        : VkImageBase{parent} {
    }

    virtual void create(int width, int height, VkFormat format,
                        VkImageTiling tiling, VkImageUsageFlags usage,
                        VkMemoryPropertyFlags properties,
                        VkImageAspectFlags aspect) override {
        
        VkImageBase::create(width, height, format, tiling, usage, properties, aspect);

        createSampler();

    }

    const VkUniquePtr<VkSampler> & sampler() const {
        return sampler_;
    }

private:
    void createSampler() {
        VkSamplerCreateInfo samplerInfo = {};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.maxAnisotropy = 0;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_WHITE;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

        CHECK_VULKAN_RUNTIME_ERROR(
            vkCreateSampler(parentDevice(), &samplerInfo, nullptr, sampler_.replace()),
            "failed to create depth sampler for offscreen!"
        );
    }
    
    

    VkUniquePtr<VkSampler> sampler_{parentDevice(), vkDestroySampler};
};

#endif  // _VK_TEXTURE_H_
