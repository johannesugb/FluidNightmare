#pragma once

#include <gvk.hpp>

// Data to be pushed to the GPU along with a specific draw call:
struct push_const_data {
	glm::vec4  mAmbientLight;
	glm::vec4  mLightDir;
	glm::mat4  mCameraTransform;
	float mCameraHalfFovAngle;
	float _padding;
	vk::Bool32  mEnableShadows;
	float mShadowsFactor;
	glm::vec4  mShadowsColor;
	vk::Bool32  mEnableAmbientOcclusion;
	float mAmbientOcclusionMinDist;
	float mAmbientOcclusionMaxDist;
	float mAmbientOcclusionFactor;
	glm::vec4  mAmbientOcclusionColor;
};


