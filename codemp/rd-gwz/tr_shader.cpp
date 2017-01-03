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
#include "tr_local.h"

// tr_shader.c -- this file deals with the parsing and definition of shaders

static char *s_shaderText;

// the shader is parsed into these global variables, then copied into
// dynamically allocated memory if it is valid.
static	shaderStage_t	stages[MAX_SHADER_STAGES];
static	shader_t		shader;
static	texModInfo_t	texMods[MAX_SHADER_STAGES][TR_MAX_TEXMODS];

#define RETAIL_ROCKET_WEDGE_SHADER_HASH (1217042)
#define FILE_HASH_SIZE		1024
static	shader_t*		hashTable[FILE_HASH_SIZE];

#define MAX_SHADERTEXT_HASH		2048
static char **shaderTextHashTable[MAX_SHADERTEXT_HASH];

const int lightmapsNone[MAXLIGHTMAPS] =
{
	LIGHTMAP_NONE,
	LIGHTMAP_NONE,
	LIGHTMAP_NONE,
	LIGHTMAP_NONE
};

const int lightmaps2d[MAXLIGHTMAPS] =
{
	LIGHTMAP_2D,
	LIGHTMAP_2D,
	LIGHTMAP_2D,
	LIGHTMAP_2D
};

const int lightmapsVertex[MAXLIGHTMAPS] =
{
	LIGHTMAP_BY_VERTEX,
	LIGHTMAP_BY_VERTEX,
	LIGHTMAP_BY_VERTEX,
	LIGHTMAP_BY_VERTEX
};

const int lightmapsFullBright[MAXLIGHTMAPS] =
{
	LIGHTMAP_WHITEIMAGE,
	LIGHTMAP_WHITEIMAGE,
	LIGHTMAP_WHITEIMAGE,
	LIGHTMAP_WHITEIMAGE
};

const byte stylesDefault[MAXLIGHTMAPS] =
{
	LS_NORMAL,
	LS_LSNONE,
	LS_LSNONE,
	LS_LSNONE
};

/*
=================
SkipBracedSection

The next token should be an open brace.
Skips until a matching close brace is found.
Internal brace depths are properly skipped.
=================
*/
void SkipBracedSection_NoDepth(const char **program) {
	char			*token;
	int				depth;

	depth = 0;
	do {
		token = COM_ParseExt(program, qtrue);
		if (token[1] == 0) {
			if (token[0] == '{') {
				depth++;
			}
			else if (token[0] == '}') {
				depth--;
			}
		}
	} while (depth && *program);
}

/*
=================
SkipBracedSection

The next token should be an open brace or set depth to 1 if already parsed it.
Skips until a matching close brace is found.
Internal brace depths are properly skipped.
=================
*/
qboolean SkipBracedSection_Depth(const char **program, int depth) {
	char			*token;

	do {
		token = COM_ParseExt(program, qtrue);
		if (token[1] == 0) {
			if (token[0] == '{') {
				depth++;
			}
			else if (token[0] == '}') {
				depth--;
			}
		}
	} while (depth && *program);

	return (qboolean)(depth == 0);
}

qhandle_t RE_RegisterShaderLightMap( const char *name, const int *lightmapIndexes, const byte *styles );

void KillTheShaderHashTable(void)
{
	memset(shaderTextHashTable, 0, sizeof(shaderTextHashTable));
}

qboolean ShaderHashTableExists(void)
{
	if (shaderTextHashTable[0])
	{
		return qtrue;
	}
	return qfalse;
}

static void ClearGlobalShader(void)
{
	int	i;

	memset( &shader, 0, sizeof( shader ) );
	memset( &stages, 0, sizeof( stages ) );
	for ( i = 0 ; i < MAX_SHADER_STAGES ; i++ ) {
		stages[i].bundle[0].texMods = texMods[i];
		//stages[i].mGLFogColorOverride = GLFOGOVERRIDE_NONE;

		// default normal/specular
		VectorSet4(stages[i].normalScale, 0.0f, 0.0f, 0.0f, 0.0f);
		//VectorSet4(stages[i].subsurfaceExtinctionCoefficient, 0.0f, 0.0f, 0.0f, 0.0f);
		VectorSet4(stages[i].specularScale, 0.0f, 0.0f, 0.0f, 0.0f); // UQ1: if not set, will fall back to material type defaults instead of cvars...
	}

	shader.contentFlags = CONTENTS_SOLID | CONTENTS_OPAQUE;
}

static uint32_t generateHashValueForText(const char *string, size_t length)
{
	int i = 0;
	uint32_t hash = 0;

	while (length--)
	{
		hash += string[i] * (i + 119);
		i++;
	}

	return (hash ^ (hash >> 10) ^ (hash >> 20));
}



/*
================
return a hash value for the filename
================
*/
#ifdef __GNUCC__
  #warning TODO: check if long is ok here
#endif
static long generateHashValue( const char *fname, const int size ) {
	int		i;
	long	hash;
	char	letter;

	hash = 0;
	i = 0;
	while (fname[i] != '\0') {
		letter = tolower(fname[i]);
		if (letter =='.') break;				// don't include extension
		if (letter =='\\') letter = '/';		// damn path names
		if (letter == PATH_SEP) letter = '/';		// damn path names
		hash+=(long)(letter)*(i+119);
		i++;
	}
	hash = (hash ^ (hash >> 10) ^ (hash >> 20));
	hash &= (size-1);
	return hash;
}

void R_RemapShader(const char *shaderName, const char *newShaderName, const char *timeOffset) {
	char		strippedName[MAX_QPATH];
	int			hash;
	shader_t	*sh, *sh2;
	qhandle_t	h;

	sh = R_FindShaderByName( shaderName );
	if (sh == NULL || sh == tr.defaultShader) {
		h = RE_RegisterShaderLightMap (shaderName, lightmapsNone, stylesDefault);
		sh = R_GetShaderByHandle(h);
	}
	if (sh == NULL || sh == tr.defaultShader) {
		ri->Printf( PRINT_WARNING, "WARNING: R_RemapShader: shader %s not found\n", shaderName );
		return;
	}

	sh2 = R_FindShaderByName( newShaderName );
	if (sh2 == NULL || sh2 == tr.defaultShader) {
		h = RE_RegisterShaderLightMap (newShaderName, lightmapsNone, stylesDefault);
		sh2 = R_GetShaderByHandle(h);
	}

	if (sh2 == NULL || sh2 == tr.defaultShader) {
		ri->Printf( PRINT_WARNING, "WARNING: R_RemapShader: new shader %s not found\n", newShaderName );
		return;
	}

	// remap all the shaders with the given name
	// even tho they might have different lightmaps
	COM_StripExtension(shaderName, strippedName, sizeof(strippedName));
	hash = generateHashValue(strippedName, FILE_HASH_SIZE);
	for (sh = hashTable[hash]; sh; sh = sh->next) {
		if (Q_stricmp(sh->name, strippedName) == 0) {
			if (sh != sh2) {
				sh->remappedShader = sh2;
			} else {
				sh->remappedShader = NULL;
			}
		}
	}
	if (timeOffset) {
		sh2->timeOffset = atof(timeOffset);
	}
}

/*
===============
ParseVector
===============
*/
static qboolean ParseVector( const char **text, int count, float *v ) {
	char	*token;
	int		i;

	// FIXME: spaces are currently required after parens, should change parseext...
	token = COM_ParseExt( text, qfalse );
	if ( strcmp( token, "(" ) ) {
		ri->Printf( PRINT_WARNING, "WARNING: missing parenthesis in shader '%s'\n", shader.name );
		return qfalse;
	}

	for ( i = 0 ; i < count ; i++ ) {
		token = COM_ParseExt( text, qfalse );
		if ( !token[0] ) {
			ri->Printf( PRINT_WARNING, "WARNING: missing vector element in shader '%s'\n", shader.name );
			return qfalse;
		}
		v[i] = atof( token );
	}

	token = COM_ParseExt( text, qfalse );
	if ( strcmp( token, ")" ) ) {
		ri->Printf( PRINT_WARNING, "WARNING: missing parenthesis in shader '%s'\n", shader.name );
		return qfalse;
	}

	return qtrue;
}


/*
===============
NameToAFunc
===============
*/
static unsigned NameToAFunc( const char *funcname )
{
	if ( !Q_stricmp( funcname, "GT0" ) )
	{
		return GLS_ATEST_GT_0;
	}
	else if ( !Q_stricmp( funcname, "LT128" ) )
	{
		return GLS_ATEST_LT_128;
	}
	else if ( !Q_stricmp( funcname, "GE128" ) )
	{
		return GLS_ATEST_GE_128;
	}
	else if ( !Q_stricmp( funcname, "GE192" ) )
	{
		return GLS_ATEST_GE_192;
	}

	ri->Printf( PRINT_WARNING, "WARNING: invalid alphaFunc name '%s' in shader '%s'\n", funcname, shader.name );
	return 0;
}


/*
===============
NameToSrcBlendMode
===============
*/
static int NameToSrcBlendMode( const char *name )
{
	if ( !Q_stricmp( name, "GL_ONE" ) )
	{
		return GLS_SRCBLEND_ONE;
	}
	else if ( !Q_stricmp( name, "GL_ZERO" ) )
	{
		return GLS_SRCBLEND_ZERO;
	}
	else if ( !Q_stricmp( name, "GL_DST_COLOR" ) )
	{
		return GLS_SRCBLEND_DST_COLOR;
	}
	else if ( !Q_stricmp( name, "GL_ONE_MINUS_DST_COLOR" ) )
	{
		return GLS_SRCBLEND_ONE_MINUS_DST_COLOR;
	}
	else if ( !Q_stricmp( name, "GL_SRC_ALPHA" ) )
	{
		return GLS_SRCBLEND_SRC_ALPHA;
	}
	else if ( !Q_stricmp( name, "GL_ONE_MINUS_SRC_ALPHA" ) )
	{
		return GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA;
	}
	else if ( !Q_stricmp( name, "GL_DST_ALPHA" ) )
	{
		if ( r_ignoreDstAlpha->integer )
		{
			return GLS_DSTBLEND_ONE;
		}

		return GLS_SRCBLEND_DST_ALPHA;
	}
	else if ( !Q_stricmp( name, "GL_ONE_MINUS_DST_ALPHA" ) )
	{
		if ( r_ignoreDstAlpha->integer )
		{
			return GLS_DSTBLEND_ZERO;
		}

		return GLS_SRCBLEND_ONE_MINUS_DST_ALPHA;
	}
	else if ( !Q_stricmp( name, "GL_SRC_ALPHA_SATURATE" ) )
	{
		return GLS_SRCBLEND_ALPHA_SATURATE;
	}

	ri->Printf( PRINT_WARNING, "WARNING: unknown blend mode '%s' in shader '%s', substituting GL_ONE\n", name, shader.name );
	return GLS_SRCBLEND_ONE;
}

/*
===============
NameToDstBlendMode
===============
*/
static int NameToDstBlendMode( const char *name )
{
	if ( !Q_stricmp( name, "GL_ONE" ) )
	{
		return GLS_DSTBLEND_ONE;
	}
	else if ( !Q_stricmp( name, "GL_ZERO" ) )
	{
		return GLS_DSTBLEND_ZERO;
	}
	else if ( !Q_stricmp( name, "GL_SRC_ALPHA" ) )
	{
		return GLS_DSTBLEND_SRC_ALPHA;
	}
	else if ( !Q_stricmp( name, "GL_ONE_MINUS_SRC_ALPHA" ) )
	{
		return GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
	}
	else if ( !Q_stricmp( name, "GL_DST_ALPHA" ) )
	{
		if ( r_ignoreDstAlpha->integer )
		{
			return GLS_DSTBLEND_ONE;
		}

		return GLS_DSTBLEND_DST_ALPHA;
	}
	else if ( !Q_stricmp( name, "GL_ONE_MINUS_DST_ALPHA" ) )
	{
		if ( r_ignoreDstAlpha->integer )
		{
			return GLS_DSTBLEND_ZERO;
		}

		return GLS_DSTBLEND_ONE_MINUS_DST_ALPHA;
	}
	else if ( !Q_stricmp( name, "GL_SRC_COLOR" ) )
	{
		return GLS_DSTBLEND_SRC_COLOR;
	}
	else if ( !Q_stricmp( name, "GL_ONE_MINUS_SRC_COLOR" ) )
	{
		return GLS_DSTBLEND_ONE_MINUS_SRC_COLOR;
	}

	ri->Printf( PRINT_WARNING, "WARNING: unknown blend mode '%s' in shader '%s', substituting GL_ONE\n", name, shader.name );
	return GLS_DSTBLEND_ONE;
}

/*
===============
NameToGenFunc
===============
*/
static genFunc_t NameToGenFunc( const char *funcname )
{
	if ( !Q_stricmp( funcname, "sin" ) )
	{
		return GF_SIN;
	}
	else if ( !Q_stricmp( funcname, "square" ) )
	{
		return GF_SQUARE;
	}
	else if ( !Q_stricmp( funcname, "triangle" ) )
	{
		return GF_TRIANGLE;
	}
	else if ( !Q_stricmp( funcname, "sawtooth" ) )
	{
		return GF_SAWTOOTH;
	}
	else if ( !Q_stricmp( funcname, "inversesawtooth" ) )
	{
		return GF_INVERSE_SAWTOOTH;
	}
	else if ( !Q_stricmp( funcname, "noise" ) )
	{
		return GF_NOISE;
	}
	else if ( !Q_stricmp( funcname, "random" ) )
	{
		return GF_RAND;
	}

	ri->Printf( PRINT_WARNING, "WARNING: invalid genfunc name '%s' in shader '%s'\n", funcname, shader.name );
	return GF_SIN;
}


/*
===================
ParseWaveForm
===================
*/
static void ParseWaveForm( const char **text, waveForm_t *wave )
{
	char *token;

	token = COM_ParseExt( text, qfalse );
	if ( token[0] == 0 )
	{
		ri->Printf( PRINT_WARNING, "WARNING: missing waveform parm in shader '%s'\n", shader.name );
		return;
	}
	wave->func = NameToGenFunc( token );

	// BASE, AMP, PHASE, FREQ
	token = COM_ParseExt( text, qfalse );
	if ( token[0] == 0 )
	{
		ri->Printf( PRINT_WARNING, "WARNING: missing waveform parm in shader '%s'\n", shader.name );
		return;
	}
	wave->base = atof( token );

	token = COM_ParseExt( text, qfalse );
	if ( token[0] == 0 )
	{
		ri->Printf( PRINT_WARNING, "WARNING: missing waveform parm in shader '%s'\n", shader.name );
		return;
	}
	wave->amplitude = atof( token );

	token = COM_ParseExt( text, qfalse );
	if ( token[0] == 0 )
	{
		ri->Printf( PRINT_WARNING, "WARNING: missing waveform parm in shader '%s'\n", shader.name );
		return;
	}
	wave->phase = atof( token );

	token = COM_ParseExt( text, qfalse );
	if ( token[0] == 0 )
	{
		ri->Printf( PRINT_WARNING, "WARNING: missing waveform parm in shader '%s'\n", shader.name );
		return;
	}
	wave->frequency = atof( token );
}


