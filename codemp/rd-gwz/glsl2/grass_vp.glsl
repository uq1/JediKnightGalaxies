#extension GL_EXT_gpu_shader4 : enable

#define WAVE

//#define USE_DEFORM_VERTEXES
//#define USE_TCGEN
//#define USE_TCMOD

attribute vec3 attr_Position;
attribute vec3 attr_Normal;

attribute vec4 attr_Color;
attribute vec2 attr_TexCoord0;

#if defined(USE_LIGHTMAP) || defined(USE_TCGEN)
attribute vec2 attr_TexCoord1;
#endif

uniform vec4   u_DiffuseTexMatrix;
uniform vec4   u_DiffuseTexOffTurb;

#if defined(USE_TCGEN)
uniform vec3   u_LocalViewOrigin;
#endif

#if defined(USE_TCGEN)
uniform int    u_TCGen0;
uniform vec3   u_TCGen0Vector0;
uniform vec3   u_TCGen0Vector1;
#endif

#if defined(USE_DEFORM_VERTEXES)
uniform int    u_DeformGen;
uniform float  u_DeformParams[5];
#endif

uniform mat4	u_ModelViewProjectionMatrix;
uniform mat4	u_ModelMatrix;
uniform mat4	u_invEyeProjectionMatrix;

varying vec2   var_DiffuseTex;
varying vec4   var_Color;

uniform vec2	u_Dimensions;
uniform vec3	u_ViewOrigin;
uniform float	u_Time;
uniform vec4	u_Local5; // grassLayer, grassLength, wavespeed, wavesize

#define m_Layer		u_Local5.r
#define m_Length	u_Local5.g

#ifdef WAVE
bool	m_Wave = true;
float	m_WaveSpeed = u_Local5.b;
float	m_WaveSize = u_Local5.a;
#endif

varying vec2	var_TexCoords;
varying vec2	var_Dimensions;
varying vec3	var_vertPos;
varying float	var_Time;
varying vec3	var_Normal;
varying vec3	var_ViewDir;


#if defined(USE_DEFORM_VERTEXES)
vec3 DeformPosition(const vec3 pos, const vec3 normal, const vec2 st)
{
	float base =      u_DeformParams[0];
	float amplitude = u_DeformParams[1];
	float phase =     u_DeformParams[2];
	float frequency = u_DeformParams[3];
	float spread =    u_DeformParams[4];

	if (u_DeformGen == DGEN_BULGE)
	{
		phase *= st.x;
	}
	else // if (u_DeformGen <= DGEN_WAVE_INVERSE_SAWTOOTH)
	{
		phase += dot(pos.xyz, vec3(spread));
	}

	float value = phase + (u_Time * frequency);
	float func;

	if (u_DeformGen == DGEN_WAVE_SIN)
	{
		func = sin(value * 2.0 * M_PI);
	}
	else if (u_DeformGen == DGEN_WAVE_SQUARE)
	{
		func = sign(fract(0.5 - value));
	}
	else if (u_DeformGen == DGEN_WAVE_TRIANGLE)
	{
		func = abs(fract(value + 0.75) - 0.5) * 4.0 - 1.0;
	}
	else if (u_DeformGen == DGEN_WAVE_SAWTOOTH)
	{
		func = fract(value);
	}
	else if (u_DeformGen == DGEN_WAVE_INVERSE_SAWTOOTH)
	{
		func = (1.0 - fract(value));
	}
	else // if (u_DeformGen == DGEN_BULGE)
	{
		func = sin(value);
	}

	return pos + normal * (base + func * amplitude);
}
#endif

#if defined(USE_TCGEN)
vec2 GenTexCoords(int TCGen, vec3 position, vec3 normal, vec3 TCGenVector0, vec3 TCGenVector1)
{
	vec2 tex = attr_TexCoord0.st;

	if (TCGen == TCGEN_LIGHTMAP)
	{
		tex = attr_TexCoord1.st;
	}
	else if (TCGen == TCGEN_ENVIRONMENT_MAPPED)
	{
		vec3 viewer = normalize(u_LocalViewOrigin - position);
		vec2 ref = reflect(viewer, normal).yz;
		tex.s = ref.x * -0.5 + 0.5;
		tex.t = ref.y *  0.5 + 0.5;
	}
	else if (TCGen == TCGEN_VECTOR)
	{
		tex = vec2(dot(position, TCGenVector0), dot(position, TCGenVector1));
	}
	
	return tex;
}
#endif


