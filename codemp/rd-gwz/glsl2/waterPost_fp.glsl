//#define SIMPLIFIED_FRESNEL		// Seems no faster... not as good?
#define REAL_WAVES					// You probably always want this turned on.
#define USE_UNDERWATER				// TODO: Convert from HLSL when I can be bothered.
#define USE_REFLECTION				// Enable reflections on water.
//#define HEIGHT_BASED_FOG			// Height based volumetric fog. Works but has some issues I can't be bothered fixing atm.
#define FIX_WATER_DEPTH_ISSUES		// Use basic depth value for sky hits...
//#define __TEST_WATER__				// Testing experimental water...

/*
heightMap – height-map used for waves generation as described in the section “Modifying existing geometry”
backBufferMap – current contents of the back buffer
positionMap – texture storing scene position vectors (material type is alpha)
waterPositionMap – texture storing scene water position vectors (alpha is 1.0 when water is at this pixel. 0.0 if not).
waterPositionMap2 – texture storing scene water position vectors (This one is at slightly increased height to include max wave height).
normalMap – texture storing normal vectors for normal mapping as described in the section “The computation of normal vectors”
foamMap – texture containing foam – in my case it is a photo of foam converted to greyscale
*/

uniform mat4		u_ModelViewProjectionMatrix;

uniform sampler2D	u_WaterHeightMap;
uniform sampler2D	u_DiffuseMap;			// backBufferMap
uniform sampler2D	u_PositionMap;
uniform sampler2D	u_NormalMap;
uniform sampler2D	u_OverlayMap;			// foamMap
uniform sampler2D	u_ScreenDepthMap;

uniform sampler2D	u_WaterPositionMap;
uniform sampler2D	u_WaterPositionMap2;

uniform vec4		u_Mins;					// MAP_MINS[0], MAP_MINS[1], MAP_MINS[2], 0.0
uniform vec4		u_Maxs;					// MAP_MAXS[0], MAP_MAXS[1], MAP_MAXS[2], 0.0
uniform vec4		u_MapInfo;				// MAP_INFO_SIZE[0], MAP_INFO_SIZE[1], MAP_INFO_SIZE[2], SUN_VISIBLE

uniform vec4		u_Local0;				// testvalue0, testvalue1, testvalue2, testvalue3
uniform vec4		u_Local1;				// MAP_WATER_LEVEL, USE_GLSL_REFLECTION, IS_UNDERWATER
uniform vec4		u_Local10;				// waveHeight, waveDensity

uniform vec2		u_Dimensions;
uniform vec4		u_ViewInfo;				// zmin, zmax, zmax / zmin

varying vec2		var_TexCoords;

uniform vec4		u_PrimaryLightOrigin;
uniform vec3		u_PrimaryLightColor;

// Position of the camera
uniform vec3		u_ViewOrigin;
#define ViewOrigin	u_ViewOrigin.xzy

// Timer
uniform float		u_Time;
#define systemtimer		(u_Time * 5000.0)


// Over-all water clearness...
//const float waterClarity = 0.001;
const float waterClarity = 0.03;
//float waterClarity = u_Local0.r;

// How fast will colours fade out. You can also think about this
// values as how clear water is. Therefore use smaller values (eg. 0.05f)
// to have crystal clear water and bigger to achieve "muddy" water.
const float fadeSpeed = 0.15;

// Normals scaling factor
const float normalScale = 1.0;

// R0 is a constant related to the index of refraction (IOR).
// It should be computed on the CPU and passed to the shader.
const float R0 = 0.5;

// Maximum waves amplitude
//const float waveHeight = 6.0;//4.0;
#define waveHeight u_Local10.r
#define waveDensity u_Local10.g

// Colour of the sun
vec3 sunColor = u_PrimaryLightColor.rgb;

// The smaller this value is, the more soft the transition between
// shore and water. If you want hard edges use very big value.
// Default is 1.0f.
const float shoreHardness = 0.2;//1.0;

// This value modifies current fresnel term. If you want to weaken
// reflections use bigger value. If you want to empasize them use
// value smaller then 0. Default is 0.0.
const float refractionStrength = 0.0;
//float refractionStrength = -0.3;

// Modifies 4 sampled normals. Increase first values to have more
// smaller "waves" or last to have more bigger "waves"
const vec4 normalModifier = vec4(1.0, 2.0, 4.0, 8.0);

// Describes at what depth foam starts to fade out and
// at what it is completely invisible. The third value is at
// what height foam for waves appear (+ waterLevel).
//const vec3 foamExistence = vec3(1.5, 5.35, 2.3); //vec3(0.65, 1.35, 0.5);
//const vec3 foamExistence = vec3(1.5, 50.0, waveHeight * 0.85/*5.0*/);
#define foamExistence vec3(1.5, 50.0, waveHeight * 0.85/*5.0*/)
//vec3 foamExistence = vec3(u_Local0.r, u_Local0.g, u_Local0.b);

