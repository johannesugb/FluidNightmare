#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : require

layout(set = 0, binding = 0) uniform sampler2D textures[];

struct MaterialGpuData
{
	vec4 mDiffuseReflectivity;
	vec4 mAmbientReflectivity;
	vec4 mSpecularReflectivity;
	vec4 mEmissiveColor;
	vec4 mTransparentColor;
	vec4 mReflectiveColor;
	vec4 mAlbedo;

	float mOpacity;
	float mBumpScaling;
	float mShininess;
	float mShininessStrength;
	
	float mRefractionIndex;
	float mReflectivity;
	float mMetallic;
	float mSmoothness;
	
	float mSheen;
	float mThickness;
	float mRoughness;
	float mAnisotropy;
	
	vec4 mAnisotropyRotation;
	vec4 mCustomData;
	
	int mDiffuseTexIndex;
	int mSpecularTexIndex;
	int mAmbientTexIndex;
	int mEmissiveTexIndex;
	int mHeightTexIndex;
	int mNormalsTexIndex;
	int mShininessTexIndex;
	int mOpacityTexIndex;
	int mDisplacementTexIndex;
	int mReflectionTexIndex;
	int mLightmapTexIndex;
	int mExtraTexIndex;
	
	vec4 mDiffuseTexOffsetTiling;
	vec4 mSpecularTexOffsetTiling;
	vec4 mAmbientTexOffsetTiling;
	vec4 mEmissiveTexOffsetTiling;
	vec4 mHeightTexOffsetTiling;
	vec4 mNormalsTexOffsetTiling;
	vec4 mShininessTexOffsetTiling;
	vec4 mOpacityTexOffsetTiling;
	vec4 mDisplacementTexOffsetTiling;
	vec4 mReflectionTexOffsetTiling;
	vec4 mLightmapTexOffsetTiling;
	vec4 mExtraTexOffsetTiling;
};

layout(set = 0, binding = 1) buffer Material 
{
	MaterialGpuData materials[];
} matSsbo;

layout(set = 0, binding = 2) uniform usamplerBuffer indexBuffers[];
layout(set = 0, binding = 3) uniform samplerBuffer texCoordsBuffers[];
layout(set = 0, binding = 4) uniform samplerBuffer normalsBuffers[];

layout(set = 2, binding = 0) uniform accelerationStructureEXT topLevelAS;

layout(location = 0) rayPayloadInEXT vec3 hitValue;

// Receive barycentric coordinates from the geometry hit:
hitAttributeEXT vec3 hitAttribs;

layout(location = 2) rayPayloadEXT float secondaryRayHitValue;

layout(push_constant) uniform PushConstants {
	mat4  mCameraTransform;
    float mCameraHalfFovAngle;
    float _padding1, _padding2, _padding3;
    vec4  mLightDir;
} pushConstants;

vec4 sample_from_diffuse_texture(int matIndex, vec2 uv)
{
	int texIndex = matSsbo.materials[matIndex].mDiffuseTexIndex;
	vec4 offsetTiling = matSsbo.materials[matIndex].mDiffuseTexOffsetTiling;
	vec2 texCoords = uv * offsetTiling.zw + offsetTiling.xy;
	return textureLod(textures[texIndex], texCoords, 0.0);
}

void main()
{
	// Compute normalized barycentric coordinates from the triangle hit:
    const vec3 bary = vec3(1.0 - hitAttribs.x - hitAttribs.y, hitAttribs.x, hitAttribs.y);

	// Read the custom index that we have stored in struct geometry_instance::mInstanceCustomIndex:
	// We can use it as index into the materials and into the buffer views.
	const int customIndex = nonuniformEXT(gl_InstanceCustomIndexEXT);

	// Read the triangle indices from the index buffer:
	const ivec3 indices = ivec3(texelFetch(indexBuffers[customIndex], gl_PrimitiveID).rgb);

	// Use barycentric coordinates to compute the interpolated uv coordinates:
//	const vec2 uv     = bc.x * texelFetch(texCoordsBuffers[customIndex], indices.x).rg
//	                  + bc.y * texelFetch(texCoordsBuffers[customIndex], indices.y).rg 
//				      + bc.z * texelFetch(texCoordsBuffers[customIndex], indices.z).rg;
	const vec2 uv0 = texelFetch(texCoordsBuffers[customIndex], indices.x).st;
	const vec2 uv1 = texelFetch(texCoordsBuffers[customIndex], indices.y).st;
	const vec2 uv2 = texelFetch(texCoordsBuffers[customIndex], indices.z).st;
	const vec2 uv = (bary.x * uv0 + bary.y * uv1 + bary.z * uv2);


	// Use barycentric coordinates to compute the interpolated normals
	const vec3 normal = normalize(
	                        bary.x * texelFetch(normalsBuffers[customIndex], indices.x).rgb
	                      + bary.y * texelFetch(normalsBuffers[customIndex], indices.y).rgb 
	                      + bary.z * texelFetch(normalsBuffers[customIndex], indices.z).rgb
					    );

	// Sample color from diffuse texture
	vec3 diffuseTexColor = sample_from_diffuse_texture(customIndex, uv).rgb;

	// Compute diffuse lighting towards light source:
	float nDotL = dot(normal, normalize(pushConstants.mLightDir.xyz));

	// Set diffusely illuminated result as the hitValue:
	hitValue = diffuseTexColor * max(0.0, nDotL);

    vec3 origin = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
    vec3 direction = normalize(pushConstants.mLightDir.xyz);
    uint rayFlags = gl_RayFlagsOpaqueEXT | gl_RayFlagsTerminateOnFirstHitEXT;
    uint cullMask = 0xff;
    float tmin = 0.001;
    float tmax = 100.0;


//    traceRayEXT(topLevelAS, rayFlags, cullMask, 1 /* sbtRecordOffset */, 0 /* sbtRecordStride */, 1 /* missIndex */, origin, tmin, direction, tmax, 2 /*payload location*/);

//	 gl_InstanceCustomIndex = VkGeometryInstance::instanceId
//    hitValue = (barycentrics * 0.5 + vec3(0.5, 0.5, 0.5))
//		* uniformBuffers[nonuniformEXT(gl_InstanceCustomIndexEXT)].color.rgb
//		* ( secondaryRayHitValue < tmax ? 0.25 : 1.0 );
//	hitValue = barycentrics * 0.5 + vec3(0.5, 0.5, 0.5);
//	hitValue = vec3(clamp(uv,0,1), 0) * (secondaryRayHitValue < tmax ? 0.25 : 1.0);
}