#ifdef _MSC_VER
#pragma once
#endif

#ifndef _VK_VBO_H_
#define _VK_VBO_H_

#include <cstdio>

#include <vulkan/vulkan.h>

#include "vkbufferbase.h"

struct VertexAttribInfo {
    VertexAttribInfo()
        : location{0u}
        , offset{0u}
        , stride{0u}
        , format{} {
    }

    VertexAttribInfo(uint32_t l, uint32_t o, uint32_t s, VkFormat f)
        : location{l}
        , offset{o}
        , stride{s}
        , format{f} {
    }

    uint32_t location;
    VkDeviceSize offset;
    uint32_t stride;
    VkFormat format;
};

class VkVbo : public VkBufferBase {
public:
    VkVbo(VkBaseApp *parent)
        : VkBufferBase{parent} {
    }

    void addVertexAttribute(const std::vector<float> &data, uint32_t location, uint32_t stride, VkFormat format) {
        if (data.empty()) {
            fprintf(stderr, "[WARNING] input data is empty!");
            return;
        }

        const uint32_t offset = vertexData_.size() * sizeof(float);
        vertexData_.insert(vertexData_.end(), data.begin(), data.end());
        attribInfo_.emplace_back(location, offset, stride, format);
    }

    void addIndices(const std::vector<uint32_t> &indices) {
        indices_.insert(indices_.end(), indices.begin(), indices.end());
    }

    void setReady() {
        createVertexBuffer();
        createIndexBuffer();
    }

    std::vector<VkVertexInputBindingDescription> getBindingDescription() {
        std::vector<VkVertexInputBindingDescription> bindDesc = {};

        int binding = 0;
        for (const auto &attrib : attribInfo_) {
            VkVertexInputBindingDescription desc = {};
            desc.binding = binding++;
            desc.stride = attrib.stride;
            desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
            bindDesc.push_back(desc);
        }

        return std::move(bindDesc);
    }

    std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions() const {
        std::vector<VkVertexInputAttributeDescription> attribDesc = {};

        int binding = 0;
        for (const auto &attrib : attribInfo_) {
            VkVertexInputAttributeDescription desc = {};
            desc.binding = binding++;
            desc.location = attrib.location;
            desc.format = attrib.format;
            desc.offset = 0;
            attribDesc.push_back(desc);
        }
        return std::move(attribDesc);
    }

    void bindVertexBuffers(VkCommandBuffer commandBuffer) const {
        int binding = 0;
        for (const auto &attrib : attribInfo_) {
            vkCmdBindVertexBuffers(commandBuffer, binding++, 1, &vertexBuffer_, &attrib.offset);
        }
    }

    void bindIndexBuffer(VkCommandBuffer commandBuffer) const {
         vkCmdBindIndexBuffer(commandBuffer, indexBuffer_, 0, VK_INDEX_TYPE_UINT32);
    }

    inline VkBuffer vertexBuffer() const {
        return vertexBuffer_;
    }

    inline VkBuffer indexBuffer() const {
        return indexBuffer_;
    }

private:
    void createVertexBuffer() {
        VkDeviceSize bufferSize = vertexData_.size() * sizeof(vertexData_[0]);
        
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

        void* data;
        vkMapMemory(parentDevice(), stagingBufferMemory, 0, bufferSize, 0, &data);
            memcpy(data, vertexData_.data(), (size_t) bufferSize);
        vkUnmapMemory(parentDevice(), stagingBufferMemory);

        createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer_, vertexBufferMemory_);

        copyBuffer(stagingBuffer, vertexBuffer_, bufferSize);
    }

    void createIndexBuffer() {
        VkDeviceSize bufferSize = indices_.size() * sizeof(indices_[0]);
        
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

        void* data;
        vkMapMemory(parentDevice(), stagingBufferMemory, 0, bufferSize, 0, &data);
            memcpy(data, indices_.data(), (size_t) bufferSize);
        vkUnmapMemory(parentDevice(), stagingBufferMemory);

        createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer_, indexBufferMemory_);

        copyBuffer(stagingBuffer, indexBuffer_, bufferSize);
    }

private:
    std::vector<float> vertexData_;
    std::vector<uint32_t> indices_;
    std::vector<VertexAttribInfo> attribInfo_;

    VkBuffer vertexBuffer_;
    VkDeviceMemory vertexBufferMemory_;
    VkBuffer indexBuffer_;
    VkDeviceMemory indexBufferMemory_;
};

#endif  // _VK_VBO_H_