const float sunScale = 3.0;

const float shininess = 0.7;
const float specularScale = 0.07;


// Colour of the water surface
const vec3 depthColour = vec3(0.0078, 0.5176, 0.7);

// Colour of the water depth
//const vec3 bigDepthColour = vec3(0.0059, 0.3096, 0.445);
const vec3 bigDepthColour = vec3(0.0059, 0.1276, 0.18);
//vec3 bigDepthColour = vec3(u_Local0.r, u_Local0.g, u_Local0.b);

//const vec3 extinction = vec3(7.0, 30.0, 40.0);			// Horizontal
//const vec3 extinction = vec3(7.0, 96.0, 128.0);			// Horizontal
const vec3 extinction = vec3(35.0, 480.0, 640.0);
//vec3 extinction = vec3(u_Local0.r, u_Local0.g, u_Local0.b);

// Water transparency along eye vector.
//const float visibility = 32.0;
const float visibility = 320.0;

// Increase this value to have more smaller waves.
const vec2 scale = vec2(0.002, 0.002);
const float refractionScale = 0.005;

// Wind force in x and z axes.
const vec2 wind = vec2(-0.3, 0.7);


float linearize(float depth)
{
	return clamp(1.0 / mix(u_ViewInfo.z, 1.0, depth), 0.0, 1.0);
}

mat3 compute_tangent_frame(in vec3 N, in vec3 P, in vec2 UV) {
    vec3 dp1 = dFdx(P);
    vec3 dp2 = dFdy(P);
    vec2 duv1 = dFdx(UV);
    vec2 duv2 = dFdy(UV);
    // solve the linear system
    vec3 dp1xdp2 = cross(dp1, dp2);
    mat2x3 inverseM = mat2x3(cross(dp2, dp1xdp2), cross(dp1xdp2, dp1));
    vec3 T = inverseM * vec2(duv1.x, duv2.x);
    vec3 B = inverseM * vec2(duv1.y, duv2.y);
    // construct tangent frame
    float maxLength = max(length(T), length(B));
    T = T / maxLength;
    B = B / maxLength;
    return mat3(T, B, N);
}

// Function calculating fresnel term.
// - normal - normalized normal vector
// - eyeVec - normalized eye vector
float fresnelTerm(vec3 normal, vec3 eyeVec)
{
#ifdef SIMPLIFIED_FRESNEL
		// Simplified
		return R0 + (1.0f - R0) * pow(1.0f - dot(eyeVec, normal), 5.0f);
#else //!SIMPLIFIED_FRESNEL
		float angle = 1.0f - clamp(dot(normal, eyeVec), 0.0, 1.0);
		float fresnel = angle * angle;
		fresnel = fresnel * fresnel;
		fresnel = fresnel * angle;
		return clamp(fresnel * (1.0 - clamp(R0, 0.0, 1.0)) + R0 - refractionStrength, 0.0, 1.0);
#endif //SIMPLIFIED_FRESNEL
}

vec4 waterMapAtCoord ( vec2 coord )
{
	vec4 wmap = texture2D(u_WaterPositionMap, coord).xzya;
	wmap.y -= waveHeight;
	return wmap;
}

vec4 waterMap2AtCoord ( vec2 coord )
{
	//return texture2D(u_WaterPositionMap2, coord).xzya;
	return texture2D(u_WaterPositionMap, coord).xzya;
}

//#define unOpenGlIsFuckedUpify(x) ( x / 524288.0 )

vec4 positionMapAtCoord ( vec2 coord )
{
	//return texture2D(u_PositionMap, coord).xzya;
	vec4 pos = texture2D(u_PositionMap, coord);
	pos.xyz *= 524288.0;
	return pos.xzya;
}

float pw = (1.0/u_Dimensions.x);
float ph = (1.0/u_Dimensions.y);

