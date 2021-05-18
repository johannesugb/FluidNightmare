#pragma once

#include <gvk.hpp>
#include <imgui.h>
#include <imgui_internal.h>

#include "preprocessor_defines.hpp"
#include "cpu_to_gpu_data_types.hpp"
#include "fluid_nightmare_main.hpp"

// An invokee that handles triangle mesh geometry:
class procedural_geometry_manager : public gvk::invokee
{
public: // v== gvk::invokee overrides which will be invoked by the framework ==v
	procedural_geometry_manager(avk::queue& aQueue)
		: invokee{ -10 } // This invokee must execute BEFORE the main invokee
		, mQueue{ &aQueue }
	{}

	void initialize() override
	{
		// Create a descriptor cache that helps us to conveniently create descriptor sets,
		// which describe where shaders can find resources like buffers or images:
		mDescriptorCache = gvk::context().create_descriptor_cache();
		
		// For the BLAS, one single AABB is sufficient. Build it:
		mBlas = gvk::context().create_bottom_level_acceleration_structure({ avk::acceleration_structure_size_requirements::from_aabbs(1u) }, false);
		mBlas->build({ VkAabbPositionsKHR{ /* min: */ -1.f, -1.f, -1.f,  /* max: */ 1.f,  1.f,  1.f } });

		// Create a buffer to hold a number of spawned particle candidiates, each one represented just by their position:
		mSpawnedParticlesBuffer = gvk::context().create_buffer(
			avk::memory_usage::host_coherent, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc,
			avk::storage_buffer_meta::create_from_size(cNewParticleCandidatesToSpawn * sizeof(glm::vec4))
		);
		
		// Create our ray tracing pipeline which spawns particles:
		mPipeline = gvk::context().create_ray_tracing_pipeline_for(
			avk::define_shader_table(
				avk::ray_generation_shader("shaders/particle_spawner/spawn_particles.rgen"),
				avk::triangles_hit_group::create_with_rchit_only("shaders/particle_spawner/spawn_particles_triangles.rchit"),
				avk::procedural_hit_group::create_with_rint_and_rchit("shaders/rt_aabb.rint", "shaders/particle_spawner/spawn_particles_procedural.rchit"),
				avk::miss_shader("shaders/empty_miss_shader.rmiss")
			),
			// We won't need the maximum recursion depth, but why not:
			gvk::context().get_max_ray_tracing_recursion_depth(),
			// Define push constants and descriptor bindings:
			avk::push_constant_binding_data{ avk::shader_type::ray_generation | avk::shader_type::closest_hit, 0, sizeof(push_const_data_particle_spawner) },
			avk::descriptor_binding<avk::top_level_acceleration_structure>(0, 0, 1),
			avk::descriptor_binding(0, 1, mSpawnedParticlesBuffer->as_storage_buffer())
		);

#if ENABLE_SHADER_HOT_RELOADING_FOR_RAY_TRACING_PIPELINE
		// Create an updater:
		mUpdater.emplace();
		mPipeline.enable_shared_ownership(); // The updater needs to hold a reference to it, so we need to enable shared ownership.
		mUpdater->on(gvk::shader_files_changed_event(mPipeline))
			.update(mPipeline);
#endif
		
		// Add an "ImGui Manager" which handles the UI specific to the requirements of this invokee:
		auto imguiManager = gvk::current_composition()->element_by_type<gvk::imgui_manager>();
		if (nullptr != imguiManager) {
			imguiManager->add_callback([this]() {
				ImGui::Begin("Procedural Geometry");
				ImGui::SetWindowPos(ImVec2(422, 2.0f), ImGuiCond_FirstUseEver);
				ImGui::SetWindowSize(ImVec2(402.0f, 224.0f), ImGuiCond_FirstUseEver);

				ImGui::Separator();
				ImGui::Text("Spawn Settings:");
				ImGui::DragFloat3("Spawn Origin", glm::value_ptr(mSpawnOrigin), 0.1f);
				ImGui::DragFloat3("Spawn Direction", glm::value_ptr(mSpawnDirection), 0.1f);
				ImGui::SliderFloat("Spawn Cone Angle (Degrees)", &mSpawnAngle, 10.0f, 80.0f);
				ImGui::Checkbox("Add Random Offset", &mRandomlyOffsetDirecion);
				ImGui::SliderFloat("Radius of newly spawned particle", &mRadiusOfNewWaterParticles, 0.0001f, 1.0f);

				ImGui::Separator();
				ImVec4 particlesStatusTextColor(0.0f, 0.9f, 0.3f, 1.0f);
				if (mGeometryInstances.size() >= cMaxNumParticles) {
					mCurrentlySpawningWaterParticles = false; // Can't spawn any more
					ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true); // Disable the following checkbox
					particlesStatusTextColor = ImVec4(0.9f, 0.3f, 0.0f, 1.0f);
				}
				ImGui::Checkbox("SPAWN NEW WATER PARTICLES!", &mCurrentlySpawningWaterParticles);
				if (mGeometryInstances.size() >= cMaxNumParticles) {
					ImGui::PopItemFlag();
				}
				auto spawnStatus = fmt::format("{} particles spawned so far.", mGeometryInstances.size());
				ImGui::TextColored(particlesStatusTextColor, spawnStatus.c_str());

				ImGui::End();
			});
		}
	}
	
	// Returns true if a TLAS that uses the geometry of this invokee must be updated because the geometry has changed,
	// which in this case always means: more particles have been added.
	[[nodiscard]] bool has_updated_geometry_for_tlas() const
	{
		return mTlasUpdateRequired;
	}

	void reset_update_required_flag()
	{
		mTlasUpdateRequired = false;
	}

	// Return the geometry instances to the caller, who will use it for a TLAS build:
	[[nodiscard]] const auto& get_geometry_instances_buffer()
	{
		return mGeometryInstances;
	}

	// Invoked by the framework every frame:
	void update() override
	{
		// Tidy up some of the values:
		mSpawnDirection = glm::normalize(mSpawnDirection);
		if (glm::any(glm::isnan(mSpawnDirection))) {
			mSpawnDirection = glm::vec3{ 0.0f, -1.0f, 0.0f };
		}
		mSpawnAngleRad = glm::radians(mSpawnAngle);

		if (mCurrentlySpawningWaterParticles && mGeometryInstances.size() <= cMaxNumParticles) {

			// Okay, here's what we're going to do:
			//  1) We let the GPU trace several rays
			//  2) We read back the result
			//  3) We select ONE particle position and add that to our instances
			
			auto& commandPool = gvk::context().get_command_pool_for_single_use_command_buffers(*mQueue);
			auto cmdbfr = commandPool->alloc_command_buffer(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
			cmdbfr->begin_recording();

			// We're using only one TLAS for all frames in flight. Therefore, we need to set up a barrier
			// affecting the whole queue which waits until all previous ray tracing work has completed:
			cmdbfr->establish_execution_barrier(
				avk::pipeline_stage::ray_tracing_shaders, /* -> */ avk::pipeline_stage::acceleration_structure_build
			);

			auto* mainInvokee = gvk::current_composition()->element_by_type<fluid_nightmare_main>();
			assert(nullptr != mainInvokee);

			cmdbfr->bind_pipeline(avk::const_referenced(mPipeline));
			cmdbfr->bind_descriptors(mPipeline->layout(), mDescriptorCache.get_or_create_descriptor_sets({
				avk::descriptor_binding(0, 0, mainInvokee->get_tlas()),
				avk::descriptor_binding(0, 1, mSpawnedParticlesBuffer->as_storage_buffer())
			}));

			// Set the push constants:
			auto pushConstantsForThisDrawCall = push_const_data_particle_spawner{
				gvk::matrix_from_transforms(
					glm::vec3{ mSpawnOrigin },                                 // Location of our spawning point
					// Build a from-to-rotation quaternion:
					glm::quat(glm::vec3{0.0f, -1.0f, 0.0f}, mSpawnDirection),  // How our spawning direction will be rotated => Create rotation relative to our default -y direction!
					glm::vec3{1.0f}                                            // Scale doesn't matter
				),
				mSpawnAngleRad,
				mRadiusOfNewWaterParticles
			};
			cmdbfr->handle().pushConstants(mPipeline->layout_handle(), vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR, 0, sizeof(pushConstantsForThisDrawCall), &pushConstantsForThisDrawCall);

			// Do it:
			cmdbfr->trace_rays(
				vk::Extent3D{ cNewParticleCandidatesToSpawn, 1u, 1u },
				mPipeline->shader_binding_table(),
				avk::using_raygen_group_at_index(0),
				avk::using_miss_group_at_index(0),
				avk::using_hit_group_at_index(0)
			);

			// We don't add a barrier here. We'll just wait for completion via the fence.

			cmdbfr->end_recording();
			auto fen = mQueue->submit_with_fence(avk::referenced(cmdbfr));
			fen->wait_until_signalled();

			// Read back the data into an array:
			auto candidates = mSpawnedParticlesBuffer->read<std::array<glm::vec4, cNewParticleCandidatesToSpawn>>(0, avk::sync::wait_idle());
			// Select the "best" of the candidates. We'll just go for the candidate with minimal y coordinates:
			glm::vec4 selectedCandidate = candidates[0];
			for (uint32_t i = 1u; i < cNewParticleCandidatesToSpawn; ++i) {
				if (candidates[i].y < selectedCandidate.y) {
					selectedCandidate = candidates[i];
				}
			}
			
			mGeometryInstances.push_back(
				gvk::context().create_geometry_instance(mBlas) // Refer to the concrete BLAS; it is the same for each water particle
					// Handle water particles instance offset of 1; i.e. based on that, the
					// right (procedural) shaders will be chosen from the shader binding table:
					.set_instance_offset(1)
					// Set this instance's transformation matrix (offset by the selected candidate's position, do not rotate, scale according to the current setting):
				.set_transform_column_major(gvk::to_array(gvk::matrix_from_transforms(glm::vec3{ selectedCandidate }, glm::quat(), glm::vec3{ mRadiusOfNewWaterParticles })))
			);

			mTlasUpdateRequired = true;
		}
		else {
			std::vector<glm::vec4> candidates(cNewParticleCandidatesToSpawn, glm::vec4{ 0.0f });
			mSpawnedParticlesBuffer->read(candidates.data(), 0, avk::sync::wait_idle());

			// Select the "best" of the candidates. We'll just go for the candidate with minimal y coordinates:
			glm::vec4 selectedCandidate = candidates[0];
			for (uint32_t i = 1u; i < cNewParticleCandidatesToSpawn; ++i) {
				if (candidates[i].y < selectedCandidate.y) {
					selectedCandidate = candidates[i];
				}
			}
		}
	}

	// Some getters that will be used by the main invokee:
	[[nodiscard]] constexpr uint32_t max_number_of_geometry_instances() const { return cMaxNumParticles; }
	
private: // v== Member variables ==v

	// --------------- Some fundamental stuff -----------------

	// Our only queue where we submit command buffers to:
	avk::queue* mQueue;

	// Our only descriptor cache which stores reusable descriptor sets:
	avk::descriptor_cache mDescriptorCache;

	// How many new particle candidates shall be spawned at a time
	const static uint32_t cNewParticleCandidatesToSpawn = 16u * 16u;
	
	// A buffer that will contain potential positions of new particles
	avk::buffer mSpawnedParticlesBuffer;
	
	// ------------------- Constants/Settings ----------------------

	const static uint32_t cMaxNumParticles = 524288; // 500k water particles max. The buffer will be sized according to this value.
	
	// ---------------- Acceleration Structures --------------------

	// The ray tracing pipeline that spawns new particles:
	avk::ray_tracing_pipeline mPipeline;
	
	// ---------------- Acceleration Structures --------------------

	// A BLAS which represents one single water particle. All other particles are instanced:
	avk::bottom_level_acceleration_structure mBlas;

	// A buffer which contains all a geometry instance for every single water particle:
	std::vector<avk::geometry_instance> mGeometryInstances;

	// ------------------- UI settings -----------------------

	// The origin where from spawning rays are sent out (in world space):
	glm::vec3 mSpawnOrigin = glm::vec3(0.0f, 20.0f, 0.0f);

	// The orientation of the spawning frustum (in world space):
	glm::vec3 mSpawnDirection = glm::vec3(0.0f, -1.0f, 0.0f);

	// The possible max.  deviation from mSpawnDirection for the spawning rays:
	float mSpawnAngle = 45.0f;
	float mSpawnAngleRad;

	// If set to true, a random offset will be added to the spawn direction 
	bool mRandomlyOffsetDirecion = true;
	
	// The water particle's (uniform) scale:
	float mRadiusOfNewWaterParticles = 0.35f;

	// True if water particles are currently being spawned:
	bool mCurrentlySpawningWaterParticles = false;

	// True when an TLAS update is immanent:
	bool mTlasUpdateRequired = true;

	// How often a particle has been spawned. I.e. this number should
	// represent the total number of water particles in the scene:
	int mNumberOfSpawnInvocations = 0;

}; // End of procedural_geometry_manager
