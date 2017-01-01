uniform sampler2D u_DiffuseMap;

uniform vec4		u_ViewInfo; // zmin, zmax, zmax / zmin
uniform vec2		u_Dimensions;

varying vec2		var_TexCoords;

#define BLUR_RADIUS 2.0

#define px (1.0/u_Dimensions.x)
#define py (1.0/u_Dimensions.y)

#define RADIUS_X (BLUR_RADIUS * px)
#define RADIUS_Y (BLUR_RADIUS * py)

float linearize(float depth)
{
	return 1.0 / mix(u_ViewInfo.z, 1.0, depth);
}

vec4 FastBlur(void)
{
	vec4 color = texture2D(u_DiffuseMap, var_TexCoords.xy);

	int NUM_BLUR_PIXELS = 1;

	for (float x = -RADIUS_X; x <= RADIUS_X; x += px)
	{
		for (float y = -RADIUS_Y; y <= RADIUS_Y; y += py)
		{
			color.rgb += texture2D(u_DiffuseMap, vec2(var_TexCoords.x + x, var_TexCoords.y + y)).rgb;
			NUM_BLUR_PIXELS++;
		}
	}

	color.rgb /= NUM_BLUR_PIXELS;
	return color;
}

void main()
{
	gl_FragColor = FastBlur();
}
