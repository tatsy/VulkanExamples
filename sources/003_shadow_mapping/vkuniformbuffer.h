#ifdef _MSC_VER
#pragma once
#endif

#ifndef _VK_UNIFORM_BUFFER_H_
#define _VK_UNIFORM_BUFFER_H_

#include "vkbufferbase.h"

template <class T>
class VkUniformBuffer : public VkBufferBase {
public:
    VkUniformBuffer(VkBaseApp *parent)
        : VkBufferBase{parent} {
    }

    void create() {
        VkDeviceSize bufferSize = sizeof(T);
        createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     stagingBuffer_, stagingBufferMemory_);
        createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, buffer_, bufferMemory_);
    }

    void update(const T &ubo) {
        void *data;
        vkMapMemory(parentDevice(), stagingBufferMemory_, 0, sizeof(ubo), 0, &data);
        std::memcpy(data, &ubo, sizeof(ubo));
        vkUnmapMemory(parentDevice(), stagingBufferMemory_);

        copyBuffer(stagingBuffer_, buffer_, sizeof(ubo));    
    }

    const VkUniquePtr<VkBuffer> & buffer() const {
        return buffer_;
    }

    VkDeviceSize size() const {
        return sizeof(T);
    }

private:
    VkUniquePtr<VkBuffer> buffer_{parentDevice(), vkDestroyBuffer};
    VkUniquePtr<VkDeviceMemory> bufferMemory_{parentDevice(), vkFreeMemory};
    VkUniquePtr<VkBuffer> stagingBuffer_{parentDevice(), vkDestroyBuffer};
    VkUniquePtr<VkDeviceMemory> stagingBufferMemory_{parentDevice(), vkFreeMemory};
};

#endif  // _VK_UNIFORM_BUFFER_H_
