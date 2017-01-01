attribute vec2 attr_TexCoord0;

#if defined(USE_TESSELLATION) || defined(USE_ICR_CULLING)
out vec3 Normal_CS_in;
out vec2 TexCoord_CS_in;
out vec4 WorldPos_CS_in;
out vec3 ViewDir_CS_in;
out vec4 Tangent_CS_in;
out vec4 Bitangent_CS_in;
out vec4 Color_CS_in;
out vec4 PrimaryLightDir_CS_in;
out vec2 TexCoord2_CS_in;
out vec3 Blending_CS_in;
out float Slope_CS_in;
out float usingSteepMap_CS_in;
#endif

#if defined(USE_LIGHTMAP) || defined(USE_TCGEN)
attribute vec2 attr_TexCoord1;
#endif
attribute vec4 attr_Color;

attribute vec3 attr_Position;
attribute vec3 attr_Normal;
attribute vec4 attr_Tangent;

#if defined(USE_VERTEX_ANIMATION)
attribute vec3 attr_Position2;
attribute vec3 attr_Normal2;
attribute vec4 attr_Tangent2;
#elif defined(USE_SKELETAL_ANIMATION)
attribute vec4 attr_BoneIndexes;
attribute vec4 attr_BoneWeights;
#endif

uniform vec4	u_Local1; // parallaxScale, haveSpecular, specularScale, materialType
uniform vec4	u_Local2; // ExtinctionCoefficient
uniform vec4	u_Local3; // RimScalar, MaterialThickness, subSpecPower, cubemapScale
uniform vec4	u_Local4; // haveNormalMap, isMetalic, hasRealSubsurfaceMap, sway
uniform vec4	u_Local5; // hasRealOverlayMap, overlaySway, blinnPhong, hasSteepMap
uniform vec4	u_Local6; // useSunLightSpecular
uniform vec4	u_Local7; // hasSplatMap1, hasSplatMap2, hasSplatMap3, hasSplatMap4
uniform vec4	u_Local9;

uniform sampler2D			u_DiffuseMap;

uniform float	u_Time;

#if defined(USE_DELUXEMAP)
uniform vec4   u_EnableTextures; // x = normal, y = deluxe, z = specular, w = cube
#endif

uniform vec3   u_ViewOrigin;

#if defined(USE_TCGEN)
uniform int    u_TCGen0;
uniform vec3   u_TCGen0Vector0;
uniform vec3   u_TCGen0Vector1;
uniform vec3   u_LocalViewOrigin;
#endif

#if defined(USE_TCMOD)
uniform vec4   u_DiffuseTexMatrix;
uniform vec4   u_DiffuseTexOffTurb;
#endif

uniform mat4   u_ModelViewProjectionMatrix;
uniform mat4	u_ViewProjectionMatrix;
uniform mat4   u_ModelMatrix;
uniform mat4	u_NormalMatrix;

uniform vec4   u_BaseColor;
uniform vec4   u_VertColor;

#if defined(USE_VERTEX_ANIMATION)
uniform float  u_VertexLerp;
#elif defined(USE_SKELETAL_ANIMATION)
uniform mat4   u_BoneMatrices[20];
#endif

uniform vec4  u_PrimaryLightOrigin;
uniform float u_PrimaryLightRadius;

varying vec2   var_TexCoords;
varying vec2   var_TexCoords2;

varying vec4   var_Color;

varying vec3 var_N;
varying vec4   var_Normal;
varying vec4   var_Tangent;
varying vec4   var_Bitangent;
varying vec3   var_ViewDir;

varying vec4   var_PrimaryLightDir;

varying vec3   var_vertPos;

varying vec3	var_Blending;
varying float	var_Slope;
varying float	var_usingSteepMap;

varying vec2   var_nonTCtexCoords; // for steep maps

