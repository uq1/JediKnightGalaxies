/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// tr_sky.c
#include "tr_local.h"

#define SKY_SUBDIVISIONS		8
#define HALF_SKY_SUBDIVISIONS	(SKY_SUBDIVISIONS/2)

static float s_cloudTexCoords[6][SKY_SUBDIVISIONS+1][SKY_SUBDIVISIONS+1][2];
static float s_cloudTexP[6][SKY_SUBDIVISIONS+1][SKY_SUBDIVISIONS+1];

/*
===================================================================================

POLYGON TO BOX SIDE PROJECTION

===================================================================================
*/

static vec3_t sky_clip[6] = 
{
	{1,1,0},
	{1,-1,0},
	{0,-1,1},
	{0,1,1},
	{1,0,1},
	{-1,0,1} 
};

static float	sky_mins[2][6], sky_maxs[2][6];
static float	sky_min, sky_max;

/*
================
AddSkyPolygon
================
*/
static void AddSkyPolygon (int nump, vec3_t vecs) 
{
	int		i,j;
	vec3_t	v, av;
	float	s, t, dv;
	int		axis;
	float	*vp;
	// s = [0]/[2], t = [1]/[2]
	static int	vec_to_st[6][3] =
	{
		{-2,3,1},
		{2,3,-1},

		{1,3,2},
		{-1,3,-2},

		{-2,-1,3},
		{-2,1,-3}

	//	{-1,2,3},
	//	{1,2,-3}
	};

	// decide which face it maps to
	VectorCopy (vec3_origin, v);
	for (i=0, vp=vecs ; i<nump ; i++, vp+=3)
	{
		VectorAdd (vp, v, v);
	}
	av[0] = fabs(v[0]);
	av[1] = fabs(v[1]);
	av[2] = fabs(v[2]);
	if (av[0] > av[1] && av[0] > av[2])
	{
		if (v[0] < 0)
			axis = 1;
		else
			axis = 0;
	}
	else if (av[1] > av[2] && av[1] > av[0])
	{
		if (v[1] < 0)
			axis = 3;
		else
			axis = 2;
	}
	else
	{
		if (v[2] < 0)
			axis = 5;
		else
			axis = 4;
	}

	// project new texture coords
	for (i=0 ; i<nump ; i++, vecs+=3)
	{
		j = vec_to_st[axis][2];
		if (j > 0)
			dv = vecs[j - 1];
		else
			dv = -vecs[-j - 1];
		if (dv < 0.001)
			continue;	// don't divide by zero
		j = vec_to_st[axis][0];
		if (j < 0)
			s = -vecs[-j -1] / dv;
		else
			s = vecs[j-1] / dv;
		j = vec_to_st[axis][1];
		if (j < 0)
			t = -vecs[-j -1] / dv;
		else
			t = vecs[j-1] / dv;

		if (s < sky_mins[0][axis])
			sky_mins[0][axis] = s;
		if (t < sky_mins[1][axis])
			sky_mins[1][axis] = t;
		if (s > sky_maxs[0][axis])
			sky_maxs[0][axis] = s;
		if (t > sky_maxs[1][axis])
			sky_maxs[1][axis] = t;
	}
}

#define	ON_EPSILON		0.1f			// point on plane side epsilon
#define	MAX_CLIP_VERTS	64
/*
================
ClipSkyPolygon
================
*/
static void ClipSkyPolygon (int nump, vec3_t vecs, int stage) 
{
	float	*norm;
	float	*v;
	qboolean	front, back;
	float	d, e;
	float	dists[MAX_CLIP_VERTS];
	int		sides[MAX_CLIP_VERTS];
	vec3_t	newv[2][MAX_CLIP_VERTS];
	int		newc[2];
	int		i, j;

	if (nump > MAX_CLIP_VERTS-2)
		ri->Error (ERR_DROP, "ClipSkyPolygon: MAX_CLIP_VERTS");
	if (stage == 6)
	{	// fully clipped, so draw it
		AddSkyPolygon (nump, vecs);
		return;
	}

	front = back = qfalse;
	norm = sky_clip[stage];
	for (i=0, v = vecs ; i<nump ; i++, v+=3)
	{
		d = DotProduct (v, norm);
		if (d > ON_EPSILON)
		{
			front = qtrue;
			sides[i] = SIDE_FRONT;
		}
		else if (d < -ON_EPSILON)
		{
			back = qtrue;
			sides[i] = SIDE_BACK;
		}
		else
			sides[i] = SIDE_ON;
		dists[i] = d;
	}

	if (!front || !back)
	{	// not clipped
		ClipSkyPolygon (nump, vecs, stage+1);
		return;
	}

	// clip it
	sides[i] = sides[0];
	dists[i] = dists[0];
	VectorCopy (vecs, (vecs+(i*3)) );
	newc[0] = newc[1] = 0;

	for (i=0, v = vecs ; i<nump ; i++, v+=3)
	{
		switch (sides[i])
		{
		case SIDE_FRONT:
			VectorCopy (v, newv[0][newc[0]]);
			newc[0]++;
			break;
		case SIDE_BACK:
			VectorCopy (v, newv[1][newc[1]]);
			newc[1]++;
			break;
		case SIDE_ON:
			VectorCopy (v, newv[0][newc[0]]);
			newc[0]++;
			VectorCopy (v, newv[1][newc[1]]);
			newc[1]++;
			break;
		}

		if (sides[i] == SIDE_ON || sides[i+1] == SIDE_ON || sides[i+1] == sides[i])
			continue;

		d = dists[i] / (dists[i] - dists[i+1]);
		for (j=0 ; j<3 ; j++)
		{
			e = v[j] + d*(v[j+3] - v[j]);
			newv[0][newc[0]][j] = e;
			newv[1][newc[1]][j] = e;
		}
		newc[0]++;
		newc[1]++;
	}

	// continue
	ClipSkyPolygon (newc[0], newv[0][0], stage+1);
	ClipSkyPolygon (newc[1], newv[1][0], stage+1);
}

