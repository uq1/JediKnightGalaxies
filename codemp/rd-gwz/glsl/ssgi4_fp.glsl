uniform sampler2D u_DiffuseMap;
uniform sampler2D u_ScreenDepthMap;
uniform sampler2D u_NormalMap;
uniform sampler2D u_DeluxeMap; // actually saturation map image
uniform sampler2D u_SpecularMap;

varying vec2		var_TexCoords;
varying vec2		var_Dimensions;
varying vec4		var_ViewInfo; // zmin, zmax, zmax / zmin

#if 1

/*
*CSSGI shader (Coherent Screen Space Global Illumination)
*This shader requires a depth pass and a normal map pass.
*/

#define PI  3.14159265

#define USE_RANDOMMAP
//#define USE_GLOWMAP
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
	 return normal_from_depth(depth, coord) * 2.0 - 1.0;
#else //!USE_DEPTHMAP
	 return texture2D(u_NormalMap, coord).rgb;
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
	float incx = ratex*30.0;//gi radius
	float incy = ratey*30.0;

	float incx2 = ratex*8.0;//ao radius
	float incy2 = ratey*8.0;

	//do the actual calculations:
	for(float i=-hf; i < hf; i+=1.0){
		for(float j=-hf; j < hf; j+=1.0){
			if (i != 0.0 || j != 0.0) {
				vec2 coords = vec2(i*incx,j*incy)/prof;
				vec2 coords2 = vec2(i*incx2,j*incy2)/prof;

				float prof2 = texture2D(u_ScreenDepthMap,var_TexCoords.st+coords*rand(var_TexCoords)).x;
				prof2 = zFar * zNear / (prof2 * (zFar - zNear) - zFar);  //linearize z sample

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

				//COLOR BLEEDING:
#ifdef USE_GLOWMAP
				vec3 dcolor2 = texture2D(u_DeluxeMap, var_TexCoords.st+coords*rand(var_TexCoords)).xyz;
#else //USE_GLOWMAP
				vec3 dcolor2 = texture2D(u_DiffuseMap, var_TexCoords.st+coords*rand(var_TexCoords)).xyz;
				
				if (length(dcolor2)>0.3)//color threshold
#endif //USE_GLOWMAP
				{
					vec3 norm2 = normalize(vec3(SampleNormals(u_DiffuseMap,var_TexCoords.st+coords*rand(var_TexCoords)).xyz)); 

					//calculate approximate pixel distance:
					vec3 dist = vec3(coords,abs(prof-prof2));

					//calculate normal and sampling direction coherence:
					float coherence = dot(normalize(-coords),normalize(vec2(norm2.xy)));

					//if there is coherence, calculate bleeding:
					if (coherence > 0.0){
						float pformfactor = ((1.0-dot(norm,norm2)))/(3.1416*pow(abs(length(dist*2.0)),2.0)+0.5);//el 4: depthscale
						fcolor += dcolor2*(clamp(pformfactor,0.0,1.0));
					}
				}
			}
		}
	}

	// COLOR BLEED & OCCLUSION
	float MODIFIER = 1.0 - clamp( length(dcolor1.rgb) / 1.5, 0.0, 1.0 );
	vec3 bleeding = (fcolor/samples)*0.5;
	float occlusion = 1.0-((sum/samples) * 0.75 * MODIFIER);

	//gl_FragColor = vec4(occlusion,occlusion,occlusion,1.0);
	//return;

#ifdef USE_GLOWMAP
	bleeding *= clamp(length(dcolor1) * 0.333 * MODIFIER, 0.0, 1.0);
	vec3 final_color = vec3((dcolor1 * occlusion) + (bleeding));// * 1.25;
#else //!USE_GLOWMAP
	bleeding *= 0.5;
	vec3 final_color = vec3((dcolor1 * occlusion) + (dcolor1 * bleeding));// * 1.25;
#endif //USE_GLOWMAP

	// UQ1: Let's add some of the flare color as well... Just to boost colors/glows...
	vec3 flare_color = clamp(texture2D(u_DeluxeMap, var_TexCoords.st).rgb, 0.0, 1.0);
	vec3 add_flare = CalculateFlare(flare_color, final_color);
	final_color = clamp(((final_color * 5.0) + max(add_flare, final_color)) / 6.0, 0.0, 1.0);

	gl_FragColor = vec4(final_color,1.0);
}

#else

