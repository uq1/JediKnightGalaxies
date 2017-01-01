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
// tr_shade.c

#include "tr_local.h"

/*

  THIS ENTIRE FILE IS BACK END

  This file deals with applying shaders to surface data in the tess struct.
*/

qboolean MATRIX_UPDATE = qtrue;
qboolean CLOSE_LIGHTS_UPDATE = qtrue;

color4ub_t	styleColors[MAX_LIGHT_STYLES];

extern void RB_DrawSurfaceSprites( shaderStage_t *stage, shaderCommands_t *input);

extern qboolean RB_CheckOcclusion(matrix_t MVP, shaderCommands_t *input);

/*
==================
R_DrawElements

==================
*/

void TesselatedGlDrawElements( int numIndexes, glIndex_t firstIndex, glIndex_t minIndex, glIndex_t maxIndex )
{// Would really suck if this is the only way... I hate opengl...
	GLint MaxPatchVertices = 0;
	qglGetIntegerv(GL_MAX_PATCH_VERTICES, &MaxPatchVertices);

	for (int i = 0; i < maxIndex-minIndex; i++)
	{
		qglPatchParameteri(GL_PATCH_VERTICES, 3);
		qglDrawElements(GL_PATCHES, 3, GL_INDEX_TYPE, BUFFER_OFFSET((firstIndex+i) * sizeof(glIndex_t)));
	}
}

void R_DrawElementsVBO( int numIndexes, glIndex_t firstIndex, glIndex_t minIndex, glIndex_t maxIndex, glIndex_t numVerts, qboolean tesselation )
{
	if (r_tesselation->integer && tesselation)
	{
		GLint MaxPatchVertices = 0;
		qglGetIntegerv(GL_MAX_PATCH_VERTICES, &MaxPatchVertices);
		//printf("Max supported patch vertices %d\n", MaxPatchVertices);
		qglPatchParameteri(GL_PATCH_VERTICES, 3);
		qglDrawRangeElements(GL_PATCHES, minIndex, maxIndex, numIndexes, GL_INDEX_TYPE, BUFFER_OFFSET(firstIndex * sizeof(glIndex_t)));

		//TesselatedGlDrawElements( numIndexes, firstIndex, minIndex, maxIndex );
	}
	else
	{
		qglDrawRangeElements(GL_TRIANGLES, minIndex, maxIndex, numIndexes, GL_INDEX_TYPE, BUFFER_OFFSET(firstIndex * sizeof(glIndex_t)));
	}
}

void TesselatedGlMultiDrawElements( GLenum mode, GLsizei *count, GLenum type, const GLvoid **indices, GLsizei primcount )
{// Would really suck if this is the only way... I hate opengl...
	GLint MaxPatchVertices = 0;
	qglGetIntegerv(GL_MAX_PATCH_VERTICES, &MaxPatchVertices);

	for (int i = 0; i < primcount; i++)
	{
		if (count[i] > 0)
		{
			qglPatchParameteri(GL_PATCH_VERTICES, 3);
			qglDrawElements(mode, count[i], type, indices[i]);
		}
	}
}

void R_DrawMultiElementsVBO( int multiDrawPrimitives, glIndex_t *multiDrawMinIndex, glIndex_t *multiDrawMaxIndex,
	GLsizei *multiDrawNumIndexes, glIndex_t **multiDrawFirstIndex, glIndex_t numVerts, qboolean tesselation)
{
	if (r_tesselation->integer && tesselation)
	{
		//TesselatedGlMultiDrawElements( GL_PATCHES, multiDrawNumIndexes, GL_INDEX_TYPE, (const GLvoid **)multiDrawFirstIndex, multiDrawPrimitives );
		GLint MaxPatchVertices = 0;
		qglGetIntegerv(GL_MAX_PATCH_VERTICES, &MaxPatchVertices);
		qglPatchParameteri(GL_PATCH_VERTICES, 3);
		qglMultiDrawElements(GL_PATCHES, multiDrawNumIndexes, GL_INDEX_TYPE, (const GLvoid **)multiDrawFirstIndex, multiDrawPrimitives);
	}
	else
	{
		qglMultiDrawElements(GL_TRIANGLES, multiDrawNumIndexes, GL_INDEX_TYPE, (const GLvoid **)multiDrawFirstIndex, multiDrawPrimitives);
	}
}


/*
=============================================================

SURFACE SHADERS

=============================================================
*/

shaderCommands_t	tess;


/*
=================
R_BindAnimatedImageToTMU

=================
*/
void R_BindAnimatedImageToTMU( textureBundle_t *bundle, int tmu ) {
	int		index;

	if ( bundle->isVideoMap ) {
		int oldtmu = glState.currenttmu;
		GL_SelectTexture(tmu);
		ri->CIN_RunCinematic(bundle->videoMapHandle);
		ri->CIN_UploadCinematic(bundle->videoMapHandle);
		GL_SelectTexture(oldtmu);
		return;
	}

	if ( bundle->numImageAnimations <= 1 ) {
		GL_BindToTMU( bundle->image[0], tmu);
		return;
	}

	if (backEnd.currentEntity->e.renderfx & RF_SETANIMINDEX )
	{
		index = backEnd.currentEntity->e.skinNum;
	}
	else
	{
		// it is necessary to do this messy calc to make sure animations line up
		// exactly with waveforms of the same frequency
		index = Q_ftol( tess.shaderTime * bundle->imageAnimationSpeed * FUNCTABLE_SIZE );
		index >>= FUNCTABLE_SIZE2;

		if ( index < 0 ) {
			index = 0;	// may happen with shader time offsets
		}
	}

	if ( bundle->oneShotAnimMap )
	{
		if ( index >= bundle->numImageAnimations )
		{
			// stick on last frame
			index = bundle->numImageAnimations - 1;
		}
	}
	else
	{
		// loop
		index %= bundle->numImageAnimations;
	}

	GL_BindToTMU( bundle->image[ index ], tmu );
}


/*
================
DrawTris

Draws triangle outlines for debugging
================
*/
static void DrawTris (shaderCommands_t *input) {
	GL_Bind( tr.whiteImage );

	GL_State( GLS_POLYMODE_LINE | GLS_DEPTHMASK_TRUE );
	qglDepthRange( 0, 0 );

	{
		shaderProgram_t *sp = &tr.textureColorShader;
		vec4_t color;

		GLSL_VertexAttribsState(ATTR_POSITION);
		GLSL_BindProgram(sp);

		GLSL_SetUniformMatrix16(sp, UNIFORM_MODELVIEWPROJECTIONMATRIX, glState.modelviewProjection);
		VectorSet4(color, 1, 1, 1, 1);
		GLSL_SetUniformVec4(sp, UNIFORM_COLOR, color);

		if (input->multiDrawPrimitives)
		{
			R_DrawMultiElementsVBO(input->multiDrawPrimitives, input->multiDrawMinIndex, input->multiDrawMaxIndex, input->multiDrawNumIndexes, input->multiDrawFirstIndex, input->numVertexes, qfalse);
		}
		else
		{
			R_DrawElementsVBO(input->numIndexes, input->firstIndex, input->minIndex, input->maxIndex, input->numVertexes, qfalse);
		}
	}

	qglDepthRange( 0, 1 );
}


/*
================
DrawNormals

Draws vertex normals for debugging
================
*/
static void DrawNormals (shaderCommands_t *input) {
	//FIXME: implement this
}

/*
==============
RB_BeginSurface

We must set some things up before beginning any tesselation,
because a surface may be forced to perform a RB_End due
to overflow.
==============
*/
void RB_BeginSurface( shader_t *shader, int fogNum, int cubemapIndex ) {

	shader_t *state = (shader->remappedShader) ? shader->remappedShader : shader;

	tess.numIndexes = 0;
	tess.firstIndex = 0;
	tess.numVertexes = 0;
	tess.multiDrawPrimitives = 0;
	tess.shader = state;
	tess.fogNum = fogNum;
	tess.cubemapIndex = cubemapIndex;
	//tess.dlightBits = 0;		// will be OR'd in by surface functions
#ifdef __PSHADOWS__
	tess.pshadowBits = 0;       // will be OR'd in by surface functions
#endif
	tess.xstages = state->stages;
	tess.numPasses = state->numUnfoggedPasses;
	tess.currentStageIteratorFunc = state->optimalStageIteratorFunc;
	tess.useInternalVBO = qtrue;

	if (backEnd.depthFill || (tr.viewParms.flags & VPF_SHADOWPASS))
		tess.cubemapIndex = cubemapIndex = 0;

	tess.shaderTime = backEnd.refdef.floatTime - tess.shader->timeOffset;
	if (tess.shader->clampTime && tess.shaderTime >= tess.shader->clampTime) {
		tess.shaderTime = tess.shader->clampTime;
	}

	if (backEnd.viewParms.flags & VPF_SHADOWMAP)
	{
		tess.currentStageIteratorFunc = RB_StageIteratorGeneric;
	}
}



extern float EvalWaveForm( const waveForm_t *wf );
extern float EvalWaveFormClamped( const waveForm_t *wf );


static void ComputeTexMods( shaderStage_t *pStage, int bundleNum, float *outMatrix, float *outOffTurb)
{
	int tm;
	float matrix[6], currentmatrix[6];
	textureBundle_t *bundle = &pStage->bundle[bundleNum];

	matrix[0] = 1.0f; matrix[2] = 0.0f; matrix[4] = 0.0f;
	matrix[1] = 0.0f; matrix[3] = 1.0f; matrix[5] = 0.0f;

	currentmatrix[0] = 1.0f; currentmatrix[2] = 0.0f; currentmatrix[4] = 0.0f;
	currentmatrix[1] = 0.0f; currentmatrix[3] = 1.0f; currentmatrix[5] = 0.0f;

	outMatrix[0] = 1.0f; outMatrix[2] = 0.0f;
	outMatrix[1] = 0.0f; outMatrix[3] = 1.0f;

	outOffTurb[0] = 0.0f; outOffTurb[1] = 0.0f; outOffTurb[2] = 0.0f; outOffTurb[3] = 0.0f;

	for (tm = 0; tm < bundle->numTexMods; tm++) {
		switch (bundle->texMods[tm].type)
		{

		case TMOD_NONE:
			tm = TR_MAX_TEXMODS;		// break out of for loop
			break;

		case TMOD_TURBULENT:
			RB_CalcTurbulentFactors(&bundle->texMods[tm].wave, &outOffTurb[2], &outOffTurb[3]);
			break;

		case TMOD_ENTITY_TRANSLATE:
			RB_CalcScrollTexMatrix(backEnd.currentEntity->e.shaderTexCoord, matrix);
			break;

		case TMOD_SCROLL:
			RB_CalcScrollTexMatrix(bundle->texMods[tm].scroll,
				matrix);
			break;

		case TMOD_SCALE:
			RB_CalcScaleTexMatrix(bundle->texMods[tm].scale,
				matrix);
			break;

		case TMOD_STRETCH:
			RB_CalcStretchTexMatrix(&bundle->texMods[tm].wave,
				matrix);
			break;

		case TMOD_TRANSFORM:
			RB_CalcTransformTexMatrix(&bundle->texMods[tm],
				matrix);
			break;

		case TMOD_ROTATE:
			RB_CalcRotateTexMatrix(bundle->texMods[tm].rotateSpeed,
				matrix);
			break;

		default:
			ri->Error(ERR_DROP, "ERROR: unknown texmod '%d' in shader '%s'", bundle->texMods[tm].type, tess.shader->name);
			break;
		}

		switch (bundle->texMods[tm].type)
		{
		case TMOD_NONE:
		case TMOD_TURBULENT:
		default:
			break;

		case TMOD_ENTITY_TRANSLATE:
		case TMOD_SCROLL:
		case TMOD_SCALE:
		case TMOD_STRETCH:
		case TMOD_TRANSFORM:
		case TMOD_ROTATE:
			outMatrix[0] = matrix[0] * currentmatrix[0] + matrix[2] * currentmatrix[1];
			outMatrix[1] = matrix[1] * currentmatrix[0] + matrix[3] * currentmatrix[1];

			outMatrix[2] = matrix[0] * currentmatrix[2] + matrix[2] * currentmatrix[3];
			outMatrix[3] = matrix[1] * currentmatrix[2] + matrix[3] * currentmatrix[3];

			outOffTurb[0] = matrix[0] * currentmatrix[4] + matrix[2] * currentmatrix[5] + matrix[4];
			outOffTurb[1] = matrix[1] * currentmatrix[4] + matrix[3] * currentmatrix[5] + matrix[5];

			currentmatrix[0] = outMatrix[0];
			currentmatrix[1] = outMatrix[1];
			currentmatrix[2] = outMatrix[2];
			currentmatrix[3] = outMatrix[3];
			currentmatrix[4] = outOffTurb[0];
			currentmatrix[5] = outOffTurb[1];
			break;
		}
	}
}


static void ComputeDeformValues(int *deformGen, vec5_t deformParams)
{
	// u_DeformGen
	*deformGen = DGEN_NONE;
	if(!ShaderRequiresCPUDeforms(tess.shader))
	{
		deformStage_t  *ds;

		// only support the first one
		ds = &tess.shader->deforms[0];

		switch (ds->deformation)
		{
			case DEFORM_WAVE:
				*deformGen = ds->deformationWave.func;

				deformParams[0] = ds->deformationWave.base;
				deformParams[1] = ds->deformationWave.amplitude;
				deformParams[2] = ds->deformationWave.phase;
				deformParams[3] = ds->deformationWave.frequency;
				deformParams[4] = ds->deformationSpread;
				break;

			case DEFORM_BULGE:
				*deformGen = DGEN_BULGE;

				deformParams[0] = 0;
				deformParams[1] = ds->bulgeHeight; // amplitude
				deformParams[2] = ds->bulgeWidth;  // phase
				deformParams[3] = ds->bulgeSpeed;  // frequency
				deformParams[4] = 0;
				break;

			default:
				break;
		}
	}
}

