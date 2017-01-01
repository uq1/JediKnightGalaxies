uniform sampler2D	u_DiffuseMap;
uniform sampler2D	u_DeluxeMap;
uniform sampler2D	u_ScreenDepthMap;
uniform sampler2D	u_PositionMap;

#define				MAX_VLIGHTS 16

uniform int			u_lightCount;
uniform vec2		u_vlightPositions[MAX_VLIGHTS];
uniform float		u_vlightDistances[MAX_VLIGHTS];
uniform vec3		u_vlightColors[MAX_VLIGHTS];

uniform vec4		u_Local0;
uniform vec4		u_ViewInfo; // zmin, zmax, zmax / zmin, SUN_ID

varying vec2		var_TexCoords;
flat varying int	var_SunVisible;


//#define USING_ENGINE_GLOW_LIGHTCOLORS_SEARCH // Enable when I fix it...

//#define NO_FALLOFF
//#define NO_SUN_FALLOFF
//#define EXPERIMENTAL_SHADOWS
//#define ADJUST_DEPTH_PIXELS // Gives more shadow effect to the lighting, but also can make it too intense...

#define VOLUMETRIC_THRESHOLD 0.15


#if defined(HQ_VOLUMETRIC)
const float	iBloomraySamples = 32.0;
const float	fBloomrayDecay = 0.96875;
#elif defined (MQ_VOLUMETRIC)
const float	iBloomraySamples = 16.0;
const float	fBloomrayDecay = 0.9375;
#else //!defined(HQ_VOLUMETRIC) && !defined(MQ_VOLUMETRIC)
const float	iBloomraySamples = 8.0;
const float	fBloomrayDecay = 0.875;
#endif //defined(HQ_VOLUMETRIC) && defined(MQ_VOLUMETRIC)

const float	fBloomrayWeight = 0.5;
const float	fBloomrayDensity = 1.0;
const float fBloomrayFalloffRange = 0.4;//0.5;//0.4;

float linearize(float depth)
{
	return (1.0 / mix(u_ViewInfo.z, 1.0, depth));
}

