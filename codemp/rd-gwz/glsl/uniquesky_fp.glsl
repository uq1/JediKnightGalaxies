
//#version 120
 
uniform vec4		u_PrimaryLightOrigin;
uniform vec3		u_ViewOrigin;





#if 0





varying vec3 rayleigh;
varying vec3 mie;
varying vec3 eye;
varying vec3 hazeColor;
varying float ct;
varying float cphi;
varying float delta_z;
varying float alt;
varying float earthShade;

/*
uniform float overcast;
uniform float saturation;
uniform float visibility;
uniform float avisibility;
uniform float scattering;
uniform float terminator;
uniform float cloud_self_shading;
uniform float horizon_roughness;
*/

const float terminator = 100000.0;
const float avisibility = 100000.0;
const float visibility = 100000.0;

const float overcast = 0.3;
const float saturation = 0.15;
const float scattering = 0.15;
const float cloud_self_shading = 0.5;
const float horizon_roughness = 0.15;

const float EarthRadius = 5800000.0;

float hash( vec2 p ) {
	float h = dot(p,vec2(127.1,311.7));	
    return fract(sin(h)*43758.5453123);
}
float noise( in vec2 p ) {
    vec2 i = floor( p );
    vec2 f = fract( p );	
	vec2 u = f*f*(3.0-2.0*f);
    return -1.0+2.0*mix( mix( hash( i + vec2(0.0,0.0) ), 
                     hash( i + vec2(1.0,0.0) ), u.x),
                mix( hash( i + vec2(0.0,1.0) ), 
                     hash( i + vec2(1.0,1.0) ), u.x), u.y);
}

float Noise2D(in vec2 coord, in float wavelength)
{
	return noise(coord);
}

float fog_backscatter(in float avisibility)
{
	//return 0.5;
	return fract(sin(avisibility)*43758.5453123);
}

float light_func (in float x, in float a, in float b, in float c, in float d, in float e)
{
x = x - 0.5;

// use the asymptotics to shorten computations
if (x > 30.0) {return e;}
if (x < -15.0) {return 0.0;}

return e / pow((1.0 + a * exp(-b * (x-c)) ),(1.0/d));
}

float miePhase(in float cosTheta, in float g)
{
  float g2 = g*g;
  float a = 1.5 * (1.0 - g2);
  float b = (2.0 + g2);
  float c = 1.0 + cosTheta*cosTheta;
  float d = pow(1.0 + g2 - 2.0 * g * cosTheta, 0.6667);
 
  return (a*c) / (b*d);
}
 
float rayleighPhase(in float cosTheta)
{
  //return 1.5 * (1.0 + cosTheta*cosTheta);
  return 1.5 * (2.0 + 0.5*cosTheta*cosTheta);
}
 

