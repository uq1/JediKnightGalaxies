uniform sampler2D	u_DiffuseMap;

varying vec2		var_DiffuseTex;
varying vec4		var_Color;
varying vec3		var_Normal;
varying vec3		var_VertPos;

out vec4 out_Glow;
/*out vec4 out_Position;
out vec4 out_Normal;

#define unOpenGlIsFuckedUpify(x) ( x / 524288.0 )*/

void main()
{
	vec4 color = texture2D(u_DiffuseMap, var_DiffuseTex);
	color = texture2D(u_DiffuseMap, var_DiffuseTex) * var_Color;
	
	gl_FragColor = color;

	//gl_FragColor.rgb = vec3(1.0, 0.0, 0.0);

	#if defined(USE_GLOW_BUFFER)
		out_Glow = gl_FragColor;
	#else
		out_Glow = vec4(0.0);
	#endif

	//out_Normal = vec4(var_Normal.xyz * 0.5 + 0.5, 0.05);
	//out_Position = vec4(var_VertPos, 0.0);

	/*if (gl_FragColor.a >= 1.0 && length(gl_FragColor.rgb) > 0.0)
	{// Only write to position/normal map when the alpha is solid, and drawing over the background surface completely.
		out_Normal = vec4(var_Normal.xyz * 0.5 + 0.5, 0.05);
		out_Position = vec4(unOpenGlIsFuckedUpify(var_VertPos), 0.0);//4.0);//MATERIAL_HOLLOWMETAL u_Local1.a);
	}*/
}