/*
==============
ClearSkyBox
==============
*/
static void ClearSkyBox (void) {
	int		i;

	for (i=0 ; i<6 ; i++) {
		sky_mins[0][i] = sky_mins[1][i] = 9999;
		sky_maxs[0][i] = sky_maxs[1][i] = -9999;
	}
}

/*
================
RB_ClipSkyPolygons
================
*/
void RB_ClipSkyPolygons( shaderCommands_t *input )
{
	vec3_t		p[5];	// need one extra point for clipping
	int			i, j;

	ClearSkyBox();

	for ( i = 0; i < input->numIndexes; i += 3 )
	{
		for (j = 0 ; j < 3 ; j++) 
		{
			VectorSubtract( input->xyz[input->indexes[i+j]],
							backEnd.viewParms.ori.origin, 
							p[j] );
		}
		ClipSkyPolygon( 3, p[0], 0 );
	}
}

/*
===================================================================================

CLOUD VERTEX GENERATION

===================================================================================
*/

/*
** MakeSkyVec
**
** Parms: s, t range from -1 to 1
*/
static void MakeSkyVec( float s, float t, int axis, float outSt[2], vec3_t outXYZ )
{
	// 1 = s, 2 = t, 3 = 2048
	static int	st_to_vec[6][3] =
	{
		{3,-1,2},
		{-3,1,2},

		{1,3,2},
		{-1,-3,2},

		{-2,-1,3},		// 0 degrees yaw, look straight up
		{2,-1,-3}		// look straight down
	};

	vec3_t		b;
	int			j, k;
	float	boxSize;

	boxSize = backEnd.viewParms.zFar / 1.75;		// div sqrt(3)
	b[0] = s*boxSize;
	b[1] = t*boxSize;
	b[2] = boxSize;

	for (j=0 ; j<3 ; j++)
	{
		k = st_to_vec[axis][j];
		if (k < 0)
		{
			outXYZ[j] = -b[-k - 1];
		}
		else
		{
			outXYZ[j] = b[k - 1];
		}
	}

	// avoid bilerp seam
	s = (s+1)*0.5;
	t = (t+1)*0.5;
	if (s < sky_min)
	{
		s = sky_min;
	}
	else if (s > sky_max)
	{
		s = sky_max;
	}

	if (t < sky_min)
	{
		t = sky_min;
	}
	else if (t > sky_max)
	{
		t = sky_max;
	}

	t = 1.0 - t;


	if ( outSt )
	{
		outSt[0] = s;
		outSt[1] = t;
	}
}

static vec3_t	s_skyPoints[SKY_SUBDIVISIONS+1][SKY_SUBDIVISIONS+1];
static float	s_skyTexCoords[SKY_SUBDIVISIONS+1][SKY_SUBDIVISIONS+1][2];

