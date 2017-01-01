uniform sampler2D   u_ScreenDepthMap; //depth
uniform sampler2D   u_NormalMap; //normal

uniform vec2        u_Dimensions;
uniform vec4		u_ViewInfo; // zmin, zmax, zmax / zmin
uniform mat4        u_ModelViewProjectionMatrix;

uniform vec4		u_Local0;

const float camerazoom = 1.0;

//#define raycasts 4			//Increase for better quality
//#define raysegments 6			//Increase for better quality
//#define aoscatter 1.0

#define raycasts 1			//Increase for better quality
#define raysegments 1			//Increase for better quality
#define aoscatter 0.5

//#define raycasts u_Local0.r
//#define raysegments u_Local0.g
//#define aoscatter u_Local0.b

float linearize(float depth)
{
	//float d = clamp((1.0 / mix(u_ViewInfo.z, 1.0, depth)) + u_Local0.g * u_Local0.b, 0.0, 1.0);
	float d = clamp(pow((1.0 / mix(u_ViewInfo.z, 1.0, depth)), 0.2), 0.0, 1.0);
	return d;
}

float DepthToZPosition(in float depth) {
	return u_ViewInfo.x / (u_ViewInfo.y - depth * (u_ViewInfo.y - u_ViewInfo.x)) * u_ViewInfo.y;
}

float GetDepth(in vec2 texcoord)
{
	return linearize(texture2D(u_ScreenDepthMap, texcoord).x);
}

float readZPosition(in vec2 texcoord) {
    return DepthToZPosition( GetDepth( texcoord /*+ (0.25 / u_Dimensions)*/ ) );
}

mat3 vec3tomat3( in vec3 z ) {
        mat3 mat;
        mat[2]=z;
        vec3 v=vec3(z.z,z.x,-z.y);//make a random vector that isn't the same as vector z
        mat[0]=cross(z,v);//cross product is the x axis
        mat[1]=cross(mat[0],z);//cross product is the y axis
        return mat;
}

float compareDepths( in float depth1, in float depth2, in float m ) {
        float diff = (depth1-depth2);
		return 1.0 - clamp(diff * 128.0, 0.0, 1.0);
}

// Calculates the screen-space position of a pixel
// Make sure the depth lookup is using the right texture!!!
vec3 GetPixelPosition( in vec2 coord ) {
        vec3 pos;
        vec2 hu_Dimensions=u_Dimensions/2.0;
        pos.z = DepthToZPosition(GetDepth(coord));
        pos = vec3((((coord.x+0.5)/hu_Dimensions.x)-0.5) * 2.0,(((-coord.y+0.5)/hu_Dimensions.y)+0.5) * 2.0 / (hu_Dimensions.x/hu_Dimensions.y),pos.z);
        pos.x *= pos.z / camerazoom;
        pos.y *= -pos.z / camerazoom;
        return pos;
}

vec3 ScreenCoordToPosition( in vec2 coord, in float z ) {
        vec3 pos;
        vec2 hu_Dimensions=u_Dimensions/2.0;
        pos.z = z;
        pos.x = ((((coord.x+0.5)/hu_Dimensions.x)-0.5) * 2.0)*(pos.z / camerazoom);
        pos.y = ((((-coord.y+0.5)/hu_Dimensions.y)+0.5) * 2.0 / (hu_Dimensions.x/hu_Dimensions.y))*(-pos.z / camerazoom);
        return pos;
}

//Converts a screen position to texture coordinate
vec2 ScreenPositionToCoord( in vec3 pos ) {
        vec2 coord;
        vec2 hu_Dimensions=u_Dimensions/2.0;
        pos.x /= (pos.z / camerazoom);
        coord.x = (pos.x / 2.0 + 0.5);
        
        pos.y /= (-pos.z / camerazoom) / (hu_Dimensions.x/hu_Dimensions.y);
        coord.y = -(pos.y / 2.0 - 0.5);
        
        return coord;// + 0.5/(u_Dimensions/2.0);
}

vec4 PlaneFromPointNormal( in vec3 point, in vec3 normal ) {
        vec4 plane;
        plane.x = normal.x;
        plane.y = normal.y;
        plane.z = normal.z;
        plane.w = -dot(point,normal);
        return plane;
}

vec2 offset1 = vec2(0.0, 1.0 / u_Dimensions.y);
vec2 offset2 = vec2(1.0 / u_Dimensions.x, 0.0);

vec3 normal_from_depth(vec2 texcoords) {
	float depth = GetDepth(texcoords);
	float depth1 = GetDepth(texcoords + offset1);
	float depth2 = GetDepth(texcoords + offset2);
  
	vec3 p1 = vec3(offset1, depth1 - depth);
	vec3 p2 = vec3(offset2, depth2 - depth);
  
	vec3 normal = cross(p1, p2);
	normal.z = -normal.z;
  
	return normalize(normal);
}

