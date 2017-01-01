//#define USE_REGIONS

#define unOpenGlIsFuckedUpify(x) ( x / 524288.0 )

uniform sampler2D			u_DiffuseMap;
uniform sampler2D			u_SteepMap;
uniform sampler2D			u_SteepMap2;
uniform sampler2D			u_SplatControlMap;
uniform sampler2D			u_SplatMap1;
uniform sampler2D			u_SplatMap2;
uniform sampler2D			u_SplatMap3;
uniform sampler2D			u_SplatMap4;
uniform sampler2D			u_SplatNormalMap1;
uniform sampler2D			u_SplatNormalMap2;
uniform sampler2D			u_SplatNormalMap3;
uniform sampler2D			u_SplatNormalMap4;

uniform sampler2D			u_LightMap;

uniform sampler2D			u_NormalMap;
uniform sampler2D			u_NormalMap2;
uniform sampler2D			u_NormalMap3;

uniform sampler2D			u_DeluxeMap;

#if defined(USE_SPECULARMAP)
uniform sampler2D			u_SpecularMap;
#endif

#if defined(USE_SHADOWMAP)
uniform sampler2D			u_ShadowMap;
#endif

#if defined(USE_CUBEMAP)
#define textureCubeLod textureLod // UQ1: > ver 140 support
uniform samplerCube			u_CubeMap;
#endif

uniform sampler2D			u_OverlayMap;



uniform vec2				u_Dimensions;
uniform vec4				u_Local1; // parallaxScale, haveSpecular, specularScale, materialType
uniform vec4				u_Local2; // ExtinctionCoefficient
uniform vec4				u_Local3; // RimScalar, MaterialThickness, subSpecPower, cubemapScale
uniform vec4				u_Local4; // haveNormalMap, isMetalic, hasRealSubsurfaceMap, sway
uniform vec4				u_Local5; // hasRealOverlayMap, overlaySway, blinnPhong, hasSteepMap
uniform vec4				u_Local6; // useSunLightSpecular, hasSteepMap2, MAP_SIZE, WATER_LEVEL
uniform vec4				u_Local7; // hasSplatMap1, hasSplatMap2, hasSplatMap3, hasSplatMap4
uniform vec4				u_Local9; // testvalue0, 1, 2, 3

#define WATER_LEVEL			u_Local6.a

uniform mat4				u_ModelViewProjectionMatrix;
uniform mat4				u_ModelMatrix;
uniform mat4				u_invEyeProjectionMatrix;
uniform mat4				u_ModelViewMatrix;

uniform vec4				u_EnableTextures;

uniform vec4				u_PrimaryLightOrigin;
uniform vec3				u_PrimaryLightColor;
uniform vec3				u_PrimaryLightAmbient;

uniform vec4				u_NormalScale;
uniform vec4				u_SpecularScale;

uniform vec4				u_CubeMapInfo;
uniform float				u_CubeMapStrength;

uniform vec3				u_ViewOrigin;


#if defined(USE_TESSELLATION) || defined(USE_ICR_CULLING)

in precise vec3				Normal_FS_in;
in precise vec2				TexCoord_FS_in;
in precise vec3				WorldPos_FS_in;
in precise vec3				ViewDir_FS_in;
in precise vec4				Tangent_FS_in;
in precise vec4				Bitangent_FS_in;

in precise vec4				Color_FS_in;
in precise vec4				PrimaryLightDir_FS_in;
in precise vec2				TexCoord2_FS_in;

in precise vec3				Blending_FS_in;
flat in float				Slope_FS_in;
flat in float				usingSteepMap_FS_in;


#define m_Normal 			normalize(Normal_FS_in.xyz)

#define m_TexCoords			TexCoord_FS_in
#define m_vertPos			WorldPos_FS_in
#define m_ViewDir			ViewDir_FS_in

#define var_Normal2			ViewDir_FS_in.x

#define var_Tangent			Tangent_FS_in
#define var_Bitangent		Bitangent_FS_in

#define var_nonTCtexCoords	TexCoord_FS_in
#define var_Color			Color_FS_in
#define	var_PrimaryLightDir PrimaryLightDir_FS_in
#define var_TexCoords2		TexCoord2_FS_in

#define var_Blending		Blending_FS_in
#define var_Slope			Slope_FS_in
#define var_usingSteepMap	usingSteepMap_FS_in


#else //!defined(USE_TESSELLATION) && !defined(USE_ICR_CULLING)

varying vec2				var_TexCoords;
varying vec2				var_TexCoords2;
varying vec4				var_Normal;

#define var_Normal2			var_Normal.w

varying vec4				var_Tangent;
varying vec4				var_Bitangent;
varying vec4				var_Color;

varying vec4				var_PrimaryLightDir;

varying vec3				var_vertPos;

varying vec3				var_ViewDir;
varying vec2				var_nonTCtexCoords; // for steep maps


varying vec3				var_Blending;
varying float				var_Slope;
varying float				var_usingSteepMap;


#define m_Normal			var_Normal
#define m_TexCoords			var_TexCoords
#define m_vertPos			var_vertPos
#define m_ViewDir			var_ViewDir


#endif //defined(USE_TESSELLATION) || defined(USE_ICR_CULLING)