static void DrawSkySide( struct image_s *image, const int mins[2], const int maxs[2] )
{
	int s, t;
	int firstVertex = tess.numVertexes;
	//int firstIndex = tess.numIndexes;
	int minIndex = tess.minIndex;
	int maxIndex = tess.maxIndex;
	vec4_t color;

	//tess.numVertexes = 0;
	//tess.numIndexes = 0;
	tess.firstIndex = tess.numIndexes;
	
	GL_Bind( image );
	GL_Cull( CT_TWO_SIDED );

	for ( t = mins[1]+HALF_SKY_SUBDIVISIONS; t <= maxs[1]+HALF_SKY_SUBDIVISIONS; t++ )
	{
		for ( s = mins[0]+HALF_SKY_SUBDIVISIONS; s <= maxs[0]+HALF_SKY_SUBDIVISIONS; s++ )
		{
			tess.xyz[tess.numVertexes][0] = s_skyPoints[t][s][0];
			tess.xyz[tess.numVertexes][1] = s_skyPoints[t][s][1];
			tess.xyz[tess.numVertexes][2] = s_skyPoints[t][s][2];
			tess.xyz[tess.numVertexes][3] = 1.0;

			tess.texCoords[tess.numVertexes][0][0] = s_skyTexCoords[t][s][0];
			tess.texCoords[tess.numVertexes][0][1] = s_skyTexCoords[t][s][1];

			tess.numVertexes++;

			if(tess.numVertexes >= SHADER_MAX_VERTEXES)
			{
				ri->Error(ERR_DROP, "SHADER_MAX_VERTEXES hit in DrawSkySideVBO()");
			}
		}
	}

	for ( t = 0; t < maxs[1] - mins[1]; t++ )
	{
		for ( s = 0; s < maxs[0] - mins[0]; s++ )
		{
			if (tess.numIndexes + 6 >= SHADER_MAX_INDEXES)
			{
				ri->Error(ERR_DROP, "SHADER_MAX_INDEXES hit in DrawSkySideVBO()");
			}

			tess.indexes[tess.numIndexes++] =  s +       t      * (maxs[0] - mins[0] + 1) + firstVertex;
			tess.indexes[tess.numIndexes++] =  s +      (t + 1) * (maxs[0] - mins[0] + 1) + firstVertex;
			tess.indexes[tess.numIndexes++] = (s + 1) +  t      * (maxs[0] - mins[0] + 1) + firstVertex;

			tess.indexes[tess.numIndexes++] = (s + 1) +  t      * (maxs[0] - mins[0] + 1) + firstVertex;
			tess.indexes[tess.numIndexes++] =  s +      (t + 1) * (maxs[0] - mins[0] + 1) + firstVertex;
			tess.indexes[tess.numIndexes++] = (s + 1) + (t + 1) * (maxs[0] - mins[0] + 1) + firstVertex;
		}
	}

	tess.minIndex = firstVertex;
	tess.maxIndex = tess.numVertexes;

	// FIXME: A lot of this can probably be removed for speed, and refactored into a more convenient function
	//RB_UpdateVBOs(ATTR_POSITION | ATTR_TEXCOORD0);
	RB_UpdateVBOs(ATTR_POSITION | ATTR_TEXCOORD0 | ATTR_COLOR | ATTR_NORMAL | ATTR_TANGENT | ATTR_TEXCOORD1 | ATTR_LIGHTDIRECTION);
/*
	{
		shaderProgram_t *sp = &tr.textureColorShader;

		GLSL_VertexAttribsState(ATTR_POSITION | ATTR_TEXCOORD0);
		GLSL_BindProgram(sp);
		
		GLSL_SetUniformMatrix16(sp, UNIFORM_MODELVIEWPROJECTIONMATRIX, glState.modelviewProjection);
		
		color[0] = 
		color[1] = 
		color[2] = tr.identityLight;
		color[3] = 1.0f;
		GLSL_SetUniformVec4(sp, UNIFORM_COLOR, color);
	}
*/
	{
		shaderProgram_t *sp = &tr.lightallShader[0];
		vec4_t vector;

		GLSL_VertexAttribsState(ATTR_POSITION | ATTR_TEXCOORD0 | ATTR_COLOR | ATTR_NORMAL | ATTR_TANGENT | ATTR_TEXCOORD1 | ATTR_LIGHTDIRECTION);
		GLSL_BindProgram(sp);

		VectorSet4(vector, 0.0, 0.0, 0.0, 1024.0);
		GLSL_SetUniformVec4(sp, UNIFORM_LOCAL1, vector); // parallaxScale, hasSpecular, specularScale, materialType
		VectorSet4(vector, 0.0, 0.0, 0.0, 0.0);
		GLSL_SetUniformVec4(sp, UNIFORM_LOCAL2, vector);
		GLSL_SetUniformVec4(sp, UNIFORM_LOCAL3, vector);
		GLSL_SetUniformVec4(sp, UNIFORM_LOCAL4, vector);
		GLSL_SetUniformVec4(sp, UNIFORM_LOCAL5, vector);
		GLSL_SetUniformVec4(sp, UNIFORM_LOCAL6, vector);
		GLSL_SetUniformVec4(sp, UNIFORM_LOCAL7, vector);
		GLSL_SetUniformVec4(sp, UNIFORM_LOCAL8, vector);
		GLSL_SetUniformVec4(sp, UNIFORM_LOCAL9, vector);
		GLSL_SetUniformVec4(sp, UNIFORM_LOCAL10, vector);

		//GL_BindToTMU( tr.whiteImage, TB_SUBSURFACEMAP );
		GL_BindToTMU( tr.blackImage, TB_OVERLAYMAP );
		GL_BindToTMU( tr.whiteImage, TB_STEEPMAP );
		GL_BindToTMU( tr.whiteImage, TB_STEEPMAP2 );
		GL_BindToTMU( tr.whiteImage, TB_SPLATMAP1 );
		GL_BindToTMU( tr.whiteImage, TB_SPLATMAP2 );
		GL_BindToTMU( tr.whiteImage, TB_SPLATMAP3 );
		GL_BindToTMU( tr.whiteImage, TB_SPLATMAP4 );

		GL_BindToTMU( tr.whiteImage, TB_NORMALMAP );
		GL_BindToTMU( tr.whiteImage, TB_DELUXEMAP );
		GL_BindToTMU( tr.whiteImage, TB_SPECULARMAP );

		GLSL_SetUniformVec4(sp, UNIFORM_ENABLETEXTURES, vector);

		VectorSet4(vector, r_baseNormalX->value, r_baseNormalY->value, 1.0f, r_baseParallax->value);
		GLSL_SetUniformVec4(sp, UNIFORM_NORMALSCALE, vector);
		
		GLSL_SetUniformMatrix16(sp, UNIFORM_MODELVIEWPROJECTIONMATRIX, glState.modelviewProjection);
		
		color[0] = 
		color[1] = 
		color[2] = backEnd.refdef.colorScale;
		color[3] = 1.0f;
		GLSL_SetUniformVec4(sp, UNIFORM_BASECOLOR, color);

		color[0] = 
		color[1] = 
		color[2] = 
		color[3] = 0.0f;
		GLSL_SetUniformVec4(sp, UNIFORM_VERTCOLOR, color);

		VectorSet4(vector, 1.0, 0.0, 0.0, 1.0);
		GLSL_SetUniformVec4(sp, UNIFORM_DIFFUSETEXMATRIX, vector);

		VectorSet4(vector, 0.0, 0.0, 0.0, 0.0);
		GLSL_SetUniformVec4(sp, UNIFORM_DIFFUSETEXOFFTURB, vector);
	}

	if (r_tesselation->integer)
		R_DrawElementsVBO(tess.numIndexes - tess.firstIndex, tess.firstIndex, tess.minIndex, tess.maxIndex, tess.numVertexes, qtrue);
	else
		R_DrawElementsVBO(tess.numIndexes - tess.firstIndex, tess.firstIndex, tess.minIndex, tess.maxIndex, tess.numVertexes, qfalse);

	//qglDrawElements(GL_TRIANGLES, tess.numIndexes - tess.firstIndex, GL_INDEX_TYPE, BUFFER_OFFSET(tess.firstIndex * sizeof(glIndex_t)));
	
	//R_BindNullVBO();
	//R_BindNullIBO();

	tess.numIndexes = tess.firstIndex;
	tess.numVertexes = firstVertex;
	tess.firstIndex = 0;
	tess.minIndex = minIndex;
	tess.maxIndex = maxIndex;
}