static void ComputeShaderColors( shaderStage_t *pStage, vec4_t baseColor, vec4_t vertColor, int blend, colorGen_t *forceRGBGen, alphaGen_t *forceAlphaGen )
{
	colorGen_t rgbGen = pStage->rgbGen;
	alphaGen_t alphaGen = pStage->alphaGen;

	baseColor[0] =
   	baseColor[1] =
   	baseColor[2] =
   	baseColor[3] = 1.0f;

   	vertColor[0] =
   	vertColor[1] =
   	vertColor[2] =
   	vertColor[3] = 0.0f;

	if ( forceRGBGen != NULL && *forceRGBGen != CGEN_BAD )
	{
		rgbGen = *forceRGBGen;
	}

	if ( forceAlphaGen != NULL && *forceAlphaGen != AGEN_IDENTITY )
	{
		alphaGen = *forceAlphaGen;
	}

	switch ( rgbGen )
	{
		case CGEN_IDENTITY_LIGHTING:
			baseColor[0] =
			baseColor[1] =
			baseColor[2] = tr.identityLight;
			break;
		case CGEN_EXACT_VERTEX:
		case CGEN_EXACT_VERTEX_LIT:
			baseColor[0] =
			baseColor[1] =
			baseColor[2] =
			baseColor[3] = 0.0f;

			vertColor[0] =
			vertColor[1] =
			vertColor[2] =
			vertColor[3] = 1.0f;
			break;
		case CGEN_CONST:
			baseColor[0] = pStage->constantColor[0] / 255.0f;
			baseColor[1] = pStage->constantColor[1] / 255.0f;
			baseColor[2] = pStage->constantColor[2] / 255.0f;
			baseColor[3] = pStage->constantColor[3] / 255.0f;
			break;
		case CGEN_VERTEX:
			baseColor[0] =
			baseColor[1] =
			baseColor[2] =
			baseColor[3] = 0.0f;

			vertColor[0] =
			vertColor[1] =
			vertColor[2] = tr.identityLight;
			vertColor[3] = 1.0f;
			break;
		case CGEN_VERTEX_LIT:
			baseColor[0] =
			baseColor[1] =
			baseColor[2] =
			baseColor[3] = 0.0f;

			vertColor[0] =
			vertColor[1] =
			vertColor[2] =
			vertColor[3] = tr.identityLight;
			break;
		case CGEN_ONE_MINUS_VERTEX:
			baseColor[0] =
			baseColor[1] =
			baseColor[2] = tr.identityLight;

			vertColor[0] =
			vertColor[1] =
			vertColor[2] = -tr.identityLight;
			break;
		case CGEN_FOG:
			{
				if (!r_fog->integer)
					break;

				fog_t		*fog;

				fog = tr.world->fogs + tess.fogNum;

				baseColor[0] = ((unsigned char *)(&fog->colorInt))[0] / 255.0f;
				baseColor[1] = ((unsigned char *)(&fog->colorInt))[1] / 255.0f;
				baseColor[2] = ((unsigned char *)(&fog->colorInt))[2] / 255.0f;
				baseColor[3] = ((unsigned char *)(&fog->colorInt))[3] / 255.0f;
			}
			break;
		case CGEN_WAVEFORM:
			baseColor[0] =
			baseColor[1] =
			baseColor[2] = RB_CalcWaveColorSingle( &pStage->rgbWave );
			break;
		case CGEN_ENTITY:
		case CGEN_LIGHTING_DIFFUSE_ENTITY:
			if (backEnd.currentEntity)
			{
				baseColor[0] = ((unsigned char *)backEnd.currentEntity->e.shaderRGBA)[0] / 255.0f;
				baseColor[1] = ((unsigned char *)backEnd.currentEntity->e.shaderRGBA)[1] / 255.0f;
				baseColor[2] = ((unsigned char *)backEnd.currentEntity->e.shaderRGBA)[2] / 255.0f;
				baseColor[3] = ((unsigned char *)backEnd.currentEntity->e.shaderRGBA)[3] / 255.0f;

				if ( alphaGen == AGEN_IDENTITY &&
					backEnd.currentEntity->e.shaderRGBA[3] == 255 )
				{
					alphaGen = AGEN_SKIP;
				}
			}
			break;
		case CGEN_ONE_MINUS_ENTITY:
			if (backEnd.currentEntity)
			{
				baseColor[0] = 1.0f - ((unsigned char *)backEnd.currentEntity->e.shaderRGBA)[0] / 255.0f;
				baseColor[1] = 1.0f - ((unsigned char *)backEnd.currentEntity->e.shaderRGBA)[1] / 255.0f;
				baseColor[2] = 1.0f - ((unsigned char *)backEnd.currentEntity->e.shaderRGBA)[2] / 255.0f;
				baseColor[3] = 1.0f - ((unsigned char *)backEnd.currentEntity->e.shaderRGBA)[3] / 255.0f;
			}
			break;
		case CGEN_LIGHTMAPSTYLE:
			VectorScale4 (styleColors[pStage->lightmapStyle], 1.0f / 255.0f, baseColor);
			break;
		case CGEN_IDENTITY:
		case CGEN_LIGHTING_DIFFUSE:
		case CGEN_BAD:
			break;
	}

	//
	// alphaGen
	//
	switch ( alphaGen )
	{
		case AGEN_SKIP:
			break;
		case AGEN_CONST:
			if ( rgbGen != CGEN_CONST ) {
				baseColor[3] = pStage->constantColor[3] / 255.0f;
				vertColor[3] = 0.0f;
			}
			break;
		case AGEN_WAVEFORM:
			baseColor[3] = RB_CalcWaveAlphaSingle( &pStage->alphaWave );
			vertColor[3] = 0.0f;
			break;
		case AGEN_ENTITY:
			if (backEnd.currentEntity)
			{
				baseColor[3] = ((unsigned char *)backEnd.currentEntity->e.shaderRGBA)[3] / 255.0f;
			}
			vertColor[3] = 0.0f;
			break;
		case AGEN_ONE_MINUS_ENTITY:
			if (backEnd.currentEntity)
			{
				baseColor[3] = 1.0f - ((unsigned char *)backEnd.currentEntity->e.shaderRGBA)[3] / 255.0f;
			}
			vertColor[3] = 0.0f;
			break;
		case AGEN_VERTEX:
			if ( rgbGen != CGEN_VERTEX ) {
				baseColor[3] = 0.0f;
				vertColor[3] = 1.0f;
			}
			break;
		case AGEN_ONE_MINUS_VERTEX:
			baseColor[3] = 1.0f;
			vertColor[3] = -1.0f;
			break;
		case AGEN_IDENTITY:
		case AGEN_LIGHTING_SPECULAR:
		case AGEN_PORTAL:
			// Done entirely in vertex program
			baseColor[3] = 1.0f;
			vertColor[3] = 0.0f;
			break;
	}

	if ( forceAlphaGen != NULL )
	{
		*forceAlphaGen = alphaGen;
	}

	if ( forceRGBGen != NULL )
	{
		*forceRGBGen = rgbGen;
	}

	// multiply color by overbrightbits if this isn't a blend
	if (tr.overbrightBits
	 && !((blend & GLS_SRCBLEND_BITS) == GLS_SRCBLEND_DST_COLOR)
	 && !((blend & GLS_SRCBLEND_BITS) == GLS_SRCBLEND_ONE_MINUS_DST_COLOR)
	 && !((blend & GLS_DSTBLEND_BITS) == GLS_DSTBLEND_SRC_COLOR)
	 && !((blend & GLS_DSTBLEND_BITS) == GLS_DSTBLEND_ONE_MINUS_SRC_COLOR))
	{
		float scale = 1 << tr.overbrightBits;

		baseColor[0] *= scale;
		baseColor[1] *= scale;
		baseColor[2] *= scale;
		vertColor[0] *= scale;
		vertColor[1] *= scale;
		vertColor[2] *= scale;
	}

	// FIXME: find some way to implement this.
#if 0
	// if in greyscale rendering mode turn all color values into greyscale.
	if(r_greyscale->integer)
	{
		int scale;

		for(i = 0; i < tess.numVertexes; i++)
		{
			scale = (tess.svars.colors[i][0] + tess.svars.colors[i][1] + tess.svars.colors[i][2]) / 3;
			tess.svars.colors[i][0] = tess.svars.colors[i][1] = tess.svars.colors[i][2] = scale;
		}
	}
#endif
}


static void ComputeFogValues(vec4_t fogDistanceVector, vec4_t fogDepthVector, float *eyeT)
{
	// from RB_CalcFogTexCoords()
	fog_t  *fog;
	vec3_t  local;

	if (!tess.fogNum)
		return;

	if (!r_fog->integer)
		return;

	fog = tr.world->fogs + tess.fogNum;

	VectorSubtract( backEnd.ori.origin, backEnd.viewParms.ori.origin, local );
	fogDistanceVector[0] = -backEnd.ori.modelMatrix[2];
	fogDistanceVector[1] = -backEnd.ori.modelMatrix[6];
	fogDistanceVector[2] = -backEnd.ori.modelMatrix[10];
	fogDistanceVector[3] = DotProduct( local, backEnd.viewParms.ori.axis[0] );

	// scale the fog vectors based on the fog's thickness
	VectorScale4(fogDistanceVector, fog->tcScale, fogDistanceVector);

	// rotate the gradient vector for this orientation
	if ( fog->hasSurface ) {
		fogDepthVector[0] = fog->surface[0] * backEnd.ori.axis[0][0] +
			fog->surface[1] * backEnd.ori.axis[0][1] + fog->surface[2] * backEnd.ori.axis[0][2];
		fogDepthVector[1] = fog->surface[0] * backEnd.ori.axis[1][0] +
			fog->surface[1] * backEnd.ori.axis[1][1] + fog->surface[2] * backEnd.ori.axis[1][2];
		fogDepthVector[2] = fog->surface[0] * backEnd.ori.axis[2][0] +
			fog->surface[1] * backEnd.ori.axis[2][1] + fog->surface[2] * backEnd.ori.axis[2][2];
		fogDepthVector[3] = -fog->surface[3] + DotProduct( backEnd.ori.origin, fog->surface );

		*eyeT = DotProduct( backEnd.ori.viewOrigin, fogDepthVector ) + fogDepthVector[3];
	} else {
		*eyeT = 1;	// non-surface fog always has eye inside
	}
}


static void ComputeFogColorMask( shaderStage_t *pStage, vec4_t fogColorMask )
{
	if (r_fog->integer)
	{
		switch(pStage->adjustColorsForFog)
		{
		case ACFF_MODULATE_RGB:
			fogColorMask[0] =
				fogColorMask[1] =
				fogColorMask[2] = 1.0f;
			fogColorMask[3] = 0.0f;
			break;
		case ACFF_MODULATE_ALPHA:
			fogColorMask[0] =
				fogColorMask[1] =
				fogColorMask[2] = 0.0f;
			fogColorMask[3] = 1.0f;
			break;
		case ACFF_MODULATE_RGBA:
			fogColorMask[0] =
				fogColorMask[1] =
				fogColorMask[2] =
				fogColorMask[3] = 1.0f;
			break;
		default:
			fogColorMask[0] =
				fogColorMask[1] =
				fogColorMask[2] =
				fogColorMask[3] = 0.0f;
			break;
		}
	}
	else
	{
		fogColorMask[0] =
			fogColorMask[1] =
			fogColorMask[2] =
			fogColorMask[3] = 0.0f;
	}
}

static void ProjectPshadowVBOGLSL( void ) {
#ifdef __PSHADOWS__
	int		l;
	vec3_t	origin;
	float	radius;

	int deformGen;
	vec5_t deformParams;

	shaderCommands_t *input = &tess;

	if ( !backEnd.refdef.num_pshadows ) {
		return;
	}

	ComputeDeformValues(&deformGen, deformParams);

	for ( l = 0 ; l < backEnd.refdef.num_pshadows ; l++ ) {
		pshadow_t	*ps;
		shaderProgram_t *sp;
		vec4_t vector;

		//if ( !( tess.pshadowBits & ( 1 << l ) ) ) {
		//	continue;	// this surface definately doesn't have any of this shadow
		//}

		ps = &backEnd.refdef.pshadows[l];
		VectorCopy( ps->lightOrigin, origin );
		radius = ps->lightRadius;

		sp = &tr.pshadowShader;

		GLSL_BindProgram(sp);

		GLSL_SetUniformMatrix16(sp, UNIFORM_MODELVIEWPROJECTIONMATRIX, glState.modelviewProjection);

		VectorCopy(origin, vector);
		vector[3] = 1.0f;
		GLSL_SetUniformVec4(sp, UNIFORM_LIGHTORIGIN, vector);

		VectorScale(ps->lightViewAxis[0], 1.0f / ps->viewRadius, vector);
		GLSL_SetUniformVec3(sp, UNIFORM_LIGHTFORWARD, vector);

		VectorScale(ps->lightViewAxis[1], 1.0f / ps->viewRadius, vector);
		GLSL_SetUniformVec3(sp, UNIFORM_LIGHTRIGHT, vector);

		VectorScale(ps->lightViewAxis[2], 1.0f / ps->viewRadius, vector);
		GLSL_SetUniformVec3(sp, UNIFORM_LIGHTUP, vector);

		GLSL_SetUniformFloat(sp, UNIFORM_LIGHTRADIUS, radius);

		// include GLS_DEPTHFUNC_EQUAL so alpha tested surfaces don't add light
		// where they aren't rendered
		GL_State( GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_DEPTHFUNC_EQUAL );

		GL_BindToTMU( tr.pshadowMaps[l], TB_DIFFUSEMAP );

		//
		// draw
		//

		if (input->multiDrawPrimitives)
		{
			R_DrawMultiElementsVBO(input->multiDrawPrimitives, input->multiDrawMinIndex, input->multiDrawMaxIndex, input->multiDrawNumIndexes, input->multiDrawFirstIndex, input->numVertexes, qfalse);
		}
		else
		{
			R_DrawElementsVBO(input->numIndexes, input->firstIndex, input->minIndex, input->maxIndex, input->numVertexes, qfalse);
		}

		backEnd.pc.c_totalIndexes += tess.numIndexes;
		//backEnd.pc.c_dlightIndexes += tess.numIndexes;
	}
#endif
}



/*
===================
RB_FogPass

Blends a fog texture on top of everything else
===================
*/
static void RB_FogPass( void ) {
	fog_t		*fog;
	vec4_t  color;
	vec4_t	fogDistanceVector, fogDepthVector = {0, 0, 0, 0};
	float	eyeT = 0;
	shaderProgram_t *sp;

	int deformGen;
	vec5_t deformParams;

	ComputeDeformValues(&deformGen, deformParams);

	{
		int index = 0;

		if (deformGen != DGEN_NONE)
			index |= FOGDEF_USE_DEFORM_VERTEXES;

		if (glState.vertexAnimation)
			index |= FOGDEF_USE_VERTEX_ANIMATION;

		if (glState.skeletalAnimation)
			index |= FOGDEF_USE_SKELETAL_ANIMATION;

		sp = &tr.fogShader[index];
	}

	backEnd.pc.c_fogDraws++;

	GLSL_BindProgram(sp);

	fog = tr.world->fogs + tess.fogNum;

	GLSL_SetUniformMatrix16(sp, UNIFORM_MODELVIEWPROJECTIONMATRIX, glState.modelviewProjection);

	GLSL_SetUniformMatrix16(sp, UNIFORM_BONE_MATRICES, &glState.boneMatrices[0][0], glState.numBones);
	GLSL_SetUniformFloat(sp, UNIFORM_VERTEXLERP, glState.vertexAttribsInterpolation);

	GLSL_SetUniformInt(sp, UNIFORM_DEFORMGEN, deformGen);
	if (deformGen != DGEN_NONE)
	{
		GLSL_SetUniformFloat5(sp, UNIFORM_DEFORMPARAMS, deformParams);
		GLSL_SetUniformFloat(sp, UNIFORM_TIME, tess.shaderTime);
	}

	color[0] = ((unsigned char *)(&fog->colorInt))[0] / 255.0f;
	color[1] = ((unsigned char *)(&fog->colorInt))[1] / 255.0f;
	color[2] = ((unsigned char *)(&fog->colorInt))[2] / 255.0f;
	color[3] = ((unsigned char *)(&fog->colorInt))[3] / 255.0f;
	GLSL_SetUniformVec4(sp, UNIFORM_COLOR, color);

	ComputeFogValues(fogDistanceVector, fogDepthVector, &eyeT);

	GLSL_SetUniformVec4(sp, UNIFORM_FOGDISTANCE, fogDistanceVector);
	GLSL_SetUniformVec4(sp, UNIFORM_FOGDEPTH, fogDepthVector);
	GLSL_SetUniformFloat(sp, UNIFORM_FOGEYET, eyeT);

	if ( tess.shader->fogPass == FP_EQUAL ) {
		GL_State( GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_DEPTHFUNC_EQUAL );
	} else {
		GL_State( GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );
	}

	if (tess.multiDrawPrimitives)
	{
		shaderCommands_t *input = &tess;
		R_DrawMultiElementsVBO(input->multiDrawPrimitives, input->multiDrawMinIndex, input->multiDrawMaxIndex, input->multiDrawNumIndexes, input->multiDrawFirstIndex, input->numVertexes, qfalse);
	}
	else
	{
		R_DrawElementsVBO(tess.numIndexes, tess.firstIndex, tess.minIndex, tess.maxIndex, tess.numVertexes, qfalse);
	}
}


static unsigned int RB_CalcShaderVertexAttribs( const shader_t *shader )
{
	unsigned int vertexAttribs = shader->vertexAttribs;

	if(glState.vertexAnimation)
	{
		vertexAttribs &= ~ATTR_COLOR;
		vertexAttribs |= ATTR_POSITION2;
		if (vertexAttribs & ATTR_NORMAL)
		{
			vertexAttribs |= ATTR_NORMAL2;
			vertexAttribs |= ATTR_TANGENT2;
		}
	}

	if (glState.skeletalAnimation)
	{
		vertexAttribs |= ATTR_BONE_WEIGHTS;
		vertexAttribs |= ATTR_BONE_INDEXES;
	}

	return vertexAttribs;
}

static void UpdateTexCoords ( const shaderStage_t *stage )
{
	uint32_t updateAttribs = 0;
	if ( stage->bundle[0].image[0] != NULL )
	{
		switch (stage->bundle[0].tcGen)
		{
			case TCGEN_LIGHTMAP:
			case TCGEN_LIGHTMAP1:
			case TCGEN_LIGHTMAP2:
			case TCGEN_LIGHTMAP3:
			{
				int newLightmapIndex = stage->bundle[0].tcGen - TCGEN_LIGHTMAP + 1;
				if (newLightmapIndex != glState.vertexAttribsTexCoordOffset[0])
				{
					glState.vertexAttribsTexCoordOffset[0] = newLightmapIndex;
					updateAttribs |= ATTR_TEXCOORD0;
				}

				break;
			}

			case TCGEN_TEXTURE:
				if (glState.vertexAttribsTexCoordOffset[0] != 0)
				{
					glState.vertexAttribsTexCoordOffset[0] = 0;
					updateAttribs |= ATTR_TEXCOORD0;
				}
				break;

			default:
				break;
		}
	}

	if ( stage->bundle[TB_LIGHTMAP].image[0] != NULL )
	{
		switch (stage->bundle[TB_LIGHTMAP].tcGen)
		{
			case TCGEN_LIGHTMAP:
			case TCGEN_LIGHTMAP1:
			case TCGEN_LIGHTMAP2:
			case TCGEN_LIGHTMAP3:
			{
				int newLightmapIndex = stage->bundle[TB_LIGHTMAP].tcGen - TCGEN_LIGHTMAP + 1;
				if (newLightmapIndex != glState.vertexAttribsTexCoordOffset[1])
				{
					glState.vertexAttribsTexCoordOffset[1] = newLightmapIndex;
					updateAttribs |= ATTR_TEXCOORD1;
				}

				break;
			}

			case TCGEN_TEXTURE:
				if (glState.vertexAttribsTexCoordOffset[1] != 0)
				{
					glState.vertexAttribsTexCoordOffset[1] = 0;
					updateAttribs |= ATTR_TEXCOORD1;
				}
				break;

			default:
				break;
		}
	}

	if ( updateAttribs != 0 )
	{
		GLSL_UpdateTexCoordVertexAttribPointers (updateAttribs);
	}
}


int			overlaySwayTime = 0;
qboolean	overlaySwayDown = qfalse;
float		overlaySway = 0.0;

void RB_AdvanceOverlaySway ( void )
{
	if (overlaySwayTime > ri->Milliseconds())
		return;

	if (overlaySwayDown)
	{
		overlaySway -= 0.00016;

		if (overlaySway < 0.0)
		{
			overlaySway += 0.00032;
			overlaySwayDown = qfalse;
		}
	}
	else
	{
		overlaySway += 0.00016;

		if (overlaySway > 0.0016)
		{
			overlaySway -= 0.00032;
			overlaySwayDown = qtrue;
		}
	}

	overlaySwayTime = ri->Milliseconds() + 50;
}

extern float MAP_WATER_LEVEL;
extern float MAP_INFO_MAXSIZE;

