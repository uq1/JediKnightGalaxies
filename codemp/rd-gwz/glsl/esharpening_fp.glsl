uniform sampler2D u_TextureMap;

uniform vec2	u_Dimensions;

varying vec2   var_TexCoords;
varying vec2   var_Dimensions;

const float SamplingRange=1.0; //sharpening or blurring range
const float SharpeningAmount=2.3;
const float ScanLineAmount=0.0;
const float ScanLineRepeat=0.0; //0.5, 0.3333, 0.25, 0.125, so on
const float NoiseAmount=0.0;

//global variables, already set before executing this code
float ScreenSize = var_Dimensions.x; //width of the display resolution (1920 f.e.)

void main (void)
{
	vec4 res;
	vec4 coord = vec4(0.0);

	coord.xy = var_TexCoords.xy;
	coord.w=0.0;

	vec4 origcolor = texture2D(u_TextureMap, coord.st);

	const vec2 offset[8] = vec2[8](vec2(1.0, 1.0), vec2(-1.0, -1.0), vec2(-1.0, 1.0), vec2(1.0, -1.0), vec2(1.41, 0.0), vec2(-1.41, 0.0), vec2(0.0, 1.41), vec2(0.0, -1.41));
  
	/*
	offset[0] = vec2(1.0, 1.0);
	offset[1] = vec2(-1.0, -1.0);
	offset[2] = vec2(-1.0, 1.0);
	offset[3] = vec2(1.0, -1.0);

	offset[4] = vec2(1.41, 0.0);
	offset[5] = vec2(-1.41, 0.0);
	offset[6] = vec2(0.0, 1.41);
	offset[7] = vec2(0.0, -1.41);
	*/

	int i=0;

	vec4 tcol=origcolor;
	float invscreensize=1.0/ScreenSize;

	//for (i=0; i<8; i++) //higher quality
	for (i=0; i<4; i++)
	{
		vec2 tdir=offset[i].xy;
		coord.xy=var_TexCoords.xy+tdir.xy*invscreensize*SamplingRange;//*1.0;
		vec4 ct=texture2D(u_TextureMap, coord.st);
		tcol+=ct;
	}

	tcol*=0.2; // 1.0/(4+1)
	//tcol*=0.111; // 1.0/(8+1)  //higher quality

	//sharp
	res=origcolor*(1.0+((origcolor-tcol)*SharpeningAmount));

	//less sharpening for bright pixels
	float rgray=origcolor.z; //blue fit well
	rgray=pow(rgray, 3.0);
	res=mix(res, origcolor, clamp(rgray, 0.0, 1.0));

	res.w=1.0;
	//res.a = origcolor.a;
	gl_FragColor = res;
}
