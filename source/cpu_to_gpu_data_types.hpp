#pragma once

#include <gvk.hpp>

// Data to be pushed to the GPU along with a specific draw call:
struct push_const_data {
    glm::mat4  mCameraTransform;
    float mCameraHalfFovAngle;
    float _padding1, _padding2, _padding3;
    glm::vec4  mLightDir;
};