void RB_SetMaterialBasedProperties(shaderProgram_t *sp, shaderStage_t *pStage)
{
	vec4_t	local1, local3, local4, local5;
	float	specularScale = 1.0;
	float	materialType = 0.0;
	float   parallaxScale = 1.0;
	float	cubemapScale = 0.0;
	float	isMetalic = 0.0;
	float	useSteepParallax = 0.0;
	float	hasOverlay = 0.0;
	float	doSway = 0.0;
	float	phongFactor = r_blinnPhong->value;
	float	hasSteepMap = 0;
	float	hasSteepMap2 = 0;
	float	hasSplatMap1 = 0;
	float	hasSplatMap2 = 0;
	float	hasSplatMap3 = 0;
	float	hasSplatMap4 = 0;
	float	hasNormalMap = 0;

	if (!backEnd.depthFill && !(tr.viewParms.flags & VPF_SHADOWPASS))
	{
		if (r_normalMapping->integer >= 2
			&& pStage->bundle[TB_NORMALMAP].image[0]
			&& pStage->bundle[TB_NORMALMAP].image[0] != tr.whiteImage
			&& pStage->bundle[TB_NORMALMAP].image[0] != tr.blackImage)
		{
			hasNormalMap = 1.0;
		}

		if (pStage->bundle[TB_OVERLAYMAP].image[0]
			&& pStage->bundle[TB_OVERLAYMAP].image[0] != tr.whiteImage)
		{
			hasOverlay = 1.0;
		}

		if (pStage->bundle[TB_STEEPMAP].image[0]
			&& pStage->bundle[TB_STEEPMAP].image[0] != tr.whiteImage)
		{
			hasSteepMap = 1.0;
		}

		if (pStage->bundle[TB_STEEPMAP2].image[0]
			&& pStage->bundle[TB_STEEPMAP2].image[0] != tr.whiteImage)
		{
			hasSteepMap2 = 1.0;
		}

		if ((pStage->bundle[TB_SPLATMAP1].image[0]
			&& pStage->bundle[TB_SPLATMAP1].image[0] != tr.whiteImage))
		{
			hasSplatMap1 = 1;
		}

		if ((pStage->bundle[TB_SPLATMAP2].image[0]
			&& pStage->bundle[TB_SPLATMAP2].image[0] != tr.whiteImage))
		{
			hasSplatMap2 = 1;
		}

		if ((pStage->bundle[TB_SPLATMAP3].image[0]
			&& pStage->bundle[TB_SPLATMAP3].image[0] != tr.whiteImage))
		{
			hasSplatMap3 = 1;
		}

		if ((pStage->bundle[TB_SPLATMAP4].image[0]
			&& pStage->bundle[TB_SPLATMAP4].image[0] != tr.whiteImage))
		{
			hasSplatMap4 = 1;
		}

		if (pStage->isWater && r_glslWater->integer)
		{
			specularScale = 1.5;
			materialType = (float)MATERIAL_WATER;
			parallaxScale = 2.0;
		}
		else
		{
			switch( tess.shader->surfaceFlags & MATERIAL_MASK )
			{
			case MATERIAL_WATER:			// 13			// light covering of water on a surface
				specularScale = 1.0;
				cubemapScale = 1.5;
				materialType = (float)MATERIAL_WATER;
				parallaxScale = 2.0;
				break;
			case MATERIAL_SHORTGRASS:		// 5			// manicured lawn
				specularScale = 0.05;
				cubemapScale = 0.0;
				materialType = (float)MATERIAL_SHORTGRASS;
				parallaxScale = 2.0;
				phongFactor = -phongFactor; // no blinn phong on grassy terrains (to stop the joins being so obvious)
				break;
			case MATERIAL_LONGGRASS:		// 6			// long jungle grass
				specularScale = 0.05;
				cubemapScale = 0.0;
				materialType = (float)MATERIAL_LONGGRASS;
				parallaxScale = 2.5;
				phongFactor = -phongFactor; // no blinn phong on grassy terrains (to stop the joins being so obvious)
				break;
			case MATERIAL_SAND:				// 8			// sandy beach
				specularScale = 0.15;
				cubemapScale = 0.0;
				materialType = (float)MATERIAL_SAND;
				parallaxScale = 1.5;
				break;
			case MATERIAL_CARPET:			// 27			// lush carpet
				specularScale = 0.15;
				cubemapScale = 0.0;
				materialType = (float)MATERIAL_CARPET;
				parallaxScale = 1.5;
				break;
			case MATERIAL_GRAVEL:			// 9			// lots of small stones
				specularScale = 0.25;
				cubemapScale = 0.0;
				materialType = (float)MATERIAL_GRAVEL;
				parallaxScale = 1.5;
				break;
			case MATERIAL_ROCK:				// 23			//
				specularScale = 0.15;
				cubemapScale = 0.0;
				materialType = (float)MATERIAL_ROCK;
				parallaxScale = 2.5;
				useSteepParallax = 1.0;
				break;
			case MATERIAL_TILES:			// 26			// tiled floor
				specularScale = 0.56;
				cubemapScale = 0.15;
				materialType = (float)MATERIAL_TILES;
				parallaxScale = 2.5;
				useSteepParallax = 1.0;
				break;
			case MATERIAL_SOLIDWOOD:		// 1			// freshly cut timber
				specularScale = 0.15;
				cubemapScale = 0.0;
				materialType = (float)MATERIAL_SOLIDWOOD;
				parallaxScale = 2.5;
				//useSteepParallax = 1.0;
				break;
			case MATERIAL_HOLLOWWOOD:		// 2			// termite infested creaky wood
				specularScale = 0.15;
				cubemapScale = 0.0;
				materialType = (float)MATERIAL_HOLLOWWOOD;
				parallaxScale = 2.5;
				//useSteepParallax = 1.0;
				break;
			case MATERIAL_SOLIDMETAL:		// 3			// solid girders
				specularScale = 0.98;
				cubemapScale = 0.98;
				materialType = (float)MATERIAL_SOLIDMETAL;
				parallaxScale = 2.5;
				isMetalic = 1.0;
				break;
			case MATERIAL_HOLLOWMETAL:		// 4			// hollow metal machines -- UQ1: Used for weapons to force lower parallax and high reflection...
				specularScale = 1.0;
				cubemapScale = 1.0;
				materialType = (float)MATERIAL_HOLLOWMETAL;
				parallaxScale = 2.0;
				isMetalic = 1.0;
				break;
			case MATERIAL_DRYLEAVES:		// 19			// dried up leaves on the floor
				specularScale = 0.1;
				cubemapScale = 0.0;
				materialType = (float)MATERIAL_DRYLEAVES;
				parallaxScale = 0.0;
				//useSteepParallax = 1.0;
				break;
			case MATERIAL_GREENLEAVES:		// 20			// fresh leaves still on a tree
				specularScale = 0.35;
				cubemapScale = 0.0;
				materialType = (float)MATERIAL_GREENLEAVES;
				parallaxScale = 0.0; // GreenLeaves should NEVER be parallaxed.. It's used for surfaces with an alpha channel and parallax screws it up...
				//useSteepParallax = 1.0;
				break;
			case MATERIAL_FABRIC:			// 21			// Cotton sheets
				specularScale = 0.25;
				cubemapScale = 0.0;
				materialType = (float)MATERIAL_FABRIC;
				parallaxScale = 2.0;
				break;
			case MATERIAL_CANVAS:			// 22			// tent material
				specularScale = 0.25;
				cubemapScale = 0.0;
				materialType = (float)MATERIAL_CANVAS;
				parallaxScale = 2.0;
				break;
			case MATERIAL_MARBLE:			// 12			// marble floors
				specularScale = 0.2;
				cubemapScale = 0.6;
				materialType = (float)MATERIAL_MARBLE;
				parallaxScale = 2.0;
				//useSteepParallax = 1.0;
				break;
			case MATERIAL_SNOW:				// 14			// freshly laid snow
				specularScale = 0.3;
				cubemapScale = 0.25;
				materialType = (float)MATERIAL_SNOW;
				parallaxScale = 3.0;
				useSteepParallax = 1.0;
				break;
			case MATERIAL_MUD:				// 17			// wet soil
				specularScale = 0.25;
				cubemapScale = 0.1;
				materialType = (float)MATERIAL_MUD;
				parallaxScale = 1.5;
				useSteepParallax = 1.0;
				break;
			case MATERIAL_DIRT:				// 7			// hard mud
				specularScale = 0.15;
				cubemapScale = 0.0;
				materialType = (float)MATERIAL_DIRT;
				parallaxScale = 1.5;
				useSteepParallax = 1.0;
				break;
			case MATERIAL_CONCRETE:			// 11			// hardened concrete pavement
				specularScale = 0.25;
				cubemapScale = 0.05;
				materialType = (float)MATERIAL_CONCRETE;
				parallaxScale = 2.5;
				//useSteepParallax = 1.0;
				break;
			case MATERIAL_FLESH:			// 16			// hung meat, corpses in the world
				specularScale = 0.15;
				cubemapScale = 0.0;
				materialType = (float)MATERIAL_FLESH;
				parallaxScale = 1.5;
				break;
			case MATERIAL_RUBBER:			// 24			// hard tire like rubber
				specularScale = 0.15;
				cubemapScale = 0.0;
				materialType = (float)MATERIAL_RUBBER;
				parallaxScale = 1.5;
				break;
			case MATERIAL_PLASTIC:			// 25			//
				specularScale = 0.58;
				cubemapScale = 0.3;
				materialType = (float)MATERIAL_PLASTIC;
				parallaxScale = 1.5;
				break;
			case MATERIAL_PLASTER:			// 28			// drywall style plaster
				specularScale = 0.3;
				cubemapScale = 0.05;
				materialType = (float)MATERIAL_PLASTER;
				parallaxScale = 2.0;
				break;
			case MATERIAL_SHATTERGLASS:		// 29			// glass with the Crisis Zone style shattering
				specularScale = 0.88;
				cubemapScale = 1.0;
				materialType = (float)MATERIAL_SHATTERGLASS;
				parallaxScale = 1.0;
				break;
			case MATERIAL_ARMOR:			// 30			// body armor
				specularScale = 0.4;
				cubemapScale = 0.4;
				materialType = (float)MATERIAL_ARMOR;
				parallaxScale = 2.0;
				isMetalic = 1.0;
				break;
			case MATERIAL_ICE:				// 15			// packed snow/solid ice
				specularScale = 0.45;
				cubemapScale = 0.4;
				materialType = (float)MATERIAL_ICE;
				parallaxScale = 2.0;
				useSteepParallax = 1.5;
				break;
			case MATERIAL_GLASS:			// 10			//
				specularScale = 0.95;
				cubemapScale = 1.0;
				materialType = (float)MATERIAL_GLASS;
				parallaxScale = 1.0;
				break;
			case MATERIAL_BPGLASS:			// 18			// bulletproof glass
				specularScale = 0.93;
				cubemapScale = 0.93;
				materialType = (float)MATERIAL_BPGLASS;
				parallaxScale = 1.0;
				break;
			case MATERIAL_COMPUTER:			// 31			// computers/electronic equipment
				specularScale = 0.92;
				cubemapScale = 0.92;
				materialType = (float)MATERIAL_COMPUTER;
				parallaxScale = 2.0;
				break;
			default:
				specularScale = 0.0;
				cubemapScale = 0.0;
				materialType = (float)0.0;
				parallaxScale = 1.0;
				break;
			}
		}

		// Shader overrides material...
		if (pStage->cubeMapScale > 0.0)
		{
			cubemapScale = pStage->cubeMapScale;
		}

		if (pStage->isFoliage)
		{
			doSway = 0.7;
		}

		if (tess.shader == tr.sunShader)
		{// SPECIAL MATERIAL TYPE FOR SUN
			materialType = 1025.0;
		}

		VectorSet4(local1, parallaxScale*r_parallaxScale->value, (float)pStage->hasSpecular, specularScale, materialType);
		GLSL_SetUniformVec4(sp, UNIFORM_LOCAL1, local1);
		//GLSL_SetUniformVec4(sp, UNIFORM_LOCAL2, pStage->subsurfaceExtinctionCoefficient);
		VectorSet4(local3, 0.0/*pStage->subsurfaceRimScalar*/, 0.0/*pStage->subsurfaceMaterialThickness*/, 0.0/*pStage->subsurfaceSpecularPower*/, cubemapScale);
		GLSL_SetUniformVec4(sp, UNIFORM_LOCAL3, local3);
		VectorSet4(local4, hasNormalMap, isMetalic, 0.0/*(float)pStage->hasRealSubsurfaceMap*/, doSway);
		GLSL_SetUniformVec4(sp, UNIFORM_LOCAL4, local4);
		VectorSet4(local5, hasOverlay, overlaySway, phongFactor, hasSteepMap);
		GLSL_SetUniformVec4(sp, UNIFORM_LOCAL5, local5);

		vec4_t local6;
		VectorSet4(local6, r_sunlightSpecular->value, hasSteepMap2, MAP_INFO_MAXSIZE, MAP_WATER_LEVEL);
		GLSL_SetUniformVec4(sp, UNIFORM_LOCAL6,  local6);

		vec4_t local7;
		VectorSet4(local7, hasSplatMap1, hasSplatMap2, hasSplatMap3, hasSplatMap4);
		GLSL_SetUniformVec4(sp, UNIFORM_LOCAL7,  local7);
	}
	else
	{// Don't waste time on unneeded stuff... Absolute minimum shader complexity...
		specularScale = 0.0;
		cubemapScale = 0.0;
		materialType = (float)0.0;
		parallaxScale = 0.0;

		if (tess.shader == tr.sunShader)
		{// SPECIAL MATERIAL TYPE FOR SUN
			materialType = 1025.0;
		}
		else
		{
			materialType = ( tess.shader->surfaceFlags & MATERIAL_MASK );
		}

		if (pStage->isFoliage)
		{
			doSway = 0.7;
		}

		VectorSet4(local1, 0.0, 0.0, 0.0, materialType);
		GLSL_SetUniformVec4(sp, UNIFORM_LOCAL1, local1);
		//GLSL_SetUniformVec4(sp, UNIFORM_LOCAL2, pStage->subsurfaceExtinctionCoefficient);
		VectorSet4(local3, 0.0, 0.0, 0.0, 0.0);
		GLSL_SetUniformVec4(sp, UNIFORM_LOCAL3, local3);
		VectorSet4(local4, 0.0, 0.0, 0.0, doSway);
		GLSL_SetUniformVec4(sp, UNIFORM_LOCAL4, local4);
		VectorSet4(local5, 0.0, overlaySway, 0.0, 0.0);
		GLSL_SetUniformVec4(sp, UNIFORM_LOCAL5, local5);

		vec4_t local6;
		VectorSet4(local6, 0.0, 0.0, MAP_INFO_MAXSIZE, MAP_WATER_LEVEL);
		GLSL_SetUniformVec4(sp, UNIFORM_LOCAL6,  local6);

		vec4_t local7;
		VectorSet4(local7, 0.0, 0.0, 0.0, 0.0);
		GLSL_SetUniformVec4(sp, UNIFORM_LOCAL7,  local7);
	}

	vec4_t specMult;

	if (backEnd.depthFill || (tr.viewParms.flags & VPF_SHADOWPASS))
	{// Don't waste time on speculars...
		VectorSet4(specMult, 0.0, 0.0, 0.0, 0.0);
		GLSL_SetUniformVec4(sp, UNIFORM_SPECULARSCALE, specMult);
	}
	else if (pStage->specularScale[0] + pStage->specularScale[1] + pStage->specularScale[2] + pStage->specularScale[3] != 0.0)
	{// Shader Specified...
		GLSL_SetUniformVec4(sp, UNIFORM_SPECULARSCALE, pStage->specularScale);
	}
	else // Material Defaults...
	{
		VectorSet4(specMult, specularScale, specularScale, specularScale, 1.0);

		if (( tess.shader->surfaceFlags & MATERIAL_MASK ) == 30.0 /* ARMOR */
			|| ( tess.shader->surfaceFlags & MATERIAL_MASK ) == 25.0 /* PLASTIC */
			|| ( tess.shader->surfaceFlags & MATERIAL_MASK ) == 12.0 /* MARBLE */)
		{// Armor, plastic, and marble should remain somewhat shiny...
			specMult[0] = 0.333;
			specMult[1] = 0.333;
			specMult[2] = 0.333;
			GLSL_SetUniformVec4(sp, UNIFORM_SPECULARSCALE, specMult);
		}
		else if (( tess.shader->surfaceFlags & MATERIAL_MASK ) != 0.0 /* METALS */
			&& ( tess.shader->surfaceFlags & MATERIAL_MASK ) != 10.0 /* GLASS */
			&& ( tess.shader->surfaceFlags & MATERIAL_MASK ) != 29.0 /* SHATTERGLASS */
			&& ( tess.shader->surfaceFlags & MATERIAL_MASK ) != 18.0 /* BPGLASS */
			&& ( tess.shader->surfaceFlags & MATERIAL_MASK ) != 31.0 /* COMPUTER */
			&& ( tess.shader->surfaceFlags & MATERIAL_MASK ) != 15.0 /* ICE */)
		{// Only if not metalic... Metals should remain nice and shiny...
			specMult[0] *= 0.04;
			specMult[1] *= 0.04;
			specMult[2] *= 0.04;
			GLSL_SetUniformVec4(sp, UNIFORM_SPECULARSCALE, specMult);
		}
		else
		{
			GLSL_SetUniformVec4(sp, UNIFORM_SPECULARSCALE, specMult);
		}
	}

	//GLSL_SetUniformFloat(sp, UNIFORM_TIME, tess.shaderTime);
	GLSL_SetUniformFloat(sp, UNIFORM_TIME, backEnd.refdef.floatTime);
}

