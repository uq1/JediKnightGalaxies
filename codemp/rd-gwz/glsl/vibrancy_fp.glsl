uniform sampler2D	u_DiffuseMap;

uniform vec4		u_Local0; // vibrancy, 0, 0, 0

varying vec2		var_TexCoords;

void main()
{
	vec3	res;
	vec3	origcolor = texture2D(u_DiffuseMap, var_TexCoords.xy).rgb;
	vec3	lumCoeff = vec3(0.212656, 0.715158, 0.072186);  				//Calculate luma with these values
	
	float	max_color = max(origcolor.r, max(origcolor.g,origcolor.b)); 	//Find the strongest color
	float	min_color = min(origcolor.r, min(origcolor.g,origcolor.b)); 	//Find the weakest color

	float	color_saturation = max_color - min_color; 						//Saturation is the difference between min and max

	float	luma = dot(lumCoeff, origcolor.rgb); 							//Calculate luma (grey)

	res = mix(vec3(luma), origcolor.rgb, (1.0 + (u_Local0.x * (1.0 - (sign(u_Local0.x) * color_saturation))))); 	//Extrapolate between luma and original by 1 + (1-saturation) - current
	
	gl_FragColor = vec4(res.rgb, 1.0);
}