void main ( void )
{
	vec4 diffuseColor = texture2D(u_DiffuseMap, var_TexCoords.xy);
	int  SUN_ID = int(u_ViewInfo.a);

	//gl_FragColor = vec4(texture2D(u_DeluxeMap, vec2(var_TexCoords.x, 1.0 - var_TexCoords.y)).rgb, 1.0);
	//return;

	if (u_lightCount <= 0)
	{
#ifdef DUAL_PASS
		gl_FragColor = vec4(0.0);
#else //!DUAL_PASS
		gl_FragColor = diffuseColor;
#endif //DUAL_PASS
		return;
	}

	//float depth = 1.0 - linearize(texture2D(u_ScreenDepthMap, var_TexCoords).r);
	//gl_FragColor = vec4(depth, depth, depth, 1.0);
	//return;

	vec2		inRangePositions[16];
	vec3		lightColors[16];
#ifndef NO_FALLOFF
	float		fallOffRanges[16];
#else
	int			isSun = -1;
#endif
	float		lightDepths[16];
	int			numInRange = 0;

	for (int i = 0; i < MAX_VLIGHTS/*u_lightCount*/; i++) // MAX_VLIGHTS to use constant loop size
	{
		if (i >= u_lightCount) continue;

		float dist = length(var_TexCoords - u_vlightPositions[i]);
		//float depth = 1.0 - linearize(texture2D(u_ScreenDepthMap, u_vlightPositions[i]).r);
		float depth = 1.0 - u_vlightDistances[i];

#ifndef NO_FALLOFF
		float fall = clamp((fBloomrayFalloffRange * depth) - dist, 0.0, 1.0) * depth;

		if (i == SUN_ID) 
		{
			if (var_SunVisible <= 0) continue;

#ifdef NO_SUN_FALLOFF
			fall = 1.0; // none
#else
			fall *= 4.0;
#endif
		}
#endif

#ifndef NO_FALLOFF
		if (fall > 0.0)
		{
#endif
			inRangePositions[numInRange] = u_vlightPositions[i];
			lightDepths[numInRange] = depth;
#ifndef NO_FALLOFF
			fallOffRanges[numInRange] = (fall + (fall*fall)) / 2.0;
#endif
			lightColors[numInRange] = u_vlightColors[i];

			if (lightColors[numInRange].r == 0.0 && lightColors[numInRange].g == 0.0 && lightColors[numInRange].b == 0.0)
			{// When no color is set, skip adding this light... Should never happen, but just in case (for speed)...
				continue;
			}

#ifdef USING_ENGINE_GLOW_LIGHTCOLORS_SEARCH
			if (lightColors[numInRange].r < -1.0 && lightColors[numInRange].g < -1.0 && lightColors[numInRange].b < -1.0)
			{// < 1.0 means no average color was found by game code. Need to fallback to glow map lookups...
				vec3 spotColor = texture2D(u_DeluxeMap, vec2(inRangePositions[numInRange].x, 1.0 - inRangePositions[numInRange].y)).rgb;

				if (length(spotColor) > VOLUMETRIC_THRESHOLD)
				{
					if (length(spotColor) <= 1.0)
					{
						spotColor *= 4.0;
					}
					else if (length(spotColor) <= 2.0)
					{
						spotColor *= 2.5;
					}

					lightColors[numInRange] = -(spotColor + 0.5); // + 0.5 to make sure all .rgb are negative...
				}
				else
				{// Not bright enough for a volumetric light...
					continue;
				}
			}
			else if (lightColors[numInRange].r < 0.0 && lightColors[numInRange].g < 0.0 && lightColors[numInRange].b < 0.0)
			{// Game sent us a light color for this texture, use it...
				vec3 spotColor = -lightColors[numInRange];

				if (length(spotColor) > VOLUMETRIC_THRESHOLD)
				{
					if (length(spotColor) <= 1.0)
					{
						spotColor *= 4.0;
					}
					else if (length(spotColor) <= 2.0)
					{
						spotColor *= 2.5;
					}

					lightColors[numInRange] = -(spotColor + 0.5); // + 0.5 to make sure all .rgb are negative...
				}
				else
				{// Not bright enough for a volumetric light...
					continue;
				}
			}
#else //!USING_ENGINE_GLOW_LIGHTCOLORS_SEARCH
			//if (lightColors[numInRange].r < 0.0 && lightColors[numInRange].g < 0.0 && lightColors[numInRange].b < 0.0)
			{// < 1.0 means no average color was found by game code. Need to fallback to glow map lookups...
				vec3 spotColor = texture2D(u_DeluxeMap, vec2(inRangePositions[numInRange].x, 1.0 - inRangePositions[numInRange].y)).rgb;

				if (length(spotColor) > VOLUMETRIC_THRESHOLD)
				{
					if (length(spotColor) <= 1.0)
					{
						spotColor *= 4.0;
					}
					else if (length(spotColor) <= 2.0)
					{
						spotColor *= 2.5;
					}

					lightColors[numInRange] = -(spotColor + 0.5); // + 0.5 to make sure all .rgb are negative...
					lightColors[numInRange] *= 0.22;
				}
				else
				{// Not bright enough for a volumetric light...
					continue;
				}
			}
#endif //USING_ENGINE_GLOW_LIGHTCOLORS_SEARCH

			if (length(lightColors[numInRange]) > VOLUMETRIC_THRESHOLD)
			{// Only use it if it is not a dark pixel...
#ifdef NO_FALLOFF
				if (i == SUN_ID) 
				{
					isSun = numInRange;
				}
#endif

				numInRange++;
			}
		}
#ifndef NO_FALLOFF
	}
#endif

	if (numInRange <= 0)
	{// Nothing in range...
#ifdef DUAL_PASS
		gl_FragColor = vec4(0.0);
#else //!DUAL_PASS
		gl_FragColor = diffuseColor;
#endif //DUAL_PASS
		return;
	}

	vec4 totalColor = vec4(0.0, 0.0, 0.0, 0.0);

	for (int i = 0; i < MAX_VLIGHTS/*numInRange*/; i++) // MAX_VLIGHTS to use constant loop size
	{
		if (i >= numInRange) continue;

		vec4	lens = vec4(0.0, 0.0, 0.0, 0.0);
		vec2	ScreenLightPos = inRangePositions[i];
		vec2	texCoord = var_TexCoords;
		float	lightDepth = lightDepths[i];
		vec2	deltaTexCoord = (texCoord.xy - ScreenLightPos.xy);
		//int		samples = int(float(iBloomraySamples) * lightDepth);

		//deltaTexCoord *= 1.0 / float(samples * fBloomrayDensity);
		deltaTexCoord *= 1.0 / float(iBloomraySamples * fBloomrayDensity);

		float illuminationDecay = 1.0;

		//for(int g = 0; g < int(samples); g++)
		for(int g = 0; g < int(iBloomraySamples); g++)
		{
			texCoord -= deltaTexCoord;

			float linDepth = linearize(texture2D(u_ScreenDepthMap, texCoord.xy).r);

#ifdef ADJUST_DEPTH_PIXELS
			//if (linDepth < lightDepths[i]) linDepth = 0.0;
			//else linDepth = 1.0;
			if (linDepth >= lightDepths[i]) linDepth = 1.0;
#endif //ADJUST_DEPTH_PIXELS

			float sample2 = linDepth * illuminationDecay * fBloomrayWeight;

			if (lightColors[i].r < 0.0 && lightColors[i].g < 0.0 && lightColors[i].b < 0.0)
			{// This is a map glow. Since we have no color value, look at the pixel...
				lens.xyz += (sample2 * 0.5) * -(lightColors[i] + 0.5) * 1.5;//u_Local0.r; // + 0.5 to undo + 0.5 above...
			}
			else
			{
				lens.xyz += (sample2 * 0.5) * lightColors[i];
			}

			illuminationDecay *= fBloomrayDecay;

			if (illuminationDecay <= 0.0)
				break;
		}

#ifdef NO_FALLOFF
		if (i == isSun)
		{
			totalColor += clamp((lens * lightDepth), 0.0, 1.0);
		}
		else
		{
			totalColor += clamp((lens * lightDepth) * 0.03/*0.03*/, 0.0, 1.0);
		}
#else
		totalColor += clamp((lens * lightDepth) * fallOffRanges[i], 0.0, 1.0);
#endif
	}

#ifdef DUAL_PASS
#ifdef NO_FALLOFF
#ifdef EXPERIMENTAL_SHADOWS
	if (isSun != -1 && max(max(totalColor.r, totalColor.g), totalColor.b) <= u_Local0.r)
	{
		totalColor.a = 0.0;
	}
	else
#endif
	{
		totalColor.a = 1.0;
	}
#else
	totalColor.a = 1.0;
#endif

	gl_FragColor = totalColor;
#else //!DUAL_PASS

#ifdef EXPERIMENTAL_SHADOWS
	if (isSun != -1 && max(max(totalColor.r, totalColor.g), totalColor.b) <= u_Local0.r)
	{
		diffuseColor.rgb *= vec3(0.2);
	}
#endif

	totalColor.a = 0.0;

	gl_FragColor = diffuseColor + totalColor;
#endif //DUAL_PASS
}