out vec4 out_Glow;
out vec4 out_Position;
out vec4 out_Normal;


// 'threshold ' is constant , 'value ' is smoothly varying
/*float aastep ( float threshold , float value )
{
	float afwidth = 0.7 * length ( vec2 ( dFdx ( value ) , dFdy ( value ))) ;
	// GLSL 's fwidth ( value ) is abs ( dFdx ( value )) + abs ( dFdy ( value ))
	return smoothstep ( threshold - afwidth , threshold + afwidth , value ) ;
}*/

vec4 ConvertToNormals ( vec4 color )
{
	// This makes silly assumptions, but it adds variation to the output. Hopefully this will look ok without doing a crapload of texture lookups or
	// wasting vram on real normals.
	//
	// UPDATE: In my testing, this method looks just as good as real normal maps. I am now using this as default method unless r_normalmapping >= 2
	// for the very noticable FPS boost over texture lookups.

	vec3 color2 = color.rgb;

	vec3 N = vec3(clamp(color2.r + color2.b, 0.0, 1.0), clamp(color2.g + color2.b, 0.0, 1.0), clamp(color2.r + color2.g, 0.0, 1.0));

	vec3 brightness = color2.rgb; //adaptation luminance
	brightness = (brightness/(brightness+1.0));
	brightness = vec3(max(brightness.x, max(brightness.y, brightness.z)));
	vec3 brightnessMult = (vec3(1.0) - brightness) * 0.5;

	color2 = pow(clamp(color2 + brightnessMult, 0.0, 1.0), vec3(2.0));

	N.xy = 1.0 - N.xy;
	N.xyz = N.xyz * 0.5 + 0.5;
	N.xyz = pow(N.xyz, vec3(2.0));
	N.xyz *= 0.8;

	//float displacement = brightness.r;
	float displacement = clamp(length(color.rgb), 0.0, 1.0);
	//float displacement = clamp(length(color2.rgb), 0.0, 1.0);
#define const_1 ( 32.0 / 255.0)
#define const_2 (255.0 / 219.0)
	displacement = clamp((clamp(displacement - const_1, 0.0, 1.0)) * const_2, 0.0, 1.0);

	vec4 norm = vec4(N, displacement);
	return norm;
}

#if defined(USE_TRI_PLANAR) || defined(USE_REGIONS)
vec4 GetControlMap( sampler2D tex, float scale)
{
	vec4 xaxis = texture( tex, (m_vertPos.yz * scale) * 0.5 + 0.5);
	vec4 yaxis = texture( tex, (m_vertPos.xz * scale) * 0.5 + 0.5);
	vec4 zaxis = texture( tex, (m_vertPos.xy * scale) * 0.5 + 0.5);

	return xaxis * var_Blending.x + yaxis * var_Blending.y + zaxis * var_Blending.z;
}
#endif //defined(USE_TRI_PLANAR) || defined(USE_REGIONS)

#if defined(USE_REGIONS)
float region1min = 0.0;
float region1max = 0.5;

float region2min = 0.5;
float region2max = 0.7;

float region3min = 0.7;
float region3max = 0.85;

float region4min = 0.85;
float region4max = 1.0;

/*
float region5min = 1.2;
float region5max = 1.5;

float region6min = 1.5;
float region6max = 1.8;

float region7min = 1.8;
float region7max = 2.1;
*/

#define region1ColorMap u_DiffuseMap
#define region2ColorMap u_SplatMap1
#define region3ColorMap u_SplatMap2
#define region4ColorMap u_SplatMap3
/*#define region5ColorMap u_SplatMap4
#define region6ColorMap u_SteepMap
#define region7ColorMap u_SteepMap2*/

