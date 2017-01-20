#ifdef _MSC_VER
#pragma once
#endif

#ifndef _VDELETER_H_
#define _VDELETER_H_

#include <functional>
#include <vulkan/vulkan.h>

template <typename T>
class VkUniquePtr {
public:
    VkUniquePtr() : VkUniquePtr([](T, VkAllocationCallbacks*) {}) {}

    VkUniquePtr(std::function<void(T, VkAllocationCallbacks*)> deletef) {
        this->deleter = [=](T obj) { deletef(obj, nullptr); };
    }

    VkUniquePtr(const VkUniquePtr<VkInstance>& instance, std::function<void(VkInstance, T, VkAllocationCallbacks*)> deletef) {
        this->deleter = [&instance, deletef](T obj) { deletef(instance, obj, nullptr); };
    }

    VkUniquePtr(const VkUniquePtr<VkDevice>& device, std::function<void(VkDevice, T, VkAllocationCallbacks*)> deletef) {
        this->deleter = [&device, deletef](T obj) { deletef(device, obj, nullptr); };
    }

    ~VkUniquePtr() {
        cleanup();
    }

    const T* operator &() const {
        return &object;
    }

    T* replace() {
        cleanup();
        return &object;
    }

    operator T() const {
        return object;
    }

    void operator=(T rhs) {
        if (rhs != object) {
            cleanup();
            object = rhs;
        }
    }

    template<typename V>
    bool operator==(V rhs) {
        return object == T(rhs);
    }

private:
    T object{VK_NULL_HANDLE};
    std::function<void(T)> deleter;

    void cleanup() {
        if (object != VK_NULL_HANDLE) {
            deleter(object);
        }
        object = VK_NULL_HANDLE;
    }
};

#endif  // _VDELETER_H_
