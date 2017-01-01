uniform sampler2D u_DiffuseMap;
uniform sampler2D u_ScreenDepthMap;
uniform sampler2D u_NormalMap;
uniform sampler2D u_DeluxeMap; // actually saturation map image

varying vec2		var_TexCoords;
varying vec2		var_Dimensions;
varying vec4		var_ViewInfo; // zmin, zmax, zmax / zmin

vec3 CalculateFlare ( vec3 flare_color, vec3 final_color )
{
	float mult = clamp((3.0 - (flare_color.r + flare_color.g + flare_color.b)) + 0.1, 0.0, 3.0);
	vec3 add_flare = clamp(flare_color.rgb * mult, 0.0, 1.0);
#if 0
#define const_1 ( 12.0 / 255.0)
#define const_2 (255.0 / 219.0)
	add_flare = ((clamp(add_flare - const_1, 0.0, 1.0)) * const_2);
#endif
	return clamp(((add_flare * add_flare * 0.5) + 1.0) * final_color, 0.0, 1.0);
}

void main()
{   
	// Fast (just color bleed) mode...
	vec3 final_color = texture2D(u_DiffuseMap, var_TexCoords.st).xyz;// * 1.25;
		
	// UQ1: Let's add some of the flare color as well... Just to boost colors/glows...
	vec3 flare_color = clamp(texture2D(u_DeluxeMap, var_TexCoords.st).rgb, 0.0, 1.0);
	vec3 add_flare = CalculateFlare(flare_color, final_color);
	final_color = clamp(((final_color * 5.0) + max(add_flare, final_color)) / 6.0, 0.0, 1.0);

	gl_FragColor = vec4(final_color,1.0);
}