vec4 GenerateTerrainMap(vec2 coord)
{
	vec4 controlMap = GetControlMap(u_SplatControlMap, 1.0 / u_Local6.b /* control scale */);
    vec4 terrainColor = vec4(0.0, 0.0, 0.0, 1.0);
    float height = clamp(length(controlMap.rgb) * 3.0, 0.0, 1.0);
    float regionMin = 0.0;
    float regionMax = 0.0;
    float regionRange = 0.0;
    float regionWeight = 0.0;

    // Terrain region 1.
    regionMin = region1min;
    regionMax = region1max;
    regionRange = regionMax - regionMin;
    regionWeight = (regionRange - abs(height - regionMax)) / regionRange;
    regionWeight = max(0.0, regionWeight);
    terrainColor += regionWeight * texture2D(region1ColorMap, coord);

    // Terrain region 2.
    regionMin = region2min;
    regionMax = region2max;
    regionRange = regionMax - regionMin;
    regionWeight = (regionRange - abs(height - regionMax)) / regionRange;
    regionWeight = max(0.0, regionWeight);
	if (u_Local7.r > 0.0)
		terrainColor += regionWeight * texture2D(region2ColorMap, coord);
	else
		terrainColor += regionWeight * texture2D(region1ColorMap, coord);

    // Terrain region 3.
    regionMin = region3min;
    regionMax = region3max;
    regionRange = regionMax - regionMin;
    regionWeight = (regionRange - abs(height - regionMax)) / regionRange;
    regionWeight = max(0.0, regionWeight);
	if (u_Local7.g > 0.0)
		terrainColor += regionWeight * texture2D(region3ColorMap, coord);
	else
		terrainColor += regionWeight * texture2D(region1ColorMap, coord);

    // Terrain region 4.
    regionMin = region4min;
    regionMax = region4max;
    regionRange = regionMax - regionMin;
    regionWeight = (regionRange - abs(height - regionMax)) / regionRange;
    regionWeight = max(0.0, regionWeight);
	if (u_Local7.b > 0.0)
		terrainColor += regionWeight * texture2D(region4ColorMap, coord);
	else
		terrainColor += regionWeight * texture2D(region1ColorMap, coord);

	/*
	// Terrain region 5.
	regionMin = region5min;
	regionMax = region5max;
	regionRange = regionMax - regionMin;
	regionWeight = (regionRange - abs(height - regionMax)) / regionRange;
	regionWeight = max(0.0, regionWeight);
	if (u_Local7.a > 0.0)
		terrainColor += regionWeight * texture2D(region5ColorMap, coord);
	else
		terrainColor += regionWeight * texture2D(region1ColorMap, coord);

	// Terrain region 6.
	regionMin = region6min;
	regionMax = region6max;
	regionRange = regionMax - regionMin;
	regionWeight = (regionRange - abs(height - regionMax)) / regionRange;
	regionWeight = max(0.0, regionWeight);
	if (u_Local5.a > 0.0)
		terrainColor += regionWeight * texture2D(region6ColorMap, coord);
	else
		terrainColor += regionWeight * texture2D(region1ColorMap, coord);

	// Terrain region 7.
	regionMin = region7min;
	regionMax = region7max;
	regionRange = regionMax - regionMin;
	regionWeight = (regionRange - abs(height - regionMax)) / regionRange;
	regionWeight = max(0.0, regionWeight);
	if (u_Local6.g > 0.0)
		terrainColor += regionWeight * texture2D(region7ColorMap, coord);
	else
		terrainColor += regionWeight * texture2D(region1ColorMap, coord);
	*/

    return terrainColor;
}
#endif //defined(USE_REGIONS)

#if defined(USE_TRI_PLANAR)
vec3 vLocalSeed;

// This function returns random number from zero to one
float randZeroOne()
{
    uint n = floatBitsToUint(vLocalSeed.y * 214013.0 + vLocalSeed.x * 2531011.0 + vLocalSeed.z * 141251.0);
    n = n * (n * n * 15731u + 789221u);
    n = (n >> 9u) | 0x3F800000u;

    float fRes =  2.0 - uintBitsToFloat(n);
    vLocalSeed = vec3(vLocalSeed.x + 147158.0 * fRes, vLocalSeed.y*fRes  + 415161.0 * fRes, vLocalSeed.z + 324154.0*fRes);
    return fRes;
}

// For fake normal map lookups.
#define FAKE_MAP_NONE 0
#define FAKE_MAP_NORMALMAP 1
#define FAKE_MAP_NORMALMAP2 2
#define FAKE_MAP_NORMALMAP3 3

vec3 splatblend(vec4 texture1, float a1, vec4 texture2, float a2)
{
    float depth = 0.2;
    float ma = max(texture1.a + a1, texture2.a + a2) - depth;

    float b1 = max(texture1.a + a1 - ma, 0);
    float b2 = max(texture2.a + a2 - ma, 0);

    return (texture1.rgb * b1 + texture2.rgb * b2) / (b1 + b2);
}

vec4 GetMap( in sampler2D tex, float scale, vec2 ParallaxOffset, int fakeMapType)
{
	bool fakeNormal = false;
	vec4 xaxis;
	vec4 yaxis;
	vec4 zaxis;

	if (fakeMapType == FAKE_MAP_NORMALMAP)
	{
		fakeNormal = true;

		xaxis = texture( u_DiffuseMap, (m_vertPos.yz * scale) + ParallaxOffset.xy);
		yaxis = texture( u_DiffuseMap, (m_vertPos.xz * scale) + ParallaxOffset.xy);
		zaxis = texture( u_DiffuseMap, (m_vertPos.xy * scale) + ParallaxOffset.xy);
	}
	else if (fakeMapType == FAKE_MAP_NORMALMAP2)
	{
		fakeNormal = true;

		xaxis = texture( u_SteepMap, (m_vertPos.yz * scale) + ParallaxOffset.xy);
		yaxis = texture( u_SteepMap, (m_vertPos.xz * scale) + ParallaxOffset.xy);
		zaxis = texture( u_SteepMap, (m_vertPos.xy * scale) + ParallaxOffset.xy);
	}
	else if (fakeMapType == FAKE_MAP_NORMALMAP3)
	{
		fakeNormal = true;

		xaxis = texture( u_SteepMap2, (m_vertPos.yz * scale) + ParallaxOffset.xy);
		yaxis = texture( u_SteepMap2, (m_vertPos.xz * scale) + ParallaxOffset.xy);
		zaxis = texture( u_SteepMap2, (m_vertPos.xy * scale) + ParallaxOffset.xy);
	}
	else
	{
		xaxis = texture( tex, (m_vertPos.yz * scale) + ParallaxOffset.xy);
		yaxis = texture( tex, (m_vertPos.xz * scale) + ParallaxOffset.xy);
		zaxis = texture( tex, (m_vertPos.xy * scale) + ParallaxOffset.xy);
	}

	if (fakeNormal)
	{
		return ConvertToNormals(xaxis * var_Blending.x + yaxis * var_Blending.y + zaxis * var_Blending.z);
	}

	return xaxis * var_Blending.x + yaxis * var_Blending.y + zaxis * var_Blending.z;
}

