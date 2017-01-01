uniform sampler2D	u_DiffuseMap;
uniform sampler2D	u_NormalMap;

uniform vec2		u_Dimensions;

varying vec2		var_ScreenTex;

// Enable output Debugging...
//#define DEBUG

// Shall we blur the result?
#define BLUR_WIDTH 1.0

// Shall we pixelize randomly the output? -- Sucks!
//#define RANDOMIZE_PIXELS

//#define APPLY_CONTRAST
//#define APPLY_CONTRAST2
//#define EXPERIMENTAL_SHADOWS

#ifdef RANDOMIZE_PIXELS
//noise producing function to eliminate banding (got it from someone else´s shader):
float rand(vec2 co){
	return 0.5+(fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453))*0.5;
}
#endif

void main()
{
	vec4 diffuseColor = texture2D(u_DiffuseMap, var_ScreenTex);
#ifdef RANDOMIZE_PIXELS
	float random = rand(var_ScreenTex);
#endif

#if defined(BLUR_WIDTH)
	vec2 offset = vec2(1.0) / u_Dimensions;

	vec4 pixelLight = texture2D(u_NormalMap, var_ScreenTex);
	vec3 volumeLight = pixelLight.rgb;
	
	for (float i = 1.0; i < BLUR_WIDTH+1.0; i+=1.0)
	{
		volumeLight += texture2D(u_NormalMap, var_ScreenTex + (offset * vec2(i, 0.0))).rgb;
		volumeLight += texture2D(u_NormalMap, var_ScreenTex + (offset * vec2(0.0, i))).rgb;
		volumeLight += texture2D(u_NormalMap, var_ScreenTex + (offset * vec2(i, i))).rgb;
		volumeLight += texture2D(u_NormalMap, var_ScreenTex + (offset * vec2(-i, 0.0))).rgb;
		volumeLight += texture2D(u_NormalMap, var_ScreenTex + (offset * vec2(0.0, -i))).rgb;
		volumeLight += texture2D(u_NormalMap, var_ScreenTex + (offset * vec2(-i, -i))).rgb;
	}

	volumeLight /= ((6.0 * (BLUR_WIDTH+1.0)) + 1.0);
#else
	vec3 volumeLight = texture2D(u_NormalMap, var_ScreenTex).rgb;
#endif

#ifdef APPLY_CONTRAST
#define const_1 ( 18.0 / 255.0)
#define const_2 (255.0 / 248.0)
	volumeLight.rgb = clamp((clamp(volumeLight.rgb - const_1, 0.0, 1.0)) * const_2, 0.0, 1.0);
#endif
#ifdef APPLY_CONTRAST2
	float contrast = clamp(((volumeLight.r + volumeLight.g + volumeLight.b) / 1.5) + 0.25, 0.25, 1.5);
	//diffuseColor.rgb *= contrast;
	volumeLight.rgb *= contrast;
#endif

#ifdef DEBUG
	gl_FragColor.rgb = volumeLight;
	gl_FragColor.a = 1.0;
	return;
#endif

#ifdef RANDOMIZE_PIXELS
	gl_FragColor = diffuseColor + vec4(volumeLight*random, 0.0);
#else

#ifdef EXPERIMENTAL_SHADOWS
	if (pixelLight.a <= 0.0)
	{
		diffuseColor.rgb *= 0.2;
		gl_FragColor = diffuseColor + vec4(volumeLight, 0.0);
	}
	else
	{
		gl_FragColor = diffuseColor + vec4(volumeLight, 0.0);
	}
#else
	gl_FragColor = vec4(diffuseColor.rgb + volumeLight, 1.0);
#endif
#endif	
}
