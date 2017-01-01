#if 1
uniform sampler2D				u_DiffuseMap;
uniform sampler2D				u_ScreenDepthMap;
uniform sampler2D				u_NormalMap;
uniform sampler2D				u_PositionMap;

uniform mat4					u_ModelViewProjectionMatrix;
uniform vec4					u_ViewInfo; // zmin, zmax, zmax / zmin
uniform vec3					u_ViewOrigin;
uniform vec4					u_PrimaryLightOrigin;
uniform vec4					u_Local0;
uniform vec4					u_Local1; // sunx, suny

varying vec2					var_ScreenTex;
varying vec2					var_Dimensions;


float linearize(float depth)
{
	return clamp(1.0 / mix(u_ViewInfo.z, 1.0, depth), 0.0, 1.0);
}

vec4 WorldSpacePositionData ( vec2 coord )
{
	return texture2D(u_PositionMap, coord);
}

float DepthFromPositionInfo ( vec4 positionInfo )
{
	return clamp(distance(positionInfo.xyz, u_ViewOrigin) / u_ViewInfo.y, 0.0, 1.0);
}

float DepthAtCoordinate ( vec2 coord )
{
	return clamp(distance(WorldSpacePositionData(coord).xyz, u_ViewOrigin) / u_ViewInfo.y, 0.0, 1.0);
}

void main(void){
	vec3 dglow = texture2D(u_NormalMap, var_ScreenTex).rgb;
	float dglowStrength = clamp(length(dglow.rgb) * 3.0, 0.0, 1.0);

	vec2 texel_size = vec2(1.0 / var_Dimensions);
	//float depth = texture2D(u_ScreenDepthMap, var_ScreenTex).r;
	//depth = linearize(depth);
	vec4 posInfo = WorldSpacePositionData(var_ScreenTex);
	float depth = DepthFromPositionInfo(posInfo);

	//gl_FragColor = vec4(vec3(depth), 1.0);
	//return;

	float invDepth = 1.0 - depth;

	/*vec2 distFromCenter = vec2((0.5 - var_ScreenTex.x) * 2.0, (1.0 - var_ScreenTex.y) * 0.5);
	vec2 pixOffset = clamp((distFromCenter * invDepth) * texel_size * 80.0, 0.0 - (texel_size * 80.0), texel_size * 80.0);
	vec2 pos = var_ScreenTex + pixOffset;*/
	//float distFromCenter = 0.5 - var_ScreenTex.x;
	//vec2 pixOffset = vec2((distFromCenter*2.0) * 20.0 * texel_size.x, -(length(distFromCenter*2.0) * 20.0 * texel_size.y));
	//vec2 pos = clamp(var_ScreenTex + pixOffset, 0.0, 1.0);
	vec2 pixOffset = vec2(20.0 * texel_size.x, -(20.0 * texel_size.y));
	vec2 pos = clamp(var_ScreenTex + pixOffset, 0.0, 1.0);

	//vec2 distFromCenter = vec2(1.0 - (u_Local1.x - var_ScreenTex.x), 1.0 - (u_Local1.y - var_ScreenTex.y));
	//vec2 pixOffset = clamp((distFromCenter * invDepth) * texel_size * 40.0, (texel_size * 40.0), texel_size * 40.0);
	//vec2 pos = clamp(var_ScreenTex + pixOffset, 0.0, 1.0);

	//float d2 = texture2D(u_ScreenDepthMap, pos).r;
	//d2 = linearize(d2);
	vec4 posInfo2 = WorldSpacePositionData(pos);
	float d2 = DepthFromPositionInfo(posInfo2);

	vec4 diffuse = texture2D(u_DiffuseMap, var_ScreenTex);
	vec4 oDiffuse = diffuse;

	float depthDiff = depth - d2;

	if (depthDiff < u_Local0.r || depthDiff > u_Local0.g || depth > 0.9 || d2 > 0.9)
	{
		gl_FragColor = vec4(diffuse.rgb, 1.0);
		return;
	}

	//vec3 shadow = diffuse.rgb * 0.25;
	//shadow += diffuse.rgb * (0.75 * clamp(depthDiff / u_Local0.g, 0.0, 1.0)); // less darkness at higher distance for blending
	//float invDglow = 1.0 - dglowStrength;
	//diffuse.rgb = (diffuse.rgb * dglowStrength) + (shadow * invDglow);
	vec3 shadow = diffuse.rgb * 0.25;//clamp(1.0 - pow((depthDiff / u_Local0.g), 4.0), 0.25, 1.0);
	diffuse.rgb = shadow;

	diffuse = min(diffuse, oDiffuse);

	gl_FragColor = vec4(diffuse.rgb, 1.0);
}



#else



uniform sampler2D					u_DiffuseMap;
uniform sampler2D					u_ScreenDepthMap;
uniform sampler2D					u_NormalMap;

uniform mat4						u_ModelViewProjectionMatrix;
uniform mat4						u_ProjectionMatrix;
uniform mat4						u_NormalMatrix;
uniform mat4						u_invProjectionMatrix;
uniform vec4						u_ViewInfo; // zmin, zmax, zmax / zmin
uniform vec4						u_PrimaryLightOrigin;
uniform vec3						u_ViewOrigin;
uniform vec4						u_Local0;
uniform vec4						u_Local1; // sunx, suny