vec4 GetNonSplatMap( in sampler2D tex, vec2 coord )
{
	return texture( tex, coord );
}

vec4 GetSplatMap(vec2 texCoords, vec2 ParallaxOffset, float pixRandom, vec4 inColor, float inA1)
{
	if (u_Local7.r <= 0.0 && u_Local7.g <= 0.0 && u_Local7.b <= 0.0 && u_Local7.a <= 0.0)
	{
		return inColor;
	}

	if (u_Local6.g > 0.0 && m_vertPos.z <= WATER_LEVEL)// + 128.0) + (64.0 * pixRandom))
	{// Steep maps (water edges)... Underwater doesn't use splats for now...
		return inColor;
	}

	// Splat blend in all the textues using the control strengths...

	vec4 splatColor = inColor;

	vec4 control = GetControlMap(u_SplatControlMap, 1.0 / u_Local6.b /* control scale */);
	control = clamp(pow(control, vec4(2.5)) * 10.0, 0.0, 1.0);

	if (length(control.rgb/*a*/) <= 0.0)
	{
		return inColor;
	}

	if (u_Local7.r > 0.0 && control.r > 0.0)
	{
		vec4 tex = GetMap(u_SplatMap1, 0.01, ParallaxOffset, FAKE_MAP_NONE);
		splatColor = mix(splatColor, tex, control.r * tex.a);
	}

	if (u_Local7.g > 0.0 && control.g > 0.0)
	{
		vec4 tex = GetMap(u_SplatMap2, 0.01, ParallaxOffset, FAKE_MAP_NONE);
		splatColor = mix(splatColor, tex, control.g * tex.a);
	}

	if (u_Local7.b > 0.0 && control.b > 0.0)
	{
		vec4 tex = GetMap(u_SplatMap3, 0.01, ParallaxOffset, FAKE_MAP_NONE);
		splatColor = mix(splatColor, tex, control.b * tex.a);
	}
	/*
	if (u_Local7.a > 0.0 && control.a > 0.0)
	{
		vec4 tex = GetMap(u_SplatMap4, 0.01, ParallaxOffset, FAKE_MAP_NONE);
		splatColor = mix(splatColor, tex, control.a * tex.a);
	}
	*/

	return splatColor;
}

vec4 GetDiffuse(vec2 texCoords, vec2 ParallaxOffset, float pixRandom)
{
	if (u_Local6.g > 0.0 && m_vertPos.z <= WATER_LEVEL + 128.0 + (64.0 * pixRandom))
	{// Steep maps (water edges)...
		float mixVal = ((WATER_LEVEL + 128.0) - m_vertPos.z) / 128.0;

		vec4 tex1 = GetMap(u_SteepMap2, 0.0075, ParallaxOffset, FAKE_MAP_NONE);
		float a1 = 0.0;

		if (u_Local4.r <= 0.0) // save texture lookup
			a1 = ConvertToNormals(tex1).a;
		else
			a1 = GetMap(u_NormalMap3, 0.0075, ParallaxOffset, FAKE_MAP_NORMALMAP3).a;

		vec4 tex2 = GetMap(u_DiffuseMap, 0.0075, ParallaxOffset, FAKE_MAP_NONE);
		float a2 = 0.0;

		if (u_Local4.r <= 0.0) // save texture lookup
			a2 = ConvertToNormals(tex2).a;
		else
			a2 = GetMap(u_NormalMap, 0.0075, ParallaxOffset, FAKE_MAP_NORMALMAP).a;

		if (u_Local7.r <= 0.0 && u_Local7.g <= 0.0 && u_Local7.b <= 0.0 && u_Local7.a <= 0.0)
		{// No splat maps...
			return vec4(splatblend(tex1, a1 * (a1 * mixVal), tex2, a2 * (1.0 - (a2 * mixVal))), 1.0);
		}

		// Splat mapping...
		tex2 = GetSplatMap(texCoords, ParallaxOffset, pixRandom, tex2, a2);

		return vec4(splatblend(tex1, a1 * (a1 * mixVal), tex2, a2 * (1.0 - (a2 * mixVal))), 1.0);
	}
	else if (u_Local5.a > 0.0 && var_Slope > 0)
	{// Steep maps (high angles)...
		return GetMap(u_SteepMap, 0.0025, ParallaxOffset, FAKE_MAP_NONE);
	}
	else if (u_Local5.a > 0.0)
	{// Steep maps (low angles)...
		if (u_Local7.r <= 0.0 && u_Local7.g <= 0.0 && u_Local7.b <= 0.0 && u_Local7.a <= 0.0)
		{// No splat maps...
			return GetMap(u_DiffuseMap, 0.0075, ParallaxOffset, FAKE_MAP_NONE);
		}

		// Splat mapping...
		vec4 tex = GetMap(u_DiffuseMap, 0.0075, ParallaxOffset, FAKE_MAP_NONE);
		float a1 = 0.0;

		if (u_Local4.r <= 0.0) // save texture lookup
			a1 = ConvertToNormals(tex).a;
		else
			a1 = GetMap(u_NormalMap, 0.0075, ParallaxOffset, FAKE_MAP_NORMALMAP).a;

		return GetSplatMap(texCoords, ParallaxOffset, pixRandom, tex, a1);
	}
	else
	{
		return GetNonSplatMap(u_DiffuseMap, texCoords);
	}
}