static void DrawSkyBox( shader_t *shader )
{
	int		i;

#ifdef __ORIGINAL_OCCLUSION__
	if (r_occlusion->integer)
	{
		tess.numIndexes = 0;
		tess.numVertexes = 0;
		tess.firstIndex = 0;
		tess.minIndex = 0;
		tess.maxIndex = 0;
	}
#endif //__ORIGINAL_OCCLUSION__

	sky_min = 0;
	sky_max = 1;

	Com_Memset( s_skyTexCoords, 0, sizeof( s_skyTexCoords ) );

	for (i=0 ; i<6 ; i++)
	{
		int sky_mins_subd[2], sky_maxs_subd[2];
		int s, t;

		sky_mins[0][i] = floor( sky_mins[0][i] * HALF_SKY_SUBDIVISIONS ) / HALF_SKY_SUBDIVISIONS;
		sky_mins[1][i] = floor( sky_mins[1][i] * HALF_SKY_SUBDIVISIONS ) / HALF_SKY_SUBDIVISIONS;
		sky_maxs[0][i] = ceil( sky_maxs[0][i] * HALF_SKY_SUBDIVISIONS ) / HALF_SKY_SUBDIVISIONS;
		sky_maxs[1][i] = ceil( sky_maxs[1][i] * HALF_SKY_SUBDIVISIONS ) / HALF_SKY_SUBDIVISIONS;

		if ( ( sky_mins[0][i] >= sky_maxs[0][i] ) ||
			 ( sky_mins[1][i] >= sky_maxs[1][i] ) )
		{
			continue;
		}

		sky_mins_subd[0] = sky_mins[0][i] * HALF_SKY_SUBDIVISIONS;
		sky_mins_subd[1] = sky_mins[1][i] * HALF_SKY_SUBDIVISIONS;
		sky_maxs_subd[0] = sky_maxs[0][i] * HALF_SKY_SUBDIVISIONS;
		sky_maxs_subd[1] = sky_maxs[1][i] * HALF_SKY_SUBDIVISIONS;

		if ( sky_mins_subd[0] < -HALF_SKY_SUBDIVISIONS ) 
			sky_mins_subd[0] = -HALF_SKY_SUBDIVISIONS;
		else if ( sky_mins_subd[0] > HALF_SKY_SUBDIVISIONS ) 
			sky_mins_subd[0] = HALF_SKY_SUBDIVISIONS;
		if ( sky_mins_subd[1] < -HALF_SKY_SUBDIVISIONS )
			sky_mins_subd[1] = -HALF_SKY_SUBDIVISIONS;
		else if ( sky_mins_subd[1] > HALF_SKY_SUBDIVISIONS ) 
			sky_mins_subd[1] = HALF_SKY_SUBDIVISIONS;

		if ( sky_maxs_subd[0] < -HALF_SKY_SUBDIVISIONS ) 
			sky_maxs_subd[0] = -HALF_SKY_SUBDIVISIONS;
		else if ( sky_maxs_subd[0] > HALF_SKY_SUBDIVISIONS ) 
			sky_maxs_subd[0] = HALF_SKY_SUBDIVISIONS;
		if ( sky_maxs_subd[1] < -HALF_SKY_SUBDIVISIONS ) 
			sky_maxs_subd[1] = -HALF_SKY_SUBDIVISIONS;
		else if ( sky_maxs_subd[1] > HALF_SKY_SUBDIVISIONS ) 
			sky_maxs_subd[1] = HALF_SKY_SUBDIVISIONS;

		//
		// iterate through the subdivisions
		//
		for ( t = sky_mins_subd[1]+HALF_SKY_SUBDIVISIONS; t <= sky_maxs_subd[1]+HALF_SKY_SUBDIVISIONS; t++ )
		{
			for ( s = sky_mins_subd[0]+HALF_SKY_SUBDIVISIONS; s <= sky_maxs_subd[0]+HALF_SKY_SUBDIVISIONS; s++ )
			{
				MakeSkyVec( ( s - HALF_SKY_SUBDIVISIONS ) / ( float ) HALF_SKY_SUBDIVISIONS, 
							( t - HALF_SKY_SUBDIVISIONS ) / ( float ) HALF_SKY_SUBDIVISIONS, 
							i, 
							s_skyTexCoords[t][s], 
							s_skyPoints[t][s] );
			}
		}

		DrawSkySide( shader->sky.outerbox[i],
			         sky_mins_subd,
					 sky_maxs_subd );
	}

}

static void FillCloudySkySide( const int mins[2], const int maxs[2], qboolean addIndexes )
{
	int s, t;
	int vertexStart = tess.numVertexes;
	int tHeight, sWidth;

	tHeight = maxs[1] - mins[1] + 1;
	sWidth = maxs[0] - mins[0] + 1;

	for ( t = mins[1]+HALF_SKY_SUBDIVISIONS; t <= maxs[1]+HALF_SKY_SUBDIVISIONS; t++ )
	{
		for ( s = mins[0]+HALF_SKY_SUBDIVISIONS; s <= maxs[0]+HALF_SKY_SUBDIVISIONS; s++ )
		{
			VectorAdd( s_skyPoints[t][s], backEnd.viewParms.ori.origin, tess.xyz[tess.numVertexes] );
			tess.texCoords[tess.numVertexes][0][0] = s_skyTexCoords[t][s][0];
			tess.texCoords[tess.numVertexes][0][1] = s_skyTexCoords[t][s][1];

			tess.numVertexes++;

			if ( tess.numVertexes >= SHADER_MAX_VERTEXES )
			{
				ri->Error( ERR_DROP, "SHADER_MAX_VERTEXES hit in FillCloudySkySide()" );
			}
		}
	}

	// only add indexes for one pass, otherwise it would draw multiple times for each pass
	if ( addIndexes ) {
		for ( t = 0; t < tHeight-1; t++ )
		{	
			for ( s = 0; s < sWidth-1; s++ )
			{
				tess.indexes[tess.numIndexes] = vertexStart + s + t * ( sWidth );
				tess.numIndexes++;
				tess.indexes[tess.numIndexes] = vertexStart + s + ( t + 1 ) * ( sWidth );
				tess.numIndexes++;
				tess.indexes[tess.numIndexes] = vertexStart + s + 1 + t * ( sWidth );
				tess.numIndexes++;

				tess.indexes[tess.numIndexes] = vertexStart + s + ( t + 1 ) * ( sWidth );
				tess.numIndexes++;
				tess.indexes[tess.numIndexes] = vertexStart + s + 1 + ( t + 1 ) * ( sWidth );
				tess.numIndexes++;
				tess.indexes[tess.numIndexes] = vertexStart + s + 1 + t * ( sWidth );
				tess.numIndexes++;
			}
		}
	}
}