void RB_SetStageImageDimensions(shaderProgram_t *sp, shaderStage_t *pStage)
{
	vec2_t dimensions;

	dimensions[0] = pStage->bundle[0].image[0]->width;
	dimensions[1] = pStage->bundle[0].image[0]->height;

	if (pStage->bundle[TB_DIFFUSEMAP].image[0])
	{
		dimensions[0] = pStage->bundle[TB_DIFFUSEMAP].image[0]->width;
		dimensions[1] = pStage->bundle[TB_DIFFUSEMAP].image[0]->height;
	}
	else if (pStage->bundle[TB_NORMALMAP].image[0])
	{
		dimensions[0] = pStage->bundle[TB_NORMALMAP].image[0]->width;
		dimensions[1] = pStage->bundle[TB_NORMALMAP].image[0]->height;
	}
	else if (pStage->bundle[TB_SPECULARMAP].image[0])
	{
		dimensions[0] = pStage->bundle[TB_SPECULARMAP].image[0]->width;
		dimensions[1] = pStage->bundle[TB_SPECULARMAP].image[0]->height;
	}
	/*else if (pStage->bundle[TB_SUBSURFACEMAP].image[0])
	{
		dimensions[0] = pStage->bundle[TB_SUBSURFACEMAP].image[0]->width;
		dimensions[1] = pStage->bundle[TB_SUBSURFACEMAP].image[0]->height;
	}*/
	else if (pStage->bundle[TB_OVERLAYMAP].image[0])
	{
		dimensions[0] = pStage->bundle[TB_OVERLAYMAP].image[0]->width;
		dimensions[1] = pStage->bundle[TB_OVERLAYMAP].image[0]->height;
	}
	else if (pStage->bundle[TB_STEEPMAP].image[0])
	{
		dimensions[0] = pStage->bundle[TB_STEEPMAP].image[0]->width;
		dimensions[1] = pStage->bundle[TB_STEEPMAP].image[0]->height;
	}
	else if (pStage->bundle[TB_STEEPMAP2].image[0])
	{
		dimensions[0] = pStage->bundle[TB_STEEPMAP2].image[0]->width;
		dimensions[1] = pStage->bundle[TB_STEEPMAP2].image[0]->height;
	}

	GLSL_SetUniformVec2(sp, UNIFORM_DIMENSIONS, dimensions);
}

qboolean RB_ShouldUseTesselation ( int materialType )
{
	if ( materialType == MATERIAL_SHORTGRASS
		|| materialType == MATERIAL_LONGGRASS
		|| materialType == MATERIAL_SAND
		//|| materialType == MATERIAL_ROCK
		|| materialType == MATERIAL_ICE)
		return qtrue;

	return qfalse;
}

float RB_GetTesselationAlphaLevel ( int materialType )
{
	float tessAlphaLevel = r_tesselationAlpha->value;

	switch( materialType )
	{
	case MATERIAL_SHORTGRASS:
	case MATERIAL_LONGGRASS:
	case MATERIAL_SAND:
	//case MATERIAL_ROCK:
	case MATERIAL_ICE:
		tessAlphaLevel = 10.0 * r_tesselationAlpha->value;
		break;
	default:
		tessAlphaLevel = 0.001;
		break;
	}

	return tessAlphaLevel;
}

float RB_GetTesselationInnerLevel ( int materialType )
{
	return r_tesselationLevel->value;

	float tessInnerLevel = Q_clamp(1.0, r_tesselationLevel->value, 2.25);

	switch( materialType )
	{
	case MATERIAL_SHORTGRASS:		// 5			// manicured lawn
		tessInnerLevel = Q_clamp(1.0, r_tesselationLevel->value, 2.25);
		break;
	case MATERIAL_LONGGRASS:		// 6			// long jungle grass
		tessInnerLevel = Q_clamp(1.0, r_tesselationLevel->value, 2.25);
		break;
	case MATERIAL_SAND:				// 8			// sandy beach
		tessInnerLevel = Q_clamp(1.0, r_tesselationLevel->value * 0.1, 2.25);
		break;
	/*case MATERIAL_CARPET:			// 27			// lush carpet
		tessInnerLevel = Q_clamp(1.0, r_tesselationLevel->value * 0.3, 2.25);
		break;
	case MATERIAL_GRAVEL:			// 9			// lots of small stones
		tessInnerLevel = Q_clamp(1.0, r_tesselationLevel->value, 2.25);
		break;
	case MATERIAL_ROCK:				// 23			//
		tessInnerLevel = Q_clamp(1.0, r_tesselationLevel->value * 0.5, 2.25);
		break;
	case MATERIAL_TILES:			// 26			// tiled floor
		tessInnerLevel = Q_clamp(1.0, r_tesselationLevel->value * 0.3, 2.25);
		break;
	case MATERIAL_SOLIDWOOD:		// 1			// freshly cut timber
		tessInnerLevel = Q_clamp(1.0, r_tesselationLevel->value, r_tesselationLevel->value);
		break;
	case MATERIAL_HOLLOWWOOD:		// 2			// termite infested creaky wood
		tessInnerLevel = Q_clamp(1.0, r_tesselationLevel->value, r_tesselationLevel->value);
		break;
	case MATERIAL_SOLIDMETAL:		// 3			// solid girders
		tessInnerLevel = Q_clamp(1.0, r_tesselationLevel->value * 0.5, 2.25);
		break;
	case MATERIAL_HOLLOWMETAL:		// 4			// hollow metal machines -- UQ1: Used for weapons to force lower parallax and high reflection...
		tessInnerLevel = Q_clamp(1.0, r_tesselationLevel->value * 0.3, 2.25);
		break;
	case MATERIAL_DRYLEAVES:		// 19			// dried up leaves on the floor
		tessInnerLevel = Q_clamp(1.0, r_tesselationLevel->value, 2.25);
		break;
	case MATERIAL_GREENLEAVES:		// 20			// fresh leaves still on a tree
		tessInnerLevel = Q_clamp(1.0, r_tesselationLevel->value, 2.25);
		break;
	case MATERIAL_FABRIC:			// 21			// Cotton sheets
		tessInnerLevel = Q_clamp(1.0, r_tesselationLevel->value, r_tesselationLevel->value);
		break;
	case MATERIAL_CANVAS:			// 22			// tent material
		tessInnerLevel = Q_clamp(1.0, r_tesselationLevel->value, 2.25);
		break;
	case MATERIAL_MARBLE:			// 12			// marble floors
		tessInnerLevel = Q_clamp(1.0, r_tesselationLevel->value * 0.5, 2.25);
		break;
	case MATERIAL_SNOW:				// 14			// freshly laid snow
		tessInnerLevel = Q_clamp(1.0, r_tesselationLevel->value * 0.1, 2.25);
		break;
	case MATERIAL_MUD:				// 17			// wet soil
		tessInnerLevel = Q_clamp(1.0, r_tesselationLevel->value * 0.1, 2.25);
		break;
	case MATERIAL_DIRT:				// 7			// hard mud
		tessInnerLevel = Q_clamp(1.0, r_tesselationLevel->value * 0.1, 2.25);
		break;
	case MATERIAL_CONCRETE:			// 11			// hardened concrete pavement
		tessInnerLevel = Q_clamp(1.0, r_tesselationLevel->value * 0.3, 2.25);
		break;
	case MATERIAL_FLESH:			// 16			// hung meat, corpses in the world
		tessInnerLevel = Q_clamp(1.0, r_tesselationLevel->value, r_tesselationLevel->value);
		break;
	case MATERIAL_RUBBER:			// 24			// hard tire like rubber
		tessInnerLevel = Q_clamp(1.0, r_tesselationLevel->value, 2.25);
		break;
	case MATERIAL_PLASTIC:			// 25			//
		tessInnerLevel = Q_clamp(1.0, r_tesselationLevel->value * 0.5, 2.25);
		break;
	case MATERIAL_PLASTER:			// 28			// drywall style plaster
		tessInnerLevel = Q_clamp(1.0, r_tesselationLevel->value * 0.3, 2.25);
		break;
	case MATERIAL_ARMOR:			// 30			// body armor
		tessInnerLevel = Q_clamp(1.0, r_tesselationLevel->value, r_tesselationLevel->value);
		break;*/
	case MATERIAL_ICE:				// 15			// packed snow/solid ice
		tessInnerLevel = Q_clamp(1.0, r_tesselationLevel->value * 0.3, r_tesselationLevel->value);
		break;
	case MATERIAL_WATER:			// 13			// light covering of water on a surface
	case MATERIAL_SHATTERGLASS:		// 29			// glass with the Crisis Zone style shattering
	case MATERIAL_GLASS:			// 10			//
	case MATERIAL_BPGLASS:			// 18			// bulletproof glass
	case MATERIAL_COMPUTER:			// 31			// computers/electronic equipment
	default:
		tessInnerLevel = 1.0;
		break;
	}

	return tessInnerLevel;
}


extern qboolean R_SurfaceIsAllowedFoliage( int materialType );

qboolean RB_ShouldUseGeometryGrass (int materialType )
{
	if ( materialType <= MATERIAL_NONE )
	{
		return qfalse;
	}

	if ( materialType == MATERIAL_SHORTGRASS
		|| materialType == MATERIAL_LONGGRASS )
	{
		return qtrue;
	}

	if ( R_SurfaceIsAllowedFoliage( materialType ) )
	{// *sigh* due to surfaceFlags mixing materials with other flags, we need to do it this way...
		return qtrue;
	}

	return qfalse;
}

qboolean RB_ShouldUseGeometryPebbles(int materialType)
{
	if (materialType <= MATERIAL_NONE)
	{
		return qfalse;
	}

	if (materialType == MATERIAL_SAND || materialType == MATERIAL_DIRT || materialType == MATERIAL_GRAVEL || materialType == MATERIAL_MUD)
	{
		return qtrue;
	}

	return qfalse;
}

matrix_t MATRIX_TRANS, MATRIX_MODEL, MATRIX_MVP, MATRIX_INVTRANS, MATRIX_NORMAL, MATRIX_VP, MATRIX_INVMV;

void RB_UpdateMatrixes ( void )
{
	if (!MATRIX_UPDATE) return;

	// UQ1: Calculate some matrixes that rend2 doesn't seem to have (or have correct)...
	Matrix16Translation( backEnd.viewParms.ori.origin, MATRIX_TRANS );
	Matrix16Multiply( backEnd.viewParms.world.modelMatrix, MATRIX_TRANS, MATRIX_MODEL );
	Matrix16Multiply(backEnd.viewParms.projectionMatrix, MATRIX_MODEL, MATRIX_MVP);
	Matrix16Multiply(backEnd.viewParms.projectionMatrix, backEnd.viewParms.world.modelMatrix, MATRIX_VP);

	Matrix16SimpleInverse( MATRIX_TRANS, MATRIX_INVTRANS);
	Matrix16SimpleInverse( backEnd.viewParms.projectionMatrix, MATRIX_NORMAL);


	//GLSL_SetUniformMatrix16(sp, UNIFORM_MODELVIEWPROJECTIONMATRIX, glState.modelviewProjection);
	//GLSL_SetUniformMatrix16(sp, UNIFORM_INVEYEPROJECTIONMATRIX, glState.invEyeProjection);
	Matrix16SimpleInverse( glState.modelviewProjection, glState.invEyeProjection);
	Matrix16SimpleInverse( MATRIX_MODEL, MATRIX_INVMV);

	MATRIX_UPDATE = qfalse;
}

int			NUM_CLOSE_LIGHTS = 0;
int			CLOSEST_LIGHTS[MAX_LIGHTALL_DLIGHTS] = {0};
vec3_t		CLOSEST_LIGHTS_POSITIONS[MAX_LIGHTALL_DLIGHTS] = {0};
float		CLOSEST_LIGHTS_DISTANCES[MAX_LIGHTALL_DLIGHTS] = {0};
vec3_t		CLOSEST_LIGHTS_COLORS[MAX_LIGHTALL_DLIGHTS] = {0};

void RB_UpdateCloseLights ( void )
{
	if (!CLOSE_LIGHTS_UPDATE) return; // Already done for this frame...

	NUM_CLOSE_LIGHTS = 0;

	for ( int l = 0 ; l < backEnd.refdef.num_dlights ; l++ )
	{
		dlight_t	*dl = &backEnd.refdef.dlights[l];

#ifndef USING_ENGINE_GLOW_LIGHTCOLORS_SEARCH
		if (dl->color[0] < 0.0 && dl->color[1] < 0.0 && dl->color[2] < 0.0)
		{// Surface glow light... But has no color assigned...
			continue;
		}
#endif

		float distance = Distance(tr.refdef.vieworg, dl->origin);

		if (NUM_CLOSE_LIGHTS < MAX_LIGHTALL_DLIGHTS)
		{// Have free light slots for a new light...
			CLOSEST_LIGHTS[NUM_CLOSE_LIGHTS] = l;
			VectorCopy(dl->origin, CLOSEST_LIGHTS_POSITIONS[NUM_CLOSE_LIGHTS]);
			CLOSEST_LIGHTS_DISTANCES[NUM_CLOSE_LIGHTS] = dl->radius;
			CLOSEST_LIGHTS_COLORS[NUM_CLOSE_LIGHTS][0] = dl->color[0];
			CLOSEST_LIGHTS_COLORS[NUM_CLOSE_LIGHTS][1] = dl->color[1];
			CLOSEST_LIGHTS_COLORS[NUM_CLOSE_LIGHTS][2] = dl->color[2];
			NUM_CLOSE_LIGHTS++;
			continue;
		}
		else
		{// See if this is closer then one of our other lights...
			int		farthest_light = 0;
			float	farthest_distance = 0.0;

			for (int i = 0; i < NUM_CLOSE_LIGHTS; i++)
			{// Find the most distance light in our current list to replace, if this new option is closer...
				dlight_t	*thisLight = &backEnd.refdef.dlights[CLOSEST_LIGHTS[i]];
				float		dist = Distance(thisLight->origin, tr.refdef.vieworg);

				if (dist > farthest_distance)
				{// This one is further!
					farthest_light = i;
					farthest_distance = dist;
					break;
				}
			}

			if (Distance(dl->origin, tr.refdef.vieworg) < farthest_distance)
			{// This light is closer. Replace this one in our array of closest lights...
				CLOSEST_LIGHTS[farthest_light] = l;
				VectorCopy(dl->origin, CLOSEST_LIGHTS_POSITIONS[farthest_light]);
				CLOSEST_LIGHTS_DISTANCES[farthest_light] = dl->radius;
				CLOSEST_LIGHTS_COLORS[farthest_light][0] = dl->color[0];
				CLOSEST_LIGHTS_COLORS[farthest_light][1] = dl->color[1];
				CLOSEST_LIGHTS_COLORS[farthest_light][2] = dl->color[2];
			}
		}
	}

	for (int i = 0; i < NUM_CLOSE_LIGHTS; i++)
	{
		if (CLOSEST_LIGHTS_DISTANCES[i] < 0.0)
		{// Remove volume light markers...
			CLOSEST_LIGHTS_DISTANCES[i] = -CLOSEST_LIGHTS_DISTANCES[i];
		}

#ifdef USING_ENGINE_GLOW_LIGHTCOLORS_SEARCH
		if (CLOSEST_LIGHTS_COLORS[i][0] < 0.0 && CLOSEST_LIGHTS_COLORS[i][1] < 0.0 && CLOSEST_LIGHTS_COLORS[i][2] < 0.0)
		{// Surface glow lights...
			CLOSEST_LIGHTS_COLORS[i][0] = -CLOSEST_LIGHTS_COLORS[i][0] * 0.5;
			CLOSEST_LIGHTS_COLORS[i][1] = -CLOSEST_LIGHTS_COLORS[i][1] * 0.5;
			CLOSEST_LIGHTS_COLORS[i][2] = -CLOSEST_LIGHTS_COLORS[i][2] * 0.5;
		}
#endif //USING_ENGINE_GLOW_LIGHTCOLORS_SEARCH

		// Double the range on all lights...
		CLOSEST_LIGHTS_DISTANCES[i] *= 2.0;
	}

	//ri->Printf(PRINT_ALL, "Found %i close lights this frame.\n", NUM_CLOSE_LIGHTS);

	CLOSE_LIGHTS_UPDATE = qfalse;
}

extern image_t *skyImage;

float waveTime = 0.5;
float waveFreq = 0.1;

extern void GLSL_AttachTextures( void );
extern void GLSL_AttachGenericTextures( void );
extern void GLSL_AttachGlowTextures( void );
extern void GLSL_AttachWaterTextures( void );
extern void GLSL_AttachWaterTextures2( void );

extern qboolean ALLOW_GL_400;