int LFSR_Rand_Gen(in int n)
{
  // <<, ^ and & require GL_EXT_gpu_shader4.
  n = (n << 13) ^ n;
  return (n * (n*n*15731+789221) + 1376312589) & 0x7fffffff;
}

float LFSR_Rand_Gen_f(int n )
{
  return float(LFSR_Rand_Gen(n));
}


float noise(vec3 p) {
  ivec3 ip = ivec3(floor(p));
  vec3 u = fract(p);
  u = u*u*(3.0-2.0*u);

  int n = ip.x + ip.y*57 + ip.z*113;

  float res = mix(mix(mix(LFSR_Rand_Gen_f(n+(0+57*0+113*0)),
                          LFSR_Rand_Gen_f(n+(1+57*0+113*0)),u.x),
                      mix(LFSR_Rand_Gen_f(n+(0+57*1+113*0)),
                          LFSR_Rand_Gen_f(n+(1+57*1+113*0)),u.x),u.y),
                 mix(mix(LFSR_Rand_Gen_f(n+(0+57*0+113*1)),
                          LFSR_Rand_Gen_f(n+(1+57*0+113*1)),u.x),
                      mix(LFSR_Rand_Gen_f(n+(0+57*1+113*1)),
                          LFSR_Rand_Gen_f(n+(1+57*1+113*1)),u.x),u.y),u.z);

  return 1.0 - res*(1.0/1073741824.0);
}


void main()
{
	vec3 position  = attr_Position;
	vec3 normal    = attr_Normal;// * 2.0 - vec3(1.0);

#if defined(USE_DEFORM_VERTEXES)
	position = DeformPosition(position, normal, attr_TexCoord0.st);
#endif

#if defined(USE_TCGEN)
	vec2 tex = GenTexCoords(u_TCGen0, position, normal, u_TCGen0Vector0, u_TCGen0Vector1);
#else
	vec2 tex = attr_TexCoord0.st;
#endif

	//position  = (u_ModelMatrix * vec4(position, 1.0)).xyz;
	//normal    = (u_ModelMatrix * vec4(normal,   0.0)).xyz;

	vec3 P = attr_Position + (normal * (m_Length*m_Layer));

#ifdef WAVE
	//Use a noise function to choose a movement group
    float gravGroup = noise(attr_Position);

    //cycle the bend factor based on the total time
    //THIS LINE CAUSES THE WAVING EFFECT!  Tweak this depending on your application.
    float bendAmt = sin((u_Time + gravGroup)*m_WaveSpeed);

    vec3 vGravity = vec3(0,0,0);
	float bendValue = bendAmt * 0.1 * m_WaveSize;
    
    if (gravGroup < 0.3) {
       vGravity = (vec4((-0.1)+bendValue, 0, 0, 0)*u_ModelViewProjectionMatrix).xyz; // g_WorldViewMatrix
    } else if (gravGroup < 0.6) {
       vGravity = (vec4((0.1)+bendValue, 0, 0, 0)*u_ModelViewProjectionMatrix).xyz; // g_WorldViewMatrix
    } else {
       vGravity = (vec4(0, 0, (-0.1)+bendValue, 0)*u_ModelViewProjectionMatrix).xyz; // g_WorldViewMatrix
    }
#else
	vec3 vGravity = ((vec4(-0.1,0,0,0))*u_ModelViewProjectionMatrix).xyz; // g_WorldViewMatrix
#endif

    float k =  pow(m_Layer, 3);  // The higher the exponent is, the closer to the tip the grass will start curving
    P = P + vGravity*k;

    gl_Position = u_ModelViewProjectionMatrix * vec4(P, 1.0);

	var_TexCoords = tex;
	var_Dimensions = u_Dimensions.st;
	var_vertPos = gl_Position.xyz;
	var_Time = u_Time;
	var_Normal = normal;
	var_ViewDir = u_ViewOrigin - position;
}

