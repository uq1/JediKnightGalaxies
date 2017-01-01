uniform sampler2D	u_DiffuseMap;
uniform sampler2D	u_PositionMap;
uniform sampler2D	u_NormalMap;
uniform sampler2D	u_DeluxeMap;
uniform sampler2D	u_GlowMap;

#if defined(USE_SHADOWMAP)
uniform sampler2D	u_ShadowMap;
#endif //defined(USE_SHADOWMAP)

uniform vec2		u_Dimensions;

uniform vec4		u_Local2;

uniform vec3		u_ViewOrigin;
uniform vec4		u_PrimaryLightOrigin;
uniform vec3		u_PrimaryLightColor;

#define MAX_LIGHTALL_DLIGHTS 24

uniform int			u_lightCount;
uniform vec3		u_lightPositions2[MAX_LIGHTALL_DLIGHTS];
uniform float		u_lightDistances[MAX_LIGHTALL_DLIGHTS];
uniform vec3		u_lightColors[MAX_LIGHTALL_DLIGHTS];

varying vec2		var_TexCoords;


#define unOpenGlIsFuckedUpify(x) ( x / 524288.0 )

float drawObject(in vec3 p){
    p = abs(fract(p)-.5);
    return dot(p, vec3(.5));
}

float cellTile(in vec3 p){
    p /= 5.5;
    // Draw four overlapping objects at various positions throughout the tile.
    vec4 v, d; 
    d.x = drawObject(p - vec3(.81, .62, .53));
    p.xy = vec2(p.y-p.x, p.y + p.x)*.7071;
    d.y = drawObject(p - vec3(.39, .2, .11));
    p.yz = vec2(p.z-p.y, p.z + p.y)*.7071;
    d.z = drawObject(p - vec3(.62, .24, .06));
    p.xz = vec2(p.z-p.x, p.z + p.x)*.7071;
    d.w = drawObject(p - vec3(.2, .82, .64));

    v.xy = min(d.xz, d.yw), v.z = min(max(d.x, d.y), max(d.z, d.w)), v.w = max(v.x, v.y); 
   
    d.x =  min(v.z, v.w) - min(v.x, v.y); // Maximum minus second order, for that beveled Voronoi look. Range [0, 1].
    //d.x =  min(v.x, v.y); // First order.
        
    return d.x*2.66; // Normalize... roughly.
}

float map(vec3 p){
    float n = (.5-cellTile(p))*1.5;
    return p.y + dot(sin(p/2. + cos(p.yzx/2. + 3.14159/2.)), vec3(.5)) + n;
}

float calculateAO(in vec3 pos, in vec3 nor)
{
	float sca = 2.0, occ = 0.0;
    for( int i=0; i<5; i++ ){
    
        float hr = 0.01 + float(i)*0.5/4.0;        
        float dd = map(nor * hr + pos);
        occ += (hr - dd)*sca;
        sca *= 0.7;
    }
    return clamp( 1.0 - occ, 0.0, 1.0 );    
}


//
// Full lighting... Blinn phong and basic lighting as well...
//

// Blinn-Phong shading model with rim lighting (diffuse light bleeding to the other side).
// `normal`, `view` and `light` should be normalized.
vec3 blinn_phong(vec3 normal, vec3 view, vec3 light, vec3 diffuseColor, vec3 specularColor) {
	vec3 halfLV = normalize(light + view);
	float spe = pow(max(dot(normal, halfLV), 0.0), 32.0);
	float dif = dot(normal, light) * 0.5 + 0.75;
	return dif*diffuseColor + spe*specularColor;
}

