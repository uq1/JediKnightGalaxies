uniform sampler2D	u_DiffuseMap;
uniform sampler2D	u_NormalMap;

uniform vec2		u_Dimensions;

varying vec2		var_ScreenTex;

//#define DEBUG
#define BLUR_WIDTH	8.0

void main()
{
#if defined(DEBUG)
	gl_FragColor = vec4(texture2D(u_NormalMap, var_ScreenTex).rgb, 1.0);
#else //!defined(DEBUG)
	vec4 color = texture2D(u_DiffuseMap, var_ScreenTex);
	/*vec3 hbao = texture2D(u_NormalMap, var_ScreenTex).rgb;
	gl_FragColor = vec4(color.rgb * (hbao.rgb * 0.7), color.a);*/

	float NUM_VALUES = 1.0;
	vec2 PIXEL_OFFSET = vec2(1.0 / u_Dimensions);
	vec2 texcoord = var_ScreenTex;
	//texcoord.x += PIXEL_OFFSET.x;

	float hbao0 = texture2D(u_NormalMap, texcoord.xy).r;
	float hbao = hbao0;

	for (float width = -BLUR_WIDTH; width <= BLUR_WIDTH; width += 1.0)
	{
		//float dist_mult = clamp(1.0 - (width / BLUR_WIDTH), 0.333, 1.0);
		float hbao1 = texture2D(u_NormalMap, texcoord.xy + (width * PIXEL_OFFSET)).r;
		float hbao2 = texture2D(u_NormalMap, texcoord.xy - (width * PIXEL_OFFSET)).r;
		float hbao3 = texture2D(u_NormalMap, texcoord.xy + (width * vec2(PIXEL_OFFSET.x, -PIXEL_OFFSET.y))).r;
		float hbao4 = texture2D(u_NormalMap, texcoord.xy + (width * vec2(-PIXEL_OFFSET.x, PIXEL_OFFSET.y))).r;
		float hbao5 = texture2D(u_NormalMap, texcoord.xy + (width * vec2(PIXEL_OFFSET.x, 0.0))).r;
		float hbao6 = texture2D(u_NormalMap, texcoord.xy + (width * vec2(0.0, PIXEL_OFFSET.y))).r;
		float hbao7 = texture2D(u_NormalMap, texcoord.xy + (width * vec2(-PIXEL_OFFSET.x, 0.0))).r;
		float hbao8 = texture2D(u_NormalMap, texcoord.xy + (width * vec2(0.0, -PIXEL_OFFSET.y))).r;
		float ao = clamp(((/*hbao0 +*/ hbao1 + hbao2 + hbao2 + hbao4 + hbao5 + hbao6 + hbao7 + hbao8) /** dist_mult*/) / 8.0/*9.0*/, 0.333, 1.0);

		hbao += ao;
		NUM_VALUES += 1.0;
	}

	hbao /= NUM_VALUES;
	hbao = clamp(hbao, 0.75, 1.0);

	gl_FragColor = vec4((color.rgb * hbao), color.a);
#endif //defined(DEBUG)
}
