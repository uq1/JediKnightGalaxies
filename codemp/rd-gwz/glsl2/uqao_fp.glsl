uniform sampler2D	u_TextureMap; // Screen Image...
uniform sampler2D	u_NormalMap; // Water Map...
uniform sampler2D	u_SubsurfaceMap; // Sky Image...
uniform sampler2D	u_ScreenDepthMap; // Depth Map...
uniform sampler2D	u_SpecularMap; // Random Map...

uniform vec2		u_Dimensions;
uniform float		u_Time;
uniform vec4		u_Local0;

varying vec2		var_TexCoords;
varying vec3		var_ViewDir;
varying vec3		var_position;
varying vec3		var_viewOrg;

void main()
{
	vec4 origColor = texture2D(u_TextureMap, var_TexCoords);
	gl_FragColor = origColor;
}
