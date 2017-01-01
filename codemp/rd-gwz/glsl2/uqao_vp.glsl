attribute vec3	attr_Position;
attribute vec4	attr_TexCoord0;

uniform mat4	u_ModelViewProjectionMatrix;
uniform vec3	u_ViewOrigin;

varying vec2	var_TexCoords;
varying vec3	var_ViewDir;
varying vec3	var_position;
varying vec3	var_viewOrg;

void main()
{
	gl_Position = u_ModelViewProjectionMatrix * vec4(attr_Position, 1.0);
	var_TexCoords = attr_TexCoord0.st;
	
	var_ViewDir = u_ModelViewProjectionMatrix * vec4(u_ViewOrigin - attr_Position.xyz, 1.0);
	//var_ViewDir = u_ViewOrigin;
	//var_viewOrg = u_ViewOrigin;
	var_viewOrg = u_ModelViewProjectionMatrix * vec4(u_ViewOrigin, 1.0);
	var_position = u_ModelViewProjectionMatrix * vec4(attr_Position.xyz, 1.0);
}
