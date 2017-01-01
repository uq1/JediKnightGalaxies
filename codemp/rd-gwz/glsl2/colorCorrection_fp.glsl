uniform sampler2D	u_DiffuseMap;
uniform sampler2D	u_DeluxeMap;
uniform sampler2D	u_GlowMap;

varying vec2		var_TexCoords;

void main (void)
{
	vec4 color = texture2D(u_DiffuseMap, var_TexCoords);

	color.rgb = clamp(color.rgb, 0.0, 1.0);

	vec3 brightness = texture2D(u_GlowMap, vec2(0.5)).rgb; //adaptation luminance
	brightness = (brightness/(brightness+1.0));
	brightness = vec3(max(brightness.x, max(brightness.y, brightness.z)));

	vec3	palette;
	vec2	uvsrc = vec2(0.0);

	uvsrc.x = color.r;
	uvsrc.y = brightness.r;
	palette.r = texture2D(u_DeluxeMap, uvsrc).r;
	uvsrc.x = color.g;
	uvsrc.y = brightness.g;
	palette.g = texture2D(u_DeluxeMap, uvsrc).g;
	uvsrc.x = color.b;
	uvsrc.y = brightness.b;
	palette.b = texture2D(u_DeluxeMap, uvsrc).b;
	color.rgb = (color.rgb + (palette.rgb * color.rgb)) / 2.0;

	gl_FragColor = vec4(color.rgb, 1.0);
}