vec4 GetNormal(vec2 texCoords, vec2 ParallaxOffset, float pixRandom)
{
	if (u_Local6.g > 0.0 && m_vertPos.z <= WATER_LEVEL + 128.0 + (64.0 * pixRandom))
	{// Steep maps (water edges)...
		float mixVal = ((WATER_LEVEL + 128.0) - m_vertPos.z) / 128.0;

		vec4 tex1 = GetMap(u_NormalMap3, 0.0075, ParallaxOffset, FAKE_MAP_NORMALMAP3);
		float a1 = tex1.a;

		vec4 tex2 = GetMap(u_NormalMap, 0.0075, ParallaxOffset, FAKE_MAP_NORMALMAP);
		float a2 = tex2.a;

		return vec4(splatblend(tex1, a1 * (a1 * mixVal), tex2, a2 * (1.0 - (a2 * mixVal))), 1.0);
	}
	else if (u_Local5.a > 0.0 && var_Slope > 0)
	{// Steep maps (high angles)...
		return GetMap(u_NormalMap2, 0.0025, ParallaxOffset, FAKE_MAP_NORMALMAP2);
	}
	else if (u_Local5.a > 0.0)
	{// Steep maps (normal angles)...
		return GetMap(u_NormalMap, 0.0075, ParallaxOffset, FAKE_MAP_NORMALMAP);
	}
	else
	{
		if (u_Local4.r <= 0.0)
		{
			return ConvertToNormals(GetNonSplatMap(u_DiffuseMap, texCoords));
		}

		return texture(u_NormalMap, texCoords);
	}
}

#else //!defined(USE_TRI_PLANAR)

vec4 GetDiffuse(vec2 texCoords, vec2 ParallaxOffset, float pixRandom)
{
#if defined(USE_REGIONS)
	return GenerateTerrainMap(texCoords);
#else
	return texture(u_DiffuseMap, texCoords);
#endif
}

vec4 GetNormal(vec2 texCoords, vec2 ParallaxOffset, float pixRandom)
{
#if defined(USE_REGIONS)
	return ConvertToNormals(GenerateTerrainMap(texCoords));
#else
	if (u_Local4.r <= 0.0)
	{
		return ConvertToNormals(texture(u_DiffuseMap, texCoords));
	}

	return texture(u_NormalMap, texCoords);
#endif
}
#endif //!defined(USE_TRI_PLANAR)

float GetDepth(vec2 t)
{
	return 1.0 - GetNormal(t, vec2(0.0), 0.0).a;
}

#if defined(USE_PARALLAXMAP)

float RayIntersectDisplaceMap(vec2 dp, inout float displacement)
{
	if (u_Local1.x == 0.0)
		return 0.0;

	displacement = GetDepth(dp);
	return (displacement - 1.0) * u_Local1.x;
}

float ReliefMapping(vec2 dp, vec2 ds)
{
	const int linear_steps = 10;
	const int binary_steps = 5;
	float depth_step = 1.0 / linear_steps;
	float size = depth_step;
	float depth = 1.0;
	float best_depth = 1.0;
	for (int i = 0 ; i < linear_steps - 1 ; ++i) {
		depth -= size;
		float t = GetDepth(dp + ds * depth);
		if (depth >= 1.0 - t)
			best_depth = depth;
	}
	depth = best_depth - size;
	for (int i = 0 ; i < binary_steps ; ++i) {
		size *= 0.5;
		float t = GetDepth(dp + ds * depth);
		if (depth >= 1.0 - t) {
			best_depth = depth;
			depth -= 2 * size;
		}
		depth += size;
	}
	return clamp(best_depth, 0.0, 1.0);
}
#endif //defined(USE_PARALLAXMAP)

vec3 EnvironmentBRDF(float gloss, float NE, vec3 specular)
{
	vec4 t = vec4( 1/0.96, 0.475, (0.0275 - 0.25 * 0.04)/0.96,0.25 ) * gloss;
	t += vec4( 0.0, 0.0, (0.015 - 0.75 * 0.04)/0.96,0.75 );
	float a0 = t.x * min( t.y, exp2( -9.28 * NE ) ) + t.z;
	float a1 = t.w;
	return clamp( a0 + specular * ( a1 - a0 ), 0.0, 1.0 );
}



float blinnPhongSpecular(in vec3 normalVec, in vec3 lightVec, in float specPower)
{
    vec3 halfAngle = normalize(normalVec + lightVec);
    return pow(clamp(dot(normalVec,halfAngle),0.0,1.0),specPower);
}

//---------------------------------------------------------------------------------------------
// Normal Blending Techniques
//---------------------------------------------------------------------------------------------

