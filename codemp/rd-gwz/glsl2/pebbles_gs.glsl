#if !defined(USE_400)
#extension GL_ARB_gpu_shader5 : enable
//layout(triangles) in;
//#else
//layout(triangles, invocations = 6) in;
#endif

#define MAX_PEBBLES				100
#define MAX_PEBBLES_LOOP		64

layout(triangles) in;
layout(triangle_strip, max_vertices = MAX_PEBBLES) out;

uniform mat4				u_ModelViewProjectionMatrix;
uniform mat4				u_ModelMatrix;
uniform mat4				u_ModelViewMatrix;
uniform mat4				u_NormalMatrix;

uniform sampler2D			u_SplatControlMap;

uniform vec4				u_Local6; // useSunLightSpecular, hasSteepMap2, MAP_SIZE, WATER_LEVEL // -- only MAP_SIZE is used here
uniform vec4				u_Local7; // hasSplatMap1, hasSplatMap2, hasSplatMap3, hasSplatMap4
uniform vec4				u_Local8; // passnum, 0, 0, 0
uniform vec4				u_Local9; // testvalue0, 1, 2, 3
uniform vec4				u_Local10; // foliageLODdistance, foliageDensity, MAP_WATER_LEVEL, tessOffset

uniform vec3				u_ViewOrigin;
uniform float				u_Time;

flat in	int					isSlope[];

smooth out vec2				vTexCoord;
out vec3					vVertPosition;
flat out int				iGrassType;

#define MAP_WATER_LEVEL			u_Local10.b // TODO: Use water map
#define PASS_NUMBER				u_Local8.r

//
// General Settings...
//

const float					fGrassPatchSize = 3.0;
float						controlScale = 1.0 / u_Local6.b;

//
// LOD Range Settings...
//

#define MAX_RANGE			u_Local10.r

vec3 vLocalSeed;

// This function returns random number from zero to one
float randZeroOne()
{
	uint n = floatBitsToUint(vLocalSeed.y * 214013.0 + vLocalSeed.x * 2531011.0 + vLocalSeed.z * 141251.0);
	n = n * (n * n * 15731u + 789221u);
	n = (n >> 9u) | 0x3F800000u;

	float fRes = 2.0 - uintBitsToFloat(n);
	vLocalSeed = vec3(vLocalSeed.x + 147158.0 * fRes, vLocalSeed.y*fRes + 415161.0 * fRes, vLocalSeed.z + 324154.0*fRes);
	return fRes;
}

int randomInt(int min, int max)
{
	float fRandomFloat = randZeroOne();
	return int(float(min) + fRandomFloat*float(max - min));
}

// Produce a psuedo random point that exists on the current triangle primitive.
vec4 randomBarycentricCoordinate() {
	float R = randZeroOne();
	float S = randZeroOne();

	if (R + S >= 1) {
		R = 1 - R;
		S = 1 - S;
	}

	return gl_in[0].gl_Position + (R * (gl_in[1].gl_Position - gl_in[0].gl_Position)) + (S * (gl_in[2].gl_Position - gl_in[0].gl_Position));
}

#define M_PI		3.14159265358979323846


bool InstanceCloudReductionCulling(vec4 InstancePosition, vec3 ObjectExtent)
{
	/* create the bounding box of the object */
	vec4 BoundingBox[8];
	BoundingBox[0] = u_ModelViewProjectionMatrix * (InstancePosition + vec4(ObjectExtent.x, ObjectExtent.y, ObjectExtent.z, 1.0));
	BoundingBox[1] = u_ModelViewProjectionMatrix * (InstancePosition + vec4(-ObjectExtent.x, ObjectExtent.y, ObjectExtent.z, 1.0));
	BoundingBox[2] = u_ModelViewProjectionMatrix * (InstancePosition + vec4(ObjectExtent.x, -ObjectExtent.y, ObjectExtent.z, 1.0));
	BoundingBox[3] = u_ModelViewProjectionMatrix * (InstancePosition + vec4(-ObjectExtent.x, -ObjectExtent.y, ObjectExtent.z, 1.0));
	BoundingBox[4] = u_ModelViewProjectionMatrix * (InstancePosition + vec4(ObjectExtent.x, ObjectExtent.y, -ObjectExtent.z, 1.0));
	BoundingBox[5] = u_ModelViewProjectionMatrix * (InstancePosition + vec4(-ObjectExtent.x, ObjectExtent.y, -ObjectExtent.z, 1.0));
	BoundingBox[6] = u_ModelViewProjectionMatrix * (InstancePosition + vec4(ObjectExtent.x, -ObjectExtent.y, -ObjectExtent.z, 1.0));
	BoundingBox[7] = u_ModelViewProjectionMatrix * (InstancePosition + vec4(-ObjectExtent.x, -ObjectExtent.y, -ObjectExtent.z, 1.0));

	/* check how the bounding box resides regarding to the view frustum */
	int outOfBound[6];

	for (int i = 0; i < 6; i++) outOfBound[i] = 0;

	for (int i = 0; i<8; i++)
	{
		if (BoundingBox[i].x >  BoundingBox[i].w) outOfBound[0]++;
		if (BoundingBox[i].x < -BoundingBox[i].w) outOfBound[1]++;
		if (BoundingBox[i].y >  BoundingBox[i].w) outOfBound[2]++;
		if (BoundingBox[i].y < -BoundingBox[i].w) outOfBound[3]++;
		if (BoundingBox[i].z >  BoundingBox[i].w) outOfBound[4]++;
		if (BoundingBox[i].z < -BoundingBox[i].w) outOfBound[5]++;
	}

	bool inFrustum = true;

	for (int i = 0; i < 6; i++)
	{
		if (outOfBound[i] == 8)
		{
			inFrustum = false;
			break;
		}
	}

	return !inFrustum;
}