void main()
{

  //vec3 shadedFogColor = vec3(0.65, 0.67, 0.78);
   vec3 shadedFogColor = vec3(0.55, 0.67, 0.88);
  float cosTheta = dot(normalize(eye), u_PrimaryLightOrigin.xyz);
 
  // position of the horizon line

  float lAltitude = alt + delta_z;
  float radiusEye = EarthRadius + alt;
  float radiusLayer = EarthRadius + lAltitude;
  float cthorizon;
  float ctterrain;

  if (radiusEye > radiusLayer) cthorizon = -sqrt(radiusEye * radiusEye - radiusLayer * radiusLayer)/radiusEye;
  else cthorizon = sqrt(radiusLayer * radiusLayer - radiusEye * radiusEye)/radiusLayer;

  ctterrain = -sqrt(radiusEye * radiusEye - EarthRadius * EarthRadius)/radiusEye;

  vec3 color = rayleigh * rayleighPhase(cosTheta);
  color += mie * miePhase(cosTheta, -0.8);

  vec3 black = vec3(0.0,0.0,0.0);

  
  float ovc = overcast;



  float sat = 1.0 - ((1.0 - saturation) * 2.0);
  if (sat < 0.3) sat = 0.3;


  

if (color.r > 0.58) color.r = 1.0 - exp(-1.5 * color.r);
if (color.g > 0.58) color.g = 1.0 - exp(-1.5 * color.g);
if (color.b > 0.58) color.b = 1.0 - exp(-1.5 * color.b);
  


// fog computations for a ground haze layer, extending from zero to lAltitude



float transmission;
float vAltitude;
float delta_zv;

float costheta = ct;

float vis = min(visibility, avisibility);


 if (delta_z > 0.0) // we're inside the layer
	{
  	if (costheta>0.0 + ctterrain) // looking up, view ray intersecting upper layer edge
		{
		transmission  = exp(-min((delta_z/max(costheta,0.1)),25000.0)/vis);
		//transmission = 1.0;
		vAltitude = min(vis * costheta, delta_z);
  		delta_zv = delta_z - vAltitude;
		}

	else // looking down, view range intersecting terrain (which may not be drawn)
		{
		transmission = exp(alt/vis/costheta);
		vAltitude = min(-vis * costheta, alt);
  		delta_zv = delta_z + vAltitude;
		}
	}
  else // we see the layer from above
	{	
	if (costheta < 0.0 + cthorizon) 
		{
		transmission = exp(-min(lAltitude/abs(costheta),25000.0)/vis);
		transmission = transmission * exp(-alt/avisibility/abs(costheta));
		transmission = 1.0 - (1.0 - transmission) * smoothstep(0+cthorizon, -0.02+cthorizon, costheta);
   		vAltitude = min(lAltitude, -vis * costheta);
		delta_zv = vAltitude; 
		}
	else
		{	
		transmission = 1.0;
		delta_zv = 0.0;
		}
	}

// combined intensity reduction by cloud shading and fog self-shading, corrected for Weber-Fechner perception law
float eqColorFactor = 1.0 - 0.1 * delta_zv/vis - (1.0 - min(scattering,cloud_self_shading));


// there's always residual intensity, we should never be driven to zero
if (eqColorFactor < 0.2) eqColorFactor = 0.2;


// postprocessing of haze color
vec3 hColor = hazeColor;


// high altitude desaturation
float intensity = length(hColor);
hColor = intensity * normalize (mix(hColor, intensity * vec3 (1.0,1.0,1.0), 0.7 * smoothstep(5000.0, 50000.0, alt)));

hColor = clamp(hColor,0.0,1.0);

// blue hue
hColor.x = 0.83 * hColor.x;
hColor.y = 0.9 * hColor.y;



// further blueshift when in shadow, either cloud shadow, or self-shadow or Earth shadow, dependent on indirect 
// light

float fade_out = max(0.65 - 0.3 *overcast, 0.45);
intensity = length(hColor);
vec3 oColor = hColor;
oColor = intensity * normalize(mix(oColor,  shadedFogColor, (smoothstep(0.1,1.0,ovc)))); 
oColor = clamp(oColor,0.0,1.0);
color = ovc *  mix(color, oColor * earthShade ,smoothstep(-0.1+ctterrain, 0.0+ctterrain, ct)) + (1.0-ovc) * color; 


hColor = intensity * normalize(mix(hColor,  1.5 * shadedFogColor, 1.0 -smoothstep(0.25, fade_out,earthShade) ));
hColor = intensity * normalize(mix(hColor,  shadedFogColor, (1.0 - smoothstep(0.5,0.9,eqColorFactor)))); 
hColor = hColor * earthShade;

// accounting for overcast and saturation 



color = sat * color + (1.0 - sat) * mix(color, black, smoothstep(0.4+cthorizon,0.2+cthorizon,ct));


// the terrain below the horizon gets drawn in one optical thickness
vec3 terrainHazeColor = eqColorFactor * hColor;	

// determine a visibility-dependent angle for how smoothly the haze blends over the skydome

float hazeBlendAngle = max(0.01,1000.0/avisibility + 0.3 * (1.0 - smoothstep(5000.0, 30000.0, avisibility)));
float altFactor = smoothstep(-300.0, 0.0, delta_z);
float altFactor2 =  0.2 + 0.8 * smoothstep(-3000.0, 0.0, delta_z);
hazeBlendAngle = hazeBlendAngle + 0.1 * altFactor;
hazeBlendAngle = hazeBlendAngle +  (1.0-horizon_roughness) * altFactor2 * 0.1 *  Noise2D(vec2(0.0,cphi), 0.3);

terrainHazeColor = clamp(terrainHazeColor,0.0,1.0);


// don't let the light fade out too rapidly
float lightArg = (terminator + 200000.0)/100000.0;
float minLightIntensity = min(0.2,0.16 * lightArg + 0.5);
vec3 minLight = minLightIntensity * vec3 (0.2, 0.3, 0.4);

// this is for the bare Rayleigh and Mie sky, highly altitude dependent
color.rgb = max(color.rgb, minLight.rgb * (1.0- min(alt/100000.0,1.0)) * (1.0 - costheta));

// this is for the terrain drawn
terrainHazeColor = max(terrainHazeColor.rgb, minLight.rgb);

color = mix(color, terrainHazeColor ,smoothstep(hazeBlendAngle + ctterrain, 0.0+ctterrain, ct));


// add the brightening of fog by lights

    vec3 secondary_light = vec3 (0.0,0.0,0.0);


// mix fog the skydome with the right amount of haze

hColor *= eqColorFactor;
hColor = max(hColor.rgb, minLight.rgb);

hColor = clamp(hColor,0.0,1.0);

color = mix(hColor+secondary_light * fog_backscatter(avisibility),color, transmission);



  gl_FragColor = vec4(color, 1.0);
  gl_FragDepth = 0.1;
}