void main(void)
{
	vec4 color = texture2D(u_DiffuseMap, var_TexCoords);
	gl_FragColor = vec4(color.rgb, 1.0);

#if defined(USE_SHADOWMAP)
	float shadowValue = texture(u_ShadowMap, var_TexCoords/*gl_FragCoord.xy * r_FBufScale*/).r;
	//gl_FragColor.rgb *= clamp(shadowValue, 0.4, 1.0);
	gl_FragColor.rgb *= clamp(shadowValue, 0.85, 1.0);
#endif //defined(USE_SHADOWMAP)

	//vec4 glow = texture2D(u_GlowMap, var_TexCoords);
	//float glowMult = clamp(1.0 - (length(glow) / 3.0), 0.0, 1.0);
	float glowMult = 1.0;

	// GLSL distance() can't work with large numbers?!?!??!?!?!!??
	highp vec3 viewOrg = unOpenGlIsFuckedUpify(abs(u_ViewOrigin.xyz));
	highp vec4 position = abs(texture2D(u_PositionMap, var_TexCoords));

	if (position.a == 1024.0 || position.a == 1025.0)
	{// Skybox... Skip...
		//gl_FragColor = vec4(vec3(1.0, 0.0, 0.0), 1.0);
		return;
	}

	if (position.a == 0.0 && position.xyz == vec3(0.0))
	{// Unknown... Skip...
		//gl_FragColor = vec4(vec3(0.0, 1.0, 0.0), 1.0);
		return;
	}

	vec4 norm = texture2D(u_NormalMap, var_TexCoords);

	if (position.a != 0.0 && position.a != 1024.0 && position.a != 1025.0 && norm.a == 0.05)
	{// Generic GLSL. Probably a glow or something, ignore the lighting...
		return;
	}

	/*if (u_Local2.a == 1.0)
	{
		gl_FragColor = vec4(normalize(norm.rgb), 1.0);
		return;
	}*/

	norm.a = norm.a * 0.5 + 0.5;
	norm.rgb = normalize(norm.rgb * 2.0 - 1.0);
	vec3 E = normalize(u_ViewOrigin.xyz - position.xyz);
	vec3 N = norm.xyz;

	//gl_FragColor = vec4(N.xyz * 0.5 + 0.5, 1.0);
	//return;

	vec3 PrimaryLightDir = normalize(u_PrimaryLightOrigin.xyz - position.xyz);
	float lambertian2 = dot(PrimaryLightDir.xyz, N);
	float spec2 = 0.0;
	bool noSunPhong = false;
	float phongFactor = u_Local2.r;

	if (phongFactor < 0.0)
	{// Negative phong value is used to tell terrains not to use sunlight (to hide the triangle edge differences)
		noSunPhong = true;
		phongFactor = 0.0;
	}

	if (!noSunPhong && lambertian2 > 0.0)
	{// this is blinn phong
		vec3 halfDir2 = normalize(PrimaryLightDir.xyz + E);
		float specAngle = max(dot(halfDir2, N), 0.0);
		spec2 = pow(specAngle, 16.0);
		gl_FragColor.rgb += vec3(spec2 * (1.0 - norm.a)) * gl_FragColor.rgb * u_PrimaryLightColor.rgb * phongFactor * glowMult;
	}

	if (noSunPhong)
	{// Invert phong value so we still have non-sun lights...
		phongFactor = -u_Local2.r;
	}

	if (u_lightCount > 0.0)
	{
		vec3 addedLight = vec3(0.0);

		for (int li = 0; li < u_lightCount /*min(u_lightCount, 1)*/; li++)
		{
			vec3 lightPos = unOpenGlIsFuckedUpify(abs(u_lightPositions2[li].xyz));

			float lightDist = distance(lightPos, position.xyz);
			float lightMax = unOpenGlIsFuckedUpify(u_lightDistances[li]) /** 1.5*/;

			if (lightDist < lightMax)
			{
				highp float lightStrength = clamp(1.0 - (lightDist / lightMax), 0.0, 1.0);
				//lightStrength = clamp(pow(lightStrength * 0.9, 3.0), 0.0, 1.0) * 0.5;
				lightStrength = pow(lightStrength, 2.0) * 0.1;

				if (lightStrength > 0.0)
				{
					// Add some basic light...
					addedLight += u_lightColors[li].rgb * lightStrength; // Always add some basic light...

					vec3 lightDir = normalize(lightPos - position.xyz);
					float lambertian3 = dot(lightDir.xyz, N);

					if (lambertian3 > 0.0)
					{// this is blinn phong
						// Diffuse...
						addedLight += (u_lightColors[li].rgb * lightStrength) * gl_FragColor.rgb * lambertian3;

						// Specular...
						vec3 halfDir3 = normalize(lightDir.xyz + E);
						float specAngle3 = max(dot(halfDir3, N), 0.0);
						float spec3 = pow(specAngle3, 16.0);

						highp float strength = ((1.0 - spec3) * (1.0 - norm.a)) * lightStrength * phongFactor;
						addedLight +=  u_lightColors[li].rgb * strength * 0.5;
					}
				}
			}
		}

		//gl_FragColor.rgb += (addedLight * u_Local2.g) / u_lightCount;
		gl_FragColor.rgb += addedLight * glowMult;
		gl_FragColor.rgb = clamp(gl_FragColor.rgb, 0.0, 1.0);
	}

	//if (u_Local2.g >= 1.0)
	//if (position.a != 0.0 && position.a != 1024.0 && position.a != 1025.0)
	{
		float ao = calculateAO(position.xyz, N.xyz);

		ao = clamp(ao * 0.2 + 0.8, 0.0, 1.0);
		float ao2 = clamp(ao + 0.75, 0.0, 1.0);
		ao = (ao + ao2) / 2.0;
		//ao *= ao;
		ao = pow(ao, 4.0);

		gl_FragColor.rgb *= ao;
	}

	//gl_FragColor = vec4(vec3(0.0, 0.0, 1.0), 1.0);
}

