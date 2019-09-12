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

    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer &buffer, VkDeviceMemory &bufferMemory) {
        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(parentDevice(), &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to create buffer!");
        }

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(parentDevice(), buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(parentDevice(), &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate buffer memory!");
        }

        vkBindBufferMemory(parentDevice(), buffer, bufferMemory, 0);
    }

    const VkDevice & parentDevice() const {
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
