#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include <cstring>
#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>
#include <vector>
#include <set>
#include <unordered_map>
#include <array>
#include <functional>
#include <chrono>
#include <stdexcept>
#include <memory>

#include "common.h"
#include "vksmartptr.h"
#include "vkbaseapp.h"
#include "trimesh.h"
#include "vkutils.h"
#include "vkvbo.h"
#include "vkuniformbuffer.h"
#include "vktexture.h"
#include "vkonetimecommand.h"

static const std::string TEAPOT_FILE = std::string(DATA_DIRECTORY) + "teapot.obj";
static const std::string FLOOR_FILE = std::string(DATA_DIRECTORY) + "floor.obj";

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

// Uniform buffer object for draw (vertex shader).
struct UBOScene {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
    glm::mat4 normal;
    glm::mat4 depthBiasMVP;
    glm::vec3 lightPos;
};

// Uniform buffer object for offscreen (vertex shader).
struct UBOShadowMap {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

class MyVulkanApp : public VkBaseApp {
public:
    MyVulkanApp(bool isEnableValidationLayers)
        : VkBaseApp{isEnableValidationLayers} {
        initializeMembers();
    }
    
    virtual ~MyVulkanApp() {
    }

protected:
    void initializeVk() override {
        createRenderPass();
        createDescriptorSetLayout();

        createOffscreenImageStuff();
        createOffscreenRenderPass();
        createOffscreenFramebuffer();

        loadModel();
        createVbo();
        createGraphicsPipeline();
        setupVbo();

        createFramebuffers();
        createUniformBuffer();
        createDescriptorPool();
        createDescriptorSet();
        createCommandBuffers();
        createOffscreenCommandBuffer();

        createSemaphores();
    }

    void resizeVk(int width, int height) override {
        createRenderPass();
        createGraphicsPipeline();
        createFramebuffers();
        createCommandBuffers();
    }

    void paintVk() override {
        updateUniformBuffer();

        int swapChainIndex = startPaint(imageAvailableSemaphore);
        if (swapChainIndex < 0) {
            return;
        }

        // Offscreen submission
        {
            VkSubmitInfo submitInfo = {};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

            VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
            submitInfo.waitSemaphoreCount = 1;
            submitInfo.pWaitSemaphores = &imageAvailableSemaphore;
            submitInfo.pWaitDstStageMask = waitStages;

            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &offscreenCommandBuffer;

            submitInfo.signalSemaphoreCount = 1;
            submitInfo.pSignalSemaphores = &offscreenFinishedSemaphore;

            queueSubmit(submitInfo);
        }

        // Main draw submission
        {
            VkSubmitInfo submitInfo = {};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

            VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
            submitInfo.waitSemaphoreCount = 1;
            submitInfo.pWaitSemaphores = &offscreenFinishedSemaphore;
            submitInfo.pWaitDstStageMask = waitStages;

            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = getCommandBufferPtr(swapChainIndex);
            
            submitInfo.signalSemaphoreCount = 1;
            submitInfo.pSignalSemaphores = &renderFinishedSemaphore;

            queueSubmit(submitInfo);
        }

        endPaint(swapChainIndex, renderFinishedSemaphore);
    }

private:

    void initializeMembers() {
        descriptorSetLayout = {device(), vkDestroyDescriptorSetLayout};

        graphicsPipelines.object = { device(), vkDestroyPipeline };
        graphicsPipelines.floor = { device(), vkDestroyPipeline };
        graphicsPipelines.shadowMap = { device(), vkDestroyPipeline };

        renderPasses.scene = { device(), vkDestroyRenderPass };
        renderPasses.shadowMap = { device(), vkDestroyRenderPass };

        pipelineLayouts.scene = { device(), vkDestroyPipelineLayout };
        pipelineLayouts.shadowMap = { device(), vkDestroyPipelineLayout };

    }

