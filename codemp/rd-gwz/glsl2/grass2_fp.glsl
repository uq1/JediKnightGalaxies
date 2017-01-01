//#define USE_PHONG_LIGHTING_ON_GRASS

uniform sampler2D	u_DiffuseMap;
uniform sampler2D	u_SplatMap1;
uniform sampler2D	u_SplatMap2;
uniform sampler2D	u_OverlayMap;

/*
#if defined(USE_SHADOWMAP)
uniform sampler2D	u_ShadowMap;
#endif
*/

uniform mat4		u_ModelViewProjectionMatrix;
uniform mat4		u_ModelMatrix;
uniform mat4		u_invEyeProjectionMatrix;
uniform mat4		u_ModelViewMatrix;

uniform vec4		u_Local9;

uniform vec3		u_ViewOrigin;
uniform vec4		u_PrimaryLightOrigin;
uniform vec3		u_PrimaryLightColor;

flat in int			iGrassType;
smooth in vec2		vTexCoord;
in vec3				vVertPosition;

out vec4			out_Glow;
out vec4			out_Normal;
out vec4			out_Position;

vec4 ConvertToNormals ( vec4 color )
{
	// This makes silly assumptions, but it adds variation to the output. Hopefully this will look ok without doing a crapload of texture lookups or
	// wasting vram on real normals.
	//
	// UPDATE: In my testing, this method looks just as good as real normal maps. I am now using this as default method unless r_normalmapping >= 2
	// for the very noticable FPS boost over texture lookups.

	//N = vec3((color.r + color.b) / 2.0, (color.g + color.b) / 2.0, (color.r + color.g) / 2.0);
	vec3 N = vec3(clamp(color.r + color.b, 0.0, 1.0), clamp(color.g + color.b, 0.0, 1.0), clamp(color.r + color.g, 0.0, 1.0));
	N.xy = 1.0 - N.xy;
	vec4 norm = vec4(N, 1.0 - (length(N.xyz) / 3.0));
	return norm;
}

void main() 
{
	vec4 diffuse;

	if (iGrassType >= 3)
		diffuse = texture(u_OverlayMap, vTexCoord);
	else if (iGrassType >= 2)
		diffuse = texture(u_SplatMap2, vTexCoord);
	else if (iGrassType >= 1)
		diffuse = texture(u_SplatMap1, vTexCoord);
	else
		diffuse = texture(u_DiffuseMap, vTexCoord);

	if (diffuse.a <= 0.0) discard;

	diffuse.rgb *= clamp((1.0-vTexCoord.y) * 1.5, 0.3, 1.0);
	if (diffuse.a > 0.5) diffuse.a = 1.0;
	else diffuse.a = 0.0;

	if (diffuse.a <= 0.0) discard;

	/*
	#if defined(USE_SHADOWMAP)

		vec2 shadowTex = gl_FragCoord.xy * r_FBufScale;
		float shadowValue = texture2D(u_ShadowMap, shadowTex).r;
		diffuse.rgb *= clamp(shadowValue, 0.4, 1.0);

	#endif //defined(USE_SHADOWMAP)
	*/

	gl_FragColor = diffuse;

	vec4 m_Normal = ConvertToNormals(diffuse);

#if defined(USE_PHONG_LIGHTING_ON_GRASS)
	vec3 lightDir = /*vVertPosition.xyz*/u_ViewOrigin.xyz - u_PrimaryLightOrigin.xyz;
	lightDir.xy = -lightDir.xy;

	float lambertian2 = dot(-lightDir.xyz, -m_Normal.xyz);

	if(lambertian2 > 0.0)
	{// this is blinn phong
		vec3 eyeVecNorm = normalize(vVertPosition - u_ViewOrigin);
		vec3 mirrorEye = (2.0 * dot(eyeVecNorm, -m_Normal.xyz) * -m_Normal.xyz - eyeVecNorm);
		vec3 halfDir2 = normalize(-lightDir.xyz + mirrorEye);
		float specAngle = max(dot(halfDir2, -m_Normal.xyz), 0.0);
		float spec2 = pow(specAngle, 16.0);
		gl_FragColor.rgb += vec3(spec2 * (1.0 - m_Normal.a)) * gl_FragColor.rgb * u_PrimaryLightColor.rgb;
	}
	else
	{
		gl_FragColor.rgb *= u_PrimaryLightColor.rgb;
	}
#endif //defined(USE_PHONG_LIGHTING_ON_GRASS)

	out_Glow = vec4(0.0);
	out_Normal = vec4(m_Normal.xyz, 0.1);
	out_Position = vec4(vVertPosition, 6.0);//0.1875); // 6.0 / MATERIAL_LAST (0.1875) is MATERIAL_LONGGRASS
}
