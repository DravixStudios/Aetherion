#pragma once
#include <iostream>

#include "Utils.h"
#include "Core/Containers.h"

#include "Core/Renderer/Device.h"

class Renderer {
public:
    static constexpr const char* CLASS_NAME = "Renderer";

    using Ptr = Ref<Renderer>;

    virtual ~Renderer() = default;
    virtual void Create(GLFWwindow* pWindow) = 0;

    virtual Ref<Device> CreateDevice();
};