    void createRenderPass() {
        VkAttachmentDescription colorAttachment = {};
        colorAttachment.format = imageFormat();
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentDescription depthAttachment = {};
        depthAttachment.format = findDepthFormat(physicalDevice());
        depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference colorAttachmentRef = {};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depthAttachmentRef = {};
        depthAttachmentRef.attachment = 1;
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;
        subpass.pDepthStencilAttachment = &depthAttachmentRef;

        VkSubpassDependency dependency = {};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};
        VkRenderPassCreateInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = attachments.size();
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        if (vkCreateRenderPass(device(), &renderPassInfo, nullptr, renderPasses.scene.replace()) != VK_SUCCESS) {
            throw std::runtime_error("failed to create render pass!");
        }
    }

    void createDescriptorSetLayout() {
        VkDescriptorSetLayoutBinding uboLayoutBinding = {};
        uboLayoutBinding.binding = 0;
        uboLayoutBinding.descriptorCount = 1;
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.pImmutableSamplers = nullptr;
        uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
        samplerLayoutBinding.binding = 1;
        samplerLayoutBinding.descriptorCount = 1;
        samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerLayoutBinding.pImmutableSamplers = nullptr;
        samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        std::array<VkDescriptorSetLayoutBinding, 2> bindings = {uboLayoutBinding, samplerLayoutBinding};
        VkDescriptorSetLayoutCreateInfo layoutInfo = {};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = bindings.size();
        layoutInfo.pBindings = bindings.data();

        if (vkCreateDescriptorSetLayout(device(), &layoutInfo, nullptr, descriptorSetLayout.replace()) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor set layout!");
        }
    }

    void createFramebuffers() {
        setupFramebuffers([&](VkFramebuffer* pFramebuffer, const VkImageView &colorImageView, const VkImageView &depthImageView) {
            std::array<VkImageView, 2> attachments = {
                colorImageView, depthImageView
            };

            VkFramebufferCreateInfo framebufferInfo = {};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = renderPasses.scene;
            framebufferInfo.attachmentCount = attachments.size();
            framebufferInfo.pAttachments = attachments.data();
            framebufferInfo.width = this->width();
            framebufferInfo.height = this->height();
            framebufferInfo.layers = 1;

            CHECK_VULKAN_RUNTIME_ERROR(
                vkCreateFramebuffer(device(), &framebufferInfo, nullptr, pFramebuffer),
                "failed to create framebuffer!"
            );
        });    
    }

#pragma endregion

#pragma region Preparation for offscreen

    void createOffscreenImageStuff() {
        // Create image.
        shadowMapFbo = std::make_unique<VkTexture>(this);
        shadowMapFbo->create(shadowMapSize, shadowMapSize,
                             VK_FORMAT_D32_SFLOAT, VK_IMAGE_TILING_OPTIMAL,
                             VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_DEPTH_BIT);
    }

