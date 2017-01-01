uniform sampler2D u_TextureMap;
uniform sampler2D u_LevelsMap;
uniform vec4      u_Color;
uniform vec2      u_AutoExposureMinMax;
uniform vec3      u_ToneMinAvgMaxLinear;

uniform vec4	u_ViewInfo; // zfar / znear, zfar
uniform vec2	u_Dimensions;

varying vec2   var_TexCoords;
varying vec4	var_ViewInfo; // zfar / znear, zfar
varying vec2   var_Dimensions;
varying vec4   var_Local0; // depthScale, 0, 0, 0

vec2 texCoord = var_TexCoords;

float near = u_ViewInfo.x;
float far = u_ViewInfo.y;
float viewWidth = var_Dimensions.x;
float viewHeight = var_Dimensions.y;

//float depthScale = 12.0
float depthScale = var_Local0.r;
//float parallax = var_Local0.g;

vec4 GetEmboss( vec3 col2, float embossScale, vec2 coord )
{
	vec2 embossFactor = vec2(1.0f / viewWidth, 1.0f / viewHeight); // calculated from screen size now
	vec4 out_color = vec4(0.0f, 0.0f, 0.0f, 0.0f);
	vec3 col1 = texture2D(u_TextureMap, coord.xy - vec2(embossFactor)).rgb;
	vec3 col3 = texture2D(u_TextureMap, coord.xy + vec2(embossFactor)).rgb;
	vec3 colEmboss = col1 * 2.0 - col2 - col3;
	float colDot = max( 0.0, dot( colEmboss, vec3( 0.333, 0.333, 0.333 ) ) ) * embossScale;
	vec3 colFinal = col2 - vec3( colDot, colDot, colDot ) ;
	float luminance = dot( col2, vec3( 0.6, 0.2, 0.2 ) );
	out_color.rgb = mix( colFinal, col2, luminance * luminance );
	out_color.a	= 1.0;
	return out_color;
}

void main (void)
{
	vec4 color;

	color = texture2D(u_TextureMap, texCoord);

//	Do a heavy emboss, but only splash the result in lightly...
	color.rgb = (color.rgb + color.rgb + GetEmboss( color.rgb, depthScale, texCoord ).rgb) * 0.333;
	color.a = 1.0 / 20.0;

	gl_FragColor = color;
}
