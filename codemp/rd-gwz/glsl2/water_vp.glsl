#define USE_VERTEX_ANIMATION
//#define USE_SKELETAL_ANIMATION

#define USE_DEFORM_VERTEXES
#define USE_TCGEN
#define USE_TCMOD
//#define USE_LIGHTMAP
//#define USE_RGBAGEN
//#define USE_FOG

attribute vec3 attr_Position;
attribute vec3 attr_Normal;

#if defined(USE_VERTEX_ANIMATION)
attribute vec3 attr_Position2;
attribute vec3 attr_Normal2;
#elif defined(USE_SKELETAL_ANIMATION)
attribute vec4 attr_BoneIndexes;
attribute vec4 attr_BoneWeights;
#endif

attribute vec4 attr_Color;
attribute vec2 attr_TexCoord0;

#if defined(USE_LIGHTMAP) || defined(USE_TCGEN)
attribute vec2 attr_TexCoord1;
#endif

uniform vec4   u_DiffuseTexMatrix;
uniform vec4   u_DiffuseTexOffTurb;

#if defined(USE_TCGEN) || defined(USE_RGBAGEN)
uniform vec3   u_LocalViewOrigin;
#endif

#if defined(USE_TCGEN)
uniform int    u_TCGen0;
uniform vec3   u_TCGen0Vector0;
uniform vec3   u_TCGen0Vector1;
#endif

#if defined(USE_FOG)
uniform vec4   u_FogDistance;
uniform vec4   u_FogDepth;
uniform float  u_FogEyeT;
uniform vec4   u_FogColorMask;
#endif

#if defined(USE_DEFORM_VERTEXES)
uniform int    u_DeformGen;
uniform float  u_DeformParams[5];
#endif

uniform mat4   u_ModelViewProjectionMatrix;
uniform vec4   u_BaseColor;
uniform vec4   u_VertColor;

#if defined(USE_RGBAGEN)
uniform int    u_ColorGen;
uniform int    u_AlphaGen;
uniform vec3   u_AmbientLight;
uniform vec3   u_DirectedLight;
uniform vec3   u_ModelLightDir;
uniform float  u_PortalRange;
#endif

#if defined(USE_VERTEX_ANIMATION)
uniform float  u_VertexLerp;
#elif defined(USE_SKELETAL_ANIMATION)
uniform mat4   u_BoneMatrices[20];
#endif

varying vec2   var_DiffuseTex;
#if defined(USE_LIGHTMAP)
varying vec2   var_LightTex;
#endif
varying vec4   var_Color;

uniform vec2	u_Dimensions;
uniform vec3	u_ViewOrigin;
uniform float	u_Time;
uniform vec4	u_Local0; // (1=water, 2=lava), 0, 0, 0
uniform vec4	u_Local1; // parallaxScale, haveSpecular, specularScale, materialType

varying vec2	var_TexCoords;
varying vec4	var_Local0; // (1=water, 2=lava), 0, 0, 0
varying vec4	var_Local1; // parallaxScale, haveSpecular, specularScale, materialType
varying vec2	var_Dimensions;
varying vec3	var_vertPos;
varying float	var_Time;
varying vec3	var_Normal;
varying vec3	var_ViewDir;
varying vec3	var_position;
varying vec3	var_viewOrg;

varying vec2 fragCoord;

void main()
{
    vec4 object_space_pos = vec4( attr_Position.xyz, 1.0);
    gl_Position = u_ModelViewProjectionMatrix * object_space_pos;
    
	//fragCoord = attr_Position.xy;
	fragCoord = attr_TexCoord0.xy;

	var_Normal    = attr_Normal * 2.0 - vec3(1.0);
	var_ViewDir = u_ViewOrigin - object_space_pos.xyz;
	var_Time = u_Time;
}