vec3 AddReflection(vec2 coord, vec3 positionMap, vec3 waterMap, vec3 inColor)
{
	if (positionMap.y > waterMap.y)
	{
		return inColor;
	}

	// Quick scan for pixel that is not water...
	float QLAND_Y = 0.0;

	for (float y = coord.y; y < 1.0; y += ph * 5.0)
	{
		vec4 wMap = waterMapAtCoord(vec2(coord.x, y));
		vec4 pMap = positionMapAtCoord(vec2(coord.x, y));
		float isWater = wMap.a;

		if (isWater <= 0.0 && (pMap.y >= waterMap.y || length(pMap.xyz) == 0.0))
		{
			QLAND_Y = y;
			break;
		}
	}

	if (QLAND_Y <= 0.0)
	{// Found no non-water surfaces...
		return inColor;
	}
	
	QLAND_Y -= ph * 5.0;
	
	// Full scan from within 5 px for the real 1st pixel...
	float upPos = coord.y;
	float LAND_Y = 0.0;

	for (float y = QLAND_Y; y < 1.0; y += ph)
	{
		vec4 wMap = waterMapAtCoord(vec2(coord.x, y));
		vec4 pMap = positionMapAtCoord(vec2(coord.x, y));
		float isWater = wMap.a;

		if (isWater <= 0.0 && (pMap.y >= waterMap.y || length(pMap.xyz) == 0.0))
		{
			LAND_Y = y;
			break;
		}
	}

	if (LAND_Y <= 0.0)
	{// Found no non-water surfaces...
		return inColor;
	}

	upPos = clamp(coord.y + ((LAND_Y - coord.y) * 2.0), 0.0, 1.0);

	if (upPos > 1.0 || upPos < 0.0)
	{// Not on screen...
		return inColor;
	}


	vec4 wMap = waterMapAtCoord(vec2(coord.x, upPos));

	if (wMap.a > 0.0)
	{// This position is water, or it is closer then the reflection pixel...
		return inColor;
	}

	vec4 landColor = texture2D(u_DiffuseMap, vec2(coord.x, upPos));
	landColor += texture2D(u_DiffuseMap, vec2(coord.x + pw, upPos));
	landColor += texture2D(u_DiffuseMap, vec2(coord.x - pw, upPos));
	landColor += texture2D(u_DiffuseMap, vec2(coord.x, upPos + ph));
	landColor += texture2D(u_DiffuseMap, vec2(coord.x, upPos - ph));
	landColor += texture2D(u_DiffuseMap, vec2(coord.x + pw, upPos + ph));
	landColor += texture2D(u_DiffuseMap, vec2(coord.x - pw, upPos - ph));
	landColor += texture2D(u_DiffuseMap, vec2(coord.x + pw, upPos - ph));
	landColor += texture2D(u_DiffuseMap, vec2(coord.x - pw, upPos + ph));
	landColor /= 9.0;

	return mix(inColor.rgb, landColor.rgb, vec3(1.0 - pow(upPos, 4.0)) * 0.28/*u_Local0.r*/);
}

vec3 applyFog2( in vec3  rgb,      // original color of the pixel
               in float distance, // camera to point distance
               in vec3  rayOri,   // camera position
               in vec3  rayDir,   // camera to point vector
               in vec3  sunDir )  // sun light direction
{
	const float b = 0.5;//0.7;//u_Local0.r; // the falloff of this density

#if defined(HEIGHT_BASED_FOG)
	float c = u_Local0.g; // height falloff

    float fogAmount = c * exp(-rayOri.z*b) * (1.0-exp( -distance*rayDir.z*b ))/rayDir.z; // height based fog
#else //!defined(HEIGHT_BASED_FOG)
	float fogAmount = 1.0 - exp( -distance*b );
#endif //defined(HEIGHT_BASED_FOG)

	fogAmount = clamp(fogAmount, 0.1, 1.0/*u_Local0.a*/);
	float sunAmount = max( clamp(dot( rayDir, sunDir )/**u_Local0.b*/, 0.0, 1.0), 0.0 );
	if (u_MapInfo.a <= 0.0) sunAmount = 0.0;
    vec3  fogColor  = mix( vec3(0.5,0.6,0.7), // bluish
                           vec3(1.0,0.9,0.7), // yellowish
                           pow(sunAmount,8.0) );
	return mix( rgb, fogColor, fogAmount );
}

#ifdef __TEST_WATER__
/*
"Seascape" by Alexander Alekseev aka TDM - 2014
License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.
Contact: tdmaav@gmail.com
*/

const int NUM_STEPS = 8;
const float PI = 3.1415;
const float EPSILON = 1e-3;
float EPSILON_NRM = 0.1 / u_Dimensions.x;

#define iGlobalTime u_Time

// sea
const int ITER_GEOMETRY = 3;
const int ITER_FRAGMENT = 5;
const float SEA_HEIGHT = 0.6;
const float SEA_CHOPPY = 4.0;
const float SEA_SPEED = 0.8;
const float SEA_FREQ = 0.16;
const vec3 SEA_BASE = vec3(0.1, 0.19, 0.22);
const vec3 SEA_WATER_COLOR = vec3(0.8, 0.9, 0.6);
float SEA_TIME = 1.0 + iGlobalTime * SEA_SPEED;
mat2 octave_m = mat2(1.6, 1.2, -1.2, 1.6);

