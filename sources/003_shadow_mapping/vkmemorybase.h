#ifdef _MSC_VER
#pragma once
#endif

#ifndef _VK_MEMORY_BASE_H_
#define _VK_MEMORY_BASE_H_

#include "vkutils.h"
#include "vkbaseapp.h"
#include "vkonetimecommand.h"

class VkMemoryBase {
public:
    VkMemoryBase(VkBaseApp *parent)
        : parent_{parent}
        , oneTimeCommand_{parent} {
    }

    virtual ~VkMemoryBase() {
    }

    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(parent_->physicalDevice_, &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }
        std::abort();

        throw std::runtime_error("failed to find suitable memory type!");
    }

    const VkUniquePtr<VkDevice> & parentDevice() const {
        return parent_->device_;
    }

    VkOneTimeCommand & oneTimeCommand() {
        return oneTimeCommand_;
    }

private:
    VkBaseApp *parent_ = nullptr;
    VkOneTimeCommand oneTimeCommand_;
};

#endif  // _VK_MEMORY_BASE_H_
