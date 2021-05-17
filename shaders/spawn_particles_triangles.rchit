#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : require

// Push constants passed from the application:
layout(push_constant) uniform PushConstants {
	mat4  mSpawnTransformation;
    float mSpawnAngleRad;
    float mNewParticlesRadius;
} pushConstants;

// Ray payload to be sent back to the ray generation shader (Hence rayPayloadInEXT, not rayPayloadEXT):
layout(location = 0) rayPayloadInEXT vec3 newParticleCoords;

void main()
{
	const vec3 hitPos = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
	newParticleCoords = hitPos - normalize(gl_WorldRayDirectionEXT) * pushConstants.mNewParticlesRadius / sqrt(2.0);
}