static void RB_IterateStagesGeneric( shaderCommands_t *input )
{
	vec4_t	fogDistanceVector, fogDepthVector = {0, 0, 0, 0};
	float	eyeT = 0;
	int		deformGen;
	vec5_t	deformParams;

	ComputeDeformValues(&deformGen, deformParams);

	ComputeFogValues(fogDistanceVector, fogDepthVector, &eyeT);

	RB_UpdateMatrixes();
	//RB_UpdateCloseLights();

	for ( int stage = 0; stage < MAX_SHADER_STAGES; stage++ )
	{
		shaderStage_t *pStage = input->xstages[stage];
		shaderProgram_t *sp = NULL, *sp2 = NULL, *sp3 = NULL;
		vec4_t texMatrix;
		vec4_t texOffTurb;
		int stateBits;
		colorGen_t forceRGBGen = CGEN_BAD;
		alphaGen_t forceAlphaGen = AGEN_IDENTITY;
		qboolean isGeneric = qtrue;
		qboolean isLightAll = qfalse;
		qboolean useTesselation = qfalse;
		qboolean isWater = qfalse;
		qboolean isGrass = qfalse;
		qboolean isPebbles = qfalse;
		qboolean multiPass = qtrue;
		qboolean usingLight = qfalse;
		qboolean isGlowStage = qfalse;
		qboolean isUsingRegions = qfalse;

		float cubeMapStrength = 0.0;
		vec4_t cubeMapVec;

		int passNum = 0, passMax = 0;

		if ( !pStage )
		{
			break;
		}

		if ( !pStage->active )
		{// Shouldn't this be here, just in case???
			continue;
		}

		if ( pStage->isSurfaceSprite )
		{
#ifdef __SURFACESPRITES__
			if (!r_surfaceSprites->integer)
#endif //__SURFACESPRITES__
			{
continue;
			}
		}

		if (backEnd.depthFill || (tr.viewParms.flags & VPF_SHADOWPASS))
		{
			if (pStage->glslShaderGroup == tr.lightallShader)
			{
				int index = 0;

				if (backEnd.currentEntity && backEnd.currentEntity != &tr.worldEntity)
				{
					index |= LIGHTDEF_ENTITY;

					if (glState.vertexAnimation)
					{
						index |= LIGHTDEF_USE_VERTEX_ANIMATION;
					}

					if (glState.skeletalAnimation)
					{
						index |= LIGHTDEF_USE_SKELETAL_ANIMATION;
					}
				}

				if (pStage->stateBits & GLS_ATEST_BITS)
				{
					index |= LIGHTDEF_USE_TCGEN_AND_TCMOD;
				}

				if (r_tesselation->integer && RB_ShouldUseTesselation(tess.shader->surfaceFlags & MATERIAL_MASK))
				{
					index |= LIGHTDEF_USE_TESSELLATION;
					useTesselation = qtrue;
				}

				if (r_foliage->integer
					&& r_sunlightMode->integer >= 2
					&& r_foliageShadows->integer
					&& RB_ShouldUseGeometryGrass(tess.shader->surfaceFlags & MATERIAL_MASK))
				{
					isGrass = qtrue;
				}
				else if (r_pebbles->integer
					&& r_sunlightMode->integer >= 2
					&& r_foliageShadows->integer
					&& RB_ShouldUseGeometryPebbles(tess.shader->surfaceFlags & MATERIAL_MASK))
				{
					isPebbles = qtrue;
				}

				sp = &pStage->glslShaderGroup[index];
				isGeneric = qfalse;
				isLightAll = qtrue;
			}
			else
			{
				int shaderAttribs = 0;

				if (tess.shader->numDeforms && !ShaderRequiresCPUDeforms(tess.shader))
				{
					shaderAttribs |= GENERICDEF_USE_DEFORM_VERTEXES;
				}

				if (glState.vertexAnimation)
				{
					shaderAttribs |= GENERICDEF_USE_VERTEX_ANIMATION;
				}

				if (glState.skeletalAnimation)
				{
					shaderAttribs |= GENERICDEF_USE_SKELETAL_ANIMATION;
				}

				if (pStage->stateBits & GLS_ATEST_BITS)
				{
					shaderAttribs |= GENERICDEF_USE_TCGEN_AND_TCMOD;
				}

				sp = &tr.genericShader[shaderAttribs];
				isGeneric = qtrue;
				isLightAll = qfalse;
			}
		}
		else if (pStage->glslShaderGroup == tr.lightallShader
			|| pStage->bundle[TB_STEEPMAP].image[0]
			|| pStage->bundle[TB_STEEPMAP2].image[0]
			|| pStage->bundle[TB_SPLATMAP1].image[0]
			|| pStage->bundle[TB_SPLATMAP2].image[0]
			|| pStage->bundle[TB_SPLATMAP3].image[0]
			|| pStage->bundle[TB_SPLATMAP4].image[0])
		{
			int index = pStage->glslShaderIndex;

			if (pStage->glslShaderGroup != tr.lightallShader
				&& (pStage->bundle[TB_STEEPMAP].image[0]
					|| pStage->bundle[TB_STEEPMAP2].image[0]
					|| pStage->bundle[TB_SPLATMAP1].image[0]
					|| pStage->bundle[TB_SPLATMAP2].image[0]
					|| pStage->bundle[TB_SPLATMAP3].image[0]
					|| pStage->bundle[TB_SPLATMAP4].image[0]))
			{// When we have splatmaps we need to force lightall...
				pStage->glslShaderGroup = tr.lightallShader;
				pStage->glslShaderIndex = 0;
				index = pStage->glslShaderIndex;
			}

			if (r_foliage->integer
				&& RB_ShouldUseGeometryGrass(tess.shader->surfaceFlags & MATERIAL_MASK))
			{
				isGrass = qtrue;
			}
			else if (r_pebbles->integer
				&& RB_ShouldUseGeometryPebbles(tess.shader->surfaceFlags & MATERIAL_MASK))
			{
				isPebbles = qtrue;
			}

			if (backEnd.currentEntity && backEnd.currentEntity != &tr.worldEntity)
			{
				index |= LIGHTDEF_ENTITY;

				if (glState.vertexAnimation)
				{
					index |= LIGHTDEF_USE_VERTEX_ANIMATION;
				}

				if (glState.skeletalAnimation)
				{
					index |= LIGHTDEF_USE_SKELETAL_ANIMATION;
				}
			}

			/*
			if ((r_sunlightMode->integer >= 2)
				&& ((backEnd.viewParms.flags & VPF_USESUNLIGHT))
				&& (( tess.shader->surfaceFlags & MATERIAL_MASK ) == MATERIAL_GREENLEAVES
					|| ( tess.shader->surfaceFlags & MATERIAL_MASK ) == MATERIAL_SHORTGRASS
					|| ( tess.shader->surfaceFlags & MATERIAL_MASK ) == MATERIAL_LONGGRASS))
			{
				index |= LIGHTDEF_USE_SHADOWMAP;
			}
			else if ((r_sunlightMode->integer >= 2)
				&& ((backEnd.viewParms.flags & VPF_USESUNLIGHT))
				&& ( tess.shader->surfaceFlags & MATERIAL_MASK ) == MATERIAL_DRYLEAVES)
			{// No shadows on dryleaves (billboards)...

			}
			else if ((r_sunlightMode->integer >= 2)
				&& ((backEnd.viewParms.flags & VPF_USESUNLIGHT)))
			{
				index |= LIGHTDEF_USE_SHADOWMAP;
			}
			*/

			if (r_lightmap->integer
				&& ( tess.shader->surfaceFlags & MATERIAL_MASK ) != MATERIAL_DRYLEAVES
				&& ( tess.shader->surfaceFlags & MATERIAL_MASK ) != MATERIAL_GREENLEAVES
				&& !tess.shader->isSky && !pStage->glow)
			{
				index = LIGHTDEF_USE_LIGHTMAP;
			}

			//
			// testing cube map
			//
			if (!(tr.viewParms.flags & VPF_NOCUBEMAPS) && input->cubemapIndex && r_cubeMapping->integer >= 1)
			{
				cubeMapVec[0] = tr.cubemapOrigins[input->cubemapIndex - 1][0] - backEnd.viewParms.ori.origin[0];
				cubeMapVec[1] = tr.cubemapOrigins[input->cubemapIndex - 1][1] - backEnd.viewParms.ori.origin[1];
				cubeMapVec[2] = tr.cubemapOrigins[input->cubemapIndex - 1][2] - backEnd.viewParms.ori.origin[2];
				cubeMapVec[3] = 1.0f;

				float dist = Distance(tr.refdef.vieworg, tr.cubemapOrigins[input->cubemapIndex - 1]);
				float mult = r_cubemapCullFalloffMult->value - (r_cubemapCullFalloffMult->value * 0.04);

				if (dist < r_cubemapCullRange->value)
				{// In range for full effect...
					cubeMapStrength = 1.0;
					index |= LIGHTDEF_USE_CUBEMAP;
				}
				else if (dist >= r_cubemapCullRange->value && dist < r_cubemapCullRange->value * mult)
				{// Further scale the strength of the cubemap by the fade-out distance...
					float extraDist =		dist - r_cubemapCullRange->value;
					float falloffDist =		(r_cubemapCullRange->value * mult) - r_cubemapCullRange->value;
					float strength =		(falloffDist - extraDist) / falloffDist;

					cubeMapStrength = strength;
					index |= LIGHTDEF_USE_CUBEMAP;
				}
				else
				{// Out of range completely...
					cubeMapStrength = 0.0;
					index &= ~LIGHTDEF_USE_CUBEMAP;
				}
			}

			if (r_tesselation->integer && RB_ShouldUseTesselation(tess.shader->surfaceFlags & MATERIAL_MASK))
			{
				index |= LIGHTDEF_USE_TESSELLATION;
				useTesselation = qtrue;
			}

			if ((tess.shader->surfaceFlags & MATERIAL_MASK) == MATERIAL_ROCK
				&& (pStage->bundle[TB_STEEPMAP].image[0]
					|| pStage->bundle[TB_STEEPMAP2].image[0]
					|| pStage->bundle[TB_SPLATMAP1].image[0]
					|| pStage->bundle[TB_SPLATMAP2].image[0]
					|| pStage->bundle[TB_SPLATMAP3].image[0]
					|| pStage->bundle[TB_SPLATMAP4].image[0]))
			{
				isUsingRegions = qtrue;
				index |= LIGHTDEF_USE_REGIONS;
			}
			else if (pStage->bundle[TB_STEEPMAP].image[0]
				|| pStage->bundle[TB_STEEPMAP2].image[0]
				|| pStage->bundle[TB_SPLATMAP1].image[0]
				|| pStage->bundle[TB_SPLATMAP2].image[0]
				|| pStage->bundle[TB_SPLATMAP3].image[0]
				|| pStage->bundle[TB_SPLATMAP4].image[0])
			{
				index |= LIGHTDEF_USE_TRIPLANAR;
			}

			if (index & LIGHTDEF_USE_GLOW_BUFFER)
			{
				isGlowStage = qtrue;
			}

			sp = &pStage->glslShaderGroup[index];
			isGeneric = qfalse;
			isLightAll = qtrue;

			backEnd.pc.c_lightallDraws++;
		}
		else
		{
			sp = GLSL_GetGenericShaderProgram(stage);
			isGeneric = qtrue;
			isLightAll = qfalse;

			backEnd.pc.c_genericDraws++;
		}

		if (pStage->isWater && r_glslWater->integer && MAP_WATER_LEVEL > -131072.0)
		{
#ifdef __USE_WATERMAP__
			if (stage <= 0 && !backEnd.depthFill && !(tr.viewParms.flags & VPF_SHADOWPASS))
			//if (pStage->bundle[TB_DIFFUSEMAP].image[0])
			{
				sp = &tr.waterShader;
				pStage->glslShaderGroup = &tr.waterShader;
				GLSL_BindProgram(sp);

				RB_SetMaterialBasedProperties(sp, pStage);

				isGeneric = qfalse;
				isLightAll = qfalse;
				isWater = qtrue;
			}
			else
			{// Only do one stage on GLSL water...
				break;
			}
#else //!__USE_WATERMAP__
			sp = &tr.waterShader;
			pStage->glslShaderGroup = &tr.waterShader;
			GLSL_BindProgram(sp);

			RB_SetMaterialBasedProperties(sp, pStage);

			isGeneric = qfalse;
			isLightAll = qfalse;
			isWater = qtrue;
#endif //__USE_WATERMAP__
		}
		else
		{
			if (!sp || !sp->program)
			{
				pStage->glslShaderGroup = tr.lightallShader;
				sp = &pStage->glslShaderGroup[0];

				if (r_foliage->integer
					&& RB_ShouldUseGeometryGrass(tess.shader->surfaceFlags & MATERIAL_MASK))
				{
					isGrass = qtrue;
				}
				else if (r_pebbles->integer
					&& RB_ShouldUseGeometryPebbles(tess.shader->surfaceFlags & MATERIAL_MASK))
				{
					isPebbles = qtrue;
				}
			}

			GLSL_BindProgram(sp);
		}

		if (!sp->tesselation && ((tr.viewParms.flags & VPF_SHADOWPASS) || (backEnd.depthFill && backEnd.currentEntity == &tr.worldEntity)))
		{
			sp = &tr.shadowPassShader;
			GLSL_BindProgram(sp);

			if (!r_foliageShadows->integer || r_sunlightMode->integer < 2)
			{
				isGrass = qfalse;
				isPebbles = qfalse;
				multiPass = qfalse;
			}
		}

		if (isGrass || isPebbles)
		{
			if (isGrass && r_foliage->integer)
			{
				sp2 = &tr.grass2Shader;
				multiPass = qtrue;
				passMax = r_foliagePasses->integer;

				//if (ALLOW_GL_400) passMax = 2; // uses hardware invocations instead

				if (r_pebbles->integer)
				{
					sp3 = &tr.pebblesShader;
					passMax = r_foliagePasses->integer + r_pebblesPasses->integer;
				}
			}
			else
			{
				if (r_pebbles->integer)
				{
					sp2 = &tr.pebblesShader;
					passMax = r_pebblesPasses->integer;
				}
			}
		}

		/*
		if (isWater && r_glslWater->integer && MAP_WATER_LEVEL > -131072.0)
		{// Attach dummy water output textures...
			if (glState.currentFBO == tr.renderFbo)
			{
				multiPass = qtrue;
				passMax = 2;
			}
		}
		*/

		//
		// UQ1: Split up uniforms by what is actually used...
		//

		RB_SetMaterialBasedProperties(sp, pStage);

		stateBits = pStage->stateBits;

		/*if (backEnd.depthFill || (tr.viewParms.flags & VPF_SHADOWPASS))
		{
			stateBits = GLS_DEPTHMASK_TRUE | GLS_DEPTHFUNC_LESS;
		}
		else
		{
			//stateBits &= ~GLS_DEPTHMASK_TRUE;
			//stateBits |= GLS_DEPTHFUNC_LESS;
			stateBits = GLS_DEPTHMASK_TRUE | GLS_DEPTHFUNC_LESS;
		}*/

		if ( backEnd.currentEntity )
		{
			assert(backEnd.currentEntity->e.renderfx >= 0);

			if ( backEnd.currentEntity->e.renderfx & RF_DISINTEGRATE1 )
			{
				// we want to be able to rip a hole in the thing being disintegrated, and by doing the depth-testing it avoids some kinds of artefacts, but will probably introduce others?
				stateBits = GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_DEPTHMASK_TRUE | GLS_ATEST_GE_192;
			}

			if ( backEnd.currentEntity->e.renderfx & RF_RGB_TINT )
			{//want to use RGBGen from ent
				forceRGBGen = CGEN_ENTITY;
			}

			if ( backEnd.currentEntity->e.renderfx & RF_FORCE_ENT_ALPHA )
			{
				stateBits = GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
				if ( backEnd.currentEntity->e.renderfx & RF_ALPHA_DEPTH )
				{ //depth write, so faces through the model will be stomped over by nearer ones. this works because
					//we draw RF_FORCE_ENT_ALPHA stuff after everything else, including standard alpha surfs.
					stateBits |= GLS_DEPTHMASK_TRUE;
				}
			}
		}
		else
		{// UQ: - FPS TESTING - This may cause issues, we will need to keep an eye on things...
			if (!(tr.viewParms.flags & VPF_SHADOWPASS))
				stateBits |= GLS_DEPTHMASK_TRUE | GLS_DEPTHFUNC_LESS | GLS_DEPTHFUNC_EQUAL;
		}

		{// UQ1: Used by both generic and lightall...
			RB_SetStageImageDimensions(sp, pStage);

			GLSL_SetUniformMatrix16(sp, UNIFORM_VIEWPROJECTIONMATRIX, MATRIX_VP);
			GLSL_SetUniformMatrix16(sp, UNIFORM_MODELMATRIX, backEnd.ori.transformMatrix);
			GLSL_SetUniformMatrix16(sp, UNIFORM_MODELVIEWPROJECTIONMATRIX, glState.modelviewProjection);
			GLSL_SetUniformMatrix16(sp, UNIFORM_INVEYEPROJECTIONMATRIX, glState.invEyeProjection);

			// UQ: Other *needed* stuff... Hope these are correct...
			GLSL_SetUniformMatrix16(sp, UNIFORM_PROJECTIONMATRIX, glState.projection);
			GLSL_SetUniformMatrix16(sp, UNIFORM_MODELVIEWMATRIX, MATRIX_MODEL);
			GLSL_SetUniformMatrix16(sp, UNIFORM_VIEWMATRIX, MATRIX_TRANS);
			GLSL_SetUniformMatrix16(sp, UNIFORM_INVVIEWMATRIX, MATRIX_INVTRANS);
			GLSL_SetUniformMatrix16(sp, UNIFORM_NORMALMATRIX, MATRIX_NORMAL);
			GLSL_SetUniformMatrix16(sp, UNIFORM_INVMODELVIEWMATRIX, MATRIX_INVMV);

			GLSL_SetUniformVec3(sp, UNIFORM_LOCALVIEWORIGIN, backEnd.ori.viewOrigin);
			GLSL_SetUniformFloat(sp, UNIFORM_VERTEXLERP, glState.vertexAttribsInterpolation);

			GLSL_SetUniformVec3(sp, UNIFORM_VIEWORIGIN, backEnd.viewParms.ori.origin);

			if (pStage->normalScale[0] == 0 && pStage->normalScale[1] == 0 && pStage->normalScale[2] == 0)
			{
				vec4_t normalScale;
				VectorSet4(normalScale, r_baseNormalX->value, r_baseNormalY->value, 1.0f, r_baseParallax->value);
				GLSL_SetUniformVec4(sp, UNIFORM_NORMALSCALE, normalScale);
			}
			else
			{
				GLSL_SetUniformVec4(sp, UNIFORM_NORMALSCALE, pStage->normalScale);
			}

			if (glState.skeletalAnimation)
			{
				GLSL_SetUniformMatrix16(sp, UNIFORM_BONE_MATRICES, &glState.boneMatrices[0][0], glState.numBones);
			}

			GLSL_SetUniformInt(sp, UNIFORM_DEFORMGEN, deformGen);
			if (deformGen != DGEN_NONE)
			{
				GLSL_SetUniformFloat5(sp, UNIFORM_DEFORMPARAMS, deformParams);
				GLSL_SetUniformFloat(sp, UNIFORM_TIME, tess.shaderTime);
			}

			GLSL_SetUniformInt(sp, UNIFORM_TCGEN0, pStage->bundle[0].tcGen);
			if (pStage->bundle[0].tcGen == TCGEN_VECTOR)
			{
				vec3_t vec;

				VectorCopy(pStage->bundle[0].tcGenVectors[0], vec);
				GLSL_SetUniformVec3(sp, UNIFORM_TCGEN0VECTOR0, vec);
				VectorCopy(pStage->bundle[0].tcGenVectors[1], vec);
				GLSL_SetUniformVec3(sp, UNIFORM_TCGEN0VECTOR1, vec);
			}

			if (!(backEnd.depthFill || (tr.viewParms.flags & VPF_SHADOWPASS)))
			{
				vec4_t baseColor;
				vec4_t vertColor;

				ComputeShaderColors(pStage, baseColor, vertColor, stateBits, &forceRGBGen, &forceAlphaGen);

				if ((backEnd.refdef.colorScale != 1.0f) && !(backEnd.refdef.rdflags & RDF_NOWORLDMODEL))
				{
					// use VectorScale to only scale first three values, not alpha
					VectorScale(baseColor, backEnd.refdef.colorScale, baseColor);
					VectorScale(vertColor, backEnd.refdef.colorScale, vertColor);
				}

				if ( backEnd.currentEntity != NULL &&
					(backEnd.currentEntity->e.renderfx & RF_FORCE_ENT_ALPHA) )
				{
					vertColor[3] = backEnd.currentEntity->e.shaderRGBA[3] / 255.0f;
				}

				GLSL_SetUniformVec4(sp, UNIFORM_BASECOLOR, baseColor);
				GLSL_SetUniformVec4(sp, UNIFORM_VERTCOLOR, vertColor);
			}

			if (!(backEnd.depthFill || (tr.viewParms.flags & VPF_SHADOWPASS)))
			{
				if (pStage->rgbGen == CGEN_LIGHTING_DIFFUSE || pStage->rgbGen == CGEN_LIGHTING_DIFFUSE_ENTITY)
				{
					vec4_t vec;

					VectorScale(backEnd.currentEntity->ambientLight, 1.0f / 255.0f, vec);
					GLSL_SetUniformVec3(sp, UNIFORM_AMBIENTLIGHT, vec);

					VectorScale(backEnd.currentEntity->directedLight, 1.0f / 255.0f, vec);
					GLSL_SetUniformVec3(sp, UNIFORM_DIRECTEDLIGHT, vec);

					VectorCopy(backEnd.currentEntity->lightDir, vec);
					vec[3] = 0.0f;
					GLSL_SetUniformVec4(sp, UNIFORM_LIGHTORIGIN, vec);
					GLSL_SetUniformVec3(sp, UNIFORM_MODELLIGHTDIR, backEnd.currentEntity->modelLightDir);

					if (!isGeneric)
					{
						GLSL_SetUniformFloat(sp, UNIFORM_LIGHTRADIUS, 0.0f);
					}
				}
			}

			ComputeTexMods( pStage, TB_DIFFUSEMAP, texMatrix, texOffTurb );
			GLSL_SetUniformVec4(sp, UNIFORM_DIFFUSETEXMATRIX, texMatrix);
			GLSL_SetUniformVec4(sp, UNIFORM_DIFFUSETEXOFFTURB, texOffTurb);
		}

		if (isGeneric)
		{// UQ1: Only generic uses these...
			if (!(backEnd.depthFill || (tr.viewParms.flags & VPF_SHADOWPASS)))
			{
				if (pStage->alphaGen == AGEN_PORTAL)
				{
					GLSL_SetUniformFloat(sp, UNIFORM_PORTALRANGE, tess.shader->portalRange);
				}

				if (r_fog->integer)
				{
					if (input->fogNum)
					{
						vec4_t fogColorMask;
						GLSL_SetUniformVec4(sp, UNIFORM_FOGDISTANCE, fogDistanceVector);
						GLSL_SetUniformVec4(sp, UNIFORM_FOGDEPTH, fogDepthVector);
						GLSL_SetUniformFloat(sp, UNIFORM_FOGEYET, eyeT);

						ComputeFogColorMask(pStage, fogColorMask);
						GLSL_SetUniformVec4(sp, UNIFORM_FOGCOLORMASK, fogColorMask);
					}
				}

				GLSL_SetUniformInt(sp, UNIFORM_COLORGEN, forceRGBGen);
				GLSL_SetUniformInt(sp, UNIFORM_ALPHAGEN, forceAlphaGen);
			}
		}
		else
		{// UQ1: Only lightall uses these...
			//GLSL_SetUniformFloat(sp, UNIFORM_MAPLIGHTSCALE, backEnd.refdef.mapLightScale);

			//
			// testing cube map
			//
			if (backEnd.depthFill || (tr.viewParms.flags & VPF_SHADOWPASS))
			{
				GL_BindToTMU( tr.blackImage, TB_CUBEMAP);
				GLSL_SetUniformFloat(sp, UNIFORM_CUBEMAPSTRENGTH, 0.0);
				VectorSet4(cubeMapVec, 0.0, 0.0, 0.0, 0.0);
				GLSL_SetUniformVec4(sp, UNIFORM_CUBEMAPINFO, cubeMapVec);
			}
			else if (!(tr.viewParms.flags & VPF_NOCUBEMAPS) && input->cubemapIndex && r_cubeMapping->integer >= 1 && cubeMapStrength > 0.0)
			{
				GL_BindToTMU( tr.cubemaps[input->cubemapIndex - 1], TB_CUBEMAP);
				GLSL_SetUniformFloat(sp, UNIFORM_CUBEMAPSTRENGTH, cubeMapStrength);
				VectorScale4(cubeMapVec, 1.0f / 1000.0f, cubeMapVec);
				GLSL_SetUniformVec4(sp, UNIFORM_CUBEMAPINFO, cubeMapVec);
			}
		}

		//
		//
		//

		if (backEnd.depthFill || (tr.viewParms.flags & VPF_SHADOWPASS))
		{

		}
		else
		{
			if (pStage->bundle[TB_STEEPMAP].image[0])
			{
				//ri->Printf(PRINT_WARNING, "Image bound to steep map %i x %i.\n", pStage->bundle[TB_STEEPMAP].image[0]->width, pStage->bundle[TB_STEEPMAP].image[0]->height);
				//R_BindAnimatedImageToTMU( &pStage->bundle[TB_STEEPMAP], TB_STEEPMAP);
				GL_BindToTMU( pStage->bundle[TB_STEEPMAP].image[0], TB_STEEPMAP );
			}
			else
			{
				GL_BindToTMU( tr.whiteImage, TB_STEEPMAP );
			}

			if (pStage->bundle[TB_STEEPMAP2].image[0])
			{
				//ri->Printf(PRINT_WARNING, "Image %s bound to steep map %i x %i.\n", pStage->bundle[TB_STEEPMAP2].image[0]->imgName, pStage->bundle[TB_STEEPMAP2].image[0]->width, pStage->bundle[TB_STEEPMAP2].image[0]->height);
				//R_BindAnimatedImageToTMU( &pStage->bundle[TB_STEEPMAP2], TB_STEEPMAP2);
				GL_BindToTMU( pStage->bundle[TB_STEEPMAP2].image[0], TB_STEEPMAP2 );
			}
			else
			{
				GL_BindToTMU( tr.whiteImage, TB_STEEPMAP2 );
			}

			if (pStage->bundle[TB_SPLATMAP1].image[0] && pStage->bundle[TB_SPLATMAP1].image[0] != tr.whiteImage)
			{
				GL_BindToTMU( pStage->bundle[TB_SPLATMAP1].image[0], TB_SPLATMAP1 );
				GL_BindToTMU( pStage->bundle[TB_SPLATMAP1].image[1], TB_SPLATNORMALMAP1 );
			}
			else
			{
				if (pStage->bundle[TB_DIFFUSEMAP].image[0])
				{
					R_BindAnimatedImageToTMU( &pStage->bundle[TB_DIFFUSEMAP], TB_SPLATMAP1);
					GL_BindToTMU( pStage->bundle[TB_NORMALMAP].image[0], TB_SPLATNORMALMAP1 );
				}
				else // will never get used anyway
				{
					GL_BindToTMU( tr.whiteImage, TB_SPLATMAP1 );
					GL_BindToTMU( tr.whiteImage, TB_SPLATNORMALMAP1 );
				}
			}

			if (pStage->bundle[TB_SPLATMAP2].image[0] && pStage->bundle[TB_SPLATMAP2].image[0] != tr.whiteImage)
			{
				GL_BindToTMU( pStage->bundle[TB_SPLATMAP2].image[0], TB_SPLATMAP2 );
				GL_BindToTMU( pStage->bundle[TB_SPLATMAP2].image[1], TB_SPLATNORMALMAP2 );
			}
			else
			{
				if (pStage->bundle[TB_DIFFUSEMAP].image[0])
				{
					R_BindAnimatedImageToTMU( &pStage->bundle[TB_DIFFUSEMAP], TB_SPLATMAP2);
					GL_BindToTMU( pStage->bundle[TB_NORMALMAP].image[0], TB_SPLATNORMALMAP2 );
				}
				else // will never get used anyway
				{
					GL_BindToTMU( tr.whiteImage, TB_SPLATMAP2 );
					GL_BindToTMU( tr.whiteImage, TB_SPLATNORMALMAP2 );
				}
			}

			if (pStage->bundle[TB_SPLATMAP3].image[0] && pStage->bundle[TB_SPLATMAP3].image[0] != tr.whiteImage)
			{
				GL_BindToTMU( pStage->bundle[TB_SPLATMAP3].image[0], TB_SPLATMAP3 );
				GL_BindToTMU( pStage->bundle[TB_SPLATMAP3].image[1], TB_SPLATNORMALMAP3 );
			}
			else
			{
				if (pStage->bundle[TB_DIFFUSEMAP].image[0])
				{
					R_BindAnimatedImageToTMU( &pStage->bundle[TB_DIFFUSEMAP], TB_SPLATMAP3);
					GL_BindToTMU( pStage->bundle[TB_NORMALMAP].image[0], TB_SPLATNORMALMAP3 );
				}
				else // will never get used anyway
				{
					GL_BindToTMU( tr.whiteImage, TB_SPLATMAP3 );
					GL_BindToTMU( tr.whiteImage, TB_SPLATNORMALMAP3 );
				}
			}

			if (pStage->bundle[TB_SPLATMAP4].image[0] && pStage->bundle[TB_SPLATMAP4].image[0] != tr.whiteImage)
			{
				GL_BindToTMU( pStage->bundle[TB_SPLATMAP4].image[0], TB_SPLATMAP4 );
				GL_BindToTMU( pStage->bundle[TB_SPLATMAP4].image[1], TB_SPLATNORMALMAP4 );
			}
			else
			{
				if (pStage->bundle[TB_DIFFUSEMAP].image[0])
				{
					R_BindAnimatedImageToTMU( &pStage->bundle[TB_DIFFUSEMAP], TB_SPLATMAP4);
					GL_BindToTMU( pStage->bundle[TB_NORMALMAP].image[0], TB_SPLATNORMALMAP4 );
				}
				else // will never get used anyway
				{
					GL_BindToTMU( tr.whiteImage, TB_SPLATMAP4 );
					GL_BindToTMU( tr.whiteImage, TB_SPLATNORMALMAP4 );
				}
			}

			/*if (pStage->bundle[TB_SUBSURFACEMAP].image[0])
			{
			R_BindAnimatedImageToTMU( &pStage->bundle[TB_SUBSURFACEMAP], TB_SUBSURFACEMAP);
			}
			else
			{
			GL_BindToTMU( tr.whiteImage, TB_SUBSURFACEMAP );
			}*/

			if (pStage->bundle[TB_OVERLAYMAP].image[0])
			{
				R_BindAnimatedImageToTMU( &pStage->bundle[TB_OVERLAYMAP], TB_OVERLAYMAP);
			}
			else
			{
				GL_BindToTMU( tr.blackImage, TB_OVERLAYMAP );
			}
		}

		if (!(backEnd.depthFill || (tr.viewParms.flags & VPF_SHADOWPASS)))
		{
			if (r_sunlightMode->integer && (r_sunlightSpecular->integer || (backEnd.viewParms.flags & VPF_USESUNLIGHT)))
			{
				/*if (backEnd.viewParms.flags & VPF_USESUNLIGHT)
				{
					GL_BindToTMU(tr.screenShadowImage, TB_SHADOWMAP);
				}
				else
				{
					GL_BindToTMU(tr.whiteImage, TB_SHADOWMAP);
				}*/

				GLSL_SetUniformVec3(sp, UNIFORM_PRIMARYLIGHTAMBIENT, backEnd.refdef.sunAmbCol);
				GLSL_SetUniformVec3(sp, UNIFORM_PRIMARYLIGHTCOLOR,   backEnd.refdef.sunCol);
				GLSL_SetUniformVec4(sp, UNIFORM_PRIMARYLIGHTORIGIN,  backEnd.refdef.sunDir);


				//if (pStage->glow && !pStage->glowColorFound)
				{// No light added to glow stages...
					GLSL_SetUniformInt(sp, UNIFORM_LIGHTCOUNT, 0);
				}
				/*else
				{
					GLSL_SetUniformInt(sp, UNIFORM_LIGHTCOUNT, NUM_CLOSE_LIGHTS);
					GLSL_SetUniformVec3xX(sp, UNIFORM_LIGHTPOSITIONS2, CLOSEST_LIGHTS_POSITIONS, MAX_LIGHTALL_DLIGHTS);
					GLSL_SetUniformVec3xX(sp, UNIFORM_LIGHTCOLORS, CLOSEST_LIGHTS_COLORS, MAX_LIGHTALL_DLIGHTS);
					GLSL_SetUniformFloatxX(sp, UNIFORM_LIGHTDISTANCES, CLOSEST_LIGHTS_DISTANCES, MAX_LIGHTALL_DLIGHTS);
				}*/
			}
		}

		if (!(backEnd.depthFill || (tr.viewParms.flags & VPF_SHADOWPASS)))
		{
			GL_BindToTMU(tr.whiteImage, TB_NORMALMAP);
			GL_BindToTMU( tr.whiteImage, TB_NORMALMAP2 );
			GL_BindToTMU( tr.whiteImage, TB_NORMALMAP3 );
		}

		//
		// do multitexture
		//
		if (tr.viewParms.flags & VPF_SHADOWPASS)
		{
			if (pStage->bundle[TB_DIFFUSEMAP].image[0])
				R_BindAnimatedImageToTMU(&pStage->bundle[TB_DIFFUSEMAP], TB_DIFFUSEMAP);
			else if (!(pStage->stateBits & GLS_ATEST_BITS))
				GL_BindToTMU(tr.whiteImage, 0);
			else if (pStage->bundle[TB_COLORMAP].image[0] != 0)
				R_BindAnimatedImageToTMU(&pStage->bundle[TB_COLORMAP], TB_COLORMAP);
		}
		else if (backEnd.depthFill)
		{
			if (!(pStage->stateBits & GLS_ATEST_BITS))
				GL_BindToTMU( tr.whiteImage, 0 );
			else if ( pStage->bundle[TB_COLORMAP].image[0] != 0 )
				R_BindAnimatedImageToTMU( &pStage->bundle[TB_COLORMAP], TB_COLORMAP );
		}
		else if ( !isGeneric && (pStage->glslShaderGroup == tr.lightallShader ) )
		{
			int i;
			vec4_t enableTextures;

			VectorSet4(enableTextures, 0, 0, 0, 0);
			if ((r_lightmap->integer == 1 || r_lightmap->integer == 2) && pStage->bundle[TB_LIGHTMAP].image[0])
			{
				for (i = 0; i < NUM_TEXTURE_BUNDLES; i++)
				{
					if (i == TB_LIGHTMAP)
						R_BindAnimatedImageToTMU( &pStage->bundle[TB_LIGHTMAP], i);
					else
						GL_BindToTMU( tr.whiteImage, i );
				}
			}
			else if (r_lightmap->integer == 3 && pStage->bundle[TB_DELUXEMAP].image[0])
			{
				for (i = 0; i < NUM_TEXTURE_BUNDLES; i++)
				{
					if (i == TB_LIGHTMAP)
						R_BindAnimatedImageToTMU( &pStage->bundle[TB_DELUXEMAP], i);
					else
						GL_BindToTMU( tr.whiteImage, i );
				}
			}
			else
			{
				qboolean light = qtrue;
				qboolean fastLight = qfalse;

				if (light && !fastLight)
					usingLight = qtrue;

				if (pStage->bundle[TB_DIFFUSEMAP].image[0])
					R_BindAnimatedImageToTMU( &pStage->bundle[TB_DIFFUSEMAP], TB_DIFFUSEMAP);

				if (pStage->bundle[TB_LIGHTMAP].image[0])
					R_BindAnimatedImageToTMU( &pStage->bundle[TB_LIGHTMAP], TB_LIGHTMAP);
				else
					GL_BindToTMU( tr.whiteImage, TB_LIGHTMAP );

				// bind textures that are sampled and used in the glsl shader, and
				// bind whiteImage to textures that are sampled but zeroed in the glsl shader
				//
				// alternatives:
				//  - use the last bound texture
				//     -> costs more to sample a higher res texture then throw out the result
				//  - disable texture sampling in glsl shader with #ifdefs, as before
				//     -> increases the number of shaders that must be compiled
				//
				//if ((light || pStage->hasRealNormalMap || pStage->hasSpecular /*|| pStage->hasRealSubsurfaceMap*/ || pStage->hasRealOverlayMap || pStage->hasRealSteepMap) && !fastLight)
				{
					if (r_normalMapping->integer >= 2
						&& !input->shader->isPortal
						&& !input->shader->isSky
						&& !pStage->glow
						&& !pStage->bundle[TB_DIFFUSEMAP].normalsLoaded2
						&& (!pStage->bundle[TB_NORMALMAP].image[0] || pStage->bundle[TB_NORMALMAP].image[0] == tr.whiteImage)
						&& pStage->bundle[TB_DIFFUSEMAP].image[0]->imgName[0]
					&& pStage->bundle[TB_DIFFUSEMAP].image[0]->imgName[0] != '*'
						&& pStage->bundle[TB_DIFFUSEMAP].image[0]->imgName[0] != '$'
						&& pStage->bundle[TB_DIFFUSEMAP].image[0]->imgName[0] != '_'
						&& pStage->bundle[TB_DIFFUSEMAP].image[0]->imgName[0] != '!'
						&& !(pStage->bundle[TB_DIFFUSEMAP].image[0]->flags & IMGFLAG_CUBEMAP)
						&& pStage->bundle[TB_DIFFUSEMAP].image[0]->type != IMGTYPE_NORMAL
						&& pStage->bundle[TB_DIFFUSEMAP].image[0]->type != IMGTYPE_SPECULAR
						/*&& pStage->bundle[TB_DIFFUSEMAP].image[0]->type != IMGTYPE_SUBSURFACE*/
						&& pStage->bundle[TB_DIFFUSEMAP].image[0]->type != IMGTYPE_OVERLAY
						&& pStage->bundle[TB_DIFFUSEMAP].image[0]->type != IMGTYPE_STEEPMAP
						&& pStage->bundle[TB_DIFFUSEMAP].image[0]->type != IMGTYPE_STEEPMAP2
						// gfx dirs can be exempted I guess...
						&& !(r_disableGfxDirEnhancement->integer && StringContainsWord(pStage->bundle[TB_DIFFUSEMAP].image[0]->imgName, "gfx/")))
					{// How did this happen??? Oh well, generate a normal map now...
						char imgname[68];
						//ri->Printf(PRINT_WARNING, "Realtime generating normal map for %s.\n", pStage->bundle[TB_DIFFUSEMAP].image[0]->imgName);
						sprintf(imgname, "%s_n", pStage->bundle[TB_DIFFUSEMAP].image[0]->imgName);
						pStage->bundle[TB_NORMALMAP].image[0] = R_CreateNormalMapGLSL( imgname, NULL, pStage->bundle[TB_DIFFUSEMAP].image[0]->width, pStage->bundle[TB_DIFFUSEMAP].image[0]->height, pStage->bundle[TB_DIFFUSEMAP].image[0]->flags, pStage->bundle[TB_DIFFUSEMAP].image[0] );

						if (pStage->bundle[TB_NORMALMAP].image[0] && pStage->bundle[TB_NORMALMAP].image[0] != tr.whiteImage)
						{
							pStage->hasRealNormalMap = true;
							RB_SetMaterialBasedProperties(sp, pStage);

							if (pStage->normalScale[0] == 0 && pStage->normalScale[1] == 0 && pStage->normalScale[2] == 0)
							{
								VectorSet4(pStage->normalScale, r_baseNormalX->value, r_baseNormalY->value, 1.0f, r_baseParallax->value);
								GLSL_SetUniformVec4(sp, UNIFORM_NORMALSCALE, pStage->normalScale);
							}
						}

						//if (pStage->bundle[TB_NORMALMAP].image[0] != tr.whiteImage)
						pStage->bundle[TB_DIFFUSEMAP].normalsLoaded2 = qtrue;
					}

					if (pStage->bundle[TB_NORMALMAP].image[0])
					{
						R_BindAnimatedImageToTMU( &pStage->bundle[TB_NORMALMAP], TB_NORMALMAP);
						enableTextures[0] = 1.0f;
					}
					else if (r_normalMapping->integer >= 2)
					{
						GL_BindToTMU( tr.whiteImage, TB_NORMALMAP );
					}

					if (pStage->bundle[TB_NORMALMAP2].image[0])
					{
						R_BindAnimatedImageToTMU( &pStage->bundle[TB_NORMALMAP2], TB_NORMALMAP2);
					}
					else if (r_normalMapping->integer >= 2)
					{
						GL_BindToTMU( tr.whiteImage, TB_NORMALMAP2 );
					}

					if (pStage->bundle[TB_NORMALMAP3].image[0])
					{
						R_BindAnimatedImageToTMU( &pStage->bundle[TB_NORMALMAP3], TB_NORMALMAP3);
					}
					else if (r_normalMapping->integer >= 2)
					{
						GL_BindToTMU( tr.whiteImage, TB_NORMALMAP3 );
					}

					if (pStage->bundle[TB_DELUXEMAP].image[0])
					{
						R_BindAnimatedImageToTMU( &pStage->bundle[TB_DELUXEMAP], TB_DELUXEMAP);
						enableTextures[1] = 1.0f;
					}
					else if (r_deluxeMapping->integer)
					{
						GL_BindToTMU( tr.whiteImage, TB_DELUXEMAP );
					}

					if (pStage->bundle[TB_SPECULARMAP].image[0])
					{
						R_BindAnimatedImageToTMU( &pStage->bundle[TB_SPECULARMAP], TB_SPECULARMAP);
						enableTextures[2] = 1.0f;
					}
					else if (r_specularMapping->integer)
					{
						GL_BindToTMU( tr.whiteImage, TB_SPECULARMAP );
					}
				}

				if (backEnd.depthFill || (tr.viewParms.flags & VPF_SHADOWPASS))
				{
					enableTextures[3] = 0.0f;
				}
				else
				{
					enableTextures[3] = (r_cubeMapping->integer >= 1 && !(tr.viewParms.flags & VPF_NOCUBEMAPS) && input->cubemapIndex) ? 1.0f : 0.0f;
				}
			}

			GLSL_SetUniformVec4(sp, UNIFORM_ENABLETEXTURES, enableTextures);
		}
		else if ( pStage->bundle[1].image[0] != 0 )
		{
			R_BindAnimatedImageToTMU( &pStage->bundle[0], 0 );
			R_BindAnimatedImageToTMU( &pStage->bundle[1], 1 );
		}
		else
		{
			//
			// set state
			//
			R_BindAnimatedImageToTMU( &pStage->bundle[0], 0 );
		}

#if 0
		if (glState.currentFBO == tr.renderFbo)
		{
			if (isGeneric /*|| tess.shader->hasAlpha*/ || (pStage->type != ST_DIFFUSEMAP && pStage->type != ST_GLSL))
			{// So we dont ever output to position and normal maps...
				GLSL_AttachGenericTextures();
			}
			else
			{
				GLSL_AttachTextures();
			}
		}
#endif

		/*
		// Seems to work, but doesnt seem to be required...
		if (isGeneric)
		{
			//GLSL_AttachGenericTextures();
		}
		else if (isLightAll && isGlowStage)
		{
			GLSL_AttachGlowTextures();
		}
		else if (isLightAll && pStage->type != ST_GLSL)
		{
			//GLSL_AttachGlowTextures();
			GLSL_AttachGenericTextures();
		}
		else if (isLightAll)
		{
			GLSL_AttachTextures();
		}
		*/
		

		while (1)
		{
			float tessInner = 0.0;
			float tessOuter = 0.0;
			float tessAlpha = 0.0;

			if (useTesselation)
			{
				tessInner = RB_GetTesselationInnerLevel(tess.shader->surfaceFlags & MATERIAL_MASK);
				tessOuter = tessInner;
				tessAlpha = RB_GetTesselationAlphaLevel(tess.shader->surfaceFlags & MATERIAL_MASK);
			}

			if (isGrass && passNum == 1 && sp2)
			{// Switch to grass geometry shader, once... Repeats will reuse it...
				sp = sp2;
				sp2 = NULL;

				GLSL_BindProgram(sp);

				stateBits = GLS_DEPTHMASK_TRUE;

				RB_SetMaterialBasedProperties(sp, pStage);

				GLSL_SetUniformFloat(sp, UNIFORM_TIME, tess.shaderTime);

				GLSL_SetUniformMatrix16(sp, UNIFORM_VIEWPROJECTIONMATRIX, MATRIX_VP);
				GLSL_SetUniformMatrix16(sp, UNIFORM_MODELMATRIX, backEnd.ori.transformMatrix);
				GLSL_SetUniformMatrix16(sp, UNIFORM_MODELVIEWPROJECTIONMATRIX, glState.modelviewProjection);
				GLSL_SetUniformMatrix16(sp, UNIFORM_INVEYEPROJECTIONMATRIX, glState.invEyeProjection);

				// UQ: Other *needed* stuff... Hope these are correct...
				GLSL_SetUniformMatrix16(sp, UNIFORM_PROJECTIONMATRIX, glState.projection);
				GLSL_SetUniformMatrix16(sp, UNIFORM_MODELVIEWMATRIX, MATRIX_MODEL);
				GLSL_SetUniformMatrix16(sp, UNIFORM_VIEWMATRIX, MATRIX_TRANS);
				GLSL_SetUniformMatrix16(sp, UNIFORM_INVVIEWMATRIX, MATRIX_INVTRANS);
				GLSL_SetUniformMatrix16(sp, UNIFORM_NORMALMATRIX, MATRIX_NORMAL);
				GLSL_SetUniformMatrix16(sp, UNIFORM_INVMODELVIEWMATRIX, MATRIX_INVMV);

				GLSL_SetUniformVec3(sp, UNIFORM_LOCALVIEWORIGIN, backEnd.ori.viewOrigin);
				GLSL_SetUniformFloat(sp, UNIFORM_VERTEXLERP, glState.vertexAttribsInterpolation);

				GLSL_SetUniformVec3(sp, UNIFORM_VIEWORIGIN, backEnd.viewParms.ori.origin);

				GL_BindToTMU( tr.grassImage[0], TB_DIFFUSEMAP );
				GL_BindToTMU( tr.grassImage[1], TB_SPLATMAP1 );
				GL_BindToTMU( tr.grassImage[2], TB_SPLATMAP2 );
				GL_BindToTMU( tr.seaGrassImage, TB_OVERLAYMAP );

				vec4_t l10;
				VectorSet4(l10, r_foliageDistance->value, r_foliageDensity->value, MAP_WATER_LEVEL, 0.0);
				GLSL_SetUniformVec4(sp, UNIFORM_LOCAL10, l10);

				GLSL_SetUniformVec3(sp, UNIFORM_PRIMARYLIGHTAMBIENT, backEnd.refdef.sunAmbCol);
				GLSL_SetUniformVec3(sp, UNIFORM_PRIMARYLIGHTCOLOR,   backEnd.refdef.sunCol);
				GLSL_SetUniformVec4(sp, UNIFORM_PRIMARYLIGHTORIGIN,  backEnd.refdef.sunDir);

				GL_BindToTMU( tr.defaultGrassMapImage, TB_SPLATCONTROLMAP );

				/*
				if (r_sunlightMode->integer && (r_sunlightSpecular->integer || (backEnd.viewParms.flags & VPF_USESUNLIGHT)))
				{
					if (backEnd.viewParms.flags & VPF_USESUNLIGHT)
					{
						GL_BindToTMU(tr.screenShadowImage, TB_SHADOWMAP);
					}
					else
					{
						GL_BindToTMU(tr.whiteImage, TB_SHADOWMAP);
					}
				}
				*/
			}
			else if (isGrass && passNum > r_foliagePasses->integer && sp3)
			{// Switch to pebbles geometry shader, once... Repeats will reuse it...
				sp = sp3;
				sp3 = NULL;

				GLSL_BindProgram(sp);

				stateBits = GLS_DEPTHMASK_TRUE;

				RB_SetMaterialBasedProperties(sp, pStage);

				GLSL_SetUniformFloat(sp, UNIFORM_TIME, tess.shaderTime);

				GLSL_SetUniformMatrix16(sp, UNIFORM_VIEWPROJECTIONMATRIX, MATRIX_VP);
				GLSL_SetUniformMatrix16(sp, UNIFORM_MODELMATRIX, backEnd.ori.transformMatrix);
				GLSL_SetUniformMatrix16(sp, UNIFORM_MODELVIEWPROJECTIONMATRIX, glState.modelviewProjection);
				GLSL_SetUniformMatrix16(sp, UNIFORM_INVEYEPROJECTIONMATRIX, glState.invEyeProjection);

				// UQ: Other *needed* stuff... Hope these are correct...
				GLSL_SetUniformMatrix16(sp, UNIFORM_PROJECTIONMATRIX, glState.projection);
				GLSL_SetUniformMatrix16(sp, UNIFORM_MODELVIEWMATRIX, MATRIX_MODEL);
				GLSL_SetUniformMatrix16(sp, UNIFORM_VIEWMATRIX, MATRIX_TRANS);
				GLSL_SetUniformMatrix16(sp, UNIFORM_INVVIEWMATRIX, MATRIX_INVTRANS);
				GLSL_SetUniformMatrix16(sp, UNIFORM_NORMALMATRIX, MATRIX_NORMAL);
				GLSL_SetUniformMatrix16(sp, UNIFORM_INVMODELVIEWMATRIX, MATRIX_INVMV);

				GLSL_SetUniformVec3(sp, UNIFORM_LOCALVIEWORIGIN, backEnd.ori.viewOrigin);
				GLSL_SetUniformFloat(sp, UNIFORM_VERTEXLERP, glState.vertexAttribsInterpolation);

				GLSL_SetUniformVec3(sp, UNIFORM_VIEWORIGIN, backEnd.viewParms.ori.origin);

				GL_BindToTMU(tr.pebblesImage[0], TB_DIFFUSEMAP);
				GL_BindToTMU(tr.pebblesImage[1], TB_SPLATMAP1);
				GL_BindToTMU(tr.pebblesImage[2], TB_SPLATMAP2);
				GL_BindToTMU(tr.pebblesImage[3], TB_OVERLAYMAP );

				vec4_t l10;
				float tessOffset = useTesselation ? 7.5 : 0.0;
				VectorSet4(l10, r_pebblesDistance->value, r_foliageDensity->value, MAP_WATER_LEVEL, tessOffset);
				GLSL_SetUniformVec4(sp, UNIFORM_LOCAL10, l10);

				GLSL_SetUniformVec3(sp, UNIFORM_PRIMARYLIGHTAMBIENT, backEnd.refdef.sunAmbCol);
				GLSL_SetUniformVec3(sp, UNIFORM_PRIMARYLIGHTCOLOR, backEnd.refdef.sunCol);
				GLSL_SetUniformVec4(sp, UNIFORM_PRIMARYLIGHTORIGIN, backEnd.refdef.sunDir);

				GL_BindToTMU(tr.defaultGrassMapImage, TB_SPLATCONTROLMAP);

				/*
				if (r_sunlightMode->integer && (r_sunlightSpecular->integer || (backEnd.viewParms.flags & VPF_USESUNLIGHT)))
				{
					if (backEnd.viewParms.flags & VPF_USESUNLIGHT)
					{
						GL_BindToTMU(tr.screenShadowImage, TB_SHADOWMAP);
					}
					else
					{
						GL_BindToTMU(tr.whiteImage, TB_SHADOWMAP);
					}
				}
				*/
			}
			else if (isPebbles && passNum == 1 && sp2)
			{// Switch to pebbles geometry shader, once... Repeats will reuse it...
				sp = sp2;
				sp2 = NULL;

				GLSL_BindProgram(sp);

				stateBits = GLS_DEPTHMASK_TRUE;

				RB_SetMaterialBasedProperties(sp, pStage);

				GLSL_SetUniformFloat(sp, UNIFORM_TIME, tess.shaderTime);

				GLSL_SetUniformMatrix16(sp, UNIFORM_VIEWPROJECTIONMATRIX, MATRIX_VP);
				GLSL_SetUniformMatrix16(sp, UNIFORM_MODELMATRIX, backEnd.ori.transformMatrix);
				GLSL_SetUniformMatrix16(sp, UNIFORM_MODELVIEWPROJECTIONMATRIX, glState.modelviewProjection);
				GLSL_SetUniformMatrix16(sp, UNIFORM_INVEYEPROJECTIONMATRIX, glState.invEyeProjection);

				// UQ: Other *needed* stuff... Hope these are correct...
				GLSL_SetUniformMatrix16(sp, UNIFORM_PROJECTIONMATRIX, glState.projection);
				GLSL_SetUniformMatrix16(sp, UNIFORM_MODELVIEWMATRIX, MATRIX_MODEL);
				GLSL_SetUniformMatrix16(sp, UNIFORM_VIEWMATRIX, MATRIX_TRANS);
				GLSL_SetUniformMatrix16(sp, UNIFORM_INVVIEWMATRIX, MATRIX_INVTRANS);
				GLSL_SetUniformMatrix16(sp, UNIFORM_NORMALMATRIX, MATRIX_NORMAL);
				GLSL_SetUniformMatrix16(sp, UNIFORM_INVMODELVIEWMATRIX, MATRIX_INVMV);

				GLSL_SetUniformVec3(sp, UNIFORM_LOCALVIEWORIGIN, backEnd.ori.viewOrigin);
				GLSL_SetUniformFloat(sp, UNIFORM_VERTEXLERP, glState.vertexAttribsInterpolation);

				GLSL_SetUniformVec3(sp, UNIFORM_VIEWORIGIN, backEnd.viewParms.ori.origin);

				GL_BindToTMU(tr.pebblesImage[0], TB_DIFFUSEMAP);
				GL_BindToTMU(tr.pebblesImage[1], TB_SPLATMAP1);
				GL_BindToTMU(tr.pebblesImage[2], TB_SPLATMAP2);
				GL_BindToTMU(tr.pebblesImage[3], TB_OVERLAYMAP);

				vec4_t l10;
				float tessOffset = useTesselation ? 7.5 : 0.0;
				VectorSet4(l10, r_pebblesDistance->value, r_foliageDensity->value, MAP_WATER_LEVEL, tessOffset);
				GLSL_SetUniformVec4(sp, UNIFORM_LOCAL10, l10);

				GLSL_SetUniformVec3(sp, UNIFORM_PRIMARYLIGHTAMBIENT, backEnd.refdef.sunAmbCol);
				GLSL_SetUniformVec3(sp, UNIFORM_PRIMARYLIGHTCOLOR, backEnd.refdef.sunCol);
				GLSL_SetUniformVec4(sp, UNIFORM_PRIMARYLIGHTORIGIN, backEnd.refdef.sunDir);

				GL_BindToTMU(tr.defaultGrassMapImage, TB_SPLATCONTROLMAP);

				/*
				if (r_sunlightMode->integer && (r_sunlightSpecular->integer || (backEnd.viewParms.flags & VPF_USESUNLIGHT)))
				{
					if (backEnd.viewParms.flags & VPF_USESUNLIGHT)
					{
						GL_BindToTMU(tr.screenShadowImage, TB_SHADOWMAP);
					}
					else
					{
						GL_BindToTMU(tr.whiteImage, TB_SHADOWMAP);
					}
				}
				*/
			}

			if ((isGrass || isPebbles) && passNum > 0)
			{// Use grass map...

			}
			else
			{
				if (!(backEnd.depthFill || (tr.viewParms.flags & VPF_SHADOWPASS)))
				{
					if (pStage->bundle[TB_SPLATCONTROLMAP].image[0] && pStage->bundle[TB_SPLATCONTROLMAP].image[0] != tr.blackImage)
					{
						//ri->Printf(PRINT_WARNING, "Image %s bound to splat control map %i x %i.\n", pStage->bundle[TB_SPLATCONTROLMAP].image[0]->imgName, pStage->bundle[TB_SPLATCONTROLMAP].image[0]->width, pStage->bundle[TB_SPLATCONTROLMAP].image[0]->height);
						GL_BindToTMU(pStage->bundle[TB_SPLATCONTROLMAP].image[0], TB_SPLATCONTROLMAP);
					}
					else
					{
						//GL_BindToTMU( tr.blackImage, TB_SPLATCONTROLMAP ); // bind black image so we never use any of the splat images
						GL_BindToTMU(tr.defaultSplatControlImage, TB_SPLATCONTROLMAP); // really need to make a blured (possibly also considering heightmap) version of this...
					}
				}
			}

#ifdef __USE_WATERMAP__
			if (isWater && r_glslWater->integer && MAP_WATER_LEVEL > -131072.0)
			{// Attach dummy water output textures...
				if (glState.currentFBO == tr.renderFbo)
				{// Only attach textures when doing a render pass...
					stateBits = GLS_DEFAULT | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_DEPTHMASK_TRUE;
					tess.shader->cullType = CT_TWO_SIDED; // Always...

					/*if (passNum > 0)
					{
						GLSL_AttachWaterTextures2();
					}
					else*/
					{
						GLSL_AttachWaterTextures();
					}
				}
				else
				{
					break;
				}
			}
#else //!__USE_WATERMAP__
			if (isWater && r_glslWater->integer && MAP_WATER_LEVEL > -131072.0)
			{
				stateBits = GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
			}
#endif //__USE_WATERMAP__

			qboolean tesselation = qfalse;

			if (isGrass && passNum > 0 && r_foliage->integer)
			{// Geometry grass drawing passes...
				vec4_t l8;
				VectorSet4(l8, (float)passNum, 0.0, 0.0, 0.0);
				GLSL_SetUniformVec4(sp, UNIFORM_LOCAL8, l8);

				GL_Cull( CT_TWO_SIDED );
			}
			else if (isGrass && passNum > r_foliagePasses->integer && r_pebbles->integer)
			{// Geometry pebbles drawing passes...
				vec4_t l8;
				VectorSet4(l8, (float)passNum, 0.0, 0.0, 0.0);
				GLSL_SetUniformVec4(sp, UNIFORM_LOCAL8, l8);

				GL_Cull(CT_TWO_SIDED);
			}
			else if (isPebbles && passNum > 0 && r_pebbles->integer)
			{// Geometry pebbles drawing passes...
				vec4_t l8;
				VectorSet4(l8, (float)passNum, 0.0, 0.0, 0.0);
				GLSL_SetUniformVec4(sp, UNIFORM_LOCAL8, l8);

				GL_Cull(CT_TWO_SIDED);
			}
			else if (r_tesselation->integer && sp->tesselation)
			{
				tesselation = qtrue;

				//float tessInner = RB_GetTesselationInnerLevel(tess.shader->surfaceFlags & MATERIAL_MASK);
				//float tessOuter = tessInner;
				//float tessAlpha = RB_GetTesselationAlphaLevel(tess.shader->surfaceFlags & MATERIAL_MASK);

				vec4_t l10;
				VectorSet4(l10, tessAlpha, tessInner, tessOuter, 0.0);
				GLSL_SetUniformVec4(sp, UNIFORM_LOCAL10, l10);
			}

			vec4_t l9;
			VectorSet4(l9, r_testshaderValue1->value, r_testshaderValue2->value, r_testshaderValue3->value, r_testshaderValue4->value);
			GLSL_SetUniformVec4(sp, UNIFORM_LOCAL9, l9);

#ifdef __USE_WATERMAP__
			if (isWater && r_glslWater->integer && MAP_WATER_LEVEL > -131072.0)
			{// Attach dummy water output textures...
				if (glState.currentFBO == tr.renderFbo)
				{// Only attach textures when doing a render pass...
					vec4_t passInfo;
					VectorSet4(passInfo, /*passNum*/0.0, r_waterWaveHeight->value, 0.0, 0.0);
					GLSL_SetUniformVec4(sp, UNIFORM_LOCAL10, passInfo);
				}
			}
#endif //__USE_WATERMAP__

			UpdateTexCoords (pStage);

			GL_State( stateBits );

			//
			// draw
			//

			qboolean occluded = qfalse;
			
#ifdef __ORIGINAL_OCCLUSION__
			if (r_occlusion->integer 
				&& !isGeneric 
				&& !(tr.viewParms.flags & VPF_SHADOWPASS) 
				&& !backEnd.depthFill 
				/*&& backEnd.viewParms.targetFbo != tr.renderCubeFbo*/)
			{
				if (RB_CheckOcclusion(glState.modelviewProjection, input))
					break;
			}
#endif //__ORIGINAL_OCCLUSION__

			if (input->multiDrawPrimitives)
			{
				R_DrawMultiElementsVBO(input->multiDrawPrimitives, input->multiDrawMinIndex, input->multiDrawMaxIndex, input->multiDrawNumIndexes, input->multiDrawFirstIndex, input->numVertexes, tesselation);
			}
			else
			{
				R_DrawElementsVBO(input->numIndexes, input->firstIndex, input->minIndex, input->maxIndex, input->numVertexes, tesselation);
			}

#ifdef __USE_WATERMAP__
			if (isWater && r_glslWater->integer && MAP_WATER_LEVEL > -131072.0)
			{// Unattach dummy water output textures...
				if (glState.currentFBO == tr.renderFbo)
				{// Only attach textures when doing a render pass...
					GLSL_AttachTextures();
				}
			}
#endif //__USE_WATERMAP__

			passNum++;

			if (multiPass && passNum > passMax)
			{// Finished all passes...
				multiPass = qfalse;
			}

			if (!multiPass)
			{
				if ((isGrass && r_foliage->integer) || (isPebbles && r_pebbles->integer))
				{// Set cull type back to original... Just in case...
					GL_Cull( input->shader->cullType );
				}

				break;
			}
		}

		// allow skipping out to show just lightmaps during development
		if ( r_lightmap->integer && ( pStage->bundle[0].isLightmap || pStage->bundle[1].isLightmap ) )
		{
			break;
		}

		if (backEnd.depthFill)
		//if (!(backEnd.depthFill || (tr.viewParms.flags & VPF_SHADOWPASS)))
			break;
	}
}