vec3 GetPixelNormal( in vec2 coord ) {
		return normal_from_depth(coord);
}

vec4 GetPixelPlane( in vec2 coord ) {
        vec3 point = GetPixelPosition( coord );
        vec3 normal = GetPixelNormal( coord );
        return PlaneFromPointNormal( point, normal);
}

float PointPlaneDistance( in vec4 plane, in vec3 point ) {
        return plane.x*point.x+plane.y*point.y+plane.z*point.z+plane.w;
}

float rand(vec2 co) {
        return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

void main( void ) {
        float ao=0.0;
        float dn;
        float zd;
        float newdepth;
        float angletest;
        float lineardepth;
        float depth;
        vec3 newnormal;
        vec3 newdiffuse;
        vec3 normal;
        vec2 texcoord2;
        vec2 texcoord = gl_FragCoord.xy/u_Dimensions + 0.5/(u_Dimensions*0.5);
        vec3 screencoord;
        vec4 outcolor;

		depth=GetDepth( texcoord );
        
        if (depth<1.0) {
                
				lineardepth = DepthToZPosition(depth);

                normal = GetPixelNormal( texcoord ).rgb;
                normal=normalize(normal);
                normal.z=-normal.z;
                
                vec3 p0=ScreenCoordToPosition(vec2(0.0,0.5),lineardepth);
                vec3 p1=ScreenCoordToPosition(vec2(1.0,0.5),lineardepth);
                float dist = abs(p1.x-p0.x);
                
                screencoord = vec3((((gl_FragCoord.x+0.5)/u_Dimensions.x)-0.5) * 2.0,(((-gl_FragCoord.y+0.5)/u_Dimensions.y)+0.5) * 2.0 / (u_Dimensions.x/u_Dimensions.y),lineardepth);
                screencoord.x *= screencoord.z / camerazoom;
                screencoord.y *= -screencoord.z / camerazoom;
                
                vec3 newpoint;
                vec2 coord;
                vec3 raynormal;
                vec3 offsetvector;
                float diff;             
                vec2 randcoord;         
                float randsum = u_ModelViewProjectionMatrix[0][0]+u_ModelViewProjectionMatrix[0][1]+u_ModelViewProjectionMatrix[0][2];
                randsum+=u_ModelViewProjectionMatrix[1][0]+u_ModelViewProjectionMatrix[1][1]+u_ModelViewProjectionMatrix[1][2];
                randsum+=u_ModelViewProjectionMatrix[2][0]+u_ModelViewProjectionMatrix[2][1]+u_ModelViewProjectionMatrix[2][2];
                randsum+=u_ModelViewProjectionMatrix[3][0]+u_ModelViewProjectionMatrix[3][1]+u_ModelViewProjectionMatrix[3][2];
                
                float raylength = 0.5;//*dist*1.0*50.0;
                float cdm = 1.0;// * dist * 50.0;
                
                ao=0.0;
                
                vec4 gicolor;
                float gisamples;
                mat3 mat=vec3tomat3(normal);
                float a;
                float wheredepthshouldbe;
                
                float mix=(1.0-(clamp(lineardepth-50.0,0.0,50.0)/50.0));
                
                //Get a random number
                a=rand( randsum+texcoord);// + float(53.0*m*raysegments + i*13.0) );
                                        
                if (mix>0.0) {
                        for ( int i=0;i<(raycasts);i++ ) {
                                for ( int m=0;m<(raysegments);m++ ) {
                                        

                                        
                                        offsetvector.x=cos(a+float(i)/float(raycasts)*3.14*4.0)*aoscatter;
                                        offsetvector.y=sin(a+float(i)/float(raycasts)*3.14*4.0)*aoscatter;
                                        offsetvector.z=1.0;
                                        offsetvector=normalize(offsetvector);
                                        
                                        //Create the ray vector
                                        raynormal=mat*offsetvector;
                                        
                                        //Add the ray vector to the screen position
                                        newpoint = screencoord + raynormal * (raylength/raysegments) * float(m+1);
                                        wheredepthshouldbe=newpoint.z;
                                        
                                        //Turn the point back into a screen coord:
                                        coord = ScreenPositionToCoord( newpoint );
                                        
                                        //Look up the depth value at that rays position
                                        newdepth = readZPosition( coord );
                                        
                                        //If new depth is closer to camera darken ao value
                                        //rayao = max(rayao,compareDepths( lineardepth, newdepth ));
                                        //ao = min(ao,compareDepths( lineardepth, newdepth ));
                                        ao += compareDepths( wheredepthshouldbe,newdepth, cdm );
                                        //ao+=compareDepths( lineardepth, newdepth );
                                }
                        }
                }
                
                ao /= (raycasts*raysegments);
                //ao = max(ao * ao,0.5);
                ao = ao * mix + (1.0-mix);              
                gl_FragColor=vec4(ao);
        }
        
        else {
                gl_FragColor=vec4(1.0);
        }
        
}
