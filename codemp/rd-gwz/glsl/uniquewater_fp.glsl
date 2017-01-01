#define USE_WATERMAP

uniform vec4		u_Local6; // blah, blah, blah, MAP_WATER_LEVEL
uniform vec4		u_Local9;
uniform vec4		u_Local10;

varying vec2		var_TexCoords;
varying vec3		var_vertPos;
varying vec3		var_Normal;

out vec4 out_Glow;
out vec4 out_Normal;
out vec4 out_Position;

// Maximum waves amplitude
#define maxAmplitude u_Local10.g

vec3 vectoangles( in vec3 value1 ) {
	float	forward;
	float	yaw, pitch;
	vec3	angles;

	if ( value1.g == 0 && value1.r == 0 ) {
		yaw = 0;
		if ( value1.b > 0 ) {
			pitch = 90;
		}
		else {
			pitch = 270;
		}
	}
	else {
		if ( value1.r > 0 ) {
			yaw = ( atan ( value1.g, value1.r ) * 180 / M_PI );
		}
		else if ( value1.g > 0 ) {
			yaw = 90;
		}
		else {
			yaw = 270;
		}
		if ( yaw < 0 ) {
			yaw += 360;
		}

		forward = sqrt ( value1.r*value1.r + value1.g*value1.g );
		pitch = ( atan(value1.b, forward) * 180 / M_PI );
		if ( pitch < 0 ) {
			pitch += 360;
		}
	}

	angles.r = -pitch;
	angles.g = yaw;
	angles.b = 0.0;

	return angles;
}

void main()
{
	out_Glow = vec4(0.0);
	out_Normal = vec4(var_Normal * 0.5 + 0.5, 0.75);
#if defined(USE_WATERMAP)
	float isWater = 1.0;
	out_Color = vec4(0.0059, 0.3096, 0.445, 0.5);

	//if (u_Local9.r >= 3.0 && var_Normal.r > 0.1) isWater = 2.0;
	//else if (u_Local9.r >= 2.0 && var_Normal.g > 0.1) isWater = 2.0;
	//else if (u_Local9.r >= 1.0 && var_Normal.b > 0.1) isWater = 2.0;

	float pitch = vectoangles( var_Normal.xyz ).r;
	
	if (pitch > 180)
		pitch -= 360;

	if (pitch < -180)
		pitch += 360;

	pitch += 90.0f;

	if (pitch < 0.0) pitch = -pitch;

	if (pitch > 16.0/*u_Local9.r*//*46.0*/) isWater = 2.0;
			
	out_Position = vec4(var_vertPos.xyz, isWater);
#else //!defined(USE_WATERMAP)
	out_Color = vec4(0.0059, 0.3096, 0.445, 0.1);
	out_Position = vec4(var_vertPos, 13.0);// 13.0 / MATERIAL_LAST);
#endif //defined(USE_WATERMAP)
}
