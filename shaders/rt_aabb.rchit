#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : require

layout(set = 2, binding = 0) uniform accelerationStructureEXT topLevelAS;

// Ray payload to be sent back to the ray generation shader (Hence rayPayloadInEXT, not rayPayloadEXT):
layout(location = 0) rayPayloadInEXT vec3 hitValue;

// Receive barycentric coordinates from the geometry hit:
hitAttributeEXT vec3 hitAttribs;

void main()
{
	hitValue = vec3(
		((gl_InstanceID >> 16) & 0xFF) / 255.0,
		((gl_InstanceID >>  8) & 0xFF) / 255.0,
		((gl_InstanceID >>  0) & 0xFF) / 255.0
	);
}