void RB_ExternalIterateStagesGeneric( shaderCommands_t *input )
{
	RB_IterateStagesGeneric( input );
}


static void RB_RenderShadowmap( shaderCommands_t *input )
{
	int deformGen;
	vec5_t deformParams;

	ComputeDeformValues(&deformGen, deformParams);

	{
		shaderProgram_t *sp = &tr.shadowmapShader;

		vec4_t vector;

		GLSL_BindProgram(sp);

		GLSL_SetUniformMatrix16(sp, UNIFORM_MODELVIEWPROJECTIONMATRIX, glState.modelviewProjection);

		GLSL_SetUniformMatrix16(sp, UNIFORM_MODELMATRIX, backEnd.ori.transformMatrix);

		GLSL_SetUniformFloat(sp, UNIFORM_VERTEXLERP, glState.vertexAttribsInterpolation);

		GLSL_SetUniformInt(sp, UNIFORM_DEFORMGEN, deformGen);
		if (deformGen != DGEN_NONE)
		{
			GLSL_SetUniformFloat5(sp, UNIFORM_DEFORMPARAMS, deformParams);
			GLSL_SetUniformFloat(sp, UNIFORM_TIME, tess.shaderTime);
		}

		VectorCopy(backEnd.viewParms.ori.origin, vector);
		vector[3] = 1.0f;
		GLSL_SetUniformVec4(sp, UNIFORM_LIGHTORIGIN, vector);
		GLSL_SetUniformFloat(sp, UNIFORM_LIGHTRADIUS, backEnd.viewParms.zFar);

		GL_State( 0 );

		//
		// do multitexture
		//
		//if ( pStage->glslShaderGroup )
		{
			//
			// draw
			//

			if (input->multiDrawPrimitives)
			{
				R_DrawMultiElementsVBO(input->multiDrawPrimitives, input->multiDrawMinIndex, input->multiDrawMaxIndex, input->multiDrawNumIndexes, input->multiDrawFirstIndex, input->numVertexes, qfalse);
			}
			else
			{
				R_DrawElementsVBO(input->numIndexes, input->firstIndex, input->minIndex, input->maxIndex, input->numVertexes, qfalse);
			}
		}
	}
}