#else






//uniform vec3 sunPosition;
//uniform vec3 playerPos;
//uniform vec3 playerLookAtDir;

in vec3 fWorldPos;
in float playerLookingAtSun;	// the dot of the player looking at the sun - should be the same for all verts

#define sunPosition u_PrimaryLightOrigin.xyz
#define playerPos u_ViewOrigin.xyz
#define playerLookAtDir normalize(u_ViewOrigin.xyz - fWorldPos.xyz)

float tween(float t)
{
	return clamp(t*t*t*(t*(t*6-15)+10),0,1);
}

float sunfade(float t)
{
	return clamp(t*t, 0, 1);
}

float sunglow(float t)
{
	return clamp(t*t*t*t*t, 0, 1);
}

void main()
{
	// to get here, we need to draw a sphere around the player
	// this is the frag shader for that sphere

	//calculated vars - all need to be normalized
	vec3 SunDir = normalize(u_PrimaryLightOrigin.xyz - (fWorldPos.xyz * u_PrimaryLightOrigin.w));//normalize(sunPosition) - playerPos);
	vec3 FragDir = normalize(fWorldPos - playerPos);
	vec3 UpDir = normalize(playerPos);

	float dotSU = dot(SunDir, UpDir);
	float dotSF = dot(SunDir, FragDir);
	float dotFU = dot(FragDir, UpDir);
	
	//colors
	vec3 DaySkyColor =   mix( vec3(0.25, 0.45, 0.80), vec3(0.00f, 0.25f, 0.60f), max(dotFU,0));     // Day sky color
	vec3 NightSkyColor = vec3(0.00f, 0.01f, 0.05f);     // night sky color
	//vec3 TransColor =    vec3(1.00f, 0.46f, 0.00f);     // sunrise/sunset color
	vec3 TransColor =    vec3(1.00f, 0.50f, 0.25f);     // sunrise/sunset color
	vec3 SunColor =      vec3(1.00f, 1.00f, 0.90f);		// color of the sun 
	vec3 SunGlowColor =  vec3(1.00f, 0.99f, 0.87f);		// color of the sun's glow
	vec3 HorizonGlowColor = TransColor;  // the horizon's glow at rise/set
	//vec3 HorizonGlowColor = vec3(1.00f, 0.80f, 0.40f);  // the horizon's glow at rise/set
	  
	// Things that could be done with the colors:
	// 1. Change sun color depending on sun's closeness to horizon (i.e. dotSU closeness to 0)
	// 2. Change all colors depending on elevation (aka thinner atmosphere at heights)

	vec3 tColor = vec3(0.0f, 1.0f, 0.0f);  // temporary result color

	// controls when the sunset/sunrise sky starts
	float Trans =  0.45;  // the closer to 0, the less time for those events
	float Night = -0.10;  // determines when total nighttime occurs
	float NightTransFade = 0.1; // how much before night the sun's glow starts diminishing
	float transBlendFactor  = 0.35;   // 0 < this < 1 - larger makes the sunset/rise glow closer to sky
	float horizonGlow = 0.33;       // the size of the glow of the horizon near sunrise/sunset
	float horizonSunGlow = 0.0;    // how far from the sun the horizon glow extends
	float sunHorizonFade = 0.02;    // how far up from the horizon does the sun blend
	float underHorizonFade = -0.1;  // from horizon to this line, there's a fade - below this line is solid color
	float SkyLitOffset = 0.2;		// determines how much before 'night' the sun gets brighter or stays brighter
	
	// controls how big the sun will appear in the sky
	float SunDisk = 0.999;  // closer to 1 means smaller the disk
	// sun glow is dependant on how close the player is looking at the sun
	float sunGlowScale = mix(0, playerLookingAtSun, tween(clamp( 1 - (0.4 - dotSU)/(0.4 - 0.25), 0, 1)));
	float SunGlow = mix(0.997, 0.940, sunGlowScale);  // closer to SunDisk, smaller the glow (always should be < SunDisk)
	float MoonDisk = -0.9995;  //experimental moon disk

	// controls the size of the reddish glow near the sun at sunrise/sunset
	float SunTransGlow = 0.5;  // smaller num means larger glow (should not exceed SunGlow, and actually be a lot below it)
	
	// determine sky color beforehand
	tColor = mix(NightSkyColor, DaySkyColor, clamp((dotSU - (Night - SkyLitOffset)) / (Trans - (Night - SkyLitOffset)), 0 ,1) );
	
	if (dotSU > Trans)
	{

	}
	else if (dotSU > Night + NightTransFade)
	{  // we're at sunrise/sunset
	  vec3 tBeforeSun = tColor;
	  // also, set the sky glow of the sunrise/sunset
	  if (dotSF > SunTransGlow)
	  {  // the fragment falls inside the glow side
    	// recalc transcolor based on var:
        TransColor = mix(TransColor, tColor, transBlendFactor);
		// blend smooth glow
		vec3 tTransGlowColor = mix(tColor, TransColor, sunfade((dotSF - SunTransGlow) / (1 - SunTransGlow)) );
		// and now blend depending on dotSU so the sunrise/sunset glow doesn't just 'pop' into existence
		tTransGlowColor = mix(tTransGlowColor, tColor, sunfade((dotSU - Night - NightTransFade) / (Trans - Night - NightTransFade)));
		tColor = tTransGlowColor;
	  }
	  // now if fragment is near horizon, add a glow
	  if (abs(dotFU) < horizonGlow)
	  {
	    // first make the horizon glow color less intrusive
	    HorizonGlowColor = mix(HorizonGlowColor, tBeforeSun, 0.4);
	    // then do some fun stuff with it
	    HorizonGlowColor = mix(HorizonGlowColor, tColor, (horizonSunGlow - dotSF)/(1 - horizonSunGlow));
	    HorizonGlowColor = mix(HorizonGlowColor, tColor, (abs(dotFU))/(horizonGlow));
	    HorizonGlowColor = mix(HorizonGlowColor, tColor, abs(dotFU)/(horizonGlow));
	    tColor = mix(HorizonGlowColor, tColor, (dotSU - Night - NightTransFade) / (Trans - Night - NightTransFade));
	  }
	}
	else if (dotSU > Night)
	{  // we're at sunrise/sunset
	  vec3 tBeforeSun = tColor;
	  // also, set the sky glow of the sunrise/sunset
	  if (dotSF > SunTransGlow)
	  {  // the fragment falls inside the glow side
    	// recalc transcolor based on var:
        TransColor = mix(TransColor, tColor, transBlendFactor);
		// blend smooth glow
		vec3 tTransGlowColor = mix(tColor, TransColor, tween((dotSF - SunTransGlow) / (1 - SunTransGlow)) );
		// and now blend depending on dotSU so the sunrise/sunset glow doesn't just 'pop' into existence
		tTransGlowColor = mix(tColor, tTransGlowColor, (dotSU - Night) / (NightTransFade));
		tColor = tTransGlowColor;
	  }
	  // now if fragment is near horizon, add a glow
	  if (abs(dotFU) < horizonGlow)
	  {
	    // first make the horizon glow color less intrusive
	    HorizonGlowColor = mix(HorizonGlowColor, tBeforeSun, 0.4);
	    // then do some fun stuff with it
	    HorizonGlowColor = mix(HorizonGlowColor, tColor, (horizonSunGlow - dotSF)/(1 - horizonSunGlow));
	    HorizonGlowColor = mix(HorizonGlowColor, tColor, (abs(dotFU))/(horizonGlow));
	    HorizonGlowColor = mix(HorizonGlowColor, tColor, abs(dotFU)/(horizonGlow));
	    tColor = mix(tColor, HorizonGlowColor, (dotSU - Night) / (NightTransFade));
	  }
	}
	else
	{  // it's night time
	  // TODO: lighten up near the horizon (and below) (lessening the lightening up closer to night we are)
	}

	// draw the spot of the sun on the sky, only if the fragment is above the 'horizon'
    if (dotFU > 0)
    {
        vec3 tSunColor = tColor;
        if (dotSF > SunDisk)
        {
            tSunColor = SunColor;
        }
        else if (dotSF > SunGlow)
        {
			SunGlowColor = mix(SunColor, SunGlowColor, sunglow((dotSF - SunGlow) / (SunDisk - SunGlow)) );
            tSunColor = mix(tColor, SunGlowColor, sunglow((dotSF - SunGlow) / (SunDisk - SunGlow)));
        }
        else if (dotSF < MoonDisk)
        {
            tSunColor = vec3(0.89, 0.89, 0.89);
        }
        if (dotFU < sunHorizonFade)
            tColor = mix(tColor, tSunColor, dotFU / sunHorizonFade); // performs a mix to blend the sun with the horizon
        else
            tColor = tSunColor;
    }
	else
	{// Fragment is below the horizon line 
	    vec3 darkColor = tColor/2.0;
		if (dotFU > underHorizonFade)
		    tColor = mix(darkColor, tColor, (dotFU - underHorizonFade) / abs(underHorizonFade));	// darken it a bit
		else
		    tColor = darkColor;
	}

	gl_FragColor = vec4(tColor.rgb, 1.0);
}





#endif
