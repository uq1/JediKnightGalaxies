attribute vec3		attr_Position;
attribute vec4		attr_TexCoord0;

uniform sampler2D	u_PositionMap;

uniform mat4		u_ModelViewProjectionMatrix;

uniform vec2		u_Dimensions;
uniform vec4		u_ViewInfo; // zmin, zmax, zmax / zmin, SUN_ID

uniform int			u_lightCount;
uniform vec2		u_vlightPositions[16];

varying vec2		var_TexCoords;
flat varying int	var_SunVisible;

vec2 pixel = 1.0 / u_Dimensions;

bool ScanForSunMaterial(vec2 coord)
{
	float material = texture(u_PositionMap, coord).a;

	if (material == 1024.0 || material == 1025.0)
		return true;

	bool found = false;

	for (int y = -10; y < 10; y++)
	{
		if (found) break;

		material = texture(u_PositionMap, coord + vec2(0.0, pixel.y * float(y) * 2.0)).a;

		if (material == 1024.0 || material == 1025.0)
			found = true;

		material = texture(u_PositionMap, coord + vec2(pixel.x * 2.0, pixel.y * float(y) * 2.0)).a;

		if (material == 1024.0 || material == 1025.0)
			found = true;

		material = texture(u_PositionMap, coord + vec2(pixel.x * -2.0, pixel.y * float(y) * 2.0)).a;

		if (material == 1024.0 || material == 1025.0)
			found = true;
	}
	
	/*
	for (int y = -50; y < 50; y += 2.0)
	{
		if (found) break;

		for (int x = -50; x < 50; x += 2.0)
		{
			if (found) break;

			material = texture(u_PositionMap, coord + vec2(pixel.x * float(x), pixel.y * float(y))).a;

			if (material == 1024.0 || material == 1025.0)
				found = true;
		}
	}
	*/

	return found;
}

void main()
{
	gl_Position = u_ModelViewProjectionMatrix * vec4(attr_Position, 1.0);
	var_TexCoords = attr_TexCoord0.st;

	// Find if the sun is visible on screen or not. Saves doing scans per pixel...
	int  SUN_ID = int(u_ViewInfo.a);

	var_SunVisible = 0;

	if (SUN_ID >= 0 && SUN_ID < 16)
	{
		if (ScanForSunMaterial(u_vlightPositions[SUN_ID])) 
			var_SunVisible = 1;
	}
}