static void FillCloudBox( const shader_t *shader, int stage )
{
	int i;

	for ( i =0; i < 6; i++ )
	{
		int sky_mins_subd[2], sky_maxs_subd[2];
		int s, t;
		float MIN_T;

		if ( 1 ) // FIXME? shader->sky.fullClouds )
		{
			MIN_T = -HALF_SKY_SUBDIVISIONS;

			// still don't want to draw the bottom, even if fullClouds
			if ( i == 5 )
				continue;
		}
		else
		{
			switch( i )
			{
			case 0:
			case 1:
			case 2:
			case 3:
				MIN_T = -1;
				break;
			case 5:
				// don't draw clouds beneath you
				continue;
			case 4:		// top
			default:
				MIN_T = -HALF_SKY_SUBDIVISIONS;
				break;
			}
		}

		sky_mins[0][i] = floor( sky_mins[0][i] * HALF_SKY_SUBDIVISIONS ) / HALF_SKY_SUBDIVISIONS;
		sky_mins[1][i] = floor( sky_mins[1][i] * HALF_SKY_SUBDIVISIONS ) / HALF_SKY_SUBDIVISIONS;
		sky_maxs[0][i] = ceil( sky_maxs[0][i] * HALF_SKY_SUBDIVISIONS ) / HALF_SKY_SUBDIVISIONS;
		sky_maxs[1][i] = ceil( sky_maxs[1][i] * HALF_SKY_SUBDIVISIONS ) / HALF_SKY_SUBDIVISIONS;

		if ( ( sky_mins[0][i] >= sky_maxs[0][i] ) ||
			 ( sky_mins[1][i] >= sky_maxs[1][i] ) )
		{
			continue;
		}

		sky_mins_subd[0] = Q_ftol(sky_mins[0][i] * HALF_SKY_SUBDIVISIONS);
		sky_mins_subd[1] = Q_ftol(sky_mins[1][i] * HALF_SKY_SUBDIVISIONS);
		sky_maxs_subd[0] = Q_ftol(sky_maxs[0][i] * HALF_SKY_SUBDIVISIONS);
		sky_maxs_subd[1] = Q_ftol(sky_maxs[1][i] * HALF_SKY_SUBDIVISIONS);

		if ( sky_mins_subd[0] < -HALF_SKY_SUBDIVISIONS ) 
			sky_mins_subd[0] = -HALF_SKY_SUBDIVISIONS;
		else if ( sky_mins_subd[0] > HALF_SKY_SUBDIVISIONS ) 
			sky_mins_subd[0] = HALF_SKY_SUBDIVISIONS;
		if ( sky_mins_subd[1] < MIN_T )
			sky_mins_subd[1] = MIN_T;
		else if ( sky_mins_subd[1] > HALF_SKY_SUBDIVISIONS ) 
			sky_mins_subd[1] = HALF_SKY_SUBDIVISIONS;

		if ( sky_maxs_subd[0] < -HALF_SKY_SUBDIVISIONS ) 
			sky_maxs_subd[0] = -HALF_SKY_SUBDIVISIONS;
		else if ( sky_maxs_subd[0] > HALF_SKY_SUBDIVISIONS ) 
			sky_maxs_subd[0] = HALF_SKY_SUBDIVISIONS;
		if ( sky_maxs_subd[1] < MIN_T )
			sky_maxs_subd[1] = MIN_T;
		else if ( sky_maxs_subd[1] > HALF_SKY_SUBDIVISIONS ) 
			sky_maxs_subd[1] = HALF_SKY_SUBDIVISIONS;

		//
		// iterate through the subdivisions
		//
		for ( t = sky_mins_subd[1]+HALF_SKY_SUBDIVISIONS; t <= sky_maxs_subd[1]+HALF_SKY_SUBDIVISIONS; t++ )
		{
			for ( s = sky_mins_subd[0]+HALF_SKY_SUBDIVISIONS; s <= sky_maxs_subd[0]+HALF_SKY_SUBDIVISIONS; s++ )
			{
				MakeSkyVec( ( s - HALF_SKY_SUBDIVISIONS ) / ( float ) HALF_SKY_SUBDIVISIONS, 
							( t - HALF_SKY_SUBDIVISIONS ) / ( float ) HALF_SKY_SUBDIVISIONS, 
							i, 
							NULL,
							s_skyPoints[t][s] );

				s_skyTexCoords[t][s][0] = s_cloudTexCoords[i][t][s][0];
				s_skyTexCoords[t][s][1] = s_cloudTexCoords[i][t][s][1];
			}
		}

		// only add indexes for first stage
		FillCloudySkySide( sky_mins_subd, sky_maxs_subd, (qboolean)( stage == 0 ) );
	}
}