varying vec3						var_Position;
varying vec2						var_ScreenTex;
varying vec2						var_Dimensions;
varying vec3						viewRay;
varying vec3						lightPos;
varying vec4						var_PrimaryLightDir;

//vec3 viewRay = vec3(var_Position.xy/var_Position.z, 1.0);

#define DepthMap					u_ScreenDepthMap
//#define projectionMatrix			u_ModelViewProjectionMatrix
//#define projectionMatrix			u_ProjectionMatrix
#define projectionMatrix			u_NormalMatrix
//#define light_p						var_PrimaryLightDir.xyz//(u_invProjectionMatrix * vec4(u_ViewOrigin-u_PrimaryLightOrigin.xyz, 1.0)).xyz
#define light_p						lightPos//(u_invProjectionMatrix * (u_ModelViewProjectionMatrix * vec4(u_PrimaryLightOrigin.xyz, 1.0))).xyz
#define pass_TexCoord				var_ScreenTex
#define resolution					var_Dimensions

vec2 projAB = vec2( u_ViewInfo.g / (u_ViewInfo.g - u_ViewInfo.r), u_ViewInfo.g * u_ViewInfo.r / (u_ViewInfo.g - u_ViewInfo.r) );

float linearize(float depth)
{
	return 1.0 / mix(u_ViewInfo.z, 1.0, depth);
}

vec3 CalcPosition(void){
    float depth = texture2D(DepthMap, pass_TexCoord).r;
	float linearDepth = projAB.y / (depth - projAB.x);
	//float linearDepth = linearize(depth);
	vec3 ray = normalize(viewRay);
	
	//direction towards they eye (camera) in the view (eye) space
    //vec3 ecEyeDir = normalize(-var_Position);
    //direction towards the camera in the world space
    //vec3 wcEyeDir = vec3(u_invProjectionMatrix * vec4(ecEyeDir, 0.0));
	//vec3 ray = normalize(wcEyeDir);

    ray = ray / ray.z;
    return linearDepth * ray;
}

void main(void){
	vec4 diffuse = texture2D(u_DiffuseMap, pass_TexCoord);
	float out_SH = 1.0;
    vec2 texel_size = vec2(1.0 / resolution.x, 1.0 / resolution.y);
    vec3 origin = CalcPosition();

    if(origin.z < -9999)//u_Local1.a)//-99)
	{//Don't check points at infinity
		gl_FragColor = diffuse;
		return;
	}

    vec2 pixOrigin = pass_TexCoord;

    vec3 dir = normalize(light_p - origin);
    vec4 tempDir = projectionMatrix * vec4(dir, 0.0);
	//vec3 dir = normalize(light_p);
	//vec4 tempDir = vec4(dir, 0.0);

    vec2 pixDir = -tempDir.xy / tempDir.w;
    float dirLength = length(pixDir);
    pixDir = pixDir / dirLength;


    vec2 nextT, deltaT;

    if(pixDir.x < 0){
        deltaT.x = -texel_size.x / pixDir.x;
        nextT.x = (floor(pixOrigin.x * resolution.x) * texel_size.x - pixOrigin.x) / pixDir.x;
    }
    else {
        deltaT.x = texel_size.x / pixDir.x;
        nextT.x = ((floor(pixOrigin.x * resolution.x) + 1.0) * texel_size.x - pixOrigin.x) / pixDir.x;
    }
    if(pixDir.y < 0){
        deltaT.y = -texel_size.y / pixDir.y;
        nextT.y = (floor(pixOrigin.y * resolution.y) * texel_size.y - pixOrigin.y) / pixDir.y;
    }
    else {
        deltaT.y = texel_size.y / pixDir.y;
        nextT.y = ((floor(pixOrigin.y * resolution.y) + 1.0) * texel_size.y - pixOrigin.y) / pixDir.y;
    }


    float t = 0.0;
    vec2 pixIndex = vec2(pixOrigin * resolution);

    while(true){
        if(t > 0.0){
            float rayDepth = (origin + t * dir).z;
            vec2 texCoord = pixOrigin + 0.5 * pixDir * t * dirLength;
            float depth = texture2D(DepthMap, texCoord).r;
			float linearDepth = projAB.y / (depth - projAB.x);
			//float linearDepth = linearize(depth);
            if(linearDepth > rayDepth + u_Local1.b && linearDepth < rayDepth + u_Local1.a)//0.1)
			{
                out_SH = 0.2;
                break;
            }
        }
        if(nextT.x < nextT.y){
            t = nextT.x;
            nextT.x += deltaT.x;
            if(pixDir.x < 0.0) pixIndex.x -= 1.0;
            else pixIndex.x += 1.0;
        }
        else {
            t = nextT.y;
            nextT.y += deltaT.y;
            if(pixDir.y < 0.0) pixIndex.y -= 1.0;
            else pixIndex.y += 1.0;
        }
        if(pixIndex.x < 0.0 || pixIndex.x > resolution.x || pixIndex.y < 0.0 || pixIndex.y > resolution.y) break;
    }

	diffuse.rgb *= out_SH;

/*
	float depth = texture2D(DepthMap, pass_TexCoord).r;
    float linearDepth = linearize(depth);//projAB.y / (depth - projAB.x);
	vec4 diffuse = vec4(linearDepth, linearDepth, linearDepth, 1.0);
*/
	gl_FragColor = diffuse;
}


#endif