#ifdef RANDOM_TEXTURE
uniform sampler2D rand; // Random texture 
#endif
vec2 camerarange = var_ViewInfo.xy;
//vec2 camerarange = vec2(1.0, 1024.0);
      
   float pw = 1.0/var_Dimensions.x;
   float ph = 1.0/var_Dimensions.y;

   float rand(vec2 co) {
		//return 0.5+(fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453))*0.5;
		return (fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453));
   }

   float readDepth(in vec2 coord)  
   {  
     if (coord.x<0||coord.y<0) return 1.0;
	  float nearZ = camerarange.x;  
      float farZ =camerarange.y;  
      float posZ = texture2D(u_ScreenDepthMap, coord).x;   
      return (2.0 * nearZ) / (nearZ + farZ - posZ * (farZ - nearZ));  
   }

   float compareDepths(in float depth1, in float depth2,inout int far)  
   {
     float diff = (depth1 - depth2)*100.0; //depth difference (0-100)
     float gdisplace = 0.2; //gauss bell center
     float garea = 2.0; //gauss bell width 2

     //reduce left bell width to avoid self-shadowing
     if (diff<gdisplace){ 
        garea = 0.1;
     }else{
        far = 1;
     }
     float gauss = pow(2.7182,-2*(diff-gdisplace)*(diff-gdisplace)/(garea*garea));

     return gauss;
   }

   vec3 readColor(in vec2 coord)  
   {
     //return texture2D(u_DiffuseMap, coord).xyz;
	 //return texture2D(u_DeluxeMap, coord).xyz;
	 return texture2D(u_DiffuseMap, coord).xyz + (texture2D(u_DeluxeMap, coord).xyz * 0.333);
   }

   vec3 calAO(float depth,float dw, float dh, inout float ao)  
   {  
	 vec3 bleed = vec3(0.0,0.0,0.0);
     float temp = 0;
     float temp2 = 0;
     float coordw = var_TexCoords.x + dw/depth;
     float coordh = var_TexCoords.y + dh/depth;
     float coordw2 = var_TexCoords.x - dw/depth;
     float coordh2 = var_TexCoords.y - dh/depth;

     if (coordw  < 1.0 && coordw  > 0.0 && coordh < 1.0 && coordh  > 0.0) {
     	vec2 coord = vec2(coordw , coordh);
        vec2 coord2 = vec2(coordw2, coordh2);
        int far = 0;
     	temp = compareDepths(depth, readDepth(coord),far);
		bleed = readColor(coord);

        //DEPTH EXTRAPOLATION:
        if (far > 0){
          temp2 = compareDepths(readDepth(coord2),depth,far);
          temp += (1.0-temp)*temp2; 
		  //bleed = readColor(coord);
        }
     }
 
     ao += temp;
     return temp*bleed;
   }
     
   void main(void)  
   {  
#ifdef RANDOM_TEXTURE
	 //randomization texture:
	 vec2 fres = vec2(20,20);
     vec3 random = texture2D(rand, var_TexCoords.st*fres.xy);
     random = random*2.0-vec3(1.0);
#else
	 //code random
	 vec3 random = vec3(rand(var_TexCoords.st));
     random = random*2.0-vec3(1.0);
#endif

     //initialize stuff:
     float depth = readDepth(var_TexCoords);
	 vec3 gi = vec3(0.0,0.0,0.0);
     float ao = 0.0;

     for(int i=0; i<4; ++i) 
     {  
       //calculate color bleeding and ao:
       gi+=calAO(depth,  pw, ph, ao);  
       gi+=calAO(depth,  pw, -ph, ao);  
       gi+=calAO(depth,  -pw, ph, ao);  
       gi+=calAO(depth,  -pw, -ph, ao);

       gi+=calAO(depth,  pw*1.2, 0, ao);  
       gi+=calAO(depth,  -pw*1.2, 0, ao);  
       gi+=calAO(depth,  0, ph*1.2, ao);  
       gi+=calAO(depth,  0, -ph*1.2, ao);
     
       //sample jittering:
       pw += random.x*0.0007;
       ph += random.y*0.0007;

       //increase sampling area:
       pw *= 1.7;  
       ph *= 1.7;    
     }

     //final values, some adjusting:
     vec3 finalAO = vec3(1.0-(ao/32.0)) * 0.25;
	 vec3 finalGI = (gi/32)*0.6;

	 vec4 color = texture2D(u_DiffuseMap, var_TexCoords.st);

	 gl_FragColor = vec4((finalAO, 1.0) * color) + vec4(finalGI*0.5, 1.0);
     //gl_FragColor = vec4((0.3+finalAO*0.7,1.0) * color) + vec4(finalGI*0.5, 1.0);
	 //gl_FragColor = vec4((0.3+finalAO*0.7,1.0) * (color + vec4(finalGI*0.5, 1.0)));
   }

#endif