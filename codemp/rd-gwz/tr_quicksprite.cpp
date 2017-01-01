// tr_QuickSprite.cpp: implementation of the CQuickSpriteSystem class.
//
//////////////////////////////////////////////////////////////////////
#include "tr_local.h"

#include "tr_quicksprite.h"

extern void R_BindAnimatedImageToTMU( textureBundle_t *bundle, int tmu );


//////////////////////////////////////////////////////////////////////
// Singleton System
//////////////////////////////////////////////////////////////////////
CQuickSpriteSystem SQuickSprite;


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CQuickSpriteSystem::CQuickSpriteSystem()
{
	int i;

	for (i=0; i<SHADER_MAX_VERTEXES; i+=4)
	{
		// Bottom right
		mTextureCoords[i+0][0] = 1.0;
		mTextureCoords[i+0][1] = 1.0;
		// Top right
		mTextureCoords[i+1][0] = 1.0;
		mTextureCoords[i+1][1] = 0.0;
		// Top left
		mTextureCoords[i+2][0] = 0.0;
		mTextureCoords[i+2][1] = 0.0;
		// Bottom left
		mTextureCoords[i+3][0] = 0.0;
		mTextureCoords[i+3][1] = 1.0;
	}
}

CQuickSpriteSystem::~CQuickSpriteSystem()
{

}


void CQuickSpriteSystem::Flush(void)
{
	if (mNextVert==0)
	{
		return;
	}

	/*
	if (mUseFog && r_drawfog->integer == 2 &&
		mFogIndex == tr.world->globalFog)
	{ //enable hardware fog when we draw this thing if applicable -rww
		fog_t *fog = tr.world->fogs + mFogIndex;

		qglFogf(GL_FOG_MODE, GL_EXP2);
		qglFogf(GL_FOG_DENSITY, logtestExp2 / fog->parms.depthForOpaque);
		qglFogfv(GL_FOG_COLOR, fog->parms.color);
		qglEnable(GL_FOG);
	}
	*/
	//this should not be needed, since I just wait to disable fog for the surface til after surface sprites are done
	//ri->Printf(PRINT_WARNING, "Drawing ss.\n");

	//
	// render the main pass
	//
	//qglLoadIdentity ();

	R_BindAnimatedImageToTMU( mTexBundle, TB_DIFFUSEMAP );
	//GL_State(mGLStateBits);

	//
	// set arrays and lock
	//
	/*qglTexCoordPointer( 2, GL_FLOAT, 0, mTextureCoords );
	qglEnableClientState( GL_TEXTURE_COORD_ARRAY);

	qglEnableClientState( GL_COLOR_ARRAY);
	qglColorPointer( 4, GL_UNSIGNED_BYTE, 0, mColors );

	qglVertexPointer (3, GL_FLOAT, 16, mVerts);*/

	//if ( qglLockArraysEXT )
	//{
	//	qglLockArraysEXT(0, mNextVert);
	//	GLimp_LogComment( "glLockArraysEXT\n" );
	//}

	//qglBegin( GL_QUADS );
	////qglDrawArraysInstanced?
	//qglDrawArrays(GL_QUADS, 0, mNextVert);
	//qglEnd ();

	GLSL_BindProgram(&tr.textureColorShader);
	
	GLSL_SetUniformMatrix16(&tr.textureColorShader, UNIFORM_MODELVIEWPROJECTIONMATRIX, glState.modelviewProjection);
	GLSL_SetUniformVec4(&tr.textureColorShader, UNIFORM_COLOR, colorWhite);

	//qglDrawRangeElements(GL_QUADS, 0, mNextVert, mNextVert, GL_INDEX_TYPE, mVerts);
	//RB_InstantQuad(mVerts);
	//RB_InstantQuad2(vec4_t quadVerts[4], vec2_t texCoords[4])

	for (int i = 0; i < mNextVert; i+=4)
	{
		vec4_t v[4];
		vec2_t t[4];
		VectorCopy4(mVerts[i], v[0]);
		VectorCopy4(mVerts[i+1], v[1]);
		VectorCopy4(mVerts[i+2], v[2]);
		VectorCopy4(mVerts[i+3], v[3]);
		VectorCopy2(mTextureCoords[i], t[0]);
		VectorCopy2(mTextureCoords[i+1], t[1]);
		VectorCopy2(mTextureCoords[i+2], t[2]);
		VectorCopy2(mTextureCoords[i+3], t[3]);
		RB_InstantQuad2(v, t);
		
		if(r_surfaceSprites->integer >= 4)
			ri->Printf(PRINT_WARNING, "v[0] %f %f %f %f v[1] %f %f %f %f v[2] %f %f %f %f v[3] %f %f %f %f\n", v[0][0], v[0][1], v[0][2], v[0][3], v[1][0], v[1][1], v[1][2], v[1][3], v[2][0], v[2][1], v[2][2], v[2][3], v[3][0], v[3][1], v[3][2], v[3][3]);
	}
	//RB_InstantQuad2(mVerts, mTextureCoords);

	backEnd.pc.c_vertexes += mNextVert;
	backEnd.pc.c_indexes += mNextVert;
	backEnd.pc.c_totalIndexes += mNextVert;

	//only for software fog pass (global soft/volumetric) -rww
#if 0
	if (mUseFog && (r_drawfog->integer != 2 || mFogIndex != tr.world->globalFog))
	{
		fog_t *fog = tr.world->fogs + mFogIndex;

		//
		// render the fog pass
		//
		GL_Bind( tr.fogImage );
		GL_State( GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_DEPTHFUNC_EQUAL );

		//
		// set arrays and lock
		//
		qglTexCoordPointer( 2, GL_FLOAT, 0, mFogTextureCoords);
//		qglEnableClientState( GL_TEXTURE_COORD_ARRAY);	// Done above

		qglDisableClientState( GL_COLOR_ARRAY );
		qglColor4ubv((GLubyte *)&fog->colorInt);

//		qglVertexPointer (3, GL_FLOAT, 16, mVerts);	// Done above

		qglDrawArrays(GL_QUADS, 0, mNextVert);

		// Second pass from fog
		backEnd.pc.c_totalIndexes += mNextVert;
	}
#endif

	//
	// unlock arrays
	//
	//if (qglUnlockArraysEXT)
	//{
	//	qglUnlockArraysEXT();
	//	GLimp_LogComment( "glUnlockArraysEXT\n" );
	//}

	mNextVert=0;
}


