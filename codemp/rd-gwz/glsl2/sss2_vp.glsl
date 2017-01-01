attribute vec3					attr_Position;
attribute vec2					attr_TexCoord0;

uniform mat4					u_ModelViewProjectionMatrix;
uniform mat4					u_ModelViewMatrix;
uniform mat4					u_ProjectionMatrix;
uniform mat4					u_invProjectionMatrix;
uniform vec2					u_Dimensions;
uniform vec4					u_ViewInfo; // zmin, zmax, zmax / zmin
uniform vec3					u_ViewOrigin;
uniform vec4					u_PrimaryLightOrigin;
uniform float					u_PrimaryLightRadius;

varying vec3					var_Position;
varying vec2					var_ScreenTex;
varying vec2					var_Dimensions;
varying vec3					viewRay;
varying vec3					lightPos;
varying vec4					var_PrimaryLightDir;

void main()
{
	gl_Position = u_ModelViewProjectionMatrix * vec4(attr_Position, 1.0);
	var_ScreenTex = attr_TexCoord0.st;
	var_Dimensions = u_Dimensions;
#if 1
	var_Position = (u_ModelViewMatrix * vec4(attr_Position, 1.0)).xyz;//attr_Position;
	//viewRay = attr_Position - u_ViewOrigin;
	var_PrimaryLightDir.xyz = u_PrimaryLightOrigin.xyz - (attr_Position * u_PrimaryLightOrigin.w);
	var_PrimaryLightDir.w = u_PrimaryLightRadius * u_PrimaryLightRadius;
	//const float fov = 1.0;//0.57735;
	//float aspect = u_Dimensions.x / u_Dimensions.y;
	//viewRay = (u_ModelViewMatrix * vec4(vec3(-attr_Position.x*aspect*fov, -attr_Position.y*fov, 1.0), 1.0)).xyz;
	//viewRay = (u_invProjectionMatrix * vec4(vec3(gl_Position.xy * (-u_ViewInfo.g/gl_Position.z), -u_ViewInfo.g), 0.0)).xyz;
	//viewRay = (u_ModelViewMatrix * vec4(vec3(attr_Position.xy / attr_Position.z, 1.0), 1.0)).xyz;
	viewRay = -gl_Position.xyz;//-vec3(u_ModelViewMatrix * vec4(attr_Position, 1.0));
	//lightPos = (u_invProjectionMatrix * (u_ModelViewProjectionMatrix * vec4(u_PrimaryLightOrigin.xyz, 1.0))).xyz;
	lightPos = (u_ModelViewMatrix * vec4(u_PrimaryLightOrigin.xyz, 1.0)).xyz;
#endif
}