// math
mat3 fromEuler(vec3 ang) {
	vec2 a1 = vec2(sin(ang.x), cos(ang.x));
	vec2 a2 = vec2(sin(ang.y), cos(ang.y));
	vec2 a3 = vec2(sin(ang.z), cos(ang.z));
	mat3 m;
	m[0] = vec3(a1.y*a3.y + a1.x*a2.x*a3.x, a1.y*a2.x*a3.x + a3.y*a1.x, -a2.y*a3.x);
	m[1] = vec3(-a2.y*a1.x, a1.y*a2.y, a2.x);
	m[2] = vec3(a3.y*a1.x*a2.x + a1.y*a3.x, a1.x*a3.x - a1.y*a3.y*a2.x, a2.y*a3.y);
	return m;
}
float hash(vec2 p) {
	float h = dot(p, vec2(127.1, 311.7));
	return fract(sin(h)*43758.5453123);
}
float noise(in vec2 p) {
	vec2 i = floor(p);
	vec2 f = fract(p);
	vec2 u = f*f*(3.0 - 2.0*f);
	return -1.0 + 2.0*mix(mix(hash(i + vec2(0.0, 0.0)),
		hash(i + vec2(1.0, 0.0)), u.x),
		mix(hash(i + vec2(0.0, 1.0)),
		hash(i + vec2(1.0, 1.0)), u.x), u.y);
}

// lighting
float diffuse(vec3 n, vec3 l, float p) {
	return pow(dot(n, l) * 0.4 + 0.6, p);
}
float specular(vec3 n, vec3 l, vec3 e, float s) {
	float nrm = (s + 8.0) / (3.1415 * 8.0);
	return pow(max(dot(reflect(e, n), l), 0.0), s) * nrm;
}

// sky
vec3 getSkyColor(vec3 e) {
	e.y = max(e.y, 0.0);
	vec3 ret;
	ret.x = pow(1.0 - e.y, 2.0);
	ret.y = 1.0 - e.y;
	ret.z = 0.6 + (1.0 - e.y)*0.4;
	return ret;
}

// sea
float sea_octave(vec2 uv, float choppy) {
	uv += noise(uv);
	vec2 wv = 1.0 - abs(sin(uv));
	vec2 swv = abs(cos(uv));
	wv = mix(wv, swv, wv);
	return pow(1.0 - pow(wv.x * wv.y, 0.65), choppy);
}

float map(vec3 p) {
	float freq = SEA_FREQ;
	float amp = SEA_HEIGHT;
	float choppy = SEA_CHOPPY;
	vec2 uv = p.xz; uv.x *= 0.75;

	float d, h = 0.0;
	for (int i = 0; i < ITER_GEOMETRY; i++) {
		d = sea_octave((uv + SEA_TIME)*freq, choppy);
		d += sea_octave((uv - SEA_TIME)*freq, choppy);
		h += d * amp;
		uv *= octave_m; freq *= 1.9; amp *= 0.22;
		choppy = mix(choppy, 1.0, 0.2);
	}
	return p.y - h;
}

float map_detailed(vec3 p) {
	float freq = SEA_FREQ;
	float amp = SEA_HEIGHT;
	float choppy = SEA_CHOPPY;
	vec2 uv = p.xz; uv.x *= 0.75;

	float d, h = 0.0;
	for (int i = 0; i < ITER_FRAGMENT; i++) {
		d = sea_octave((uv + SEA_TIME)*freq, choppy);
		d += sea_octave((uv - SEA_TIME)*freq, choppy);
		h += d * amp;
		uv *= octave_m; freq *= 1.9; amp *= 0.22;
		choppy = mix(choppy, 1.0, 0.2);
	}
	return p.y - h;
}

vec3 getSeaColor(vec3 p, vec3 n, vec3 l, vec3 eye, vec3 dist) {
	float fresnel = clamp(1.0 - dot(n, -eye), 0.0, 1.0);
	fresnel = pow(fresnel, 3.0) * 0.65;

	vec3 reflected = getSkyColor(reflect(eye, n));
	vec3 refracted = SEA_BASE + diffuse(n, l, 80.0) * SEA_WATER_COLOR * 0.12;

	vec3 color = mix(refracted, reflected, fresnel);

	float atten = max(1.0 - dot(dist, dist) * 0.001, 0.0);
	color += SEA_WATER_COLOR * (p.y - SEA_HEIGHT) * 0.18 * atten;

	color += vec3(specular(n, l, eye, 60.0));

	return color;
}

// tracing
vec3 getNormal(vec3 p, float eps) {
	vec3 n;
	n.y = map_detailed(p);
	n.x = map_detailed(vec3(p.x + eps, p.y, p.z)) - n.y;
	n.z = map_detailed(vec3(p.x, p.y, p.z + eps)) - n.y;
	n.y = eps;
	return normalize(n);
}