    void createOffscreenRenderPass() {
        VkAttachmentDescription depthAttachment = {};
        depthAttachment.format = VK_FORMAT_D32_SFLOAT;
        depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

        VkAttachmentReference depthAttachmentRef = {};
        depthAttachmentRef.attachment = 0;
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 0;
        subpass.pDepthStencilAttachment = &depthAttachmentRef;

        std::array<VkSubpassDependency, 2> dependencies = {};
        
        dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[0].dstSubpass = 0;
        dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        dependencies[0].dstStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
        
        dependencies[1].srcSubpass = 0;
        dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[1].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        dependencies[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        VkRenderPassCreateInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &depthAttachment;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = dependencies.size();
        renderPassInfo.pDependencies = dependencies.data();

        if (vkCreateRenderPass(device(), &renderPassInfo, nullptr, renderPasses.shadowMap.replace()) != VK_SUCCESS) {
            throw std::runtime_error("failed to create render pass for offscreen!");
        }
    }

    void createOffscreenFramebuffer() {
        VkFramebufferCreateInfo frameInfo = {};
        frameInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        frameInfo.renderPass = renderPasses.shadowMap;
        frameInfo.attachmentCount = 1;
        frameInfo.pAttachments = &shadowMapFbo->imageView();
        frameInfo.width = shadowMapSize;
        frameInfo.height = shadowMapSize;
        frameInfo.layers = 1;

        if (vkCreateFramebuffer(device(), &frameInfo, nullptr, shadowMapFramebuffer.replace()) != VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer for offscreen!");
        }
    }

    void createOffscreenCommandBuffer() {
        VkCommandBufferAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = commandPool();
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;

        if (vkAllocateCommandBuffers(device(), &allocInfo, &offscreenCommandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate command buffers!");
        }

        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

        vkBeginCommandBuffer(offscreenCommandBuffer, &beginInfo);

        VkClearValue clearValues[1];
        clearValues[0].depthStencil.depth = 1.0f;

        VkRenderPassBeginInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPasses.shadowMap;
        renderPassInfo.framebuffer = shadowMapFramebuffer;
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = { shadowMapSize, shadowMapSize };
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = clearValues;

        VkViewport viewport;
        viewport.x = 0;
        viewport.y = 0;
        viewport.width = (float)shadowMapSize;
        viewport.height = (float)shadowMapSize;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(offscreenCommandBuffer, 0, 1, &viewport);
        
        VkRect2D scissor;
        scissor.offset = { 0, 0 };
        scissor.extent = { shadowMapSize, shadowMapSize }; 
        vkCmdSetScissor(offscreenCommandBuffer, 0, 1, &scissor);

        vkCmdSetDepthBias(offscreenCommandBuffer, depthBiasConstant, 0.0f, depthBiasSlope);

        vkCmdBeginRenderPass(offscreenCommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(offscreenCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipelines.shadowMap);
        vkCmdBindDescriptorSets(offscreenCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.shadowMap, 0, 1, &descriptorSets.shadowMap, 0, nullptr);

        // Draw teapot
        {
            teapotVbo->bindVertexBuffers(offscreenCommandBuffer);
            teapotVbo->bindIndexBuffer(offscreenCommandBuffer);

            vkCmdDrawIndexed(offscreenCommandBuffer, teapotMesh.indices.size(), 1, 0, 0, 0);
        }

        vkCmdEndRenderPass(offscreenCommandBuffer);

        vkEndCommandBuffer(offscreenCommandBuffer);
    }

#pragma endregion

    void createGraphicsPipeline() {
        // Shader
        auto vertShaderCode = readFile(std::string(SOURCE_DIRECTORY) + "shaders/vert.spv");
        auto fragShaderCode = readFile(std::string(SOURCE_DIRECTORY) + "shaders/frag.spv");

        VkUniquePtr<VkShaderModule> vertShaderModule{device(), vkDestroyShaderModule};
        VkUniquePtr<VkShaderModule> fragShaderModule{device(), vkDestroyShaderModule};
        createShaderModule(vertShaderCode, vertShaderModule);
        createShaderModule(fragShaderCode, fragShaderModule);

        VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = vertShaderModule;
        vertShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragShaderModule;
        fragShaderStageInfo.pName = "main";
            
        VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

        // Vertex buffer object
        VkPipelineVertexInputStateCreateInfo teapotVertexInputInfo = {};
        teapotVertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        auto bindingDescription = teapotVbo->getBindingDescription();
        auto attributeDescriptions = teapotVbo->getAttributeDescriptions();

        teapotVertexInputInfo.vertexBindingDescriptionCount = bindingDescription.size();
        teapotVertexInputInfo.vertexAttributeDescriptionCount = attributeDescriptions.size();
        teapotVertexInputInfo.pVertexBindingDescriptions = bindingDescription.data();
        teapotVertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

        VkPipelineVertexInputStateCreateInfo floorVertexInputInfo = {};
        floorVertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        auto bindingDescription2 = floorVbo->getBindingDescription();
        auto attributeDescriptions2 = floorVbo->getAttributeDescriptions();

        floorVertexInputInfo.vertexBindingDescriptionCount = bindingDescription2.size();
        floorVertexInputInfo.pVertexBindingDescriptions = bindingDescription2.data();
        floorVertexInputInfo.vertexAttributeDescriptionCount = attributeDescriptions2.size();
        floorVertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions2.data();

        // Assembly
        VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        // Other information
        VkViewport viewport = {};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float)this->width();
        viewport.height = (float)this->height();
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor = {};
        scissor.offset = { 0, 0 };
        scissor.extent = { (uint32_t)width(), (uint32_t)height() };
        
        VkPipelineViewportStateCreateInfo viewportState = {};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.pViewports = &viewport;
        viewportState.scissorCount = 1;
        viewportState.pScissors = &scissor;

        VkPipelineRasterizationStateCreateInfo rasterizer = {};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_NONE;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;

        VkPipelineMultisampleStateCreateInfo multisampling = {};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineDepthStencilStateCreateInfo depthStencil = {};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.stencilTestEnable = VK_FALSE;

        VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;

        VkPipelineColorBlendStateCreateInfo colorBlending = {};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f;
        colorBlending.blendConstants[1] = 0.0f;
        colorBlending.blendConstants[2] = 0.0f;
        colorBlending.blendConstants[3] = 0.0f;

        VkDescriptorSetLayout setLayouts[] = {descriptorSetLayout};
        VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = setLayouts;

        if (vkCreatePipelineLayout(device(), &pipelineLayoutInfo, nullptr, pipelineLayouts.scene.replace()) != VK_SUCCESS) {
            throw std::runtime_error("failed to create pipeline layout!");
        }

        if (vkCreatePipelineLayout(device(), &pipelineLayoutInfo, nullptr, pipelineLayouts.shadowMap.replace()) != VK_SUCCESS) {
            throw std::runtime_error("failed to create pipeline layout for offscreen!");
        }

        // Graphics pipeline for object.
        VkGraphicsPipelineCreateInfo pipelineInfo = {};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &teapotVertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.layout = pipelineLayouts.scene;
        pipelineInfo.renderPass = renderPasses.scene;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

        if (vkCreateGraphicsPipelines(device(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, graphicsPipelines.object.replace()) != VK_SUCCESS) {
            throw std::runtime_error("failed to create graphics pipeline!");
        }

        // Graphics pipeline for floor.
        pipelineInfo.pVertexInputState = &floorVertexInputInfo;
 
        if (vkCreateGraphicsPipelines(device(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, graphicsPipelines.floor.replace()) != VK_SUCCESS) {
            throw std::runtime_error("failed to create graphics pipeline!");
        }

        // Graphics pipeline for offscreen.
        auto vertShaderCode2 = readFile(std::string(SOURCE_DIRECTORY) + "shaders/smvert.spv");
        auto fragShaderCode2 = readFile(std::string(SOURCE_DIRECTORY) + "shaders/smfrag.spv");

        VkUniquePtr<VkShaderModule> vertShaderModule2{device(), vkDestroyShaderModule};
        VkUniquePtr<VkShaderModule> fragShaderModule2{device(), vkDestroyShaderModule};
        createShaderModule(vertShaderCode2, vertShaderModule2);
        createShaderModule(fragShaderCode2, fragShaderModule2);

        VkPipelineShaderStageCreateInfo vertShaderStageInfo2 = {};
        shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        shaderStages[0].module = vertShaderModule2;
        shaderStages[0].pName = "main";

        VkPipelineShaderStageCreateInfo fragShaderStageInfo2 = {};
        shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        shaderStages[1].module = fragShaderModule2;
        shaderStages[1].pName = "main";
        
        colorBlending.attachmentCount = 0;        
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
        rasterizer.depthBiasEnable = VK_TRUE;

        pipelineInfo.pVertexInputState = &teapotVertexInputInfo;
        pipelineInfo.layout = pipelineLayouts.shadowMap;
        pipelineInfo.renderPass = renderPasses.shadowMap;
        
        if (vkCreateGraphicsPipelines(device(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, graphicsPipelines.shadowMap.replace()) != VK_SUCCESS) {
            throw std::runtime_error("failed to create graphics pipeline for offscreen!");
        }
    }


    void copyImage(VkImage srcImage, VkImage dstImage, uint32_t width, uint32_t height) {
        VkOneTimeCommand oneTimeCommand(this);

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
            oneTimeCommand.command(),
            srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &region
        );

        oneTimeCommand.end();
    }

    void loadModel() {
        teapotMesh.load(TEAPOT_FILE);
        floorMesh.load(FLOOR_FILE);
    }

    void createVbo() {
        teapotVbo = std::make_unique<VkVbo>(this);
        floorVbo = std::make_unique<VkVbo>(this);
        
        teapotVbo->addVertexAttribute(teapotMesh.positions, 0, sizeof(float) * 3, VK_FORMAT_R32G32B32_SFLOAT);
        teapotVbo->addVertexAttribute(teapotMesh.normals, 1, sizeof(float) * 3, VK_FORMAT_R32G32B32_SFLOAT);
        teapotVbo->addIndices(teapotMesh.indices);

        floorVbo->addVertexAttribute(floorMesh.positions, 0, sizeof(float) * 3, VK_FORMAT_R32G32B32_SFLOAT);
        floorVbo->addVertexAttribute(floorMesh.normals, 1, sizeof(float) * 3, VK_FORMAT_R32G32B32_SFLOAT);
        floorVbo->addIndices(floorMesh.indices);
    }

    void setupVbo() {
        teapotVbo->setReady();
        floorVbo->setReady();
    }

    void createUniformBuffer() {
        uniformBuffers.scene = std::make_unique<VkUniformBuffer<UBOScene>>(this);
        uniformBuffers.scene->create();

        uniformBuffers.shadowMap = std::make_unique<VkUniformBuffer<UBOShadowMap>>(this);
        uniformBuffers.shadowMap->create();
    }

    void createDescriptorPool() {
        std::array<VkDescriptorPoolSize, 2> poolSizes = {};
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[0].descriptorCount = 2;
        poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[1].descriptorCount = 2;

        VkDescriptorPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = poolSizes.size();
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = 2;

        if (vkCreateDescriptorPool(device(), &poolInfo, nullptr, descriptorPool.replace()) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor pool!");
        }
    }

    void createDescriptorSet() {
        // Descriptor set allocation
        VkDescriptorSetLayout layouts[] = {descriptorSetLayout};
        VkDescriptorSetAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = layouts;

        if (vkAllocateDescriptorSets(device(), &allocInfo, &descriptorSets.scene) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate descriptor set!");
        }

        // Update descriptor set
        VkDescriptorBufferInfo bufferInfo = {};
        bufferInfo.buffer = uniformBuffers.scene->buffer();
        bufferInfo.offset = 0;
        bufferInfo.range = uniformBuffers.scene->size();

        VkDescriptorImageInfo imageInfo = {};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        imageInfo.imageView = shadowMapFbo->imageView();
        imageInfo.sampler = shadowMapFbo->sampler();

        std::array<VkWriteDescriptorSet, 2> descriptorWrites = {};

        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = descriptorSets.scene;
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfo;

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = descriptorSets.scene;
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(device(), descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);

        // Update descriptor set for offscreen
        if (vkAllocateDescriptorSets(device(), &allocInfo, &descriptorSets.shadowMap) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate descriptor set for offscreen!");
        }

        VkDescriptorBufferInfo offscreenUniformBufferInfo = {};
        offscreenUniformBufferInfo.buffer = uniformBuffers.shadowMap->buffer();
        offscreenUniformBufferInfo.offset = 0;
        offscreenUniformBufferInfo.range = uniformBuffers.shadowMap->size();

        std::array<VkWriteDescriptorSet, 1> offscreenDescriptorWrites = {};
        offscreenDescriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        offscreenDescriptorWrites[0].dstSet = descriptorSets.shadowMap;
        offscreenDescriptorWrites[0].dstBinding = 0;
        offscreenDescriptorWrites[0].dstArrayElement = 0;
        offscreenDescriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        offscreenDescriptorWrites[0].descriptorCount = 1;
        offscreenDescriptorWrites[0].pBufferInfo = &offscreenUniformBufferInfo;

        vkUpdateDescriptorSets(device(), offscreenDescriptorWrites.size(), offscreenDescriptorWrites.data(), 0, nullptr);
    }

    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkUniquePtr<VkBuffer>& buffer, VkUniquePtr<VkDeviceMemory>& bufferMemory) {
        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
            bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(device(), &bufferInfo, nullptr, buffer.replace()) != VK_SUCCESS) {
            throw std::runtime_error("failed to create buffer!");
        }

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(device(), buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(physicalDevice(), memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(device(), &allocInfo, nullptr, bufferMemory.replace()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate buffer memory!");
        }

        vkBindBufferMemory(device(), buffer, bufferMemory, 0);
    }


    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
        VkOneTimeCommand oneTimeCommand(this);

        VkBufferCopy copyRegion = {};
        copyRegion.size = size;
        vkCmdCopyBuffer(oneTimeCommand.command(), srcBuffer, dstBuffer, 1, &copyRegion);

        oneTimeCommand.end();
    }

    void createCommandBuffers() {
        setupRenderCommandBuffers([&](const VkCommandBuffer &commandBuffer, const VkFramebuffer &frameBuffer) {
            VkCommandBufferBeginInfo beginInfo = {};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

            vkBeginCommandBuffer(commandBuffer, &beginInfo);

            VkRenderPassBeginInfo renderPassInfo = {};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassInfo.renderPass = renderPasses.scene;
            renderPassInfo.framebuffer = frameBuffer;
            renderPassInfo.renderArea.offset = { 0, 0 };
            renderPassInfo.renderArea.extent = { (uint32_t)width(), (uint32_t)height() };
            
            std::array<VkClearValue, 2> clearValues = {};
            clearValues[0].color = {0.0f, 0.0f, 0.0f, 1.0f};
            clearValues[1].depthStencil = {1.0f, 0};

            renderPassInfo.clearValueCount = clearValues.size();
            renderPassInfo.pClearValues = clearValues.data();

            vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

            // Draw teapot
            {
                vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipelines.object);

                VkBuffer vertexBuffers[] = { teapotVbo->vertexBuffer() };
                VkDeviceSize offsets[] = { 0 };

                teapotVbo->bindVertexBuffers(commandBuffer);
                teapotVbo->bindIndexBuffer(commandBuffer);

                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.scene, 0, 1, &descriptorSets.scene, 0, nullptr);

                vkCmdDrawIndexed(commandBuffer, teapotMesh.indices.size(), 1, 0, 0, 0);
            }

            // Draw floor
            {
                vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipelines.floor);

                VkBuffer vertexBuffers[] = { floorVbo->vertexBuffer() };
                VkDeviceSize offsets[] = { 0 };

                floorVbo->bindVertexBuffers(commandBuffer);
                floorVbo->bindIndexBuffer(commandBuffer);

                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.scene, 0, 1, &descriptorSets.scene, 0, nullptr);

                vkCmdDrawIndexed(commandBuffer, floorMesh.indices.size(), 1, 0, 0, 0);
            }

            vkCmdEndRenderPass(commandBuffer);

            CHECK_VULKAN_RUNTIME_ERROR(
                vkEndCommandBuffer(commandBuffer),
                "failed to record command buffer!"
            );
        });

    }

    void createSemaphores() {
        VkSemaphoreCreateInfo semaphoreInfo = {};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        if (vkCreateSemaphore(device(), &semaphoreInfo, nullptr, imageAvailableSemaphore.replace()) != VK_SUCCESS ||
            vkCreateSemaphore(device(), &semaphoreInfo, nullptr, renderFinishedSemaphore.replace()) != VK_SUCCESS ||
            vkCreateSemaphore(device(), &semaphoreInfo, nullptr, offscreenFinishedSemaphore.replace()) != VK_SUCCESS) {

            throw std::runtime_error("failed to create semaphores!");
        }
    }

    void updateUniformBuffer() {
        static auto startTime = std::chrono::high_resolution_clock::now();
        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count() / 1000.0f;
        glm::mat4 rotMat = glm::rotate(glm::mat4(), time * glm::radians(30.0f), glm::vec3(0.0f, 1.0f, 0.0f));

        // Offscreen
        UBOShadowMap osUbo = {};
        osUbo.model = rotMat;
        osUbo.view = glm::lookAt(lightPos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        osUbo.proj = glm::perspective(glm::radians(60.0f), 1.0f, 0.1f, 100.0f);
        osUbo.proj[1][1] *= -1;

        uniformBuffers.shadowMap->update(osUbo);

        // Main draw
        UBOScene ubo = {};
        ubo.model = rotMat;
        ubo.view = glm::lookAt(glm::vec3(10.0f, 10.0f, 10.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        ubo.proj = glm::perspective(glm::radians(45.0f), (float)width() / (float)height(), 0.1f, 100.0f);
        ubo.proj[1][1] *= -1;
        ubo.normal = glm::transpose(glm::inverse(ubo.view * ubo.model));
        ubo.depthBiasMVP = osUbo.proj * osUbo.view * osUbo.model;
        ubo.lightPos = lightPos;

        uniformBuffers.scene->update(ubo);
    }

    void createShaderModule(const std::vector<char>& code, VkUniquePtr<VkShaderModule>& shaderModule) {
        VkShaderModuleCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = (uint32_t*) code.data();

        if (vkCreateShaderModule(device(), &createInfo, nullptr, shaderModule.replace()) != VK_SUCCESS) {
            throw std::runtime_error("failed to create shader module!");
        }
    }

    static std::vector<char> readFile(const std::string& filename) {
        std::ifstream file(filename.c_str(), std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            throw std::runtime_error("failed to open file!");
        }

        size_t fileSize = (size_t) file.tellg();
        std::vector<char> buffer(fileSize);

        file.seekg(0);
        file.read(buffer.data(), fileSize);

        file.close();

        return buffer;
    }

#pragma endregion

#pragma region private parameters
    VkUniquePtr<VkDescriptorSetLayout> descriptorSetLayout;

    struct GraphicsPipelines {
        VkUniquePtr<VkPipeline> object;
        VkUniquePtr<VkPipeline> floor;
        VkUniquePtr<VkPipeline> shadowMap;
    } graphicsPipelines;

    struct RenderPasses {
        VkUniquePtr<VkRenderPass> scene;
        VkUniquePtr<VkRenderPass> shadowMap;
    } renderPasses;

    struct PipelineLayouts {
        VkUniquePtr<VkPipelineLayout> scene;
        VkUniquePtr<VkPipelineLayout> shadowMap;
    } pipelineLayouts;

    struct DescriptorSets {
        VkDescriptorSet scene;
        VkDescriptorSet shadowMap;
    } descriptorSets;

    // Resources for offscreen
    VkCommandBuffer offscreenCommandBuffer;

    const uint32_t shadowMapSize = 1024;
    const float depthBiasConstant = 1.25f;
    const float depthBiasSlope = 1.75f;

    const glm::vec3 lightPos = { 0.0f, 25.0f, 0.0f };

    VkUniquePtr<VkFramebuffer> shadowMapFramebuffer{device(), vkDestroyFramebuffer};
    std::unique_ptr<VkTexture> shadowMapFbo = nullptr;
    
    // Meshes
    TriMesh teapotMesh;
    TriMesh floorMesh;

    std::unique_ptr<VkVbo> teapotVbo = nullptr;
    std::unique_ptr<VkVbo> floorVbo = nullptr;

    struct UniformBuffers {
        std::unique_ptr<VkUniformBuffer<UBOScene>> scene;
        std::unique_ptr<VkUniformBuffer<UBOShadowMap>> shadowMap;
    } uniformBuffers;

    VkUniquePtr<VkDescriptorPool> descriptorPool{device(), vkDestroyDescriptorPool};

    VkUniquePtr<VkSemaphore> imageAvailableSemaphore{device(), vkDestroySemaphore};
    VkUniquePtr<VkSemaphore> renderFinishedSemaphore{device(), vkDestroySemaphore};
    VkUniquePtr<VkSemaphore> offscreenFinishedSemaphore{device(), vkDestroySemaphore};

#pragma endregion

};

int main(int argc, char **argv) {
    static const int WIN_WIDTH = 800;
    static const int WIN_HEIGHT = 600;

    MyVulkanApp app(enableValidationLayers);
    app.createWindow(WIN_WIDTH, WIN_HEIGHT, "Vulkan: shadow mapping");

    try {
        app.run();
    } catch (const std::runtime_error &e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