/*
** RB_StageIteratorGeneric
*/
void RB_StageIteratorGeneric( void )
{
	shaderCommands_t *input;
	unsigned int vertexAttribs = 0;

	input = &tess;

	if (!input->numVertexes || !input->numIndexes)
	{
		return;
	}

	if (tess.useInternalVBO)
	{
		RB_DeformTessGeometry();
	}

	vertexAttribs = RB_CalcShaderVertexAttribs( input->shader );

	if (tess.useInternalVBO)
	{
		RB_UpdateVBOs(vertexAttribs);
	}
	else
	{
		backEnd.pc.c_staticVboDraws++;
	}

	//
	// log this call
	//
	if ( r_logFile->integer )
	{
		// don't just call LogComment, or we will get
		// a call to va() every frame!
		GLimp_LogComment( va("--- RB_StageIteratorGeneric( %s ) ---\n", tess.shader->name) );
	}

	//
	// set face culling appropriately
	//
	if ((backEnd.viewParms.flags & VPF_DEPTHSHADOW))
	{
		//GL_Cull( CT_TWO_SIDED );

		if (input->shader->cullType == CT_TWO_SIDED)
			GL_Cull( CT_TWO_SIDED );
		else if (input->shader->cullType == CT_FRONT_SIDED)
			GL_Cull( CT_BACK_SIDED );
		else
			GL_Cull( CT_FRONT_SIDED );

	}
	else
		GL_Cull( input->shader->cullType );

	// set polygon offset if necessary
	if ( input->shader->polygonOffset )
	{
		qglEnable( GL_POLYGON_OFFSET_FILL );
		qglPolygonOffset( r_offsetFactor->value, r_offsetUnits->value );
	}

	//
	// Set vertex attribs and pointers
	//
	GLSL_VertexAttribsState(vertexAttribs);

	//
	// UQ1: Set up any special shaders needed for this surface/contents type...
	//

	if ((tess.shader->isWater && r_glslWater->integer)
		|| (tess.shader->contentFlags & CONTENTS_WATER)
		/*|| (tess.shader->contentFlags & CONTENTS_LAVA)*/
		|| (tess.shader->surfaceFlags & MATERIAL_MASK) == MATERIAL_WATER)
	{
		if (input && input->xstages[0] && input->xstages[0]->isWater == 0 && r_glslWater->integer) // In case it is already set, no need looping more then once on the same shader...
		{
			int isWater = 1;

			if (tess.shader->contentFlags & CONTENTS_LAVA)
				isWater = 2;

			for ( int stage = 0; stage < MAX_SHADER_STAGES; stage++ )
			{
				if (input->xstages[stage])
				{
					input->xstages[stage]->isWater = isWater;
					//input->xstages[stage]->glslShaderGroup = tr.lightallShader;
				}
			}
		}
	}

	//
	// render depth if in depthfill mode
	//
	if (backEnd.depthFill)
	{
		RB_IterateStagesGeneric( input );

		//
		// reset polygon offset
		//
		if ( input->shader->polygonOffset )
		{
			qglDisable( GL_POLYGON_OFFSET_FILL );
		}

		return;
	}

	//
	// render shadowmap if in shadowmap mode
	//
	if (backEnd.viewParms.flags & VPF_SHADOWMAP)
	{
		if ( input->shader->sort == SS_OPAQUE )
		{
			RB_RenderShadowmap( input );
		}
		//
		// reset polygon offset
		//
		if ( input->shader->polygonOffset )
		{
			qglDisable( GL_POLYGON_OFFSET_FILL );
		}

		return;
	}

	//
	//
	// call shader function
	//
	RB_IterateStagesGeneric( input );

#ifdef __PSHADOWS__
	ProjectPshadowVBOGLSL();
#endif

	// Now check for surfacesprites.
#if 0
	if (r_surfaceSprites->integer)
	{
		//for ( int stage = 1; stage < MAX_SHADER_STAGES/*tess.shader->numUnfoggedPasses*/; stage++ )
		for ( int stage = 0; stage < MAX_SHADER_STAGES/*tess.shader->numUnfoggedPasses*/; stage++ )
		{
			if (tess.xstages[stage])
			{
				if (tess.xstages[stage]->ss && tess.xstages[stage]->ss->surfaceSpriteType)
				{	// Draw the surfacesprite
					RB_DrawSurfaceSprites( tess.xstages[stage], input);
				}
			}
		}
	}
#endif

	//
	// now do fog
	//
	if ( r_fog->integer && tess.fogNum && tess.shader->fogPass ) {
		RB_FogPass();
	}

	//
	// reset polygon offset
	//
	if ( input->shader->polygonOffset )
	{
		qglDisable( GL_POLYGON_OFFSET_FILL );
	}
}


