#pragma once

#include <gvk.hpp>

// Data to be pushed to the GPU along with a specific draw call:
struct push_const_data_scene_rendering {
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

// Data to be pushed to the GPU along with a ray tracing pipeline invocation
// for the purpose of spawning further particles:
struct push_const_data_particle_spawner {
	// Represents both, offset and rotation for the spawn origin and direction:
	glm::mat4  mSpawnTransformation;
	// The spawning angle in radians:
	float      mSpawnAngleRad;
	// The new particle's radius:
	float      mNewParticlesRadius;
};