void CQuickSpriteSystem::StartGroup(textureBundle_t *bundle, uint32_t glbits, int fogIndex )
{
	mNextVert = 0;

	mTexBundle = bundle;
	//mGLStateBits = glbits;
	if (fogIndex != -1)
	{
		mUseFog = qtrue;
		mFogIndex = fogIndex;
	}
	else
	{
		mUseFog = qfalse;
	}

	//qglDisable(GL_CULL_FACE);
}


void CQuickSpriteSystem::EndGroup(void)
{
	Flush();

	//qglColor4ub(255,255,255,255);
	//qglEnable(GL_CULL_FACE);
}




void CQuickSpriteSystem::Add(float *pointdata, color4ub_t color, vec2_t fog)
{
	float *curcoord;
	float *curfogtexcoord;
	uint32_t *curcolor;

	if (mNextVert>SHADER_MAX_VERTEXES-4)
	{
		Flush();
	}

	curcoord = mVerts[mNextVert];
	// This is 16*sizeof(float) because, pointdata comes from a float[16]
	memcpy(curcoord, pointdata, 16*sizeof(float));

	// Set up color
	curcolor = &mColors[mNextVert];
	*curcolor++ = *(uint32_t *)color;
	*curcolor++ = *(uint32_t *)color;
	*curcolor++ = *(uint32_t *)color;
	*curcolor++ = *(uint32_t *)color;

	if (fog)
	{
		curfogtexcoord = &mFogTextureCoords[mNextVert][0];
		*curfogtexcoord++ = fog[0];
		*curfogtexcoord++ = fog[1];

		*curfogtexcoord++ = fog[0];
		*curfogtexcoord++ = fog[1];

		*curfogtexcoord++ = fog[0];
		*curfogtexcoord++ = fog[1];

		*curfogtexcoord++ = fog[0];
		*curfogtexcoord++ = fog[1];

		mUseFog=qtrue;
	}
	else
	{
		mUseFog=qfalse;
	}

	mNextVert+=4;
}