/*
** RB_EndSurface
*/
void RB_EndSurface( void ) {
	shaderCommands_t *input;

	input = &tess;

	if (input->numIndexes == 0 || input->numVertexes == 0) {
		return;
	}

	if (input->indexes[SHADER_MAX_INDEXES-1] != 0) {
		ri->Error (ERR_DROP, "RB_EndSurface() - SHADER_MAX_INDEXES hit");
	}
	if (input->xyz[SHADER_MAX_VERTEXES-1][0] != 0) {
		ri->Error (ERR_DROP, "RB_EndSurface() - SHADER_MAX_VERTEXES hit");
	}

	if ( tess.shader == tr.shadowShader ) {
		RB_ShadowTessEnd();
		return;
	}

	// for debugging of sort order issues, stop rendering after a given sort value
	if ( r_debugSort->integer && r_debugSort->integer < tess.shader->sort ) {
		return;
	}

	//
	// update performance counters
	//
	backEnd.pc.c_shaders++;
	backEnd.pc.c_vertexes += tess.numVertexes;
	backEnd.pc.c_indexes += tess.numIndexes;
	backEnd.pc.c_totalIndexes += tess.numIndexes * tess.numPasses;

	//
	// call off to shader specific tess end function
	//
	tess.currentStageIteratorFunc();

	//
	// draw debugging stuff
	//
	if ( r_showtris->integer ) {
		DrawTris (input);
	}
	if ( r_shownormals->integer ) {
		DrawNormals (input);
	}
	// clear shader so we can tell we don't have any unclosed surfaces
	tess.numIndexes = 0;
	tess.numVertexes = 0;
	tess.firstIndex = 0;
	tess.multiDrawPrimitives = 0;

	GLimp_LogComment( "----------\n" );
}