/*
** R_BuildCloudData
*/
void R_BuildCloudData( shaderCommands_t *input )
{
	int			i;
	shader_t	*shader;

	shader = input->shader;

	assert( shader->isSky );

	sky_min = 1.0 / 256.0f;		// FIXME: not correct?
	sky_max = 255.0 / 256.0f;

	// set up for drawing
	tess.numIndexes = 0;
	tess.numVertexes = 0;
	tess.firstIndex = 0;

	if ( shader->sky.cloudHeight )
	{
		for ( i = 0; i < MAX_SHADER_STAGES; i++ )
		{
			if ( !tess.xstages[i] ) {
				break;
			}
			FillCloudBox( shader, i );
		}
	}
}

/*
** R_InitSkyTexCoords
** Called when a sky shader is parsed
*/
#define SQR( a ) ((a)*(a))
void R_InitSkyTexCoords( float heightCloud )
{
	int i, s, t;
	float radiusWorld = 4096;
	float p;
	float sRad, tRad;
	vec3_t skyVec;
	vec3_t v;

	// init zfar so MakeSkyVec works even though
	// a world hasn't been bounded
	backEnd.viewParms.zFar = 1024;

	for ( i = 0; i < 6; i++ )
	{
		for ( t = 0; t <= SKY_SUBDIVISIONS; t++ )
		{
			for ( s = 0; s <= SKY_SUBDIVISIONS; s++ )
			{
				// compute vector from view origin to sky side integral point
				MakeSkyVec( ( s - HALF_SKY_SUBDIVISIONS ) / ( float ) HALF_SKY_SUBDIVISIONS, 
							( t - HALF_SKY_SUBDIVISIONS ) / ( float ) HALF_SKY_SUBDIVISIONS, 
							i, 
							NULL,
							skyVec );

				// compute parametric value 'p' that intersects with cloud layer
				p = ( 1.0f / ( 2 * DotProduct( skyVec, skyVec ) ) ) *
					( -2 * skyVec[2] * radiusWorld + 
					   2 * sqrt( SQR( skyVec[2] ) * SQR( radiusWorld ) + 
					             2 * SQR( skyVec[0] ) * radiusWorld * heightCloud +
								 SQR( skyVec[0] ) * SQR( heightCloud ) + 
								 2 * SQR( skyVec[1] ) * radiusWorld * heightCloud +
								 SQR( skyVec[1] ) * SQR( heightCloud ) + 
								 2 * SQR( skyVec[2] ) * radiusWorld * heightCloud +
								 SQR( skyVec[2] ) * SQR( heightCloud ) ) );

				s_cloudTexP[i][t][s] = p;

				// compute intersection point based on p
				VectorScale( skyVec, p, v );
				v[2] += radiusWorld;

				// compute vector from world origin to intersection point 'v'
				VectorNormalize( v );

				sRad = Q_acos( v[0] );
				tRad = Q_acos( v[1] );

				s_cloudTexCoords[i][t][s][0] = sRad;
				s_cloudTexCoords[i][t][s][1] = tRad;
			}
		}
	}
}

//======================================================================================

vec3_t		SUN_POSITION;
vec2_t		SUN_SCREEN_POSITION;
qboolean	SUN_VISIBLE = qfalse;

#ifdef __DAY_NIGHT__
extern float DAY_NIGHT_CURRENT_TIME;
#endif //__DAY_NIGHT__

extern vec3_t VOLUMETRIC_ROOF;

extern qboolean RB_UpdateSunFlareVis(void);
extern qboolean Volumetric_Visible(vec3_t from, vec3_t to, qboolean isSun);
extern void Volumetric_RoofHeight(vec3_t from);

