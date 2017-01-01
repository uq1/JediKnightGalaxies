//#define HEIGHT_BASED_FOG

uniform sampler2D	u_DiffuseMap;
uniform sampler2D	u_ScreenDepthMap;
uniform sampler2D	u_PositionMap;

uniform vec4		u_ViewInfo; // zmin, zmax, zmax / zmin
uniform vec2		u_Dimensions;

uniform vec4		u_Local0;		// testvalue0, testvalue1, testvalue2, testvalue3
uniform vec4		u_MapInfo;		// MAP_INFO_SIZE[0], MAP_INFO_SIZE[1], MAP_INFO_SIZE[2], SUN_VISIBLE

uniform vec3		u_ViewOrigin;
uniform vec4		u_PrimaryLightOrigin;

varying vec2		var_TexCoords;

#define unOpenGlIsFuckedUpify(x) ( x / 524288.0 )

vec4 positionMapAtCoord ( vec2 coord )
{
	return texture2D(u_PositionMap, coord).xyza;
}

float linearize(float depth)
{
	return clamp(1.0 / mix(u_ViewInfo.z, 1.0, depth), 0.0, 1.0);
}

vec3 applyFog2( in vec3  rgb,      // original color of the pixel
               in float distance, // camera to point distance
               in vec3  rayOri,   // camera position
               in vec3  rayDir,   // camera to point vector
               in vec3  sunDir,
			   in vec4 position )  // sun light direction
{
	const float b = 0.5;//0.7;//u_Local0.r; // the falloff of this density

#if defined(HEIGHT_BASED_FOG)
	float c = u_Local0.g; // height falloff

    float fogAmount = c * exp(-rayOri.z*b) * (1.0-exp( -distance*rayDir.z*b ))/rayDir.z; // height based fog
#else //!defined(HEIGHT_BASED_FOG)
	float fogAmount = 1.0 - exp( -distance*b );
#endif //defined(HEIGHT_BASED_FOG)

	fogAmount = clamp(fogAmount, 0.1, 1.0/*u_Local0.a*/);
	float sunAmount = max( clamp(dot( rayDir, sunDir )*1.1, 0.0, 1.0), 0.0 );
	
	//if (u_MapInfo.a <= 0.0) sunAmount = 0.0;
	if (!(position.a == 1024.0 || position.a == 1025.0))
	{// Not Skybox or Sun... No don't do sun color here...
		sunAmount = 0.0;
	}

    vec3  fogColor  = mix( vec3(0.5,0.6,0.7), // bluish
                           vec3(1.0,0.9,0.7), // yellowish
                           pow(sunAmount,8.0) );

	return mix( rgb, fogColor, fogAmount );
}

void main ( void )
{
	vec4 pMap = positionMapAtCoord( var_TexCoords );
	vec4 pixelColor = texture2D(u_DiffuseMap, var_TexCoords);
	float depth = linearize(texture2D(u_ScreenDepthMap, var_TexCoords).r);
	vec3 viewOrg = unOpenGlIsFuckedUpify(u_ViewOrigin.xyz);
	vec3 sunOrg = unOpenGlIsFuckedUpify(u_PrimaryLightOrigin.xyz);
	vec3 fogColor = applyFog2(pixelColor.rgb, depth, viewOrg.xyz/*pMap.xyz*/, normalize(viewOrg.xyz - pMap.xyz), normalize(viewOrg.xyz - sunOrg.xyz), pMap);
	gl_FragColor = vec4(fogColor, 1.0);
}