float heightMapTracing(vec3 ori, vec3 dir, out vec3 p) {
	float tm = 0.0;
	float tx = 1000.0;
	float hx = map(ori + dir * tx);
	if (hx > 0.0) return tx;
	float hm = map(ori + dir * tm);
	float tmid = 0.0;
	for (int i = 0; i < NUM_STEPS; i++) {
		tmid = mix(tm, tx, hm / (hm - hx));
		p = ori + dir * tmid;
		float hmid = map(p);
		if (hmid < 0.0) {
			tx = tmid;
			hx = hmid;
		}
		else {
			tm = tmid;
			hm = hmid;
		}
	}
	return tmid;
}

uniform mat4		u_ModelMatrix;

#define M_PI				3.14159265358979323846

float atan2(in float y, in float x)
{
	bool s = (abs(x) > abs(y));
	return mix(PI / 2.0 - atan(x, y), atan(y, x), s);
	//return x == 0.0 ? sign(y)*M_PI / 2 : atan(y, x)
}

// main
void mainImage(out vec4 fragColor, in vec2 fragCoord, in vec4 positionMap, in vec4 waterMap, in vec4 waterMap2, in vec3 screenCenterOrg)
{
	//vec2 uv = fragCoord.xy / u_Dimensions.xy;
	//uv = uv * 2.0 - 1.0;
	//uv.x *= u_Dimensions.x / u_Dimensions.y;
	vec2 uv = fragCoord.xy;
	uv = uv * 2.0 - 1.0;
	uv.x *= u_Dimensions.x / u_Dimensions.y;
	float time = iGlobalTime * 0.3;// +iMouse.x*0.01;

	// ray
	// bteitler: Calculated a vector that smoothly changes over time in a sinusoidal (wave) pattern.  
	// This will be used to drive where the user is looking in world space.
	//vec3 ang = vec3(0.0);//vec3(sin(time*3.0)*0.1,sin(time)*0.2+0.3,time);    
	//vec3 ang = vec3(0.0, iMouse.y * 0.1, iMouse.x * 0.1); // UQ1: This moves based on mouse x,y coords...
	//vec3 ang = vec3(0.0, (iMouse.y / iResolution.y) * PI, (iMouse.x / iResolution.x) * PI * 2.0); // UQ1: full 360 view available

	// bteitler: Calculate the "origin" of the camera in world space based on time.  Camera is located
	// at height 3.5, at x 0 (zero), and flies over the ocean in the z axis over time.
	//vec3 ori = vec3(0.0, 3.5, time*5.0);
	//vec3 ori = waterMap.xyz;
	//vec3 ori = (u_ModelViewProjectionMatrix * vec4(waterMap.xyz, 1.0)).xyz;
	
	//vec3 ori = vec3(iMouse.x * 0.1,13.5,iMouse.y * 0.1); // UQ1: This moves left/right and forward/back based on mouse...
	//vec3 ang = vec3(0.0, (iMouse.y / iResolution.y) * PI, (iMouse.x / iResolution.x) * PI * 2.0); // UQ1: full 360 view available
	
	//vec3 ori = vec3(u_ViewOrigin.x, u_ViewOrigin.y - waterMap.y, u_ViewOrigin.z) * u_Local0.a
	highp vec3 vOrg = (u_ViewOrigin.xyz + 524288.0);
	highp vec3 wOrg = (waterMap.xyz + 524288.0);
	//highp vec3 ori = vec3(u_ViewOrigin.x / u_Local0.a, (u_ViewOrigin.y - waterMap.y) / u_Local0.b, u_ViewOrigin.z / u_Local0.a);
	highp vec3 ori = vec3(vOrg.x / u_Local0.a, (vOrg.y - wOrg.y) / u_Local0.b, vOrg.z / u_Local0.a);
	
	//vec3 vOri = (u_ModelViewProjectionMatrix * vec4(u_ViewOrigin.xyz, 1.0)).xyz;
	//vec3 wOri = (u_ModelViewProjectionMatrix * vec4(waterMap.xyz, 1.0)).xyz;
	//vec3 ori = vec3(vOri.x, vOri.y - wOri.y, vOri.z) * u_Local0.a;

	vec3 vDir = normalize(u_ViewOrigin.xyz - waterMap.xyz) *0.5 + 0.5;
	vec3 ang = vec3(0.0, vDir.y * PI, vDir.x * PI * 2.0); // UQ1: full 360 view available
	//vec3 ang = vec3(0.0);

	if (u_Local0.r == 1) ang.y *= -1.0;
	if (u_Local0.r == 2) ang.z *= -1.0;

	vec3 dir = normalize(vec3(uv.xy, -2.0)); // commented version method...
	//vec3 dir = normalize(vec3(uv.xy, -2.0)); dir.z += length(uv) * 0.15;
	dir = normalize(dir) * fromEuler(ang);

	vec3 p;
	heightMapTracing(ori, dir, p);
	vec3 dist = p - ori;

	vec3 n = getNormal(p, dot(dist, dist) * EPSILON_NRM);
	vec3 light = normalize(vec3(0.0, 1.0, 0.8));

	// color
	vec3 color = mix(
		fragColor.rgb/*getSkyColor(dir)*/,
		getSeaColor(p, n, light, dir, dist),
		pow(smoothstep(0.0, -0.05, dir.y), 0.3));

	// post
	fragColor = vec4(pow(color, vec3(0.75)), 1.0);
}
#endif //__TEST_WATER__

