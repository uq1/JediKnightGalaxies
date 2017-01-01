attribute vec3		attr_Position;
attribute vec3		attr_Normal;

flat out	int		isSlope;

#define M_PI				3.14159265358979323846

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

bool SlopeTooGreat(vec3 normal)
{
	float pitch = vectoangles( normal.xyz ).r;
	
	if (pitch > 180)
		pitch -= 360;

	if (pitch < -180)
		pitch += 360;

	pitch += 90.0f;

	if (pitch < 0.0) pitch = -pitch;

	if (pitch > 46.0)
	{
		return true; // This slope is too steep for grass...
	}

	return false;
}

void main()
{
	isSlope = 0;

	if (SlopeTooGreat(attr_Normal.xyz * 2.0 - 1.0))
		isSlope = 1;

	gl_Position = vec4(attr_Position.xyz, 1.0);
}