/*
** RB_DrawSun
*/
void RB_DrawSun( float scale, shader_t *shader ) {
	float		size;
	float		dist;
	vec3_t		origin, vec1, vec2;

	if ( !backEnd.skyRenderedThisView ) {
		return;
	}

#ifdef __DAY_NIGHT__
	if (DAY_NIGHT_CYCLE_ENABLED)
	{
		float Time24h = DAY_NIGHT_CURRENT_TIME*24.0;

		if (Time24h < 6.0 || Time24h > 22.0)
		{
			SUN_VISIBLE = qfalse;
			return;
		}
	}
#endif //__DAY_NIGHT__

	//qglLoadMatrixf( backEnd.viewParms.world.modelMatrix );
	//qglTranslatef (backEnd.viewParms.ori.origin[0], backEnd.viewParms.ori.origin[1], backEnd.viewParms.ori.origin[2]);
	{
		// FIXME: this could be a lot cleaner
		matrix_t translation, modelview;

		Matrix16Translation( backEnd.viewParms.ori.origin, translation );
		Matrix16Multiply( backEnd.viewParms.world.modelMatrix, translation, modelview );
		GL_SetModelviewMatrix( modelview );
	}

	dist = 	backEnd.viewParms.zFar / 1.75;		// div sqrt(3)
	//dist = 32768.0;
	size = dist * scale;

	//VectorSet(tr.sunDirection, r_testshaderValue1->value, r_testshaderValue2->value, r_testshaderValue3->value);
	VectorScale( tr.sunDirection, dist, origin );

	PerpendicularVector( vec1, tr.sunDirection );
	CrossProduct( tr.sunDirection, vec1, vec2 );

	VectorScale( vec1, size, vec1 );
	VectorScale( vec2, size, vec2 );

	// farthest depth range
	qglDepthRange( 1.0, 1.0 );

	RB_BeginSurface( shader, 0, 0 );

	RB_AddQuadStamp(origin, vec1, vec2, tr.refdef.sunAmbCol/*colorWhite*/);

	RB_EndSurface();

	// back to normal depth range
	qglDepthRange( 0.0, 1.0 );

	if (r_dynamiclight->integer)
	{// Lets have some volumetrics with that!
		const float cutoff = 0.25f;
		float dot = DotProduct(tr.sunDirection, backEnd.viewParms.ori.axis[0]);

		float dist;
		vec4_t pos, hpos;
		matrix_t trans, model, mvp;

		Matrix16Translation( backEnd.viewParms.ori.origin, trans );
		Matrix16Multiply( backEnd.viewParms.world.modelMatrix, trans, model );
		Matrix16Multiply(backEnd.viewParms.projectionMatrix, model, mvp);

		//dist = backEnd.viewParms.zFar / 1.75;		// div sqrt(3)
		dist = 4096.0;
		//dist = 32768.0;

		VectorScale( tr.sunDirection, dist, pos );

		VectorCopy(pos, SUN_POSITION);
	
		// project sun point
		Matrix16Transform(mvp, pos, hpos);

		// transform to UV coords
		hpos[3] = 0.5f / hpos[3];

		pos[0] = 0.5f + hpos[0] * hpos[3];
		pos[1] = 0.5f + hpos[1] * hpos[3];

		VectorCopy(pos, SUN_SCREEN_POSITION);
		
		if (dot < cutoff)
		{
			SUN_VISIBLE = qfalse;
			return;
		}

		if (!RB_UpdateSunFlareVis())
		{
			SUN_VISIBLE = qfalse;
			return;
		}

		if (!Volumetric_Visible(backEnd.refdef.vieworg, SUN_POSITION, qtrue))
		{// Trace to actual position failed... Try above...
			vec3_t tmpOrg;
			vec3_t eyeOrg;
			vec3_t tmpRoof;
			vec3_t eyeRoof;

			// Calculate ceiling heights at both positions...
			//Volumetric_RoofHeight(SUN_POSITION);
			//VectorCopy(VOLUMETRIC_ROOF, tmpRoof);
			//Volumetric_RoofHeight(backEnd.refdef.vieworg);
			//VectorCopy(VOLUMETRIC_ROOF, eyeRoof);

			VectorSet(tmpRoof, SUN_POSITION[0], SUN_POSITION[1], SUN_POSITION[2] + 512.0);
			VectorSet(eyeRoof, backEnd.refdef.vieworg[0], backEnd.refdef.vieworg[1], backEnd.refdef.vieworg[2] + 128.0);
			
			VectorSet(tmpOrg, tmpRoof[0], SUN_POSITION[1], SUN_POSITION[2]);
			VectorSet(eyeOrg, backEnd.refdef.vieworg[0], backEnd.refdef.vieworg[1], backEnd.refdef.vieworg[2]);
			if (!Volumetric_Visible(eyeOrg, tmpOrg, qtrue))
			{// Trace to above position failed... Try trace from above viewer...
				VectorSet(tmpOrg, SUN_POSITION[0], SUN_POSITION[1], SUN_POSITION[2]);
				VectorSet(eyeOrg, eyeRoof[0], backEnd.refdef.vieworg[1], backEnd.refdef.vieworg[2]);
				if (!Volumetric_Visible(eyeOrg, tmpOrg, qtrue))
				{// Trace from above viewer failed... Try trace from above, to above...
					VectorSet(tmpOrg, tmpRoof[0], SUN_POSITION[1], SUN_POSITION[2]);
					VectorSet(eyeOrg, eyeRoof[0], backEnd.refdef.vieworg[1], backEnd.refdef.vieworg[2]);
					if (!Volumetric_Visible(eyeOrg, tmpOrg, qtrue))
					{// Trace from/to above viewer failed...
						SUN_VISIBLE = qfalse;
						return; // Can't see this...
					}
				}
			}
		}

		SUN_VISIBLE = qtrue;
	}
}


