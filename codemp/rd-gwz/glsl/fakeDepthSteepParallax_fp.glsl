uniform sampler2D			u_TextureMap;
uniform sampler2D			u_NormalMap;
uniform sampler2D			u_PositionMap;

uniform vec3				u_ViewOrigin;
uniform vec2				u_Dimensions;
uniform vec4				u_Local0; // depthScale, depthMultiplier, 0, 0

varying vec2				var_TexCoords;

#define depthScale			u_Local0.r
#define depthMultiplier		u_Local0.g

#define pixelSize			(1.0 / u_Dimensions.xy)

vec3 vLocalSeed;

#define unOpenGlIsFuckedUpify(x) ( x / 524288.0 )

// This function returns random number from zero to one
float randZeroOne()
{
    uint n = floatBitsToUint(vLocalSeed.y * 214013.0 + vLocalSeed.x * 2531011.0 + vLocalSeed.z * 141251.0);
    n = n * (n * n * 15731u + 789221u);
    n = (n >> 9u) | 0x3F800000u;
 
    float fRes =  2.0 - uintBitsToFloat(n);
    vLocalSeed = vec3(vLocalSeed.x + 147158.0 * fRes, vLocalSeed.y*fRes  + 415161.0 * fRes, vLocalSeed.z + 324154.0*fRes);
    return fRes;
}

float GetHeight(vec2 Coord)
{
	return texture2D( u_NormalMap, Coord ).a;
}

mat3 cotangent_frame( vec3 N, vec3 p, vec2 uv )
{
	// get edge vectors of the pixel triangle
	vec3 dp1 = dFdx( p );
	vec3 dp2 = dFdy( p );
	vec2 duv1 = dFdx( uv );
	vec2 duv2 = dFdy( uv );

	// solve the linear system
	vec3 dp2perp = cross( dp2, N );
	vec3 dp1perp = cross( N, dp1 );
	vec3 T = dp2perp * duv1.x + dp1perp * duv2.x;
	vec3 B = dp2perp * duv1.y + dp1perp * duv2.y;

	// construct a scale-invariant frame 
	float invmax = inversesqrt( max( dot(T,T), dot(B,B) ) );
	return mat3( T * invmax, B * invmax, N );
}

// Transfers val from the range val0->val1 to the range res0->res1
float blend(float val, float val0, float val1, float res0, float res1) {
	if (val <= val0)
		return res0;
	if (val >= val1)
		return res1;

	return res0 + (val - val0) * (res1 - res0) / (val1 - val0);
}

void main (void)
{
	//gl_FragColor = vec4(texture2D( u_NormalMap, var_TexCoords ).rgb, 1.0);
	//return;
#if 1

	vec4 positionMap = texture2D( u_PositionMap, var_TexCoords.xy );

	//if (positionMap.a == 0.0 || positionMap.a == 1024.0)
	/*if (positionMap.a != 5.0 && positionMap.a != 6.0) // grass material
	{
		gl_FragColor = texture2D( u_TextureMap, var_TexCoords.xy );
		return;
	}*/

	vec4 coPos = positionMap;
	//#define unOpenGlIsFuckedUpify(x) ( x / 524288.0 ) // FIXME: If I ever finish this...
	float dist = distance(u_ViewOrigin.xyz, coPos.xyz);
	float depth = 1.0 - clamp(dist / 2048.0, 0.0, 1.0);

	if (positionMap.a == 1024.0)
	{
		depth = 0.0;
		dist = 512000.0;
	}

	vec4 norm = texture2D( u_NormalMap, var_TexCoords.xy );
	norm.xyz = norm.xyz * 2.0 - 1.0;
	vec3 viewDir = normalize(u_ViewOrigin.xyz - positionMap.xyz);
	mat3 tangentToWorld = cotangent_frame(norm.xyz, -viewDir, var_TexCoords.xy);
	vec3 E = normalize(viewDir.xyz);

	vec3 offsetDir = normalize(E * tangentToWorld);
	vec2 ParallaxXY = offsetDir.xy * pixelSize * depth * depthScale;

	// Steep Parallax
	float Step = 0.01;
	vec2 dt = ParallaxXY * Step;
	float Height = 0.0;//0.5;
	float oldHeight = 0.0;//-0.5;
	vec2 Coord = var_TexCoords.xy;
	vec2 oldCoord = Coord;
	float HeightMap = norm.a * depth;
	float oldHeightMap = HeightMap;
	//float material = positionMap.a;
	float origDist = dist;

	while( HeightMap >= Height && (/*material != positionMap.a ||*/ origDist >= dist) )
	{
		coPos = texture2D( u_PositionMap, Coord );
		//material = coPos.a;
		dist = distance(u_ViewOrigin.xyz, coPos.xyz);

		depth = 1.0 - clamp(dist / 2048.0, 0.0, 1.0);

		if (coPos.a == 1024.0)
		{
			depth = 0.0;
			dist = 512000.0;
		}
		
		if (/*material != positionMap.a &&*/ origDist < dist)
		{
			break;
		}

		oldHeightMap = HeightMap;
		oldHeight = Height;
		oldCoord = Coord;

		Height += Step;
		Coord -= dt;
		HeightMap = GetHeight(Coord) * depth;
	}

	//Coord = (Coord + oldCoord)*0.5;
	if( Height < 0.0 )
	{
		Coord = oldCoord;
		Height = 0.0;
	}

	gl_FragColor = texture2D( u_TextureMap, Coord );

#elif 0

	bool isGrass = true;
	vec2 dpos = var_TexCoords.xy;

#if UNITY_UV_STARTS_AT_TOP
	dpos.y = 1 - dpos.y;
#endif

	vec4 positionMap = texture2D( u_PositionMap, var_TexCoords.xy );

	if (positionMap.a != 5.0 && positionMap.a != 6.0)
	{
		isGrass = false;
	}

	float depth = distance(u_ViewOrigin.xyz, positionMap.xyz);
	vec4 backcolor = texture2D(u_TextureMap, var_TexCoords.xy);

	if (isGrass)
	{
		vec3 up_vec = vec3(0.0, 1.0, 0.0);
		vec4 color = vec4(0, 0, 0, 0);

		vec2 p = dpos;

		float d = blend(depth, 0, 500, 100, 500);
		float dclose = blend(depth, 0, 20, 30, 1);

		d *= dclose;

		p.y += p.x * 1009 + p.x * 1259 + p.x * 2713;
		//p.y += _Time.y * 0.004; // wind

		float yoffset = fract(p.y * d) / d;
		yoffset *= depthScale;

#if UNITY_UV_STARTS_AT_TOP
		vec2 uvoffset = var_TexCoords.xy + up_vec.xy * yoffset;
		vec2 uvoffset_d = dpos.xy - up_vec.xy * yoffset;
#else
		vec2 uvoffset = var_TexCoords.xy + up_vec.xy * yoffset;
		vec2 uvoffset_d = dpos.xy + up_vec.xy * yoffset;
#endif

		color = texture2D(u_TextureMap, uvoffset);
		float depth2 = distance(u_ViewOrigin.xyz, texture2D( u_PositionMap, uvoffset_d.xy).xyz);

		if (depth2 < depth)
		{
			gl_FragColor = backcolor;
			return;
		}

		gl_FragColor = mix(backcolor, color, clamp(1 - yoffset * d / 3.8, 0.0, 1.0));
		return;
	}

	gl_FragColor = backcolor;

#endif
}
