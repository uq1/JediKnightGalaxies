uniform sampler2D u_DiffuseMap;
uniform sampler2D u_ScreenDepthMap;
uniform sampler2D u_NormalMap;
uniform sampler2D u_DeluxeMap; // actually saturation map image
uniform sampler2D u_SpecularMap;

varying vec2		var_TexCoords;
varying vec2		var_Dimensions;
varying vec4		var_ViewInfo; // zmin, zmax, zmax / zmin

/*
*CSSGI shader (Coherent Screen Space Global Illumination)
*This shader requires a depth pass and a normal map pass.
*/

#define PI  3.14159265

#define USE_RANDOMMAP
#define USE_DEPTHMAP

#ifdef USE_DEPTHMAP
const float depthMult = 255.0;
#endif //USE_DEPTHMAP

float ratex = (1.0/var_Dimensions.x);
float ratey = (1.0/var_Dimensions.y);

vec2 offset1 = vec2(0.0, ratey);
vec2 offset2 = vec2(ratex, 0.0);

#ifdef USE_DEPTHMAP
vec3 normal_from_depth(float depth, vec2 texcoords) {
  float depth1 = texture2D(u_ScreenDepthMap, texcoords + offset1).r * depthMult;
  float depth2 = texture2D(u_ScreenDepthMap, texcoords + offset2).r * depthMult;
  
  vec3 p1 = vec3(offset1, depth1 - depth);
  vec3 p2 = vec3(offset2, depth2 - depth);
  
  vec3 normal = cross(p1, p2);
  normal.z = -normal.z;
  
  return normalize(normal);
}
#endif //USE_DEPTHMAP

vec3 SampleNormals(sampler2D normalMap, in vec2 coord)  
{
#ifdef USE_DEPTHMAP
	 float depth = texture2D(u_ScreenDepthMap, coord/*var_TexCoords*/).r * depthMult;
	 return normal_from_depth(depth, coord);
#else //!USE_DEPTHMAP
	 return texture2D(u_NormalMap, coord).rgb * 2.0 - 1.0;
#endif //USE_DEPTHMAP
}

#ifdef USE_RANDOMMAP
float rand2(vec2 coord)
{
	return texture2D(u_SpecularMap, coord*vec2(var_Dimensions)*vec2(1./256.)).r;
}

float rand(vec2 co){
	return texture2D(u_SpecularMap, co*vec2(var_Dimensions)*vec2(1./256.)).b;
}
#else //!USE_RANDOMMAP
float rand2(vec2 coord) //generating noise/pattern texture for dithering
{
	float noise = ((fract(1.0-coord.s*(var_Dimensions.x/2.0))*0.25)+(fract(coord.t*(var_Dimensions.y/2.0))*0.75))*2.0-1.0;
	return noise;
}

//noise producing function to eliminate banding (got it from someone else´s shader):
float rand(vec2 co){
	return 0.5+(fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453))*0.5;
}
#endif //USE_RANDOMMAP

vec3 CalculateFlare ( vec3 flare_color, vec3 final_color )
{
	float mult = clamp((3.0 - (flare_color.r + flare_color.g + flare_color.b)) + 0.1, 0.0, 3.0);
	vec3 add_flare = clamp(flare_color.rgb * mult, 0.0, 1.0);
#if 0
#define const_1 ( 12.0 / 255.0)
#define const_2 (255.0 / 219.0)
	add_flare = ((clamp(add_flare - const_1, 0.0, 1.0)) * const_2);
#endif
	return clamp(((add_flare * add_flare * 0.5) + 1.0) * final_color, 0.0, 1.0);
}

void main()
{
#ifdef FAST_SSGI
	const float NUM_SAMPLES = 2.0;
#else //!FAST_SSGI
	const float NUM_SAMPLES = 4.0;
#endif //FAST_SSGI

	//calculate sampling rates:
	//initialize occlusion sum and gi color:
	float sum = 0.0;
	vec3 fcolor = vec3(0,0,0);

	//far and near clip planes:
	//float zFar = 1.0;
	float zFar = var_ViewInfo.y / var_Dimensions.y;
	float zNear = var_ViewInfo.x / var_Dimensions.x;

	//get depth at current pixel:
	float prof = texture2D(u_ScreenDepthMap, var_TexCoords.st).x;
	//scale sample number with depth:
	float samples = round(NUM_SAMPLES/(0.5+prof));
	prof = zFar * zNear / (prof * (zFar - zNear) - zFar);  //linearize z sample

	//obtain normal and color at current pixel:
	vec3 norm = normalize(vec3(SampleNormals(u_DiffuseMap,var_TexCoords.st).xyz));
	vec3 dcolor1 = texture2D(u_DiffuseMap, var_TexCoords.st).xyz;

	float hf = samples/2.0;

	//calculate kernel steps:
	float incx2 = ratex*8.0;//ao radius
	float incy2 = ratey*8.0;

	//do the actual calculations:
	for(float i=-hf; i < hf; i+=1.0){
		for(float j=-hf; j < hf; j+=1.0){

			if (i != 0.0 || j != 0.0) {
				vec2 coords2 = vec2(i*incx2,j*incy2)/prof;

				float prof2g = texture2D(u_ScreenDepthMap,var_TexCoords.st+coords2*rand(var_TexCoords)).x;
				prof2g = zFar * zNear / (prof2g * (zFar - zNear) - zFar);  //linearize z sample

				//OCCLUSION:
				vec3 norm2g = normalize(vec3(SampleNormals(u_DiffuseMap,var_TexCoords.st+coords2*rand(var_TexCoords)).xyz)); 

				//calculate approximate pixel distance:
				vec3 dist2 = vec3(coords2,prof-prof2g);

				//calculate normal and sampling direction coherence:
				float coherence2 = dot(normalize(-coords2),normalize(vec2(norm2g.xy)));

				//if there is coherence, calculate occlusion:
				if (coherence2 > 0.0){
					float pformfactor2 = 0.5*((1.0-dot(norm,norm2g)))/(3.1416*pow(abs(length(dist2*2.0)),2.0)+0.5);//el 4: depthscale
					sum += clamp(pformfactor2*0.2,0.0,1.0);//ao intensity; 
				}
			}
		}
	}

	// OCCLUSION ONLY
	float MODIFIER = 1.0 - clamp( length(dcolor1.rgb) / 1.5, 0.0, 1.0 );
	float occlusion = 1.0-((sum/samples) * 0.75 * MODIFIER);
	vec3 final_color = vec3(dcolor1 * occlusion);// * 1.25;

	// UQ1: Let's add some of the flare color as well... Just to boost colors/glows...
	vec3 flare_color = clamp(texture2D(u_DeluxeMap, var_TexCoords.st).rgb, 0.0, 1.0);
	vec3 add_flare = CalculateFlare(flare_color, final_color);
	final_color = clamp(((final_color * 5.0) + max(add_flare, final_color)) / 6.0, 0.0, 1.0);

	gl_FragColor = vec4(final_color,1.0);
}