// RNM
vec3 NormalBlend_RNM(vec3 n1, vec3 n2)
{
    // Unpack (see article on why it's not just n*2-1)
	n1 = n1*vec3( 2,  2, 2) + vec3(-1, -1,  0);
    n2 = n2*vec3(-2, -2, 2) + vec3( 1,  1, -1);

    // Blend
    return n1*dot(n1, n2)/n1.z - n2;
}

// Linear Blending
vec3 NormalBlend_Linear(vec3 n1, vec3 n2)
{
    // Unpack
	n1 = n1*2.0 - 1.0;
    n2 = n2*2.0 - 1.0;

	return normalize(n1 + n2);
}

#define TECHNIQUE_RNM 				 0
#define TECHNIQUE_Linear		     1

// Combine normals
vec3 CombineNormal(vec3 n1, vec3 n2, int technique)
{
 	if (technique == TECHNIQUE_RNM)
        return NormalBlend_RNM(n1, n2);
    else
        return NormalBlend_Linear(n1, n2);
}

mat3 cotangent_frame( vec3 N, vec3 p, vec2 uv )
{
	// get edge vectors of the pixel triangle
	vec3 dp1 = dFdx( p );
	vec3 dp2 = dFdy( p );
	vec2 duv1 = dFdx( uv );
	vec2 duv2 = dFdy( uv );

	// solve the linear system
	vec3 dp2perp = cross( dp2, N );
	vec3 dp1perp = cross( N, dp1 );
	vec3 T = dp2perp * duv1.x + dp1perp * duv2.x;
	vec3 B = dp2perp * duv1.y + dp1perp * duv2.y;

	// construct a scale-invariant frame
	float invmax = inversesqrt( max( dot(T,T), dot(B,B) ) );
	return mat3( T * invmax, B * invmax, N );
}

