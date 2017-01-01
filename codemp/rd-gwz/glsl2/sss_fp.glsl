uniform sampler2D				u_DiffuseMap;
uniform sampler2D				u_ScreenDepthMap;
uniform sampler2D				u_NormalMap;
uniform sampler2D				u_PositionMap;

uniform mat4					u_ModelViewProjectionMatrix;
uniform mat4					u_invEyeProjectionMatrix;
uniform vec4					u_ViewInfo; // zmin, zmax, zmax / zmin
uniform vec3					u_ViewOrigin;
uniform vec4					u_LightOrigin;
uniform vec4					u_Local0;

varying vec2					var_ScreenTex;
varying vec2					var_Dimensions;


float linearize(float depth)
{
	return 1.0 / mix(u_ViewInfo.z, 1.0, depth);
}

vec3 WorldSpacePosition ( vec2 coord )
{
	return texture2D(u_PositionMap, coord).xyz;
}

float DepthAtCoordinate ( vec2 coord )
{
	return clamp(distance(WorldSpacePosition(coord), u_ViewOrigin) / u_ViewInfo.y, 0.0, 1.0);
}

void main(void){
	//gl_FragColor = vec4(texture2D(u_NormalMap, var_ScreenTex).rgb, 1.0);
	//gl_FragColor = texture2D(u_PositionMap, var_ScreenTex);
	//gl_FragColor = vec4(vec3(linearize(texture2D(u_ScreenDepthMap, var_ScreenTex).r)), 1.0);
	//gl_FragColor = vec4(texture2D(u_PositionMap, var_ScreenTex).rgb, 1.0);
	//float fDepth = DepthAtCoordinate(var_ScreenTex);
	//gl_FragColor = vec4(vec3(fDepth), 1.0);
	//return;

	vec3 dglow = texture2D(u_NormalMap, var_ScreenTex).rgb;
	float dglowStrength = clamp(length(dglow.rgb) * 3.0, 0.0, 1.0);

	vec2 texel_size = vec2(1.0 / var_Dimensions);
	
	int material_type = int(texture2D(u_PositionMap, var_ScreenTex).a);// * MATERIAL_LAST);
	float isFoliage = 0.0;
	if (material_type == 5 || material_type == 6) isFoliage = 1.0; // MATERIAL_SHORTGRASS or MATERIAL_LONGGRASS
	//gl_FragColor = vec4(vec3(float(material_type) / MATERIAL_LAST), 1.0);
	//gl_FragColor = vec4(vec3(isFoliage), 1.0);
	//return;

	vec4 diffuse = texture2D(u_DiffuseMap, var_ScreenTex);
	vec4 oDiffuse = diffuse;

	float depth = DepthAtCoordinate(var_ScreenTex);//texture2D(u_ScreenDepthMap, var_ScreenTex).r;
	//depth = linearize(depth);

	float invDepth = 1.0 - depth;

	vec2 distFromCenter = vec2((0.5 - var_ScreenTex.x) * 2.0, (1.0 - var_ScreenTex.y) * 0.5);
	vec2 pixOffset = clamp((distFromCenter * invDepth) * texel_size * 80.0, 0.0 - (texel_size * 80.0), texel_size * 80.0);
	vec2 pos = var_ScreenTex + pixOffset;

	//vec2 distFromCenter = vec2((0.5 - var_ScreenTex.x) * 2.0, (1.0 - var_ScreenTex.y) * 0.5);
	//vec2 pixOffset = clamp((distFromCenter * invDepth) * texel_size * 80.0, vec2(0.0), texel_size * 80.0);
	//vec2 pos = var_ScreenTex + pixOffset;

	int material_type2 = int(texture2D(u_PositionMap, pos).a);// * MATERIAL_LAST);
	float isFoliage2 = 0.0;
	if (material_type2 == 5 || material_type2 == 6) isFoliage2 = 1.0; // MATERIAL_SHORTGRASS or MATERIAL_LONGGRASS

	float d2 = DepthAtCoordinate(pos);//texture2D(u_ScreenDepthMap, pos).r;
	//d2 = linearize(d2);

	float depthDiff = clamp(depth - d2, 0.0, 1.0);

	if (isFoliage > 0.0 && isFoliage2 > 0.0)
	{
		if (depthDiff > u_Local0.r/*0.0001*/ && depthDiff < u_Local0.g)
		{
			vec3 shadow = diffuse.rgb * 0.25;
			shadow += diffuse.rgb * (0.75 * (depthDiff / u_Local0.g)); // less darkness at higher distance for blending
			float invDglow = 1.0 - dglowStrength;
			diffuse.rgb = (diffuse.rgb * dglowStrength) + (shadow * invDglow);
		}
		else if (depthDiff < u_Local0.r/*0.0001*/)
		{
			vec3 shadow = diffuse.rgb * 0.25;
			shadow += diffuse.rgb * (0.75 * (1.0 - (depthDiff / u_Local0.r/*0.0001*/))); // less darkness at lower distance for blending
			float invDglow = 1.0 - dglowStrength;
			diffuse.rgb = (diffuse.rgb * dglowStrength) + (shadow * invDglow);
		}
	}

	/*
	if (length(diffuse.rgb) > length(oDiffuse.rgb))
	{// Shadow would be lighter due to blending, use original...
		diffuse.rgb = oDiffuse.rgb;
	}
	*/

	gl_FragColor = vec4(diffuse.rgb, 1.0);
}