#if defined(USE_TCGEN)
vec2 GenTexCoords(int TCGen, vec3 position, vec3 normal, vec3 TCGenVector0, vec3 TCGenVector1)
{
	vec2 tex = attr_TexCoord0.st;

	if (TCGen >= TCGEN_LIGHTMAP && TCGen <= TCGEN_LIGHTMAP3)
	{
		tex = attr_TexCoord1.st;
	}
	else if (TCGen == TCGEN_ENVIRONMENT_MAPPED)
	{
		vec3 viewer = normalize(u_LocalViewOrigin - position);
		vec2 ref = reflect(viewer, normal).yz;
		tex.s = ref.x * -0.5 + 0.5;
		tex.t = ref.y *  0.5 + 0.5;
	}
	else if (TCGen == TCGEN_VECTOR)
	{
		tex = vec2(dot(position, TCGenVector0), dot(position, TCGenVector1));
	}

	return tex;
}
#endif //defined(USE_TCGEN)

#if defined(USE_TCMOD)
vec2 ModTexCoords(vec2 st, vec3 position, vec4 texMatrix, vec4 offTurb)
{
	float amplitude = offTurb.z;
	float phase = offTurb.w * 2.0 * M_PI;
	vec2 st2;
	st2.x = st.x * texMatrix.x + (st.y * texMatrix.z + offTurb.x);
	st2.y = st.x * texMatrix.y + (st.y * texMatrix.w + offTurb.y);

	vec2 offsetPos = vec2(position.x + position.z, position.y);

	vec2 texOffset = sin(offsetPos * (2.0 * M_PI / 1024.0) + vec2(phase));

	return st2 + texOffset * amplitude;
}
#endif //defined(USE_TCMOD)

#if defined(USE_TRI_PLANAR) || defined(USE_REGIONS)
void GetBlending(vec3 normal)
{
	if (u_Local5.a > 0.0)
	{// Steep maps...
		vec3 blend_weights = abs(normalize(normal.xyz));   // Tighten up the blending zone:
		blend_weights = (blend_weights - 0.2) * 7.0;
		blend_weights = max(blend_weights, 0.0);      // Force weights to sum to 1.0 (very important!)
		blend_weights /= vec3(blend_weights.x + blend_weights.y + blend_weights.z);
		var_Blending = blend_weights;
	}
}
#endif //defined(USE_TRI_PLANAR) || defined(USE_REGIONS)

#if defined(USE_TRI_PLANAR)
vec3 vectoangles(in vec3 value1) {
	float	forward;
	float	yaw, pitch;
	vec3	angles;

	if (value1.g == 0 && value1.r == 0) {
		yaw = 0;
		if (value1.b > 0) {
			pitch = 90;
		}
		else {
			pitch = 270;
		}
	}
	else {
		if (value1.r > 0) {
			yaw = (atan(value1.g, value1.r) * 180 / M_PI);
		}
		else if (value1.g > 0) {
			yaw = 90;
		}
		else {
			yaw = 270;
		}
		if (yaw < 0) {
			yaw += 360;
		}

		forward = sqrt(value1.r*value1.r + value1.g*value1.g);
		pitch = (atan(value1.b, forward) * 180 / M_PI);
		if (pitch < 0) {
			pitch += 360;
		}
	}

	angles.r = -pitch;
	angles.g = yaw;
	angles.b = 0.0;

	return angles;
}
#endif //defined(USE_TRI_PLANAR)

vec4 ConvertToNormals(vec4 color)
{
	// This makes silly assumptions, but it adds variation to the output. Hopefully this will look ok without doing a crapload of texture lookups or
	// wasting vram on real normals.
	//
	// UPDATE: In my testing, this method looks just as good as real normal maps. I am now using this as default method unless r_normalmapping >= 2
	// for the very noticable FPS boost over texture lookups.

	//N = vec3((color.r + color.b) / 2.0, (color.g + color.b) / 2.0, (color.r + color.g) / 2.0);
	vec3 N = vec3(clamp(color.r + color.b, 0.0, 1.0), clamp(color.g + color.b, 0.0, 1.0), clamp(color.r + color.g, 0.0, 1.0));
	N.xy = 1.0 - N.xy;
	N.xyz *= 0.04;
	vec4 norm = vec4(N, 1.0 - (length(N.xyz) / 3.0));
	return norm;
}