void main()
{
	if (isSlope[0] > 0 || isSlope[1] > 0 || isSlope[2] > 0)
	{
		return; // This slope is too steep for grass...
	}

	iGrassType = 0;

	//face center------------------------
	vec3 Vert1 = gl_in[0].gl_Position.xyz;
	vec3 Vert2 = gl_in[1].gl_Position.xyz;
	vec3 Vert3 = gl_in[2].gl_Position.xyz;

	vec3 Pos = (Vert1 + Vert2 + Vert3) / 3.0;   //Center of the triangle - copy for later
	//-----------------------------------

	//if (Pos.z < MAP_WATER_LEVEL - 512.0)
	//{// Below map's water level... Early cull... (Maybe underwater plants later???)
	//	return;
	//}

	// UQ1: Checked and distance is faster
	float VertDist = distance(u_ViewOrigin, Pos);//(u_ModelViewProjectionMatrix*vec4(Pos, 1.0)).z;

	if (VertDist >= MAX_RANGE + 1024 // Too far from viewer...
		|| (VertDist >= 1024.0 && Pos.z < MAP_WATER_LEVEL && u_ViewOrigin.z >= MAP_WATER_LEVEL) // Underwater and distant and player is not...
		|| (VertDist >= 1024.0 && Pos.z >= MAP_WATER_LEVEL && u_ViewOrigin.z < MAP_WATER_LEVEL)) // Above water and player is below...
	{// Early cull...
		return;
	}

	// Do some ICR culling on the base surfaces... Save us looping through extra surfaces...
	/*vec3 maxs;
	maxs = max(gl_in[0].gl_Position.xyz - Pos, gl_in[1].gl_Position.xyz - Pos);
	maxs = max(maxs, gl_in[2].gl_Position.xyz - Pos);

	if (InstanceCloudReductionCulling(vec4(Pos, 0.0), maxs))
	{
	return;
	}*/

	vec3 normal = normalize(cross(gl_in[2].gl_Position.xyz - gl_in[0].gl_Position.xyz, gl_in[1].gl_Position.xyz - gl_in[0].gl_Position.xyz)); //calculate normal for this face

	//#if !defined(USE_400)
	// No invocations support...
	vLocalSeed = Pos*PASS_NUMBER;
	//#else
	//	// invocations support...
	//	vLocalSeed = Pos*float(gl_InvocationID);
	//#endif

	float m = 1.0 - clamp((VertDist - 1024.0) / MAX_RANGE, 0.0, 1.0);

	float USE_DENSITY = pow(m, 3.0);

	int numAddedVerts = 0;
	int maxAddedVerts = int(float(MAX_PEBBLES)*USE_DENSITY);

	for (int x = 0; x < MAX_PEBBLES_LOOP; x++)
	{
		if (float(numAddedVerts) >= maxAddedVerts)
		{
			break;
		}

		vec3 vGrassFieldPos = randomBarycentricCoordinate().xyz;

		/*if (vGrassFieldPos.z < MAP_WATER_LEVEL && float(numAddedVerts) >= float(maxAddedVerts) * 0.025)
		{
			break;
		}*/

		float VertDist2 = distance(u_ViewOrigin, vGrassFieldPos);

		if (VertDist2 >= MAX_RANGE)
		{// Too far from viewer... Cull...
			continue;
		}


		iGrassType = randomInt(0, 3);

		float fGrassPatchHeight = randZeroOne() * 0.5 + 0.5;

		float size = fGrassPatchSize*fGrassPatchHeight;

		vGrassFieldPos += u_Local10.a;

		vGrassFieldPos += size * 0.05;// 0.1;
		vec3 toCamera = normalize(u_ViewOrigin - vGrassFieldPos);
		vec3 up = vec3(0.0, 1.0, 0.0);
		vec3 right = cross(toCamera, up) * size;

		vGrassFieldPos -= (right * 0.5);
		vVertPosition = vGrassFieldPos.xyz;
		gl_Position = u_ModelViewProjectionMatrix * vec4(vGrassFieldPos, 1.0);
		vTexCoord = vec2(0.0, 0.0);
		EmitVertex();

		vGrassFieldPos.y += size;
		vVertPosition = vGrassFieldPos.xyz;
		gl_Position = u_ModelViewProjectionMatrix * vec4(vGrassFieldPos, 1.0);
		vTexCoord = vec2(0.0, 1.0);
		EmitVertex();

		vGrassFieldPos.y -= size;
		vGrassFieldPos += right;
		vVertPosition = vGrassFieldPos.xyz;
		gl_Position = u_ModelViewProjectionMatrix * vec4(vGrassFieldPos, 1.0);
		vTexCoord = vec2(1.0, 0.0);
		EmitVertex();

		vGrassFieldPos.y += size;
		vVertPosition = vGrassFieldPos.xyz;
		gl_Position = u_ModelViewProjectionMatrix * vec4(vGrassFieldPos, 1.0);
		vTexCoord = vec2(1.0, 1.0);
		EmitVertex();

		EndPrimitive();

		numAddedVerts += 4;
	}
}