void DrawSkyDome ( shader_t *skyShader )
{
	vec4_t color;

	// bloom
	color[0] =
		color[1] =
		color[2] = pow(2, r_cameraExposure->value);
	color[3] = 1.0f;

	GLSL_BindProgram(&tr.uniqueskyShader);
	GL_BindToTMU(skyShader->sky.outerbox[0], TB_LEVELSMAP);

	/*
	matrix_t trans, model, mvp, invTrans;

	Matrix16Translation( backEnd.viewParms.ori.origin, trans );
	Matrix16Multiply( backEnd.viewParms.world.modelMatrix, trans, model );
	Matrix16Multiply(backEnd.viewParms.projectionMatrix, model, mvp);
	
	Matrix16SimpleInverse( trans, invTrans);
					
	GLSL_SetUniformMatrix16(&tr.uniqueskyShader, UNIFORM_PROJECTIONMATRIX, glState.projection);
	GLSL_SetUniformMatrix16(&tr.uniqueskyShader, UNIFORM_MODELVIEWMATRIX, model);
	GLSL_SetUniformMatrix16(&tr.uniqueskyShader, UNIFORM_VIEWMATRIX, trans);
	GLSL_SetUniformMatrix16(&tr.uniqueskyShader, UNIFORM_INVVIEWMATRIX, invTrans);
	*/

	matrix_t trans, model, mvp, invMvp, normalMatrix;

	Matrix16Translation( backEnd.viewParms.ori.origin, trans );
	Matrix16Multiply( backEnd.viewParms.world.modelMatrix, trans, model );
	Matrix16Multiply(backEnd.viewParms.projectionMatrix, model, mvp);
	Matrix16SimpleInverse( mvp, invMvp);
	Matrix16SimpleInverse( model, normalMatrix);
	
	//mat4 normalMatrix = transpose(inverse(modelView));

	GLSL_SetUniformMatrix16(&tr.uniqueskyShader, UNIFORM_MODELVIEWPROJECTIONMATRIX, glState.modelviewProjection);
	GLSL_SetUniformMatrix16(&tr.uniqueskyShader, UNIFORM_MODELVIEWMATRIX, model);
	GLSL_SetUniformMatrix16(&tr.uniqueskyShader, UNIFORM_INVPROJECTIONMATRIX, invMvp);
	GLSL_SetUniformMatrix16(&tr.uniqueskyShader, UNIFORM_NORMALMATRIX, normalMatrix);

	GLSL_SetUniformFloat(&tr.uniqueskyShader, UNIFORM_TIME, backEnd.refdef.floatTime);
	GLSL_SetUniformVec3(&tr.uniqueskyShader, UNIFORM_VIEWORIGIN,  backEnd.refdef.vieworg);

	GLSL_SetUniformVec3(&tr.uniqueskyShader, UNIFORM_PRIMARYLIGHTAMBIENT, backEnd.refdef.sunAmbCol);
	GLSL_SetUniformVec3(&tr.uniqueskyShader, UNIFORM_PRIMARYLIGHTCOLOR,   backEnd.refdef.sunCol);
	GLSL_SetUniformVec4(&tr.uniqueskyShader, UNIFORM_PRIMARYLIGHTORIGIN,  backEnd.refdef.sunDir);

	vec4_t l0;
	VectorSet4(l0, r_testshaderValue1->value, r_testshaderValue2->value, r_testshaderValue3->value, r_testshaderValue4->value);
	GLSL_SetUniformVec4(&tr.uniqueskyShader, UNIFORM_LOCAL0, l0);

	vec4_t l1;
	VectorSet4(l1, r_testshaderValue5->value, r_testshaderValue6->value, r_testshaderValue7->value, r_testshaderValue8->value);
	GLSL_SetUniformVec4(&tr.uniqueskyShader, UNIFORM_LOCAL1, l1);

	if (skyShader->sky.outerbox[0])
	{
		vec2_t screensize;
		screensize[0] = skyShader->sky.outerbox[0]->width;
		screensize[1] = skyShader->sky.outerbox[0]->height;

		GLSL_SetUniformVec2(&tr.uniqueskyShader, UNIFORM_DIMENSIONS, screensize);

		vec4i_t		imageBox;
		imageBox[0] = 0;
		imageBox[1] = 0;
		imageBox[2] = skyShader->sky.outerbox[0]->width;
		imageBox[3] = skyShader->sky.outerbox[0]->height;

		vec4i_t		screenBox;
		screenBox[0] = 0;
		screenBox[1] = 0;
		screenBox[2] = glConfig.vidWidth * r_superSampleMultiplier->value;
		screenBox[3] = glConfig.vidHeight * r_superSampleMultiplier->value;

		FBO_BlitFromTexture(skyShader->sky.outerbox[0], imageBox, NULL, glState.currentFBO, screenBox, &tr.uniqueskyShader, NULL, 0);
	}
	else
	{
		vec4i_t		imageBox;
		imageBox[0] = 0;
		imageBox[1] = 0;
		imageBox[2] = tr.whiteImage->width;
		imageBox[3] = tr.whiteImage->height;

		vec4i_t		screenBox;
		screenBox[0] = 0;
		screenBox[1] = 0;
		screenBox[2] = glConfig.vidWidth * r_superSampleMultiplier->value;
		screenBox[3] = glConfig.vidHeight * r_superSampleMultiplier->value;

		FBO_BlitFromTexture(tr.whiteImage, imageBox, NULL, glState.currentFBO, screenBox, &tr.uniqueskyShader, NULL, 0);
	}
}

/*
================
RB_StageIteratorSky

All of the visible sky triangles are in tess

Other things could be stuck in here, like birds in the sky, etc
================
*/

image_t *skyImage = NULL;

//#define ___FORCED_SKYDOME___

void RB_StageIteratorSky( void ) {
#ifndef ___FORCED_SKYDOME___
	if ( r_fastsky->integer ) {
		return;
	}

	// VOID REMOVE HACK
	//int clearBits = GL_COLOR_BUFFER_BIT;	// FIXME: only if sky shaders have been used
	//qglClearColor( 0.0f, 0.0f, 0.0f, 1.0f );	// FIXME: get color of sky
	//qglClear( clearBits );

	// go through all the polygons and project them onto
	// the sky box to see which blocks on each side need
	// to be drawn
	if ( tess.shader->sky.outerbox[0] && tess.shader->sky.outerbox[0] != tr.defaultImage )
	{
		RB_ClipSkyPolygons( &tess );
	}

	// r_showsky will let all the sky blocks be drawn in
	// front of everything to allow developers to see how
	// much sky is getting sucked in
	if ( r_showsky->integer ) {
		qglDepthRange( 0.0, 0.0 );
	} else {
		qglDepthRange( 1.0, 1.0 );
	}

	if ( !tess.shader->sky.outerbox[0] || tess.shader->sky.outerbox[0] == tr.defaultImage ) 
	{// UQ1: Set a default image...
		GL_State( 0 );
		DrawSkyDome(tess.shader);
	}
	else
	{// draw the outer skybox
		matrix_t oldmodelview;

		skyImage = tess.shader->sky.outerbox[r_skynum->integer];
		
		GL_State( 0 );
		//qglTranslatef (backEnd.viewParms.ori.origin[0], backEnd.viewParms.ori.origin[1], backEnd.viewParms.ori.origin[2]);

		{
			// FIXME: this could be a lot cleaner
			matrix_t trans, product;

			Matrix16Copy( glState.modelview, oldmodelview );
			Matrix16Translation( backEnd.viewParms.ori.origin, trans );
			Matrix16Multiply( glState.modelview, trans, product );
			GL_SetModelviewMatrix( product );

		}

		DrawSkyBox( tess.shader );

		GL_SetModelviewMatrix( oldmodelview );
	}

#else //___FORCED_SKYDOME___
	DrawSkyDome(tess.shader);
#endif //___FORCED_SKYDOME___


	// generate the vertexes for all the clouds, which will be drawn
	// by the generic shader routine
	R_BuildCloudData( &tess );
	RB_StageIteratorGeneric();

	// draw the inner skybox


	// back to normal depth range
	qglDepthRange( 0.0, 1.0 );

	// note that sky was drawn so we will draw a sun later
	backEnd.skyRenderedThisView = qtrue;
}





