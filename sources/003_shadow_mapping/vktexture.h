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

        // Initialize sampler info.
        samplerInfo_.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo_.magFilter = VK_FILTER_LINEAR;
        samplerInfo_.minFilter = VK_FILTER_LINEAR;
        samplerInfo_.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo_.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo_.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo_.anisotropyEnable = VK_TRUE;
        samplerInfo_.maxAnisotropy = 16;
        samplerInfo_.borderColor = VK_BORDER_COLOR_INT_OPAQUE_WHITE;
        samplerInfo_.unnormalizedCoordinates = VK_FALSE;
        samplerInfo_.compareEnable = VK_FALSE;
        samplerInfo_.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo_.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    }

    virtual void create(int width, int height, VkFormat format,
                        VkImageTiling tiling, VkImageUsageFlags usage,
                        VkMemoryPropertyFlags properties,
                        VkImageAspectFlags aspect) override {
        
        VkImageBase::create(width, height, format, tiling, usage, properties, aspect);

        createSampler();
    }
    
    void setMagnificationFilter(VkFilter magFilter) {
        samplerInfo_.magFilter = magFilter;
    }
    
    void setMinificationFilter(VkFilter minFilter) {
        samplerInfo_.minFilter = minFilter;
    }
    
    void setAddressModeU(VkSamplerAddressMode mode) {
        samplerInfo_.addressModeU = mode;
    }

    void setAddressModeV(VkSamplerAddressMode mode) {
        samplerInfo_.addressModeV = mode;
    }

    void setAddressModeW(VkSamplerAddressMode mode) {
        samplerInfo_.addressModeW = mode;
    }

    const VkUniquePtr<VkSampler> & sampler() const {
        return sampler_;
    }

private:
    void createSampler() {
        CHECK_VULKAN_RUNTIME_ERROR(
            vkCreateSampler(parentDevice(), &samplerInfo_, nullptr, sampler_.replace()),
            "failed to create depth sampler for offscreen!"
        );
    }
    
    VkSamplerCreateInfo samplerInfo_{};
    VkUniquePtr<VkSampler> sampler_{parentDevice(), vkDestroySampler};
};

#endif  // _VK_TEXTURE_H_