// lighting
float getdiffuse(vec3 n, vec3 l, float p) {
	return pow(dot(n, l) * 0.4 + 0.6, p);
}
float getspecular(vec3 n, vec3 l, vec3 e, float s) {
	float nrm = (s + 8.0) / (3.1415 * 8.0);
	return pow(max(dot(reflect(e, n), l), 0.0), s) * nrm;
}

void main ( void )
{
	vec3 color2 = texture2D(u_DiffuseMap, var_TexCoords).rgb;
	vec4 waterMap2 = waterMap2AtCoord(var_TexCoords);
	bool IS_UNDERWATER = false;

	if (u_Local1.b > 0.0) 
	{
		IS_UNDERWATER = true;
	}

	if (waterMap2.a <= 0.0)
	{// Should be safe to skip everything.
		gl_FragColor = vec4(color2, 1.0);
		return;
	}

	bool pixelIsInWaterRange = false;
	bool pixelIsUnderWater = false;
	bool pixelIsWaterfall = false;
	vec3 color = color2;

	vec4 waterMap = waterMap2;//waterMapAtCoord(var_TexCoords);
	waterMap.y -= waveHeight;

	vec4 positionMap = positionMapAtCoord(var_TexCoords);
	vec3 position = positionMap.xyz;

#if defined(FIX_WATER_DEPTH_ISSUES)
	if (positionMap.a == 1024.0)
	{
		position.xyz = waterMap.xyz;
		position.y -= 1024.0;
	}
#endif //defined(FIX_WATER_DEPTH_ISSUES)

	if (waterMap.a >= 2.0 || waterMap2.a >= 2.0)
	{// Low horizontal normal, this is a waterfall...
		pixelIsWaterfall = true;
	}
	else if (/*waterMap.a > 0.0 ||*/ position.y <= waterMap.y)
	{
		pixelIsInWaterRange = true;
	}
	else if (IS_UNDERWATER || waterMap.y > ViewOrigin.y)
	{
		pixelIsUnderWater = true;
	}

	if (!pixelIsInWaterRange && !pixelIsWaterfall && !pixelIsUnderWater)
	{// No water here. Skip calculations.
		gl_FragColor = vec4(color2, 1.0);
		return;
	}

	float timer = systemtimer * (waveHeight / 16.0);

	if (pixelIsWaterfall)
	{// How???
		vec2 texCoord = var_TexCoords.xy;
		texCoord.x += sin(timer * 0.002 + 3.0 * abs(position.y)) * refractionScale;

		vec3 refraction = texture2D(u_DiffuseMap, texCoord).rgb;
		gl_FragColor = vec4(mix(depthColour, refraction, 0.7), 1.0);
		return;
	}

#ifdef __TEST_WATER__
	if (pixelIsInWaterRange || pixelIsUnderWater)
	{
		vec4 col2 = vec4(0.0);
		vec3 screenCenterOrg = positionMapAtCoord(vec2(0.0)).xzy;
		mainImage(col2, var_TexCoords, positionMap.xzya, waterMap.xzya, waterMap2.xzya, screenCenterOrg);
		color.rgb = col2.rgb;
	}
#else //!__TEST_WATER__

	float waterLevel = waterMap.y;
	float level = waterMap.y;
	float depth = 0.0;

	if (pixelIsInWaterRange || pixelIsUnderWater)
	{
		// Find intersection with water surface
		//vec3 eyeVecNorm = normalize(waterMap.xyz/*position*/ - ViewOrigin);
		vec3 eyeVecNorm = normalize(ViewOrigin - waterMap.xyz/*position*/);
		float t = ((level - ViewOrigin.y) / eyeVecNorm.y);
		vec3 surfacePoint = ViewOrigin + eyeVecNorm * t;
		//vec3 surfacePoint = waterMap.xyz;


		vec2 texCoord;
		//level = 0.0;

#ifdef REAL_WAVES
		for(int i = 0; i < 10; i++)
#endif //REAL_WAVES
		{
			texCoord = ((surfacePoint.xz + eyeVecNorm.xz * 0.1) * scale + timer * 0.000005 * wind) * waveDensity;
			
			float bias = texture2D(u_WaterHeightMap, texCoord).r;
	
			bias *= 0.1;
			level += bias * waveHeight;

			t = ((level - ViewOrigin.y) / eyeVecNorm.y);
			surfacePoint = ViewOrigin + eyeVecNorm * t;
			//surfacePoint = waterMap.xyz + vec3(0.0, level, 0.0);
			//surfacePoint = waterMap.xyz + (eyeVecNorm * level);
		}

		//level = surfacePoint.y;

		depth = length(position - surfacePoint);
		float depth2 = surfacePoint.y - position.y;
		float depthN = depth * fadeSpeed;

		if (pixelIsUnderWater)
		{
			//depth = -depth;
			//depth2 = -depth2;
			//depthN = depth * fadeSpeed;

			depth = length(surfacePoint - position);
			depth2 = position.y - surfacePoint.y;
			depthN = depth * fadeSpeed;

			//depth = length(position - ViewOrigin.xyz);
			//depthN = depth * fadeSpeed;
			//depth2 = level - ViewOrigin.y;

			if (position.y <= level)
			{// This pixel is below the water line... Fast path...
				vec3 waterCol = clamp(length(sunColor) / vec3(sunScale), 0.0, 1.0);
				waterCol = waterCol * mix(depthColour, bigDepthColour, clamp(depth2 / extinction, 0.0, 1.0));

				color2 = color2 - color2 * clamp(depth2 / extinction, 0.0, 1.0);
				color = mix(color2, waterCol, clamp(depthN / visibility, 0.0, 1.0));

				gl_FragColor = vec4(color, 1.0);
				return;
			}
		}
		else if (position.y > level)
		{// Waves against shoreline. Pixel is above waterLevel + waveHeight... (but ignore anything marked as actual water - eg: not a shoreline)
			gl_FragColor = vec4(color2, 1.0);
			return;
		}

		eyeVecNorm = normalize(ViewOrigin - /*waterMap2.xyz*/surfacePoint);

		float normal1 = texture2D(u_WaterHeightMap, (texCoord + (vec2(-1.0, 0.0) / 256.0))).r;
		float normal2 = texture2D(u_WaterHeightMap, (texCoord + (vec2(1.0, 0.0) / 256.0))).r;
		float normal3 = texture2D(u_WaterHeightMap, (texCoord + (vec2(0.0, -1.0) / 256.0))).r;
		float normal4 = texture2D(u_WaterHeightMap, (texCoord + (vec2(0.0, 1.0) / 256.0))).r;
		
		vec3 myNormal = normalize(vec3((normal1 - normal2) * waveHeight,
										   normalScale,
										   (normal3 - normal4) * waveHeight));

		texCoord = surfacePoint.xz * 1.6 + wind * timer * 0.00016;
		mat3 tangentFrame = compute_tangent_frame(myNormal, eyeVecNorm, texCoord);
		vec3 normal0a = normalize(tangentFrame * (2.0 * texture2D(u_NormalMap, texCoord).xyz - 1.0));

		texCoord = surfacePoint.xz * 0.8 + wind * timer * 0.00008;
		tangentFrame = compute_tangent_frame(myNormal, eyeVecNorm, texCoord);
		vec3 normal1a = normalize(tangentFrame * (2.0 * texture2D(u_NormalMap, texCoord).xyz - 1.0));
		
		texCoord = surfacePoint.xz * 0.4 + wind * timer * 0.00004;
		tangentFrame = compute_tangent_frame(myNormal, eyeVecNorm, texCoord);
		vec3 normal2a = normalize(tangentFrame * (2.0 * texture2D(u_NormalMap, texCoord).xyz - 1.0));
		
		texCoord = surfacePoint.xz * 0.1 + wind * timer * 0.00002;
		tangentFrame = compute_tangent_frame(myNormal, eyeVecNorm, texCoord);
		vec3 normal3a = normalize(tangentFrame * (2.0 * texture2D(u_NormalMap, texCoord).xyz - 1.0));
		
		vec3 normal = normalize(((normal0a * normalModifier.x) + (normal1a * normalModifier.y) +
						(normal2a * normalModifier.z) + (normal3a * normalModifier.w)));

		texCoord = var_TexCoords.xy;

		texCoord.x += sin(timer * 0.002 + 3.0 * abs(position.y)) * (refractionScale * min(depth2, 1.0));

		vec3 refraction = texture2D(u_DiffuseMap, texCoord).rgb;

		vec4 position2 = positionMapAtCoord(texCoord);
		vec4 waterMap3 = waterMapAtCoord(texCoord);

#if defined(FIX_WATER_DEPTH_ISSUES)
		if (position2.a == 1024.0)
		{
			position2.xyz = waterMap3.xyz;
			position2.y -= 1024.0;
		}
#endif //defined(FIX_WATER_DEPTH_ISSUES)

		if (!pixelIsUnderWater && position2.y > level)
		{
			refraction = color2;
		}

		float waterCol = clamp(length(sunColor) / sunScale, 0.0, 1.0);
		
		vec3 refraction1 = mix(refraction, depthColour * vec3(waterCol), clamp(vec3(depthN) / vec3(visibility), 0.0, 1.0));
		refraction = mix(refraction1, bigDepthColour * vec3(waterCol), clamp(vec3(depth2) / vec3(extinction), 0.0, 1.0));

		vec4 foam = vec4(0.0);

		texCoord = (surfacePoint.xz + eyeVecNorm.xz * 0.1) * 0.05 + timer * 0.00001 * wind + sin(timer * 0.001 + position.x) * 0.005;
		vec2 texCoord2 = (surfacePoint.xz + eyeVecNorm.xz * 0.1) * 0.05 + timer * 0.00002 * wind + sin(timer * 0.001 + position.z) * 0.005;
		
		
		if (depth2 < foamExistence.x)
		{
			foam = (texture2D(u_OverlayMap, texCoord) + texture2D(u_OverlayMap, texCoord2)) * 0.5;
		}
		else if (depth2 < foamExistence.y)
		{
			foam = mix((texture2D(u_OverlayMap, texCoord) + texture2D(u_OverlayMap, texCoord2)) * 0.5, vec4(0.0),
						 vec4((depth2 - foamExistence.x) / (foamExistence.y - foamExistence.x)));
		}
		
		
		if (waveHeight - foamExistence.z > 0.0001)
		{
			foam += (texture2D(u_OverlayMap, texCoord) + texture2D(u_OverlayMap, texCoord2)) * 0.5 * 
				clamp((level - (waterLevel + foamExistence.z)) / (waveHeight - foamExistence.z), 0.0, 1.0);
		}
		

		vec3 specular = vec3(0.0);

		vec3 lightDir = normalize(ViewOrigin.xyz - u_PrimaryLightOrigin.xzy);
		//normal *= -myNormal.xyz;

		float lambertian2 = dot(lightDir.xyz, normal);
		float spec2 = 0.0;
		
#if 1
		float fresnel = clamp(1.0 - dot(normal, -eyeVecNorm), 0.0, 1.0);
		fresnel = pow(fresnel, 3.0) * 0.65;
#else
		float fresnel = fresnelTerm(normal, eyeVecNorm);
#endif

		if(lambertian2 > 0.0)
		{// this is blinn phong
			vec3 mirrorEye = (2.0 * dot(eyeVecNorm, normal) * normal - eyeVecNorm);
			vec3 halfDir2 = normalize(lightDir.xyz + mirrorEye);
			float specAngle = max(dot(halfDir2, normal), 0.0);
			spec2 = pow(specAngle, 16.0);
			specular = vec3(clamp(1.0 - fresnel, 0.4, 1.0)) * (vec3(spec2 * shininess)) * sunColor * specularScale * 25.0;
		}


#if 1
		/* TESTING */
		vec3 dist = -eyeVecNorm;

		color = mix(refraction, bigDepthColour, fresnel);

		float atten = max(1.0 - dot(dist, dist) * 0.001, 0.0);
		color += depthColour.rgb * (clamp(waveHeight - waterMap.y, 0.0, 1.0))* 0.18 * atten;

		color += vec3(getspecular(normal, lightDir, eyeVecNorm, 60.0));
		/* END - TESTING */
#else
		color = mix(refraction, bigDepthColour, fresnel);
#endif

#if defined(USE_REFLECTION)
		if (!pixelIsUnderWater && u_Local1.g >= 2.0)
		{
			color = AddReflection(var_TexCoords, position, vec3(waterMap3.x, level, waterMap3.z), color.rgb);
		}
#endif //defined(USE_REFLECTION)

		color = clamp(color + max(specular, foam.rgb * sunColor), 0.0, 1.0);
		
		color = mix(refraction, color, clamp(depth * shoreHardness, 0.0, 1.0));

		color = mix(color, color2, 1.0 - clamp(waterClarity * depth, 0.8, 1.0));

		/*if (position.y > level && waterMap.a <= 0.0)
		{// Waves against shoreline. Pixel is above waterLevel + waveHeight... (but ignore anything marked as actual water - eg: not a shoreline)
			color = color2;
		}
		else*/
		{
			float depthMap = linearize(texture2D(u_ScreenDepthMap, var_TexCoords).r);//length(u_ViewOrigin.xyz - position.xzy);
			color = applyFog2( color.rgb, depthMap, u_ViewOrigin.xyz/*position.xzy*/, normalize(u_ViewOrigin.xyz - position.xzy), normalize(u_ViewOrigin.xyz - u_PrimaryLightOrigin.xyz) );
		}
	}
#endif //__TEST_WATER__

	gl_FragColor = vec4(color, 1.0);
}