void main()
{
	vec3 normal = vec3(attr_Normal.xyz);
	vec3 position = vec3(attr_Position.xyz);

#if defined(USE_VERTEX_ANIMATION)
	position = mix(attr_Position, attr_Position2, u_VertexLerp);
	normal = mix(attr_Normal, attr_Normal2, u_VertexLerp);
	vec3 tangent = mix(attr_Tangent.xyz, attr_Tangent2.xyz, u_VertexLerp);

	normal = normal  * 2.0 - 1.0;
	tangent = tangent * 2.0 - 1.0;
#elif defined(USE_SKELETAL_ANIMATION)
	vec4 position4 = vec4(0.0);
	vec4 normal4 = vec4(0.0);
	vec4 originalPosition = vec4(attr_Position, 1.0);
	//vec4 originalNormal = vec4(attr_Normal - vec3(0.5), 0.0);
	vec4 originalNormal = vec4(attr_Normal * 2.0 - 1.0, 0.0);
	vec4 tangent4 = vec4(0.0);
	//vec4 originalTangent = vec4(attr_Tangent.xyz - vec3(0.5), 0.0);
	vec4 originalTangent = vec4(attr_Tangent.xyz * 2.0 - 1.0, 0.0);

	for (int i = 0; i < 4; i++)
	{
		int boneIndex = int(attr_BoneIndexes[i]);

		position4 += (u_BoneMatrices[boneIndex] * originalPosition) * attr_BoneWeights[i];
		normal4 += (u_BoneMatrices[boneIndex] * originalNormal) * attr_BoneWeights[i];
		tangent4 += (u_BoneMatrices[boneIndex] * originalTangent) * attr_BoneWeights[i];
	}

	position = position4.xyz;
	normal = normalize(normal4.xyz);
	vec3 tangent = normalize(tangent4.xyz);
#else
	vec3 tangent = attr_Tangent.xyz;

	normal = normal  * 2.0 - 1.0;
	tangent = tangent * 2.0 - 1.0;
#endif

#if defined(USE_TCGEN)
	vec2 texCoords = GenTexCoords(u_TCGen0, position, normal, u_TCGen0Vector0, u_TCGen0Vector1);
#else
	vec2 texCoords = attr_TexCoord0.st;
#endif

#if defined(USE_TCMOD)
	var_TexCoords.xy = ModTexCoords(texCoords, position, u_DiffuseTexMatrix, u_DiffuseTexOffTurb);
#else
	var_TexCoords.xy = texCoords;
#endif

	gl_Position = u_ModelViewProjectionMatrix * vec4(position, 1.0);

	//vec3 preMMPos = position.xyz;
	/*vec3 preMMNorm = normal.xyz;
	vec4 preMMtangent = vec4(tangent, 0.0);
	vec4 preMMbitangent = vec4(cross(normal, tangent) * (attr_Tangent.w * 2.0 - 1.0), 0.0);*/

#if defined(USE_MODELMATRIX) && defined(USE_VERTEX_ANIMATION) //&& !defined(USE_VERTEX_ANIMATION) && !defined(USE_SKELETAL_ANIMATION)
	position = (u_ModelMatrix * vec4(position, 1.0)).xyz;
	normal = (u_ModelMatrix * vec4(normal, 0.0)).xyz;
	tangent = (u_ModelMatrix * vec4(tangent, 0.0)).xyz;
#endif

	vec3 preMMPos = position.xyz;
	vec3 preMMNorm = normal.xyz;
	vec4 preMMtangent = vec4(tangent, 0.0);
	vec4 preMMbitangent = vec4(cross(normal, tangent) * (attr_Tangent.w * 2.0 - 1.0), 0.0);

	//var_vertPos = position.xyz;

	vec3 bitangent = cross(normal, tangent) * (attr_Tangent.w * 2.0 - 1.0);

#if defined(USE_LIGHTMAP)
	var_TexCoords2 = attr_TexCoord1.st;
#else
	var_TexCoords2 = vec2(0.0);
#endif

	var_Color = u_VertColor * attr_Color + u_BaseColor;

	var_PrimaryLightDir.xyz = u_PrimaryLightOrigin.xyz - (position * u_PrimaryLightOrigin.w);
	var_PrimaryLightDir.w = u_PrimaryLightRadius * u_PrimaryLightRadius;

	vec3 viewDir = u_ViewOrigin - position;
	var_ViewDir = viewDir;

	// store view direction in tangent space to save on varyings
	var_Normal = vec4(normal, viewDir.x);
	var_Tangent = vec4(tangent, viewDir.y);
	var_Bitangent = vec4(bitangent, viewDir.z);

	var_nonTCtexCoords = attr_TexCoord0.st;

	var_usingSteepMap = 0.0;
	var_Slope = 0.0;

#if defined(USE_TRI_PLANAR)

	//
	// Steep Maps...
	//

	if (u_Local5.a > 0.0)
	{// Steep maps...
		float pitch = vectoangles(normalize(normal.xyz)).r;

		if (pitch > 180)
			pitch -= 360;

		if (pitch < -180)
			pitch += 360;

		pitch += 90.0f;

		if (pitch > 46.0 || pitch < -46.0)
		{
			var_usingSteepMap = 1.0;
			var_Slope = 1.0;
		}
		else if (pitch > 26.0 || pitch < -26.0)
		{// do not add to foliage map on this slope, but still do original texture
			var_usingSteepMap = 1.0;
			var_Slope = 0.0;
		}
		else
		{
			var_usingSteepMap = 0.0;
			var_Slope = 0.0;
		}
	}

#endif //defined(USE_TRI_PLANAR)

	var_Blending = vec3(0.0);

#if defined(USE_TRI_PLANAR) || defined(USE_REGIONS)

	GetBlending(normalize(attr_Normal.xyz * 2.0 - 1.0));

#endif //defined(USE_TRI_PLANAR) || defined(USE_REGIONS)

#if defined(USE_TESSELLATION) || defined(USE_ICR_CULLING)
	//mat3 tangentToWorld = mat3(var_Tangent.xyz, var_Bitangent.xyz, attr_Normal.xyz * 2.0 - 1.0);
	//vec4 color = texture(u_DiffuseMap, var_TexCoords.xy);
	//vec4 no = ConvertToNormals(color);
	//vec3 nMap = normalize(tangentToWorld * ((no * 2.0 - 1.0).xyz * no.a));
	//vec3 nMap = normalize(attr_Normal.xyz * 2.0 - 1.0);
	//vec3 nMap = normalize(tangentToWorld * attr_Normal.xyz * 2.0 - 1.0);
	//nMap.z = sqrt(clamp((0.25 - nMap.x * nMap.x) - nMap.y * nMap.y, 0.0, 1.0));

	WorldPos_CS_in = vec4(preMMPos, 1.0);
	//WorldPos_CS_in = vec4(var_vertPos.xyz, 1.0);
	TexCoord_CS_in = var_TexCoords.xy;
	//Normal_CS_in = /*nMap.xyz;*/attr_Normal.xyz * 2.0 - 1.0;//-preMMNorm.xyz;//-var_Normal.xyz;
	Normal_CS_in = var_Normal.xyz;
	ViewDir_CS_in = var_ViewDir;
	Tangent_CS_in = var_Tangent;
	Bitangent_CS_in = var_Bitangent;
	Color_CS_in = var_Color;
	PrimaryLightDir_CS_in = var_PrimaryLightDir;
	TexCoord2_CS_in = var_TexCoords2;
	Blending_CS_in = var_Blending;
	Slope_CS_in = var_Slope;
	usingSteepMap_CS_in = var_usingSteepMap;
	gl_Position = vec4(preMMPos, 1.0);
	//gl_Position = vec4(var_vertPos.xyz, 1.0);

#else

	var_vertPos = preMMPos.xyz;

#endif //defined(USE_TESSELLATION)
}