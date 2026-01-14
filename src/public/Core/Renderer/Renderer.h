#pragma once
#include <iostream>

#include "Utils.h"
#include "Core/Containers.h"

class Renderer {
public:
    ~Renderer() = default;
    virtual void Create() = 0;
};
