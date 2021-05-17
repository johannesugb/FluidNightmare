#pragma once

#include <gvk.hpp>
#include <imgui.h>
#include <imgui_internal.h>

#include "preprocessor_defines.hpp"
#include "cpu_to_gpu_data_types.hpp"

// Main invokee of this application:
class fluid_nightmare_main : public gvk::invokee
{
public: // v== gvk::invokee overrides which will be invoked by the framework ==v
	fluid_nightmare_main(avk::queue& aQueue);

	void initialize() override;

	void update() override;

	void render() override;

	[[nodiscard]] const avk::top_level_acceleration_structure& get_tlas() const;

private: // v== Member variables ==v

	// --------------- Some fundamental stuff -----------------

	// Our only queue where we submit command buffers to:
	avk::queue* mQueue;

	// Our only descriptor cache which stores reusable descriptor sets:
	avk::descriptor_cache mDescriptorCache;

	// ------------- Scene and model properties ---------------

	// Ambient light of our scene:
	glm::vec3 mAmbientLight = { 0.5f, 0.5f, 0.5f };

	// The direction of our single light source, which is a directional light:
	glm::vec3 mLightDir = { 0.0f, -1.0f, 0.0f };

	// ----------- Resources required for ray tracing -----------

	// We are using one single top-level acceleration structure (TLAS) to keep things simple:
	//    (We're not duplicating the TLAS per frame in flight. Instead, we
	//     are using barriers to ensure correct rendering after some data 
	//     has changed in one or multiple of the acceleration structures.)
	avk::top_level_acceleration_structure mTlas;

	// We are rendering into one single target offscreen image (Otherwise we would need multiple
	// TLAS instances, too.) to keep things simple:
	avk::image_view mOffscreenImageView;
	// (After blitting this image into one of the window's backbuffers, the GPU can 
	//  possibly achieve some parallelization of work during presentation.)

	// The ray tracing pipeline that renders everything into the mOffscreenImageView:
	avk::ray_tracing_pipeline mPipeline;

	// ----------------- Further invokees --------------------

	// A camera to navigate our scene, which provides us with the view matrix:
	gvk::quake_camera mQuakeCam;

	// ------------------- UI settings -----------------------

	float mFieldOfViewForRayTracing = 45.0f;
	bool mEnableShadows = true;
	float mShadowsFactor = 0.5f;
	glm::vec3 mShadowsColor = glm::vec3{ 0.0f, 0.0f, 0.0f };
	bool mEnableAmbientOcclusion = true;
	float mAmbientOcclusionMinDist = 0.05f;
	float mAmbientOcclusionMaxDist = 0.25f;
	float mAmbientOcclusionFactor = 0.5f;
	glm::vec3 mAmbientOcclusionColor = glm::vec3{ 0.0f, 0.0f, 0.0f };

	// One boolean per geometry instance to tell if it shall be included in the
	// generation of the TLAS or not:
	std::vector<bool> mGeometryInstanceActive;

	bool mTlasUpdateRequired = false;

}; // End of fluid_nightmare_main