void main()
{
	vec3 viewDir = vec3(0.0), lightColor = vec3(0.0), ambientColor = vec3(0.0);
	vec4 specular = vec4(0.0);
	vec3 N, E, H;
	float NL, NH, NE, EH, attenuation;
	vec2 tex_offset = vec2(1.0 / u_Dimensions);
	vec2 texCoords = m_TexCoords.xy;

	float dist = distance(m_vertPos.xyz, u_ViewOrigin.xyz);
	bool isDistant = false;

	if (dist > 4096.0)
	{
		isDistant = true;
	}

	/*if (u_Local9.r == 1.0 && dist > 16384.0)
	{// TODO: Fog and not draw?
		discard;
	}*/

	#if 0
	{
		vec4 nm = GetNormal(texCoords, vec2(0.0), 0.0);
		gl_FragColor = vec4(nm.xyz, 1.0);
		//gl_FragColor = vec4(nm.a, nm.a, nm.a, 1.0);

		#if defined(USE_GLOW_BUFFER)
			out_Glow = gl_FragColor;
		#else
			out_Glow = vec4(0.0);
		#endif

		out_Normal = vec4(m_Normal.xyz * 0.5 + 0.5, 0.2);
		out_Position = vec4(m_vertPos, u_Local1.a );/// MATERIAL_LAST);
		return;
	}
	#endif


#if defined(USE_TRI_PLANAR)
	vLocalSeed = m_vertPos.xyz;
	float pixRandom = randZeroOne();
#else //!defined(USE_TRI_PLANAR)
	float pixRandom = 0.0; // Don't use it anyway...
#endif //defined(USE_TRI_PLANAR)



	#if !defined(USE_GLOW_BUFFER)
		if (u_Local4.a > 0.0 && !(u_Local5.a > 0.0 && var_Slope > 0) && !(u_Local6.g > 0.0 && m_vertPos.z <= WATER_LEVEL + 128.0 + (64.0 * pixRandom)))
		{// Sway...
			texCoords += vec2(u_Local5.y * u_Local4.a * ((1.0 - m_TexCoords.y) + 1.0), 0.0);
		}
	#endif //!defined(USE_GLOW_BUFFER)

	viewDir = u_ViewOrigin.xyz - m_vertPos.xyz;

	//mat3 tangentToWorld = mat3(normalize(var_Tangent.xyz), normalize(var_Bitangent.xyz), normalize(m_Normal.xyz));
	//mat3 tangentToWorld = cotangent_frame(normalize(m_Normal.xyz), -viewDir, texCoords.xy);

	E = normalize(viewDir);


	#if defined(USE_LIGHTMAP) && !defined(USE_GLOW_BUFFER)

		vec4 lightmapColor = texture(u_LightMap, var_TexCoords2.st);

		#if defined(RGBM_LIGHTMAP)
			lightmapColor.rgb *= lightmapColor.a;
		#endif //defined(RGBM_LIGHTMAP)

	#endif //defined(USE_LIGHTMAP)


	vec2 ParallaxOffset = vec2(0.0);
	float displacement = 0.0;


#if defined(USE_PARALLAXMAP) && !defined(USE_GLOW_BUFFER) && !(defined(USE_MODELMATRIX) && !defined(USE_VERTEX_ANIMATION) && !defined(USE_SKELETAL_ANIMATION)) // MODELMATRIX causes screwed up normals...
	if (u_Local1.x > 0.0 && !isDistant)
	{
		#if defined(FAST_PARALLAX)

			//vec3 offsetDir = normalize(E * tangentToWorld);
			vec3 offsetDir = normalize((normalize(var_Tangent.xyz) * E.x) + (normalize(var_Bitangent.xyz) * E.y) + (normalize(m_Normal.xyz) * E.z));
			vec2 ParallaxXY = offsetDir.xy * tex_offset * 0.7;// u_Local1.x;

			ParallaxOffset = ParallaxXY * RayIntersectDisplaceMap(texCoords, displacement);
			texCoords += ParallaxOffset;

		#else //!defined(FAST_PARALLAX)
			//vec3 offsetDir = normalize(E * tangentToWorld);
			vec3 offsetDir = normalize((normalize(var_Tangent.xyz) * E.x) + (normalize(var_Bitangent.xyz) * E.y) + (normalize(m_Normal.xyz) * E.z));
			vec2 ParallaxXY = offsetDir.xy * tex_offset * u_Local1.x;

			// Steep Parallax
			float Step = 0.01;
			vec2 dt = ParallaxXY * Step;
			float Height = 0.5;
			float oldHeight = 0.5;
			vec2 Coord = texCoords;
			vec2 oldCoord = Coord;
			float HeightMap = GetDepth( Coord );
			float oldHeightMap = HeightMap;

			while( Height >= 0.0 && HeightMap < Height )
			{
				oldHeightMap = HeightMap;
				oldHeight = Height;
				oldCoord = Coord;

				Height -= Step;
				Coord += dt;
				HeightMap = GetDepth( Coord );
			}

			displacement = HeightMap;

			//Coord = (Coord + oldCoord)*0.5;
			if( Height < 0.0 )
			{
				Coord = oldCoord;
				Height = 0.0;
				displacement = oldHeightMap;
			}

			ParallaxOffset = texCoords - Coord;
			texCoords = Coord;

		#endif //defined(FAST_PARALLAX)
	}
	#endif //defined(USE_PARALLAXMAP)




	vec4 diffuse = GetDiffuse(texCoords, ParallaxOffset, pixRandom);


	#if defined(USE_GAMMA2_TEXTURES)
		diffuse.rgb *= diffuse.rgb;
	#endif


		/*if (diffuse.a <= 0.0)
		{
			discard; // no point going further??!?!?!
		}*/


	ambientColor = vec3(0.0);
	lightColor = var_Color.rgb;
	attenuation = 1.0;

	#if defined(USE_LIGHTMAP) && !defined(USE_GLOW_BUFFER)
		float lmBrightMult = clamp(1.0 - (length(lightmapColor.rgb) / 3.0), 0.0, 0.9);
		lmBrightMult *= lmBrightMult * 0.7;
		lightColor	= lightmapColor.rgb * lmBrightMult * var_Color.rgb;
	#endif


	/*
	#if defined(USE_SHADOWMAP) && !defined(USE_GLOW_BUFFER)

		vec2 shadowTex = gl_FragCoord.xy * r_FBufScale;
		float shadowValue = texture(u_ShadowMap, shadowTex).r;

	#endif //defined(USE_SHADOWMAP)
	*/



	//vec4 norm = vec4(m_Normal.xyz, 0.0);

	vec4 norm;

	if (u_Local4.r <= 0.0)
	{
		norm = ConvertToNormals(diffuse);
	}
	else
	{
		norm = GetNormal(texCoords, ParallaxOffset, pixRandom);
	}

	N.xy = norm.xy * 2.0 - 1.0;

	//N = CombineNormal(m_Normal.xyz * 0.5 + 0.5, norm.xyz, int(u_Local9.r));
	//N.xy *= u_NormalScale.xy;
	//N.xy *= 0.004;
	N.xy *= 0.25;
	N.z = sqrt(clamp((0.25 - N.x * N.x) - N.y * N.y, 0.0, 1.0));
	//N.z = sqrt(1.0 - dot(N.xy, N.xy));
	N = normalize((normalize(var_Tangent.xyz) * N.x) + (normalize(var_Bitangent.xyz) * N.y) + (normalize(m_Normal.xyz) * N.z));
	//N = normalize(tangentToWorld * N);

#if 0
	if (u_Local9.r >= 1)
	{
		if (u_Local9.r == 1)
			gl_FragColor = vec4(N.xyz * 0.5 + 0.5, 1.0);
		else if (u_Local9.r == 2)
			gl_FragColor = vec4(diffuse.rgb, 1.0);
		else if (u_Local9.r == 3)
			gl_FragColor = vec4(diffuse);
		else if (u_Local9.r == 4)
			gl_FragColor = vec4(var_Tangent.xyz * 0.5 + 0.5, 1.0);
		else if (u_Local9.r == 5)
			gl_FragColor = vec4(var_Bitangent.xyz * 0.5 + 0.5, 1.0);
		else if (u_Local9.r == 6)
			gl_FragColor = vec4(m_Normal.xyz * 0.5 + 0.5, 1.0);
		//gl_FragColor = vec4(N.a, N.a, N.a, 1.0);

#if defined(USE_GLOW_BUFFER)
		out_Glow = gl_FragColor;
#else
		out_Glow = vec4(0.0);
#endif

		out_Normal = vec4(m_Normal.xyz * 0.5 + 0.5, 0.2);
		out_Position = vec4(m_vertPos, u_Local1.a);/// MATERIAL_LAST);
		return;
	}
#endif

	/*if (diffuse.a <= 0.0)
	{
		discard; // no point going further??!?!?!
	}*/


	#if defined(USE_LIGHTMAP) && !defined(USE_GLOW_BUFFER)

		ambientColor = lightColor;
//#if defined(USE_TESSELLATION) || defined(USE_ICR_CULLING)
//		float surfNL = clamp(dot(var_PrimaryLightDir.xyz, N.xyz/*m_Normal.xyz*/), 0.0, 1.0);
//#else //!
		float surfNL = clamp(-dot(var_PrimaryLightDir.xyz, N.xyz/*m_Normal.xyz*/), 0.0, 1.0);
//#endif //defined(USE_TESSELLATION) || defined(USE_ICR_CULLING)
		lightColor /= max(surfNL, 0.25);
		ambientColor = clamp(ambientColor - lightColor * surfNL, 0.0, 1.0);
		lightColor *= lightmapColor.rgb;

	#endif //defined(USE_LIGHTMAP)

		gl_FragColor = vec4 (diffuse.rgb + (diffuse.rgb * ambientColor), diffuse.a * var_Color.a);

		if (u_Local1.a == 19 || u_Local1.a == 20)
		{// Tree billboards... Need to match tree colors...
			gl_FragColor.rgb = pow(gl_FragColor.rgb, vec3(1.3));
		}

		//specular.a = (1.0 - norm.a);
		specular.a = ((clamp(u_Local1.g, 0.0, 1.0) + clamp(u_Local3.a, 0.0, 1.0)) / 2.0) * 1.6;

		#if defined(USE_SPECULARMAP) && !defined(USE_GLOW_BUFFER)
		if (u_Local1.g != 0.0)
		{// Real specMap...
			specular = texture(u_SpecularMap, texCoords);
		}
		else
		#endif //defined(USE_SPECULARMAP)
		{// Fake it...
			specular.rgb = gl_FragColor.rgb;
		}

		#if defined(USE_GAMMA2_TEXTURES)
			specular.rgb *= specular.rgb;
		#endif //defined(USE_GAMMA2_TEXTURES)

		specular.rgb *= u_SpecularScale.rgb;

	#if defined(USE_CUBEMAP) && !defined(USE_GLOW_BUFFER) && !defined(USE_TRI_PLANAR) && !defined(USE_REGIONS)
		if (!isDistant && u_Local3.a > 0.0 && u_EnableTextures.w > 0.0 && u_CubeMapStrength > 0.0)
		{
			float spec = 0.0;

			//#if defined(USE_TESSELLATION) || defined(USE_ICR_CULLING)
			//	NE = clamp(dot(m_Normal.xyz/*N*/, -E), 0.0, 1.0);
			//#else
				NE = clamp(dot(m_Normal.xyz/*N*/, E), 0.0, 1.0);
			//#endif
			vec3 R = reflect(E, m_Normal.xyz/*N*/);
			vec3 reflectance = EnvironmentBRDF(clamp(specular.a, 0.5, 1.0) * 100.0, NE, specular.rgb);
			vec3 parallax = u_CubeMapInfo.xyz + u_CubeMapInfo.w * viewDir;
			vec3 cubeLightColor = textureCubeLod(u_CubeMap, R + parallax, 7.0 - specular.a * 7.0).rgb * u_EnableTextures.w * 0.25; //u_Local3.a
			gl_FragColor.rgb += (cubeLightColor * reflectance * (u_Local3.a * specular.a)) * u_CubeMapStrength * 0.5;
		}
	#endif

	/*
	#if defined(USE_SHADOWMAP) && !defined(USE_GLOW_BUFFER)
		//gl_FragColor.rgb *= clamp(shadowValue + 0.5, 0.0, 1.0);
		gl_FragColor.rgb *= clamp(shadowValue, 0.4, 1.0);
	#endif //defined(USE_SHADOWMAP)
	*/

	gl_FragColor.rgb *= lightColor;

	#if 0
		gl_FragColor = vec4(N.xyz, 1.0);
		//gl_FragColor = vec4(N.a, N.a, N.a, 1.0);

		#if defined(USE_GLOW_BUFFER)
			out_Glow = gl_FragColor;
		#else
			out_Glow = vec4(0.0);
		#endif

		out_Normal = vec4(m_Normal.xyz * 0.5 + 0.5, 0.2);
		out_Position = vec4(m_vertPos, u_Local1.a );/// MATERIAL_LAST);
		return;
	#endif

	#if defined(USE_GLOW_BUFFER)
		out_Glow = gl_FragColor;
		/*if (gl_FragColor.a >= 1.0)
		{// Only write to position/normal map when the alpha is solid, and drawing over the background surface completely.
			//out_Normal = vec4(N.xyz * 0.5 + 0.5, specular.a);
			out_Normal = vec4(m_Normal.xyz * 0.5 + 0.5, 0.05);
			out_Position = vec4(unOpenGlIsFuckedUpify(m_vertPos), u_Local1.a);
		}*/
	#else
		out_Glow = vec4(0.0);
		out_Normal = vec4(N.xyz * 0.5 + 0.5, specular.a);
		out_Position = vec4(unOpenGlIsFuckedUpify(m_vertPos), u_Local1.a);
	#endif
}