/*
===================
ParseTexMod
===================
*/
static void ParseTexMod( const char *_text, shaderStage_t *stage )
{
	const char *token;
	const char **text = &_text;
	texModInfo_t *tmi;

	if ( stage->bundle[0].numTexMods == TR_MAX_TEXMODS ) {
		ri->Error( ERR_DROP, "ERROR: too many tcMod stages in shader '%s'", shader.name );
		return;
	}

	tmi = &stage->bundle[0].texMods[stage->bundle[0].numTexMods];
	stage->bundle[0].numTexMods++;

	token = COM_ParseExt( text, qfalse );

	//
	// turb
	//
	if ( !Q_stricmp( token, "turb" ) )
	{
		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri->Printf( PRINT_WARNING, "WARNING: missing tcMod turb parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->wave.base = atof( token );
		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri->Printf( PRINT_WARNING, "WARNING: missing tcMod turb in shader '%s'\n", shader.name );
			return;
		}
		tmi->wave.amplitude = atof( token );
		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri->Printf( PRINT_WARNING, "WARNING: missing tcMod turb in shader '%s'\n", shader.name );
			return;
		}
		tmi->wave.phase = atof( token );
		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri->Printf( PRINT_WARNING, "WARNING: missing tcMod turb in shader '%s'\n", shader.name );
			return;
		}
		tmi->wave.frequency = atof( token );

		tmi->type = TMOD_TURBULENT;
	}
	//
	// scale
	//
	else if ( !Q_stricmp( token, "scale" ) )
	{
		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri->Printf( PRINT_WARNING, "WARNING: missing scale parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->scale[0] = atof( token );

		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri->Printf( PRINT_WARNING, "WARNING: missing scale parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->scale[1] = atof( token );
		tmi->type = TMOD_SCALE;
	}
	//
	// scroll
	//
	else if ( !Q_stricmp( token, "scroll" ) )
	{
		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri->Printf( PRINT_WARNING, "WARNING: missing scale scroll parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->scroll[0] = atof( token );
		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri->Printf( PRINT_WARNING, "WARNING: missing scale scroll parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->scroll[1] = atof( token );
		tmi->type = TMOD_SCROLL;
	}
	//
	// stretch
	//
	else if ( !Q_stricmp( token, "stretch" ) )
	{
		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri->Printf( PRINT_WARNING, "WARNING: missing stretch parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->wave.func = NameToGenFunc( token );

		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri->Printf( PRINT_WARNING, "WARNING: missing stretch parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->wave.base = atof( token );

		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri->Printf( PRINT_WARNING, "WARNING: missing stretch parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->wave.amplitude = atof( token );

		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri->Printf( PRINT_WARNING, "WARNING: missing stretch parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->wave.phase = atof( token );

		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri->Printf( PRINT_WARNING, "WARNING: missing stretch parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->wave.frequency = atof( token );

		tmi->type = TMOD_STRETCH;
	}
	//
	// transform
	//
	else if ( !Q_stricmp( token, "transform" ) )
	{
		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri->Printf( PRINT_WARNING, "WARNING: missing transform parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->matrix[0][0] = atof( token );

		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri->Printf( PRINT_WARNING, "WARNING: missing transform parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->matrix[0][1] = atof( token );

		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri->Printf( PRINT_WARNING, "WARNING: missing transform parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->matrix[1][0] = atof( token );

		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri->Printf( PRINT_WARNING, "WARNING: missing transform parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->matrix[1][1] = atof( token );

		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri->Printf( PRINT_WARNING, "WARNING: missing transform parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->translate[0] = atof( token );

		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri->Printf( PRINT_WARNING, "WARNING: missing transform parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->translate[1] = atof( token );

		tmi->type = TMOD_TRANSFORM;
	}
	//
	// rotate
	//
	else if ( !Q_stricmp( token, "rotate" ) )
	{
		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri->Printf( PRINT_WARNING, "WARNING: missing tcMod rotate parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->rotateSpeed = atof( token );
		tmi->type = TMOD_ROTATE;
	}
	//
	// entityTranslate
	//
	else if ( !Q_stricmp( token, "entityTranslate" ) )
	{
		tmi->type = TMOD_ENTITY_TRANSLATE;
	}
	else
	{
		ri->Printf( PRINT_WARNING, "WARNING: unknown tcMod '%s' in shader '%s'\n", token, shader.name );
	}
}

static animMapType_t AnimMapType( const char *token )
{
	if ( !Q_stricmp( token, "clampanimMap" ) ) { return ANIMMAP_CLAMP; }
	else if ( !Q_stricmp( token, "oneshotanimMap" ) ) { return ANIMMAP_ONESHOT; }
	else { return ANIMMAP_NORMAL; }
}

static const char *animMapNames[] = {
	"animMap",
	"clapanimMap",
	"oneshotanimMap"
};

/*
/////===== Part of the VERTIGON system =====/////
===================
ParseSurfaceSprites
===================
*/
// surfaceSprites <type> <width> <height> <density> <fadedist>
//
// NOTE:  This parsing function used to be 12 pages long and very complex.  The new version of surfacesprites
// utilizes optional parameters parsed in ParseSurfaceSpriteOptional.
static void ParseSurfaceSprites( const char *_text, shaderStage_t *stage )
{
	const char *token;
	const char **text = &_text;
	float width, height, density, fadedist;
	int sstype=SURFSPRITE_NONE;

	//
	// spritetype
	//
	token = COM_ParseExt( text, qfalse );

	if (token[0]==0)
	{
		Com_Printf (S_COLOR_YELLOW  "WARNING: missing surfaceSprites params in shader '%s'\n", shader.name );
		return;
	}

	if (!Q_stricmp(token, "vertical"))
	{
		sstype = SURFSPRITE_VERTICAL;
	}
	else if (!Q_stricmp(token, "oriented"))
	{
		sstype = SURFSPRITE_ORIENTED;
	}
	else if (!Q_stricmp(token, "effect"))
	{
		sstype = SURFSPRITE_EFFECT;
	}
	else if (!Q_stricmp(token, "flattened"))
	{
		sstype = SURFSPRITE_FLATTENED;
	}
	else
	{
		Com_Printf (S_COLOR_YELLOW  "WARNING: invalid type in shader '%s'\n", shader.name );
		return;
	}

	//
	// width
	//
	token = COM_ParseExt( text, qfalse );
	if (token[0]==0)
	{
		Com_Printf (S_COLOR_YELLOW  "WARNING: missing surfaceSprites params in shader '%s'\n", shader.name );
		return;
	}
	width=atof(token);
	if (width <= 0)
	{
		Com_Printf (S_COLOR_YELLOW  "WARNING: invalid width in shader '%s'\n", shader.name );
		return;
	}

	//
	// height
	//
	token = COM_ParseExt( text, qfalse );
	if (token[0]==0)
	{
		Com_Printf (S_COLOR_YELLOW  "WARNING: missing surfaceSprites params in shader '%s'\n", shader.name );
		return;
	}
	height=atof(token);
	if (height <= 0)
	{
		Com_Printf (S_COLOR_YELLOW  "WARNING: invalid height in shader '%s'\n", shader.name );
		return;
	}

	//
	// density
	//
	token = COM_ParseExt( text, qfalse );
	if (token[0]==0)
	{
		Com_Printf (S_COLOR_YELLOW  "WARNING: missing surfaceSprites params in shader '%s'\n", shader.name );
		return;
	}
	density=atof(token);
	if (density <= 0)
	{
		Com_Printf (S_COLOR_YELLOW  "WARNING: invalid density in shader '%s'\n", shader.name );
		return;
	}

	//
	// fadedist
	//
	token = COM_ParseExt( text, qfalse );
	if (token[0]==0)
	{
		Com_Printf (S_COLOR_YELLOW  "WARNING: missing surfaceSprites params in shader '%s'\n", shader.name );
		return;
	}
	fadedist=atof(token);
	if (fadedist < 32)
	{
		Com_Printf (S_COLOR_YELLOW  "WARNING: invalid fadedist (%f < 32) in shader '%s'\n", fadedist, shader.name );
		return;
	}

	if (!stage->ss)
	{
		stage->ss = (surfaceSprite_t *)Hunk_Alloc( sizeof( surfaceSprite_t ), h_low );
	}

	// These are all set by the command lines.
	stage->ss->surfaceSpriteType = sstype;
	stage->ss->width = width;
	stage->ss->height = height;
	stage->ss->density = density;
	stage->ss->fadeDist = fadedist;

	// These are defaults that can be overwritten.
	stage->ss->fadeMax = fadedist*1.33;
	stage->ss->fadeScale = 0.0;
	stage->ss->wind = 0.0;
	stage->ss->windIdle = 0.0;
	stage->ss->variance[0] = 0.0;
	stage->ss->variance[1] = 0.0;
	stage->ss->facing = SURFSPRITE_FACING_NORMAL;

	// A vertical parameter that needs a default regardless
	stage->ss->vertSkew = 0.0f;

	// These are effect parameters that need defaults nonetheless.
	stage->ss->fxDuration = 1000;		// 1 second
	stage->ss->fxGrow[0] = 0.0;
	stage->ss->fxGrow[1] = 0.0;
	stage->ss->fxAlphaStart = 1.0;
	stage->ss->fxAlphaEnd = 0.0;
}




/*
/////===== Part of the VERTIGON system =====/////
===========================
ParseSurfaceSpritesOptional
===========================
*/
//
// ssFademax <fademax>
// ssFadescale <fadescale>
// ssVariance <varwidth> <varheight>
// ssHangdown
// ssAnyangle
// ssFaceup
// ssWind <wind>
// ssWindIdle <windidle>
// ssVertSkew <skew>
// ssFXDuration <duration>
// ssFXGrow <growwidth> <growheight>
// ssFXAlphaRange <alphastart> <startend>
// ssFXWeather
//
// Optional parameters that will override the defaults set in the surfacesprites command above.
//
static void ParseSurfaceSpritesOptional( const char *param, const char *_text, shaderStage_t *stage )
{
	const char *token;
	const char **text = &_text;
	float	value;

	if (!stage->ss)
	{
		stage->ss = (surfaceSprite_t *)Hunk_Alloc( sizeof( surfaceSprite_t ), h_low );
	}
	//
	// fademax
	//
	if (!Q_stricmp(param, "ssFademax"))
	{
		token = COM_ParseExt( text, qfalse);
		if (token[0]==0)
		{
			Com_Printf (S_COLOR_YELLOW  "WARNING: missing surfacesprite fademax in shader '%s'\n", shader.name );
			return;
		}
		value = atof(token);
		if (value <= stage->ss->fadeDist)
		{
			Com_Printf (S_COLOR_YELLOW  "WARNING: invalid surfacesprite fademax (%.2f <= fadeDist(%.2f)) in shader '%s'\n", value, stage->ss->fadeDist, shader.name );
			return;
		}
		stage->ss->fadeMax=value;
		return;
	}

	//
	// fadescale
	//
	if (!Q_stricmp(param, "ssFadescale"))
	{
		token = COM_ParseExt( text, qfalse);
		if (token[0]==0)
		{
			Com_Printf (S_COLOR_YELLOW  "WARNING: missing surfacesprite fadescale in shader '%s'\n", shader.name );
			return;
		}
		value = atof(token);
		stage->ss->fadeScale=value;
		return;
	}

	//
	// variance
	//
	if (!Q_stricmp(param, "ssVariance"))
	{
		token = COM_ParseExt( text, qfalse);
		if (token[0]==0)
		{
			Com_Printf (S_COLOR_YELLOW  "WARNING: missing surfacesprite variance width in shader '%s'\n", shader.name );
			return;
		}
		value = atof(token);
		if (value < 0)
		{
			Com_Printf (S_COLOR_YELLOW  "WARNING: invalid surfacesprite variance width in shader '%s'\n", shader.name );
			return;
		}
		stage->ss->variance[0]=value;

		token = COM_ParseExt( text, qfalse);
		if (token[0]==0)
		{
			Com_Printf (S_COLOR_YELLOW  "WARNING: missing surfacesprite variance height in shader '%s'\n", shader.name );
			return;
		}
		value = atof(token);
		if (value < 0)
		{
			Com_Printf (S_COLOR_YELLOW  "WARNING: invalid surfacesprite variance height in shader '%s'\n", shader.name );
			return;
		}
		stage->ss->variance[1]=value;
		return;
	}

	//
	// hangdown
	//
	if (!Q_stricmp(param, "ssHangdown"))
	{
		if (stage->ss->facing != SURFSPRITE_FACING_NORMAL)
		{
			Com_Printf (S_COLOR_YELLOW  "WARNING: Hangdown facing overrides previous facing in shader '%s'\n", shader.name );
			return;
		}
		stage->ss->facing=SURFSPRITE_FACING_DOWN;
		return;
	}

	//
	// anyangle
	//
	if (!Q_stricmp(param, "ssAnyangle"))
	{
		if (stage->ss->facing != SURFSPRITE_FACING_NORMAL)
		{
			Com_Printf (S_COLOR_YELLOW  "WARNING: Anyangle facing overrides previous facing in shader '%s'\n", shader.name );
			return;
		}
		stage->ss->facing=SURFSPRITE_FACING_ANY;
		return;
	}

	//
	// faceup
	//
	if (!Q_stricmp(param, "ssFaceup"))
	{
		if (stage->ss->facing != SURFSPRITE_FACING_NORMAL)
		{
			Com_Printf (S_COLOR_YELLOW  "WARNING: Faceup facing overrides previous facing in shader '%s'\n", shader.name );
			return;
		}
		stage->ss->facing=SURFSPRITE_FACING_UP;
		return;
	}

	//
	// wind
	//
	if (!Q_stricmp(param, "ssWind"))
	{
		token = COM_ParseExt( text, qfalse);
		if (token[0]==0)
		{
			Com_Printf (S_COLOR_YELLOW  "WARNING: missing surfacesprite wind in shader '%s'\n", shader.name );
			return;
		}
		value = atof(token);
		if (value < 0.0)
		{
			Com_Printf (S_COLOR_YELLOW  "WARNING: invalid surfacesprite wind in shader '%s'\n", shader.name );
			return;
		}
		stage->ss->wind=value;
		if (stage->ss->windIdle <= 0)
		{	// Also override the windidle, it usually is the same as wind
			stage->ss->windIdle = value;
		}
		return;
	}

	//
	// windidle
	//
	if (!Q_stricmp(param, "ssWindidle"))
	{
		token = COM_ParseExt( text, qfalse);
		if (token[0]==0)
		{
			Com_Printf (S_COLOR_YELLOW  "WARNING: missing surfacesprite windidle in shader '%s'\n", shader.name );
			return;
		}
		value = atof(token);
		if (value < 0.0)
		{
			Com_Printf (S_COLOR_YELLOW  "WARNING: invalid surfacesprite windidle in shader '%s'\n", shader.name );
			return;
		}
		stage->ss->windIdle=value;
		return;
	}

	//
	// vertskew
	//
	if (!Q_stricmp(param, "ssVertskew"))
	{
		token = COM_ParseExt( text, qfalse);
		if (token[0]==0)
		{
			Com_Printf (S_COLOR_YELLOW  "WARNING: missing surfacesprite vertskew in shader '%s'\n", shader.name );
			return;
		}
		value = atof(token);
		if (value < 0.0)
		{
			Com_Printf (S_COLOR_YELLOW  "WARNING: invalid surfacesprite vertskew in shader '%s'\n", shader.name );
			return;
		}
		stage->ss->vertSkew=value;
		return;
	}

	//
	// fxduration
	//
	if (!Q_stricmp(param, "ssFXDuration"))
	{
		token = COM_ParseExt( text, qfalse);
		if (token[0]==0)
		{
			Com_Printf (S_COLOR_YELLOW  "WARNING: missing surfacesprite duration in shader '%s'\n", shader.name );
			return;
		}
		value = atof(token);
		if (value <= 0)
		{
			Com_Printf (S_COLOR_YELLOW  "WARNING: invalid surfacesprite duration in shader '%s'\n", shader.name );
			return;
		}
		stage->ss->fxDuration=value;
		return;
	}

	//
	// fxgrow
	//
	if (!Q_stricmp(param, "ssFXGrow"))
	{
		token = COM_ParseExt( text, qfalse);
		if (token[0]==0)
		{
			Com_Printf (S_COLOR_YELLOW  "WARNING: missing surfacesprite grow width in shader '%s'\n", shader.name );
			return;
		}
		value = atof(token);
		if (value < 0)
		{
			Com_Printf (S_COLOR_YELLOW  "WARNING: invalid surfacesprite grow width in shader '%s'\n", shader.name );
			return;
		}
		stage->ss->fxGrow[0]=value;

		token = COM_ParseExt( text, qfalse);
		if (token[0]==0)
		{
			Com_Printf (S_COLOR_YELLOW  "WARNING: missing surfacesprite grow height in shader '%s'\n", shader.name );
			return;
		}
		value = atof(token);
		if (value < 0)
		{
			Com_Printf (S_COLOR_YELLOW  "WARNING: invalid surfacesprite grow height in shader '%s'\n", shader.name );
			return;
		}
		stage->ss->fxGrow[1]=value;
		return;
	}

	//
	// fxalpharange
	//
	if (!Q_stricmp(param, "ssFXAlphaRange"))
	{
		token = COM_ParseExt( text, qfalse);
		if (token[0]==0)
		{
			Com_Printf (S_COLOR_YELLOW  "WARNING: missing surfacesprite fxalpha start in shader '%s'\n", shader.name );
			return;
		}
		value = atof(token);
		if (value < 0 || value > 1.0)
		{
			Com_Printf (S_COLOR_YELLOW  "WARNING: invalid surfacesprite fxalpha start in shader '%s'\n", shader.name );
			return;
		}
		stage->ss->fxAlphaStart=value;

		token = COM_ParseExt( text, qfalse);
		if (token[0]==0)
		{
			Com_Printf (S_COLOR_YELLOW  "WARNING: missing surfacesprite fxalpha end in shader '%s'\n", shader.name );
			return;
		}
		value = atof(token);
		if (value < 0 || value > 1.0)
		{
			Com_Printf (S_COLOR_YELLOW  "WARNING: invalid surfacesprite fxalpha end in shader '%s'\n", shader.name );
			return;
		}
		stage->ss->fxAlphaEnd=value;
		return;
	}

	//
	// fxweather
	//
	if (!Q_stricmp(param, "ssFXWeather"))
	{
		if (stage->ss->surfaceSpriteType != SURFSPRITE_EFFECT)
		{
			Com_Printf (S_COLOR_YELLOW  "WARNING: weather applied to non-effect surfacesprite in shader '%s'\n", shader.name );
			return;
		}
		stage->ss->surfaceSpriteType = SURFSPRITE_WEATHERFX;
		return;
	}

	//
	// invalid ss command.
	//
	Com_Printf (S_COLOR_YELLOW  "WARNING: invalid optional surfacesprite param '%s' in shader '%s'\n", param, shader.name );
	return;
}

char *StringContains(char *str1, char *str2, int casesensitive)
{
	int len, i, j;

	len = strlen(str1) - strlen(str2);
	for (i = 0; i <= len; i++, str1++) {
		for (j = 0; str2[j]; j++) {
			if (casesensitive) {
				if (str1[j] != str2[j]) {
					break;
				}
			}
			else {
				if (toupper(str1[j]) != toupper(str2[j])) {
					break;
				}
			}
		}
		if (!str2[j]) {
			return str1;
		}
	}
	return NULL;
} //end of the function StringContains

qboolean ForceGlow ( char *shader )
{
	if (!shader) return qfalse;

	// UQ1: Testing - Force glow to obvious glow components...
	// Note that this is absolutely a complete HACK... But the only other option is to remake every other map ever made for JKA...
	// Worst case, we end up with a little extra glow - oh dear! the horror!!! :)
	if (StringContains(shader, "glw", 0))
	{
		return qtrue;
	}
	else if (StringContains(shader, "glow", 0))
	{
		return qtrue;
	}
	//else if (StringContains(shader, "light", 0))
	//{
	//	return qtrue;
	//}
	else if (StringContains(shader, "pulse", 0) && !StringContains(shader, "/pulsecannon", 0))
	{
		return qtrue;
	}
	else if (StringContains(shader, "pls", 0))
	{
		return qtrue;
	}
	else if (StringContains(shader, "mp/s_flat", 0))
	{
		return qtrue;
	}
	else if (StringContains(shader, "blend", 0))
	{
		return qtrue;
	}
	else if (StringContains(shader, "onoffr", 0))
	{
		return qtrue;
	}
	else if (StringContains(shader, "onoffg", 0))
	{
		return qtrue;
	}
	else if (StringContains(shader, "neon", 0))
	{
		return qtrue;
	}
	else if (StringContains(shader, "flare", 0))
	{
		return qtrue;
	}
	else if (StringContains(shader, "comp_panel", 0)) // doomgiver
	{
		return qtrue;
	}
	else if (StringContains(shader, "d_switch", 0)) // doomgiver
	{
		return qtrue;
	}
	else if (StringContains(shader, "doom_display", 0)) // doomgiver
	{
		return qtrue;
	}
	else if (StringContains(shader, "door1_red", 0)) // doomgiver
	{
		return qtrue;
	}
	else if (StringContains(shader, "mapd", 0)) // doomgiver - may cause issues?
	{
		return qtrue;
	}
	else if (StringContains(shader, "screen0", 0)) // doomgiver - may cause issues?
	{
		return qtrue;
	}
	else if (StringContains(shader, "_energy", 0)) // doomgiver - may cause issues?
	{
		return qtrue;
	}
	else if (StringContains(shader, "_display", 0)) // h_evil - may cause issues?
	{
		return qtrue;
	}
	else if (StringContains(shader, "switch_off", 0)) // for h_evil/switch_off
	{
		return qtrue;
	}
	else if (StringContains(shader, "switch_on", 0)) // for h_evil/switch_on
	{
		return qtrue;
	}
	else if (StringContains(shader, "_screen_", 0)) // for hoth
	{
		return qtrue;
	}
	else if (StringContains(shader, "switch_lift", 0)) // for hoth
	{
		return qtrue;
	}
	else if (StringContains(shader, "lava", 0))
	{
		return qtrue;
	}
	else if (StringContains(shader, "_grn", 0))
	{
		return qtrue;
	}
	else if (StringContains(shader, "_red", 0))
	{
		return qtrue;
	}
	else if (StringContains(shader, "static_field3", 0))
	{
		return qtrue;
	}
	else if (StringContains(shader, "power222", 0))
	{
		return qtrue;
	}
	else if (StringContains(shader, "ggoo", 0))
	{
		return qtrue;
	}
	else if (StringContains(shader, "blink", 0))
	{
		return qtrue;
	}
	else if (StringContains(shader, "_blb", 0))
	{
		return qtrue;
	}
	else if (StringContains(shader, "imp_mine/tanklight", 0))
	{
		return qtrue;
	}
	else if (StringContains(shader, "cache_panel_anim", 0))
	{
		return qtrue;
	}
	else if (StringContains(shader, "switch_on", 0))
	{
		return qtrue;
	}
	else if (StringContains(shader, "switch_off", 0))
	{
		return qtrue;
	}
	else if (StringContains(shader, "switch_open", 0))
	{
		return qtrue;
	}
	else if (StringContains(shader, "switch_locked", 0) && !StringContains(shader, "s_switch_locked", 0))
	{
		return qtrue;
	}
	else if (StringContains(shader, "door_switch_", 0)) // for desert/door_switch_red.png - testing loose check in case of other switch's - may cause bad results
	{
		return qtrue;
	}
	else if (StringContains(shader, "impdetention/doortrim01", 0))
	{
		return qtrue;
	}
	else if (StringContains(shader, "holotarget", 0))
	{
		return qtrue;
	}
	else if (StringContains(shader, "eleswitcha", 0)) // ns_streets
	{
		return qtrue;
	}
	else if (StringContains(shader, "eleswitchb", 0)) // ns_streets
	{
		return qtrue;
	}
	else if (StringContains(shader, "_crystal", 0)) // rift
	{
		return qtrue;
	}
	else if (StringContains(shader, "rocky_ruins/screen", 0)) // rocky_ruins
	{
		return qtrue;
	}
	else if (StringContains(shader, "rooftop/screen", 0)) // rooftop
	{
		return qtrue;
	}
	else if (StringContains(shader, "vjun/screen", 0)) // vjun
	{
		return qtrue;
	}
	else if (StringContains(shader, "white", 0))
	{
		return qtrue;
	}
	else if (StringContains(shader, "symbol", 0) && StringContains(shader, "fx", 0)) // rift
	{
		return qtrue;
	}
	else if (StringContains(shader, "deathconlight", 0))
	{
		return qtrue;
	}
	else if (StringContains(shader, "impgarrison/light_panel_01", 0))
	{
		return qtrue;
	}
	else if (StringContains(shader, "winlite", 0)) // ns_streets
	{
		return qtrue;
	}
	else if (StringContains(shader, "reclame", 0)) // SJC
	{
		return qtrue;
	}
	else if (StringContains(shader, "flash", 0)) // GFX
	{
		return qtrue;
	}
	else if (StringContains(shader, "blob", 0)) // GFX
	{
		return qtrue;
	}
	else if (StringContains(shader, "shot", 0) && !StringContains(shader, "acp_arraygun", 0)) // GFX
	{
		return qtrue;
	}
	else if (StringContains(shader, "fire", 0)) // GFX
	{
		return qtrue;
	}
	else if (StringContains(shader, "flame", 0)) // GFX
	{
		return qtrue;
	}
	else if (StringContains(shader, "gfx/", 0) && StringContains(shader, "cmuzzle", 0)) // GFX
	{
		return qtrue;
	}
	else if (StringContains(shader, "gfx/", 0) && StringContains(shader, "ftail", 0)) // GFX
	{
		return qtrue;
	}
	else if (StringContains(shader, "gfx/", 0) && StringContains(shader, "shot", 0)) // GFX
	{
		return qtrue;
	}
	else if (StringContains(shader, "gfx/", 0) && StringContains(shader, "lightcone", 0)) // GFX - hmm ???
	{
		return qtrue;
	}
	else if (StringContains(shader, "gfx/", 0) && StringContains(shader, "spikeb", 0)) // GFX
	{
		return qtrue;
	}
	else if (StringContains(shader, "gfx/", 0) && StringContains(shader, "stunpass", 0)) // GFX
	{
		return qtrue;
	}
	else if (StringContains(shader, "gfx/", 0) && StringContains(shader, "solidwhite", 0)) // GFX
	{
		return qtrue;
	}
	else if (StringContains(shader, "gfx/", 0) && StringContains(shader, "wookie1", 0)) // GFX
	{
		return qtrue;
	}
	else if (StringContains(shader, "gfx/", 0) && StringContains(shader, "plasma", 0)) // GFX
	{
		return qtrue;
	}
	else if (StringContains(shader, "gfx/", 0) && StringContains(shader, "plume", 0)) // GFX
	{
		return qtrue;
	}
	else if (StringContains(shader, "gfx/", 0) && StringContains(shader, "blueline", 0)) // GFX
	{
		return qtrue;
	}
	else if (StringContains(shader, "gfx/", 0) && StringContains(shader, "redline", 0)) // GFX
	{
		return qtrue;
	}
	else if (StringContains(shader, "gfx/", 0) && StringContains(shader, "embers", 0)) // GFX
	{
		return qtrue;
	}
	else if (StringContains(shader, "gfx/", 0) && StringContains(shader, "burst", 0)) // GFX
	{
		return qtrue;
	}
	else if (StringContains(shader, "gfx/", 0) && StringContains(shader, "blob", 0)) // GFX
	{
		return qtrue;
	}
	else if (StringContains(shader, "gfx/", 0) && StringContains(shader, "burn", 0)) // GFX
	{
		return qtrue;
	}
	else if (StringContains(shader, "gfx/", 0) && StringContains(shader, "caustic", 0)) // GFX
	{
		return qtrue;
	}
	else if (StringContains(shader, "gfx/", 0) && StringContains(shader, "bolt", 0)) // GFX
	{
		return qtrue;
	}
	else if (StringContains(shader, "gfx/", 0) && StringContains(shader, "exp0", 0)) // GFX
	{
		return qtrue;
	}
	else if (StringContains(shader, "gfx/", 0) && StringContains(shader, "lightning", 0)) // GFX
	{
		return qtrue;
	}
	else if (StringContains(shader, "gfx/", 0) && StringContains(shader, "mine", 0)) // GFX
	{
		return qtrue;
	}
	else if (StringContains(shader, "gfx/", 0) && StringContains(shader, "redring", 0)) // GFX
	{
		return qtrue;
	}
	else if (StringContains(shader, "gfx/", 0) && StringContains(shader, "rline", 0)) // GFX
	{
		return qtrue;
	}
	else if (StringContains(shader, "gfx/", 0) && StringContains(shader, "spark", 0)) // GFX
	{
		return qtrue;
	}
	else if (StringContains(shader, "gfx/", 0) && StringContains(shader, "sun", 0)) // GFX
	{
		return qtrue;
	}
	else if (StringContains(shader, "gfx/", 0) && StringContains(shader, "whiteline", 0)) // GFX
	{
		return qtrue;
	}
	else if (StringContains(shader, "gfx/exp/", 0)) // GFX
	{
		return qtrue;
	}

	return qfalse;
}

static void ComputeShaderGlowColors( shaderStage_t *pStage )
{
	colorGen_t rgbGen = pStage->rgbGen;
	alphaGen_t alphaGen = pStage->alphaGen;

	pStage->glowColorFound = qfalse;

	pStage->glowColor[0] =
   	pStage->glowColor[1] =
   	pStage->glowColor[2] =
   	pStage->glowColor[3] = 1.0f;

	if (!pStage->glow) return;

	switch ( rgbGen )
	{
		case CGEN_CONST:
			pStage->glowColor[0] = pStage->constantColor[0] / 255.0f;
			pStage->glowColor[1] = pStage->constantColor[1] / 255.0f;
			pStage->glowColor[2] = pStage->constantColor[2] / 255.0f;
			pStage->glowColor[3] = pStage->constantColor[3] / 255.0f;
			//ri->Printf(PRINT_ALL, "Glow color found for shader %s - %f %f %f %f.\n", shader.name, pStage->glowColor[0], pStage->glowColor[1], pStage->glowColor[2], pStage->glowColor[3]);
			pStage->glowColorFound = qtrue;
			return;
			break;
		case CGEN_LIGHTMAPSTYLE:
			VectorScale4(styleColors[pStage->lightmapStyle], 1.0f / 255.0f, pStage->glowColor);
			//ri->Printf(PRINT_ALL, "Glow color found (style) for shader %s - %f %f %f %f.\n", shader.name, pStage->glowColor[0], pStage->glowColor[1], pStage->glowColor[2], pStage->glowColor[3]);
			pStage->glowColorFound = qtrue;
			return;
			break;
		default:
			break;
	}

	//if (pStage->bundle[TB_DIFFUSEMAP].image && pStage->bundle[TB_DIFFUSEMAP].image[0] == tr.whiteImage)
	{// Testing - Assume white... Might add shader keyword for glow light colors at some point maybe...
		pStage->glowColor[0] = 1.0;
		pStage->glowColor[1] = 1.0;
		pStage->glowColor[2] = 1.0;
		pStage->glowColor[3] = 1.0;
		pStage->glowColorFound = qtrue;
		//ri->Printf(PRINT_ALL, "Glow color set for whiteimage shader %s - %f %f %f %f.\n", shader.name, pStage->glowColor[0], pStage->glowColor[1], pStage->glowColor[2], pStage->glowColor[3]);
		return;
	}
}

/*
===================
ParseStage
===================
*/
static qboolean ParseStage( shaderStage_t *stage, const char **text )
{
	char *token;
	unsigned depthMaskBits = GLS_DEPTHMASK_TRUE, blendSrcBits = 0, blendDstBits = 0, atestBits = 0, depthFuncBits = 0;
	qboolean depthMaskExplicit = qfalse;

	stage->active = qtrue;

	while ( 1 )
	{
		token = COM_ParseExt( text, qtrue );
		if ( !token[0] )
		{
			ri->Printf( PRINT_WARNING, "WARNING: no matching '}' found\n" );
			return qfalse;
		}

		if ( token[0] == '}' )
		{
			break;
		}
		//
		// map <name>
		//
		else if ( !Q_stricmp( token, "map" ) )
		{
			token = COM_ParseExt( text, qfalse );
			if ( !token[0] )
			{
				ri->Printf( PRINT_WARNING, "WARNING: missing parameter for 'map' keyword in shader '%s'\n", shader.name );
				return qfalse;
			}

			if ( !Q_stricmp( token, "$whiteimage" ) )
			{
				stage->bundle[0].image[0] = tr.whiteImage;

				// UQ1: Testing - Force glow to obvious glow components...
				//ri->Printf (PRINT_WARNING, "%s forcably marked as a glow shader.\n", stage->bundle[0].image[0]->imgName);
				stage->glow = qtrue;
				//UQ1: END - Testing - Force glow to obvious glow components...
				continue;
			}
			else if ( !Q_stricmp( token, "$lightmap" ) )
			{
				stage->bundle[0].isLightmap = qtrue;
				if ( shader.lightmapIndex[0] < 0 || shader.lightmapIndex[0] >= tr.numLightmaps ) {
#ifndef FINAL_BUILD
					ri->Printf (PRINT_ALL, S_COLOR_RED "Lightmap requested but none avilable for shader '%s'\n", shader.name);
#endif
					stage->bundle[0].image[0] = tr.whiteImage;
				} else {
					stage->bundle[0].image[0] = tr.lightmaps[shader.lightmapIndex[0]];
				}

				continue;
			}
			else if ( !Q_stricmp( token, "$deluxemap" ) )
			{
				if (!tr.worldDeluxeMapping)
				{
					ri->Printf( PRINT_WARNING, "WARNING: shader '%s' wants a deluxe map in a map compiled without them\n", shader.name );
					return qfalse;
				}

				stage->bundle[0].isLightmap = qtrue;
				if ( shader.lightmapIndex[0] < 0 || shader.lightmapIndex[0] >= tr.numLightmaps ) {
					stage->bundle[0].image[0] = tr.whiteImage;
				} else {
					stage->bundle[0].image[0] = tr.deluxemaps[shader.lightmapIndex[0]];
				}
				continue;
			}
			else
			{
				imgType_t type = IMGTYPE_COLORALPHA;
				int flags = IMGFLAG_NONE;

				if (!shader.noMipMaps)
					flags |= IMGFLAG_MIPMAP;

				if (!shader.noPicMip)
					flags |= IMGFLAG_PICMIP;

				if (shader.noTC)
					flags |= IMGFLAG_NO_COMPRESSION;

				if (stage->type == ST_NORMALMAP || stage->type == ST_NORMALPARALLAXMAP)
				{
					type = IMGTYPE_NORMAL;
					flags |= IMGFLAG_NOLIGHTSCALE;

					if (stage->type == ST_NORMALPARALLAXMAP)
						type = IMGTYPE_NORMALHEIGHT;
				}
				else if (stage->type == ST_SPECULARMAP)
				{
					type = IMGTYPE_SPECULAR;
					flags |= IMGFLAG_NOLIGHTSCALE;
				}
				/*else if (stage->type == ST_SUBSURFACEMAP)
				{
					type = IMGTYPE_SUBSURFACE;
					flags |= IMGFLAG_NOLIGHTSCALE;
				}*/
				else if (stage->type == ST_OVERLAYMAP)
				{
					type = IMGTYPE_OVERLAY;
					//flags |= IMGFLAG_NOLIGHTSCALE;
				}
				else if (stage->type == ST_STEEPMAP)
				{
					type = IMGTYPE_STEEPMAP;
					//flags |= IMGFLAG_NOLIGHTSCALE;
				}
				else if (stage->type == ST_STEEPMAP2)
				{
					type = IMGTYPE_STEEPMAP2;
					//flags |= IMGFLAG_NOLIGHTSCALE;
				}
				else
				{
					//if (r_genNormalMaps->integer)
						flags |= IMGFLAG_GENNORMALMAP;

					if (r_srgb->integer)
						flags |= IMGFLAG_SRGB;
				}

				if (ForceGlow(token) || stage->glow)
				{
					flags |= IMGFLAG_GLOW;
				}

				stage->bundle[0].image[0] = R_FindImageFile( token, type, flags );

				if ((flags & IMGFLAG_GLOW) || stage->bundle[0].image[0] == tr.whiteImage)
				{
					stage->glow = qtrue;
					flags |= IMGFLAG_GLOW;
				}

				if ( !stage->bundle[0].image[0] )
				{
					ri->Printf( PRINT_WARNING, "WARNING: R_FindImageFile could not find '%s' in shader '%s'\n", token, shader.name );
					return qfalse;
				}
			}
		}
		//
		// clampmap <name>
		//
		else if ( !Q_stricmp( token, "clampmap" ) )
		{
			imgType_t type = IMGTYPE_COLORALPHA;
			int flags = IMGFLAG_CLAMPTOEDGE;

			token = COM_ParseExt( text, qfalse );
			if ( !token[0] )
			{
				ri->Printf( PRINT_WARNING, "WARNING: missing parameter for 'clampmap' keyword in shader '%s'\n", shader.name );
				return qfalse;
			}

			if (!shader.noMipMaps)
				flags |= IMGFLAG_MIPMAP;

			if (!shader.noPicMip)
				flags |= IMGFLAG_PICMIP;

			if (shader.noTC)
				flags |= IMGFLAG_NO_COMPRESSION;

			if (stage->type == ST_NORMALMAP || stage->type == ST_NORMALPARALLAXMAP)
			{
				type = IMGTYPE_NORMAL;
				flags |= IMGFLAG_NOLIGHTSCALE;

				if (stage->type == ST_NORMALPARALLAXMAP)
					type = IMGTYPE_NORMALHEIGHT;
			}
			else
			{
				//if (r_genNormalMaps->integer)
					flags |= IMGFLAG_GENNORMALMAP;

				if (r_srgb->integer)
					flags |= IMGFLAG_SRGB;
			}

			if (ForceGlow(token) || stage->glow)
			{
				flags |= IMGFLAG_GLOW;
			}

			stage->bundle[0].image[0] = R_FindImageFile( token, type, flags );

			if ((flags & IMGFLAG_GLOW) || stage->bundle[0].image[0] == tr.whiteImage)
			{
				stage->glow = qtrue;
				flags |= IMGFLAG_GLOW;
			}

			if ( !stage->bundle[0].image[0] )
			{
				ri->Printf( PRINT_WARNING, "WARNING: R_FindImageFile could not find '%s' in shader '%s'\n", token, shader.name );
				return qfalse;
			}
		}
		//
		// animMap <frequency> <image1> .... <imageN>
		//
		else if ( !Q_stricmp( token, "animMap" ) || !Q_stricmp( token, "clampanimMap" ) || !Q_stricmp( token, "oneshotanimMap" ) )
		{
			animMapType_t type = AnimMapType( token );
			token = COM_ParseExt( text, qfalse );
			if ( !token[0] )
			{
				ri->Printf( PRINT_WARNING, "WARNING: missing parameter for '%s' keyword in shader '%s'\n", animMapNames[type], shader.name );
				return qfalse;
			}
			stage->bundle[0].imageAnimationSpeed = atof( token );
			stage->bundle[0].oneShotAnimMap = (qboolean)(type == ANIMMAP_ONESHOT);

			// parse up to MAX_IMAGE_ANIMATIONS animations
			while ( 1 ) {
				int		num;

				token = COM_ParseExt( text, qfalse );
				if ( !token[0] ) {
					break;
				}
				num = stage->bundle[0].numImageAnimations;
				if ( num < MAX_IMAGE_ANIMATIONS ) {
					int flags = type == ANIMMAP_CLAMP ? IMGFLAG_CLAMPTOEDGE : IMGFLAG_NONE;

					if (!shader.noMipMaps)
						flags |= IMGFLAG_MIPMAP;

					if (!shader.noPicMip)
						flags |= IMGFLAG_PICMIP;

					if (r_srgb->integer)
						flags |= IMGFLAG_SRGB;

					if (shader.noTC)
						flags |= IMGFLAG_NO_COMPRESSION;

					stage->bundle[0].image[num] = R_FindImageFile( token, IMGTYPE_COLORALPHA, flags );

					// UQ1: Testing - Force glow to obvious glow components...
					if (ForceGlow(stage->bundle[0].image[num]->imgName))
					{
						//ri->Printf (PRINT_WARNING, "%s forcably marked as a glow shader.\n", stage->bundle[0].image[num]->imgName);
						stage->glow = qtrue;
					}
					//UQ1: END - Testing - Force glow to obvious glow components...

					if ( !stage->bundle[0].image[num] )
					{
						ri->Printf( PRINT_WARNING, "WARNING: R_FindImageFile could not find '%s' in shader '%s'\n", token, shader.name );
						return qfalse;
					}
					stage->bundle[0].numImageAnimations++;
				}
			}
		}
		else if ( !Q_stricmp( token, "videoMap" ) )
		{
			token = COM_ParseExt( text, qfalse );
			if ( !token[0] )
			{
				ri->Printf( PRINT_WARNING, "WARNING: missing parameter for 'videoMap' keyword in shader '%s'\n", shader.name );
				return qfalse;
			}
			stage->bundle[0].videoMapHandle = ri->CIN_PlayCinematic( token, 0, 0, 256, 256, (CIN_loop | CIN_silent | CIN_shader));
			if (stage->bundle[0].videoMapHandle != -1) {
				stage->bundle[0].isVideoMap = qtrue;
				stage->bundle[0].image[0] = tr.scratchImage[stage->bundle[0].videoMapHandle];
			}
		}
		//
		// alphafunc <func>
		//
		else if ( !Q_stricmp( token, "alphaFunc" ) )
		{
			token = COM_ParseExt( text, qfalse );
			if ( !token[0] )
			{
				ri->Printf( PRINT_WARNING, "WARNING: missing parameter for 'alphaFunc' keyword in shader '%s'\n", shader.name );
				return qfalse;
			}

			atestBits = NameToAFunc( token );
			shader.hasAlpha = qtrue;
		}
		//
		// depthFunc <func>
		//
		else if ( !Q_stricmp( token, "depthfunc" ) )
		{
			token = COM_ParseExt( text, qfalse );

			if ( !token[0] )
			{
				ri->Printf( PRINT_WARNING, "WARNING: missing parameter for 'depthfunc' keyword in shader '%s'\n", shader.name );
				return qfalse;
			}

			if ( !Q_stricmp( token, "lequal" ) )
			{
				depthFuncBits = 0;
			}
			else if ( !Q_stricmp( token, "equal" ) )
			{
				depthFuncBits = GLS_DEPTHFUNC_EQUAL;
			}
			else if ( !Q_stricmp( token, "disable" ) )
			{
				depthFuncBits = GLS_DEPTHTEST_DISABLE;
			}
			else
			{
				ri->Printf( PRINT_WARNING, "WARNING: unknown depthfunc '%s' in shader '%s'\n", token, shader.name );
				continue;
			}
		}
		//
		// detail
		//
		else if ( !Q_stricmp( token, "detail" ) )
		{
			stage->isDetail = qtrue;
		}
		//
		// blendfunc <srcFactor> <dstFactor>
		// or blendfunc <add|filter|blend>
		//
		else if ( !Q_stricmp( token, "blendfunc" ) )
		{
			token = COM_ParseExt( text, qfalse );
			if ( token[0] == 0 )
			{
				ri->Printf( PRINT_WARNING, "WARNING: missing parm for blendFunc in shader '%s'\n", shader.name );
				continue;
			}
			// check for "simple" blends first
			if ( !Q_stricmp( token, "add" ) ) {
				blendSrcBits = GLS_SRCBLEND_ONE;
				blendDstBits = GLS_DSTBLEND_ONE;
			} else if ( !Q_stricmp( token, "filter" ) ) {
				blendSrcBits = GLS_SRCBLEND_DST_COLOR;
				blendDstBits = GLS_DSTBLEND_ZERO;
			} else if ( !Q_stricmp( token, "blend" ) ) {
				blendSrcBits = GLS_SRCBLEND_SRC_ALPHA;
				blendDstBits = GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
			} else {
				// complex double blends
				blendSrcBits = NameToSrcBlendMode( token );

				token = COM_ParseExt( text, qfalse );
				if ( token[0] == 0 )
				{
					ri->Printf( PRINT_WARNING, "WARNING: missing parm for blendFunc in shader '%s'\n", shader.name );
					continue;
				}
				blendDstBits = NameToDstBlendMode( token );
			}

			// clear depth mask for blended surfaces
			if ( !depthMaskExplicit )
			{
				depthMaskBits = 0;
			}
		}
		//
		// stage <type>
		//
		else if(!Q_stricmp(token, "stage"))
		{
			token = COM_ParseExt(text, qfalse);
			if(token[0] == 0)
			{
				ri->Printf(PRINT_WARNING, "WARNING: missing parameters for stage in shader '%s'\n", shader.name);
				continue;
			}

			if(!Q_stricmp(token, "diffuseMap"))
			{
				stage->type = ST_DIFFUSEMAP;
			}
			else if(!Q_stricmp(token, "normalMap") || !Q_stricmp(token, "bumpMap"))
			{
				stage->type = ST_NORMALMAP;
				VectorSet4(stage->normalScale, r_baseNormalX->value, r_baseNormalY->value, 1.0f, r_baseParallax->value);
			}
			else if(!Q_stricmp(token, "normalParallaxMap") || !Q_stricmp(token, "bumpParallaxMap"))
			{
				if (r_parallaxMapping->integer)
					stage->type = ST_NORMALPARALLAXMAP;
				else
					stage->type = ST_NORMALMAP;
					VectorSet4(stage->normalScale, r_baseNormalX->value, r_baseNormalY->value, 1.0f, r_baseParallax->value);
			}
			else if(!Q_stricmp(token, "specularMap"))
			{
				stage->type = ST_SPECULARMAP;
				//VectorSet4(stage->specularScale, r_baseSpecular->integer, r_baseSpecular->integer, r_baseSpecular->integer, 1.0f);
				VectorSet4(stage->specularScale, 1.0f, 1.0f, 1.0f, 1.0f);
			}
			/*else if(!Q_stricmp(token, "subsurfaceMap"))
			{
				stage->type = ST_SUBSURFACEMAP;
				//VectorSet4(stage->specularScale, r_baseSpecular->integer, r_baseSpecular->integer, r_baseSpecular->integer, 1.0f);
				VectorSet4(stage->subsurfaceExtinctionCoefficient, 0.0f, 0.0f, 0.0f, 0.0f);
			}*/
			else if(!Q_stricmp(token, "overlayMap"))
			{
				stage->type = ST_OVERLAYMAP;
			}
			else if(!Q_stricmp(token, "steepMap"))
			{
				stage->type = ST_STEEPMAP;
			}
			else if(!Q_stricmp(token, "steepMap2"))
			{
				stage->type = ST_STEEPMAP2;
			}
			else
			{
				ri->Printf(PRINT_WARNING, "WARNING: unknown stage parameter '%s' in shader '%s'\n", token, shader.name);
				continue;
			}
		}
		else if (!Q_stricmp(token, "cubeMapScale"))
		{
			token = COM_ParseExt(text, qfalse);
			if ( token[0] == 0 )
			{
				ri->Printf( PRINT_WARNING, "WARNING: missing parameter for cubeMapScale in shader '%s'\n", shader.name );
				continue;
			}
			stage->cubeMapScale = atof( token );
		}
		/*else if (!Q_stricmp(token, "subsurfaceRimScalar"))
		{
			token = COM_ParseExt(text, qfalse);
			if ( token[0] == 0 )
			{
				ri->Printf( PRINT_WARNING, "WARNING: missing parameter for subsurfaceRimScalar in shader '%s'\n", shader.name );
				continue;
			}
			stage->subsurfaceRimScalar = atof( token );
		}
		else if (!Q_stricmp(token, "subsurfaceMaterialThickness"))
		{
			token = COM_ParseExt(text, qfalse);
			if ( token[0] == 0 )
			{
				ri->Printf( PRINT_WARNING, "WARNING: missing parameter for subsurfaceMaterialThickness in shader '%s'\n", shader.name );
				continue;
			}
			stage->subsurfaceMaterialThickness = atof( token );
		}
		else if (!Q_stricmp(token, "subsurfaceSpecularPower"))
		{
			token = COM_ParseExt(text, qfalse);
			if ( token[0] == 0 )
			{
				ri->Printf( PRINT_WARNING, "WARNING: missing parameter for subsurfaceSpecularPower in shader '%s'\n", shader.name );
				continue;
			}
			stage->subsurfaceSpecularPower = atof( token );
		}
		else if (!Q_stricmp(token, "subsurfaceExtinctionCoefficient"))
		{
			token = COM_ParseExt(text, qfalse);
			if ( token[0] == 0 )
			{
				ri->Printf( PRINT_WARNING, "WARNING: missing parameter for subsurfaceExtinctionCoefficient in shader '%s'\n", shader.name );
				continue;
			}

			stage->subsurfaceExtinctionCoefficient[0] = atof( token );

			token = COM_ParseExt(text, qfalse);
			if ( token[0] == 0 )
			{
				ri->Printf( PRINT_WARNING, "WARNING: missing parameter for subsurfaceExtinctionCoefficient in shader '%s'\n", shader.name );
				continue;
			}

			stage->subsurfaceExtinctionCoefficient[1] = atof( token );

			token = COM_ParseExt(text, qfalse);

			if ( token[0] == 0 )
			{
				ri->Printf( PRINT_WARNING, "WARNING: missing parameter for subsurfaceExtinctionCoefficient in shader '%s'\n", shader.name );
				continue;
			}

			stage->subsurfaceExtinctionCoefficient[2] = atof( token );
		}*/
		//
		// specularReflectance <value>
		//
		else if (!Q_stricmp(token, "specularreflectance"))
		{
			token = COM_ParseExt(text, qfalse);
			if ( token[0] == 0 )
			{
				ri->Printf( PRINT_WARNING, "WARNING: missing parameter for specular reflectance in shader '%s'\n", shader.name );
				continue;
			}
			stage->specularScale[0] =
			stage->specularScale[1] =
			stage->specularScale[2] = Com_Clamp( 0.0f, 1.0f, atof( token ) );
		}
		//
		// specularExponent <value>
		//
		else if (!Q_stricmp(token, "specularexponent"))
		{
			float exponent;

			token = COM_ParseExt(text, qfalse);
			if ( token[0] == 0 )
			{
				ri->Printf( PRINT_WARNING, "WARNING: missing parameter for specular exponent in shader '%s'\n", shader.name );
				continue;
			}
			exponent = atof( token );

			// Change shininess to gloss
			// FIXME: assumes max exponent of 8192 and min of 1, must change here if altered in lightall_fp.glsl
			exponent = CLAMP(exponent, 1.0, 8192.0);
			stage->specularScale[3] = log(exponent) / log(8192.0);
		}
		//
		// gloss <value>
		//
		else if ( !Q_stricmp( token, "gloss" ) )
		{
			token = COM_ParseExt(text, qfalse);
			if ( token[0] == 0 )
			{
				ri->Printf( PRINT_WARNING, "WARNING: missing parameter for gloss in shader '%s'\n", shader.name );
				continue;
			}

			stage->specularScale[3] = atof( token );
		}
		//
		// parallaxDepth <value>
		//
		else if (!Q_stricmp(token, "parallaxdepth"))
		{
			token = COM_ParseExt(text, qfalse);
			if ( token[0] == 0 )
			{
				ri->Printf( PRINT_WARNING, "WARNING: missing parameter for parallaxDepth in shader '%s'\n", shader.name );
				continue;
			}

			stage->normalScale[3] = atof( token );
		}
		//
		// normalScale <xy>
		// or normalScale <x> <y>
		// or normalScale <x> <y> <height>
		//
		else if (!Q_stricmp(token, "normalscale"))
		{
			token = COM_ParseExt(text, qfalse);
			if ( token[0] == 0 )
			{
				ri->Printf( PRINT_WARNING, "WARNING: missing parameter for normalScale in shader '%s'\n", shader.name );
				continue;
			}

			stage->normalScale[0] = atof( token );

			token = COM_ParseExt(text, qfalse);
			if ( token[0] == 0 )
			{
				// one value, applies to X/Y
				stage->normalScale[1] = stage->normalScale[0];
				continue;
			}

			stage->normalScale[1] = atof( token );

			token = COM_ParseExt(text, qfalse);
			if ( token[0] == 0 )
			{
				// two values, no height
				continue;
			}

			stage->normalScale[3] = atof( token );
		}
		//
		// specularScale <rgb> <gloss>
		// or specularScale <r> <g> <b>
		// or specularScale <r> <g> <b> <gloss>
		//
		else if (!Q_stricmp(token, "specularscale"))
		{
			token = COM_ParseExt(text, qfalse);
			if ( token[0] == 0 )
			{
				ri->Printf( PRINT_WARNING, "WARNING: missing parameter for specularScale in shader '%s'\n", shader.name );
				continue;
			}

			stage->specularScale[0] = atof( token );

			token = COM_ParseExt(text, qfalse);
			if ( token[0] == 0 )
			{
				ri->Printf( PRINT_WARNING, "WARNING: missing parameter for specularScale in shader '%s'\n", shader.name );
				continue;
			}

			stage->specularScale[1] = atof( token );

			token = COM_ParseExt(text, qfalse);
			if ( token[0] == 0 )
			{
				// two values, rgb then gloss
				stage->specularScale[3] = stage->specularScale[1];
				stage->specularScale[1] =
				stage->specularScale[2] = stage->specularScale[0];
				continue;
			}

			stage->specularScale[2] = Com_Clamp( 0.0f, 1.0f, atof( token ) );

			token = COM_ParseExt(text, qfalse);
			if ( token[0] == 0 )
			{
				// three values, rgb
				continue;
			}

			stage->specularScale[3] = atof( token );
		}
		//
		// rgbGen
		//
		else if ( !Q_stricmp( token, "rgbGen" ) )
		{
			token = COM_ParseExt( text, qfalse );
			if ( token[0] == 0 )
			{
				ri->Printf( PRINT_WARNING, "WARNING: missing parameters for rgbGen in shader '%s'\n", shader.name );
				continue;
			}

			if ( !Q_stricmp( token, "wave" ) )
			{
				ParseWaveForm( text, &stage->rgbWave );
				stage->rgbGen = CGEN_WAVEFORM;
			}
			else if ( !Q_stricmp( token, "const" ) )
			{
				vec3_t	color;

				ParseVector( text, 3, color );
				stage->constantColor[0] = 255 * color[0];
				stage->constantColor[1] = 255 * color[1];
				stage->constantColor[2] = 255 * color[2];

				stage->rgbGen = CGEN_CONST;
			}
			else if ( !Q_stricmp( token, "identity" ) )
			{
				stage->rgbGen = CGEN_IDENTITY;
			}
			else if ( !Q_stricmp( token, "identityLighting" ) )
			{
				stage->rgbGen = CGEN_IDENTITY_LIGHTING;
			}
			else if ( !Q_stricmp( token, "entity" ) )
			{
				stage->rgbGen = CGEN_ENTITY;
			}
			else if ( !Q_stricmp( token, "oneMinusEntity" ) )
			{
				stage->rgbGen = CGEN_ONE_MINUS_ENTITY;
			}
			else if ( !Q_stricmp( token, "vertex" ) )
			{
				stage->rgbGen = CGEN_VERTEX;
				if ( stage->alphaGen == 0 ) {
					stage->alphaGen = AGEN_VERTEX;
				}
			}
			else if ( !Q_stricmp( token, "exactVertex" ) )
			{
				stage->rgbGen = CGEN_EXACT_VERTEX;
			}
			else if ( !Q_stricmp( token, "vertexLit" ) )
			{
				stage->rgbGen = CGEN_VERTEX_LIT;
				if ( stage->alphaGen == 0 ) {
					stage->alphaGen = AGEN_VERTEX;
				}
			}
			else if ( !Q_stricmp( token, "exactVertexLit" ) )
			{
				stage->rgbGen = CGEN_EXACT_VERTEX_LIT;
			}
			else if ( !Q_stricmp( token, "lightingDiffuse" ) )
			{
				stage->rgbGen = CGEN_LIGHTING_DIFFUSE;
			}
			else if ( !Q_stricmp( token, "lightingDiffuseEntity" ) )
			{
				if (shader.lightmapIndex[0] != LIGHTMAP_NONE)
				{
					Com_Printf( S_COLOR_RED "ERROR: rgbGen lightingDiffuseEntity used on a misc_model! in shader '%s'\n", shader.name );
				}
				stage->rgbGen = CGEN_LIGHTING_DIFFUSE_ENTITY;
			}
			else if ( !Q_stricmp( token, "oneMinusVertex" ) )
			{
				stage->rgbGen = CGEN_ONE_MINUS_VERTEX;
			}
			else
			{
				ri->Printf( PRINT_WARNING, "WARNING: unknown rgbGen parameter '%s' in shader '%s'\n", token, shader.name );
				continue;
			}
		}
		//
		// alphaGen
		//
		else if ( !Q_stricmp( token, "alphaGen" ) )
		{
			token = COM_ParseExt( text, qfalse );
			if ( token[0] == 0 )
			{
				ri->Printf( PRINT_WARNING, "WARNING: missing parameters for alphaGen in shader '%s'\n", shader.name );
				continue;
			}

			if ( !Q_stricmp( token, "wave" ) )
			{
				ParseWaveForm( text, &stage->alphaWave );
				stage->alphaGen = AGEN_WAVEFORM;
			}
			else if ( !Q_stricmp( token, "const" ) )
			{
				token = COM_ParseExt( text, qfalse );
				stage->constantColor[3] = 255 * atof( token );
				stage->alphaGen = AGEN_CONST;
			}
			else if ( !Q_stricmp( token, "identity" ) )
			{
				stage->alphaGen = AGEN_IDENTITY;
			}
			else if ( !Q_stricmp( token, "entity" ) )
			{
				stage->alphaGen = AGEN_ENTITY;
			}
			else if ( !Q_stricmp( token, "oneMinusEntity" ) )
			{
				stage->alphaGen = AGEN_ONE_MINUS_ENTITY;
			}
			else if ( !Q_stricmp( token, "vertex" ) )
			{
				stage->alphaGen = AGEN_VERTEX;
			}
			else if ( !Q_stricmp( token, "lightingSpecular" ) )
			{
				stage->alphaGen = AGEN_LIGHTING_SPECULAR;
			}
			else if ( !Q_stricmp( token, "oneMinusVertex" ) )
			{
				stage->alphaGen = AGEN_ONE_MINUS_VERTEX;
			}
			else if ( !Q_stricmp( token, "dot" ) )
			{
				//stage->alphaGen = AGEN_DOT;
			}
			else if ( !Q_stricmp( token, "oneMinusDot" ) )
			{
				//stage->alphaGen = AGEN_ONE_MINUS_DOT;
			}
			else if ( !Q_stricmp( token, "portal" ) )
			{
				stage->alphaGen = AGEN_PORTAL;
				token = COM_ParseExt( text, qfalse );
				if ( token[0] == 0 )
				{
					shader.portalRange = 256;
					ri->Printf( PRINT_WARNING, "WARNING: missing range parameter for alphaGen portal in shader '%s', defaulting to 256\n", shader.name );
				}
				else
				{
					shader.portalRange = atof( token );
				}
			}
			else
			{
				ri->Printf( PRINT_WARNING, "WARNING: unknown alphaGen parameter '%s' in shader '%s'\n", token, shader.name );
				continue;
			}
		}
		//
		// tcGen <function>
		//
		else if ( !Q_stricmp(token, "texgen") || !Q_stricmp( token, "tcGen" ) )
		{
			token = COM_ParseExt( text, qfalse );
			if ( token[0] == 0 )
			{
				ri->Printf( PRINT_WARNING, "WARNING: missing texgen parm in shader '%s'\n", shader.name );
				continue;
			}

			if ( !Q_stricmp( token, "environment" ) )
			{
				stage->bundle[0].tcGen = TCGEN_ENVIRONMENT_MAPPED;
			}
			else if ( !Q_stricmp( token, "lightmap" ) )
			{
				stage->bundle[0].tcGen = TCGEN_LIGHTMAP;
			}
			else if ( !Q_stricmp( token, "texture" ) || !Q_stricmp( token, "base" ) )
			{
				stage->bundle[0].tcGen = TCGEN_TEXTURE;
			}
			else if ( !Q_stricmp( token, "vector" ) )
			{
				ParseVector( text, 3, stage->bundle[0].tcGenVectors[0] );
				ParseVector( text, 3, stage->bundle[0].tcGenVectors[1] );

				stage->bundle[0].tcGen = TCGEN_VECTOR;
			}
			else
			{
				ri->Printf( PRINT_WARNING, "WARNING: unknown texgen parm in shader '%s'\n", shader.name );
			}
		}
		//
		// tcMod <type> <...>
		//
		else if ( !Q_stricmp( token, "tcMod" ) )
		{
			char buffer[1024] = "";

			while ( 1 )
			{
				token = COM_ParseExt( text, qfalse );
				if ( token[0] == 0 )
					break;
				Q_strcat( buffer, sizeof( buffer ), token );
				Q_strcat( buffer, sizeof( buffer ), " " );
			}

			ParseTexMod( buffer, stage );

			continue;
		}
		//
		// depthmask
		//
		else if ( !Q_stricmp( token, "depthwrite" ) )
		{
			depthMaskBits = GLS_DEPTHMASK_TRUE;
			depthMaskExplicit = qtrue;

			continue;
		}
		// If this stage has glow...	GLOWXXX
		else if ( Q_stricmp( token, "glow" ) == 0 )
		{
			stage->glow = qtrue;

			continue;
		}
		//
		// surfaceSprites <type> ...
		//
		else if ( !Q_stricmp( token, "surfaceSprites" ) )
		{
			// Mark this stage as a surface sprite so we can skip it for now
			stage->isSurfaceSprite = qtrue;
#ifndef __SURFACESPRITES__
			SkipRestOfLine( text );
#else //__SURFACESPRITES__
			char buffer[1024] = "";

			while ( 1 )
			{
				token = COM_ParseExt( text, qfalse );
				if ( token[0] == 0 )
					break;
				Q_strcat( buffer, sizeof( buffer ), token );
				Q_strcat( buffer, sizeof( buffer ), " " );
			}

			ParseSurfaceSprites( buffer, stage );
#endif //__SURFACESPRITES__
			continue;
		}
		//
		// ssFademax <fademax>
		// ssFadescale <fadescale>
		// ssVariance <varwidth> <varheight>
		// ssHangdown
		// ssAnyangle
		// ssFaceup
		// ssWind <wind>
		// ssWindIdle <windidle>
		// ssDuration <duration>
		// ssGrow <growwidth> <growheight>
		// ssWeather
		//
		else if (!Q_stricmpn(token, "ss", 2))	// <--- NOTE ONLY COMPARING FIRST TWO LETTERS
		{
			//SkipRestOfLine( text );
			char buffer[1024] = "";
			char param[128];
			strcpy(param,token);

			while ( 1 )
			{
				token = COM_ParseExt( text, qfalse );
				if ( token[0] == 0 )
					break;
				Q_strcat( buffer, sizeof( buffer ), token );
				Q_strcat( buffer, sizeof( buffer ), " " );
			}

			ParseSurfaceSpritesOptional( param, buffer, stage );
			stage->isSurfaceSprite = qtrue;
			continue;
		}
		else
		{
			ri->Printf( PRINT_WARNING, "WARNING: unknown parameter '%s' in shader '%s'\n", token, shader.name );
			return qfalse;
		}
	}

	//
	// if cgen isn't explicitly specified, use either identity or identitylighting
	//
	if ( stage->rgbGen == CGEN_BAD ) {
		if ( blendSrcBits == 0 ||
			blendSrcBits == GLS_SRCBLEND_ONE ||
			blendSrcBits == GLS_SRCBLEND_SRC_ALPHA ) {
			stage->rgbGen = CGEN_IDENTITY_LIGHTING;
		} else {
			stage->rgbGen = CGEN_IDENTITY;
		}
	}


	//
	// implicitly assume that a GL_ONE GL_ZERO blend mask disables blending
	//
	if ( ( blendSrcBits == GLS_SRCBLEND_ONE ) &&
		 ( blendDstBits == GLS_DSTBLEND_ZERO ) )
	{
		blendDstBits = blendSrcBits = 0;
		depthMaskBits = GLS_DEPTHMASK_TRUE;
	}

	// decide which agens we can skip
	if ( stage->alphaGen == AGEN_IDENTITY ) {
		if ( stage->rgbGen == CGEN_IDENTITY
			|| stage->rgbGen == CGEN_LIGHTING_DIFFUSE ) {
			stage->alphaGen = AGEN_SKIP;
		}
	}

	if (stage->glow)
	{
		ComputeShaderGlowColors( stage );
	}

	//
	// compute state bits
	//
	stage->stateBits = depthMaskBits |
		               blendSrcBits | blendDstBits |
					   atestBits |
					   depthFuncBits;

	return qtrue;
}

/*
===============
ParseDeform

deformVertexes wave <spread> <waveform> <base> <amplitude> <phase> <frequency>
deformVertexes normal <frequency> <amplitude>
deformVertexes move <vector> <waveform> <base> <amplitude> <phase> <frequency>
deformVertexes bulge <bulgeWidth> <bulgeHeight> <bulgeSpeed>
deformVertexes projectionShadow
deformVertexes autoSprite
deformVertexes autoSprite2
deformVertexes text[0-7]
===============
*/
static void ParseDeform( const char **text ) {
	char	*token;
	deformStage_t	*ds;

	token = COM_ParseExt( text, qfalse );
	if ( token[0] == 0 )
	{
		ri->Printf( PRINT_WARNING, "WARNING: missing deform parm in shader '%s'\n", shader.name );
		return;
	}

	if ( shader.numDeforms == MAX_SHADER_DEFORMS ) {
		ri->Printf( PRINT_WARNING, "WARNING: MAX_SHADER_DEFORMS in '%s'\n", shader.name );
		return;
	}

	ds = &shader.deforms[ shader.numDeforms ];
	shader.numDeforms++;

	if ( !Q_stricmp( token, "projectionShadow" ) ) {
		ds->deformation = DEFORM_PROJECTION_SHADOW;
		return;
	}

	if ( !Q_stricmp( token, "autosprite" ) ) {
		ds->deformation = DEFORM_AUTOSPRITE;
		return;
	}

	if ( !Q_stricmp( token, "autosprite2" ) ) {
		ds->deformation = DEFORM_AUTOSPRITE2;
		return;
	}

	if ( !Q_stricmpn( token, "text", 4 ) ) {
		int		n;

		n = token[4] - '0';
		if ( n < 0 || n > 7 ) {
			n = 0;
		}
		ds->deformation = (deform_t)(DEFORM_TEXT0 + n);
		return;
	}

	if ( !Q_stricmp( token, "bulge" ) )	{
		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri->Printf( PRINT_WARNING, "WARNING: missing deformVertexes bulge parm in shader '%s'\n", shader.name );
			return;
		}
		ds->bulgeWidth = atof( token );

		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri->Printf( PRINT_WARNING, "WARNING: missing deformVertexes bulge parm in shader '%s'\n", shader.name );
			return;
		}
		ds->bulgeHeight = atof( token );

		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri->Printf( PRINT_WARNING, "WARNING: missing deformVertexes bulge parm in shader '%s'\n", shader.name );
			return;
		}
		ds->bulgeSpeed = atof( token );

		ds->deformation = DEFORM_BULGE;
		return;
	}

	if ( !Q_stricmp( token, "wave" ) )
	{
		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri->Printf( PRINT_WARNING, "WARNING: missing deformVertexes parm in shader '%s'\n", shader.name );
			return;
		}

		if ( atof( token ) != 0 )
		{
			ds->deformationSpread = 1.0f / atof( token );
		}
		else
		{
			ds->deformationSpread = 100.0f;
			ri->Printf( PRINT_WARNING, "WARNING: illegal div value of 0 in deformVertexes command for shader '%s'\n", shader.name );
		}

		ParseWaveForm( text, &ds->deformationWave );
		ds->deformation = DEFORM_WAVE;
		return;
	}

	if ( !Q_stricmp( token, "normal" ) )
	{
		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri->Printf( PRINT_WARNING, "WARNING: missing deformVertexes parm in shader '%s'\n", shader.name );
			return;
		}
		ds->deformationWave.amplitude = atof( token );

		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri->Printf( PRINT_WARNING, "WARNING: missing deformVertexes parm in shader '%s'\n", shader.name );
			return;
		}
		ds->deformationWave.frequency = atof( token );

		ds->deformation = DEFORM_NORMALS;
		return;
	}

	if ( !Q_stricmp( token, "move" ) ) {
		int		i;

		for ( i = 0 ; i < 3 ; i++ ) {
			token = COM_ParseExt( text, qfalse );
			if ( token[0] == 0 ) {
				ri->Printf( PRINT_WARNING, "WARNING: missing deformVertexes parm in shader '%s'\n", shader.name );
				return;
			}
			ds->moveVector[i] = atof( token );
		}

		ParseWaveForm( text, &ds->deformationWave );
		ds->deformation = DEFORM_MOVE;
		return;
	}

	ri->Printf( PRINT_WARNING, "WARNING: unknown deformVertexes subtype '%s' found in shader '%s'\n", token, shader.name );
}


/*
===============
ParseSkyParms

skyParms <outerbox> <cloudheight> <innerbox>
===============
*/
static void ParseSkyParms( const char **text ) {
	char				*token;
	static const char	*suf[6] = {"rt", "lf", "bk", "ft", "up", "dn"};
	char		pathname[MAX_QPATH];
	int			i;
	int imgFlags = IMGFLAG_MIPMAP | IMGFLAG_PICMIP;

	if (shader.noTC)
		imgFlags |= IMGFLAG_NO_COMPRESSION;

	if (r_srgb->integer)
		imgFlags |= IMGFLAG_SRGB;

	// outerbox
	token = COM_ParseExt( text, qfalse );
	if ( token[0] == 0 ) {
		ri->Printf( PRINT_WARNING, "WARNING: 'skyParms' missing parameter in shader '%s'\n", shader.name );
		return;
	}
	if ( strcmp( token, "-" ) ) {
		for (i=0 ; i<6 ; i++) {
			Com_sprintf( pathname, sizeof(pathname), "%s_%s", token, suf[i] );
			shader.sky.outerbox[i] = R_FindImageFile( ( char * ) pathname, IMGTYPE_COLORALPHA, imgFlags | IMGFLAG_CLAMPTOEDGE );

			if ( !shader.sky.outerbox[i] ) {
				if ( i )
					shader.sky.outerbox[i] = shader.sky.outerbox[i-1];	//not found, so let's use the previous image
				else
					shader.sky.outerbox[i] = tr.defaultImage;
			}
		}
	}

	// cloudheight
	token = COM_ParseExt( text, qfalse );
	if ( token[0] == 0 ) {
		ri->Printf( PRINT_WARNING, "WARNING: 'skyParms' missing cloudheight in shader '%s'\n", shader.name );
		return;
	}
	shader.sky.cloudHeight = atof( token );
	if ( !shader.sky.cloudHeight ) {
		shader.sky.cloudHeight = 512;
	}
	R_InitSkyTexCoords( shader.sky.cloudHeight );

	// innerbox
	token = COM_ParseExt( text, qfalse );
	if ( strcmp( token, "-" ) ) {
		ri->Printf( PRINT_WARNING, "WARNING: in shader '%s' 'skyParms', innerbox is not supported!", shader.name );
	}

	shader.isSky = qtrue;
}


/*
=================
ParseSort
=================
*/
void ParseSort( const char **text ) {
	char	*token;

	token = COM_ParseExt( text, qfalse );
	if ( token[0] == 0 ) {
		ri->Printf( PRINT_WARNING, "WARNING: missing sort parameter in shader '%s'\n", shader.name );
		return;
	}

	if ( !Q_stricmp( token, "portal" ) ) {
		shader.sort = SS_PORTAL;
	} else if ( !Q_stricmp( token, "sky" ) ) {
		shader.sort = SS_ENVIRONMENT;
	} else if ( !Q_stricmp( token, "opaque" ) ) {
		shader.sort = SS_OPAQUE;
	} else if ( !Q_stricmp( token, "decal" ) ) {
		shader.sort = SS_DECAL;
	} else if ( !Q_stricmp( token, "seeThrough" ) ) {
		shader.sort = SS_SEE_THROUGH;
	} else if ( !Q_stricmp( token, "banner" ) ) {
		shader.sort = SS_BANNER;
	} else if ( !Q_stricmp( token, "additive" ) ) {
		shader.sort = SS_BLEND1;
	} else if ( !Q_stricmp( token, "nearest" ) ) {
		shader.sort = SS_NEAREST;
	} else if ( !Q_stricmp( token, "underwater" ) ) {
		shader.sort = SS_UNDERWATER;
	} else if ( !Q_stricmp( token, "inside" ) ) {
		shader.sort = SS_INSIDE;
	} else if ( !Q_stricmp( token, "mid_inside" ) ) {
		shader.sort = SS_MID_INSIDE;
	} else if ( !Q_stricmp( token, "middle" ) ) {
		shader.sort = SS_MIDDLE;
	} else if ( !Q_stricmp( token, "mid_outside" ) ) {
		shader.sort = SS_MID_OUTSIDE;
	} else if ( !Q_stricmp( token, "outside" ) ) {
		shader.sort = SS_OUTSIDE;
	}
	else {
		shader.sort = atof( token );
	}
}

/*
=================
ParseMaterial
=================
*/
const char *materialNames[MATERIAL_LAST] =
{
	MATERIALS
};

void ParseMaterial( const char **text )
{
	char	*token;
	int		i;

	token = COM_ParseExt( text, qfalse );
	if ( token[0] == 0 )
	{
		Com_Printf (S_COLOR_YELLOW  "WARNING: missing material in shader '%s'\n", shader.name );
		return;
	}
	for(i = 0; i < MATERIAL_LAST; i++)
	{
		if ( !Q_stricmp( token, materialNames[i] ) )
		{
			shader.surfaceFlags |= i;
			break;
		}
	}
}


// this table is also present in q3map

typedef struct infoParm_s {
	const char	*name;
	uint32_t	clearSolid, surfaceFlags, contents;
} infoParm_t;

infoParm_t	infoParms[] = {
	// Game content Flags
	{ "nonsolid",		~CONTENTS_SOLID,					SURF_NONE,			CONTENTS_NONE },		// special hack to clear solid flag
	{ "nonopaque",		~CONTENTS_OPAQUE,					SURF_NONE,			CONTENTS_NONE },		// special hack to clear opaque flag
	{ "lava",			~CONTENTS_SOLID,					SURF_NONE,			CONTENTS_LAVA },		// very damaging
	{ "slime",			~CONTENTS_SOLID,					SURF_NONE,			CONTENTS_SLIME },		// mildly damaging
	{ "water",			~CONTENTS_SOLID,					SURF_NONE,			CONTENTS_WATER },		//
	{ "fog",			~CONTENTS_SOLID,					SURF_NONE,			CONTENTS_FOG},			// carves surfaces entering
	{ "shotclip",		~CONTENTS_SOLID,					SURF_NONE,			CONTENTS_SHOTCLIP },	// block shots, but not people
	{ "playerclip",		~(CONTENTS_SOLID|CONTENTS_OPAQUE),	SURF_NONE,			CONTENTS_PLAYERCLIP },	// block only the player
	{ "monsterclip",	~(CONTENTS_SOLID|CONTENTS_OPAQUE),	SURF_NONE,			CONTENTS_MONSTERCLIP },	//
	{ "botclip",		~(CONTENTS_SOLID|CONTENTS_OPAQUE),	SURF_NONE,			CONTENTS_BOTCLIP },		// for bots
	{ "trigger",		~(CONTENTS_SOLID|CONTENTS_OPAQUE),	SURF_NONE,			CONTENTS_TRIGGER },		//
	{ "nodrop",			~(CONTENTS_SOLID|CONTENTS_OPAQUE),	SURF_NONE,			CONTENTS_NODROP },		// don't drop items or leave bodies (death fog, lava, etc)
	{ "terrain",		~(CONTENTS_SOLID|CONTENTS_OPAQUE),	SURF_NONE,			CONTENTS_TERRAIN },		// use special terrain collsion
	{ "ladder",			~(CONTENTS_SOLID|CONTENTS_OPAQUE),	SURF_NONE,			CONTENTS_LADDER },		// climb up in it like water
	{ "abseil",			~(CONTENTS_SOLID|CONTENTS_OPAQUE),	SURF_NONE,			CONTENTS_ABSEIL },		// can abseil down this brush
	{ "outside",		~(CONTENTS_SOLID|CONTENTS_OPAQUE),	SURF_NONE,			CONTENTS_OUTSIDE },		// volume is considered to be in the outside (i.e. not indoors)
	{ "inside",			~(CONTENTS_SOLID|CONTENTS_OPAQUE),	SURF_NONE,			CONTENTS_INSIDE },		// volume is considered to be inside (i.e. indoors)

	{ "detail",			CONTENTS_ALL,						SURF_NONE,			CONTENTS_DETAIL },		// don't include in structural bsp
	{ "trans",			CONTENTS_ALL,						SURF_NONE,			CONTENTS_TRANSLUCENT },	// surface has an alpha component

	/* Game surface flags */
	{ "sky",			CONTENTS_ALL,						SURF_SKY,			CONTENTS_NONE },		// emit light from an environment map
	{ "slick",			CONTENTS_ALL,						SURF_SLICK,			CONTENTS_NONE },		//

	{ "nodamage",		CONTENTS_ALL,						SURF_NODAMAGE,		CONTENTS_NONE },		//
	{ "noimpact",		CONTENTS_ALL,						SURF_NOIMPACT,		CONTENTS_NONE },		// don't make impact explosions or marks
	{ "nomarks",		CONTENTS_ALL,						SURF_NOMARKS,		CONTENTS_NONE },		// don't make impact marks, but still explode
	{ "nodraw",			CONTENTS_ALL,						SURF_NODRAW,		CONTENTS_NONE },		// don't generate a drawsurface (or a lightmap)
	{ "nosteps",		CONTENTS_ALL,						SURF_NOSTEPS,		CONTENTS_NONE },		//
	{ "nodlight",		CONTENTS_ALL,						SURF_NODLIGHT,		CONTENTS_NONE },		// don't ever add dynamic lights
	{ "metalsteps",		CONTENTS_ALL,						SURF_METALSTEPS,	CONTENTS_NONE },		//
	{ "nomiscents",		CONTENTS_ALL,						SURF_NOMISCENTS,	CONTENTS_NONE },		// No misc ents on this surface
	{ "forcefield",		CONTENTS_ALL,						SURF_FORCEFIELD,	CONTENTS_NONE },		//
	{ "forcesight",		CONTENTS_ALL,						SURF_FORCESIGHT,	CONTENTS_NONE },		// only visible with force sight
};


/*
===============
ParseSurfaceParm

surfaceparm <name>
===============
*/
static void ParseSurfaceParm( const char **text ) {
	char	*token;
	int		numInfoParms = ARRAY_LEN( infoParms );
	int		i;

	token = COM_ParseExt( text, qfalse );
	for ( i = 0 ; i < numInfoParms ; i++ ) {
		if ( !Q_stricmp( token, infoParms[i].name ) ) {
			shader.surfaceFlags |= infoParms[i].surfaceFlags;
			shader.contentFlags |= infoParms[i].contents;
			shader.contentFlags &= infoParms[i].clearSolid;
			break;
		}
	}
}

/*
======================================================================================================================================
                                            Rend2 - Backward Compatibility - Material Types.
======================================================================================================================================

This does the job of backward compatibility well enough...
Sucky method, and I know it, but I don't have the time or patience to manually edit every shader in the game manually...
This is good enough!

TODO: At some point, add external overrides file...

*/

qboolean HaveSurfaceType( int surfaceFlags )
{
	switch( surfaceFlags & MATERIAL_MASK )
	{
	case MATERIAL_WATER:			// 13			// light covering of water on a surface
	case MATERIAL_SHORTGRASS:		// 5			// manicured lawn
	case MATERIAL_LONGGRASS:		// 6			// long jungle grass
	case MATERIAL_SAND:				// 8			// sandy beach
	case MATERIAL_CARPET:			// 27			// lush carpet
	case MATERIAL_GRAVEL:			// 9			// lots of small stones
	case MATERIAL_ROCK:				// 23			//
	case MATERIAL_TILES:			// 26			// tiled floor
	case MATERIAL_SOLIDWOOD:		// 1			// freshly cut timber
	case MATERIAL_HOLLOWWOOD:		// 2			// termite infested creaky wood
	case MATERIAL_SOLIDMETAL:		// 3			// solid girders
	case MATERIAL_HOLLOWMETAL:		// 4			// hollow metal machines
	case MATERIAL_DRYLEAVES:		// 19			// dried up leaves on the floor
	case MATERIAL_GREENLEAVES:		// 20			// fresh leaves still on a tree
	case MATERIAL_FABRIC:			// 21			// Cotton sheets
	case MATERIAL_CANVAS:			// 22			// tent material
	case MATERIAL_MARBLE:			// 12			// marble floors
	case MATERIAL_SNOW:				// 14			// freshly laid snow
	case MATERIAL_MUD:				// 17			// wet soil
	case MATERIAL_DIRT:				// 7			// hard mud
	case MATERIAL_CONCRETE:			// 11			// hardened concrete pavement
	case MATERIAL_FLESH:			// 16			// hung meat, corpses in the world
	case MATERIAL_RUBBER:			// 24			// hard tire like rubber
	case MATERIAL_PLASTIC:			// 25			//
	case MATERIAL_PLASTER:			// 28			// drywall style plaster
	case MATERIAL_SHATTERGLASS:		// 29			// glass with the Crisis Zone style shattering
	case MATERIAL_ARMOR:			// 30			// body armor
	case MATERIAL_ICE:				// 15			// packed snow/solid ice
	case MATERIAL_GLASS:			// 10			//
	case MATERIAL_BPGLASS:			// 18			// bulletproof glass
	case MATERIAL_COMPUTER:			// 31			// computers/electronic equipment
		return qtrue;
		break;
	default:
		break;
	}

	return qfalse;
}

void DebugSurfaceTypeSelection( const char *name, int surfaceFlags )
{
	if (!r_materialDebug->integer)	return; // disable debugging for now

	if (StringContainsWord(name, "gfx/") || StringContainsWord(name, "sprites/") || StringContainsWord(name, "powerups/"))
		return; // Ignore all these to reduce spam for now...

	if (StringContainsWord(name, "models/weapon"))
		return; // Ignore all these to reduce spam for now...

	//if (StringContainsWord(name, "models/player"))
	//	return; // Ignore all these to reduce spam for now...

	switch( surfaceFlags & MATERIAL_MASK )
	{
	case MATERIAL_WATER:			// 13			// light covering of water on a surface
		ri->Printf(PRINT_WARNING, "Surface %s was set to MATERIAL_WATER.\n", name);
		break;
	case MATERIAL_SHORTGRASS:		// 5			// manicured lawn
		ri->Printf(PRINT_WARNING, "Surface %s was set to MATERIAL_SHORTGRASS.\n", name);
		break;
	case MATERIAL_LONGGRASS:		// 6			// long jungle grass
		ri->Printf(PRINT_WARNING, "Surface %s was set to MATERIAL_LONGGRASS.\n", name);
		break;
	case MATERIAL_SAND:				// 8			// sandy beach
		ri->Printf(PRINT_WARNING, "Surface %s was set to MATERIAL_SAND.\n", name);
		break;
	case MATERIAL_CARPET:			// 27			// lush carpet
		ri->Printf(PRINT_WARNING, "Surface %s was set to MATERIAL_CARPET.\n", name);
		break;
	case MATERIAL_GRAVEL:			// 9			// lots of small stones
		ri->Printf(PRINT_WARNING, "Surface %s was set to MATERIAL_GRAVEL.\n", name);
		break;
	case MATERIAL_ROCK:				// 23			//
		ri->Printf(PRINT_WARNING, "Surface %s was set to MATERIAL_ROCK.\n", name);
		break;
	case MATERIAL_TILES:			// 26			// tiled floor
		ri->Printf(PRINT_WARNING, "Surface %s was set to MATERIAL_TILES.\n", name);
		break;
	case MATERIAL_SOLIDWOOD:		// 1			// freshly cut timber
		ri->Printf(PRINT_WARNING, "Surface %s was set to MATERIAL_SOLIDWOOD.\n", name);
		break;
	case MATERIAL_HOLLOWWOOD:		// 2			// termite infested creaky wood
		ri->Printf(PRINT_WARNING, "Surface %s was set to MATERIAL_HOLLOWWOOD.\n", name);
		break;
	case MATERIAL_SOLIDMETAL:		// 3			// solid girders
		ri->Printf(PRINT_WARNING, "Surface %s was set to MATERIAL_SOLIDMETAL.\n", name);
		break;
	case MATERIAL_HOLLOWMETAL:		// 4			// hollow metal machines
		ri->Printf(PRINT_WARNING, "Surface %s was set to MATERIAL_HOLLOWMETAL.\n", name);
		break;
	case MATERIAL_DRYLEAVES:		// 19			// dried up leaves on the floor
		ri->Printf(PRINT_WARNING, "Surface %s was set to MATERIAL_DRYLEAVES.\n", name);
		break;
	case MATERIAL_GREENLEAVES:		// 20			// fresh leaves still on a tree
		ri->Printf(PRINT_WARNING, "Surface %s was set to MATERIAL_GREENLEAVES.\n", name);
		break;
	case MATERIAL_FABRIC:			// 21			// Cotton sheets
		ri->Printf(PRINT_WARNING, "Surface %s was set to MATERIAL_FABRIC.\n", name);
		break;
	case MATERIAL_CANVAS:			// 22			// tent material
		ri->Printf(PRINT_WARNING, "Surface %s was set to MATERIAL_CANVAS.\n", name);
		break;
	case MATERIAL_MARBLE:			// 12			// marble floors
		ri->Printf(PRINT_WARNING, "Surface %s was set to MATERIAL_MARBLE.\n", name);
		break;
	case MATERIAL_SNOW:				// 14			// freshly laid snow
		ri->Printf(PRINT_WARNING, "Surface %s was set to MATERIAL_SNOW.\n", name);
		break;
	case MATERIAL_MUD:				// 17			// wet soil
		ri->Printf(PRINT_WARNING, "Surface %s was set to MATERIAL_MUD.\n", name);
		break;
	case MATERIAL_DIRT:				// 7			// hard mud
		ri->Printf(PRINT_WARNING, "Surface %s was set to MATERIAL_DIRT.\n", name);
		break;
	case MATERIAL_CONCRETE:			// 11			// hardened concrete pavement
		ri->Printf(PRINT_WARNING, "Surface %s was set to MATERIAL_CONCRETE.\n", name);
		break;
	case MATERIAL_FLESH:			// 16			// hung meat, corpses in the world
		ri->Printf(PRINT_WARNING, "Surface %s was set to MATERIAL_FLESH.\n", name);
		break;
	case MATERIAL_RUBBER:			// 24			// hard tire like rubber
		ri->Printf(PRINT_WARNING, "Surface %s was set to MATERIAL_RUBBER.\n", name);
		break;
	case MATERIAL_PLASTIC:			// 25			//
		ri->Printf(PRINT_WARNING, "Surface %s was set to MATERIAL_PLASTIC.\n", name);
		break;
	case MATERIAL_PLASTER:			// 28			// drywall style plaster
		ri->Printf(PRINT_WARNING, "Surface %s was set to MATERIAL_PLASTER.\n", name);
		break;
	case MATERIAL_SHATTERGLASS:		// 29			// glass with the Crisis Zone style shattering
		ri->Printf(PRINT_WARNING, "Surface %s was set to MATERIAL_SHATTERGLASS.\n", name);
		break;
	case MATERIAL_ARMOR:			// 30			// body armor
		ri->Printf(PRINT_WARNING, "Surface %s was set to MATERIAL_ARMOR.\n", name);
		break;
	case MATERIAL_ICE:				// 15			// packed snow/solid ice
		ri->Printf(PRINT_WARNING, "Surface %s was set to MATERIAL_ICE.\n", name);
		break;
	case MATERIAL_GLASS:			// 10			//
		ri->Printf(PRINT_WARNING, "Surface %s was set to MATERIAL_GLASS.\n", name);
		break;
	case MATERIAL_BPGLASS:			// 18			// bulletproof glass
		ri->Printf(PRINT_WARNING, "Surface %s was set to MATERIAL_BPGLASS.\n", name);
		break;
	case MATERIAL_COMPUTER:			// 31			// computers/electronic equipment
		ri->Printf(PRINT_WARNING, "Surface %s was set to MATERIAL_COMPUTER.\n", name);
		break;
	default:
		ri->Printf(PRINT_WARNING, "Surface %s was set to MATERIAL_NONE.\n", name);
		break;
	}
}

qboolean StringsContainWord ( const char *heystack, const char *heystack2,  char *needle )
{
	if (StringContainsWord(heystack, needle)) return qtrue;
	if (StringContainsWord(heystack2, needle)) return qtrue;
	return qfalse;
}

qboolean IsKnownShinyMap2 ( const char *heystack )
{
	if (StringContainsWord(heystack, "/players/")) return qfalse;
	if (StringContainsWord(heystack, "/bespin/")) return qtrue;
	if (StringContainsWord(heystack, "/cloudcity/")) return qtrue;
	if (StringContainsWord(heystack, "/byss/")) return qtrue;
	if (StringContainsWord(heystack, "/cairn/")) return qtrue;
	if (StringContainsWord(heystack, "/doomgiver/")) return qtrue;
	if (StringContainsWord(heystack, "/factory/")) return qtrue;
	if (StringContainsWord(heystack, "/hoth/")) return qtrue;
	if (StringContainsWord(heystack, "/impdetention/")) return qtrue;
	if (StringContainsWord(heystack, "/imperial/")) return qtrue;
	if (StringContainsWord(heystack, "/impgarrison/")) return qtrue;
	if (StringContainsWord(heystack, "/kejim/")) return qtrue;
	if (StringContainsWord(heystack, "/nar_hideout/")) return qtrue;
	if (StringContainsWord(heystack, "/nar_streets/")) return qtrue;
	if (StringContainsWord(heystack, "/narshaddaa/")) return qtrue;
	if (StringContainsWord(heystack, "/rail/")) return qtrue;
	if (StringContainsWord(heystack, "/rooftop/")) return qtrue;
	if (StringContainsWord(heystack, "/taspir/")) return qtrue; // lots of metal... will try this
	if (StringContainsWord(heystack, "/vjun/")) return qtrue;
	if (StringContainsWord(heystack, "/wedge/")) return qtrue;

	// MB2 Maps...
	if (StringContainsWord(heystack, "/epiii_boc/")) return qtrue;
	if (StringContainsWord(heystack, "/amace_cc/")) return qtrue;
	if (StringContainsWord(heystack, "/bespinsp/")) return qtrue;
	if (StringContainsWord(heystack, "/imm_cc/")) return qtrue;
	if (StringContainsWord(heystack, "/com_tower/")) return qtrue;
	if (StringContainsWord(heystack, "/imperial_tram/")) return qtrue;
	if (StringContainsWord(heystack, "/corellia/")) return qtrue;
	if (StringContainsWord(heystack, "/mb2_outlander/")) return qtrue;
	if (StringContainsWord(heystack, "/falcon_sjc/")) return qtrue;
	if (StringContainsWord(heystack, "/second-deathstar/")) return qtrue;
	if (StringContainsWord(heystack, "/thedeathstar/")) return qtrue;
	if (StringContainsWord(heystack, "/casa_del_paria/")) return qtrue;
	if (StringContainsWord(heystack, "/evil3_")) return qtrue;
	if (StringContainsWord(heystack, "/hanger/")) return qtrue;
	if (StringContainsWord(heystack, "/naboo/")) return qtrue;
	if (StringContainsWord(heystack, "/shinfl/")) return qtrue;
	if (StringContainsWord(heystack, "/hangar/")) return qtrue;
	if (StringContainsWord(heystack, "/lab/")) return qtrue;
	if (StringContainsWord(heystack, "/mainhall/")) return qtrue;
	if (StringContainsWord(heystack, "/amace_kamino/")) return qtrue;
	if (StringContainsWord(heystack, "/kamino/")) return qtrue;
	if (StringContainsWord(heystack, "/mb2_kamino/")) return qtrue;
	if (StringContainsWord(heystack, "/plasma_Kamino/")) return qtrue;
	if (StringContainsWord(heystack, "/plasma_Mustafar/")) return qtrue; // hmm... maybe???
	if (StringContainsWord(heystack, "/mygeeto1a/")) return qtrue;
	if (StringContainsWord(heystack, "/mygeeto1c/")) return qtrue;
	if (StringContainsWord(heystack, "/ddee_hangarc/")) return qtrue;
	if (StringContainsWord(heystack, "/droidee/")) return qtrue;
	if (StringContainsWord(heystack, "/amace_detention/")) return qtrue;
	if (StringContainsWord(heystack, "/amace_leviathan/")) return qtrue;
	if (StringContainsWord(heystack, "/amace_reactor/")) return qtrue;
	if (StringContainsWord(heystack, "/Asjc_battle_over/")) return qtrue;
	if (StringContainsWord(heystack, "/Asjc_palp/")) return qtrue;
	if (StringContainsWord(heystack, "/Asjc_starship/")) return qtrue;
	if (StringContainsWord(heystack, "/Asjc_utapua/")) return qtrue;
	if (StringContainsWord(heystack, "/mace_dark/")) return qtrue;
	if (StringContainsWord(heystack, "/mace_hanger/")) return qtrue;
	if (StringContainsWord(heystack, "/plasma_TantIV/")) return qtrue;
	if (StringContainsWord(heystack, "/ship/")) return qtrue;
	if (StringContainsWord(heystack, "/plasma_Exec/")) return qtrue;
	if (StringContainsWord(heystack, "/tantive/")) return qtrue;
	if (StringContainsWord(heystack, "/tantive1/")) return qtrue;
	if (StringContainsWord(heystack, "/MMT/")) return qtrue;
	if (StringContainsWord(heystack, "/plasma_Hangar/")) return qtrue;
	if (StringContainsWord(heystack, "/plasma_TFed/")) return qtrue;
	if (StringContainsWord(heystack, "/plasma_TFedTOO/")) return qtrue;
	if (StringContainsWord(heystack, "/plasma_TradeFed/")) return qtrue;

	// Misc Maps...
	if (StringContainsWord(heystack, "/atlantica/")) return qtrue;
	if (StringContainsWord(heystack, "/Carida/")) return qtrue;
	if (StringContainsWord(heystack, "/bunker/")) return qtrue;
	if (StringContainsWord(heystack, "/DF/")) return qtrue;
	if (StringContainsWord(heystack, "/bespinnew/")) return qtrue;
	if (StringContainsWord(heystack, "/cloudcity/")) return qtrue;
	if (StringContainsWord(heystack, "/coruscantsjc/")) return qtrue;
	if (StringContainsWord(heystack, "/ffawedge/")) return qtrue;
	if (StringContainsWord(heystack, "/jenshotel/")) return qtrue;
	if (StringContainsWord(heystack, "/CoruscantStreets/")) return qtrue;
	if (StringContainsWord(heystack, "/ctf_fighterbays/")) return qtrue;
	if (StringContainsWord(heystack, "/e3sjc/")) return qtrue;
	if (StringContainsWord(heystack, "/mustafar_sjc/")) return qtrue;
	if (StringContainsWord(heystack, "/fearis/")) return qtrue;
	if (StringContainsWord(heystack, "/kotor_dantooine/")) return qtrue;
	if (StringContainsWord(heystack, "/kotor_ebon_hawk/")) return qtrue;
	if (StringContainsWord(heystack, "/deltaphantom/")) return qtrue;
	if (StringContainsWord(heystack, "/pass_me_around/")) return qtrue;
	if (StringContainsWord(heystack, "/AMegaCity/")) return qtrue;
	if (StringContainsWord(heystack, "/mantell")) return qtrue;
	if (StringContainsWord(heystack, "/rcruiser/")) return qtrue;
	if (StringContainsWord(heystack, "/Asjc_padme/")) return qtrue;
	if (StringContainsWord(heystack, "/Anaboo/")) return qtrue;
	if (StringContainsWord(heystack, "/Asjc_imort/")) return qtrue;
	if (StringContainsWord(heystack, "/Asjc_mygeeto/")) return qtrue;
	if (StringContainsWord(heystack, "/ACrimeHutt/")) return qtrue;
	if (StringContainsWord(heystack, "/ASenateBase/")) return qtrue;

	// Warzone
	if (StringContainsWord(heystack, "/impfact/")) return qtrue;

	return qfalse;
}

qboolean IsKnownShinyMap ( const char *heystack )
{
	if (IsKnownShinyMap2( heystack ))
	{
		if (r_materialDebug->integer >= 2)
			ri->Printf(PRINT_WARNING, "Surface %s is known shiny.\n", heystack);

		return qtrue;
	}

	return qfalse;
}

int DetectMaterialType ( const char *name )
{
	//
	// Special cases - where we are pretty sure we want lots of specular and reflection...
	//
	if (StringContainsWord(name, "jetpack"))
		return MATERIAL_SOLIDMETAL;
	else if (StringContainsWord(name, "plastic") || StringContainsWord(name, "stormtrooper") || StringContainsWord(name, "snowtrooper") || StringContainsWord(name, "medpac") || StringContainsWord(name, "bacta") || StringContainsWord(name, "helmet") || StringContainsWord(name, "feather"))
		return MATERIAL_PLASTIC;
	else if (StringContainsWord(name, "/ships/") || StringContainsWord(name, "engine") || StringContainsWord(name, "mp/flag"))
		return MATERIAL_SOLIDMETAL;//MATERIAL_PLASTIC;
	else if (StringContainsWord(name, "wing") || StringContainsWord(name, "xwbody") || StringContainsWord(name, "tie_"))
		return MATERIAL_SOLIDMETAL;//MATERIAL_PLASTIC;
	else if (StringContainsWord(name, "ship") || StringContainsWord(name, "shuttle") || StringContainsWord(name, "falcon"))
		return MATERIAL_SOLIDMETAL;//MATERIAL_PLASTIC;
	else if (StringContainsWord(name, "freight") || StringContainsWord(name, "transport") || StringContainsWord(name, "crate"))
		return MATERIAL_SOLIDMETAL;//MATERIAL_PLASTIC;
	else if (StringContainsWord(name, "container") || StringContainsWord(name, "barrel") || StringContainsWord(name, "train"))
		return MATERIAL_SOLIDMETAL;//MATERIAL_PLASTIC;
	else if (StringContainsWord(name, "crane") || StringContainsWord(name, "plate") || StringContainsWord(name, "cargo"))
		return MATERIAL_SOLIDMETAL;//MATERIAL_PLASTIC;
	else if (StringContainsWord(name, "ship_"))
		return MATERIAL_SOLIDMETAL;//MATERIAL_PLASTIC;
	else if (!StringContainsWord(name, "trainer") && StringContainsWord(name, "train"))
		return MATERIAL_SOLIDMETAL;
	else if (StringContainsWord(name, "reborn") || StringContainsWord(name, "trooper"))
		return MATERIAL_ARMOR;
	else if (StringContainsWord(name, "boba") || StringContainsWord(name, "pilot"))
		return MATERIAL_ARMOR;
	else if (StringContainsWord(name, "water") && !StringContainsWord(name, "splash") && !StringContainsWord(name, "drip") && !StringContainsWord(name, "ripple") && !StringContainsWord(name, "bubble") && !StringContainsWord(name, "woosh") && !StringContainsWord(name, "underwater") && !StringContainsWord(name, "bottom"))
	{
		return MATERIAL_WATER;
		shader.isWater = qtrue;
	}
	else if (StringContainsWord(name, "grass") || StringContainsWord(name, "foliage") || StringContainsWord(name, "yavin/ground") || StringContainsWord(name, "mp/s_ground") || StringContainsWord(name, "volcano/terrain") || StringContainsWord(name, "bay/terrain") || StringContainsWord(name, "towers/terrain") || StringContainsWord(name, "yavinassault/terrain"))
		return MATERIAL_SHORTGRASS;
	else if (StringContainsWord(name, "vj4")) // special case for vjun rock...
		return MATERIAL_ROCK;

	//
	// Player model stuff overrides
	//
	else if (StringContainsWord(name, "players") && StringContainsWord(name, "eye") || StringContainsWord(name, "goggles"))
		return MATERIAL_GLASS;
	else if (StringContainsWord(name, "bespin/bench") && StringContainsWord(name, "bespin/light"))
		return MATERIAL_HOLLOWMETAL;
	else if (!StringContainsWord(name, "players") && StringContainsWord(name, "bespin"))
		return MATERIAL_MARBLE;
	else if (StringContainsWord(name, "players") && !StringContainsWord(name, "glass") && (StringContainsWord(name, "sithsoldier") || StringContainsWord(name, "sith_assassin") || StringContainsWord(name, "r2d2") || StringContainsWord(name, "protocol") || StringContainsWord(name, "r5d2") || StringContainsWord(name, "c3po") || StringContainsWord(name, "hk4") || StringContainsWord(name, "hk5") || StringContainsWord(name, "droid") || StringContainsWord(name, "shadowtrooper")))
		return MATERIAL_SOLIDMETAL;
	else if (StringContainsWord(name, "players") && StringContainsWord(name, "shadowtrooper"))
		return MATERIAL_SOLIDMETAL; // dunno about this one.. looks good as armor...
	else if (StringContainsWord(name, "players") && (StringContainsWord(name, "mandalore") || StringContainsWord(name, "mandalorian") || StringContainsWord(name, "sith_warrior/body")))
		return MATERIAL_ARMOR;
	else if (StringContainsWord(name, "players") && (StringContainsWord(name, "hood") || StringContainsWord(name, "robe") || StringContainsWord(name, "cloth") || StringContainsWord(name, "pants") || StringContainsWord(name, "sith_warrior/bandon_body")))
		return MATERIAL_FABRIC;
	else if (StringContainsWord(name, "players") && (StringContainsWord(name, "hair") || StringContainsWord(name, "chewbacca"))) // use carpet
		return MATERIAL_FABRIC;//MATERIAL_CARPET; Just because it has a bit of parallax and suitable specular...
	else if (StringContainsWord(name, "players") && (StringContainsWord(name, "flesh") || StringContainsWord(name, "body") || StringContainsWord(name, "leg") || StringContainsWord(name, "hand") || StringContainsWord(name, "head") || StringContainsWord(name, "hips") || StringContainsWord(name, "torso") || StringContainsWord(name, "tentacles") || StringContainsWord(name, "face") || StringContainsWord(name, "arms") || StringContainsWord(name, "sith_warrior/head") || StringContainsWord(name, "sith_warrior/bandon_head")))
		return MATERIAL_FLESH;
	else if (StringContainsWord(name, "players") && (StringContainsWord(name, "arm") || StringContainsWord(name, "foot") || StringContainsWord(name, "neck")))
		return MATERIAL_FLESH;
	else if (StringContainsWord(name, "players") && (StringContainsWord(name, "skirt") || StringContainsWord(name, "boots") || StringContainsWord(name, "accesories") || StringContainsWord(name, "accessories") || StringContainsWord(name, "vest") || StringContainsWord(name, "holster") || StringContainsWord(name, "cap") || StringContainsWord(name, "collar")))
		return MATERIAL_FABRIC;
	else if (!StringContainsWord(name, "players") && StringContainsWord(name, "_cc"))
		return MATERIAL_MARBLE;
	//
	// If player model material not found above, use defaults...
	//

	//
	// Stuff we can be pretty sure of...
	//
	else if (StringContainsWord(name, "concrete"))
		return MATERIAL_CONCRETE;
	else if (!StringContainsWord(name, "glow") && !StringContainsWord(name, "glw") && StringContainsWord(name, "models/weapon") && StringContainsWord(name, "saber") && StringContainsWord(name, "bespin/bench") && StringContainsWord(name, "bespin/light"))
		return MATERIAL_HOLLOWMETAL; // UQ1: Using hollowmetal for weapons to force low parallax setting...
	else if (!StringContainsWord(name, "glow") && !StringContainsWord(name, "glw") && (StringContainsWord(name, "models/weapon") || StringContainsWord(name, "scope") || StringContainsWord(name, "blaster") || StringContainsWord(name, "pistol") || StringContainsWord(name, "thermal") || StringContainsWord(name, "bowcaster") || StringContainsWord(name, "cannon") || StringContainsWord(name, "saber") || StringContainsWord(name, "rifle") || StringContainsWord(name, "rocket")))
		return MATERIAL_HOLLOWMETAL; // UQ1: Using hollowmetal for weapons to force low parallax setting...
	else if (StringContainsWord(name, "metal") || StringContainsWord(name, "pipe") || StringContainsWord(name, "shaft") || StringContainsWord(name, "jetpack") || StringContainsWord(name, "antenna") || StringContainsWord(name, "xwing") || StringContainsWord(name, "tie_") || StringContainsWord(name, "raven") || StringContainsWord(name, "falcon") || StringContainsWord(name, "engine") || StringContainsWord(name, "elevator") || StringContainsWord(name, "evaporator") || StringContainsWord(name, "airpur") || StringContainsWord(name, "gonk") || StringContainsWord(name, "droid") || StringContainsWord(name, "cart") || StringContainsWord(name, "vent") || StringContainsWord(name, "tank") || StringContainsWord(name, "transformer") || StringContainsWord(name, "generator") || StringContainsWord(name, "grate") || StringContainsWord(name, "rack") || StringContainsWord(name, "mech") || StringContainsWord(name, "turbolift") || StringContainsWord(name, "tube") || StringContainsWord(name, "coil") || StringContainsWord(name, "vader_trim") || StringContainsWord(name, "newfloor_vjun") || StringContainsWord(name, "bay_beam"))
		return MATERIAL_SOLIDMETAL;
	else if (StringContainsWord(name, "armor") || StringContainsWord(name, "armour"))
		return MATERIAL_ARMOR;
	else if (StringContainsWord(name, "textures/byss/") && !StringContainsWord(name, "glow") && !StringContainsWord(name, "glw") && !StringContainsWord(name, "static") && !StringContainsWord(name, "isd") && !StringContainsWord(name, "power") && !StringContainsWord(name, "env_") && !StringContainsWord(name, "byss_switch"))
		return MATERIAL_SOLIDMETAL; // special for byss shiny
	else if (StringContainsWord(name, "textures/vjun/") && !StringContainsWord(name, "glow") && !StringContainsWord(name, "glw") && !StringContainsWord(name, "static") && !StringContainsWord(name, "light") && !StringContainsWord(name, "env_") && !StringContainsWord(name, "_env") && !StringContainsWord(name, "switch_off") && !StringContainsWord(name, "switch_on") && !StringContainsWord(name, "screen") && !StringContainsWord(name, "blend") && !StringContainsWord(name, "o_ground") && !StringContainsWord(name, "_onoffg") && !StringContainsWord(name, "_onoffr") && !StringContainsWord(name, "console"))
		return MATERIAL_SOLIDMETAL; // special for vjun shiny
	else if (StringContainsWord(name, "sand"))
		return MATERIAL_SAND;
	else if (StringContainsWord(name, "gravel"))
		return MATERIAL_GRAVEL;
	else if ((StringContainsWord(name, "dirt") || StringContainsWord(name, "ground")) && !StringContainsWord(name, "menus/main_background"))
		return MATERIAL_DIRT;
	else if (IsKnownShinyMap(name) && StringContainsWord(name, "stucco"))
		return MATERIAL_TILES;
	else if (StringContainsWord(name, "rift") && StringContainsWord(name, "piller"))
		return MATERIAL_MARBLE;
	else if (StringContainsWord(name, "stucco") || StringContainsWord(name, "piller") || StringContainsWord(name, "sith_jp"))
		return MATERIAL_CONCRETE;
	else if (StringContainsWord(name, "marbl") || StringContainsWord(name, "teeth"))
		return MATERIAL_MARBLE;
	else if (StringContainsWord(name, "snow"))
		return MATERIAL_SNOW;
	else if (StringContainsWord(name, "canvas"))
		return MATERIAL_CANVAS;
	else if (StringContainsWord(name, "rock"))
		return MATERIAL_ROCK;
	else if (StringContainsWord(name, "rubber"))
		return MATERIAL_RUBBER;
	else if (StringContainsWord(name, "carpet"))
		return MATERIAL_CARPET;
	else if (StringContainsWord(name, "plaster"))
		return MATERIAL_PLASTER;
	else if (StringContainsWord(name, "computer") || StringContainsWord(name, "console") || StringContainsWord(name, "button") || StringContainsWord(name, "terminal") || StringContainsWord(name, "switch") || StringContainsWord(name, "panel") || StringContainsWord(name, "control"))
		return MATERIAL_COMPUTER;
	else if (StringContainsWord(name, "fabric"))
		return MATERIAL_FABRIC;
	else if (StringContainsWord(name, "tree") || StringContainsWord(name, "leaf") || StringContainsWord(name, "leaves") || StringContainsWord(name, "fern") || StringContainsWord(name, "vine"))
		return MATERIAL_GREENLEAVES;
	else if (StringContainsWord(name, "wood") && !StringContainsWord(name, "street"))
		return MATERIAL_SOLIDWOOD;
	else if (StringContainsWord(name, "mud"))
		return MATERIAL_MUD;
	else if (StringContainsWord(name, "ice"))
		return MATERIAL_ICE;
	else if ((StringContainsWord(name, "grass") || StringContainsWord(name, "foliage")) && (StringContainsWord(name, "long") || StringContainsWord(name, "tall") || StringContainsWord(name, "thick")))
		return MATERIAL_LONGGRASS;
	else if (StringContainsWord(name, "grass") || StringContainsWord(name, "foliage"))
		return MATERIAL_SHORTGRASS;
	else if (IsKnownShinyMap(name) && StringContainsWord(name, "floor"))
		return MATERIAL_TILES;
	else if (StringContainsWord(name, "floor"))
		return MATERIAL_CONCRETE;
	else if (StringContainsWord(name, "textures/mp/") && !StringContainsWord(name, "glow") && !StringContainsWord(name, "glw") && !StringContainsWord(name, "static") && !StringContainsWord(name, "light") && !StringContainsWord(name, "env_") && !StringContainsWord(name, "_env") && !StringContainsWord(name, "underside") && !StringContainsWord(name, "blend") && !StringContainsWord(name, "t_pit") && !StringContainsWord(name, "desert") && !StringContainsWord(name, "cliff"))
		return MATERIAL_SOLIDMETAL; // special for mp shiny
	else if (IsKnownShinyMap(name) && StringContainsWord(name, "frame"))
		return MATERIAL_SOLIDMETAL;
	else if (IsKnownShinyMap(name) && StringContainsWord(name, "wall"))
		return MATERIAL_SOLIDMETAL;
	else if (StringContainsWord(name, "wall") || StringContainsWord(name, "underside"))
		return MATERIAL_CONCRETE;
	else if (IsKnownShinyMap(name) && StringContainsWord(name, "door"))
		return MATERIAL_SOLIDMETAL;
	else if (StringContainsWord(name, "door"))
		return MATERIAL_CONCRETE;
	else if (IsKnownShinyMap(name) && StringContainsWord(name, "ground"))
		return MATERIAL_TILES; // dunno about this one
	else if (StringContainsWord(name, "ground"))
		return MATERIAL_CONCRETE;
	else if (StringContainsWord(name, "desert"))
		return MATERIAL_CONCRETE;
	else if (IsKnownShinyMap(name) && (StringContainsWord(name, "tile") || StringContainsWord(name, "lift")))
		return MATERIAL_SOLIDMETAL;
	else if (StringContainsWord(name, "tile") || StringContainsWord(name, "lift"))
		return MATERIAL_TILES;
	else if (StringContainsWord(name, "glass") || StringContainsWord(name, "light") || StringContainsWord(name, "screen") || StringContainsWord(name, "lamp") || StringContainsWord(name, "crystal"))
		return MATERIAL_GLASS;
	else if (StringContainsWord(name, "flag"))
		return MATERIAL_FABRIC;
	else if (StringContainsWord(name, "column") || StringContainsWord(name, "stone") || StringContainsWord(name, "statue"))
		return MATERIAL_MARBLE;
	// Extra backup - backup stuff. Used when nothing better found...
	else if (StringContainsWord(name, "red") || StringContainsWord(name, "blue") || StringContainsWord(name, "yellow") || StringContainsWord(name, "white") || StringContainsWord(name, "monitor"))
		return MATERIAL_PLASTIC;
	else if (StringContainsWord(name, "yavin") && (StringContainsWord(name, "trim") || StringContainsWord(name, "step") || StringContainsWord(name, "pad")))
		return MATERIAL_ROCK;
	else if (!StringContainsWord(name, "players") && (StringContainsWord(name, "coruscant") || StringContainsWord(name, "/rooftop/") || StringContainsWord(name, "/nar_") || StringContainsWord(name, "/imperial/")))
		return MATERIAL_TILES;
	else if (!StringContainsWord(name, "players") && (StringContainsWord(name, "deathstar") || StringContainsWord(name, "imperial") || StringContainsWord(name, "shuttle") || StringContainsWord(name, "destroyer")))
		return MATERIAL_TILES;
	else if (!StringContainsWord(name, "players") && StringContainsWord(name, "dantooine"))
		return MATERIAL_MARBLE;
	else if (StringContainsWord(name, "outside"))
		return MATERIAL_CONCRETE; // Outside, assume concrete...
	else if (StringContainsWord(name, "out") && (StringContainsWord(name, "trim") || StringContainsWord(name, "step") || StringContainsWord(name, "pad")))
		return MATERIAL_CONCRETE; // Outside, assume concrete...
	else if (StringContainsWord(name, "out") && (StringContainsWord(name, "frame") || StringContainsWord(name, "wall") || StringContainsWord(name, "round") || StringContainsWord(name, "crate") || StringContainsWord(name, "trim") || StringContainsWord(name, "support") || StringContainsWord(name, "step") || StringContainsWord(name, "pad") || StringContainsWord(name, "weapon") || StringContainsWord(name, "gun")))
		return MATERIAL_CONCRETE; // Outside, assume concrete...
	else if (StringContainsWord(name, "frame") || StringContainsWord(name, "wall") || StringContainsWord(name, "round") || StringContainsWord(name, "crate") || StringContainsWord(name, "trim") || StringContainsWord(name, "support") || StringContainsWord(name, "step") || StringContainsWord(name, "pad") || StringContainsWord(name, "weapon") || StringContainsWord(name, "gun"))
		return MATERIAL_CONCRETE;
	else if (StringContainsWord(name, "yavin"))
		return MATERIAL_ROCK; // On yavin maps, assume rock for anything else...
	else if (StringContainsWord(name, "black") || StringContainsWord(name, "boon") || StringContainsWord(name, "items") || StringContainsWord(name, "shield"))
		return MATERIAL_PLASTIC;
	else if (StringContainsWord(name, "refract") || StringContainsWord(name, "reflect"))
		return MATERIAL_PLASTIC;
	else if (StringContainsWord(name, "map_objects") || StringContainsWord(name, "key"))
		return MATERIAL_SOLIDMETAL; // hmmm, maybe... testing...
	else if (StringContainsWord(name, "rodian"))
		return MATERIAL_FLESH;
	else if (StringContainsWord(name, "players")) // Fall back to flesh on anything not caught above...
		return MATERIAL_FLESH;
	else if (IsKnownShinyMap(name)) // Chances are it's shiny...
		return MATERIAL_TILES;
	else
	{
		if (!StringContainsWord(name, "gfx/")
			&& !StringContainsWord(name, "hud")
			&& !StringContainsWord(name, "fire")
			&& !StringContainsWord(name, "force")
			&& !StringContainsWord(name, "explo")
			&& !StringContainsWord(name, "cursor")
			&& !StringContainsWord(name, "sky")
			&& !StringContainsWord(name, "powerup")
			&& !StringContainsWord(name, "slider")
			&& !StringContainsWord(name, "mp/dark_")) // Dont bother reporting gfx/ or hud items...
			if (r_materialDebug->integer)
				ri->Printf(PRINT_WARNING, "Could not work out a default surface type for shader %s. It will fallback to default parallax and specular.\n", name);
	}

	return MATERIAL_NONE;
}

void AssignMaterialType ( const char *name, const char *text )
{
	//ri->Printf(PRINT_WARNING, "Check material type for %s.\n", name);

	if (r_disableGfxDirEnhancement->integer
		&& (StringContainsWord(name, "gfx/"))) return;

	if (StringContainsWord(name, "gfx/2d")
		|| StringContainsWord(name, "gfx/console")
		|| StringContainsWord(name, "gfx/colors")
		|| StringContainsWord(name, "gfx/digits")
		|| StringContainsWord(name, "gfx/hud")
		|| StringContainsWord(name, "gfx/jkg")
		|| StringContainsWord(name, "gfx/menu")) return;

	if (!HaveSurfaceType(shader.surfaceFlags))
	{
		int material = DetectMaterialType( name );

		if (material)
			shader.surfaceFlags |= material;
		//else
		//	shader.surfaceFlags |= MATERIAL_CARPET; // Fallback to a non-shiny default...
	}
	else
	{
		//
		// Special cases - where we are pretty sure we want lots of specular and reflection... Override!
		//
		if (StringContainsWord(name, "vj4")) // special case for vjun rock...
			shader.surfaceFlags |= MATERIAL_ROCK;
		else if (StringContainsWord(name, "plastic") || StringContainsWord(name, "stormtrooper") || StringContainsWord(name, "snowtrooper") || StringContainsWord(name, "medpac") || StringContainsWord(name, "bacta") || StringContainsWord(name, "helmet"))
			shader.surfaceFlags |= MATERIAL_PLASTIC;
		else if (StringContainsWord(name, "/ships/") || StringContainsWord(name, "engine") || StringContainsWord(name, "mp/flag"))
			shader.surfaceFlags |= MATERIAL_SOLIDMETAL;//MATERIAL_PLASTIC;
		else if (StringContainsWord(name, "wing") || StringContainsWord(name, "xwbody") || StringContainsWord(name, "tie_"))
			shader.surfaceFlags |= MATERIAL_SOLIDMETAL;//MATERIAL_PLASTIC;
		else if (StringContainsWord(name, "ship") || StringContainsWord(name, "shuttle") || StringContainsWord(name, "falcon"))
			shader.surfaceFlags |= MATERIAL_SOLIDMETAL;//MATERIAL_PLASTIC;
		else if (StringContainsWord(name, "freight") || StringContainsWord(name, "transport") || StringContainsWord(name, "crate"))
			shader.surfaceFlags |= MATERIAL_SOLIDMETAL;//MATERIAL_PLASTIC;
		else if (StringContainsWord(name, "container") || StringContainsWord(name, "barrel") || StringContainsWord(name, "train"))
			shader.surfaceFlags |= MATERIAL_SOLIDMETAL;//MATERIAL_PLASTIC;
		else if (StringContainsWord(name, "crane") || StringContainsWord(name, "plate") || StringContainsWord(name, "cargo"))
			shader.surfaceFlags |= MATERIAL_SOLIDMETAL;//MATERIAL_PLASTIC;
		else if (StringContainsWord(name, "ship_"))
			shader.surfaceFlags |= MATERIAL_SOLIDMETAL;//MATERIAL_PLASTIC;
		else if (StringContainsWord(name, "models/weapon") && StringContainsWord(name, "saber") && !StringContainsWord(name, "glow") && StringContainsWord(name, "bespin/bench") && StringContainsWord(name, "bespin/light"))
			shader.surfaceFlags |= MATERIAL_HOLLOWMETAL; // UQ1: Using hollowmetal for weapons to force low parallax setting...
		else if (StringContainsWord(name, "reborn") || StringContainsWord(name, "trooper"))
			shader.surfaceFlags |= MATERIAL_ARMOR;
		else if (StringContainsWord(name, "boba") || StringContainsWord(name, "pilot"))
			shader.surfaceFlags |= MATERIAL_ARMOR;
		else if (StringContainsWord(name, "grass") || (StringContainsWord(name, "foliage") && !StringContainsWord(name, "billboard")) || StringContainsWord(name, "yavin/ground") || StringContainsWord(name, "mp/s_ground") || StringContainsWord(name, "yavinassault/terrain"))
			shader.surfaceFlags |= MATERIAL_SHORTGRASS;

		//
		// Player model stuff overrides
		//
		else if (StringContainsWord(name, "players") && StringContainsWord(name, "eye"))
			shader.surfaceFlags |= MATERIAL_GLASS;
		else if (StringContainsWord(name, "bespin/bench") && StringContainsWord(name, "bespin/light"))
			shader.surfaceFlags |= MATERIAL_HOLLOWMETAL;
		else if (!StringContainsWord(name, "players") && StringContainsWord(name, "bespin"))
			shader.surfaceFlags |= MATERIAL_MARBLE;
		else if (StringContainsWord(name, "players") && (StringContainsWord(name, "sithsoldier") || StringContainsWord(name, "r2d2") || StringContainsWord(name, "protocol") || StringContainsWord(name, "r5d2") || StringContainsWord(name, "c3po")))
			shader.surfaceFlags |= MATERIAL_SOLIDMETAL;
		else if (StringContainsWord(name, "players") && (StringContainsWord(name, "hood") || StringContainsWord(name, "robe") || StringContainsWord(name, "cloth") || StringContainsWord(name, "pants")))
			shader.surfaceFlags |= MATERIAL_FABRIC;
		else if (StringContainsWord(name, "players") && (StringContainsWord(name, "hair") || StringContainsWord(name, "chewbacca"))) // use carpet
			shader.surfaceFlags |= MATERIAL_FABRIC;//MATERIAL_CARPET; Just because it has a bit of parallax and suitable specular...
		else if (StringContainsWord(name, "players") && (StringContainsWord(name, "flesh") || StringContainsWord(name, "body") || StringContainsWord(name, "leg") || StringContainsWord(name, "hand") || StringContainsWord(name, "head") || StringContainsWord(name, "hips") || StringContainsWord(name, "torso") || StringContainsWord(name, "tentacles") || StringContainsWord(name, "face") || StringContainsWord(name, "arms")))
			shader.surfaceFlags |= MATERIAL_FLESH;
		else if (StringContainsWord(name, "players") && (StringContainsWord(name, "arm") || StringContainsWord(name, "foot") || StringContainsWord(name, "neck")))
			shader.surfaceFlags |= MATERIAL_FLESH;
		else if (StringContainsWord(name, "players") && (StringContainsWord(name, "skirt") || StringContainsWord(name, "boots") || StringContainsWord(name, "accesories") || StringContainsWord(name, "accessories") || StringContainsWord(name, "vest") || StringContainsWord(name, "holster") || StringContainsWord(name, "cap") || StringContainsWord(name, "collar")))
			shader.surfaceFlags |= MATERIAL_FABRIC;
		else if (!StringContainsWord(name, "players") && StringContainsWord(name, "_cc"))
			shader.surfaceFlags |= MATERIAL_MARBLE;
		//
		// If player model material not found above, use defaults...
		//
	}

	if (StringContainsWord(name, "common/water") && !StringContainsWord(name, "splash") && !StringContainsWord(name, "drip") && !StringContainsWord(name, "ripple") && !StringContainsWord(name, "bubble") && !StringContainsWord(name, "woosh") && !StringContainsWord(name, "underwater") && !StringContainsWord(name, "bottom"))
	{
		int oldmat = ( shader.surfaceFlags & MATERIAL_MASK );
		if (oldmat) shader.surfaceFlags &= ~oldmat;
		shader.surfaceFlags |= MATERIAL_WATER;
		shader.isWater = qtrue;
	}
	else if (StringContainsWord(name, "vj4"))
	{// special case for vjun rock...
		int oldmat = (shader.surfaceFlags & MATERIAL_MASK);
		if (oldmat) shader.surfaceFlags &= ~oldmat;
		shader.surfaceFlags |= MATERIAL_ROCK;
	}
	else if (shader.hasAlpha &&
		!StringContainsWord(name, "billboard") &&
		(StringContainsWord(name, "grass") || StringContainsWord(name, "foliage") || StringContainsWord(name, "yavin/ground")
		|| StringContainsWord(name, "mp/s_ground") || StringContainsWord(name, "yavinassault/terrain")
		|| StringContainsWord(name, "tree") || StringContainsWord(name, "plant") || StringContainsWord(name, "bush")
		|| StringContainsWord(name, "shrub") || StringContainsWord(name, "leaf") || StringContainsWord(name, "leaves")
		|| StringContainsWord(name, "branch") || StringContainsWord(name, "flower") || StringContainsWord(name, "weed")))
	{// Always greenleaves... No parallax...
		int oldmat = ( shader.surfaceFlags & MATERIAL_MASK );
		if (oldmat) shader.surfaceFlags &= ~oldmat;

#ifdef FIXME_TREE_BARK_PARALLAX
		if (StringContainsWord(name, "bark") || StringContainsWord(name, "trunk") || StringContainsWord(name, "giant_tree") || StringContainsWord(name, "vine01"))
			shader.surfaceFlags |= MATERIAL_SOLIDWOOD;
		else
#endif
			shader.surfaceFlags |= MATERIAL_GREENLEAVES;

	}
	else if (StringContainsWord(name, "plastic") || StringContainsWord(name, "trooper") || StringContainsWord(name, "medpack"))
		if (!(shader.surfaceFlags & MATERIAL_PLASTIC)) shader.surfaceFlags |= MATERIAL_PLASTIC;
	else if (StringContainsWord(name, "grass") || (StringContainsWord(name, "foliage") && !StringContainsWord(name, "billboard")) || StringContainsWord(name, "yavin/ground")
		|| StringContainsWord(name, "mp/s_ground") || StringContainsWord(name, "yavinassault/terrain"))
		if (!(shader.surfaceFlags & MATERIAL_SHORTGRASS)) shader.surfaceFlags |= MATERIAL_SHORTGRASS;

	DebugSurfaceTypeSelection(name, shader.surfaceFlags);
}

/*
=================
ParseShader

The current text pointer is at the explicit text definition of the
shader.  Parse it into the global shader variable.  Later functions
will optimize it.
=================
*/
static qboolean ParseShader( const char *name, const char **text )
{
	char *token;
	const char *begin = *text;
	int s;

	s = 0;

	shader.frameOverride = -1; // Initialize this, so things work right...

	token = COM_ParseExt( text, qtrue );
	if ( token[0] != '{' )
	{
		ri->Printf( PRINT_WARNING, "WARNING: expecting '{', found '%s' instead in shader '%s'\n", token, shader.name );
		return qfalse;
	}

	while ( 1 )
	{
		token = COM_ParseExt( text, qtrue );

		if ( !token[0] )
		{
			ri->Printf( PRINT_WARNING, "WARNING: no concluding '}' in shader %s\n", shader.name );
			return qfalse;
		}

		if (StringsContainWord(name, token, "reborn"))
		{
			//ri->Printf(PRINT_WARNING, "Reborn seen in shader %s.\n", name);
			if (!(shader.surfaceFlags & MATERIAL_ARMOR)) shader.surfaceFlags |= MATERIAL_ARMOR;
		}

		// end of shader definition
		if ( token[0] == '}' )
		{
			break;
		}

		// stage definition
		else if ( token[0] == '{' )
		{
			if ( s >= MAX_SHADER_STAGES ) {
				ri->Printf( PRINT_WARNING, "WARNING: too many stages in shader %s\n", shader.name );
				return qfalse;
			}

			if ( !ParseStage( &stages[s], text ) )
			{
				return qfalse;
			}
			stages[s].active = qtrue;
			s++;

			continue;
		}
		// skip stuff that only the QuakeEdRadient needs
		else if ( !Q_stricmpn( token, "qer", 3 ) ) {
			SkipRestOfLine( text );
			continue;
		}
		// material deprecated as of 11 Jan 01
		// material undeprecated as of 7 May 01 - q3map_material deprecated
		else if ( !Q_stricmp( token, "material" ) || !Q_stricmp( token, "q3map_material" ) )
		{
			ParseMaterial( text );
		}
		// sun parms
		else if ( !Q_stricmp( token, "sun" ) || !Q_stricmp( token, "q3map_sun" ) || !Q_stricmp( token, "q3map_sunExt" ) || !Q_stricmp( token, "q3gl2_sun" ) ) {
			float	a, b;
			qboolean isGL2Sun = qfalse;

			if (!Q_stricmp( token, "q3gl2_sun" ) && r_sunlightMode->integer >= 2 )
			{
				isGL2Sun = qtrue;
				tr.sunShadows = qtrue;
			}

			token = COM_ParseExt( text, qfalse );
			tr.sunLight[0] = atof( token );
			token = COM_ParseExt( text, qfalse );
			tr.sunLight[1] = atof( token );
			token = COM_ParseExt( text, qfalse );
			tr.sunLight[2] = atof( token );

			VectorNormalize( tr.sunLight );

			token = COM_ParseExt( text, qfalse );
			a = atof( token );
			VectorScale( tr.sunLight, a, tr.sunLight);

			token = COM_ParseExt( text, qfalse );
			a = atof( token );
			a = a / 180 * M_PI;

			token = COM_ParseExt( text, qfalse );
			b = atof( token );
			b = b / 180 * M_PI;

			tr.sunDirection[0] = cos( a ) * cos( b );
			tr.sunDirection[1] = sin( a ) * cos( b );
			tr.sunDirection[2] = sin( b );

			if (isGL2Sun)
			{
				token = COM_ParseExt( text, qfalse );
				tr.mapLightScale = atof(token);

				token = COM_ParseExt( text, qfalse );
				tr.sunShadowScale = atof(token);

				if (tr.sunShadowScale < 0.0f)
				{
					ri->Printf (PRINT_WARNING, "WARNING: q3gl2_sun's 'shadow scale' value must be between 0 and 1. Clamping to 0.0.\n");
					tr.sunShadowScale = 0.0f;
				}
				else if (tr.sunShadowScale > 1.0f)
				{
					ri->Printf (PRINT_WARNING, "WARNING: q3gl2_sun's 'shadow scale' value must be between 0 and 1. Clamping to 1.0.\n");
					tr.sunShadowScale = 1.0f;
				}
			}

			SkipRestOfLine( text );
			continue;
		}
		// tonemap parms
		else if ( !Q_stricmp( token, "q3gl2_tonemap" ) ) {
			token = COM_ParseExt( text, qfalse );
			tr.toneMinAvgMaxLevel[0] = atof( token );
			token = COM_ParseExt( text, qfalse );
			tr.toneMinAvgMaxLevel[1] = atof( token );
			token = COM_ParseExt( text, qfalse );
			tr.toneMinAvgMaxLevel[2] = atof( token );

			token = COM_ParseExt( text, qfalse );
			tr.autoExposureMinMax[0] = atof( token );
			token = COM_ParseExt( text, qfalse );
			tr.autoExposureMinMax[1] = atof( token );

			SkipRestOfLine( text );
			continue;
		}
		// q3map_surfacelight deprecated as of 16 Jul 01
		else if ( !Q_stricmp( token, "surfacelight" ) || !Q_stricmp( token, "q3map_surfacelight" ) )
		{
			SkipRestOfLine( text );
			continue;
		}
		else if ( !Q_stricmp( token, "lightColor" ) )
		{
			SkipRestOfLine( text );
			continue;
		}
		else if ( !Q_stricmp( token, "deformvertexes" ) || !Q_stricmp( token, "deform" ) ) {
			ParseDeform( text );
			continue;
		}
		else if ( !Q_stricmp( token, "tesssize" ) ) {
			SkipRestOfLine( text );
			continue;
		}
		else if ( !Q_stricmp( token, "clampTime" ) ) {
			token = COM_ParseExt( text, qfalse );
			if (token[0]) {
				shader.clampTime = atof(token);
			}
		}
		// skip stuff that only the q3map needs
		else if ( !Q_stricmpn( token, "q3map", 5 ) ) {
			SkipRestOfLine( text );
			continue;
		}
		// skip stuff that only q3map or the server needs
		else if ( !Q_stricmp( token, "surfaceParm" ) ) {
			ParseSurfaceParm( text );
			continue;
		}
		// no mip maps
		else if ( !Q_stricmp( token, "nomipmaps" ) )
		{
			shader.noMipMaps = qtrue;
			shader.noPicMip = qtrue;
			continue;
		}
		// no picmip adjustment
		else if ( !Q_stricmp( token, "nopicmip" ) )
		{
			shader.noPicMip = qtrue;
			continue;
		}
		else if ( !Q_stricmp( token, "noglfog" ) )
		{
			//shader.fogPass = FP_NONE;
			continue;
		}
		// polygonOffset
		else if ( !Q_stricmp( token, "polygonOffset" ) )
		{
			shader.polygonOffset = qtrue;
			continue;
		}
		else if ( !Q_stricmp( token, "noTC" ) )
		{
			shader.noTC = qtrue;
			continue;
		}
		// entityMergable, allowing sprite surfaces from multiple entities
		// to be merged into one batch.  This is a savings for smoke
		// puffs and blood, but can't be used for anything where the
		// shader calcs (not the surface function) reference the entity color or scroll
		else if ( !Q_stricmp( token, "entityMergable" ) )
		{
			shader.entityMergable = qtrue;
			continue;
		}
		// fogParms
		else if ( !Q_stricmp( token, "fogParms" ) )
		{
			if ( !ParseVector( text, 3, shader.fogParms.color ) ) {
				return qfalse;
			}

			token = COM_ParseExt( text, qfalse );
			if ( !token[0] )
			{
				ri->Printf( PRINT_WARNING, "WARNING: missing parm for 'fogParms' keyword in shader '%s'\n", shader.name );
				continue;
			}
			shader.fogParms.depthForOpaque = atof( token );

			// skip any old gradient directions
			SkipRestOfLine( text );
			continue;
		}
		// portal
		else if ( !Q_stricmp(token, "portal") )
		{
			shader.sort = SS_PORTAL;
			shader.isPortal = qtrue;
			continue;
		}
		// skyparms <cloudheight> <outerbox> <innerbox>
		else if ( !Q_stricmp( token, "skyparms" ) )
		{
			ParseSkyParms( text );
			continue;
		}
		// light <value> determines flaring in q3map, not needed here
		else if ( !Q_stricmp(token, "light") )
		{
			COM_ParseExt( text, qfalse );
			continue;
		}
		// cull <face>
		else if ( !Q_stricmp( token, "cull") )
		{
			token = COM_ParseExt( text, qfalse );
			if ( token[0] == 0 )
			{
				ri->Printf( PRINT_WARNING, "WARNING: missing cull parms in shader '%s'\n", shader.name );
				continue;
			}

			if ( !Q_stricmp( token, "none" ) || !Q_stricmp( token, "twosided" ) || !Q_stricmp( token, "disable" ) )
			{
				shader.cullType = CT_TWO_SIDED;
			}
			else if ( !Q_stricmp( token, "back" ) || !Q_stricmp( token, "backside" ) || !Q_stricmp( token, "backsided" ) )
			{
				shader.cullType = CT_BACK_SIDED;
			}
			else
			{
				ri->Printf( PRINT_WARNING, "WARNING: invalid cull parm '%s' in shader '%s'\n", token, shader.name );
			}
			continue;
		}
		// sort
		else if ( !Q_stricmp( token, "sort" ) )
		{
			ParseSort( text );
			continue;
		}
		else
		{
			ri->Printf( PRINT_WARNING, "WARNING: unknown general shader parameter '%s' in '%s'\n", token, shader.name );
			return qfalse;
		}
	}

	//
	// UQ1: If we do not have any material type for this shader, try to guess as a backup (for parallax and specular settings)...
	//

	if (!shader.isSky)
		AssignMaterialType(name, *text);

	//
	// ignore shaders that don't have any stages, unless it is a sky or fog
	//
	if ( s == 0 && !shader.isSky && !(shader.contentFlags & CONTENTS_FOG ) ) {
		return qfalse;
	}

	shader.explicitlyDefined = qtrue;

	// The basejka rocket lock wedge shader uses the incorrect blending mode.
	// It only worked because the shader state was not being set, and relied
	// on previous state to be multiplied by alpha. Since fixing RB_RotatePic,
	// the shader needs to be fixed here to render correctly.
	//
	// We match against the retail version of gfx/2d/wedge by calculating the
	// hash value of the shader text, and comparing it against a precalculated
	// value.
	uint32_t shaderHash = generateHashValueForText(begin, *text - begin);
	if (shaderHash == RETAIL_ROCKET_WEDGE_SHADER_HASH &&
		 Q_stricmp(shader.name, "gfx/2d/wedge") == 0)
	{
		stages[0].stateBits &= ~(GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS);
		stages[0].stateBits |= GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
	}

	return qtrue;
}

/*
========================================================================================

SHADER OPTIMIZATION AND FOGGING

========================================================================================
*/

/*
===================
ComputeStageIteratorFunc

See if we can use on of the simple fastpath stage functions,
otherwise set to the generic stage function
===================
*/
static void ComputeStageIteratorFunc( void )
{
	shader.optimalStageIteratorFunc = RB_StageIteratorGeneric;

	//
	// see if this should go into the sky path
	//
	if ( shader.isSky )
	{
		shader.optimalStageIteratorFunc = RB_StageIteratorSky;
		return;
	}
}

/*
===================
ComputeVertexAttribs

Check which vertex attributes we only need, so we
don't need to submit/copy all of them.
===================
*/
static void ComputeVertexAttribs(void)
{
	int i, stage;

	// dlights always need ATTR_NORMAL
	shader.vertexAttribs = ATTR_POSITION | ATTR_NORMAL;

	// portals always need normals, for SurfIsOffscreen()
	if (shader.isPortal)
	{
		shader.vertexAttribs |= ATTR_NORMAL;
	}

	if (shader.defaultShader)
	{
		shader.vertexAttribs |= ATTR_TEXCOORD0;
		return;
	}

	if(shader.numDeforms)
	{
		for ( i = 0; i < shader.numDeforms; i++)
		{
			deformStage_t  *ds = &shader.deforms[i];

			switch (ds->deformation)
			{
				case DEFORM_BULGE:
					shader.vertexAttribs |= ATTR_NORMAL | ATTR_TEXCOORD0;
					break;

				case DEFORM_AUTOSPRITE:
					shader.vertexAttribs |= ATTR_NORMAL | ATTR_COLOR;
					break;

				case DEFORM_WAVE:
				case DEFORM_NORMALS:
				case DEFORM_TEXT0:
				case DEFORM_TEXT1:
				case DEFORM_TEXT2:
				case DEFORM_TEXT3:
				case DEFORM_TEXT4:
				case DEFORM_TEXT5:
				case DEFORM_TEXT6:
				case DEFORM_TEXT7:
					shader.vertexAttribs |= ATTR_NORMAL;
					break;

				default:
				case DEFORM_NONE:
				case DEFORM_MOVE:
				case DEFORM_PROJECTION_SHADOW:
				case DEFORM_AUTOSPRITE2:
					break;
			}
		}
	}

	for ( stage = 0; stage < MAX_SHADER_STAGES; stage++ )
	{
		shaderStage_t *pStage = &stages[stage];

		if ( !pStage->active )
		{
			break;
		}

		if (pStage->glslShaderGroup == tr.lightallShader)
		{
			shader.vertexAttribs |= ATTR_NORMAL;
			shader.vertexAttribs |= ATTR_TANGENT;
			shader.vertexAttribs |= ATTR_LIGHTDIRECTION;
		}

		for (i = 0; i < NUM_TEXTURE_BUNDLES; i++)
		{
			if ( pStage->bundle[i].image[0] == 0 )
			{
				continue;
			}

			switch(pStage->bundle[i].tcGen)
			{
				case TCGEN_TEXTURE:
					shader.vertexAttribs |= ATTR_TEXCOORD0;
					break;
				case TCGEN_LIGHTMAP:
				case TCGEN_LIGHTMAP1:
				case TCGEN_LIGHTMAP2:
				case TCGEN_LIGHTMAP3:
					shader.vertexAttribs |= ATTR_TEXCOORD1;
					break;
				case TCGEN_ENVIRONMENT_MAPPED:
					shader.vertexAttribs |= ATTR_NORMAL;
					break;

				default:
					break;
			}
		}

		switch(pStage->rgbGen)
		{
			case CGEN_EXACT_VERTEX:
			case CGEN_VERTEX:
			case CGEN_EXACT_VERTEX_LIT:
			case CGEN_VERTEX_LIT:
			case CGEN_ONE_MINUS_VERTEX:
				shader.vertexAttribs |= ATTR_COLOR;
				break;

			case CGEN_LIGHTING_DIFFUSE:
			case CGEN_LIGHTING_DIFFUSE_ENTITY:
				shader.vertexAttribs |= ATTR_NORMAL;
				break;

			default:
				break;
		}

		switch(pStage->alphaGen)
		{
			case AGEN_LIGHTING_SPECULAR:
				shader.vertexAttribs |= ATTR_NORMAL;
				break;

			case AGEN_VERTEX:
			case AGEN_ONE_MINUS_VERTEX:
				shader.vertexAttribs |= ATTR_COLOR;
				break;

			default:
				break;
		}
	}
}

void StripCrap( const char *in, char *out, int destsize )
{
	const char *dot = strrchr(in, '_'), *slash;
	if (dot && (!(slash = strrchr(in, '/')) || slash < dot))
		destsize = (destsize < dot-in+1 ? destsize : dot-in+1);

	if ( in == out && destsize > 1 )
		out[destsize-1] = '\0';
	else
		Q_strncpyz(out, in, destsize);
}

static void CollapseStagesToLightall(shaderStage_t *diffuse,
	shaderStage_t *normal, shaderStage_t *specular, shaderStage_t *lightmap/*, shaderStage_t *subsurface*/, shaderStage_t *overlay, shaderStage_t *steepmap, shaderStage_t *steepmap2, shaderStage_t *splatControlMap, shaderStage_t *splat1, shaderStage_t *splat2, shaderStage_t *splat3, shaderStage_t *splat4, qboolean parallax, qboolean tcgen)
{
	int defs = 0;
	qboolean hasRealNormalMap = qfalse;
	qboolean hasRealSpecularMap = qfalse;
	qboolean hasRealOverlayMap = qfalse;
	qboolean hasRealSteepMap = qfalse;
	qboolean hasRealSteepMap2 = qfalse;
	qboolean checkNormals = qtrue;

	if (shader.isPortal || shader.isSky || diffuse->glow /*|| shader.hasAlpha*/)// || shader.noTC)
		checkNormals = qfalse;

	//ri->Printf(PRINT_ALL, "shader %s has diffuse %s", shader.name, diffuse->bundle[0].image[0]->imgName);

	// reuse diffuse, mark others inactive
	diffuse->type = ST_GLSL;

	if (!diffuse->isFoliageChecked)
	{// Skip the string checks...
		switch( shader.surfaceFlags & MATERIAL_MASK )
		{// Switch to avoid doing string checks on everything else...
		case MATERIAL_SHORTGRASS:		// 5			// manicured lawn
		case MATERIAL_LONGGRASS:		// 6			// long jungle grass
		case MATERIAL_DRYLEAVES:		// 19			// dried up leaves on the floor
		case MATERIAL_GREENLEAVES:		// 20			// fresh leaves still on a tree
			if (diffuse->bundle[TB_DIFFUSEMAP].image[0]
				&& (StringContainsWord(diffuse->bundle[TB_DIFFUSEMAP].image[0]->imgName, "foliage/") 
					|| StringContainsWord(diffuse->bundle[TB_DIFFUSEMAP].image[0]->imgName, "foliages/")))
			{
				diffuse->isFoliage = true;
			}
			else if (diffuse->bundle[TB_DIFFUSEMAP].image[0]
				&& (StringContainsWord(diffuse->bundle[TB_DIFFUSEMAP].image[0]->imgName, "warzone/plant/") 
					|| StringContainsWord(diffuse->bundle[TB_DIFFUSEMAP].image[0]->imgName, "warzone/plants/")))
			{
				diffuse->isFoliage = true;
			}
			else if (diffuse->bundle[TB_DIFFUSEMAP].image[0]
				&& !StringContainsWord(diffuse->bundle[TB_DIFFUSEMAP].image[0]->imgName, "trunk")
				&& !StringContainsWord(diffuse->bundle[TB_DIFFUSEMAP].image[0]->imgName, "bark")
				&& !StringContainsWord(diffuse->bundle[TB_DIFFUSEMAP].image[0]->imgName, "giant_tree")
				&& (StringContainsWord(diffuse->bundle[TB_DIFFUSEMAP].image[0]->imgName, "yavin/tree") 
					|| StringContainsWord(diffuse->bundle[TB_DIFFUSEMAP].image[0]->imgName, "trees")
					|| StringContainsWord(diffuse->bundle[TB_DIFFUSEMAP].image[0]->imgName, "yavin/grass")
					|| StringContainsWord(diffuse->bundle[TB_DIFFUSEMAP].image[0]->imgName, "yavin/tree_leaves")
					|| StringContainsWord(diffuse->bundle[TB_DIFFUSEMAP].image[0]->imgName, "yavin/tree1")
					|| StringContainsWord(diffuse->bundle[TB_DIFFUSEMAP].image[0]->imgName, "yavin/vine")))
			{
				diffuse->isFoliage = true;
			}
			else if (diffuse->bundle[TB_DIFFUSEMAP].image[0]
				&& StringContainsWord(diffuse->bundle[TB_DIFFUSEMAP].image[0]->imgName, "warzone/deadtree")
					|| StringContainsWord(diffuse->bundle[TB_DIFFUSEMAP].image[0]->imgName, "warzone\\deadtree"))
			{
				if (!(StringContainsWord(diffuse->bundle[TB_DIFFUSEMAP].image[0]->imgName, "bark") 
					|| StringContainsWord(diffuse->bundle[TB_DIFFUSEMAP].image[0]->imgName, "trunk") 
					|| StringContainsWord(diffuse->bundle[TB_DIFFUSEMAP].image[0]->imgName, "giant_tree") 
					|| StringContainsWord(diffuse->bundle[TB_DIFFUSEMAP].image[0]->imgName, "vine01")))
					diffuse->isFoliage = true;
			}
			break;
		default:
			diffuse->isFoliage = false;
			break;
		}

		diffuse->isFoliageChecked = qtrue;
	}

	if (lightmap)
	{
		//ri->Printf(PRINT_ALL, ", lightmap");
		diffuse->bundle[TB_LIGHTMAP] = lightmap->bundle[0];
		defs |= LIGHTDEF_USE_LIGHTMAP;
	}
	else if (( shader.surfaceFlags & MATERIAL_MASK ) != MATERIAL_DRYLEAVES // billboards
		&& ( shader.surfaceFlags & MATERIAL_MASK ) != MATERIAL_GREENLEAVES // tree leaves
		&& !shader.isSky
		&& !diffuse->glow)
	{
		diffuse->bundle[TB_LIGHTMAP] = diffuse->bundle[TB_DIFFUSEMAP];
		diffuse->bundle[TB_LIGHTMAP].image[0] = tr.whiteImage;
		defs |= LIGHTDEF_USE_LIGHTMAP;
	}

	if (r_deluxeMapping->integer && tr.worldDeluxeMapping && lightmap)
	{
		//ri->Printf(PRINT_ALL, ", deluxemap");
		diffuse->bundle[TB_DELUXEMAP] = lightmap->bundle[0];
		diffuse->bundle[TB_DELUXEMAP].image[0] = tr.deluxemaps[shader.lightmapIndex[0]];
	}

	// UQ1: Can't we do all this in one stage ffs???
	if (r_normalMapping->integer >= 2 /*&& checkNormals*/)
	{
		image_t *diffuseImg = diffuse->bundle[TB_DIFFUSEMAP].image[0];

		if (diffuse->bundle[TB_NORMALMAP].image[0] && normal->bundle[TB_NORMALMAP].image[0] != tr.whiteImage)
		{
			if (diffuse->normalScale[0] == 0 && diffuse->normalScale[1] == 0 && diffuse->normalScale[2] == 0)
				VectorSet4(diffuse->normalScale, r_baseNormalX->value, r_baseNormalY->value, 1.0f, r_baseParallax->value);

			//VectorCopy4(normal->normalScale, diffuse->normalScale);

			hasRealNormalMap = qtrue;
		}
		else if (normal && normal->bundle[0].image[0] && normal->bundle[0].image[0] != tr.whiteImage)
		{
			//ri->Printf(PRINT_ALL, ", normalmap %s", normal->bundle[0].image[0]->imgName);
			diffuse->bundle[TB_NORMALMAP] = normal->bundle[0];

			VectorCopy4(normal->normalScale, diffuse->normalScale);

			diffuse->bundle[TB_DIFFUSEMAP].normalsLoaded = qtrue;
			hasRealNormalMap = qtrue;
		}
		else if (!diffuse->bundle[TB_DIFFUSEMAP].normalsLoaded)//if (lightmap || useLightVector || useLightVertex)
		{
			char normalName[MAX_QPATH];
			image_t *normalImg;
			int normalFlags = (diffuseImg->flags & ~(IMGFLAG_GENNORMALMAP | IMGFLAG_SRGB)) | IMGFLAG_NOLIGHTSCALE;

			COM_StripExtension( diffuseImg->imgName, normalName, sizeof( normalName ) );
			Q_strcat( normalName, sizeof( normalName ), "_n" );

			normalImg = R_FindImageFile(normalName, IMGTYPE_NORMAL, normalFlags);

			if (normalImg)
			{
				diffuse->bundle[TB_NORMALMAP] = diffuse->bundle[0];
				diffuse->bundle[TB_NORMALMAP].numImageAnimations = 0;
				diffuse->bundle[TB_NORMALMAP].image[0] = normalImg;

				VectorSet4(diffuse->normalScale, r_baseNormalX->value, r_baseNormalY->value, 1.0f, r_baseParallax->value);

				hasRealNormalMap = qtrue;
				diffuse->bundle[TB_DIFFUSEMAP].normalsLoaded = qtrue;
			}
			else
			{// Generate one...
				if (!diffuse->bundle[TB_DIFFUSEMAP].normalsLoaded
					&& !shader.isPortal
					&& !shader.isSky
					&& !diffuse->glow
					&& (!diffuse->bundle[TB_NORMALMAP].image[0] || diffuse->bundle[TB_NORMALMAP].image[0] == tr.whiteImage)
					&& diffuse->bundle[TB_DIFFUSEMAP].image[0]->imgName[0]
					&& diffuse->bundle[TB_DIFFUSEMAP].image[0]->imgName[0] != '*'
					&& diffuse->bundle[TB_DIFFUSEMAP].image[0]->imgName[0] != '$'
					&& diffuse->bundle[TB_DIFFUSEMAP].image[0]->imgName[0] != '_'
					&& diffuse->bundle[TB_DIFFUSEMAP].image[0]->imgName[0] != '!'
					&& !(diffuse->bundle[TB_DIFFUSEMAP].image[0]->flags & IMGFLAG_CUBEMAP)
					&& diffuse->bundle[TB_DIFFUSEMAP].image[0]->type != IMGTYPE_NORMAL
					&& diffuse->bundle[TB_DIFFUSEMAP].image[0]->type != IMGTYPE_SPECULAR
					/*&& diffuse->bundle[TB_DIFFUSEMAP].image[0]->type != IMGTYPE_SUBSURFACE*/
					&& diffuse->bundle[TB_DIFFUSEMAP].image[0]->type != IMGTYPE_OVERLAY
					&& diffuse->bundle[TB_DIFFUSEMAP].image[0]->type != IMGTYPE_STEEPMAP
					&& diffuse->bundle[TB_DIFFUSEMAP].image[0]->type != IMGTYPE_STEEPMAP2

					// gfx dirs can be exempted I guess...
					&& !(r_disableGfxDirEnhancement->integer && StringContainsWord(diffuse->bundle[TB_DIFFUSEMAP].image[0]->imgName, "gfx/")))
				{
					normalImg = R_CreateNormalMapGLSL( normalName, NULL, diffuse->bundle[TB_DIFFUSEMAP].image[0]->width, diffuse->bundle[TB_DIFFUSEMAP].image[0]->height, diffuse->bundle[TB_DIFFUSEMAP].image[0]->flags, diffuse->bundle[TB_DIFFUSEMAP].image[0] );

					if (normalImg)
					{
						diffuse->bundle[TB_NORMALMAP] = diffuse->bundle[0];
						diffuse->bundle[TB_NORMALMAP].numImageAnimations = 0;
						diffuse->bundle[TB_NORMALMAP].image[0] = normalImg;

						if (diffuse->bundle[TB_NORMALMAP].image[0] && diffuse->bundle[TB_NORMALMAP].image[0] != tr.whiteImage) diffuse->hasRealNormalMap = true;

						if (diffuse->normalScale[0] == 0 && diffuse->normalScale[1] == 0 && diffuse->normalScale[2] == 0)
							VectorSet4(diffuse->normalScale, r_baseNormalX->value, r_baseNormalY->value, 1.0f, r_baseParallax->value);
					}
				}
			}

			if (diffuse->bundle[TB_NORMALMAP].image[0] && diffuse->bundle[TB_NORMALMAP].image[0] != tr.whiteImage)
				diffuse->bundle[TB_DIFFUSEMAP].normalsLoaded = qtrue;
		}
	}

	if (r_specularMapping->integer && checkNormals)
	{
		image_t *diffuseImg = diffuse->bundle[TB_DIFFUSEMAP].image[0];

		if (diffuse->bundle[TB_SPECULARMAP].image[0])
		{// Got one...
			diffuse->bundle[TB_SPECULARMAP] = specular->bundle[0];
			if (specular) VectorCopy4(specular->specularScale, diffuse->specularScale);
			hasRealSpecularMap = qtrue;
		}
		else if (!diffuse->bundle[TB_SPECULARMAP].specularLoaded)
		{// Check if we can load one...
			char specularName[MAX_QPATH];
			char specularName2[MAX_QPATH];
			image_t *specularImg;
			int specularFlags = (diffuseImg->flags & ~(IMGFLAG_GENNORMALMAP | IMGFLAG_SRGB)) | IMGFLAG_NOLIGHTSCALE;

			COM_StripExtension( diffuseImg->imgName, specularName, sizeof( specularName ) );
			StripCrap( specularName, specularName2, sizeof(specularName));
			Q_strcat( specularName2, sizeof( specularName2 ), "_s" );

			specularImg = R_FindImageFile(specularName2, IMGTYPE_SPECULAR, specularFlags);

			if (!specularImg)
			{
				COM_StripExtension( diffuseImg->imgName, specularName, sizeof( specularName ) );
				StripCrap( specularName, specularName2, sizeof(specularName));
				Q_strcat( specularName2, sizeof( specularName2 ), "_spec" );

				specularImg = R_FindImageFile(specularName2, IMGTYPE_SPECULAR, specularFlags);
			}

			if (specularImg)
			{
				//ri->Printf(PRINT_WARNING, "+++++++++++++++ Loaded specular map %s.\n", specularName2);
				diffuse->bundle[TB_SPECULARMAP] = diffuse->bundle[0];
				diffuse->bundle[TB_SPECULARMAP].numImageAnimations = 0;
				diffuse->bundle[TB_SPECULARMAP].image[0] = specularImg;
				//if (!specular) specular = diffuse;
				if (specular) VectorCopy4(specular->specularScale, diffuse->specularScale);
				hasRealSpecularMap = qtrue;
			}

			diffuse->bundle[TB_SPECULARMAP].specularLoaded = qtrue;
		}
	}

	/*
	if (1 && checkNormals)
	{
		image_t *diffuseImg = diffuse->bundle[TB_DIFFUSEMAP].image[0];

		if (diffuse->bundle[TB_SUBSURFACEMAP].image[0] && diffuse->bundle[TB_SUBSURFACEMAP].image[0] != tr.whiteImage)
		{// Got one...
			diffuse->bundle[TB_SUBSURFACEMAP] = specular->bundle[0];
			if (subsurface) VectorCopy4(subsurface->subsurfaceExtinctionCoefficient, diffuse->subsurfaceExtinctionCoefficient);
			hasRealSubsurfaceMap = qtrue;
		}
		else if (!diffuse->bundle[TB_SUBSURFACEMAP].subsurfaceLoaded)
		{// Check if we can load one...
			char specularName[MAX_QPATH];
			char specularName2[MAX_QPATH];
			image_t *specularImg;
			int specularFlags = (diffuseImg->flags & ~(IMGFLAG_GENNORMALMAP | IMGFLAG_SRGB | IMGFLAG_CLAMPTOEDGE)) | IMGFLAG_NOLIGHTSCALE;

			COM_StripExtension( diffuseImg->imgName, specularName, sizeof( specularName ) );
			StripCrap( specularName, specularName2, sizeof(specularName));
			Q_strcat( specularName2, sizeof( specularName2 ), "_sub" );

			specularImg = R_FindImageFile(specularName2, IMGTYPE_SUBSURFACE, specularFlags);

			if (!specularImg)
			{
				COM_StripExtension( diffuseImg->imgName, specularName, sizeof( specularName ) );
				StripCrap( specularName, specularName2, sizeof(specularName));
				Q_strcat( specularName2, sizeof( specularName2 ), "_subsurface" );

				specularImg = R_FindImageFile(specularName2, IMGTYPE_SUBSURFACE, specularFlags);
			}

			if (specularImg)
			{
				diffuse->bundle[TB_SUBSURFACEMAP] = diffuse->bundle[0];
				diffuse->bundle[TB_SUBSURFACEMAP].numImageAnimations = 0;
				diffuse->bundle[TB_SUBSURFACEMAP].image[0] = specularImg;
				if (subsurface) VectorCopy4(subsurface->subsurfaceExtinctionCoefficient, diffuse->subsurfaceExtinctionCoefficient);
				hasRealSubsurfaceMap = qtrue;
			}
			else
			{
				VectorSet4(diffuse->subsurfaceExtinctionCoefficient, 0.0f, 0.0f, 0.0f, 0.0f);
				hasRealSubsurfaceMap = qfalse;
			}

			diffuse->bundle[TB_SUBSURFACEMAP].subsurfaceLoaded = qtrue;
		}
		else
		{
			hasRealSubsurfaceMap = qfalse;
		}
	}*/

	if (1 && checkNormals)
	{
		image_t *diffuseImg = diffuse->bundle[TB_DIFFUSEMAP].image[0];

		if (diffuse->bundle[TB_OVERLAYMAP].image[0] && diffuse->bundle[TB_OVERLAYMAP].image[0] != tr.whiteImage)
		{// Got one...
			diffuse->bundle[TB_OVERLAYMAP] = specular->bundle[0];
			hasRealOverlayMap = qtrue;
		}
		else if (!diffuse->bundle[TB_OVERLAYMAP].overlayLoaded)
		{// Check if we can load one...
			char specularName[MAX_QPATH];
			char specularName2[MAX_QPATH];
			image_t *specularImg;
			int specularFlags = (diffuseImg->flags & ~(IMGFLAG_GENNORMALMAP | IMGFLAG_SRGB | IMGFLAG_CLAMPTOEDGE)) /*| IMGFLAG_NOLIGHTSCALE*/;

			COM_StripExtension( diffuseImg->imgName, specularName, sizeof( specularName ) );
			StripCrap( specularName, specularName2, sizeof(specularName));
			Q_strcat( specularName2, sizeof( specularName2 ), "_o" );

			specularImg = R_FindImageFile(specularName2, IMGTYPE_OVERLAY, specularFlags);

			if (!specularImg)
			{
				COM_StripExtension( diffuseImg->imgName, specularName, sizeof( specularName ) );
				StripCrap( specularName, specularName2, sizeof(specularName));
				Q_strcat( specularName2, sizeof( specularName2 ), "_overlay" );

				specularImg = R_FindImageFile(specularName2, IMGTYPE_OVERLAY, specularFlags);
			}

			/*
			// This is a possibility, but requires more work...
			if (!specularImg && (StringContainsWord(specularName, "foliage/grass") || StringContainsWord(specularName, "foliages/sch")))
			{// Testing adding extra grass textures like this...
				specularImg = diffuseImg;
			}
			*/

			if (specularImg)
			{
				diffuse->bundle[TB_OVERLAYMAP] = diffuse->bundle[0];
				diffuse->bundle[TB_OVERLAYMAP].numImageAnimations = 0;
				diffuse->bundle[TB_OVERLAYMAP].image[0] = specularImg;
				hasRealOverlayMap = qtrue;
			}
			else
			{
				hasRealOverlayMap = qfalse;
			}

			diffuse->bundle[TB_OVERLAYMAP].overlayLoaded = qtrue;
		}
		else
		{
			hasRealOverlayMap = qfalse;
		}
	}

	if (1 && checkNormals)
	{
		image_t *diffuseImg = diffuse->bundle[TB_DIFFUSEMAP].image[0];

		if (steepmap && steepmap->bundle[0].image[0] && steepmap->bundle[0].image[0] != tr.whiteImage)
		{// Got one...
			diffuse->bundle[TB_STEEPMAP] = steepmap->bundle[0];
			hasRealSteepMap = qtrue;
		}
		else if (diffuse->bundle[TB_STEEPMAP].image[0] && diffuse->bundle[TB_STEEPMAP].image[0] != tr.whiteImage)
		{// Got one...
			hasRealSteepMap = qtrue;
		}
		else if (!diffuse->bundle[TB_STEEPMAP].steepMapLoaded)
		{// Check if we can load one...
			char specularName[MAX_QPATH];
			char specularName2[MAX_QPATH];
			image_t *specularImg;
			int specularFlags = (diffuseImg->flags & ~(IMGFLAG_GENNORMALMAP | IMGFLAG_SRGB | IMGFLAG_CLAMPTOEDGE)) /*| IMGFLAG_NOLIGHTSCALE*/;

			COM_StripExtension( diffuseImg->imgName, specularName, sizeof( specularName ) );
			StripCrap( specularName, specularName2, sizeof(specularName));
			Q_strcat( specularName2, sizeof( specularName2 ), "_steep" );

			specularImg = R_FindImageFile(specularName2, IMGTYPE_STEEPMAP, specularFlags);

			if (specularImg)
			{
				//ri->Printf(PRINT_WARNING, "+++++++++++++++ Loaded steep map %s [%i x %i].\n", specularName2, specularImg->width, specularImg->height);
				diffuse->bundle[TB_STEEPMAP] = diffuse->bundle[0];
				diffuse->bundle[TB_STEEPMAP].numImageAnimations = 0;
				diffuse->bundle[TB_STEEPMAP].image[0] = specularImg;
				hasRealSteepMap = qtrue;

				if (r_normalMapping->integer >= 2)
				{
					char imgname[64];
					sprintf(imgname, "%s_n", diffuse->bundle[TB_STEEPMAP].image[0]->imgName);
					diffuse->bundle[TB_NORMALMAP2].image[0] = R_CreateNormalMapGLSL( imgname, NULL, diffuse->bundle[TB_STEEPMAP].image[0]->width, diffuse->bundle[TB_STEEPMAP].image[0]->height, diffuse->bundle[TB_STEEPMAP].image[0]->flags, diffuse->bundle[TB_STEEPMAP].image[0] );
				}
			}
			else
			{
				hasRealSteepMap = qfalse;
			}

			diffuse->bundle[TB_STEEPMAP].steepMapLoaded = qtrue;
		}
		else
		{
			hasRealSteepMap = qfalse;
		}
	}

	if (1 && checkNormals)
	{
		image_t *diffuseImg = diffuse->bundle[TB_DIFFUSEMAP].image[0];

		if (steepmap2 && steepmap2->bundle[0].image[0] && steepmap2->bundle[0].image[0] != tr.whiteImage)
		{// Got one...
			diffuse->bundle[TB_STEEPMAP2] = steepmap2->bundle[0];
			hasRealSteepMap2 = qtrue;
		}
		else if (diffuse->bundle[TB_STEEPMAP2].image[0] && diffuse->bundle[TB_STEEPMAP2].image[0] != tr.whiteImage)
		{// Got one...
			hasRealSteepMap2 = qtrue;
		}
		else if (!diffuse->bundle[TB_STEEPMAP2].steepMapLoaded2)
		{// Check if we can load one...
			char specularName[MAX_QPATH];
			char specularName2[MAX_QPATH];
			image_t *specularImg;
			int specularFlags = (diffuseImg->flags & ~(IMGFLAG_GENNORMALMAP | IMGFLAG_SRGB | IMGFLAG_CLAMPTOEDGE)) /*| IMGFLAG_NOLIGHTSCALE*/;

			COM_StripExtension( diffuseImg->imgName, specularName, sizeof( specularName ) );
			StripCrap( specularName, specularName2, sizeof(specularName));
			Q_strcat( specularName2, sizeof( specularName2 ), "_steep2" );

			specularImg = R_FindImageFile(specularName2, IMGTYPE_STEEPMAP2, specularFlags);

			if (specularImg)
			{
				//ri->Printf(PRINT_WARNING, "+++++++++++++++ Loaded steep map2 %s [%i x %i].\n", specularName2, specularImg->width, specularImg->height);
				diffuse->bundle[TB_STEEPMAP2] = diffuse->bundle[0];
				diffuse->bundle[TB_STEEPMAP2].numImageAnimations = 0;
				diffuse->bundle[TB_STEEPMAP2].image[0] = specularImg;
				hasRealSteepMap2 = qtrue;

				if (r_normalMapping->integer >= 2)
				{
					char imgname[64];
					sprintf(imgname, "%s_n", diffuse->bundle[TB_STEEPMAP2].image[0]->imgName);
					diffuse->bundle[TB_NORMALMAP3].image[0] = R_CreateNormalMapGLSL( imgname, NULL, diffuse->bundle[TB_STEEPMAP2].image[0]->width, diffuse->bundle[TB_STEEPMAP2].image[0]->height, diffuse->bundle[TB_STEEPMAP2].image[0]->flags, diffuse->bundle[TB_STEEPMAP2].image[0] );
				}
			}
			else
			{
				hasRealSteepMap2 = qfalse;
			}

			diffuse->bundle[TB_STEEPMAP2].steepMapLoaded2 = qtrue;
		}
		else
		{
			hasRealSteepMap2 = qfalse;
		}
	}

	if (1)
	{
		if (splatControlMap && splatControlMap->bundle[0].image[0] && splatControlMap->bundle[0].image[0] != tr.whiteImage)
		{// Got one...
			diffuse->bundle[TB_SPLATCONTROLMAP] = splatControlMap->bundle[0];
		}
		else
		{
			// Splat Control Map - We will allow each shader to have it's own map-wide spatter control image...
			image_t *diffuseImg = diffuse->bundle[TB_DIFFUSEMAP].image[0];

			char splatName[MAX_QPATH];
			char splatName2[MAX_QPATH];
			image_t *splatImg;
			int specularFlags = (diffuseImg->flags & ~(IMGFLAG_GENNORMALMAP | IMGFLAG_SRGB | IMGFLAG_CLAMPTOEDGE)) /*| IMGFLAG_NOLIGHTSCALE*/;

			COM_StripExtension( diffuseImg->imgName, splatName, sizeof( splatName ) );
			StripCrap( splatName, splatName2, sizeof(splatName));
			Q_strcat( splatName2, sizeof( splatName2 ), "_splat" );

			splatImg = R_FindImageFile(splatName2, IMGTYPE_SPLATCONTROLMAP, specularFlags);

			if (splatImg)
			{
				//ri->Printf(PRINT_WARNING, "+++++++++++++++ Loaded splat control map %s [%i x %i].\n", splatName2, splatImg->width, splatImg->height);
				diffuse->bundle[TB_SPLATCONTROLMAP] = diffuse->bundle[0];
				diffuse->bundle[TB_SPLATCONTROLMAP].numImageAnimations = 0;
				diffuse->bundle[TB_SPLATCONTROLMAP].image[0] = splatImg;

				//char imgname[64];
				//sprintf(imgname, "%s_n", diffuse->bundle[TB_SPLATCONTROLMAP].image[0]->imgName);
				//diffuse->bundle[TB_NORMALMAP3].image[0] = R_CreateNormalMapGLSL( imgname, NULL, diffuse->bundle[TB_SPLATCONTROLMAP].image[0]->width, diffuse->bundle[TB_SPLATCONTROLMAP].image[0]->height, diffuse->bundle[TB_SPLATCONTROLMAP].image[0]->flags, diffuse->bundle[TB_SPLATCONTROLMAP].image[0] );
			}
			else
			{
				diffuse->bundle[TB_SPLATCONTROLMAP] = diffuse->bundle[0];
				diffuse->bundle[TB_SPLATCONTROLMAP].numImageAnimations = 0;
				diffuse->bundle[TB_SPLATCONTROLMAP].image[0] = NULL;
			}
		}

		if (splat1 && splat1->bundle[0].image[0] && splat1->bundle[0].image[0] != tr.whiteImage)
		{// Got one...
			diffuse->bundle[TB_SPLATMAP1] = splat1->bundle[0];
		}
		else
		{
			// Splat Map #1
			image_t *diffuseImg = diffuse->bundle[TB_DIFFUSEMAP].image[0];

			char splatName[MAX_QPATH];
			char splatName2[MAX_QPATH];
			image_t *splatImg;
			int specularFlags = (diffuseImg->flags & ~(IMGFLAG_GENNORMALMAP | IMGFLAG_SRGB | IMGFLAG_CLAMPTOEDGE)) /*| IMGFLAG_NOLIGHTSCALE*/;

			COM_StripExtension( diffuseImg->imgName, splatName, sizeof( splatName ) );
			StripCrap( splatName, splatName2, sizeof(splatName));
			Q_strcat( splatName2, sizeof( splatName2 ), "_splat1" );

			splatImg = R_FindImageFile(splatName2, IMGTYPE_SPLATMAP1, specularFlags);

			if (splatImg)
			{
				//ri->Printf(PRINT_WARNING, "+++++++++++++++ Loaded splat map1 %s [%i x %i].\n", splatName2, splatImg->width, splatImg->height);
				diffuse->bundle[TB_SPLATMAP1] = diffuse->bundle[0];
				diffuse->bundle[TB_SPLATMAP1].numImageAnimations = 0;
				diffuse->bundle[TB_SPLATMAP1].image[0] = splatImg;

				if (r_normalMapping->integer >= 2)
				{
					// Generate normal and height map for it as well...
					char imgname[64];
					sprintf(imgname, "%s_n", diffuse->bundle[TB_SPLATMAP1].image[0]->imgName);
					image_t *splatNormalImg = R_CreateNormalMapGLSL( imgname, NULL, splatImg->width, splatImg->height, splatImg->flags, splatImg );
					diffuse->bundle[TB_SPLATMAP1].image[1] = splatNormalImg;
				}
			}
			else
			{
				diffuse->bundle[TB_SPLATMAP1] = diffuse->bundle[0];
				diffuse->bundle[TB_SPLATMAP1].numImageAnimations = 0;
				diffuse->bundle[TB_SPLATMAP1].image[0] = NULL;
				diffuse->bundle[TB_SPLATMAP1].image[1] = NULL;
			}
		}

		if (splat2 && splat2->bundle[0].image[0] && splat2->bundle[0].image[0] != tr.whiteImage)
		{// Got one...
			diffuse->bundle[TB_SPLATMAP2] = splat2->bundle[0];
		}
		else
		{
			// Splat Map #2
			image_t *diffuseImg = diffuse->bundle[TB_DIFFUSEMAP].image[0];

			char splatName[MAX_QPATH];
			char splatName2[MAX_QPATH];
			image_t *splatImg;
			int specularFlags = (diffuseImg->flags & ~(IMGFLAG_GENNORMALMAP | IMGFLAG_SRGB | IMGFLAG_CLAMPTOEDGE)) /*| IMGFLAG_NOLIGHTSCALE*/;

			COM_StripExtension( diffuseImg->imgName, splatName, sizeof( splatName ) );
			StripCrap( splatName, splatName2, sizeof(splatName));
			Q_strcat( splatName2, sizeof( splatName2 ), "_splat2" );

			splatImg = R_FindImageFile(splatName2, IMGTYPE_SPLATMAP2, specularFlags);

			if (splatImg)
			{
				//ri->Printf(PRINT_WARNING, "+++++++++++++++ Loaded splat map2 %s [%i x %i].\n", splatName2, splatImg->width, splatImg->height);
				diffuse->bundle[TB_SPLATMAP2] = diffuse->bundle[0];
				diffuse->bundle[TB_SPLATMAP2].numImageAnimations = 0;
				diffuse->bundle[TB_SPLATMAP2].image[0] = splatImg;

				if (r_normalMapping->integer >= 2)
				{
					// Generate normal and height map for it as well...
					char imgname[64];
					sprintf(imgname, "%s_n", diffuse->bundle[TB_SPLATMAP2].image[0]->imgName);
					image_t *splatNormalImg = R_CreateNormalMapGLSL( imgname, NULL, splatImg->width, splatImg->height, splatImg->flags, splatImg );
					diffuse->bundle[TB_SPLATMAP2].image[1] = splatNormalImg;
				}
			}
			else
			{
				diffuse->bundle[TB_SPLATMAP2] = diffuse->bundle[0];
				diffuse->bundle[TB_SPLATMAP2].numImageAnimations = 0;
				diffuse->bundle[TB_SPLATMAP2].image[0] = NULL;
				diffuse->bundle[TB_SPLATMAP2].image[1] = NULL;
			}
		}

		if (splat3 && splat3->bundle[0].image[0] && splat3->bundle[0].image[0] != tr.whiteImage)
		{// Got one...
			diffuse->bundle[TB_SPLATMAP3] = splat3->bundle[0];
		}
		else
		{
			// Splat Map #3
			image_t *diffuseImg = diffuse->bundle[TB_DIFFUSEMAP].image[0];

			char splatName[MAX_QPATH];
			char splatName2[MAX_QPATH];
			image_t *splatImg;
			int specularFlags = (diffuseImg->flags & ~(IMGFLAG_GENNORMALMAP | IMGFLAG_SRGB | IMGFLAG_CLAMPTOEDGE)) /*| IMGFLAG_NOLIGHTSCALE*/;

			COM_StripExtension( diffuseImg->imgName, splatName, sizeof( splatName ) );
			StripCrap( splatName, splatName2, sizeof(splatName));
			Q_strcat( splatName2, sizeof( splatName2 ), "_splat3" );

			splatImg = R_FindImageFile(splatName2, IMGTYPE_SPLATMAP3, specularFlags);

			if (splatImg)
			{
				//ri->Printf(PRINT_WARNING, "+++++++++++++++ Loaded splat map3 %s [%i x %i].\n", splatName2, splatImg->width, splatImg->height);
				diffuse->bundle[TB_SPLATMAP3] = diffuse->bundle[0];
				diffuse->bundle[TB_SPLATMAP3].numImageAnimations = 0;
				diffuse->bundle[TB_SPLATMAP3].image[0] = splatImg;

				if (r_normalMapping->integer >= 2)
				{
					// Generate normal and height map for it as well...
					char imgname[64];
					sprintf(imgname, "%s_n", diffuse->bundle[TB_SPLATMAP3].image[0]->imgName);
					image_t *splatNormalImg = R_CreateNormalMapGLSL( imgname, NULL, splatImg->width, splatImg->height, splatImg->flags, splatImg );
					diffuse->bundle[TB_SPLATMAP3].image[1] = splatNormalImg;
				}
			}
			else
			{
				diffuse->bundle[TB_SPLATMAP3] = diffuse->bundle[0];
				diffuse->bundle[TB_SPLATMAP3].numImageAnimations = 0;
				diffuse->bundle[TB_SPLATMAP3].image[0] = NULL;
				diffuse->bundle[TB_SPLATMAP3].image[1] = NULL;
			}
		}

		if (splat4 && splat4->bundle[0].image[0] && splat4->bundle[0].image[0] != tr.whiteImage)
		{// Got one...
			diffuse->bundle[TB_SPLATMAP4] = splat4->bundle[0];
		}
		else
		{
			// Splat Map #4
			image_t *diffuseImg = diffuse->bundle[TB_DIFFUSEMAP].image[0];

			char splatName[MAX_QPATH];
			char splatName2[MAX_QPATH];
			image_t *splatImg;
			int specularFlags = (diffuseImg->flags & ~(IMGFLAG_GENNORMALMAP | IMGFLAG_SRGB | IMGFLAG_CLAMPTOEDGE)) /*| IMGFLAG_NOLIGHTSCALE*/;

			COM_StripExtension( diffuseImg->imgName, splatName, sizeof( splatName ) );
			StripCrap( splatName, splatName2, sizeof(splatName));
			Q_strcat( splatName2, sizeof( splatName2 ), "_splat4" );

			splatImg = R_FindImageFile(splatName2, IMGTYPE_SPLATMAP4, specularFlags);

			if (splatImg)
			{
				//ri->Printf(PRINT_WARNING, "+++++++++++++++ Loaded splat map4 %s [%i x %i].\n", splatName2, splatImg->width, splatImg->height);
				diffuse->bundle[TB_SPLATMAP4] = diffuse->bundle[0];
				diffuse->bundle[TB_SPLATMAP4].numImageAnimations = 0;
				diffuse->bundle[TB_SPLATMAP4].image[0] = splatImg;

				if (r_normalMapping->integer >= 2)
				{
					// Generate normal and height map for it as well...
					char imgname[64];
					sprintf(imgname, "%s_n", diffuse->bundle[TB_SPLATMAP4].image[0]->imgName);
					image_t *splatNormalImg = R_CreateNormalMapGLSL( imgname, NULL, splatImg->width, splatImg->height, splatImg->flags, splatImg );
					diffuse->bundle[TB_SPLATMAP4].image[1] = splatNormalImg;
				}
			}
			else
			{
				diffuse->bundle[TB_SPLATMAP4] = diffuse->bundle[0];
				diffuse->bundle[TB_SPLATMAP4].numImageAnimations = 0;
				diffuse->bundle[TB_SPLATMAP4].image[0] = NULL;
				diffuse->bundle[TB_SPLATMAP4].image[1] = NULL;
			}
		}
	}

	if (tcgen || diffuse->bundle[0].numTexMods)
	{
		defs |= LIGHTDEF_USE_TCGEN_AND_TCMOD;
	}

	if (diffuse->glow)
		defs |= LIGHTDEF_USE_GLOW_BUFFER;

	//ri->Printf(PRINT_ALL, ".\n");

	if (hasRealNormalMap)
	{
		diffuse->hasRealNormalMap = true;
	}
	else
	{
		diffuse->hasRealNormalMap = false;
	}

	if (hasRealSpecularMap)
	{
		diffuse->hasSpecular = true;
	}
	else
	{
		diffuse->hasSpecular = false;
	}

	/*if (hasRealSubsurfaceMap)
	{
		diffuse->hasRealSubsurfaceMap = true;
	}
	else
	{
		diffuse->hasRealSubsurfaceMap = false;
	}*/

	if (hasRealOverlayMap)
	{
		diffuse->hasRealOverlayMap = true;
	}
	else
	{
		diffuse->hasRealOverlayMap = false;
	}

	if (hasRealSteepMap)
	{
		diffuse->hasRealSteepMap = true;
	}
	else
	{
		diffuse->hasRealSteepMap = false;
	}

	if (hasRealSteepMap2)
	{
		diffuse->hasRealSteepMap2 = true;
	}
	else
	{
		diffuse->hasRealSteepMap2 = false;
	}

	diffuse->glslShaderGroup = tr.lightallShader;
	diffuse->glslShaderIndex = defs;
}

static qboolean CollapseStagesToGLSL(void)
{
	int i, j, numStages;
	qboolean skip = qfalse;
	qboolean hasRealNormalMap = qfalse;
	qboolean hasRealSpecularMap = qfalse;
	//qboolean hasRealSubsurfaceMap = qfalse;
	qboolean hasRealOverlayMap = qfalse;
	qboolean hasRealSteepMap = qfalse;
	qboolean hasRealSteepMap2 = qfalse;

	//ri->Printf (PRINT_DEVELOPER, "Collapsing stages for shader '%s'\n", shader.name);

	// skip shaders with deforms
	if (shader.numDeforms != 0)
	{
		skip = qtrue;
		//ri->Printf (PRINT_DEVELOPER, "> Shader has vertex deformations. Aborting stage collapsing\n");
	}

	//ri->Printf (PRINT_DEVELOPER, "> Original shader stage order:\n");

/*
	for ( int i = 0; i < MAX_SHADER_STAGES; i++ )
	{
		shaderStage_t *stage = &stages[i];

		if ( !stage->active )
		{
			continue;
		}

		ri->Printf (PRINT_DEVELOPER, "-> %s\n", stage->bundle[0].image[0]->imgName);
	}
*/

	if (!skip)
	{
		// if 2+ stages and first stage is lightmap, switch them
		// this makes it easier for the later bits to process
		if (stages[0].active &&
			stages[0].bundle[0].tcGen >= TCGEN_LIGHTMAP &&
			stages[0].bundle[0].tcGen <= TCGEN_LIGHTMAP3 &&
			stages[1].active)
		{
			int blendBits = stages[1].stateBits & ( GLS_DSTBLEND_BITS | GLS_SRCBLEND_BITS );

			if (blendBits == (GLS_DSTBLEND_SRC_COLOR | GLS_SRCBLEND_ZERO)
				|| blendBits == (GLS_DSTBLEND_ZERO | GLS_SRCBLEND_DST_COLOR))
			{
				int stateBits0 = stages[0].stateBits;
				int stateBits1 = stages[1].stateBits;
				shaderStage_t swapStage;

				swapStage = stages[0];
				stages[0] = stages[1];
				stages[1] = swapStage;

				stages[0].stateBits = stateBits0;
				stages[1].stateBits = stateBits1;

				//ri->Printf (PRINT_DEVELOPER, "> Swapped first and second stage.\n");
				//ri->Printf (PRINT_DEVELOPER, "-> First stage is now: %s\n", stages[0].bundle[0].image[0]->imgName);
				//ri->Printf (PRINT_DEVELOPER, "-> Second stage is now: %s\n", stages[1].bundle[0].image[0]->imgName);
			}
		}
	}

	if (!skip)
	{
		// scan for shaders that aren't supported
		for (i = 0; i < MAX_SHADER_STAGES; i++)
		{
			shaderStage_t *pStage = &stages[i];

			if (!pStage->active)
				continue;

			if (pStage->adjustColorsForFog)
			{
				skip = qtrue;
				break;
			}

			if (pStage->bundle[0].tcGen >= TCGEN_LIGHTMAP &&
				pStage->bundle[0].tcGen <= TCGEN_LIGHTMAP3)
			{
				int blendBits = pStage->stateBits & ( GLS_DSTBLEND_BITS | GLS_SRCBLEND_BITS );

				if (blendBits != (GLS_DSTBLEND_SRC_COLOR | GLS_SRCBLEND_ZERO)
					&& blendBits != (GLS_DSTBLEND_ZERO | GLS_SRCBLEND_DST_COLOR))
				{
					skip = qtrue;
					break;
				}
			}

			switch(pStage->bundle[0].tcGen)
			{
				case TCGEN_TEXTURE:
				case TCGEN_LIGHTMAP:
				case TCGEN_LIGHTMAP1:
				case TCGEN_LIGHTMAP2:
				case TCGEN_LIGHTMAP3:
				case TCGEN_ENVIRONMENT_MAPPED:
				case TCGEN_VECTOR:
					break;
				default:
#ifndef __EXTRA_PRETTY__
					skip = qtrue;
#endif //__EXTRA_PRETTY__
					break;
			}

			switch(pStage->alphaGen)
			{
#ifndef __EXTRA_PRETTY__
				case AGEN_LIGHTING_SPECULAR:
#endif //__EXTRA_PRETTY__
				case AGEN_PORTAL:
					skip = qtrue;
					break;
				default:
					break;
			}
		}
	}

	if (!skip)
	{
		shaderStage_t *lightmaps[MAX_SHADER_STAGES] = {};

		for (i = 0; i < MAX_SHADER_STAGES; i++)
		{
			shaderStage_t *pStage = &stages[i];

			if (!pStage->active)
				continue;

			if (pStage->bundle[0].tcGen >= TCGEN_LIGHTMAP && pStage->bundle[0].tcGen <= TCGEN_LIGHTMAP3)
			{
				lightmaps[i] = pStage;
			}
		}

		for (i = 0; i < MAX_SHADER_STAGES; i++)
		{
			shaderStage_t *pStage = &stages[i];
			shaderStage_t *diffuse, *normal, *specular, *lightmap/*, *subsurface*/, *overlay, *steep, *steep2, *splatControl, *splat1, *splat2, *splat3, *splat4;
			qboolean parallax, tcgen;

			if (!pStage->active)
				continue;

			// skip normal and specular maps
			if (pStage->type != ST_COLORMAP)
				continue;

			// skip lightmaps
			if (pStage->bundle[0].tcGen >= TCGEN_LIGHTMAP && pStage->bundle[0].tcGen <= TCGEN_LIGHTMAP3)
				continue;

			diffuse  = pStage;
			normal   = NULL;
			parallax = qfalse;
			specular = NULL;
			lightmap = NULL;
			//subsurface = NULL;
			overlay = NULL;
			steep = NULL;
			steep2 = NULL;
			splatControl = NULL;
			splat1 = NULL;
			splat2 = NULL;
			splat3 = NULL;
			splat4 = NULL;


			// we have a diffuse map, find matching normal, specular, and lightmap
			for (j = i + 1; j < MAX_SHADER_STAGES; j++)
			{
				shaderStage_t *pStage2 = &stages[j];

				if (!pStage2->active)
					continue;

				if (pStage2->glow)
					continue;

				switch(pStage2->type)
				{
					case ST_NORMALMAP:
						if (!normal)
						{
							hasRealNormalMap = qtrue;
							normal = pStage2;
						}
						break;

					case ST_NORMALPARALLAXMAP:
						if (!normal)
						{
							hasRealNormalMap = qtrue;
							normal = pStage2;
							parallax = qtrue;
						}
						break;

					case ST_SPECULARMAP:
						if (!specular)
						{
							hasRealSpecularMap = qtrue;
							specular = pStage2;
						}
						break;

					/*case ST_SUBSURFACEMAP:
						if (!subsurface)
						{
							hasRealSubsurfaceMap = qtrue;
							subsurface = pStage2;
						}
						break;*/

					case ST_OVERLAYMAP:
						if (!overlay)
						{
							hasRealOverlayMap = qtrue;
							overlay = pStage2;
						}
						break;

					case ST_STEEPMAP:
						if (!steep)
						{
							hasRealSteepMap = qtrue;
							steep = pStage2;
						}
						break;

					case ST_STEEPMAP2:
						if (!steep2)
						{
							hasRealSteepMap2 = qtrue;
							steep2 = pStage2;
						}
						break;

					case ST_SPLATCONTROLMAP:
						{
							splatControl = pStage2;
						}

					case ST_SPLATMAP1:
						{
							splat1 = pStage2;
						}

					case ST_SPLATMAP2:
						{
							splat2 = pStage2;
						}

					case ST_SPLATMAP3:
						{
							splat3 = pStage2;
						}

					case ST_SPLATMAP4:
						{
							splat4 = pStage2;
						}

					case ST_COLORMAP:
						if (pStage2->bundle[0].tcGen >= TCGEN_LIGHTMAP &&
							pStage2->bundle[0].tcGen <= TCGEN_LIGHTMAP3 &&
							pStage2->rgbGen != CGEN_EXACT_VERTEX)
						{
							//ri->Printf (PRINT_DEVELOPER, "> Setting lightmap for %s to %s\n", pStage->bundle[0].image[0]->imgName, pStage2->bundle[0].image[0]->imgName);
							lightmap = pStage2;
							lightmaps[j] = NULL;
						}
						break;

					default:
						break;
				}
			}

			tcgen = qfalse;
			if (diffuse->bundle[0].tcGen == TCGEN_ENVIRONMENT_MAPPED
			    || (diffuse->bundle[0].tcGen >= TCGEN_LIGHTMAP && diffuse->bundle[0].tcGen <= TCGEN_LIGHTMAP3)
			    || diffuse->bundle[0].tcGen == TCGEN_VECTOR)
			{
				tcgen = qtrue;
			}

			CollapseStagesToLightall(diffuse, normal, specular, lightmap/*, subsurface*/, overlay, steep, steep2, splatControl, splat1, splat2, splat3, splat4, parallax, tcgen);
		}

		// deactivate lightmap stages
		for (i = 0; i < MAX_SHADER_STAGES; i++)
		{
			shaderStage_t *pStage = &stages[i];

			if (!pStage->active)
				continue;

			if (pStage->bundle[0].tcGen >= TCGEN_LIGHTMAP &&
				pStage->bundle[0].tcGen <= TCGEN_LIGHTMAP3 &&
				lightmaps[i] == NULL)
			{
				pStage->active = qfalse;
			}
		}
	}

	// deactivate normal and specular stages
	for (i = 0; i < MAX_SHADER_STAGES; i++)
	{
		shaderStage_t *pStage = &stages[i];

		if (!pStage->active)
			continue;

		if (pStage->type == ST_NORMALMAP)
		{
			hasRealNormalMap = qfalse;
			pStage->active = qfalse;
		}

		if (pStage->type == ST_NORMALPARALLAXMAP)
		{
			hasRealNormalMap = qfalse;
			pStage->active = qfalse;
		}

		if (pStage->type == ST_SPECULARMAP)
		{
			hasRealSpecularMap = qfalse;
			pStage->active = qfalse;
		}

		/*if (pStage->type == ST_SUBSURFACEMAP)
		{
			hasRealSubsurfaceMap = qfalse;
			pStage->active = qfalse;
		}*/

		if (pStage->type == ST_OVERLAYMAP)
		{
			hasRealOverlayMap = qfalse;
			pStage->active = qfalse;
		}

		if (pStage->type == ST_STEEPMAP)
		{
			hasRealSteepMap = qfalse;
			pStage->active = qfalse;
		}

		if (pStage->type == ST_STEEPMAP2)
		{
			hasRealSteepMap2 = qfalse;
			pStage->active = qfalse;
		}

		if (pStage->type == ST_SPLATCONTROLMAP)
		{
			pStage->active = qfalse;
		}

		if (pStage->type == ST_SPLATMAP1)
		{
			pStage->active = qfalse;
		}

		if (pStage->type == ST_SPLATMAP2)
		{
			pStage->active = qfalse;
		}

		if (pStage->type == ST_SPLATMAP3)
		{
			pStage->active = qfalse;
		}

		if (pStage->type == ST_SPLATMAP4)
		{
			pStage->active = qfalse;
		}
	}

	// remove inactive stages
	numStages = 0;
	for (i = 0; i < MAX_SHADER_STAGES; i++)
	{
		if (!stages[i].active)
			continue;

		if (i == numStages)
		{
			numStages++;
			continue;
		}

		stages[numStages] = stages[i];
		stages[i].active = qfalse;
		numStages++;
	}

	// convert any remaining lightmap stages to a lighting pass with a white texture
	// only do this with r_sunlightMode non-zero, as it's only for correct shadows.
	if (r_sunlightMode->integer && shader.numDeforms == 0)
	{
		for (i = 0; i < MAX_SHADER_STAGES; i++)
		{
			shaderStage_t *pStage = &stages[i];

			if (!pStage->active)
				continue;

			if (pStage->adjustColorsForFog)
				continue;

			if (pStage->bundle[TB_DIFFUSEMAP].tcGen >= TCGEN_LIGHTMAP && pStage->bundle[TB_DIFFUSEMAP].tcGen <= TCGEN_LIGHTMAP3)
			{
				if (hasRealNormalMap) pStage->hasRealNormalMap = true;

				pStage->glslShaderGroup = tr.lightallShader;

				pStage->glslShaderIndex = LIGHTDEF_USE_LIGHTMAP;
				pStage->bundle[TB_LIGHTMAP] = pStage->bundle[TB_DIFFUSEMAP];
				pStage->bundle[TB_DIFFUSEMAP].image[0] = tr.whiteImage;
				pStage->bundle[TB_DIFFUSEMAP].isLightmap = qfalse;
				pStage->bundle[TB_DIFFUSEMAP].tcGen = TCGEN_TEXTURE;
			}
		}
	}

	// convert any remaining lightingdiffuse stages to a lighting pass
	if (shader.numDeforms == 0)
	{
		for (i = 0; i < MAX_SHADER_STAGES; i++)
		{
			shaderStage_t *pStage = &stages[i];

			if (!pStage->active)
				continue;

			if (pStage->adjustColorsForFog)
				continue;

			if (pStage->rgbGen == CGEN_LIGHTING_DIFFUSE ||
				pStage->rgbGen == CGEN_LIGHTING_DIFFUSE_ENTITY)
			{
				if (pStage->glslShaderGroup != tr.lightallShader)
				{
					if (hasRealNormalMap) pStage->hasRealNormalMap = true;

					pStage->glslShaderGroup = tr.lightallShader;
				}

				if (pStage->bundle[0].tcGen != TCGEN_TEXTURE || pStage->bundle[0].numTexMods != 0)
					pStage->glslShaderIndex |= LIGHTDEF_USE_TCGEN_AND_TCMOD;
			}
		}
	}

	//ri->Printf (PRINT_DEVELOPER, "> New shader stage order:\n");

	for ( int i = 0; i < MAX_SHADER_STAGES; i++ )
	{
		shaderStage_t *stage = &stages[i];

		if ( !stage->active )
		{
			continue;
		}

		if (hasRealNormalMap)
		{
			stage->hasRealNormalMap = true;
		}

		if (hasRealSpecularMap)
		{
			stage->hasSpecular = true;
		}

		/*if (hasRealSubsurfaceMap)
		{
			stage->hasRealSubsurfaceMap = true;
		}*/

		if (hasRealOverlayMap)
		{
			stage->hasRealOverlayMap = true;
		}

		if (hasRealSteepMap)
		{
			stage->hasRealSteepMap = true;
		}

		if (hasRealSteepMap2)
		{
			stage->hasRealSteepMap2 = true;
		}

		//ri->Printf (PRINT_DEVELOPER, "-> %s\n", stage->bundle[0].image[0]->imgName);
	}

#if 0
	if (numStages > 1)
	{
		ri->Printf(PRINT_WARNING, "Shader %s has %i stages.\n", shader.name, numStages);

		for (i = 0; i < MAX_SHADER_STAGES; i++)
		{
			shaderStage_t *pStage = &stages[i];

			if (!pStage->active)
				continue;

			if (pStage->type == ST_DIFFUSEMAP)
			{
				ri->Printf(PRINT_WARNING, "     Stage %i is DiffuseMap.", i);
			}
			else if (pStage->type == ST_NORMALMAP)
			{
				ri->Printf(PRINT_WARNING, "     Stage %i is NormalMap.", i);
			}
			else if (pStage->type == ST_NORMALPARALLAXMAP)
			{
				ri->Printf(PRINT_WARNING, "     Stage %i is NormalParallaxMap.", i);
			}
			else if (pStage->type == ST_SPECULARMAP)
			{
				ri->Printf(PRINT_WARNING, "     Stage %i is SpecularMap.", i);
			}
			/*else if (pStage->type == ST_SUBSURFACEMAP)
			{
				ri->Printf(PRINT_WARNING, "     Stage %i is SubsurfaceMap.", i);
			}*/
			else if (pStage->type == ST_OVERLAYMAP)
			{
				ri->Printf(PRINT_WARNING, "     Stage %i is OverlayMap.", i);
			}
			else if (pStage->type == ST_STEEPMAP)
			{
				ri->Printf(PRINT_WARNING, "     Stage %i is SteepMap.", i);
			}
			else if (pStage->type == ST_STEEPMAP2)
			{
				ri->Printf(PRINT_WARNING, "     Stage %i is SteepMap2.", i);
			}
			else if (pStage->type == ST_SPLATCONTROLMAP)
			{
				ri->Printf(PRINT_WARNING, "     Stage %i is SplatControlMap.", i);
			}
			else if (pStage->type == ST_SPLATMAP1)
			{
				ri->Printf(PRINT_WARNING, "     Stage %i is SplatMap1.", i);
			}
			else if (pStage->type == ST_SPLATMAP2)
			{
				ri->Printf(PRINT_WARNING, "     Stage %i is SplatMap2.", i);
			}
			else if (pStage->type == ST_SPLATMAP3)
			{
				ri->Printf(PRINT_WARNING, "     Stage %i is SplatMap3.", i);
			}
			else if (pStage->type == ST_SPLATMAP4)
			{
				ri->Printf(PRINT_WARNING, "     Stage %i is SplatMap4.", i);
			}
			else if (pStage->type == ST_GLSL)
			{
				ri->Printf(PRINT_WARNING, "     Stage %i is GLSL.", i);
			}
			else
			{
				ri->Printf(PRINT_WARNING, "     Stage %i is %i.", i, pStage->type);
			}

			if (pStage->glow)
				ri->Printf(PRINT_WARNING, " [ glow ]\n");
			else
				ri->Printf(PRINT_WARNING, "\n");
		}
	}
#endif

	return (qboolean)numStages;
}

/*
=============

FixRenderCommandList
https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=493
Arnout: this is a nasty issue. Shaders can be registered after drawsurfaces are generated
but before the frame is rendered. This will, for the duration of one frame, cause drawsurfaces
to be rendered with bad shaders. To fix this, need to go through all render commands and fix
sortedIndex.
==============
*/
extern bool gServerSkinHack;
static void FixRenderCommandList( int newShader ) {
	if( !gServerSkinHack ) {
		renderCommandList_t	*cmdList = &backEndData->commands;

		if( cmdList ) {
			const void *curCmd = cmdList->cmds;

			while ( 1 ) {
				curCmd = PADP(curCmd, sizeof(void *));

				switch ( *(const int *)curCmd ) {
				case RC_SET_COLOR:
					{
					const setColorCommand_t *sc_cmd = (const setColorCommand_t *)curCmd;
					curCmd = (const void *)(sc_cmd + 1);
					break;
					}
				case RC_STRETCH_PIC:
					{
					const stretchPicCommand_t *sp_cmd = (const stretchPicCommand_t *)curCmd;
					curCmd = (const void *)(sp_cmd + 1);
					break;
					}
				case RC_ROTATE_PIC:
				case RC_ROTATE_PIC2:
					{
						const rotatePicCommand_t *sp_cmd = (const rotatePicCommand_t *)curCmd;
						curCmd = (const void *)(sp_cmd + 1);
						break;
					}
				case RC_DRAW_SURFS:
					{
					int i;
					drawSurf_t	*drawSurf;
					shader_t	*shader;
					int64_t		fogNum;
					int64_t		entityNum;
//					int64_t		dlightMap;
					int64_t		sortedIndex;
					int64_t		postRender;
					const drawSurfsCommand_t *ds_cmd =  (const drawSurfsCommand_t *)curCmd;

					for( i = 0, drawSurf = ds_cmd->drawSurfs; i < ds_cmd->numDrawSurfs; i++, drawSurf++ ) {
						R_DecomposeSort( drawSurf->sort, &entityNum, &shader, &fogNum, &postRender );
						sortedIndex = (( drawSurf->sort >> QSORT_SHADERNUM_SHIFT ) & (MAX_SHADERS-1));
						if( sortedIndex >= newShader ) {
							sortedIndex++;
							drawSurf->sort = (sortedIndex << QSORT_SHADERNUM_SHIFT) | (entityNum << QSORT_REFENTITYNUM_SHIFT) | (fogNum << QSORT_FOGNUM_SHIFT) | (postRender << QSORT_POSTRENDER_SHIFT);// | dlightMap;
						}
					}
					curCmd = (const void *)(ds_cmd + 1);
					break;
					}
				case RC_DRAW_BUFFER:
					{
					const drawBufferCommand_t *db_cmd = (const drawBufferCommand_t *)curCmd;
					curCmd = (const void *)(db_cmd + 1);
					break;
					}
				case RC_SWAP_BUFFERS:
					{
					const swapBuffersCommand_t *sb_cmd = (const swapBuffersCommand_t *)curCmd;
					curCmd = (const void *)(sb_cmd + 1);
					break;
					}
				case RC_DRAW_OCCLUSION:
					{
					const drawOcclusionCommand_t *do_cmd = (const drawOcclusionCommand_t *)curCmd;
					curCmd = (const void *)(do_cmd + 1);
					break;
					}
				case RC_END_OF_LIST:
				default:
					return;
				}
			}
		}
	}
}

/*
==============
SortNewShader

Positions the most recently created shader in the tr.sortedShaders[]
array so that the shader->sort key is sorted reletive to the other
shaders.

Sets shader->sortedIndex
==============
*/
static void SortNewShader( void ) {
	int		i;
	float	sort;
	shader_t	*newShader;

	newShader = tr.shaders[ tr.numShaders - 1 ];
	sort = newShader->sort;

	for ( i = tr.numShaders - 2 ; i >= 0 ; i-- ) {
		if ( tr.sortedShaders[ i ]->sort <= sort ) {
			break;
		}
		tr.sortedShaders[i+1] = tr.sortedShaders[i];
		tr.sortedShaders[i+1]->sortedIndex++;
	}

	// Arnout: fix rendercommandlist
	// https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=493
	FixRenderCommandList( i+1 );

	newShader->sortedIndex = i+1;
	tr.sortedShaders[i+1] = newShader;
}


/*
====================
GeneratePermanentShader
====================
*/
static shader_t *GeneratePermanentShader( void ) {
	shader_t	*newShader;
	int			i, b;
	int			size, hash;

	if ( tr.numShaders == MAX_SHADERS ) {
		ri->Printf( PRINT_WARNING, "WARNING: GeneratePermanentShader - MAX_SHADERS hit\n");
		return tr.defaultShader;
	}

	newShader = (shader_t *)ri->Hunk_Alloc( sizeof( shader_t ), h_low );

	*newShader = shader;

	if ( shader.sort <= SS_SEE_THROUGH ) {
		newShader->fogPass = FP_EQUAL;
	} else if ( shader.contentFlags & CONTENTS_FOG ) {
		newShader->fogPass = FP_LE;
	}

	tr.shaders[ tr.numShaders ] = newShader;
	newShader->index = tr.numShaders;

	tr.sortedShaders[ tr.numShaders ] = newShader;
	newShader->sortedIndex = tr.numShaders;

	tr.numShaders++;

	for ( i = 0 ; i < newShader->numUnfoggedPasses ; i++ ) {
		if ( !stages[i].active ) {
			break;
		}
		newShader->stages[i] = (shaderStage_t *)ri->Hunk_Alloc( sizeof( stages[i] ), h_low );
		*newShader->stages[i] = stages[i];

		for ( b = 0 ; b < NUM_TEXTURE_BUNDLES ; b++ ) {
			size = newShader->stages[i]->bundle[b].numTexMods * sizeof( texModInfo_t );
			newShader->stages[i]->bundle[b].texMods = (texModInfo_t *)ri->Hunk_Alloc( size, h_low );
			Com_Memcpy( newShader->stages[i]->bundle[b].texMods, stages[i].bundle[b].texMods, size );
		}
	}

	SortNewShader();

	hash = generateHashValue(newShader->name, FILE_HASH_SIZE);
	newShader->next = hashTable[hash];
	hashTable[hash] = newShader;

	return newShader;
}

/*
=================
VertexLightingCollapse

If vertex lighting is enabled, only render a single
pass, trying to guess which is the correct one to best aproximate
what it is supposed to look like.
=================
*/
static void VertexLightingCollapse( void ) {
	int		stage;
	shaderStage_t	*bestStage;
	int		bestImageRank;
	int		rank;

	// if we aren't opaque, just use the first pass
	if ( shader.sort == SS_OPAQUE ) {

		// pick the best texture for the single pass
		bestStage = &stages[0];
		bestImageRank = -999999;

		for ( stage = 0; stage < MAX_SHADER_STAGES; stage++ ) {
			shaderStage_t *pStage = &stages[stage];

			if ( !pStage->active ) {
				break;
			}
			rank = 0;

			if ( pStage->bundle[0].isLightmap ) {
				rank -= 100;
			}
			if ( pStage->bundle[0].tcGen != TCGEN_TEXTURE ) {
				rank -= 5;
			}
			if ( pStage->bundle[0].numTexMods ) {
				rank -= 5;
			}
			if ( pStage->rgbGen != CGEN_IDENTITY && pStage->rgbGen != CGEN_IDENTITY_LIGHTING ) {
				rank -= 3;
			}

			if ( rank > bestImageRank  ) {
				bestImageRank = rank;
				bestStage = pStage;
			}
		}

		stages[0].bundle[0] = bestStage->bundle[0];
		stages[0].stateBits &= ~( GLS_DSTBLEND_BITS | GLS_SRCBLEND_BITS );
		stages[0].stateBits |= GLS_DEPTHMASK_TRUE;
		if ( shader.lightmapIndex[0] == LIGHTMAP_NONE ) {
			stages[0].rgbGen = CGEN_LIGHTING_DIFFUSE;
		} else {
			stages[0].rgbGen = CGEN_EXACT_VERTEX;
		}
		stages[0].alphaGen = AGEN_SKIP;
	} else {
		// don't use a lightmap (tesla coils)
		if ( stages[0].bundle[0].isLightmap ) {
			stages[0] = stages[1];
		}

		// if we were in a cross-fade cgen, hack it to normal
		if ( stages[0].rgbGen == CGEN_ONE_MINUS_ENTITY || stages[1].rgbGen == CGEN_ONE_MINUS_ENTITY ) {
			stages[0].rgbGen = CGEN_IDENTITY_LIGHTING;
		}
		if ( ( stages[0].rgbGen == CGEN_WAVEFORM && stages[0].rgbWave.func == GF_SAWTOOTH )
			&& ( stages[1].rgbGen == CGEN_WAVEFORM && stages[1].rgbWave.func == GF_INVERSE_SAWTOOTH ) ) {
			stages[0].rgbGen = CGEN_IDENTITY_LIGHTING;
		}
		if ( ( stages[0].rgbGen == CGEN_WAVEFORM && stages[0].rgbWave.func == GF_INVERSE_SAWTOOTH )
			&& ( stages[1].rgbGen == CGEN_WAVEFORM && stages[1].rgbWave.func == GF_SAWTOOTH ) ) {
			stages[0].rgbGen = CGEN_IDENTITY_LIGHTING;
		}
	}

	for ( stage = 1; stage < MAX_SHADER_STAGES; stage++ ) {
		shaderStage_t *pStage = &stages[stage];

		if ( !pStage->active ) {
			break;
		}

		Com_Memset( pStage, 0, sizeof( *pStage ) );
	}
}

int FindFirstLightmapStage ( const shaderStage_t *stages, int numStages )
{
	for ( int i = 0; i < numStages; i++ )
	{
		const shaderStage_t *stage = &stages[i];
		if ( stage->active && stage->bundle[0].isLightmap )
		{
			return i;
		}
	}

	return numStages;
}

int GetNumStylesInShader ( const shader_t *shader )
{
	for ( int i = 0; i < MAXLIGHTMAPS; i++ )
	{
		if ( shader->styles[i] >= LS_UNUSED )
		{
			return i - 1;
		}
	}

	return MAXLIGHTMAPS - 1;
}

/*
=========================
FinishShader

Returns a freshly allocated shader with all the needed info
from the current global working shader
=========================
*/
static shader_t *FinishShader( void ) {
	int stage;
	qboolean hasLightmapStage = qfalse;

	//
	// set sky stuff appropriate
	//
	if ( shader.isSky ) {
		shader.sort = SS_ENVIRONMENT;
	}

	//
	// set polygon offset
	//
	if ( shader.polygonOffset && !shader.sort ) {
		shader.sort = SS_DECAL;
	}

#if 0
	int firstLightmapStage;
	shaderStage_t *lmStage;

	firstLightmapStage = FindFirstLightmapStage (stages, MAX_SHADER_STAGES);
	lmStage = &stages[firstLightmapStage];

	if ( firstLightmapStage != MAX_SHADER_STAGES )
	{
		if ( shader.lightmapIndex[0] == LIGHTMAP_BY_VERTEX )
		{
			if ( firstLightmapStage == 0 )
			{
				/*// Shift all stages above it down 1.
				memmove (lmStage,
					lmStage + 1,
					sizeof (shaderStage_t) * (MAX_SHADER_STAGES - firstLightmapStage - 1));
				memset (stages + MAX_SHADER_STAGES - 1, 0, sizeof (shaderStage_t));

				// Set state bits back to default on the over-written stage.
				 lmStage->stateBits = GLS_DEFAULT;*/
				ri->Printf (PRINT_ALL, "Shader '%s' has first stage as lightmap by vertex.\n", shader.name);
			}

			/*lmStage->rgbGen = CGEN_EXACT_VERTEX_LIT;
			lmStage->alphaGen = AGEN_SKIP;

			firstLightmapStage = MAX_SHADER_STAGES;*/
		}
	}

	if ( firstLightmapStage != MAX_SHADER_STAGES )
	{
		int numStyles = GetNumStylesInShader (&shader);

		ri->Printf (PRINT_ALL, "Shader '%s' has %d stages with light styles.\n", shader.name, numStyles);
		/*if ( numStyles > 0 )
		{
			// Move back all stages, after the first lightmap stage, by 'numStyles' elements.
			memmove (lmStage + numStyles,
				lmStage + 1,
				sizeof (shaderStage_t) * (MAX_SHADER_STAGES - firstLightmapStage - numStyles - 1));

			// Insert new shader stages after first lightmap stage
			for ( int i = 1; i <= numStyles; i++ )
			{
				shaderStage_t *stage = lmStage + i;

				// Duplicate first lightmap stage into this stage.
				*stage = *lmStage;

				if ( shader.lightmapIndex[i] == LIGHTMAP_BY_VERTEX )
				{
					stage->bundle[0].image[0] = tr.whiteImage;
				}
				else if ( shader.lightmapIndex[i] < 0 )
				{
					Com_Error (ERR_DROP, "FinishShader: light style with no lightmap or vertex style in shader %s.\n", shader.name);
				}
				else
				{
					stage->bundle[0].image[0] = tr.lightmaps[shader.lightmapIndex[i]];
					stage->bundle[0].tcGen = (texCoordGen_t)(TCGEN_LIGHTMAP + i);
				}

				stage->rgbGen = CGEN_LIGHTMAPSTYLE;
				stage->stateBits &= ~(GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS);
				stage->stateBits |= GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE;
			}
		}

		// Set all the light styles for the lightmap stages.
		for ( int i = 0; i <= numStyles; i++ )
		{
			lmStage[i].lightmapStyle = shader.styles[i];
		}*/
	}
#else
	int lmStage;
	for(lmStage = 0; lmStage < MAX_SHADER_STAGES; lmStage++)
	{
		shaderStage_t *pStage = &stages[lmStage];
		if (pStage->active && pStage->bundle[0].isLightmap)
		{
			break;
		}
	}

	if (lmStage < MAX_SHADER_STAGES)
	{
		if (shader.lightmapIndex[0] == LIGHTMAP_BY_VERTEX)
		{
			if (lmStage == 0)	//< MAX_SHADER_STAGES-1)
			{//copy the rest down over the lightmap slot
				memmove(&stages[lmStage], &stages[lmStage+1], sizeof(shaderStage_t) * (MAX_SHADER_STAGES-lmStage-1));
				memset(&stages[MAX_SHADER_STAGES-1], 0, sizeof(shaderStage_t));
				//change blending on the moved down stage
				stages[lmStage].stateBits = GLS_DEFAULT;
			}
			//change anything that was moved down (or the *white if LM is first) to use vertex color
			stages[lmStage].rgbGen = CGEN_EXACT_VERTEX;
			stages[lmStage].alphaGen = AGEN_SKIP;
			lmStage = MAX_SHADER_STAGES;	//skip the style checking below
		}
	}

	if (lmStage < MAX_SHADER_STAGES)// && !r_fullbright->value)
	{
		int	numStyles;
		int	i;

		for(numStyles=0;numStyles<MAXLIGHTMAPS;numStyles++)
		{
			if (shader.styles[numStyles] >= LS_UNUSED)
			{
				break;
			}
		}
		numStyles--;
		if (numStyles > 0)
		{
			for(i=MAX_SHADER_STAGES-1;i>lmStage+numStyles;i--)
			{
				stages[i] = stages[i-numStyles];
			}

			for(i=0;i<numStyles;i++)
			{
				stages[lmStage+i+1] = stages[lmStage];
				if (shader.lightmapIndex[i+1] == LIGHTMAP_BY_VERTEX)
				{
					stages[lmStage+i+1].bundle[0].image[0] = tr.whiteImage;
				}
				else if (shader.lightmapIndex[i+1] < 0)
				{
					Com_Error( ERR_DROP, "FinishShader: light style with no light map or vertex color for shader %s", shader.name);
				}
				else
				{
					stages[lmStage+i+1].bundle[0].image[0] = tr.lightmaps[shader.lightmapIndex[i+1]];
					stages[lmStage+i+1].bundle[0].tcGen = (texCoordGen_t)(TCGEN_LIGHTMAP+i+1);
				}
				stages[lmStage+i+1].rgbGen = CGEN_LIGHTMAPSTYLE;
				stages[lmStage+i+1].stateBits &= ~(GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS);
				stages[lmStage+i+1].stateBits |= GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE;
			}
		}

		for(i=0;i<=numStyles;i++)
		{
			stages[lmStage+i].lightmapStyle = shader.styles[i];
		}
	}
#endif

	//
	// set appropriate stage information
	//
	for ( stage = 0; stage < MAX_SHADER_STAGES; ) {
		shaderStage_t *pStage = &stages[stage];

		if ( !pStage->active ) {
			break;
		}

    // check for a missing texture
		/*if ( !pStage->bundle[0].image[0] ) {
			ri->Printf( PRINT_WARNING, "Shader %s has a stage with no image\n", shader.name );
			pStage->active = qfalse;
			stage++;
			continue;
		}*/

		// check for a missing texture
		switch (pStage->type)
		{
			//case ST_LIGHTMAP:
			//	// skip
			//	break;

			case ST_COLORMAP: // + ST_DIFFUSEMAP
			default:
			{
				if(!pStage->bundle[0].image[0])
				{
					ri->Printf(PRINT_WARNING, "Shader %s has a colormap/diffusemap stage with no image\n", shader.name);
					//pStage->active = qfalse;
					//stage++;
					//continue;
					pStage->bundle[0].image[0] = tr.defaultImage;
				}
				break;
			}

			case ST_NORMALMAP:
			{
				if(!pStage->bundle[0].image[0])
				{
					ri->Printf(PRINT_WARNING, "Shader %s has a normalmap stage with no image\n", shader.name);
					pStage->bundle[0].image[0] = tr.whiteImage;
				}
				break;
			}

			case ST_SPECULARMAP:
			{
				if(!pStage->bundle[0].image[0])
				{
					ri->Printf(PRINT_WARNING, "Shader %s has a specularmap stage with no image\n", shader.name);
					pStage->bundle[0].image[0] = tr.whiteImage; // should be blackImage
				}
				break;
			}

			case ST_NORMALPARALLAXMAP:
			{
				if(!pStage->bundle[0].image[0])
				{
					ri->Printf(PRINT_WARNING, "Shader %s has a normalparallaxmap stage with no image\n", shader.name);
					pStage->active = qfalse;
					stage++;
					continue;
				}
				break;
			}

			/*case ST_SUBSURFACEMAP:
			{
				if(!pStage->bundle[0].image[0])
				{
					ri->Printf(PRINT_WARNING, "Shader %s has a subsurfacemap stage with no image\n", shader.name);
					pStage->active = qfalse;
					stage++;
					continue;
				}
				break;
			}*/

			case ST_OVERLAYMAP:
			{
				if(!pStage->bundle[0].image[0])
				{
					ri->Printf(PRINT_WARNING, "Shader %s has a overlaymap stage with no image\n", shader.name);
					pStage->active = qfalse;
					stage++;
					continue;
				}
				break;
			}

			case ST_STEEPMAP:
			{
				if(!pStage->bundle[0].image[0])
				{
					ri->Printf(PRINT_WARNING, "Shader %s has a steepmap stage with no image\n", shader.name);
					pStage->active = qfalse;
					stage++;
					continue;
				}
				break;
			}

			case ST_STEEPMAP2:
			{
				if(!pStage->bundle[0].image[0])
				{
					ri->Printf(PRINT_WARNING, "Shader %s has a steepmap stage with no image\n", shader.name);
					pStage->active = qfalse;
					stage++;
					continue;
				}
				break;
			}

			case ST_SPLATCONTROLMAP:
			{
				if(!pStage->bundle[0].image[0])
				{
					ri->Printf(PRINT_WARNING, "Shader %s has a splatcontrolmap stage with no image\n", shader.name);
					pStage->active = qfalse;
					stage++;
					continue;
				}
				break;
			}
			case ST_SPLATMAP1:
			{
				if(!pStage->bundle[0].image[0])
				{
					ri->Printf(PRINT_WARNING, "Shader %s has a splatmap1 stage with no image\n", shader.name);
					pStage->active = qfalse;
					stage++;
					continue;
				}
				break;
			}
			case ST_SPLATMAP2:
			{
				if(!pStage->bundle[0].image[0])
				{
					ri->Printf(PRINT_WARNING, "Shader %s has a splatmap2 stage with no image\n", shader.name);
					pStage->active = qfalse;
					stage++;
					continue;
				}
				break;
			}
			case ST_SPLATMAP3:
			{
				if(!pStage->bundle[0].image[0])
				{
					ri->Printf(PRINT_WARNING, "Shader %s has a splatmap3 stage with no image\n", shader.name);
					pStage->active = qfalse;
					stage++;
					continue;
				}
				break;
			}
			case ST_SPLATMAP4:
			{
				if(!pStage->bundle[0].image[0])
				{
					ri->Printf(PRINT_WARNING, "Shader %s has a splatmap4 stage with no image\n", shader.name);
					pStage->active = qfalse;
					stage++;
					continue;
				}
				break;
			}
		}

		//
		// ditch this stage if it's detail and detail textures are disabled
		//
		if ( pStage->isDetail && !r_detailTextures->integer )
		{
			int index;

			for(index = stage + 1; index < MAX_SHADER_STAGES; index++)
			{
				if(!stages[index].active)
					break;
			}

			if(index < MAX_SHADER_STAGES)
				memmove(pStage, pStage + 1, sizeof(*pStage) * (index - stage));
			else
			{
				if(stage + 1 < MAX_SHADER_STAGES)
					memmove(pStage, pStage + 1, sizeof(*pStage) * (index - stage - 1));

				Com_Memset(&stages[index - 1], 0, sizeof(*stages));
			}

			continue;
		}

		//
		// default texture coordinate generation
		//
		if ( pStage->bundle[0].isLightmap ) {
			if ( pStage->bundle[0].tcGen == TCGEN_BAD ) {
				pStage->bundle[0].tcGen = TCGEN_LIGHTMAP;
			}
			hasLightmapStage = qtrue;
		} else {
			if ( pStage->bundle[0].tcGen == TCGEN_BAD ) {
				pStage->bundle[0].tcGen = TCGEN_TEXTURE;
			}
		}

		//
		// determine sort order and fog color adjustment
		//
		if ( ( pStage->stateBits & ( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS ) ) &&
			 ( stages[0].stateBits & ( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS ) ) ) {
			int blendSrcBits = pStage->stateBits & GLS_SRCBLEND_BITS;
			int blendDstBits = pStage->stateBits & GLS_DSTBLEND_BITS;

			// fog color adjustment only works for blend modes that have a contribution
			// that aproaches 0 as the modulate values aproach 0 --
			// GL_ONE, GL_ONE
			// GL_ZERO, GL_ONE_MINUS_SRC_COLOR
			// GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA

			// modulate, additive
			if ( ( ( blendSrcBits == GLS_SRCBLEND_ONE ) && ( blendDstBits == GLS_DSTBLEND_ONE ) ) ||
				( ( blendSrcBits == GLS_SRCBLEND_ZERO ) && ( blendDstBits == GLS_DSTBLEND_ONE_MINUS_SRC_COLOR ) ) ) {
				pStage->adjustColorsForFog = ACFF_MODULATE_RGB;
			}
			// strict blend
			else if ( ( blendSrcBits == GLS_SRCBLEND_SRC_ALPHA ) && ( blendDstBits == GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA ) )
			{
				pStage->adjustColorsForFog = ACFF_MODULATE_ALPHA;
			}
			// premultiplied alpha
			else if ( ( blendSrcBits == GLS_SRCBLEND_ONE ) && ( blendDstBits == GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA ) )
			{
				pStage->adjustColorsForFog = ACFF_MODULATE_RGBA;
			} else {
				// we can't adjust this one correctly, so it won't be exactly correct in fog
			}

			// don't screw with sort order if this is a portal or environment
			if ( !shader.sort ) {
				// see through item, like a grill or grate
				if ( pStage->stateBits & GLS_DEPTHMASK_TRUE ) {
					shader.sort = SS_SEE_THROUGH;
				} else {
					if (( blendSrcBits == GLS_SRCBLEND_ONE ) && ( blendDstBits == GLS_DSTBLEND_ONE ))
					{
						// GL_ONE GL_ONE needs to come a bit later
						shader.sort = SS_BLEND1;
					}
					else
					{
						shader.sort = SS_BLEND0;
					}
				}
			}
		}

		stage++;
	}

	// there are times when you will need to manually apply a sort to
	// opaque alpha tested shaders that have later blend passes
	if ( !shader.sort ) {
		shader.sort = SS_OPAQUE;
	}

	//
	// if we are in r_vertexLight mode, never use a lightmap texture
	//
	if ( stage > 1 && (r_vertexLight->integer && !r_uiFullScreen->integer) ) {
		VertexLightingCollapse();
		hasLightmapStage = qfalse;
	}

	//
	// look for multitexture potential
	//
	stage = CollapseStagesToGLSL();

	if ( (shader.lightmapIndex[0] || shader.lightmapIndex[1] || shader.lightmapIndex[2] || shader.lightmapIndex[3]) && !hasLightmapStage )
	{
		ri->Printf( PRINT_DEVELOPER, "WARNING: shader '%s' has lightmap but no lightmap stage!\n", shader.name );
		// Don't set this, it will just add duplicate shaders to the hash
  		//shader.lightmapIndex = LIGHTMAP_NONE;
	}


	//
	// compute number of passes
	//
	shader.numUnfoggedPasses = stage;

	// fogonly shaders don't have any normal passes
	if (stage == 0 && !shader.isSky)
		shader.sort = SS_FOG;

	// determine which stage iterator function is appropriate
	ComputeStageIteratorFunc();

	// determine which vertex attributes this shader needs
	ComputeVertexAttribs();

	return GeneratePermanentShader();
}

//========================================================================================

/*
====================
FindShaderInShaderText

Scans the combined text description of all the shader files for
the given shader name.

return NULL if not found

If found, it will return a valid shader
=====================
*/
static const char *FindShaderInShaderText( const char *shadername ) {

	char *token;
	const char *p = NULL;

	int i, hash;

	hash = generateHashValue(shadername, MAX_SHADERTEXT_HASH);

	if(shaderTextHashTable[hash])
	{
		for (i = 0; shaderTextHashTable[hash][i]; i++)
		{
			p = shaderTextHashTable[hash][i];
			token = COM_ParseExt(&p, qtrue);

			if(!Q_stricmp(token, shadername))
				return p;
		}
	}

	p = s_shaderText;

	if ( !p ) {
		return NULL;
	}

	// look for label
	while ( 1 ) {
		token = COM_ParseExt( &p, qtrue );
		if ( token[0] == 0 ) {
			break;
		}

		if ( !Q_stricmp( token, shadername ) ) {
			return p;
		}
		else {
			// skip the definition
			SkipBracedSection_Depth( &p, 0 );
		}
	}

	return NULL;
}


/*
==================
R_FindShaderByName

Will always return a valid shader, but it might be the
default shader if the real one can't be found.
==================
*/
shader_t *R_FindShaderByName( const char *name ) {
	char		strippedName[MAX_QPATH];
	int			hash;
	shader_t	*sh;

	if ( (name==NULL) || (name[0] == 0) ) {
		return tr.defaultShader;
	}

	COM_StripExtension(name, strippedName, sizeof(strippedName));

	hash = generateHashValue(strippedName, FILE_HASH_SIZE);

	//
	// see if the shader is already loaded
	//
	for (sh=hashTable[hash]; sh; sh=sh->next) {
		// NOTE: if there was no shader or image available with the name strippedName
		// then a default shader is created with lightmapIndex == LIGHTMAP_NONE, so we
		// have to check all default shaders otherwise for every call to R_FindShader
		// with that same strippedName a new default shader is created.
		if (Q_stricmp(sh->name, strippedName) == 0) {
			// match found
			return sh;
		}
	}

	return tr.defaultShader;
}

static qboolean IsShader ( const shader_t *sh, const char *name, const int *lightmapIndexes, const byte *styles )
{
	if ( Q_stricmp (sh->name, name) != 0 )
	{
		return qfalse;
	}

	if ( sh->defaultShader )
	{
		return qtrue;
	}

	for ( int i = 0; i < MAXLIGHTMAPS; i++ )
	{
		if ( sh->lightmapIndex[i] != lightmapIndexes[i] )
		{
			return qfalse;
		}

		if ( sh->styles[i] != styles[i] )
		{
			return qfalse;
		}
	}

	return qtrue;
}


/*
======================================================================================================================================
                                                   Rend2 - Compatibile Generic Shaders.
======================================================================================================================================

This creates generic shaders for anything that has none to support rend2 stuff...

*/

#ifdef ___SHADER_GENERATOR___
char uniqueGenericGlow[] = "{\n"\
"map %s_glow\n"\
"blendFunc GL_ONE GL_ONE\n"\
"glow\n"\
"}\n";

char uniqueGenericFoliageShader[] = "{\n"\
"qer_editorimage	%s\n"\
"q3map_alphashadow\n"\
"q3map_material	GreenLeaves\n"\
"//surfaceparm	trans\n"\
"//surfaceparm	noimpact\n"\
"//surfaceparm	nomarks\n"\
"surfaceparm	nonsolid\n"\
"cull	twosided\n"\
"{\n"\
"map %s\n"\
"blendfunc GL_ONE GL_ZERO\n"\
"alphaFunc GE128\n"\
"depthWrite\n"\
"rgbGen identity\n"\
"}\n"\
"//{\n"\
"//map $lightmap\n"\
"//blendfunc GL_DST_COLOR GL_ZERO\n"\
"//rgbGen lightingDiffuse\n"\
"//depthFunc equal\n"\
"//}\n"\
"}\n"\
"";

//"sort seethrough\n"\
//"cull	twosided\n"\
// "rgbGen identity\n"\   ^^^
// "rgbGen entity\n"\
// "rgbGen vertex\n"\
// "rgbGen lightingDiffuse\n"\

char uniqueGenericFoliageBillboardShader[] = "{\n"\
"qer_editorimage	%s\n"\
"q3map_alphashadow\n"\
"q3map_material	DryLeaves\n"\
"surfaceparm	trans\n"\
"surfaceparm	noimpact\n"\
"surfaceparm	nomarks\n"\
"{\n"\
"map %s\n"\
"blendfunc GL_ONE GL_ZERO\n"\
"alphaFunc GE128\n"\
"depthWrite\n"\
"rgbGen identity\n"\
"}\n"\
"//{\n"\
"//map $lightmap\n"\
"//blendfunc GL_DST_COLOR GL_ZERO\n"\
"//rgbGen lightingDiffuse\n"\
"//depthFunc equal\n"\
"//}\n"\
"}\n"\
"";

//"sort seethrough\n"\
//"cull	twosided\n"\

char uniqueGenericFoliageTreeShader[] = "{\n"\
"qer_editorimage	%s\n"\
"//q3map_alphashadow\n"\
"q3map_material	solidwood\n"\
"//surfaceparm	noimpact\n"\
"//surfaceparm	nomarks\n"\
"{\n"\
"map %s\n"\
"blendfunc GL_ONE GL_ZERO\n"\
"alphaFunc GE128\n"\
"depthWrite\n"\
"rgbGen identity\n"\
"tcMod scale 2.5 2.5\n"\
"}\n"\
"%s\n"\
"//{\n"\
"//map $lightmap\n"\
"//blendfunc GL_DST_COLOR GL_ZERO\n"\
"//rgbGen lightingDiffuse\n"\
"//depthFunc equal\n"\
"//}\n"\
"}\n"\
"";

char uniqueGenericPlayerShader[] = "{\n"\
"qer_editorimage	%s\n"\
"//q3map_nolightmap\n"\
"//q3map_alphashadow\n"\
"surfaceparm	trans\n"\
"surfaceparm	noimpact\n"\
"surfaceparm	nomarks\n"\
"{\n"\
"map %s\n"\
"blendfunc GL_SRC_ALPHA GL_ZERO\n"\
"alphaFunc GE128\n"\
"depthWrite\n"\
"//rgbGen lightingDiffuse\n"\
"rgbGen entity\n"\
"}\n"\
"%s"\
"{\n"\
"map %s\n"\
"blendFunc GL_SRC_ALPHA GL_ONE\n"\
"rgbGen lightingDiffuse\n"\
"alphaGen lightingSpecular\n"\
"alphaFunc GE128\n"\
"depthFunc equal\n"\
"detail\n"\
"}\n"\
"{\n"\
"map $lightmap\n"\
"blendfunc GL_DST_COLOR GL_ZERO\n"\
"rgbGen lightingDiffuse\n"\
"depthFunc equal\n"\
"}\n"\
"}\n"\
"";

char uniqueGenericArmorShader[] = "{\n"\
"qer_editorimage	%s\n"\
"//q3map_alphashadow\n"\
"q3map_material	armor\n"\
"surfaceparm trans\n"\
"surfaceparm	noimpact\n"\
"surfaceparm	nomarks\n"\
"cull	twosided\n"\
"{\n"\
"map %s\n"\
"blendfunc GL_SRC_ALPHA GL_ZERO\n"\
"alphaFunc GE128\n"\
"//rgbGen lightingDiffuse\n"\
"depthWrite\n"\
"rgbGen entity\n"\
"}\n"\
"%s"\
"{\n"\
"map %s\n"\
"blendFunc GL_SRC_ALPHA GL_ONE\n"\
"rgbGen lightingDiffuse\n"\
"alphaGen lightingSpecular\n"\
"alphaFunc GE128\n"\
"depthFunc equal\n"\
"detail\n"\
"}\n"\
"{\n"\
"map $lightmap\n"\
"blendfunc GL_DST_COLOR GL_ZERO\n"\
"rgbGen lightingDiffuse\n"\
"depthFunc equal\n"\
"}\n"\
"}\n"\
"";

char uniqueGenericMetalShader[] = "{\n"\
"qer_editorimage	%s\n"\
"q3map_material	hollowmetal\n"\
"//q3map_alphashadow\n"\
"surfaceparm	trans\n"\
"surfaceparm	noimpact\n"\
"surfaceparm	nomarks\n"\
"cull	twosided\n"\
"{\n"\
"map %s\n"\
"blendfunc GL_SRC_ALPHA GL_ZERO\n"\
"alphaFunc GE128\n"\
"//rgbGen lightingDiffuse\n"\
"depthWrite\n"\
"rgbGen entity\n"\
"}\n"\
"%s"\
"{\n"\
"map %s\n"\
"blendFunc GL_SRC_ALPHA GL_ONE\n"\
"rgbGen lightingDiffuse\n"\
"alphaGen lightingSpecular\n"\
"alphaFunc GE128\n"\
"depthFunc equal\n"\
"detail\n"\
"}\n"\
"{\n"\
"map $lightmap\n"\
"blendfunc GL_DST_COLOR GL_ZERO\n"\
"rgbGen lightingDiffuse\n"\
"depthFunc equal\n"\
"}\n"\
"}\n"\
"";

char uniqueGenericMetalShader_OLD[] = "{\n"\
"qer_editorimage	%s\n"\
"q3map_material	hollowmetal\n"\
"//q3map_alphashadow\n"\
"surfaceparm	trans\n"\
"surfaceparm	noimpact\n"\
"surfaceparm	nomarks\n"\
"cull	twosided\n"\
"{\n"\
"map %s\n"\
"blendfunc GL_SRC_ALPHA GL_ZERO\n"\
"alphaFunc GE128\n"\
"depthWrite\n"\
"rgbGen entity\n"\
"}\n"\
"%s"\
"{\n"\
"map %s\n"\
"blendFunc GL_SRC_ALPHA GL_ONE\n"\
"rgbGen lightingDiffuse\n"\
"alphaGen lightingSpecular\n"\
"alphaFunc GE128\n"\
"depthFunc equal\n"\
"detail\n"\
"}\n"\
"}\n"\
"";

char uniqueGenericRockShader[] = "{\n"\
"qer_editorimage	%s\n"\
"//q3map_alphashadow\n"\
"q3map_material	rock\n"\
"surfaceparm	noimpact\n"\
"surfaceparm	nomarks\n"\
"{\n"\
"map %s\n"\
"%s\n"\
"blendfunc GL_ONE GL_ZERO\n"\
"alphaFunc GE128\n"\
"depthWrite\n"\
"rgbGen identity\n"\
"}\n"\
"//{\n"\
"//map $lightmap\n"\
"//blendfunc GL_DST_COLOR GL_ZERO\n"\
"//rgbGen lightingDiffuse\n"\
"//depthFunc equal\n"\
"//}\n"\
"}\n"\
"";

//"sort seethrough\n"\

#if 0
char uniqueGenericShader[] = "{\n"\
"qer_editorimage	%s\n"\
"{\n"\
"map %s\n"\
"blendfunc GL_SRC_ALPHA GL_ZERO\n"\
"alphaFunc GE128\n"\
"rgbGen vertex\n"\
"depthWrite\n"\
"}\n"\
"%s"\
"{\n"\
"map %s\n"\
"blendFunc GL_SRC_ALPHA GL_ONE\n"\
"rgbGen lightingDiffuse\n"\
"alphaGen lightingSpecular\n"\
"alphaFunc GE128\n"\
"depthFunc equal\n"\
"detail\n"\
"}\n"\
"{\n"\
"map $lightmap\n"\
"blendfunc GL_DST_COLOR GL_ZERO\n"\
"rgbGen lightingDiffuse\n"\
"depthFunc equal\n"\
"}\n"\
"}\n"\
"";

// "rgbGen identity\n"
#elif 1
char uniqueGenericShader[] = "{\n"\
"qer_editorimage	%s\n"\
"{\n"\
"map %s\n"\
"blendfunc GL_SRC_ALPHA GL_ZERO\n"\
"alphaFunc GE128\n"\
"depthWrite\n"\
"rgbGen identity\n"\
"}\n"\
"%s"\
"{\n"\
"map $lightmap\n"\
"blendfunc GL_DST_COLOR GL_ZERO\n"\
"rgbGen lightingDiffuse\n"\
"depthFunc equal\n"\
"}\n"\
"}\n"\
"";
#else
char uniqueGenericShader[] = "{\n"\
"qer_editorimage	%s\n"\
"{\n"\
"map %s\n"\
"%s\n"\
"blendfunc GL_ONE GL_ZERO\n"\
"alphaFunc GE128\n"\
"depthWrite\n"\
"rgbGen identity\n"\
"}\n"\
"%s"\
"//{\n"\
"//map $lightmap\n"\
"//blendfunc GL_DST_COLOR GL_ZERO\n"\
"//rgbGen lightingDiffuse\n"\
"//depthFunc equal\n"\
"//}\n"\
"}\n"\
"";
#endif


qboolean R_ForceGenericShader ( const char *name, const char *text )
{
	if (StringContainsWord(name, "raindrop")
		|| StringContainsWord(name, "gfx/effects/bubble")
		|| StringContainsWord(name, "gfx/water/screen_ripple")
		|| StringContainsWord(name, "gfx/water/alpha_bubbles")
		|| StringContainsWord(name, "gfx/water/overlay_bubbles"))
		return qfalse;
	else if (StringContainsWord(name, "vjun/vj4") || StringContainsWord(name, "vjun\\vj4"))
		return qtrue;
	else if (StringContainsWord(name, "warzone/foliage") || StringContainsWord(name, "warzone\\foliage"))
		return qtrue;
	else if (StringContainsWord(name, "warzone/tree") || StringContainsWord(name, "warzone\\tree"))
	{
		if (text && !StringContainsWord(text, "q3map_material"))
			return qtrue;
	}
	else if ( StringContainsWord(name, "warzone/billboard") || StringContainsWord(name, "warzone\\billboard"))
		return qtrue;
	else if (text && (StringsContainWord(name, text, "gfx")))
		return qfalse;
	else if (text && (StringsContainWord(name, text, "glow") || StringContainsWord(name, "icon")))
		return qfalse;
	else if (!text && (StringContainsWord(name, "glow") || StringContainsWord(name, "icon")))
		return qfalse;
	else if (StringContainsWord(name, "plastic") || StringContainsWord(name, "stormtrooper") || StringContainsWord(name, "snowtrooper") || StringContainsWord(name, "medpac") || StringContainsWord(name, "bacta") || StringContainsWord(name, "helmet"))
		return qtrue;
	else if (StringContainsWord(name, "mp/flag") || StringContainsWord(name, "xwing") || StringContainsWord(name, "xwbody") || StringContainsWord(name, "tie_") || StringContainsWord(name, "ship") || StringContainsWord(name, "shuttle") || StringContainsWord(name, "falcon") || StringContainsWord(name, "freight") || StringContainsWord(name, "transport") || StringContainsWord(name, "crate") || StringContainsWord(name, "container") || StringContainsWord(name, "barrel") || StringContainsWord(name, "train") || StringContainsWord(name, "crane") || StringContainsWord(name, "plate") || StringContainsWord(name, "cargo"))
		return qtrue;
	else if (StringContainsWord(name, "reborn") || StringContainsWord(name, "trooper"))
		return qtrue;
	else if (StringContainsWord(name, "boba") || StringContainsWord(name, "pilot"))
		return qtrue;

	return qfalse;
}

#endif //___SHADER_GENERATOR___

/*
===============
R_FindShader

Will always return a valid shader, but it might be the
default shader if the real one can't be found.

In the interest of not requiring an explicit shader text entry to
be defined for every single image used in the game, three default
shader behaviors can be auto-created for any image:

If lightmapIndex == LIGHTMAP_NONE, then the image will have
dynamic diffuse lighting applied to it, as apropriate for most
entity skin surfaces.

If lightmapIndex == LIGHTMAP_2D, then the image will be used
for 2D rendering unless an explicit shader is found

If lightmapIndex == LIGHTMAP_BY_VERTEX, then the image will use
the vertex rgba modulate values, as apropriate for misc_model
pre-lit surfaces.

Other lightmapIndex values will have a lightmap stage created
and src*dest blending applied with the texture, as apropriate for
most world construction surfaces.

===============
*/

#include "../client/tinythread.h"
#include "../client/fast_mutex.h"
using namespace tthread;
tthread::fast_mutex findshader_lock;

shader_t *R_FindShader( const char *name, const int *lightmapIndexes, const byte *styles, qboolean mipRawImage ) {
	char		strippedName[MAX_QPATH];
	int			hash, flags;
	const char	*shaderText;
	image_t		*image;
	shader_t	*sh;
#ifdef ___SHADER_GENERATOR___
	char		myShader[1024] = {0};
#endif //___SHADER_GENERATOR___

	if ( name[0] == 0 ) {
		return tr.defaultShader;
	}

	// use (fullbright) vertex lighting if the bsp file doesn't have
	// lightmaps
	if ( lightmapIndexes[0] >= 0 && lightmapIndexes[0] >= tr.numLightmaps ) {
		lightmapIndexes = lightmapsVertex;
	} else if ( lightmapIndexes[0] < LIGHTMAP_2D ) {
		// negative lightmap indexes cause stray pointers (think tr.lightmaps[lightmapIndex])
		ri->Printf( PRINT_WARNING, "WARNING: shader '%s' has invalid lightmap index of %d\n", name, lightmapIndexes[0]  );
		lightmapIndexes = lightmapsVertex;
	}

	COM_StripExtension(name, strippedName, sizeof(strippedName));

	hash = generateHashValue(strippedName, FILE_HASH_SIZE);

	//
	// see if the shader is already loaded
	//
	for (sh = hashTable[hash]; sh; sh = sh->next) {
		// NOTE: if there was no shader or image available with the name strippedName
		// then a default shader is created with lightmapIndex == LIGHTMAP_NONE, so we
		// have to check all default shaders otherwise for every call to R_FindShader
		// with that same strippedName a new default shader is created.
		if ( IsShader (sh, strippedName, lightmapIndexes, styles) ) {
			// match found
			return sh;
		}
	}

	// clear the global shader
	ClearGlobalShader();
	Q_strncpyz(shader.name, strippedName, sizeof(shader.name));
	Com_Memcpy (shader.lightmapIndex, lightmapIndexes, sizeof (shader.lightmapIndex));
	Com_Memcpy (shader.styles, styles, sizeof (shader.styles));

	//
	// attempt to define shader from an explicit parameter file
	//
	shaderText = FindShaderInShaderText( strippedName );

	//
	// Since this texture does not have a shader, create one for it - conditionally...
	//
#ifdef ___SHADER_GENERATOR___
	if ( shaderText && !R_ForceGenericShader(name, shaderText) ) {
#else //!___SHADER_GENERATOR___
	if ( shaderText ) {
#endif //___SHADER_GENERATOR___
		// enable this when building a pak file to get a global list
		// of all explicit shaders
		if ( r_printShaders->integer ) {
			ri->Printf( PRINT_ALL, "*SHADER* %s\n", name );
		}

		//Com_Error(ERR_FATAL, "SHADER LOOKS LIKE:\n%s\n", shaderText);

		if ( !ParseShader( name, &shaderText ) ) {
			// had errors, so use default shader
			shader.defaultShader = qtrue;
		}
#ifdef ___SHADER_GENERATOR___
		if (!shader.defaultShader || StringContainsWord(name, "icon") || !(!strncmp(name, "textures/", 9) || !strncmp(name, "models/", 7)))
		{
			sh = FinishShader();
			return sh;
		}
#else //!___SHADER_GENERATOR___
		sh = FinishShader();
		return sh;
#endif //___SHADER_GENERATOR___
	}

#ifdef ___SHADER_GENERATOR___
	if ((R_ForceGenericShader(name, shaderText) || (!strncmp(name, "textures/", 9) || !strncmp(name, "models/", 7))) 
		&& !StringContainsWord(name, "icon"))
	{
		char glowShaderAddition[256] = { 0 };
		int material = DetectMaterialType( name );

		shader.defaultShader = qfalse;

		// Check if this texture has a _glow component...
		char glowName[512];

		flags = IMGFLAG_NONE;

		if (r_srgb->integer)
			flags |= IMGFLAG_SRGB;

		if (mipRawImage)
		{
			flags |= IMGFLAG_MIPMAP | IMGFLAG_PICMIP;

			//if (r_genNormalMaps->integer)
			flags |= IMGFLAG_GENNORMALMAP;
		}
		else
		{
			flags |= IMGFLAG_CLAMPTOEDGE;
		}

		sprintf(glowName, "%s_glow", strippedName);
		image = R_FindImageFile( glowName, IMGTYPE_COLORALPHA, flags );

		if (image != NULL && image != tr.whiteImage && image != tr.blackImage && image != tr.defaultImage && image->width > 0 && image->height > 0)
			sprintf(glowShaderAddition, uniqueGenericGlow, strippedName);

		// Generate the shader...
		if (StringContainsWord(strippedName, "warzone/billboard") || StringContainsWord(strippedName, "warzone\\billboard"))
		{
			sprintf(myShader, uniqueGenericFoliageBillboardShader, strippedName, strippedName);
		}
		else if (StringContainsWord(strippedName, "warzone/foliage") || StringContainsWord(strippedName, "warzone\\foliage"))
		{
			sprintf(myShader, uniqueGenericFoliageShader, strippedName, strippedName);
		}
		else if (StringContainsWord(strippedName, "yavin/grass") || StringContainsWord(strippedName, "yavin\\grass") 
			|| StringContainsWord(strippedName, "yavin/tree_leaves") || StringContainsWord(strippedName, "yavin\\tree_leaves")
			|| StringContainsWord(strippedName, "yavin/tree1") || StringContainsWord(strippedName, "yavin\\tree1")
			|| StringContainsWord(strippedName, "yavin/vine") || StringContainsWord(strippedName, "yavin\\vine"))
		{
			sprintf(myShader, uniqueGenericFoliageShader, strippedName, strippedName);
		}
		else if (StringContainsWord(strippedName, "warzone/plants") || StringContainsWord(strippedName, "warzone\\plants"))
		{
			sprintf(myShader, uniqueGenericFoliageShader, strippedName, strippedName);
		}
		else if (StringContainsWord(strippedName, "warzone/tree") || StringContainsWord(strippedName, "warzone\\tree")
			|| StringContainsWord(strippedName, "warzone/deadtree") || StringContainsWord(strippedName, "warzone\\deadtree"))
		{
			if (StringContainsWord(strippedName, "bark") 
				|| StringContainsWord(strippedName, "trunk") 
				|| StringContainsWord(strippedName, "giant_tree") 
				|| StringContainsWord(strippedName, "vine01"))
				sprintf(myShader, uniqueGenericFoliageTreeShader, strippedName, strippedName, "");
			else
				sprintf(myShader, uniqueGenericFoliageShader, strippedName, strippedName);
		}
		else if (material == MATERIAL_ARMOR)
		{
			sprintf(myShader, uniqueGenericArmorShader, strippedName, strippedName, glowShaderAddition, strippedName);
		}
		else if (StringContainsWord(strippedName, "player"))
		{
			sprintf(myShader, uniqueGenericPlayerShader, strippedName, strippedName, glowShaderAddition, strippedName);
		}
		else if (StringContainsWord(strippedName, "weapon") || material == MATERIAL_SOLIDMETAL || material == MATERIAL_HOLLOWMETAL)
		{
			sprintf(myShader, uniqueGenericMetalShader, strippedName, strippedName, glowShaderAddition, strippedName);
		}
		else if (material == MATERIAL_ROCK || StringContainsWord(name, "warzone/rocks"))
		{
			sprintf(myShader, uniqueGenericRockShader, strippedName, strippedName, glowShaderAddition, strippedName);
		}
		else if (StringContainsWord(name, "vjun/vj4"))
		{
			if (StringContainsWord(name, "vjun/vj4_b"))
			{// pff
				char realName[128];
				sprintf(realName, "models/map_objects/vjun/vj4");
				sprintf(myShader, uniqueGenericRockShader, strippedName, realName, glowShaderAddition, realName);
			}
			else
			{
				sprintf(myShader, uniqueGenericRockShader, strippedName, strippedName, glowShaderAddition, strippedName);
			}
		}
		else
		{
			sprintf(myShader, uniqueGenericShader, strippedName, strippedName, glowShaderAddition, strippedName);
		}

		flags = IMGFLAG_NONE;

		//
		// attempt to define shader from an explicit parameter file
		//
		const char *shaderText2 = myShader;
		if ( shaderText2 ) {
			// enable this when building a pak file to get a global list
			// of all explicit shaders
			if ( r_printShaders->integer ) {
				ri->Printf( PRINT_ALL, "*SHADER* %s\n", name );
			}

			//Com_Error(ERR_FATAL, "SHADER LOOKS LIKE:\n%s\n", shaderText);

			if ( !ParseShader( name, &shaderText2 ) ) {
				// had errors, so use default shader
				shader.defaultShader = qtrue;
			} else {
				if (!StringContainsWord(name, "models/player") && !StringContainsWord(name, "models/weapon")) // skip this spam for now...
					if (r_genericShaderDebug->integer)
						ri->Printf(PRINT_WARNING, "Advanced generic shader generated for image %s.\n", name);
			}
			sh = FinishShader();
			return sh;
		}
	}
#endif //___SHADER_GENERATOR___

	//
	// if not defined in the in-memory shader descriptions,
	// look for a single supported image file
	//

	flags = IMGFLAG_NONE;

	if (r_srgb->integer)
		flags |= IMGFLAG_SRGB;

	if (mipRawImage)
	{
		flags |= IMGFLAG_MIPMAP | IMGFLAG_PICMIP;

		//if (r_genNormalMaps->integer)
			flags |= IMGFLAG_GENNORMALMAP;
	}
	else
	{
		flags |= IMGFLAG_CLAMPTOEDGE;
	}

	image = R_FindImageFile( name, IMGTYPE_COLORALPHA, flags );
	if ( !image ) {
		ri->Printf( PRINT_DEVELOPER, "Couldn't find image file for shader %s\n", name );
		shader.defaultShader = qtrue;
		return FinishShader();
	}

	//
	// create the default shading commands
	//
	if ( shader.lightmapIndex[0] == LIGHTMAP_NONE ) {
		// dynamic colors at vertexes
		stages[0].bundle[0].image[0] = image;
		stages[0].active = qtrue;
		stages[0].rgbGen = CGEN_LIGHTING_DIFFUSE;
		stages[0].stateBits = GLS_DEFAULT;
	} else if ( shader.lightmapIndex[0] == LIGHTMAP_BY_VERTEX ) {
		// explicit colors at vertexes
		stages[0].bundle[0].image[0] = image;
		stages[0].active = qtrue;
		stages[0].rgbGen = CGEN_EXACT_VERTEX;
		stages[0].alphaGen = AGEN_SKIP;
		stages[0].stateBits = GLS_DEFAULT;
	} else if ( shader.lightmapIndex[0] == LIGHTMAP_2D ) {
		// GUI elements
		stages[0].bundle[0].image[0] = image;
		stages[0].active = qtrue;
		stages[0].rgbGen = CGEN_VERTEX;
		stages[0].alphaGen = AGEN_VERTEX;
		stages[0].stateBits = GLS_DEPTHTEST_DISABLE |
			  GLS_SRCBLEND_SRC_ALPHA |
			  GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
	} else if ( shader.lightmapIndex[0] == LIGHTMAP_WHITEIMAGE ) {
		// fullbright level
		stages[0].bundle[0].image[0] = tr.whiteImage;
		stages[0].active = qtrue;
		stages[0].rgbGen = CGEN_IDENTITY_LIGHTING;
		stages[0].stateBits = GLS_DEFAULT;

		stages[1].bundle[0].image[0] = image;
		stages[1].active = qtrue;
		stages[1].rgbGen = CGEN_IDENTITY;
		stages[1].stateBits |= GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO;
	} else {
		// two pass lightmap
		stages[0].bundle[0].image[0] = tr.lightmaps[shader.lightmapIndex[0]];
		stages[0].bundle[0].isLightmap = qtrue;
		stages[0].active = qtrue;
		stages[0].rgbGen = CGEN_IDENTITY;	// lightmaps are scaled on creation
													// for identitylight
		stages[0].stateBits = GLS_DEFAULT;

		stages[1].bundle[0].image[0] = image;
		stages[1].active = qtrue;
		stages[1].rgbGen = CGEN_IDENTITY;
		stages[1].stateBits |= GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO;
	}

	return FinishShader();
}

shader_t *R_FindServerShader( const char *name, const int *lightmapIndexes, const byte *styles, qboolean mipRawImage )
{
	char		strippedName[MAX_QPATH];
	int			hash;
	shader_t	*sh;

	if ( name[0] == 0 ) {
		return tr.defaultShader;
	}

	COM_StripExtension( name, strippedName, sizeof( strippedName ) );

	hash = generateHashValue(strippedName, FILE_HASH_SIZE);

	//
	// see if the shader is already loaded
	//
	for (sh = hashTable[hash]; sh; sh = sh->next) {
		// NOTE: if there was no shader or image available with the name strippedName
		// then a default shader is created with lightmapIndex == LIGHTMAP_NONE, so we
		// have to check all default shaders otherwise for every call to R_FindShader
		// with that same strippedName a new default shader is created.
		if ( IsShader (sh, name, lightmapIndexes, styles) ) {
			// match found
			return sh;
		}
	}

	// clear the global shader
	ClearGlobalShader();
	Q_strncpyz(shader.name, strippedName, sizeof(shader.name));
	Com_Memcpy (shader.lightmapIndex, lightmapIndexes, sizeof (shader.lightmapIndex));

	shader.defaultShader = qtrue;
	return FinishShader();
}

qhandle_t RE_RegisterShaderFromImage(const char *name, const int *lightmapIndexes, const byte *styles, image_t *image, qboolean mipRawImage) {
	int			hash;
	shader_t	*sh;

	hash = generateHashValue(name, FILE_HASH_SIZE);

	// probably not necessary since this function
	// only gets called from tr_font.c with lightmapIndex == LIGHTMAP_2D
	// but better safe than sorry.
	if ( lightmapIndexes[0] >= tr.numLightmaps ) {
		lightmapIndexes = lightmapsFullBright;
	}

	//
	// see if the shader is already loaded
	//

	for (sh=hashTable[hash]; sh; sh=sh->next) {
		// NOTE: if there was no shader or image available with the name strippedName
		// then a default shader is created with lightmapIndex == LIGHTMAP_NONE, so we
		// have to check all default shaders otherwise for every call to R_FindShader
		// with that same strippedName a new default shader is created.
		if ( IsShader (sh, name, lightmapIndexes, styles) ) {
			// match found
			return sh->index;
		}
	}

	// clear the global shader
	ClearGlobalShader();
	Q_strncpyz(shader.name, name, sizeof(shader.name));
	Com_Memcpy (shader.lightmapIndex, lightmapIndexes, sizeof (shader.lightmapIndex));

	//
	// create the default shading commands
	//
	if ( shader.lightmapIndex[0] == LIGHTMAP_NONE ) {
		// dynamic colors at vertexes
		stages[0].bundle[0].image[0] = image;
		stages[0].active = qtrue;
		stages[0].rgbGen = CGEN_LIGHTING_DIFFUSE;
		stages[0].stateBits = GLS_DEFAULT;
	} else if ( shader.lightmapIndex[0] == LIGHTMAP_BY_VERTEX ) {
		// explicit colors at vertexes
		stages[0].bundle[0].image[0] = image;
		stages[0].active = qtrue;
		stages[0].rgbGen = CGEN_EXACT_VERTEX;
		stages[0].alphaGen = AGEN_SKIP;
		stages[0].stateBits = GLS_DEFAULT;
	} else if ( shader.lightmapIndex[0] == LIGHTMAP_2D ) {
		// GUI elements
		stages[0].bundle[0].image[0] = image;
		stages[0].active = qtrue;
		stages[0].rgbGen = CGEN_VERTEX;
		stages[0].alphaGen = AGEN_VERTEX;
		stages[0].stateBits = GLS_DEPTHTEST_DISABLE |
			  GLS_SRCBLEND_SRC_ALPHA |
			  GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
	} else if ( shader.lightmapIndex[0] == LIGHTMAP_WHITEIMAGE ) {
		// fullbright level
		stages[0].bundle[0].image[0] = tr.whiteImage;
		stages[0].active = qtrue;
		stages[0].rgbGen = CGEN_IDENTITY_LIGHTING;
		stages[0].stateBits = GLS_DEFAULT;

		stages[1].bundle[0].image[0] = image;
		stages[1].active = qtrue;
		stages[1].rgbGen = CGEN_IDENTITY;
		stages[1].stateBits |= GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO;
	} else {
		// two pass lightmap
		stages[0].bundle[0].image[0] = tr.lightmaps[shader.lightmapIndex[0]];
		stages[0].bundle[0].isLightmap = qtrue;
		stages[0].active = qtrue;
		stages[0].rgbGen = CGEN_IDENTITY;	// lightmaps are scaled on creation
													// for identitylight
		stages[0].stateBits = GLS_DEFAULT;

		stages[1].bundle[0].image[0] = image;
		stages[1].active = qtrue;
		stages[1].rgbGen = CGEN_IDENTITY;
		stages[1].stateBits |= GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO;
	}

	sh = FinishShader();
	return sh->index;
}


/*
====================
RE_RegisterShader

This is the exported shader entry point for the rest of the system
It will always return an index that will be valid.

This should really only be used for explicit shaders, because there is no
way to ask for different implicit lighting modes (vertex, lightmap, etc)
====================
*/
qhandle_t RE_RegisterShaderLightMap( const char *name, const int *lightmapIndexes, const byte *styles ) {
	shader_t	*sh;

	if ( strlen( name ) >= MAX_QPATH ) {
		ri->Printf( PRINT_ALL, "Shader name exceeds MAX_QPATH\n" );
		return 0;
	}

	sh = R_FindShader( name, lightmapIndexes, styles, qtrue );

	// we want to return 0 if the shader failed to
	// load for some reason, but R_FindShader should
	// still keep a name allocated for it, so if
	// something calls RE_RegisterShader again with
	// the same name, we don't try looking for it again
	if ( sh->defaultShader ) {
		return 0;
	}

	return sh->index;
}


/*
====================
RE_RegisterShader

This is the exported shader entry point for the rest of the system
It will always return an index that will be valid.

This should really only be used for explicit shaders, because there is no
way to ask for different implicit lighting modes (vertex, lightmap, etc)
====================
*/
qhandle_t RE_RegisterShader( const char *name ) {
	shader_t	*sh;

	if ( strlen( name ) >= MAX_QPATH ) {
		ri->Printf( PRINT_ALL, "Shader name exceeds MAX_QPATH\n" );
		return 0;
	}

	sh = R_FindShader( name, lightmaps2d, stylesDefault, qtrue );

	// we want to return 0 if the shader failed to
	// load for some reason, but R_FindShader should
	// still keep a name allocated for it, so if
	// something calls RE_RegisterShader again with
	// the same name, we don't try looking for it again
	if ( sh->defaultShader ) {
		return 0;
	}

	return sh->index;
}


/*
====================
RE_RegisterShaderNoMip

For menu graphics that should never be picmiped
====================
*/
qhandle_t RE_RegisterShaderNoMip( const char *name ) {
	shader_t	*sh;

	if ( strlen( name ) >= MAX_QPATH ) {
		ri->Printf( PRINT_ALL, "Shader name exceeds MAX_QPATH\n" );
		return 0;
	}

	sh = R_FindShader( name, lightmaps2d, stylesDefault, qfalse );

	// we want to return 0 if the shader failed to
	// load for some reason, but R_FindShader should
	// still keep a name allocated for it, so if
	// something calls RE_RegisterShader again with
	// the same name, we don't try looking for it again
	if ( sh->defaultShader ) {
		return 0;
	}

	return sh->index;
}

//added for ui -rww
const char *RE_ShaderNameFromIndex(int index)
{
	assert(index >= 0 && index < tr.numShaders && tr.shaders[index]);
	return tr.shaders[index]->name;
}

/*
====================
R_GetShaderByHandle

When a handle is passed in by another module, this range checks
it and returns a valid (possibly default) shader_t to be used internally.
====================
*/
shader_t *R_GetShaderByHandle( qhandle_t hShader ) {
	if ( hShader < 0 ) {
	  ri->Printf( PRINT_WARNING, "R_GetShaderByHandle: out of range hShader '%d'\n", hShader );
		return tr.defaultShader;
	}
	if ( hShader >= tr.numShaders ) {
		ri->Printf( PRINT_WARNING, "R_GetShaderByHandle: out of range hShader '%d'\n", hShader );
		return tr.defaultShader;
	}
	return tr.shaders[hShader];
}

/*
===============
R_ShaderList_f

Dump information on all valid shaders to the console
A second parameter will cause it to print in sorted order
===============
*/
void	R_ShaderList_f (void) {
	int			i;
	int			count;
	shader_t	*shader;

	ri->Printf (PRINT_ALL, "-----------------------\n");

	count = 0;
	for ( i = 0 ; i < tr.numShaders ; i++ ) {
		if ( ri->Cmd_Argc() > 1 ) {
			shader = tr.sortedShaders[i];
		} else {
			shader = tr.shaders[i];
		}

		ri->Printf( PRINT_ALL, "%i ", shader->numUnfoggedPasses );

		if (shader->lightmapIndex[0] || shader->lightmapIndex[1] || shader->lightmapIndex[2] || shader->lightmapIndex[3] ) {
			ri->Printf (PRINT_ALL, "L ");
		} else {
			ri->Printf (PRINT_ALL, "  ");
		}

		if ( shader->explicitlyDefined ) {
			ri->Printf( PRINT_ALL, "E " );
		} else {
			ri->Printf( PRINT_ALL, "  " );
		}

		if ( shader->optimalStageIteratorFunc == RB_StageIteratorGeneric ) {
			ri->Printf( PRINT_ALL, "gen " );
		} else if ( shader->optimalStageIteratorFunc == RB_StageIteratorSky ) {
			ri->Printf( PRINT_ALL, "sky " );
		} else {
			ri->Printf( PRINT_ALL, "    " );
		}

		if ( shader->defaultShader ) {
			ri->Printf (PRINT_ALL,  ": %s (DEFAULTED)\n", shader->name);
		} else {
			ri->Printf (PRINT_ALL,  ": %s\n", shader->name);
		}
		count++;
	}
	ri->Printf (PRINT_ALL, "%i total shaders\n", count);
	ri->Printf (PRINT_ALL, "------------------\n");
}

/*
====================
ScanAndLoadShaderFiles

Finds and loads all .shader files, combining them into
a single large text block that can be scanned for shader names
=====================
*/
#define	MAX_SHADER_FILES	4096
static void ScanAndLoadShaderFiles( void )
{
	char **shaderFiles;
	char *buffers[MAX_SHADER_FILES];
	const char *p;
	int numShaderFiles;
	int i;
	char *oldp, *token, *hashMem, *textEnd;
	int shaderTextHashTableSizes[MAX_SHADERTEXT_HASH], hash, size;
	char shaderName[MAX_QPATH];
	int shaderLine;

	long sum = 0, summand;
	// scan for shader files
	shaderFiles = ri->FS_ListFiles( "shaders", ".shader", &numShaderFiles );

	if ( !shaderFiles || !numShaderFiles )
	{
		ri->Printf( PRINT_WARNING, "WARNING: no shader files found\n" );
		return;
	}

	if ( numShaderFiles > MAX_SHADER_FILES ) {
		numShaderFiles = MAX_SHADER_FILES;
	}

	// load and parse shader files
	for ( i = 0; i < numShaderFiles; i++ )
	{
		char filename[MAX_QPATH];

		// look for a .mtr file first
		{
			char *ext;
			Com_sprintf( filename, sizeof( filename ), "shaders/%s", shaderFiles[i] );
			if ( (ext = strrchr(filename, '.')) )
			{
				strcpy(ext, ".mtr");
			}

			if ( ri->FS_ReadFile( filename, NULL ) <= 0 )
			{
				Com_sprintf( filename, sizeof( filename ), "shaders/%s", shaderFiles[i] );
			}
		}

		ri->Printf( PRINT_DEVELOPER, "...loading '%s'\n", filename );
		summand = ri->FS_ReadFile( filename, (void **)&buffers[i] );

		if ( !buffers[i] )
			ri->Error( ERR_DROP, "Couldn't load %s", filename );

		// Do a simple check on the shader structure in that file to make sure one bad shader file cannot fuck up all other shaders.
		p = buffers[i];
		COM_BeginParseSession(filename);
		while(1)
		{
			token = COM_ParseExt(&p, qtrue);

			if(!*token)
				break;

			Q_strncpyz(shaderName, token, sizeof(shaderName));
			shaderLine = COM_GetCurrentParseLine();

			token = COM_ParseExt(&p, qtrue);
			if(token[0] != '{' || token[1] != '\0')
			{
				ri->Printf(PRINT_WARNING, "WARNING: Ignoring shader file %s. Shader \"%s\" on line %d missing opening brace",
							filename, shaderName, shaderLine);
				if (token[0])
				{
					ri->Printf(PRINT_WARNING, " (found \"%s\" on line %d)", token, COM_GetCurrentParseLine());
				}
				ri->Printf(PRINT_WARNING, ".\n");
				ri->FS_FreeFile(buffers[i]);
				buffers[i] = NULL;
				break;
			}

			if(!SkipBracedSection_Depth(&p, 1))
			{
				ri->Printf(PRINT_WARNING, "WARNING: Ignoring shader file %s. Shader \"%s\" on line %d missing closing brace.\n",
							filename, shaderName, shaderLine);
				ri->FS_FreeFile(buffers[i]);
				buffers[i] = NULL;
				break;
			}
		}


		if (buffers[i])
			sum += summand;
	}

	// build single large buffer
	s_shaderText = (char *)ri->Hunk_Alloc( sum + numShaderFiles*2, h_low );
	s_shaderText[ 0 ] = '\0';
	textEnd = s_shaderText;

	// free in reverse order, so the temp files are all dumped
	for ( i = numShaderFiles - 1; i >= 0 ; i-- )
	{
		if ( !buffers[i] )
			continue;

		strcat( textEnd, buffers[i] );
		strcat( textEnd, "\n" );
		textEnd += strlen( textEnd );
		ri->FS_FreeFile( buffers[i] );
	}

	COM_Compress( s_shaderText );

	// free up memory
	ri->FS_FreeFileList( shaderFiles );

	Com_Memset(shaderTextHashTableSizes, 0, sizeof(shaderTextHashTableSizes));
	size = 0;

	p = s_shaderText;
	// look for shader names
	while ( 1 ) {
		token = COM_ParseExt( &p, qtrue );
		if ( token[0] == 0 ) {
			break;
		}

		hash = generateHashValue(token, MAX_SHADERTEXT_HASH);
		shaderTextHashTableSizes[hash]++;
		size++;
		SkipBracedSection_NoDepth(&p);
	}

	size += MAX_SHADERTEXT_HASH;

	hashMem = (char *)ri->Hunk_Alloc( size * sizeof(char *), h_low );

	for (i = 0; i < MAX_SHADERTEXT_HASH; i++) {
		shaderTextHashTable[i] = (char **) hashMem;
		hashMem = ((char *) hashMem) + ((shaderTextHashTableSizes[i] + 1) * sizeof(char *));
	}

	Com_Memset(shaderTextHashTableSizes, 0, sizeof(shaderTextHashTableSizes));

	p = s_shaderText;
	// look for shader names
	while ( 1 ) {
		oldp = (char *)p;
		token = COM_ParseExt( &p, qtrue );
		if ( token[0] == 0 ) {
			break;
		}

		hash = generateHashValue(token, MAX_SHADERTEXT_HASH);
		shaderTextHashTable[hash][shaderTextHashTableSizes[hash]++] = oldp;

		SkipBracedSection_NoDepth(&p);
	}

	return;

}

/*
====================
CreateInternalShaders
====================
*/
static void CreateInternalShaders( void ) {
	tr.numShaders = 0;

	// init the default shader
	Com_Memset( &shader, 0, sizeof( shader ) );
	Com_Memset( &stages, 0, sizeof( stages ) );

	Q_strncpyz( shader.name, "<default>", sizeof( shader.name ) );

	Com_Memcpy (shader.lightmapIndex, lightmapsNone, sizeof (shader.lightmapIndex));
	for ( int i = 0 ; i < MAX_SHADER_STAGES ; i++ ) {
		stages[i].bundle[0].texMods = texMods[i];
	}

	stages[0].bundle[0].image[0] = tr.defaultImage;
	stages[0].active = qtrue;
	stages[0].stateBits = GLS_DEFAULT;
	tr.defaultShader = FinishShader();

	// shadow shader is just a marker
	Q_strncpyz( shader.name, "<stencil shadow>", sizeof( shader.name ) );
	shader.sort = SS_BANNER; //SS_STENCIL_SHADOW;
	tr.shadowShader = FinishShader();

	// distortion shader is just a marker
	Q_strncpyz( shader.name, "internal_distortion", sizeof( shader.name ) );
	shader.sort = SS_BLEND0;
	shader.defaultShader = qfalse;
	tr.distortionShader = FinishShader();
	shader.defaultShader = qtrue;
}

static void CreateExternalShaders( void ) {
	tr.projectionShadowShader = R_FindShader( "projectionShadow", lightmapsNone, stylesDefault, qtrue );
	tr.flareShader = R_FindShader( "flareShader", lightmapsNone, stylesDefault, qtrue );

	// Hack to make fogging work correctly on flares. Fog colors are calculated
	// in tr_flare.c already.
	if(!tr.flareShader->defaultShader)
	{
		int index;

		for(index = 0; index < tr.flareShader->numUnfoggedPasses; index++)
		{
			tr.flareShader->stages[index]->adjustColorsForFog = ACFF_NONE;
			tr.flareShader->stages[index]->stateBits |= GLS_DEPTHTEST_DISABLE;
		}
	}

	tr.sunShader = R_FindShader( "sun", lightmapsNone, stylesDefault, qtrue );

	tr.sunFlareShader = R_FindShader( "gfx/2d/sunflare", lightmapsNone, stylesDefault, qtrue);

	// HACK: if sunflare is missing, make one using the flare image or dlight image
	if (tr.sunFlareShader->defaultShader)
	{
		image_t *image;

		if (!tr.flareShader->defaultShader && tr.flareShader->stages[0] && tr.flareShader->stages[0]->bundle[0].image[0])
			image = tr.flareShader->stages[0]->bundle[0].image[0];
		else
			image = tr.dlightImage;

		Com_Memset( &shader, 0, sizeof( shader ) );
		Com_Memset( &stages, 0, sizeof( stages ) );

		Q_strncpyz( shader.name, "gfx/2d/sunflare", sizeof( shader.name ) );

		Com_Memcpy (shader.lightmapIndex, lightmapsNone, sizeof (shader.lightmapIndex));
		stages[0].bundle[0].image[0] = image;
		stages[0].active = qtrue;
		stages[0].stateBits = GLS_DEFAULT;
		tr.sunFlareShader = FinishShader();
	}

}

/*
==================
R_InitShaders
==================
*/
void R_InitShaders( qboolean server ) {
	ri->Printf( PRINT_ALL, "Initializing Shaders\n" );

	Com_Memset(hashTable, 0, sizeof(hashTable));

	if ( !server )
	{
		CreateInternalShaders();

		ScanAndLoadShaderFiles();

		CreateExternalShaders();
	}
}

/*
=============================
JEDI KNIGHT GALAXIES
=============================
*/

// Replacement code for hacks in jkg_wpindicators.c --eez
void R_OverrideShaderFrame(qhandle_t shader, int desiredFrame, int time)
{
	shader_t* thisShader = tr.shaders[shader];
	// WTF hack here...
	thisShader->frameOverride = desiredFrame;
	if (thisShader->next != nullptr && !Q_stricmp(thisShader->next->name, thisShader->name)) {
		thisShader->next->frameOverride = desiredFrame;
	}
}
