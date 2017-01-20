#ifdef _MSC_VER
#pragma once
#endif

#ifndef _VK_ONE_TIME_COMMAND_H_
#define _VK_ONE_TIME_COMMAND_H_

#include "vkutils.h"
#include "vkbaseapp.h"

class VkOneTimeCommand {
public:
    VkOneTimeCommand(VkBaseApp *parent)
        : parent_{parent} {
    }

    virtual ~VkOneTimeCommand() {
        if (commandBuffer_ != VK_NULL_HANDLE) {
            end();
        }
    }

    void begin() {
        VkCommandBufferAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = parent_->commandPool_;
        allocInfo.commandBufferCount = 1;

        CHECK_VULKAN_RUNTIME_ERROR(
            vkAllocateCommandBuffers(parent_->device_, &allocInfo, &commandBuffer_),
            "failed to allocate command buffers!"
        );

        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        CHECK_VULKAN_RUNTIME_ERROR(
            vkBeginCommandBuffer(commandBuffer_, &beginInfo),
            "failed to begin command buffer"
        );
    }

    void end() {
        vkEndCommandBuffer(commandBuffer_);

        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer_;

        vkQueueSubmit(parent_->graphicsQueue_, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(parent_->graphicsQueue_);

        vkFreeCommandBuffers(parent_->device_, parent_->commandPool_, 1, &commandBuffer_);

        commandBuffer_ = VK_NULL_HANDLE;
    }

    inline const VkCommandBuffer & command() {
        return commandBuffer_;
    }

private:
    VkBaseApp *parent_ = nullptr;
    VkCommandBuffer commandBuffer_ = VK_NULL_HANDLE;
};

#endif  // _VK_ONE_TIME_COMMAND_H_
