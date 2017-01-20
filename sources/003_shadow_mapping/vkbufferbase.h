#ifdef _MSC_VER
#pragma once
#endif

#ifndef _VK_BUFFER_BASE_H_
#define _VK_BUFFER_BASE_H_

#include <stdexcept>

#include <vulkan/vulkan.h>

#include "vksmartptr.h"
#include "vkmemorybase.h"
#include "vkbaseapp.h"
#include "vkonetimecommand.h"

class VkBufferBase : public VkMemoryBase {
public:
    VkBufferBase(VkBaseApp *parent)
        : VkMemoryBase{parent} {
    }

protected:
    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkUniquePtr<VkBuffer>& buffer, VkUniquePtr<VkDeviceMemory>& bufferMemory) {
        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(parentDevice(), &bufferInfo, nullptr, buffer.replace()) != VK_SUCCESS) {
            throw std::runtime_error("failed to create buffer!");
        }

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(parentDevice(), buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(parentDevice(), &allocInfo, nullptr, bufferMemory.replace()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate buffer memory!");
        }

        vkBindBufferMemory(parentDevice(), buffer, bufferMemory, 0);
    }

    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
        oneTimeCommand().begin();

        VkBufferCopy copyRegion = {};
        copyRegion.size = size;
        vkCmdCopyBuffer(oneTimeCommand().command(), srcBuffer, dstBuffer, 1, &copyRegion);

        oneTimeCommand().end();
    }
};

#endif  // _VK_BUFFER_BASE_H_
