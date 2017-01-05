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
// tr_map.c

#include "tr_local.h"

extern char currentMapName[128];

extern void R_LoadMapInfo ( void );

/*

Loads and prepares a map file for scene rendering.

A single entry point:

void RE_LoadWorldMap( const char *name );

*/

world_t				s_worldData;
static	byte		*fileBase;

int			c_subdivisions;
int			c_gridVerts;

//===============================================================================

static void HSVtoRGB( float h, float s, float v, float rgb[3] )
{
	int i;
	float f;
	float p, q, t;

	h *= 5;

	i = floor( h );
	f = h - i;

	p = v * ( 1 - s );
	q = v * ( 1 - s * f );
	t = v * ( 1 - s * ( 1 - f ) );

	switch ( i )
	{
	case 0:
		rgb[0] = v;
		rgb[1] = t;
		rgb[2] = p;
		break;
	case 1:
		rgb[0] = q;
		rgb[1] = v;
		rgb[2] = p;
		break;
	case 2:
		rgb[0] = p;
		rgb[1] = v;
		rgb[2] = t;
		break;
	case 3:
		rgb[0] = p;
		rgb[1] = q;
		rgb[2] = v;
		break;
	case 4:
		rgb[0] = t;
		rgb[1] = p;
		rgb[2] = v;
		break;
	case 5:
		rgb[0] = v;
		rgb[1] = p;
		rgb[2] = q;
		break;
	}
}

/*
===============
R_ColorShiftLightingBytes

===============
*/
static	void R_ColorShiftLightingBytes( byte in[4], byte out[4] ) {
	int		shift, r, g, b;

	// shift the color data based on overbright range
	shift = r_mapOverBrightBits->integer - tr.overbrightBits;

	// shift the data based on overbright range
	r = in[0] << shift;
	g = in[1] << shift;
	b = in[2] << shift;
	
	// normalize by color instead of saturating to white
	if ( ( r | g | b ) > 255 ) {
		int		max;

		max = r > g ? r : g;
		max = max > b ? max : b;
		r = r * 255 / max;
		g = g * 255 / max;
		b = b * 255 / max;
	}

	out[0] = r;
	out[1] = g;
	out[2] = b;
	out[3] = in[3];
}


/*
===============
R_ColorShiftLightingFloats

===============
*/
static void R_ColorShiftLightingFloats(float in[4], float out[4], float scale )
{
	float r, g, b;

	scale *= pow(2.0f, r_mapOverBrightBits->integer - tr.overbrightBits);

	r = in[0] * scale;
	g = in[1] * scale;
	b = in[2] * scale;

	if ( r > 1.0f || g > 1.0f || b > 1.0f )
	{
		float high = max (max (r, g), b);

		r /= high;
		g /= high;
		b /= high;
	}

	out[0] = r;
	out[1] = g;
	out[2] = b;
	out[3] = in[3];
}


// Modified from http://graphicrants.blogspot.jp/2009/04/rgbm-color-encoding.html
void ColorToRGBM(const vec3_t color, unsigned char rgbm[4])
{
	vec3_t          sample;
	float			maxComponent;

	VectorCopy(color, sample);

	maxComponent = MAX(sample[0], sample[1]);
	maxComponent = MAX(maxComponent, sample[2]);
	maxComponent = CLAMP(maxComponent, 1.0f/255.0f, 1.0f);

	rgbm[3] = (unsigned char) ceil(maxComponent * 255.0f);
	maxComponent = 255.0f / rgbm[3];

	VectorScale(sample, maxComponent, sample);

	rgbm[0] = (unsigned char) (sample[0] * 255);
	rgbm[1] = (unsigned char) (sample[1] * 255);
	rgbm[2] = (unsigned char) (sample[2] * 255);
}

void ColorToRGBA16F(const vec3_t color, unsigned short rgba16f[4])
{
	rgba16f[0] = FloatToHalf(color[0]);
	rgba16f[1] = FloatToHalf(color[1]);
	rgba16f[2] = FloatToHalf(color[2]);
	rgba16f[3] = FloatToHalf(1.0f);
}


/*
===============
R_LoadLightmaps

===============
*/
#define	DEFAULT_LIGHTMAP_SIZE	128
#define MAX_LIGHTMAP_PAGES 2
static	void R_LoadLightmaps( lump_t *l, lump_t *surfs ) {
	byte		*buf, *buf_p;
	dsurface_t  *surf;
	int			len;
	byte		*image;
	int			i, j, numLightmaps, textureInternalFormat = 0;
	float maxIntensity = 0;
	double sumIntensity = 0;

	len = l->filelen;
	if ( !len ) {
		return;
	}
	buf = fileBase + l->fileofs;

	// we are about to upload textures
	R_IssuePendingRenderCommands();

	tr.lightmapSize = DEFAULT_LIGHTMAP_SIZE;
	numLightmaps = len / (tr.lightmapSize * tr.lightmapSize * 3);

	// check for deluxe mapping
	if (numLightmaps <= 1)
	{
		tr.worldDeluxeMapping = qfalse;
	}
	else
	{
		tr.worldDeluxeMapping = qtrue;

		// Check that none of the deluxe maps are referenced by any of the map surfaces.
		for( i = 0, surf = (dsurface_t *)(fileBase + surfs->fileofs);
			tr.worldDeluxeMapping && i < surfs->filelen / sizeof(dsurface_t);
			i++, surf++ ) {
			for ( int j = 0; j < MAXLIGHTMAPS; j++ )
			{
				int lightmapNum = LittleLong( surf->lightmapNum[j] );

				if ( lightmapNum >= 0 && (lightmapNum & 1) != 0 ) {
					tr.worldDeluxeMapping = qfalse;
					break;
				}
			}
		}
	}

	image = (byte *)Z_Malloc(tr.lightmapSize * tr.lightmapSize * 4 * 2, TAG_BSP); // don't want to use TAG_BSP :< but it's the only one that fits

	if (tr.worldDeluxeMapping)
		numLightmaps >>= 1;

	if (r_mergeLightmaps->integer && numLightmaps >= 1024 )
	{
		// FIXME: fat light maps don't support more than 1024 light maps
		ri->Printf(PRINT_WARNING, "WARNING: number of lightmaps > 1024\n");
		numLightmaps = 1024;
	}

	// use fat lightmaps of an appropriate size
	if (r_mergeLightmaps->integer)
	{
		tr.fatLightmapSize = 512;
		tr.fatLightmapStep = tr.fatLightmapSize / tr.lightmapSize;

		// at most MAX_LIGHTMAP_PAGES
		while (tr.fatLightmapStep * tr.fatLightmapStep * MAX_LIGHTMAP_PAGES < numLightmaps && tr.fatLightmapSize != glConfig.maxTextureSize )
		{
			tr.fatLightmapSize <<= 1;
			tr.fatLightmapStep = tr.fatLightmapSize / tr.lightmapSize;
		}

		tr.numLightmaps = numLightmaps / (tr.fatLightmapStep * tr.fatLightmapStep);

		if (numLightmaps % (tr.fatLightmapStep * tr.fatLightmapStep) != 0)
			tr.numLightmaps++;
	}
	else
	{
		tr.numLightmaps = numLightmaps;
	}

	tr.lightmaps = (image_t **)ri->Hunk_Alloc( tr.numLightmaps * sizeof(image_t *), h_low );

	if (tr.worldDeluxeMapping)
	{
		tr.deluxemaps = (image_t **)ri->Hunk_Alloc( tr.numLightmaps * sizeof(image_t *), h_low );
	}

	if (glRefConfig.floatLightmap)
		textureInternalFormat = GL_RGBA16F;
	else
		textureInternalFormat = GL_RGBA8;

	if (r_mergeLightmaps->integer)
	{

		for (i = 0; i < tr.numLightmaps; i++)
		{
			tr.lightmaps[i] = R_CreateImage(va("_fatlightmap%d", i), NULL, tr.fatLightmapSize, tr.fatLightmapSize, IMGTYPE_COLORALPHA, IMGFLAG_NOLIGHTSCALE | IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE, textureInternalFormat );

			if (tr.worldDeluxeMapping)
			{
				tr.deluxemaps[i] = R_CreateImage(va("_fatdeluxemap%d", i), NULL, tr.fatLightmapSize, tr.fatLightmapSize, IMGTYPE_DELUXE, IMGFLAG_NOLIGHTSCALE | IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE, 0 );
			}
		}
	}

	for(i = 0; i < numLightmaps; i++)
	{
		int xoff = 0, yoff = 0;
		int lightmapnum = i;
		// expand the 24 bit on-disk to 32 bit

		if (r_mergeLightmaps->integer)
		{
			int lightmaponpage = i % (tr.fatLightmapStep * tr.fatLightmapStep);
			xoff = (lightmaponpage % tr.fatLightmapStep) * tr.lightmapSize;
			yoff = (lightmaponpage / tr.fatLightmapStep) * tr.lightmapSize;

			lightmapnum /= (tr.fatLightmapStep * tr.fatLightmapStep);
		}

		// if (tr.worldLightmapping)
		{
			char filename[MAX_QPATH];
			byte *hdrLightmap = NULL;
			int size = 0;

			// look for hdr lightmaps
			if (r_hdr->integer)
			{
				Com_sprintf( filename, sizeof( filename ), "maps/%s/lm_%04d.hdr", s_worldData.baseName, i * (tr.worldDeluxeMapping ? 2 : 1) );
				//ri->Printf(PRINT_ALL, "looking for %s\n", filename);

				size = ri->FS_ReadFile(filename, (void **)&hdrLightmap);
			}

			if (hdrLightmap)
			{
				byte *p = hdrLightmap;
				//ri->Printf(PRINT_ALL, "found!\n");
				
				/* FIXME: don't just skip over this header and actually parse it */
				while (size && !(*p == '\n' && *(p+1) == '\n'))
				{
					size--;
					p++;
				}

				if (!size)
					ri->Error(ERR_DROP, "Bad header for %s!", filename);

				size -= 2;
				p += 2;
				
				while (size && !(*p == '\n'))
				{
					size--;
					p++;
				}

				size--;
				p++;

				buf_p = (byte *)p;

#if 0 // HDRFILE_RGBE
				if (size != tr.lightmapSize * tr.lightmapSize * 4)
					ri->Error(ERR_DROP, "Bad size for %s (%i)!", filename, size);
#else // HDRFILE_FLOAT
				if (size != tr.lightmapSize * tr.lightmapSize * 12)
					ri->Error(ERR_DROP, "Bad size for %s (%i)!", filename, size);
#endif
			}
			else
			{
				if (tr.worldDeluxeMapping)
					buf_p = buf + (i * 2) * tr.lightmapSize * tr.lightmapSize * 3;
				else
					buf_p = buf + i * tr.lightmapSize * tr.lightmapSize * 3;
			}


			for ( j = 0 ; j < tr.lightmapSize * tr.lightmapSize; j++ ) 
			{
				if (hdrLightmap)
				{
					vec4_t color;

#if 0 // HDRFILE_RGBE
					float exponent = exp2(buf_p[j*4+3] - 128);

					color[0] = buf_p[j*4+0] * exponent;
					color[1] = buf_p[j*4+1] * exponent;
					color[2] = buf_p[j*4+2] * exponent;
#else // HDRFILE_FLOAT
					memcpy(color, &buf_p[j*12], 12);

					color[0] = LittleFloat(color[0]);
					color[1] = LittleFloat(color[1]);
					color[2] = LittleFloat(color[2]);
#endif
					color[3] = 1.0f;

					R_ColorShiftLightingFloats(color, color, 1.0f/255.0f);

					if (glRefConfig.floatLightmap)
						ColorToRGBA16F(color, (unsigned short *)(&image[j*8]));
					else
						ColorToRGBM(color, &image[j*4]);
				}
				else if (glRefConfig.floatLightmap)
				{
					vec4_t color;

					//hack: convert LDR lightmap to HDR one
					color[0] = MAX(buf_p[j*3+0], 0.499f);
					color[1] = MAX(buf_p[j*3+1], 0.499f);
					color[2] = MAX(buf_p[j*3+2], 0.499f);

					// if under an arbitrary value (say 12) grey it out
					// this prevents weird splotches in dimly lit areas
					if (color[0] + color[1] + color[2] < 12.0f)
					{
						float avg = (color[0] + color[1] + color[2]) * 0.3333f;
						color[0] = avg;
						color[1] = avg;
						color[2] = avg;
					}
					color[3] = 1.0f;

					R_ColorShiftLightingFloats(color, color, 1.0f/255.0f);

					ColorToRGBA16F(color, (unsigned short *)(&image[j*8]));
				}
				else
				{
					if ( r_lightmap->integer == 2 )
					{	// color code by intensity as development tool	(FIXME: check range)
						float r = buf_p[j*3+0];
						float g = buf_p[j*3+1];
						float b = buf_p[j*3+2];
						float intensity;
						float out[3] = {0.0, 0.0, 0.0};

						intensity = 0.33f * r + 0.685f * g + 0.063f * b;

						if ( intensity > 255 )
							intensity = 1.0f;
						else
							intensity /= 255.0f;

						if ( intensity > maxIntensity )
							maxIntensity = intensity;

						HSVtoRGB( intensity, 1.00, 0.50, out );

						image[j*4+0] = out[0] * 255;
						image[j*4+1] = out[1] * 255;
						image[j*4+2] = out[2] * 255;
						image[j*4+3] = 255;

						sumIntensity += intensity;
					}
					else
					{
						R_ColorShiftLightingBytes( &buf_p[j*3], &image[j*4] );
						image[j*4+3] = 255;
					}
				}
			}

			if (r_mergeLightmaps->integer)
				R_UpdateSubImage(tr.lightmaps[lightmapnum], image, xoff, yoff, tr.lightmapSize, tr.lightmapSize);
			else
				tr.lightmaps[i] = R_CreateImage(va("*lightmap%d", i), image, tr.lightmapSize, tr.lightmapSize, IMGTYPE_COLORALPHA, IMGFLAG_NOLIGHTSCALE | IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE, textureInternalFormat );

			if (hdrLightmap)
				ri->FS_FreeFile(hdrLightmap);
		}

		if (tr.worldDeluxeMapping)
		{
			buf_p = buf + (i * 2 + 1) * tr.lightmapSize * tr.lightmapSize * 3;


			for ( j = 0 ; j < tr.lightmapSize * tr.lightmapSize; j++ ) {
				image[j*4+0] = buf_p[j*3+0];
				image[j*4+1] = buf_p[j*3+1];
				image[j*4+2] = buf_p[j*3+2];

				// make 0,0,0 into 127,127,127
				if ((image[j*4+0] == 0) && (image[j*4+1] == 0) && (image[j*4+2] == 0))
				{
					image[j*4+0] =
					image[j*4+1] =
					image[j*4+2] = 127;
				}

				image[j*4+3] = 255;
			}

			if (r_mergeLightmaps->integer)
			{
				R_UpdateSubImage(tr.deluxemaps[lightmapnum], image, xoff, yoff, tr.lightmapSize, tr.lightmapSize );
			}
			else
			{
				tr.deluxemaps[i] = R_CreateImage(va("*deluxemap%d", i), image, tr.lightmapSize, tr.lightmapSize, IMGTYPE_DELUXE, IMGFLAG_NOLIGHTSCALE | IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE, 0 );
			}
		}
	}

	if ( r_lightmap->integer == 2 )	{
		ri->Printf( PRINT_ALL, "Brightest lightmap value: %d\n", ( int ) ( maxIntensity * 255 ) );
	}

	Z_Free(image);
}


static float FatPackU(float input, int lightmapnum)
{
	if (lightmapnum < 0)
		return input;

	if (tr.worldDeluxeMapping)
		lightmapnum >>= 1;

	if(tr.fatLightmapSize > 0)
	{
		int             x;

		lightmapnum %= (tr.fatLightmapStep * tr.fatLightmapStep);

		x = lightmapnum % tr.fatLightmapStep;

		return (input / ((float)tr.fatLightmapStep)) + ((1.0 / ((float)tr.fatLightmapStep)) * (float)x);
	}

	return input;
}

static float FatPackV(float input, int lightmapnum)
{
	if (lightmapnum < 0)
		return input;

	if (tr.worldDeluxeMapping)
		lightmapnum >>= 1;

	if(tr.fatLightmapSize > 0)
	{
		int             y;

		lightmapnum %= (tr.fatLightmapStep * tr.fatLightmapStep);

		y = lightmapnum / tr.fatLightmapStep;

		return (input / ((float)tr.fatLightmapStep)) + ((1.0 / ((float)tr.fatLightmapStep)) * (float)y);
	}

	return input;
}


static int FatLightmap(int lightmapnum)
{
	if (lightmapnum < 0)
		return lightmapnum;

	if (tr.worldDeluxeMapping)
		lightmapnum >>= 1;

	if (tr.fatLightmapSize > 0)
	{
		return lightmapnum / (tr.fatLightmapStep * tr.fatLightmapStep);
	}
	
	return lightmapnum;
}

/*
=================
RE_SetWorldVisData

This is called by the clipmodel subsystem so we can share the 1.8 megs of
space in big maps...
=================
*/
void		RE_SetWorldVisData( const byte *vis ) {
	tr.externalVisData = vis;
}


/*
=================
R_LoadVisibility
=================
*/
static	void R_LoadVisibility( lump_t *l ) {
#if 1
	int		len;
	byte	*buf;

	len = l->filelen;
	if ( !len ) {
		return;
	}
	buf = fileBase + l->fileofs;

	s_worldData.numClusters = LittleLong( ((int *)buf)[0] );
	s_worldData.clusterBytes = LittleLong( ((int *)buf)[1] );

	// CM_Load should have given us the vis data to share, so
	// we don't need to allocate another copy
	if ( tr.externalVisData ) {
		s_worldData.vis = tr.externalVisData;
	} else {
		byte	*dest;

		dest = (byte *)ri->Hunk_Alloc( len - 8, h_low );
		Com_Memcpy( dest, buf + 8, len - 8 );
		s_worldData.vis = dest;
	}
#else
	s_worldData.vis = (byte *)ri->CM_GetVisibilityData();
	s_worldData.numClusters = (int)ri->CM_GetVisibilityDataClusterCount();
	s_worldData.clusterBytes = (int)ri->CM_GetVisibilityDataClusterBytesCount();
#endif
}

//===============================================================================


/*
===============
ShaderForShaderNum
===============
*/
static shader_t *ShaderForShaderNum( int shaderNum, const int *lightmapNums, const byte *lightmapStyles, const byte *vertexStyles ) {
	shader_t	*shader;
	dshader_t	*dsh;
	const byte	*styles = lightmapStyles;

	int _shaderNum = LittleLong( shaderNum );
	if ( _shaderNum < 0 || _shaderNum >= s_worldData.numShaders ) {
		ri->Error( ERR_DROP, "ShaderForShaderNum: bad num %i", _shaderNum );
	}
	dsh = &s_worldData.shaders[ _shaderNum ];

	if ( lightmapNums[0] == LIGHTMAP_BY_VERTEX ) {
		styles = vertexStyles;
	}

	if ( r_vertexLight->integer ) {
		lightmapNums = lightmapsVertex;
		styles = vertexStyles;
	}

	if ( r_fullbright->integer ) {
		lightmapNums = lightmapsFullBright;
	}

	shader = R_FindShader( dsh->shader, lightmapNums, styles, qtrue );

	// if the shader had errors, just use default shader
	if ( shader->defaultShader ) {
		return tr.defaultShader;
	}

	return shader;
}

/*
===============
ParseFace
===============
*/
static void ParseFace( dsurface_t *ds, drawVert_t *verts, float *hdrVertColors, msurface_t *surf, int *indexes  ) {
	int			i, j;
	srfBspSurface_t	*cv;
	glIndex_t  *tri;
	int			numVerts, numIndexes, badTriangles;
	int realLightmapNum[MAXLIGHTMAPS];

	for ( j = 0; j < MAXLIGHTMAPS; j++ )
	{
		realLightmapNum[j] = FatLightmap (LittleLong (ds->lightmapNum[j]));
	}

	// get fog volume
	surf->fogIndex = LittleLong( ds->fogNum ) + 1;

	// get shader value
	surf->shader = ShaderForShaderNum( ds->shaderNum, realLightmapNum, ds->lightmapStyles, ds->vertexStyles);
	if ( r_singleShader->integer && !surf->shader->isSky ) {
		surf->shader = tr.defaultShader;
	}

	numVerts = LittleLong(ds->numVerts);
	if (numVerts > MAX_FACE_POINTS) {
		ri->Printf( PRINT_WARNING, "WARNING: MAX_FACE_POINTS exceeded: %i\n", numVerts);
		numVerts = MAX_FACE_POINTS;
		surf->shader = tr.defaultShader;
	}

	numIndexes = LittleLong(ds->numIndexes);

	//cv = ri->Hunk_Alloc(sizeof(*cv), h_low);
	cv = (srfBspSurface_t *)surf->data;
	cv->surfaceType = SF_FACE;

	cv->numIndexes = numIndexes;
	cv->indexes = (glIndex_t *)ri->Hunk_Alloc(numIndexes * sizeof(cv->indexes[0]), h_low);

	cv->numVerts = numVerts;
	cv->verts = (srfVert_t *)ri->Hunk_Alloc(numVerts * sizeof(cv->verts[0]), h_low);

	// copy vertexes
	surf->cullinfo.type = CULLINFO_PLANE | CULLINFO_BOX;
	ClearBounds(surf->cullinfo.bounds[0], surf->cullinfo.bounds[1]);
	verts += LittleLong(ds->firstVert);

	for(i = 0; i < numVerts; i++)
	{
		vec4_t color;

		for(j = 0; j < 3; j++)
		{
			cv->verts[i].xyz[j] = LittleFloat(verts[i].xyz[j]);
			cv->verts[i].normal[j] = LittleFloat(verts[i].normal[j]);
		}

		AddPointToBounds(cv->verts[i].xyz, surf->cullinfo.bounds[0], surf->cullinfo.bounds[1]);

		for(j = 0; j < 2; j++)
		{
			cv->verts[i].st[j] = LittleFloat(verts[i].st[j]);
		}

		for ( j = 0; j < MAXLIGHTMAPS; j++ )
		{
			cv->verts[i].lightmap[j][0] = FatPackU(LittleFloat(verts[i].lightmap[j][0]), realLightmapNum[j]);
			cv->verts[i].lightmap[j][1] = FatPackV(LittleFloat(verts[i].lightmap[j][1]), realLightmapNum[j]);

			if (hdrVertColors)
			{
				color[0] = hdrVertColors[(ds->firstVert + i) * 3    ];
				color[1] = hdrVertColors[(ds->firstVert + i) * 3 + 1];
				color[2] = hdrVertColors[(ds->firstVert + i) * 3 + 2];
			}
			else
			{
				//hack: convert LDR vertex colors to HDR
				if (r_hdr->integer)
				{
					color[0] = MAX(verts[i].color[j][0], 0.499f);
					color[1] = MAX(verts[i].color[j][1], 0.499f);
					color[2] = MAX(verts[i].color[j][2], 0.499f);
				}
				else
				{
					color[0] = verts[i].color[j][0];
					color[1] = verts[i].color[j][1];
					color[2] = verts[i].color[j][2];
				}

			}
			color[3] = verts[i].color[j][3] / 255.0f;

			R_ColorShiftLightingFloats( color, cv->verts[i].vertexColors[j], 1.0f / 255.0f );
		}
	}

	// copy triangles
	badTriangles = 0;
	indexes += LittleLong(ds->firstIndex);

	for(i = 0, tri = cv->indexes; i < numIndexes; i += 3, tri += 3)
	{
		for(j = 0; j < 3; j++)
		{
			tri[j] = LittleLong(indexes[i + j]);

			if(tri[j] >= numVerts)
			{
				ri->Error(ERR_DROP, "Bad index in face surface");
			}
		}

		if ((tri[0] == tri[1]) || (tri[1] == tri[2]) || (tri[0] == tri[2]))
		{
			tri -= 3;
			badTriangles++;
		}
	}

	if (badTriangles)
	{
		ri->Printf(PRINT_WARNING, "Face has bad triangles, originally shader %s %d tris %d verts, now %d tris\n", surf->shader->name, numIndexes / 3, numVerts, numIndexes / 3 - badTriangles);
		cv->numIndexes -= badTriangles * 3;
	}

	// take the plane information from the lightmap vector
	for ( i = 0 ; i < 3 ; i++ ) {
		cv->cullPlane.normal[i] = LittleFloat( ds->lightmapVecs[2][i] );
	}
	cv->cullPlane.dist = DotProduct( cv->verts[0].xyz, cv->cullPlane.normal );
	SetPlaneSignbits( &cv->cullPlane );
	cv->cullPlane.type = PlaneTypeForNormal( cv->cullPlane.normal );
	surf->cullinfo.plane = cv->cullPlane;

	surf->data = (surfaceType_t *)cv;

	// Calculate tangent spaces
	{
		srfVert_t      *dv[3];

		for(i = 0, tri = cv->indexes; i < numIndexes; i += 3, tri += 3)
		{
			dv[0] = &cv->verts[tri[0]];
			dv[1] = &cv->verts[tri[1]];
			dv[2] = &cv->verts[tri[2]];

			R_CalcTangentVectors(dv);
		}
	}
}


/*
===============
ParseMesh
===============
*/
static void ParseMesh ( dsurface_t *ds, drawVert_t *verts, float *hdrVertColors, msurface_t *surf ) {
	srfBspSurface_t	*grid;
	int				i, j;
	int				width, height, numPoints;
	srfVert_t points[MAX_PATCH_SIZE*MAX_PATCH_SIZE];
	vec3_t			bounds[2];
	vec3_t			tmpVec;
	static surfaceType_t	skipData = SF_SKIP;
	int realLightmapNum[MAXLIGHTMAPS];

	for ( j = 0; j < MAXLIGHTMAPS; j++ )
	{
		realLightmapNum[j] = FatLightmap (LittleLong (ds->lightmapNum[j]));
	}

	// get fog volume
	surf->fogIndex = LittleLong( ds->fogNum ) + 1;

	// get shader value
	surf->shader = ShaderForShaderNum( ds->shaderNum, realLightmapNum, ds->lightmapStyles, ds->vertexStyles );
	if ( r_singleShader->integer && !surf->shader->isSky ) {
		surf->shader = tr.defaultShader;
	}

	// we may have a nodraw surface, because they might still need to
	// be around for movement clipping
	if ( s_worldData.shaders[ LittleLong( ds->shaderNum ) ].surfaceFlags & SURF_NODRAW ) {
		surf->data = &skipData;
		return;
	}

	width = LittleLong( ds->patchWidth );
	height = LittleLong( ds->patchHeight );

	if(width < 0 || width > MAX_PATCH_SIZE || height < 0 || height > MAX_PATCH_SIZE)
		ri->Error(ERR_DROP, "ParseMesh: bad size");

	verts += LittleLong( ds->firstVert );
	numPoints = width * height;


	for(i = 0; i < numPoints; i++)
	{
		vec4_t color;

		for(j = 0; j < 3; j++)
		{
			points[i].xyz[j] = LittleFloat(verts[i].xyz[j]);
			points[i].normal[j] = LittleFloat(verts[i].normal[j]);
		}

		for(j = 0; j < 2; j++)
		{
			points[i].st[j] = LittleFloat(verts[i].st[j]);
		}

		for ( j = 0; j < MAXLIGHTMAPS; j++ )
		{
			points[i].lightmap[j][0] = FatPackU(LittleFloat(verts[i].lightmap[j][0]), realLightmapNum[j]);
			points[i].lightmap[j][1] = FatPackV(LittleFloat(verts[i].lightmap[j][1]), realLightmapNum[j]);

			if (hdrVertColors)
			{
				color[0] = hdrVertColors[(ds->firstVert + i) * 3    ];
				color[1] = hdrVertColors[(ds->firstVert + i) * 3 + 1];
				color[2] = hdrVertColors[(ds->firstVert + i) * 3 + 2];
			}
			else
			{
				//hack: convert LDR vertex colors to HDR
				if (r_hdr->integer)
				{
					color[0] = MAX(verts[i].color[j][0], 0.499f);
					color[1] = MAX(verts[i].color[j][1], 0.499f);
					color[2] = MAX(verts[i].color[j][2], 0.499f);
				}
				else
				{
					color[0] = verts[i].color[j][0];
					color[1] = verts[i].color[j][1];
					color[2] = verts[i].color[j][2];
				}
			}
			color[3] = verts[i].color[j][3] / 255.0f;

			R_ColorShiftLightingFloats( color, points[i].vertexColors[j], 1.0f / 255.0f );
		}
	}

	// pre-tesseleate
	grid = R_SubdividePatchToGrid( width, height, points );
	surf->data = (surfaceType_t *)grid;

	// copy the level of detail origin, which is the center
	// of the group of all curves that must subdivide the same
	// to avoid cracking
	for ( i = 0 ; i < 3 ; i++ ) {
		bounds[0][i] = LittleFloat( ds->lightmapVecs[0][i] );
		bounds[1][i] = LittleFloat( ds->lightmapVecs[1][i] );
	}
	VectorAdd( bounds[0], bounds[1], bounds[1] );
	VectorScale( bounds[1], 0.5f, grid->lodOrigin );
	VectorSubtract( bounds[0], grid->lodOrigin, tmpVec );
	grid->lodRadius = VectorLength( tmpVec );
}

/*
===============
ParseTriSurf
===============
*/
static void ParseTriSurf( dsurface_t *ds, drawVert_t *verts, float *hdrVertColors, msurface_t *surf, int *indexes ) {
	srfBspSurface_t *cv;
	glIndex_t  *tri;
	int             i, j;
	int             numVerts, numIndexes, badTriangles;

	// get fog volume
	surf->fogIndex = LittleLong( ds->fogNum ) + 1;

	// get shader
	surf->shader = ShaderForShaderNum( ds->shaderNum, lightmapsVertex, ds->lightmapStyles, ds->vertexStyles );
	if ( r_singleShader->integer && !surf->shader->isSky ) {
		surf->shader = tr.defaultShader;
	}

	numVerts = LittleLong(ds->numVerts);
	numIndexes = LittleLong(ds->numIndexes);

	//cv = (srfBspSurface_t *)ri->Hunk_Alloc(sizeof(*cv), h_low);
	cv = (srfBspSurface_t *)surf->data;
	cv->surfaceType = SF_TRIANGLES;

	cv->numIndexes = numIndexes;
	cv->indexes = (glIndex_t *)ri->Hunk_Alloc(numIndexes * sizeof(cv->indexes[0]), h_low);

	cv->numVerts = numVerts;
	cv->verts = (srfVert_t *)ri->Hunk_Alloc(numVerts * sizeof(cv->verts[0]), h_low);

	surf->data = (surfaceType_t *) cv;

	// copy vertexes
	surf->cullinfo.type = CULLINFO_BOX;
	ClearBounds(surf->cullinfo.bounds[0], surf->cullinfo.bounds[1]);
	verts += LittleLong(ds->firstVert);

	for(i = 0; i < numVerts; i++)
	{
		vec4_t color;

		for(j = 0; j < 3; j++)
		{
			cv->verts[i].xyz[j] = LittleFloat(verts[i].xyz[j]);
			cv->verts[i].normal[j] = LittleFloat(verts[i].normal[j]);
		}
		/*
		switch ( LittleLong( ds->surfaceType ) ) {
			case MST_FOLIAGE:
				ri->Printf(PRINT_WARNING, "Loaded foliage surface at %f %f %f (%f %f %f) org: %f %f %f - %f %f %f.\n", cv->verts[i].xyz[0], cv->verts[i].xyz[1], cv->verts[i].xyz[2], verts[i].xyz[0], verts[i].xyz[1], verts[i].xyz[2]
				, cv->verts->xyz[0], cv->verts->xyz[1], cv->verts->xyz[2], cv->cullOrigin[0], cv->cullOrigin[1], cv->cullOrigin[2]);
				break;
			default:
				break;
		}
		*/
		AddPointToBounds( cv->verts[i].xyz, surf->cullinfo.bounds[0], surf->cullinfo.bounds[1] );

		for(j = 0; j < 2; j++)
		{
			cv->verts[i].st[j] = LittleFloat(verts[i].st[j]);
		}

		for ( j = 0; j < MAXLIGHTMAPS; j++ )
		{
			cv->verts[i].lightmap[j][0] = LittleFloat(verts[i].lightmap[j][0]);
			cv->verts[i].lightmap[j][1] = LittleFloat(verts[i].lightmap[j][1]);

			if (hdrVertColors)
			{
				color[0] = hdrVertColors[(ds->firstVert + i) * 3    ];
				color[1] = hdrVertColors[(ds->firstVert + i) * 3 + 1];
				color[2] = hdrVertColors[(ds->firstVert + i) * 3 + 2];
			}
			else
			{
				//hack: convert LDR vertex colors to HDR
				if (r_hdr->integer)
				{
					color[0] = MAX(verts[i].color[j][0], 0.499f);
					color[1] = MAX(verts[i].color[j][1], 0.499f);
					color[2] = MAX(verts[i].color[j][2], 0.499f);
				}
				else
				{
					color[0] = verts[i].color[j][0];
					color[1] = verts[i].color[j][1];
					color[2] = verts[i].color[j][2];
				}
			}
			color[3] = verts[i].color[j][3] / 255.0f;

			R_ColorShiftLightingFloats( color, cv->verts[i].vertexColors[j], 1.0f / 255.0f );
		}
	}

	// copy triangles
	badTriangles = 0;
	indexes += LittleLong(ds->firstIndex);
	for(i = 0, tri = cv->indexes; i < numIndexes; i += 3, tri += 3)
	{
		for(j = 0; j < 3; j++)
		{
			tri[j] = LittleLong(indexes[i + j]);

			if(tri[j] >= numVerts)
			{
				ri->Error(ERR_DROP, "Bad index in face surface");
			}
		}

		if ((tri[0] == tri[1]) || (tri[1] == tri[2]) || (tri[0] == tri[2]))
		{
			tri -= 3;
			badTriangles++;
		}
	}

	if (badTriangles)
	{
		ri->Printf(PRINT_WARNING, "Trisurf has bad triangles, originally shader %s %d tris %d verts, now %d tris\n", surf->shader->name, numIndexes / 3, numVerts, numIndexes / 3 - badTriangles);
		cv->numIndexes -= badTriangles * 3;
	}

	// Calculate tangent spaces
	{
		srfVert_t      *dv[3];

		for(i = 0, tri = cv->indexes; i < numIndexes; i += 3, tri += 3)
		{
			dv[0] = &cv->verts[tri[0]];
			dv[1] = &cv->verts[tri[1]];
			dv[2] = &cv->verts[tri[2]];
			
			//ri->Printf(PRINT_WARNING, "Loaded foliage surface at %f %f %f, %f %f %f, %f %f %f.\n", dv[0]->xyz[0], dv[0]->xyz[1], dv[0]->xyz[2], dv[1]->xyz[0], dv[1]->xyz[1], dv[1]->xyz[2], dv[2]->xyz[0], dv[2]->xyz[1], dv[2]->xyz[2]);

			R_CalcTangentVectors(dv);
		}
	}
}

/*
===============
ParseFlare
===============
*/
static void ParseFlare( dsurface_t *ds, drawVert_t *verts, msurface_t *surf, int *indexes ) {
	srfFlare_t		*flare;
	int				i;

	// get fog volume
	surf->fogIndex = LittleLong( ds->fogNum ) + 1;

	// get shader
	surf->shader = ShaderForShaderNum( ds->shaderNum, lightmapsVertex, ds->lightmapStyles, ds->vertexStyles );
	if ( r_singleShader->integer && !surf->shader->isSky ) {
		surf->shader = tr.defaultShader;
	}

	//flare = ri->Hunk_Alloc( sizeof( *flare ), h_low );
	flare = (srfFlare_t *)surf->data;
	flare->surfaceType = SF_FLARE;

	surf->data = (surfaceType_t *)flare;

	for ( i = 0 ; i < 3 ; i++ ) {
		flare->origin[i] = LittleFloat( ds->lightmapOrigin[i] );
		flare->color[i] = LittleFloat( ds->lightmapVecs[0][i] );
		flare->normal[i] = LittleFloat( ds->lightmapVecs[2][i] );
	}
}


/*
=================
R_MergedWidthPoints

returns true if there are grid points merged on a width edge
=================
*/
int R_MergedWidthPoints(srfBspSurface_t *grid, int offset) {
	int i, j;

	for (i = 1; i < grid->width-1; i++) {
		for (j = i + 1; j < grid->width-1; j++) {
			if ( fabs(grid->verts[i + offset].xyz[0] - grid->verts[j + offset].xyz[0]) > .1) continue;
			if ( fabs(grid->verts[i + offset].xyz[1] - grid->verts[j + offset].xyz[1]) > .1) continue;
			if ( fabs(grid->verts[i + offset].xyz[2] - grid->verts[j + offset].xyz[2]) > .1) continue;
			return qtrue;
		}
	}
	return qfalse;
}

/*
=================
R_MergedHeightPoints

returns true if there are grid points merged on a height edge
=================
*/
int R_MergedHeightPoints(srfBspSurface_t *grid, int offset) {
	int i, j;

	for (i = 1; i < grid->height-1; i++) {
		for (j = i + 1; j < grid->height-1; j++) {
			if ( fabs(grid->verts[grid->width * i + offset].xyz[0] - grid->verts[grid->width * j + offset].xyz[0]) > .1) continue;
			if ( fabs(grid->verts[grid->width * i + offset].xyz[1] - grid->verts[grid->width * j + offset].xyz[1]) > .1) continue;
			if ( fabs(grid->verts[grid->width * i + offset].xyz[2] - grid->verts[grid->width * j + offset].xyz[2]) > .1) continue;
			return qtrue;
		}
	}
	return qfalse;
}

/*
=================
R_FixSharedVertexLodError_r

NOTE: never sync LoD through grid edges with merged points!

FIXME: write generalized version that also avoids cracks between a patch and one that meets half way?
=================
*/
void R_FixSharedVertexLodError_r( int start, srfBspSurface_t *grid1 ) {
	int j, k, l, m, n, offset1, offset2, touch;
	srfBspSurface_t *grid2;

	for ( j = start; j < s_worldData.numsurfaces; j++ ) {
		//
		grid2 = (srfBspSurface_t *) s_worldData.surfaces[j].data;
		// if this surface is not a grid
		if ( grid2->surfaceType != SF_GRID ) continue;
		// if the LOD errors are already fixed for this patch
		if ( grid2->lodFixed == 2 ) continue;
		// grids in the same LOD group should have the exact same lod radius
		if ( grid1->lodRadius != grid2->lodRadius ) continue;
		// grids in the same LOD group should have the exact same lod origin
		if ( grid1->lodOrigin[0] != grid2->lodOrigin[0] ) continue;
		if ( grid1->lodOrigin[1] != grid2->lodOrigin[1] ) continue;
		if ( grid1->lodOrigin[2] != grid2->lodOrigin[2] ) continue;
		//
		touch = qfalse;
		for (n = 0; n < 2; n++) {
			//
			if (n) offset1 = (grid1->height-1) * grid1->width;
			else offset1 = 0;
			if (R_MergedWidthPoints(grid1, offset1)) continue;
			for (k = 1; k < grid1->width-1; k++) {
				for (m = 0; m < 2; m++) {

					if (m) offset2 = (grid2->height-1) * grid2->width;
					else offset2 = 0;
					if (R_MergedWidthPoints(grid2, offset2)) continue;
					for ( l = 1; l < grid2->width-1; l++) {
					//
						if ( fabs(grid1->verts[k + offset1].xyz[0] - grid2->verts[l + offset2].xyz[0]) > .1) continue;
						if ( fabs(grid1->verts[k + offset1].xyz[1] - grid2->verts[l + offset2].xyz[1]) > .1) continue;
						if ( fabs(grid1->verts[k + offset1].xyz[2] - grid2->verts[l + offset2].xyz[2]) > .1) continue;
						// ok the points are equal and should have the same lod error
						grid2->widthLodError[l] = grid1->widthLodError[k];
						touch = qtrue;
					}
				}
				for (m = 0; m < 2; m++) {

					if (m) offset2 = grid2->width-1;
					else offset2 = 0;
					if (R_MergedHeightPoints(grid2, offset2)) continue;
					for ( l = 1; l < grid2->height-1; l++) {
					//
						if ( fabs(grid1->verts[k + offset1].xyz[0] - grid2->verts[grid2->width * l + offset2].xyz[0]) > .1) continue;
						if ( fabs(grid1->verts[k + offset1].xyz[1] - grid2->verts[grid2->width * l + offset2].xyz[1]) > .1) continue;
						if ( fabs(grid1->verts[k + offset1].xyz[2] - grid2->verts[grid2->width * l + offset2].xyz[2]) > .1) continue;
						// ok the points are equal and should have the same lod error
						grid2->heightLodError[l] = grid1->widthLodError[k];
						touch = qtrue;
					}
				}
			}
		}
		for (n = 0; n < 2; n++) {
			//
			if (n) offset1 = grid1->width-1;
			else offset1 = 0;
			if (R_MergedHeightPoints(grid1, offset1)) continue;
			for (k = 1; k < grid1->height-1; k++) {
				for (m = 0; m < 2; m++) {

					if (m) offset2 = (grid2->height-1) * grid2->width;
					else offset2 = 0;
					if (R_MergedWidthPoints(grid2, offset2)) continue;
					for ( l = 1; l < grid2->width-1; l++) {
					//
						if ( fabs(grid1->verts[grid1->width * k + offset1].xyz[0] - grid2->verts[l + offset2].xyz[0]) > .1) continue;
						if ( fabs(grid1->verts[grid1->width * k + offset1].xyz[1] - grid2->verts[l + offset2].xyz[1]) > .1) continue;
						if ( fabs(grid1->verts[grid1->width * k + offset1].xyz[2] - grid2->verts[l + offset2].xyz[2]) > .1) continue;
						// ok the points are equal and should have the same lod error
						grid2->widthLodError[l] = grid1->heightLodError[k];
						touch = qtrue;
					}
				}
				for (m = 0; m < 2; m++) {

					if (m) offset2 = grid2->width-1;
					else offset2 = 0;
					if (R_MergedHeightPoints(grid2, offset2)) continue;
					for ( l = 1; l < grid2->height-1; l++) {
					//
						if ( fabs(grid1->verts[grid1->width * k + offset1].xyz[0] - grid2->verts[grid2->width * l + offset2].xyz[0]) > .1) continue;
						if ( fabs(grid1->verts[grid1->width * k + offset1].xyz[1] - grid2->verts[grid2->width * l + offset2].xyz[1]) > .1) continue;
						if ( fabs(grid1->verts[grid1->width * k + offset1].xyz[2] - grid2->verts[grid2->width * l + offset2].xyz[2]) > .1) continue;
						// ok the points are equal and should have the same lod error
						grid2->heightLodError[l] = grid1->heightLodError[k];
						touch = qtrue;
					}
				}
			}
		}
		if (touch) {
			grid2->lodFixed = 2;
			R_FixSharedVertexLodError_r ( start, grid2 );
			//NOTE: this would be correct but makes things really slow
			//grid2->lodFixed = 1;
		}
	}
}

/*
=================
R_FixSharedVertexLodError

This function assumes that all patches in one group are nicely stitched together for the highest LoD.
If this is not the case this function will still do its job but won't fix the highest LoD cracks.
=================
*/
void R_FixSharedVertexLodError( void ) {
	int i;
	srfBspSurface_t *grid1;


	for ( i = 0; i < s_worldData.numsurfaces; i++ ) {
		//
		grid1 = (srfBspSurface_t *) s_worldData.surfaces[i].data;
		// if this surface is not a grid
		if ( grid1->surfaceType != SF_GRID )
			continue;
		//
		if ( grid1->lodFixed )
			continue;
		//
		grid1->lodFixed = 2;
		// recursively fix other patches in the same LOD group
		R_FixSharedVertexLodError_r( i + 1, grid1);
	}
}


/*
===============
R_StitchPatches
===============
*/
int R_StitchPatches( int grid1num, int grid2num ) {
	float *v1, *v2;
	srfBspSurface_t *grid1, *grid2;
	int k, l, m, n, offset1, offset2, row, column;

	grid1 = (srfBspSurface_t *) s_worldData.surfaces[grid1num].data;
	grid2 = (srfBspSurface_t *) s_worldData.surfaces[grid2num].data;
	for (n = 0; n < 2; n++) {
		//
		if (n) offset1 = (grid1->height-1) * grid1->width;
		else offset1 = 0;
		if (R_MergedWidthPoints(grid1, offset1))
			continue;
		for (k = 0; k < grid1->width-2; k += 2) {

			for (m = 0; m < 2; m++) {

				if ( grid2->width >= MAX_GRID_SIZE )
					break;
				if (m) offset2 = (grid2->height-1) * grid2->width;
				else offset2 = 0;
				for ( l = 0; l < grid2->width-1; l++) {
				//
					v1 = grid1->verts[k + offset1].xyz;
					v2 = grid2->verts[l + offset2].xyz;
					if ( fabs(v1[0] - v2[0]) > .1)
						continue;
					if ( fabs(v1[1] - v2[1]) > .1)
						continue;
					if ( fabs(v1[2] - v2[2]) > .1)
						continue;

					v1 = grid1->verts[k + 2 + offset1].xyz;
					v2 = grid2->verts[l + 1 + offset2].xyz;
					if ( fabs(v1[0] - v2[0]) > .1)
						continue;
					if ( fabs(v1[1] - v2[1]) > .1)
						continue;
					if ( fabs(v1[2] - v2[2]) > .1)
						continue;
					//
					v1 = grid2->verts[l + offset2].xyz;
					v2 = grid2->verts[l + 1 + offset2].xyz;
					if ( fabs(v1[0] - v2[0]) < .01 &&
							fabs(v1[1] - v2[1]) < .01 &&
							fabs(v1[2] - v2[2]) < .01)
						continue;
					//
					//ri->Printf( PRINT_ALL, "found highest LoD crack between two patches\n" );
					// insert column into grid2 right after after column l
					if (m) row = grid2->height-1;
					else row = 0;
					grid2 = R_GridInsertColumn( grid2, l+1, row,
									grid1->verts[k + 1 + offset1].xyz, grid1->widthLodError[k+1]);
					grid2->lodStitched = qfalse;
					s_worldData.surfaces[grid2num].data = (surfaceType_t *) grid2;
					return qtrue;
				}
			}
			for (m = 0; m < 2; m++) {

				if (grid2->height >= MAX_GRID_SIZE)
					break;
				if (m) offset2 = grid2->width-1;
				else offset2 = 0;
				for ( l = 0; l < grid2->height-1; l++) {
					//
					v1 = grid1->verts[k + offset1].xyz;
					v2 = grid2->verts[grid2->width * l + offset2].xyz;
					if ( fabs(v1[0] - v2[0]) > .1)
						continue;
					if ( fabs(v1[1] - v2[1]) > .1)
						continue;
					if ( fabs(v1[2] - v2[2]) > .1)
						continue;

					v1 = grid1->verts[k + 2 + offset1].xyz;
					v2 = grid2->verts[grid2->width * (l + 1) + offset2].xyz;
					if ( fabs(v1[0] - v2[0]) > .1)
						continue;
					if ( fabs(v1[1] - v2[1]) > .1)
						continue;
					if ( fabs(v1[2] - v2[2]) > .1)
						continue;
					//
					v1 = grid2->verts[grid2->width * l + offset2].xyz;
					v2 = grid2->verts[grid2->width * (l + 1) + offset2].xyz;
					if ( fabs(v1[0] - v2[0]) < .01 &&
							fabs(v1[1] - v2[1]) < .01 &&
							fabs(v1[2] - v2[2]) < .01)
						continue;
					//
					//ri->Printf( PRINT_ALL, "found highest LoD crack between two patches\n" );
					// insert row into grid2 right after after row l
					if (m) column = grid2->width-1;
					else column = 0;
					grid2 = R_GridInsertRow( grid2, l+1, column,
										grid1->verts[k + 1 + offset1].xyz, grid1->widthLodError[k+1]);
					grid2->lodStitched = qfalse;
					s_worldData.surfaces[grid2num].data = (surfaceType_t *) grid2;
					return qtrue;
				}
			}
		}
	}
	for (n = 0; n < 2; n++) {
		//
		if (n) offset1 = grid1->width-1;
		else offset1 = 0;
		if (R_MergedHeightPoints(grid1, offset1))
			continue;
		for (k = 0; k < grid1->height-2; k += 2) {
			for (m = 0; m < 2; m++) {

				if ( grid2->width >= MAX_GRID_SIZE )
					break;
				if (m) offset2 = (grid2->height-1) * grid2->width;
				else offset2 = 0;
				for ( l = 0; l < grid2->width-1; l++) {
				//
					v1 = grid1->verts[grid1->width * k + offset1].xyz;
					v2 = grid2->verts[l + offset2].xyz;
					if ( fabs(v1[0] - v2[0]) > .1)
						continue;
					if ( fabs(v1[1] - v2[1]) > .1)
						continue;
					if ( fabs(v1[2] - v2[2]) > .1)
						continue;

					v1 = grid1->verts[grid1->width * (k + 2) + offset1].xyz;
					v2 = grid2->verts[l + 1 + offset2].xyz;
					if ( fabs(v1[0] - v2[0]) > .1)
						continue;
					if ( fabs(v1[1] - v2[1]) > .1)
						continue;
					if ( fabs(v1[2] - v2[2]) > .1)
						continue;
					//
					v1 = grid2->verts[l + offset2].xyz;
					v2 = grid2->verts[(l + 1) + offset2].xyz;
					if ( fabs(v1[0] - v2[0]) < .01 &&
							fabs(v1[1] - v2[1]) < .01 &&
							fabs(v1[2] - v2[2]) < .01)
						continue;
					//
					//ri->Printf( PRINT_ALL, "found highest LoD crack between two patches\n" );
					// insert column into grid2 right after after column l
					if (m) row = grid2->height-1;
					else row = 0;
					grid2 = R_GridInsertColumn( grid2, l+1, row,
									grid1->verts[grid1->width * (k + 1) + offset1].xyz, grid1->heightLodError[k+1]);
					grid2->lodStitched = qfalse;
					s_worldData.surfaces[grid2num].data = (surfaceType_t *) grid2;
					return qtrue;
				}
			}
			for (m = 0; m < 2; m++) {

				if (grid2->height >= MAX_GRID_SIZE)
					break;
				if (m) offset2 = grid2->width-1;
				else offset2 = 0;
				for ( l = 0; l < grid2->height-1; l++) {
				//
					v1 = grid1->verts[grid1->width * k + offset1].xyz;
					v2 = grid2->verts[grid2->width * l + offset2].xyz;
					if ( fabs(v1[0] - v2[0]) > .1)
						continue;
					if ( fabs(v1[1] - v2[1]) > .1)
						continue;
					if ( fabs(v1[2] - v2[2]) > .1)
						continue;

					v1 = grid1->verts[grid1->width * (k + 2) + offset1].xyz;
					v2 = grid2->verts[grid2->width * (l + 1) + offset2].xyz;
					if ( fabs(v1[0] - v2[0]) > .1)
						continue;
					if ( fabs(v1[1] - v2[1]) > .1)
						continue;
					if ( fabs(v1[2] - v2[2]) > .1)
						continue;
					//
					v1 = grid2->verts[grid2->width * l + offset2].xyz;
					v2 = grid2->verts[grid2->width * (l + 1) + offset2].xyz;
					if ( fabs(v1[0] - v2[0]) < .01 &&
							fabs(v1[1] - v2[1]) < .01 &&
							fabs(v1[2] - v2[2]) < .01)
						continue;
					//
					//ri->Printf( PRINT_ALL, "found highest LoD crack between two patches\n" );
					// insert row into grid2 right after after row l
					if (m) column = grid2->width-1;
					else column = 0;
					grid2 = R_GridInsertRow( grid2, l+1, column,
									grid1->verts[grid1->width * (k + 1) + offset1].xyz, grid1->heightLodError[k+1]);
					grid2->lodStitched = qfalse;
					s_worldData.surfaces[grid2num].data = (surfaceType_t *) grid2;
					return qtrue;
				}
			}
		}
	}
	for (n = 0; n < 2; n++) {
		//
		if (n) offset1 = (grid1->height-1) * grid1->width;
		else offset1 = 0;
		if (R_MergedWidthPoints(grid1, offset1))
			continue;
		for (k = grid1->width-1; k > 1; k -= 2) {

			for (m = 0; m < 2; m++) {

				if ( grid2->width >= MAX_GRID_SIZE )
					break;
				if (m) offset2 = (grid2->height-1) * grid2->width;
				else offset2 = 0;
				for ( l = 0; l < grid2->width-1; l++) {
				//
					v1 = grid1->verts[k + offset1].xyz;
					v2 = grid2->verts[l + offset2].xyz;
					if ( fabs(v1[0] - v2[0]) > .1)
						continue;
					if ( fabs(v1[1] - v2[1]) > .1)
						continue;
					if ( fabs(v1[2] - v2[2]) > .1)
						continue;

					v1 = grid1->verts[k - 2 + offset1].xyz;
					v2 = grid2->verts[l + 1 + offset2].xyz;
					if ( fabs(v1[0] - v2[0]) > .1)
						continue;
					if ( fabs(v1[1] - v2[1]) > .1)
						continue;
					if ( fabs(v1[2] - v2[2]) > .1)
						continue;
					//
					v1 = grid2->verts[l + offset2].xyz;
					v2 = grid2->verts[(l + 1) + offset2].xyz;
					if ( fabs(v1[0] - v2[0]) < .01 &&
							fabs(v1[1] - v2[1]) < .01 &&
							fabs(v1[2] - v2[2]) < .01)
						continue;
					//
					//ri->Printf( PRINT_ALL, "found highest LoD crack between two patches\n" );
					// insert column into grid2 right after after column l
					if (m) row = grid2->height-1;
					else row = 0;
					grid2 = R_GridInsertColumn( grid2, l+1, row,
										grid1->verts[k - 1 + offset1].xyz, grid1->widthLodError[k+1]);
					grid2->lodStitched = qfalse;
					s_worldData.surfaces[grid2num].data = (surfaceType_t *) grid2;
					return qtrue;
				}
			}
			for (m = 0; m < 2; m++) {

				if (grid2->height >= MAX_GRID_SIZE)
					break;
				if (m) offset2 = grid2->width-1;
				else offset2 = 0;
				for ( l = 0; l < grid2->height-1; l++) {
				//
					v1 = grid1->verts[k + offset1].xyz;
					v2 = grid2->verts[grid2->width * l + offset2].xyz;
					if ( fabs(v1[0] - v2[0]) > .1)
						continue;
					if ( fabs(v1[1] - v2[1]) > .1)
						continue;
					if ( fabs(v1[2] - v2[2]) > .1)
						continue;

					v1 = grid1->verts[k - 2 + offset1].xyz;
					v2 = grid2->verts[grid2->width * (l + 1) + offset2].xyz;
					if ( fabs(v1[0] - v2[0]) > .1)
						continue;
					if ( fabs(v1[1] - v2[1]) > .1)
						continue;
					if ( fabs(v1[2] - v2[2]) > .1)
						continue;
					//
					v1 = grid2->verts[grid2->width * l + offset2].xyz;
					v2 = grid2->verts[grid2->width * (l + 1) + offset2].xyz;
					if ( fabs(v1[0] - v2[0]) < .01 &&
							fabs(v1[1] - v2[1]) < .01 &&
							fabs(v1[2] - v2[2]) < .01)
						continue;
					//
					//ri->Printf( PRINT_ALL, "found highest LoD crack between two patches\n" );
					// insert row into grid2 right after after row l
					if (m) column = grid2->width-1;
					else column = 0;
					grid2 = R_GridInsertRow( grid2, l+1, column,
										grid1->verts[k - 1 + offset1].xyz, grid1->widthLodError[k+1]);
					if (!grid2)
						break;
					grid2->lodStitched = qfalse;
					s_worldData.surfaces[grid2num].data = (surfaceType_t *) grid2;
					return qtrue;
				}
			}
		}
	}
	for (n = 0; n < 2; n++) {
		//
		if (n) offset1 = grid1->width-1;
		else offset1 = 0;
		if (R_MergedHeightPoints(grid1, offset1))
			continue;
		for (k = grid1->height-1; k > 1; k -= 2) {
			for (m = 0; m < 2; m++) {

				if ( grid2->width >= MAX_GRID_SIZE )
					break;
				if (m) offset2 = (grid2->height-1) * grid2->width;
				else offset2 = 0;
				for ( l = 0; l < grid2->width-1; l++) {
				//
					v1 = grid1->verts[grid1->width * k + offset1].xyz;
					v2 = grid2->verts[l + offset2].xyz;
					if ( fabs(v1[0] - v2[0]) > .1)
						continue;
					if ( fabs(v1[1] - v2[1]) > .1)
						continue;
					if ( fabs(v1[2] - v2[2]) > .1)
						continue;

					v1 = grid1->verts[grid1->width * (k - 2) + offset1].xyz;
					v2 = grid2->verts[l + 1 + offset2].xyz;
					if ( fabs(v1[0] - v2[0]) > .1)
						continue;
					if ( fabs(v1[1] - v2[1]) > .1)
						continue;
					if ( fabs(v1[2] - v2[2]) > .1)
						continue;
					//
					v1 = grid2->verts[l + offset2].xyz;
					v2 = grid2->verts[(l + 1) + offset2].xyz;
					if ( fabs(v1[0] - v2[0]) < .01 &&
							fabs(v1[1] - v2[1]) < .01 &&
							fabs(v1[2] - v2[2]) < .01)
						continue;
					//
					//ri->Printf( PRINT_ALL, "found highest LoD crack between two patches\n" );
					// insert column into grid2 right after after column l
					if (m) row = grid2->height-1;
					else row = 0;
					grid2 = R_GridInsertColumn( grid2, l+1, row,
										grid1->verts[grid1->width * (k - 1) + offset1].xyz, grid1->heightLodError[k+1]);
					grid2->lodStitched = qfalse;
					s_worldData.surfaces[grid2num].data = (surfaceType_t *) grid2;
					return qtrue;
				}
			}
			for (m = 0; m < 2; m++) {

				if (grid2->height >= MAX_GRID_SIZE)
					break;
				if (m) offset2 = grid2->width-1;
				else offset2 = 0;
				for ( l = 0; l < grid2->height-1; l++) {
				//
					v1 = grid1->verts[grid1->width * k + offset1].xyz;
					v2 = grid2->verts[grid2->width * l + offset2].xyz;
					if ( fabs(v1[0] - v2[0]) > .1)
						continue;
					if ( fabs(v1[1] - v2[1]) > .1)
						continue;
					if ( fabs(v1[2] - v2[2]) > .1)
						continue;

					v1 = grid1->verts[grid1->width * (k - 2) + offset1].xyz;
					v2 = grid2->verts[grid2->width * (l + 1) + offset2].xyz;
					if ( fabs(v1[0] - v2[0]) > .1)
						continue;
					if ( fabs(v1[1] - v2[1]) > .1)
						continue;
					if ( fabs(v1[2] - v2[2]) > .1)
						continue;
					//
					v1 = grid2->verts[grid2->width * l + offset2].xyz;
					v2 = grid2->verts[grid2->width * (l + 1) + offset2].xyz;
					if ( fabs(v1[0] - v2[0]) < .01 &&
							fabs(v1[1] - v2[1]) < .01 &&
							fabs(v1[2] - v2[2]) < .01)
						continue;
					//
					//ri->Printf( PRINT_ALL, "found highest LoD crack between two patches\n" );
					// insert row into grid2 right after after row l
					if (m) column = grid2->width-1;
					else column = 0;
					grid2 = R_GridInsertRow( grid2, l+1, column,
										grid1->verts[grid1->width * (k - 1) + offset1].xyz, grid1->heightLodError[k+1]);
					grid2->lodStitched = qfalse;
					s_worldData.surfaces[grid2num].data = (surfaceType_t *) grid2;
					return qtrue;
				}
			}
		}
	}
	return qfalse;
}

/*
===============
R_TryStitchPatch

This function will try to stitch patches in the same LoD group together for the highest LoD.

Only single missing vertice cracks will be fixed.

Vertices will be joined at the patch side a crack is first found, at the other side
of the patch (on the same row or column) the vertices will not be joined and cracks
might still appear at that side.
===============
*/
int R_TryStitchingPatch( int grid1num ) {
	int j, numstitches;
	srfBspSurface_t *grid1, *grid2;

	numstitches = 0;
	grid1 = (srfBspSurface_t *) s_worldData.surfaces[grid1num].data;

	for ( j = 0; j < s_worldData.numsurfaces; j++ ) {
		//
		grid2 = (srfBspSurface_t *) s_worldData.surfaces[j].data;
		// if this surface is not a grid
		if ( grid2->surfaceType != SF_GRID ) continue;
		// grids in the same LOD group should have the exact same lod radius
		if ( grid1->lodRadius != grid2->lodRadius ) continue;
		// grids in the same LOD group should have the exact same lod origin
		if ( grid1->lodOrigin[0] != grid2->lodOrigin[0] ) continue;
		if ( grid1->lodOrigin[1] != grid2->lodOrigin[1] ) continue;
		if ( grid1->lodOrigin[2] != grid2->lodOrigin[2] ) continue;
		//
		while (R_StitchPatches(grid1num, j))
		{
			numstitches++;
		}
	}
	return numstitches;
}

/*
===============
R_StitchAllPatches
===============
*/
void R_StitchAllPatches( void ) {
	int i, stitched, numstitches;

	numstitches = 0;
	do
	{
		stitched = qfalse;

		for ( i = 0; i < s_worldData.numsurfaces; i++ ) {
			//
			srfBspSurface_t *grid1 = (srfBspSurface_t *) s_worldData.surfaces[i].data;
			// if this surface is not a grid
			if ( grid1->surfaceType != SF_GRID )
				continue;
			//
			if ( grid1->lodStitched )
				continue;
			//
			grid1->lodStitched = qtrue;
			stitched = qtrue;
			//
			numstitches += R_TryStitchingPatch( i );
		}
	}
	while (stitched);
	ri->Printf( PRINT_ALL, "stitched %d LoD cracks\n", numstitches );
}

/*
===============
R_MovePatchSurfacesToHunk
===============
*/
void R_MovePatchSurfacesToHunk(void) {
	int i;

	for ( i = 0; i < s_worldData.numsurfaces; i++ ) {
		int size;

		//
		srfBspSurface_t *grid = (srfBspSurface_t *) s_worldData.surfaces[i].data;
		// if this surface is not a grid
		if ( grid->surfaceType != SF_GRID )
			continue;
		//
		size = sizeof(*grid);
		srfBspSurface_t *hunkgrid = (srfBspSurface_t *)ri->Hunk_Alloc(size, h_low);
		Com_Memcpy(hunkgrid, grid, size);

		hunkgrid->widthLodError = (float *)ri->Hunk_Alloc( grid->width * 4, h_low );
		Com_Memcpy( hunkgrid->widthLodError, grid->widthLodError, grid->width * 4 );

		hunkgrid->heightLodError = (float *)ri->Hunk_Alloc( grid->height * 4, h_low );
		Com_Memcpy( hunkgrid->heightLodError, grid->heightLodError, grid->height * 4 );

		hunkgrid->numIndexes = grid->numIndexes;
		hunkgrid->indexes = (glIndex_t *)ri->Hunk_Alloc(grid->numIndexes * sizeof(glIndex_t), h_low);
		Com_Memcpy(hunkgrid->indexes, grid->indexes, grid->numIndexes * sizeof(glIndex_t));

		hunkgrid->numVerts = grid->numVerts;
		hunkgrid->verts = (srfVert_t *)ri->Hunk_Alloc(grid->numVerts * sizeof(srfVert_t), h_low);
		Com_Memcpy(hunkgrid->verts, grid->verts, grid->numVerts * sizeof(srfVert_t));

		R_FreeSurfaceGridMesh( grid );

		s_worldData.surfaces[i].data = (surfaceType_t *) hunkgrid;
	}
}


/*
=================
BSPSurfaceCompare
compare function for qsort()
=================
*/
static int BSPSurfaceCompare(const void *a, const void *b)
{
	msurface_t   *aa, *bb;

	aa = *(msurface_t **) a;
	bb = *(msurface_t **) b;

	// shader first
	if(aa->shader->sortedIndex < bb->shader->sortedIndex)
		return -1;

	else if(aa->shader->sortedIndex > bb->shader->sortedIndex)
		return 1;

	// by fogIndex
	if(aa->fogIndex < bb->fogIndex)
		return -1;

	else if(aa->fogIndex > bb->fogIndex)
		return 1;

	// by cubemapIndex
	if(aa->cubemapIndex < bb->cubemapIndex)
		return -1;

	else if(aa->cubemapIndex > bb->cubemapIndex)
		return 1;


	return 0;
}


static void CopyVert(const srfVert_t * in, srfVert_t * out)
{
	int             j;

	for(j = 0; j < 3; j++)
	{
		out->xyz[j]       = in->xyz[j];
		out->tangent[j]   = in->tangent[j];
		//out->bitangent[j] = in->bitangent[j];
		out->normal[j]    = in->normal[j];
		out->lightdir[j]  = in->lightdir[j];
	}

	out->tangent[3] = in->tangent[3];

	for(j = 0; j < 2; j++)
	{
		out->st[j] = in->st[j];
		Com_Memcpy (out->lightmap[j], in->lightmap[j], sizeof (out->lightmap[0]));
	}

	for(j = 0; j < 4; j++)
	{
		Com_Memcpy (out->vertexColors[j], in->vertexColors[j], sizeof (out->vertexColors[0]));
	}
}

struct packedVertex_t
{
	vec3_t position;
	uint32_t normal;
	uint32_t tangent;
	vec2_t texcoords[1 + MAXLIGHTMAPS];
	vec4_t colors[MAXLIGHTMAPS];
	uint32_t lightDirection;
};

/*
===============
R_CreateWorldVBOs
===============
*/
static void R_CreateWorldVBOs(void)
{
	int             i, j, k;

	int             numVerts;
	packedVertex_t  *verts;

	int             numIndexes;
	glIndex_t      *indexes;

    int             numSortedSurfaces, numSurfaces;
	msurface_t   *surface, **firstSurf, **lastSurf, **currSurf;
	msurface_t  **surfacesSorted;

	VBO_t *vbo;
	IBO_t *ibo;

	int maxVboSize = 16 * 1024 * 1024;
	int maxIboSize = 4 * 1024 * 1024;

	int             startTime, endTime;

	startTime = ri->Milliseconds();

	// count surfaces
	numSortedSurfaces = 0;

	for(surface = &s_worldData.surfaces[0]; surface < &s_worldData.surfaces[s_worldData.numsurfaces]; surface++)
	{
		srfBspSurface_t *bspSurf;
		shader_t *shader = surface->shader;

		if (shader->isPortal)
			continue;

		if (shader->isSky)
			continue;

		if (ShaderRequiresCPUDeforms(shader))
			continue;

		// check for this now so we can use srfBspSurface_t* universally in the rest of the function
		if (!(*surface->data == SF_FACE || *surface->data == SF_GRID || *surface->data == SF_TRIANGLES))
			continue;

		bspSurf = (srfBspSurface_t *) surface->data;

		if (!bspSurf->numIndexes || !bspSurf->numVerts)
			continue;

		numSortedSurfaces++;
	}

	// presort surfaces
	surfacesSorted = (msurface_t **)Z_Malloc(numSortedSurfaces * sizeof(*surfacesSorted), TAG_BSP);

	j = 0;
	for(surface = &s_worldData.surfaces[0]; surface < &s_worldData.surfaces[s_worldData.numsurfaces]; surface++)
	{
		srfBspSurface_t *bspSurf;
		shader_t *shader = surface->shader;

		if (shader->isPortal)
			continue;

		if (shader->isSky)
			continue;

		if (ShaderRequiresCPUDeforms(shader))
			continue;

		// check for this now so we can use srfBspSurface_t* universally in the rest of the function
		if (!(*surface->data == SF_FACE || *surface->data == SF_GRID || *surface->data == SF_TRIANGLES))
			continue;

		bspSurf = (srfBspSurface_t *) surface->data;

		if (!bspSurf->numIndexes || !bspSurf->numVerts)
			continue;

		surfacesSorted[j++] = surface;
	}

	qsort(surfacesSorted, numSortedSurfaces, sizeof(*surfacesSorted), BSPSurfaceCompare);

	k = 0;
	for(firstSurf = lastSurf = surfacesSorted; firstSurf < &surfacesSorted[numSortedSurfaces]; firstSurf = lastSurf)
	{
		int currVboSize, currIboSize;

		// Find range of surfaces to merge by:
		// - Collecting a number of surfaces which fit under maxVboSize/maxIboSize, or
		// - All the surfaces with a single shader which go over maxVboSize/maxIboSize
		currVboSize = currIboSize = 0;
		while (currVboSize < maxVboSize && currIboSize < maxIboSize && lastSurf < &surfacesSorted[numSortedSurfaces])
		{
			int addVboSize, addIboSize, currShaderIndex;

			addVboSize = addIboSize = 0;
			currShaderIndex = (*lastSurf)->shader->sortedIndex;

			for(currSurf = lastSurf; currSurf < &surfacesSorted[numSortedSurfaces] && (*currSurf)->shader->sortedIndex == currShaderIndex; currSurf++)
			{
				srfBspSurface_t *bspSurf = (srfBspSurface_t *) (*currSurf)->data;

				addVboSize += bspSurf->numVerts * sizeof(srfVert_t);
				addIboSize += bspSurf->numIndexes * sizeof(glIndex_t);
			}

			if ((currVboSize != 0 && addVboSize + currVboSize > maxVboSize)
			 || (currIboSize != 0 && addIboSize + currIboSize > maxIboSize))
				break;

			lastSurf = currSurf;

			currVboSize += addVboSize;
			currIboSize += addIboSize;
		}

		// count verts/indexes/surfaces
		numVerts = 0;
		numIndexes = 0;
		numSurfaces = 0;
		for (currSurf = firstSurf; currSurf < lastSurf; currSurf++)
		{
			srfBspSurface_t *bspSurf = (srfBspSurface_t *) (*currSurf)->data;

			numVerts += bspSurf->numVerts;
			numIndexes += bspSurf->numIndexes;
			numSurfaces++;
		}

		ri->Printf(PRINT_ALL, "...calculating world VBO %d ( %i verts %i tris )\n", k, numVerts, numIndexes / 3);

		// create arrays
		verts = (packedVertex_t *)ri->Hunk_AllocateTempMemory(numVerts * sizeof(packedVertex_t)); 
		indexes = (glIndex_t *)ri->Hunk_AllocateTempMemory(numIndexes * sizeof(glIndex_t)); 

		// set up indices and copy vertices
		numVerts = 0;
		numIndexes = 0;
		for (currSurf = firstSurf; currSurf < lastSurf; currSurf++)
		{
			srfBspSurface_t *bspSurf = (srfBspSurface_t *) (*currSurf)->data;
			glIndex_t *surfIndex;

			bspSurf->firstIndex = numIndexes;
			bspSurf->minIndex = numVerts + bspSurf->indexes[0];
			bspSurf->maxIndex = numVerts + bspSurf->indexes[0];

			for(i = 0, surfIndex = bspSurf->indexes; i < bspSurf->numIndexes; i++, surfIndex++)
			{
				indexes[numIndexes++] = numVerts + *surfIndex;
				bspSurf->minIndex = MIN(bspSurf->minIndex, numVerts + *surfIndex);
				bspSurf->maxIndex = MAX(bspSurf->maxIndex, numVerts + *surfIndex);
			}

			bspSurf->firstVert = numVerts;

			for(i = 0; i < bspSurf->numVerts; i++)
			{
				packedVertex_t& vert = verts[numVerts++];

				VectorCopy (bspSurf->verts[i].xyz, vert.position);
				vert.normal = R_VboPackNormal (bspSurf->verts[i].normal);
				vert.tangent = R_VboPackTangent (bspSurf->verts[i].tangent);
				VectorCopy2 (bspSurf->verts[i].st, vert.texcoords[0]);

				for (int j = 0; j < MAXLIGHTMAPS; j++)
				{
					VectorCopy2 (bspSurf->verts[i].lightmap[j], vert.texcoords[1 + j]);
				}

				for (int j = 0; j < MAXLIGHTMAPS; j++)
				{
					VectorCopy4 (bspSurf->verts[i].vertexColors[j], vert.colors[j]);
				}

				vert.lightDirection = R_VboPackNormal (bspSurf->verts[i].lightdir);
			}
		}

		vbo = R_CreateVBO((byte *)verts, sizeof (packedVertex_t) * numVerts, VBO_USAGE_STATIC);
		ibo = R_CreateIBO((byte *)indexes, numIndexes * sizeof (glIndex_t), VBO_USAGE_STATIC);

		// Setup the offsets and strides
		vbo->ofs_xyz = offsetof (packedVertex_t, position);
		vbo->ofs_normal = offsetof (packedVertex_t, normal);
		vbo->ofs_tangent = offsetof (packedVertex_t, tangent);
		vbo->ofs_st = offsetof (packedVertex_t, texcoords);
		vbo->ofs_vertexcolor = offsetof (packedVertex_t, colors);
		vbo->ofs_lightdir = offsetof (packedVertex_t, lightDirection);

		const size_t packedVertexSize = sizeof (packedVertex_t);
		vbo->stride_xyz = packedVertexSize;
		vbo->stride_normal = packedVertexSize;
		vbo->stride_tangent = packedVertexSize;
		vbo->stride_st = packedVertexSize;
		vbo->stride_vertexcolor = packedVertexSize;
		vbo->stride_lightdir = packedVertexSize;

		// point bsp surfaces to VBO
		for (currSurf = firstSurf; currSurf < lastSurf; currSurf++)
		{
			srfBspSurface_t *bspSurf = (srfBspSurface_t *) (*currSurf)->data;

			bspSurf->vbo = vbo;
			bspSurf->ibo = ibo;
		}

		ri->Hunk_FreeTempMemory(indexes);
		ri->Hunk_FreeTempMemory(verts);

		k++;
	}

	Z_Free(surfacesSorted);

	endTime = ri->Milliseconds();
	ri->Printf(PRINT_ALL, "world VBOs calculation time = %5.2f seconds\n", (endTime - startTime) / 1000.0);
}

/*
===============
R_LoadSurfaces
===============
*/
static	void R_LoadSurfaces( lump_t *surfs, lump_t *verts, lump_t *indexLump ) {
	dsurface_t	*in;
	msurface_t	*out;
	drawVert_t	*dv;
	int			*indexes;
	int			count;
	int			numFaces, numMeshes, numTriSurfs, numFlares;
	int			i;
	float *hdrVertColors = NULL;

	numFaces = 0;
	numMeshes = 0;
	numTriSurfs = 0;
	numFlares = 0;

	if (surfs->filelen % sizeof(*in))
		ri->Error (ERR_DROP, "LoadMap: funny lump size in %s",s_worldData.name);
	count = surfs->filelen / sizeof(*in);

	dv = (drawVert_t *)(fileBase + verts->fileofs);
	if (verts->filelen % sizeof(*dv))
		ri->Error (ERR_DROP, "LoadMap: funny lump size in %s",s_worldData.name);

	indexes = (int *)(fileBase + indexLump->fileofs);
	if ( indexLump->filelen % sizeof(*indexes))
		ri->Error (ERR_DROP, "LoadMap: funny lump size in %s",s_worldData.name);

	out = (msurface_t *)ri->Hunk_Alloc ( count * sizeof(*out), h_low );	

	s_worldData.surfaces = out;
	s_worldData.numsurfaces = count;

	s_worldData.surfacesViewCount = (int *)ri->Hunk_Alloc ( count * sizeof(*s_worldData.surfacesViewCount), h_low );
	//s_worldData.surfacesDlightBits = (int *)ri->Hunk_Alloc ( count * sizeof(*s_worldData.surfacesDlightBits), h_low );
#ifdef __PSHADOWS__
	s_worldData.surfacesPshadowBits = (int *)ri->Hunk_Alloc ( count * sizeof(*s_worldData.surfacesPshadowBits), h_low );
#endif

	// load hdr vertex colors
	if (r_hdr->integer)
	{
		char filename[MAX_QPATH];
		int size;

		Com_sprintf( filename, sizeof( filename ), "maps/%s/vertlight.raw", s_worldData.baseName);
		//ri->Printf(PRINT_ALL, "looking for %s\n", filename);

		size = ri->FS_ReadFile(filename, (void **)&hdrVertColors);

		if (hdrVertColors)
		{
			//ri->Printf(PRINT_ALL, "Found!\n");
			if (size != sizeof(float) * 3 * (verts->filelen / sizeof(*dv)))
				ri->Error(ERR_DROP, "Bad size for %s (%i, expected %i)!", filename, size, (int)((sizeof(float)) * 3 * (verts->filelen / sizeof(*dv))));
		}
	}


	// Two passes, allocate surfaces first, then load them full of data
	// This ensures surfaces are close together to reduce L2 cache misses when using VBOs,
	// which don't actually use the verts and indexes
	in = (dsurface_t *)(fileBase + surfs->fileofs);
	out = s_worldData.surfaces;
	for ( i = 0 ; i < count ; i++, in++, out++ ) {
		switch ( LittleLong( in->surfaceType ) ) {
			case MST_PATCH:
				// FIXME: do this
				break;
			//case MST_FOLIAGE:
			case MST_TRIANGLE_SOUP:
				out->data = (surfaceType_t *)ri->Hunk_Alloc( sizeof(srfBspSurface_t), h_low);
				break;
			case MST_PLANAR:
				out->data = (surfaceType_t *)ri->Hunk_Alloc( sizeof(srfBspSurface_t), h_low);
				break;
			case MST_FLARE:
				out->data = (surfaceType_t *)ri->Hunk_Alloc( sizeof(srfFlare_t), h_low);
				break;
			default:
				break;
		}
	}

	in = (dsurface_t *)(fileBase + surfs->fileofs);
	out = s_worldData.surfaces;

	for ( i = 0 ; i < count ; i++, in++, out++ ) {
		switch ( LittleLong( in->surfaceType ) ) {
		case MST_PATCH:
			ParseMesh ( in, dv, hdrVertColors, out );
			{
				srfBspSurface_t *surface = (srfBspSurface_t *)out->data;

				out->cullinfo.type = CULLINFO_BOX | CULLINFO_SPHERE;
				VectorCopy(surface->cullBounds[0], out->cullinfo.bounds[0]);
				VectorCopy(surface->cullBounds[1], out->cullinfo.bounds[1]);
				VectorCopy(surface->cullOrigin, out->cullinfo.localOrigin);
				out->cullinfo.radius = surface->cullRadius;
			}
			numMeshes++;
			break;
		/*case MST_FOLIAGE:
			ParseTriSurf( in, dv, hdrVertColors, out, indexes );
			//ri->Printf(PRINT_WARNING, "Loaded foliage surface at %f %f %f.\n", dv->xyz[0], dv->xyz[1], dv->xyz[2]);
			numTriSurfs++;
			break;*/
		case MST_TRIANGLE_SOUP:
			ParseTriSurf( in, dv, hdrVertColors, out, indexes );
			numTriSurfs++;
			break;
		case MST_PLANAR:
			ParseFace( in, dv, hdrVertColors, out, indexes );
			numFaces++;
			break;
		case MST_FLARE:
			ParseFlare( in, dv, out, indexes );
			{
				out->cullinfo.type = CULLINFO_NONE;
			}
			numFlares++;
			break;
		default:
			ri->Error( ERR_DROP, "Bad surfaceType" );
		}
	}

	if (hdrVertColors)
	{
		ri->FS_FreeFile(hdrVertColors);
	}

#ifdef PATCH_STITCHING
	R_StitchAllPatches();
#endif

	R_FixSharedVertexLodError();

#ifdef PATCH_STITCHING
	R_MovePatchSurfacesToHunk();
#endif

	ri->Printf( PRINT_ALL, "...loaded %d faces, %i meshes, %i trisurfs, %i flares\n", 
		numFaces, numMeshes, numTriSurfs, numFlares );
}



/*
=================
R_LoadSubmodels
=================
*/
static	void R_LoadSubmodels( lump_t *l ) {
	dmodel_t	*in;
	bmodel_t	*out;
	int			i, j, count;

	in = (dmodel_t *)(fileBase + l->fileofs);
	if (l->filelen % sizeof(*in))
		ri->Error (ERR_DROP, "LoadMap: funny lump size in %s",s_worldData.name);
	count = l->filelen / sizeof(*in);

	s_worldData.numBModels = count;
	s_worldData.bmodels = out = (bmodel_t *)ri->Hunk_Alloc( count * sizeof(*out), h_low );

	for ( i=0 ; i<count ; i++, in++, out++ ) {
		model_t *model;

		model = R_AllocModel();

		assert( model != NULL );			// this should never happen
		if ( model == NULL ) {
			ri->Error(ERR_DROP, "R_LoadSubmodels: R_AllocModel() failed");
		}

		model->type = MOD_BRUSH;
		model->data.bmodel = out;
		Com_sprintf( model->name, sizeof( model->name ), "*%d", i );

		for (j=0 ; j<3 ; j++) {
			out->bounds[0][j] = LittleFloat (in->mins[j]);
			out->bounds[1][j] = LittleFloat (in->maxs[j]);
		}

		CModelCache->InsertLoaded (model->name, model->index);

		out->firstSurface = LittleLong( in->firstSurface );
		out->numSurfaces = LittleLong( in->numSurfaces );

		if(i == 0)
		{
			// Add this for limiting VBO surface creation
			s_worldData.numWorldSurfaces = out->numSurfaces;
		}
	}
}



//==================================================================

/*
=================
R_SetParent
=================
*/
static	void R_SetParent (mnode_t *node, mnode_t *parent)
{
	node->parent = parent;
	if (node->contents != -1)
		return;
	R_SetParent (node->children[0], node);
	R_SetParent (node->children[1], node);
}

/*
=================
R_LoadNodesAndLeafs
=================
*/
static	void R_LoadNodesAndLeafs (lump_t *nodeLump, lump_t *leafLump) {
	int			i, j, p;
	dnode_t		*in;
	dleaf_t		*inLeaf;
	mnode_t 	*out;
	int			numNodes, numLeafs;

	in = (dnode_t *)(fileBase + nodeLump->fileofs);
	if (nodeLump->filelen % sizeof(dnode_t) ||
		leafLump->filelen % sizeof(dleaf_t) ) {
		ri->Error (ERR_DROP, "LoadMap: funny lump size in %s",s_worldData.name);
	}
	numNodes = nodeLump->filelen / sizeof(dnode_t);
	numLeafs = leafLump->filelen / sizeof(dleaf_t);

	out = (mnode_t *)ri->Hunk_Alloc ( (numNodes + numLeafs) * sizeof(*out), h_low);	

	s_worldData.nodes = out;
	s_worldData.numnodes = numNodes + numLeafs;
	s_worldData.numDecisionNodes = numNodes;

	// load nodes
	for ( i=0 ; i<numNodes; i++, in++, out++)
	{
		for (j=0 ; j<3 ; j++)
		{
			out->mins[j] = LittleLong (in->mins[j]);
			out->maxs[j] = LittleLong (in->maxs[j]);
		}
	
		p = LittleLong(in->planeNum);
		out->plane = s_worldData.planes + p;

		out->contents = CONTENTS_NODE;	// differentiate from leafs

		for (j=0 ; j<2 ; j++)
		{
			p = LittleLong (in->children[j]);
			if (p >= 0)
				out->children[j] = s_worldData.nodes + p;
			else
				out->children[j] = s_worldData.nodes + numNodes + (-1 - p);
		}
	}
	
	// load leafs
	inLeaf = (dleaf_t *)(fileBase + leafLump->fileofs);
	for ( i=0 ; i<numLeafs ; i++, inLeaf++, out++)
	{
		for (j=0 ; j<3 ; j++)
		{
			out->mins[j] = LittleLong (inLeaf->mins[j]);
			out->maxs[j] = LittleLong (inLeaf->maxs[j]);
		}

		out->cluster = LittleLong(inLeaf->cluster);
		out->area = LittleLong(inLeaf->area);

		if ( out->cluster >= s_worldData.numClusters ) {
			s_worldData.numClusters = out->cluster + 1;
		}

		out->firstmarksurface = LittleLong(inLeaf->firstLeafSurface);
		out->nummarksurfaces = LittleLong(inLeaf->numLeafSurfaces);
	}	

	// chain decendants
	R_SetParent (s_worldData.nodes, NULL);

	s_worldData.visibleLeafs[0] = (mnode_t **)ri->Hunk_Alloc( numLeafs * sizeof(mnode_t *), h_low);	
	s_worldData.visibleLeafs[1] = (mnode_t **)ri->Hunk_Alloc( numLeafs * sizeof(mnode_t *), h_low);	
	s_worldData.numVisibleLeafs[0] = 0;
	s_worldData.numVisibleLeafs[1] = 0;
}

//=============================================================================

/*
=================
R_LoadShaders
=================
*/
static	void R_LoadShaders( lump_t *l ) {
#if 1 // Renderer seems to dislike sharing these.
	int		i, count;
	dshader_t	*in, *out;
	
	in = (dshader_t *)(fileBase + l->fileofs);
	if (l->filelen % sizeof(*in))
		ri->Error (ERR_DROP, "LoadMap: funny lump size in %s",s_worldData.name);
	count = l->filelen / sizeof(*in);
	out = (dshader_t *)ri->Hunk_Alloc ( count*sizeof(*out), h_low );

	s_worldData.shaders = out;
	s_worldData.numShaders = count;

	Com_Memcpy( out, in, count*sizeof(*out) );

	for ( i=0 ; i<count ; i++ ) {
		out[i].surfaceFlags = LittleLong( out[i].surfaceFlags );
		out[i].contentFlags = LittleLong( out[i].contentFlags );
	}
#else
	s_worldData.shaders = (dshader_t *)ri->CM_GetShaderData();
	s_worldData.numShaders = (int)ri->CM_GetShaderDataCount();
#endif
}


/*
=================
R_LoadMarksurfaces
=================
*/
static	void R_LoadMarksurfaces (lump_t *l)
{
#if 0
	int		i, j, count;
	int		*in;
	int     *out;
	
	in = (int *)(fileBase + l->fileofs);
	if (l->filelen % sizeof(*in))
		ri->Error (ERR_DROP, "LoadMap: funny lump size in %s",s_worldData.name);
	count = l->filelen / sizeof(*in);
	out = (int *)ri->Hunk_Alloc ( count*sizeof(*out), h_low);	

	s_worldData.marksurfaces = out;
	s_worldData.nummarksurfaces = count;

	for ( i=0 ; i<count ; i++)
	{
		j = LittleLong(in[i]);
		out[i] = j;
	}
#else
	s_worldData.marksurfaces = (int *)ri->CM_GetLeafSurfacesData();
	s_worldData.nummarksurfaces = (int)ri->CM_GetLeafSurfacesDataCount();
#endif
}


/*
=================
R_LoadPlanes
=================
*/
static	void R_LoadPlanes( lump_t *l ) {
#if 0
	int			i, j;
	cplane_t	*out;
	dplane_t 	*in;
	int			count;
	int			bits;
	
	in = (dplane_t *)(fileBase + l->fileofs);
	if (l->filelen % sizeof(*in))
		ri->Error (ERR_DROP, "LoadMap: funny lump size in %s",s_worldData.name);
	count = l->filelen / sizeof(*in);
	out = (cplane_t *)ri->Hunk_Alloc ( count*2*sizeof(*out), h_low);

	s_worldData.planes = out;
	s_worldData.numplanes = count;

	for (i = 0; i<count; i++, in++, out++) {
		bits = 0;
		for (j = 0; j<3; j++) {
			out->normal[j] = LittleFloat(in->normal[j]);
			if (out->normal[j] < 0) {
				bits |= 1 << j;
			}
		}

		out->dist = LittleFloat(in->dist);
		out->type = PlaneTypeForNormal(out->normal);
		out->signbits = bits;
	}
#else
	s_worldData.planes = (cplane_t *)ri->CM_GetPlanesData();
	s_worldData.numplanes = (int)ri->CM_GetPlanesDataCount();
#endif
}

/*
=================
R_LoadFogs

=================
*/
static	void R_LoadFogs( lump_t *l, lump_t *brushesLump, lump_t *sidesLump ) {
	int			i;
	fog_t		*out;
	dfog_t		*fogs;
	dbrush_t 	*brushes, *brush;
	dbrushside_t	*sides;
	int			count, brushesCount, sidesCount;
	int			sideNum;
	int			planeNum;
	shader_t	*shader;
	float		d;
	int			firstSide;

	fogs = (dfog_t *)(fileBase + l->fileofs);
	if (l->filelen % sizeof(*fogs)) {
		ri->Error (ERR_DROP, "LoadMap: funny lump size in %s",s_worldData.name);
	}
	count = l->filelen / sizeof(*fogs);

	// create fog strucutres for them
	s_worldData.numfogs = count + 1;
	s_worldData.fogs = (fog_t *)ri->Hunk_Alloc ( s_worldData.numfogs*sizeof(*out), h_low);
	out = s_worldData.fogs + 1;

	if ( !count ) {
		return;
	}

	brushes = (dbrush_t *)(fileBase + brushesLump->fileofs);
	if (brushesLump->filelen % sizeof(*brushes)) {
		ri->Error (ERR_DROP, "LoadMap: funny lump size in %s",s_worldData.name);
	}
	brushesCount = brushesLump->filelen / sizeof(*brushes);

	sides = (dbrushside_t *)(fileBase + sidesLump->fileofs);
	if (sidesLump->filelen % sizeof(*sides)) {
		ri->Error (ERR_DROP, "LoadMap: funny lump size in %s",s_worldData.name);
	}
	sidesCount = sidesLump->filelen / sizeof(*sides);

	for ( i=0 ; i<count ; i++, fogs++) {
		out->originalBrushNumber = LittleLong( fogs->brushNum );

		if ( out->originalBrushNumber == -1 )
		{
			out->bounds[0][0] = out->bounds[0][1] = out->bounds[0][2] = MIN_WORLD_COORD;
			out->bounds[1][0] = out->bounds[1][1] = out->bounds[1][2] = MAX_WORLD_COORD;
			firstSide = -1;
		}
		else
		{
			if ( (unsigned)out->originalBrushNumber >= brushesCount ) {
				ri->Error( ERR_DROP, "fog brushNumber out of range" );
			}
			brush = brushes + out->originalBrushNumber;

			firstSide = LittleLong( brush->firstSide );

				if ( (unsigned)firstSide > sidesCount - 6 ) {
				ri->Error( ERR_DROP, "fog brush sideNumber out of range" );
			}

			// brushes are always sorted with the axial sides first
			sideNum = firstSide + 0;
			planeNum = LittleLong( sides[ sideNum ].planeNum );
			out->bounds[0][0] = -s_worldData.planes[ planeNum ].dist;

			sideNum = firstSide + 1;
			planeNum = LittleLong( sides[ sideNum ].planeNum );
			out->bounds[1][0] = s_worldData.planes[ planeNum ].dist;

			sideNum = firstSide + 2;
			planeNum = LittleLong( sides[ sideNum ].planeNum );
			out->bounds[0][1] = -s_worldData.planes[ planeNum ].dist;

			sideNum = firstSide + 3;
			planeNum = LittleLong( sides[ sideNum ].planeNum );
			out->bounds[1][1] = s_worldData.planes[ planeNum ].dist;

			sideNum = firstSide + 4;
			planeNum = LittleLong( sides[ sideNum ].planeNum );
			out->bounds[0][2] = -s_worldData.planes[ planeNum ].dist;

			sideNum = firstSide + 5;
			planeNum = LittleLong( sides[ sideNum ].planeNum );
			out->bounds[1][2] = s_worldData.planes[ planeNum ].dist;
		}

		// get information from the shader for fog parameters
		shader = R_FindShader( fogs->shader, lightmapsNone, stylesDefault, qtrue );

		out->parms = shader->fogParms;

		out->colorInt = ColorBytes4 ( shader->fogParms.color[0] * tr.identityLight, 
			                          shader->fogParms.color[1] * tr.identityLight, 
			                          shader->fogParms.color[2] * tr.identityLight, 1.0 );

		d = shader->fogParms.depthForOpaque < 1 ? 1 : shader->fogParms.depthForOpaque;
		out->tcScale = 1.0f / ( d * 8 );

		// set the gradient vector
		sideNum = LittleLong( fogs->visibleSide );

		out->hasSurface = qtrue;
		if ( sideNum != -1 ) {
			planeNum = LittleLong( sides[ firstSide + sideNum ].planeNum );
			VectorSubtract( vec3_origin, s_worldData.planes[ planeNum ].normal, out->surface );
			out->surface[3] = -s_worldData.planes[ planeNum ].dist;
		}

		out++;
	}

}


/*
================
R_LoadLightGrid

================
*/
void R_LoadLightGrid( lump_t *l ) {
	int		i;
	vec3_t	maxs;
	world_t	*w;
	float	*wMins, *wMaxs;

	w = &s_worldData;

	w->lightGridInverseSize[0] = 1.0f / w->lightGridSize[0];
	w->lightGridInverseSize[1] = 1.0f / w->lightGridSize[1];
	w->lightGridInverseSize[2] = 1.0f / w->lightGridSize[2];

	wMins = w->bmodels[0].bounds[0];
	wMaxs = w->bmodels[0].bounds[1];

	for ( i = 0 ; i < 3 ; i++ ) {
		w->lightGridOrigin[i] = w->lightGridSize[i] * ceil( wMins[i] / w->lightGridSize[i] );
		maxs[i] = w->lightGridSize[i] * floor( wMaxs[i] / w->lightGridSize[i] );
		w->lightGridBounds[i] = (maxs[i] - w->lightGridOrigin[i])/w->lightGridSize[i] + 1;
	}

	int numGridDataElements = l->filelen / sizeof(*w->lightGridData);

	w->lightGridData = (mgrid_t *)ri->Hunk_Alloc( l->filelen, h_low );
	Com_Memcpy( w->lightGridData, (void *)(fileBase + l->fileofs), l->filelen );

	// deal with overbright bits

	for ( i = 0 ; i < numGridDataElements ; i++ ) 
	{
		for(int j = 0; j < MAXLIGHTMAPS; j++)
		{
			R_ColorShiftLightingBytes(
				w->lightGridData[i].ambientLight[j],
				w->lightGridData[i].ambientLight[j]);
			R_ColorShiftLightingBytes(
				w->lightGridData[i].directLight[j],
				w->lightGridData[i].directLight[j]);
		}
	}

	// load hdr lightgrid
	if (r_hdr->integer)
	{
		char filename[MAX_QPATH];
		float *hdrLightGrid;
		int size;

		Com_sprintf( filename, sizeof( filename ), "maps/%s/lightgrid.raw", s_worldData.baseName);
		//ri->Printf(PRINT_ALL, "looking for %s\n", filename);

		size = ri->FS_ReadFile(filename, (void **)&hdrLightGrid);

		if (hdrLightGrid && size)
		{
			float lightScale = pow(2.0f, r_mapOverBrightBits->integer - tr.overbrightBits);

			//ri->Printf(PRINT_ALL, "found!\n");

			if (size != sizeof(float) * 6 * numGridDataElements)
			{
				ri->Error(ERR_DROP, "Bad size for %s (%i, expected %i)!", filename, size, (int)(sizeof(float)) * 6 * numGridDataElements);
			}

			w->hdrLightGrid = (float *)ri->Hunk_Alloc(size, h_low);

			for (i = 0; i < numGridDataElements ; i++)
			{
				w->hdrLightGrid[i * 6    ] = hdrLightGrid[i * 6    ] * lightScale;
				w->hdrLightGrid[i * 6 + 1] = hdrLightGrid[i * 6 + 1] * lightScale;
				w->hdrLightGrid[i * 6 + 2] = hdrLightGrid[i * 6 + 2] * lightScale;
				w->hdrLightGrid[i * 6 + 3] = hdrLightGrid[i * 6 + 3] * lightScale;
				w->hdrLightGrid[i * 6 + 4] = hdrLightGrid[i * 6 + 4] * lightScale;
				w->hdrLightGrid[i * 6 + 5] = hdrLightGrid[i * 6 + 5] * lightScale;
			}
		}

		if (hdrLightGrid && size)
			ri->FS_FreeFile(hdrLightGrid);
	}
}

/*
================
R_LoadLightGridArray

================
*/
void R_LoadLightGridArray( lump_t *l ) {
	world_t	*w;

	w = &s_worldData;

	w->numGridArrayElements = w->lightGridBounds[0] * w->lightGridBounds[1] * w->lightGridBounds[2];

	if ( (unsigned)l->filelen != w->numGridArrayElements * sizeof(*w->lightGridArray) ) {
		Com_Printf (S_COLOR_YELLOW  "WARNING: light grid array mismatch\n" );
		w->lightGridData = NULL;
		return;
	}

	w->lightGridArray = (unsigned short *)Hunk_Alloc( l->filelen, h_low );
	memcpy( w->lightGridArray, (void *)(fileBase + l->fileofs), l->filelen );
}

/*
================
R_LoadEntities
================
*/
void R_LoadEntities( lump_t *l ) {
	const char *p;
	char *token, *s;
	char keyname[MAX_TOKEN_CHARS];
	char value[MAX_TOKEN_CHARS];
	world_t	*w;

	w = &s_worldData;
	w->lightGridSize[0] = 64;
	w->lightGridSize[1] = 64;
	w->lightGridSize[2] = 128;

	tr.distanceCull = 6000;//DEFAULT_DISTANCE_CULL;

	p = (char *)(fileBase + l->fileofs);

	// store for reference by the cgame
	w->entityString = (char *)ri->Hunk_Alloc( l->filelen + 1, h_low );
	strcpy( w->entityString, p );
	w->entityParsePoint = w->entityString;

	token = COM_ParseExt( &p, qtrue );
	if (!*token || *token != '{') {
		return;
	}

	// only parse the world spawn
	while ( 1 ) {	
		// parse key
		token = COM_ParseExt( &p, qtrue );

		if ( !*token || *token == '}' ) {
			break;
		}
		Q_strncpyz(keyname, token, sizeof(keyname));

		// parse value
		token = COM_ParseExt( &p, qtrue );

		if ( !*token || *token == '}' ) {
			break;
		}
		Q_strncpyz(value, token, sizeof(value));

		// check for remapping of shaders for vertex lighting
		s = "vertexremapshader";
		if (!Q_strncmp(keyname, s, strlen(s)) ) {
			s = strchr(value, ';');
			if (!s) {
				ri->Printf( PRINT_WARNING, "WARNING: no semi colon in vertexshaderremap '%s'\n", value );
				break;
			}
			*s++ = 0;
			if (r_vertexLight->integer) {
				R_RemapShader(value, s, "0");
			}
			continue;
		}
		// check for remapping of shaders
		s = "remapshader";
		if (!Q_strncmp(keyname, s, strlen(s)) ) {
			s = strchr(value, ';');
			if (!s) {
				ri->Printf( PRINT_WARNING, "WARNING: no semi colon in shaderremap '%s'\n", value );
				break;
			}
			*s++ = 0;
			R_RemapShader(value, s, "0");
			continue;
		}
 		if (!Q_stricmp(keyname, "distanceCull")) {
			sscanf(value, "%f", &tr.distanceCull );
			continue;
		}
		// check for a different grid size
		if (!Q_stricmp(keyname, "gridsize")) {
			sscanf(value, "%f %f %f", &w->lightGridSize[0], &w->lightGridSize[1], &w->lightGridSize[2] );
			continue;
		}

		// check for auto exposure
		if (!Q_stricmp(keyname, "autoExposureMinMax")) {
			sscanf(value, "%f %f", &tr.autoExposureMinMax[0], &tr.autoExposureMinMax[1]);
			continue;
		}
	}
}

/*
=================
R_GetEntityToken
=================
*/
qboolean R_GetEntityToken( char *buffer, int size ) {
	char	*s;

	if (size == -1)
	{ //force reset
		s_worldData.entityParsePoint = s_worldData.entityString;
		return qtrue;
	}

	s = COM_Parse( (const char **)&s_worldData.entityParsePoint );
	Q_strncpyz( buffer, s, size );
	if ( !s_worldData.entityParsePoint && !s[0] ) {
		s_worldData.entityParsePoint = s_worldData.entityString;
		return qfalse;
	} else {
		return qtrue;
	}
}

#ifndef MAX_SPAWN_VARS
#define MAX_SPAWN_VARS 64
#endif

// derived from G_ParseSpawnVars() in g_spawn.c
static qboolean R_ParseSpawnVars( char *spawnVarChars, int maxSpawnVarChars, int *numSpawnVars, char *spawnVars[MAX_SPAWN_VARS][2] )
{
	char    keyname[MAX_TOKEN_CHARS];
	char	com_token[MAX_TOKEN_CHARS];
	int		numSpawnVarChars = 0;

	*numSpawnVars = 0;

	// parse the opening brace
	if ( !R_GetEntityToken( com_token, sizeof( com_token ) ) ) {
		// end of spawn string
		return qfalse;
	}
	if ( com_token[0] != '{' ) {
		ri->Printf( PRINT_ALL, "R_ParseSpawnVars: found %s when expecting {\n",com_token );
		return qfalse;
	}

	// go through all the key / value pairs
	while ( 1 ) {  
		int keyLength, tokenLength;

		// parse key
		if ( !R_GetEntityToken( keyname, sizeof( keyname ) ) ) {
			ri->Printf( PRINT_ALL, "R_ParseSpawnVars: EOF without closing brace\n" );
			return qfalse;
		}

		if ( keyname[0] == '}' ) {
			break;
		}

		// parse value  
		if ( !R_GetEntityToken( com_token, sizeof( com_token ) ) ) {
			ri->Printf( PRINT_ALL, "R_ParseSpawnVars: EOF without closing brace\n" );
			return qfalse;
		}

		if ( com_token[0] == '}' ) {
			ri->Printf( PRINT_ALL, "R_ParseSpawnVars: closing brace without data\n" );
			return qfalse;
		}

		if ( *numSpawnVars == MAX_SPAWN_VARS ) {
			ri->Printf( PRINT_ALL, "R_ParseSpawnVars: MAX_SPAWN_VARS\n" );
			return qfalse;
		}

		keyLength = strlen(keyname) + 1;
		tokenLength = strlen(com_token) + 1;

		if (numSpawnVarChars + keyLength + tokenLength > maxSpawnVarChars)
		{
			ri->Printf( PRINT_ALL, "R_ParseSpawnVars: MAX_SPAWN_VAR_CHARS\n" );
			return qfalse;
		}

		strcpy(spawnVarChars + numSpawnVarChars, keyname);
		spawnVars[ *numSpawnVars ][0] = spawnVarChars + numSpawnVarChars;
		numSpawnVarChars += keyLength;

		strcpy(spawnVarChars + numSpawnVarChars, com_token);
		spawnVars[ *numSpawnVars ][1] = spawnVarChars + numSpawnVarChars;
		numSpawnVarChars += tokenLength;

		(*numSpawnVars)++;
	}

	return qtrue;
}

static void R_LoadCubemapEntities(const char *cubemapEntityName)
{
	char spawnVarChars[2048];
	int numSpawnVars;
	char *spawnVars[MAX_SPAWN_VARS][2];
	int numCubemaps = 0;

	// count cubemaps
	numCubemaps = 0;
	while(R_ParseSpawnVars(spawnVarChars, sizeof(spawnVarChars), &numSpawnVars, spawnVars))
	{
		int i;

		for (i = 0; i < numSpawnVars; i++)
		{
			if (!Q_stricmp(spawnVars[i][0], "classname") && !Q_stricmp(spawnVars[i][1], cubemapEntityName))
				numCubemaps++;
		}
	}

	if (!numCubemaps)
		return;

	tr.numCubemaps = numCubemaps;
	tr.cubemapOrigins = (vec3_t *)ri->Hunk_Alloc( tr.numCubemaps * sizeof(*tr.cubemapOrigins), h_low);
	tr.cubemaps = (image_t **)ri->Hunk_Alloc( tr.numCubemaps * sizeof(*tr.cubemaps), h_low);

	numCubemaps = 0;
	while(R_ParseSpawnVars(spawnVarChars, sizeof(spawnVarChars), &numSpawnVars, spawnVars))
	{
		int i;
		qboolean isCubemap = qfalse;
		qboolean positionSet = qfalse;
		vec3_t origin;

		for (i = 0; i < numSpawnVars; i++)
		{
			if (!Q_stricmp(spawnVars[i][0], "classname") && !Q_stricmp(spawnVars[i][1], cubemapEntityName))
				isCubemap = qtrue;

			if (!Q_stricmp(spawnVars[i][0], "origin"))
			{
				sscanf(spawnVars[i][1], "%f %f %f", &origin[0], &origin[1], &origin[2]);
				positionSet = qtrue;
			}
		}

		if (isCubemap && positionSet)
		{
			//ri.Printf(PRINT_ALL, "cubemap at %f %f %f\n", origin[0], origin[1], origin[2]);
			VectorCopy(origin, tr.cubemapOrigins[numCubemaps]);
			numCubemaps++;
		}
	}
}

qboolean R_MaterialUsesCubemap ( int surfaceFlags )
{
	switch( surfaceFlags & MATERIAL_MASK )
	{
	case MATERIAL_WATER:			// 13			// light covering of water on a surface
		return qtrue;
		break;
	case MATERIAL_SHORTGRASS:		// 5			// manicured lawn
		return qfalse;
		break;
	case MATERIAL_LONGGRASS:		// 6			// long jungle grass
		return qfalse;
		break;
	case MATERIAL_SAND:				// 8			// sandy beach
		return qfalse;
		break;
	case MATERIAL_CARPET:			// 27			// lush carpet
		return qfalse;
		break;
	case MATERIAL_GRAVEL:			// 9			// lots of small stones
		return qfalse;
		break;
	case MATERIAL_ROCK:				// 23			//
		return qfalse;
		break;
	case MATERIAL_TILES:			// 26			// tiled floor
		return qtrue;
		break;
	case MATERIAL_SOLIDWOOD:		// 1			// freshly cut timber
		return qfalse;
		break;
	case MATERIAL_HOLLOWWOOD:		// 2			// termite infested creaky wood
		return qfalse;
		break;
	case MATERIAL_SOLIDMETAL:		// 3			// solid girders
		return qtrue;
		break;
	case MATERIAL_HOLLOWMETAL:		// 4			// hollow metal machines -- UQ1: Used for weapons to force lower parallax and high reflection...
		return qtrue;
		break;
	case MATERIAL_DRYLEAVES:		// 19			// dried up leaves on the floor
		return qfalse;
		break;
	case MATERIAL_GREENLEAVES:		// 20			// fresh leaves still on a tree
		return qfalse;
		break;
	case MATERIAL_FABRIC:			// 21			// Cotton sheets
		return qfalse;
		break;
	case MATERIAL_CANVAS:			// 22			// tent material
		return qfalse;
		break;
	case MATERIAL_MARBLE:			// 12			// marble floors
		return qtrue;
		break;
	case MATERIAL_SNOW:				// 14			// freshly laid snow
		return qfalse;
		break;
	case MATERIAL_MUD:				// 17			// wet soil
		return qfalse;
		break;
	case MATERIAL_DIRT:				// 7			// hard mud
		return qfalse;
		break;
	case MATERIAL_CONCRETE:			// 11			// hardened concrete pavement
		return qfalse;
		break;
	case MATERIAL_FLESH:			// 16			// hung meat, corpses in the world
		return qfalse;
		break;
	case MATERIAL_RUBBER:			// 24			// hard tire like rubber
		return qfalse;
		break;
	case MATERIAL_PLASTIC:			// 25			//
		return qtrue;
		break;
	case MATERIAL_PLASTER:			// 28			// drywall style plaster
		return qfalse;
		break;
	case MATERIAL_SHATTERGLASS:		// 29			// glass with the Crisis Zone style shattering
		return qtrue;
		break;
	case MATERIAL_ARMOR:			// 30			// body armor
		return qtrue;
		break;
	case MATERIAL_ICE:				// 15			// packed snow/solid ice
		return qtrue;
		break;
	case MATERIAL_GLASS:			// 10			//
		return qtrue;
		break;
	case MATERIAL_BPGLASS:			// 18			// bulletproof glass
		return qtrue;
		break;
	case MATERIAL_COMPUTER:			// 31			// computers/electronic equipment
		return qtrue;
		break;
	default:
		return qfalse;
		break;
	}

	return qfalse;
}

//#define CUBEMAPS_AT_WAYPOINTS

#ifdef CUBEMAPS_AT_WAYPOINTS
#define		MOD_DIRECTORY "Warzone"
#define		BOT_MOD_NAME	"aimod"
float		NOD_VERSION = 1.1f;

int gWPNum = 0;
vec3_t waypoints[32768];
int skipzar = 0;

qboolean
AIMOD_NODES_LoadNodes ( void )
{
	fileHandle_t	f;
	int				i, j;
	char			filename[60];
	short int		objNum[3] = { 0, 0, 0 },
	objFlags, numLinks;
	int				flags;
	vec3_t			vec;
	short int		fl2;
	int				target;
	char			name[] = BOT_MOD_NAME;
	char			nm[64] = "";
	float			version;
	char			map[64] = "";
	char			mp[64] = "";
	/*short*/ int		numberNodes;
	short int		temp, fix_aas_nodes;

	gWPNum = 0;

	i = 0;
	strcpy( filename, "nodes/" );

	////////////////////
	//ri->Cvar_Register( &mapname, "mapname", "", CVAR_SERVERINFO | CVAR_ROM );

	Q_strcat( filename, sizeof(filename), currentMapName );

	///////////////////
	//open the node file for reading, return false on error
	ri->FS_FOpenFileRead( va( "%s.bwp", filename), &f, qfalse );
	
	if ( !f )
	{
		ri->FS_FCloseFile( f );
		ri->Printf( PRINT_WARNING, "^1*** ^3%s^5: Warzone (cube mapping) - Failed to find waypoint file ^7%s.bwp^5.\n", "Warzone", filename );
		return qfalse;
	}

	strcpy( mp, currentMapName );
	ri->FS_Read( &nm, strlen( name) + 1, f );									//read in a string the size of the mod name (+1 is because all strings end in hex '00')
	ri->FS_Read( &version, sizeof(float), f );			//read and make sure the version is the same

	if ( version != NOD_VERSION && version != 1.0f )
	{
		ri->Printf( PRINT_WARNING, "^1*** ^3WARNING^5: Warzone (cube mapping) - Reading from ^7%s.bwp^3 failed^5!!!\n", filename );
		ri->Printf( PRINT_WARNING, "^1*** ^3       ^5  Old node file detected.\n" );
		ri->FS_FCloseFile( f );
		return qfalse;
	}

	ri->FS_Read( &map, strlen( mp) + 1, f );			//make sure the file is for the current map
	if ( Q_stricmp( map, mp) != 0 )
	{
		ri->Printf( PRINT_WARNING, "^1*** ^3WARNING^5: Warzone (cube mapping) - Reading from ^7%s.bwp^3 failed^5!!!\n", filename );
		ri->Printf( PRINT_WARNING, "^1*** ^3       ^5  Node file is not for this map!\n" );
		ri->FS_FCloseFile( f );
		return qfalse;
	}

	if (version == NOD_VERSION)
	{
		ri->FS_Read( &numberNodes, sizeof(/*short*/ int), f ); //read in the number of nodes in the map
	}
	else
	{
		ri->FS_Read( &temp, sizeof(short int), f ); //read in the number of nodes in the map
		numberNodes = temp;
	}

	for ( i = 0; i < numberNodes; i++ )					//loop through all the nodes
	{
		int links[32];
		int link_flags[32];
		int new_flags = 0;

		for (j = 0; j < 32; j++)
		{
			links[j] = -1;
			link_flags[j] = -1;
		}

		//read in all the node info stored in the file
		ri->FS_Read( &vec, sizeof(vec3_t), f );
		ri->FS_Read( &flags, sizeof(int), f );
		ri->FS_Read( objNum, sizeof(short int) * 3, f );
		ri->FS_Read( &objFlags, sizeof(short int), f );
		ri->FS_Read( &numLinks, sizeof(short int), f );

		//Load_AddNode( vec, flags, objNum, objFlags );	//add the node

		//loop through all of the links and read the data
		for ( j = 0; j < numLinks; j++ )
		{
			if (version == NOD_VERSION)
			{
				ri->FS_Read( &target, sizeof(/*short*/ int), f );
			}
			else
			{
				ri->FS_Read( &temp, sizeof(short int), f );
				target = temp;
			}

			ri->FS_Read( &fl2, sizeof(short int), f );
			//ConnectNodes( i, target, fl2 );				//add any links
			links[j] = target;
			link_flags[j] = fl2;
		}

		qboolean haveClose = qfalse;
		float wpDistance = 96.0;

		if (numberNodes > 49170)
		{// Increase distance between cubemap render positions...
			wpDistance = 768.0;
		}
		else if (numberNodes > 32786)
		{// Increase distance between cubemap render positions...
			wpDistance = 512.0;
		}
		else if (numberNodes > 24576)
		{// Increase distance between cubemap render positions...
			wpDistance = 384.0;
		}
		else if (numberNodes > 16384)
		{// Increase distance between cubemap render positions...
			wpDistance = 256.0;
		}
		else if (numberNodes > 8192)
		{// Increase distance between cubemap render positions...
			wpDistance = 192.0;
		}
		else if (numberNodes > 4096)
		{// Increase distance between cubemap render positions...
			wpDistance = 128.0;
		}

		for (int z = 0; z < gWPNum; z++)
		{
			if (Distance(vec, waypoints[z]) < wpDistance) {
				haveClose = qtrue;
				break;
			}
		}

		if (!haveClose)
		{
			VectorCopy(vec, waypoints[gWPNum]);
			waypoints[gWPNum][2]+=32.0;
			gWPNum++;
		}
	}

	ri->FS_Read( &fix_aas_nodes, sizeof(short int), f );
	ri->FS_FCloseFile( f );							//close the file
	ri->Printf( PRINT_WARNING, "^1*** ^3%s^5: Warzone (cube mapping) - Successfully loaded %i cubemap points from advanced waypoint file ^7%s.bwp^5.\n", "Warzone",
			  numberNodes, filename );

	return qtrue;
}
#endif //CUBEMAPS_AT_WAYPOINTS

qboolean IgnoreCubemapsOnMap( void )
{// Maps with known really bad FPS... Let's just forget rendering cubemaps here...
#if 0
	if (StringContainsWord(currentMapName, "jkg_mos_eisley"))
	{// Ignore this map... We know we don't need shiny here...
		return qtrue;
	}

	if (StringContainsWord(currentMapName, "yavin_forest"))
	{// Ignore this map... We know we don't need shiny here...
		return qtrue;
	}

	if (StringContainsWord(currentMapName, "yavin_small"))
	{// Ignore this map... We know we don't need shiny here...
		return qtrue;
	}

	if (StringContainsWord(currentMapName, "yavin7"))
	{// Ignore this map... We know we don't need shiny here...
		return qtrue;
	}

	if (StringContainsWord(currentMapName, "yavin8"))
	{// Ignore this map... We know we don't need shiny here...
		return qtrue;
	}
#endif

	return qfalse;
}

float		MAP_WATER_LEVEL = 131072.0;

#define		MAX_GLOW_LOCATIONS 65536
int			NUM_MAP_GLOW_LOCATIONS = 0;
vec3_t		MAP_GLOW_LOCATIONS[MAX_GLOW_LOCATIONS] = { 0 };
vec4_t		MAP_GLOW_COLORS[MAX_GLOW_LOCATIONS] = { 0 };
qboolean	MAP_GLOW_COLORS_AVILABLE[MAX_GLOW_LOCATIONS] = { qfalse };

extern void R_WorldToLocal (const vec3_t world, vec3_t local);
extern void R_LocalPointToWorld (const vec3_t local, vec3_t world);

static void R_SetupMapGlowsAndWaterPlane( void )
{
	NUM_MAP_GLOW_LOCATIONS = 0;

	//
	// How about we look at the material types and select surfaces that need cubemaps and generate them there instead? :)
	//

	world_t	*w;

	w = &s_worldData;

//#pragma omp parallel for /*ordered*/ schedule(dynamic) //if (r_multithread->integer > 0)
	for (int i = 0; i < w->numsurfaces; i++)
	{// Get a count of how many we need... Add them to temp list if not too close to another...
		msurface_t *surf =	&w->surfaces[i];
		vec3_t				surfOrigin;
		qboolean			bad = qfalse;

		qboolean	hasGlow = qfalse;
		vec4_t		glowColor = { 0 };

		if (surf->shader)
		{
			for ( int stage = 0; stage < MAX_SHADER_STAGES; stage++ )
			{
				if (surf->shader->stages[stage] && surf->shader->stages[stage]->glow)
				{
#ifdef USING_ENGINE_GLOW_LIGHTCOLORS_SEARCH
					hasGlow = qtrue;
					VectorCopy4(surf->shader->stages[stage]->bundle[0].image[0]->lightColor, glowColor);
#else
					if (surf->shader->stages[stage]->glowColorFound)
					{
						hasGlow = qtrue;
						VectorCopy4(surf->shader->stages[stage]->glowColor, glowColor);
					}
					else
					{
						hasGlow = qfalse;
					}
#endif
					break;
				}
			}
		}

		if (surf->cullinfo.type & CULLINFO_SPHERE)
		{
			VectorCopy(surf->cullinfo.localOrigin, surfOrigin);
		}
		else if (surf->cullinfo.type & CULLINFO_BOX)
		{
			surfOrigin[0] = (surf->cullinfo.bounds[0][0] + surf->cullinfo.bounds[1][0]) * 0.5f;
			surfOrigin[1] = (surf->cullinfo.bounds[0][1] + surf->cullinfo.bounds[1][1]) * 0.5f;
			surfOrigin[2] = (surf->cullinfo.bounds[0][2] + surf->cullinfo.bounds[1][2]) * 0.5f;
		}
		else
		{
			continue;
		}

		if ((surf->shader->surfaceFlags & MATERIAL_MASK) == MATERIAL_WATER)
		{// While doing this, also find lowest water height, so that we can cull underwater grass drawing...
			if (surfOrigin[2] < MAP_WATER_LEVEL)
				MAP_WATER_LEVEL = surfOrigin[2];
		}

		if (hasGlow && NUM_MAP_GLOW_LOCATIONS < MAX_GLOW_LOCATIONS)
		{
			VectorCopy(surfOrigin, MAP_GLOW_LOCATIONS[NUM_MAP_GLOW_LOCATIONS]);
			VectorCopy4(glowColor, MAP_GLOW_COLORS[NUM_MAP_GLOW_LOCATIONS]);
			MAP_GLOW_COLORS_AVILABLE[NUM_MAP_GLOW_LOCATIONS] = qtrue;
			NUM_MAP_GLOW_LOCATIONS++;
		}
	}

	if (MAP_WATER_LEVEL >= 131072.0)
	{// No water plane was found, set to map mins...
		MAP_WATER_LEVEL = -131072.0;
	}
}

static void R_LoadCubemapWaypoints( void )
{
	int numCubemaps = 0;

	// count cubemaps
	numCubemaps = 0;

	NUM_MAP_GLOW_LOCATIONS = 0;

	if (IgnoreCubemapsOnMap()) return;

#ifdef CUBEMAPS_AT_WAYPOINTS
	//
	// Generate cubemaps at waypoints...
	//

	if (!AIMOD_NODES_LoadNodes()) return;

	numCubemaps = gWPNum;

	if (!numCubemaps)
		return;

	tr.numCubemaps = numCubemaps;
	tr.cubemapOrigins = (vec3_t *)ri->Hunk_Alloc( tr.numCubemaps * sizeof(*tr.cubemapOrigins), h_low);
	tr.cubemaps = (image_t **)ri->Hunk_Alloc( tr.numCubemaps * sizeof(*tr.cubemaps), h_low);

	numCubemaps = 0;
	for (int i = 0; i < gWPNum; i++)
	{
		VectorCopy(waypoints[i], tr.cubemapOrigins[numCubemaps]);
		numCubemaps++;
	}

	ri->Printf(PRINT_WARNING, "^1*** ^3%s^5: Warzone (cube mapping) - Selected %i waypoints for cubemaps.\n", "Warzone", numCubemaps);
#else //!CUBEMAPS_AT_WAYPOINTS
	//
	// How about we look at the material types and select surfaces that need cubemaps and generate them there instead? :)
	//

	world_t	*w;
	vec3_t	*cubeOrgs;
	int		numcubeOrgs = 0;

	cubeOrgs = (vec3_t *)malloc(sizeof(vec3_t)*1048576);

	w = &s_worldData;

//#pragma omp parallel for /*ordered*/ schedule(dynamic) //if (r_multithread->integer > 0)
	for (int i = 0; i < w->numsurfaces; i++)
	{// Get a count of how many we need... Add them to temp list if not too close to another...
		msurface_t *surf =	&w->surfaces[i];
		vec3_t				surfOrigin;
		qboolean			bad = qfalse;

		qboolean	hasGlow = qfalse;
		vec4_t		glowColor = { 0 };

		if (surf->shader)
		{
			for ( int stage = 0; stage < MAX_SHADER_STAGES; stage++ )
			{
				if (surf->shader->stages[stage] && surf->shader->stages[stage]->glow)
				{
#ifdef USING_ENGINE_GLOW_LIGHTCOLORS_SEARCH
					hasGlow = qtrue;
					VectorCopy4(surf->shader->stages[stage]->bundle[0].image[0]->lightColor, glowColor);
#else
					if (surf->shader->stages[stage]->glowColorFound)
					{
						hasGlow = qtrue;
						VectorCopy4(surf->shader->stages[stage]->glowColor, glowColor);
					}
					else
					{
						hasGlow = qfalse;
					}
#endif
					break;
				}
			}
		}

		if (R_MaterialUsesCubemap( surf->shader->surfaceFlags )
			|| (surf->shader->surfaceFlags & MATERIAL_MASK) == MATERIAL_WATER)
		{// Ok, this surface is shiny... Make a cubemap here...
			if (surf->cullinfo.type & CULLINFO_SPHERE)
			{
				VectorCopy(surf->cullinfo.localOrigin, surfOrigin);
			}
			else if (surf->cullinfo.type & CULLINFO_BOX)
			{
				surfOrigin[0] = (surf->cullinfo.bounds[0][0] + surf->cullinfo.bounds[1][0]) * 0.5f;
				surfOrigin[1] = (surf->cullinfo.bounds[0][1] + surf->cullinfo.bounds[1][1]) * 0.5f;
				surfOrigin[2] = (surf->cullinfo.bounds[0][2] + surf->cullinfo.bounds[1][2]) * 0.5f;
			}
			else
			{
				continue;
			}

			if ((surf->shader->surfaceFlags & MATERIAL_MASK) == MATERIAL_WATER)
			{// While doing this, also find lowest water height, so that we can cull underwater grass drawing...
				if (surfOrigin[2] < MAP_WATER_LEVEL)
					MAP_WATER_LEVEL = surfOrigin[2];
			}


			if (hasGlow && NUM_MAP_GLOW_LOCATIONS < MAX_GLOW_LOCATIONS)
			{
				VectorCopy(surfOrigin, MAP_GLOW_LOCATIONS[NUM_MAP_GLOW_LOCATIONS]);
				VectorCopy4(glowColor, MAP_GLOW_COLORS[NUM_MAP_GLOW_LOCATIONS]);
				MAP_GLOW_COLORS_AVILABLE[NUM_MAP_GLOW_LOCATIONS] = qtrue;
				NUM_MAP_GLOW_LOCATIONS++;
			}

			for (int j = 0; j < numcubeOrgs; j++)
			{
				if (Distance(cubeOrgs[j], surfOrigin) < 1024) //256) // Larger separation distance for JKG.
				{
					bad = qtrue;
					break;
				}
			}

			if (bad) continue;

//#pragma omp critical
			{
				VectorCopy(surfOrigin, cubeOrgs[numcubeOrgs]);
				numcubeOrgs++;
			}
		}
		else if (hasGlow)
		{
			if (surf->cullinfo.type & CULLINFO_SPHERE)
			{
				VectorCopy(surf->cullinfo.localOrigin, surfOrigin);
			}
			else if (surf->cullinfo.type & CULLINFO_BOX)
			{
				surfOrigin[0] = (surf->cullinfo.bounds[0][0] + surf->cullinfo.bounds[1][0]) * 0.5f;
				surfOrigin[1] = (surf->cullinfo.bounds[0][1] + surf->cullinfo.bounds[1][1]) * 0.5f;
				surfOrigin[2] = (surf->cullinfo.bounds[0][2] + surf->cullinfo.bounds[1][2]) * 0.5f;
			}
			else
			{
				continue;
			}

			if (NUM_MAP_GLOW_LOCATIONS < MAX_GLOW_LOCATIONS)
			{
				VectorCopy(surfOrigin, MAP_GLOW_LOCATIONS[NUM_MAP_GLOW_LOCATIONS]);
				VectorCopy4(glowColor, MAP_GLOW_COLORS[NUM_MAP_GLOW_LOCATIONS]);
				MAP_GLOW_COLORS_AVILABLE[NUM_MAP_GLOW_LOCATIONS] = qtrue;
				NUM_MAP_GLOW_LOCATIONS++;
			}
		}
	}

	if (MAP_WATER_LEVEL >= 131072.0)
	{// No water plane was found, set to map mins...
		MAP_WATER_LEVEL = -131072.0;
	}

	tr.numCubemaps = numcubeOrgs;
	tr.cubemapOrigins = (vec3_t *)ri->Hunk_Alloc( tr.numCubemaps * sizeof(*tr.cubemapOrigins), h_low);
	tr.cubemaps = (image_t **)ri->Hunk_Alloc( tr.numCubemaps * sizeof(*tr.cubemaps), h_low);

	numCubemaps = 0;

	for (int i = 0; i < numcubeOrgs; i++)
	{// Copy to real list...
		VectorCopy(cubeOrgs[i], tr.cubemapOrigins[numCubemaps]);
		numCubemaps++;
	}

	free(cubeOrgs);

	ri->Printf(PRINT_WARNING, "^1*** ^3%s^5: Warzone (cube mapping) - Selected %i surfaces for cubemaps.\n", "Warzone", numCubemaps);
#endif //CUBEMAPS_AT_WAYPOINTS
}

static void R_AssignCubemapsToWorldSurfaces(void)
{
	world_t	*w;
	int i;

	w = &s_worldData;

	for (i = 0; i < w->numsurfaces; i++)
	{
		msurface_t *surf = &w->surfaces[i];
		vec3_t surfOrigin;
		
		if (!R_MaterialUsesCubemap( surf->shader->surfaceFlags ))
		{
			surf->cubemapIndex = 0;
		}

		if (surf->cullinfo.type & CULLINFO_SPHERE)
		{
			VectorCopy(surf->cullinfo.localOrigin, surfOrigin);
		}
		else if (surf->cullinfo.type & CULLINFO_BOX)
		{
			surfOrigin[0] = (surf->cullinfo.bounds[0][0] + surf->cullinfo.bounds[1][0]) * 0.5f;
			surfOrigin[1] = (surf->cullinfo.bounds[0][1] + surf->cullinfo.bounds[1][1]) * 0.5f;
			surfOrigin[2] = (surf->cullinfo.bounds[0][2] + surf->cullinfo.bounds[1][2]) * 0.5f;
		}
		else
		{
			//ri.Printf(PRINT_ALL, "surface %d has no cubemap\n", i);
			continue;
		}

		surf->cubemapIndex = R_CubemapForPoint(surfOrigin);
		//ri.Printf(PRINT_ALL, "surface %d has cubemap %d\n", i, surf->cubemapIndex);
	}
}


static void R_RenderAllCubemaps(void)
{
	int i, j;
	GLenum cubemapFormat = GL_RGBA8;

	if ( r_hdr->integer )
	{
		cubemapFormat = GL_RGBA16F;
	}

	for (i = 0; i < tr.numCubemaps; i++)
	{
		//tr.cubemaps[i] = R_CreateImage (va ("*cubeMap%d", i), NULL, CUBE_MAP_SIZE, CUBE_MAP_SIZE, IMGTYPE_COLORALPHA, IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE | IMGFLAG_MIPMAP | IMGFLAG_CUBEMAP, cubemapFormat);
		tr.cubemaps[i] = R_CreateImage (va ("*cubeMap%d", i), NULL, r_cubeMapSize->integer, r_cubeMapSize->integer, IMGTYPE_COLORALPHA, IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE | IMGFLAG_MIPMAP | IMGFLAG_CUBEMAP, cubemapFormat);
	}

	for (i = 0; i < tr.numCubemaps; i++)
	{
		for (j = 0; j < 6; j++)
		{
			RE_ClearScene();
			R_RenderCubemapSide(i, j, qfalse);
			R_IssuePendingRenderCommands();
			R_InitNextFrame();
		}
	}
}


/*
=================
R_MergeLeafSurfaces

Merges surfaces that share a common leaf
=================
*/

void R_MergeLeafSurfaces(void)
{
	int i, j, k;
	int numWorldSurfaces;
	int mergedSurfIndex;
	int numMergedSurfaces;
	int numUnmergedSurfaces;
	int numIboIndexes;
	int startTime, endTime;
	msurface_t *mergedSurf;

	startTime = ri->Milliseconds();

	numWorldSurfaces = s_worldData.numWorldSurfaces;

	// use viewcount to keep track of mergers
	for (i = 0; i < numWorldSurfaces; i++)
	{
		s_worldData.surfacesViewCount[i] = -1;
	}

	// mark matching surfaces
	for (i = 0; i < s_worldData.numnodes - s_worldData.numDecisionNodes; i++)
	{
		mnode_t *leaf = s_worldData.nodes + s_worldData.numDecisionNodes + i;

		for (j = 0; j < leaf->nummarksurfaces; j++)
		{
#ifdef __MERGE_MORE__
			msurface_t *surf1;
			shader_t *shader1;
			int fogIndex1;
			int cubemapIndex1;
			int surfNum1;

			surfNum1 = *(s_worldData.marksurfaces + leaf->firstmarksurface + j);

			if (s_worldData.surfacesViewCount[surfNum1] != -1)
				continue;

			surf1 = s_worldData.surfaces + surfNum1;

			shader1 = surf1->shader;

			if(shader1->isSky)
				continue;

			if(shader1->isPortal)
				continue;

			if(ShaderRequiresCPUDeforms(shader1))
				continue;

			fogIndex1 = surf1->fogIndex;
			
			cubemapIndex1 = surf1->cubemapIndex;

			s_worldData.surfacesViewCount[surfNum1] = surfNum1;

			for (k = j + 1; k < leaf->nummarksurfaces; k++)
			{
				msurface_t *surf2;
				shader_t *shader2;
				int cubemapIndex2;
				int surfNum2;

				surfNum2 = *(s_worldData.marksurfaces + leaf->firstmarksurface + k);

				if (s_worldData.surfacesViewCount[surfNum2] != -1)
					continue;
				
				surf2 = s_worldData.surfaces + surfNum2;

				shader2 = surf2->shader;

				/*if (Distance(surf1->cullinfo.bounds[0], surf2->cullinfo.bounds[0]) > 1024)
				{
					continue;
				}*/

#ifdef __MERGE_SAME_SHADER_NAMES__
				if (shader1 && shader2 
					&& shader1->stages[0] 
					&& shader2->stages[0] 
					&& ( r_glslWater->integer && shader1->stages[0]->isWater && shader2->stages[0]->isWater))
				{// UQ1: All water can be safely merged I believe...
					s_worldData.surfacesViewCount[surfNum2] = surfNum1;
					continue;
				}
				else if (shader1 != shader2
					// Merge matching shader names...
					&& stricmp(shader1->name, shader2->name))
				{
					continue;
				}
#else //!__MERGE_SAME_SHADER_NAMES__
				if (shader1 && shader2 && shader1->stages[0] && shader2->stages[0] && (r_glslWater->integer && shader1->stages[0]->isWater && shader2->stages[0]->isWater))
				{// UQ1: All water can be safely merged I believe...
					s_worldData.surfacesViewCount[surfNum2] = surfNum1;
					continue;
				}
				else if (shader1 != shader2)
				{
					continue;
				}
#endif //__MERGE_SAME_SHADER_NAMES__

				cubemapIndex2 = surf2->cubemapIndex;

				if (cubemapIndex1 != cubemapIndex2 
					&& R_MaterialUsesCubemap(shader1->surfaceFlags) 
					&& R_MaterialUsesCubemap(shader2->surfaceFlags))
				{
					if (Distance(tr.cubemapOrigins[cubemapIndex1], tr.cubemapOrigins[cubemapIndex2]) > 512.0)
					{// Too far from original cubemap, let's not merge this one...
						continue;
					}
				}

				s_worldData.surfacesViewCount[surfNum2] = surfNum1;
			}
#else //!__MERGE_MORE__
			msurface_t *surf1;
			shader_t *shader1;
			int fogIndex1;
			int cubemapIndex1;
			int surfNum1;

			surfNum1 = *(s_worldData.marksurfaces + leaf->firstmarksurface + j);

			if (s_worldData.surfacesViewCount[surfNum1] != -1)
				continue;

			surf1 = s_worldData.surfaces + surfNum1;

			if ((*surf1->data != SF_GRID) && (*surf1->data != SF_TRIANGLES) && (*surf1->data != SF_FACE))
				continue;

			shader1 = surf1->shader;

			if(shader1->isSky)
				continue;

			if(shader1->isPortal)
				continue;

			if(ShaderRequiresCPUDeforms(shader1))
				continue;

			fogIndex1 = surf1->fogIndex;
			
			cubemapIndex1 = surf1->cubemapIndex;

			s_worldData.surfacesViewCount[surfNum1] = surfNum1;

			for (k = j + 1; k < leaf->nummarksurfaces; k++)
			{
				msurface_t *surf2;
				shader_t *shader2;
				int fogIndex2;
				int cubemapIndex2;
				int surfNum2;

				surfNum2 = *(s_worldData.marksurfaces + leaf->firstmarksurface + k);

				if (s_worldData.surfacesViewCount[surfNum2] != -1)
					continue;
				
				surf2 = s_worldData.surfaces + surfNum2;

				if ((*surf2->data != SF_GRID) && (*surf2->data != SF_TRIANGLES) && (*surf2->data != SF_FACE))
					continue;

				shader2 = surf2->shader;

				if (shader1 != shader2)
				{
					continue;
				}

				fogIndex2 = surf2->fogIndex;

				if (fogIndex1 != fogIndex2)
					continue;

				cubemapIndex2 = surf2->cubemapIndex;

				if (cubemapIndex1 != cubemapIndex2)
					continue;

				s_worldData.surfacesViewCount[surfNum2] = surfNum1;
			}
#endif //__MERGE_MORE__
		}
	}

	// don't add surfaces that don't merge to any others to the merged list
	for (i = 0; i < numWorldSurfaces; i++)
	{
		qboolean merges = qfalse;

		if (s_worldData.surfacesViewCount[i] != i)
			continue;

		for (j = 0; j < numWorldSurfaces; j++)
		{
			if (j == i)
				continue;

			if (s_worldData.surfacesViewCount[j] == i)
			{
				merges = qtrue;
				break;
			}
		}

		if (!merges)
			s_worldData.surfacesViewCount[i] = -1;
	}

	// count merged/unmerged surfaces
	numMergedSurfaces = 0;
	numUnmergedSurfaces = 0;

	for (i = 0; i < numWorldSurfaces; i++)
	{
		if (s_worldData.surfacesViewCount[i] == i)
		{
			numMergedSurfaces++;
		}
		else if (s_worldData.surfacesViewCount[i] == -1)
		{
			numUnmergedSurfaces++;
		}
	}

	// Allocate merged surfaces
	s_worldData.mergedSurfaces = (msurface_t *)ri->Hunk_Alloc(sizeof(*s_worldData.mergedSurfaces) * numMergedSurfaces, h_low);
	s_worldData.mergedSurfacesViewCount = (int *)ri->Hunk_Alloc(sizeof(*s_worldData.mergedSurfacesViewCount) * numMergedSurfaces, h_low);
	//s_worldData.mergedSurfacesDlightBits = (int *)ri->Hunk_Alloc(sizeof(*s_worldData.mergedSurfacesDlightBits) * numMergedSurfaces, h_low);
#ifdef __PSHADOWS__
	s_worldData.mergedSurfacesPshadowBits = (int *)ri->Hunk_Alloc(sizeof(*s_worldData.mergedSurfacesPshadowBits) * numMergedSurfaces, h_low);
#endif
	s_worldData.numMergedSurfaces = numMergedSurfaces;
	
	// view surfaces are like mark surfaces, except negative ones represent merged surfaces
	// -1 represents 0, -2 represents 1, and so on
	s_worldData.viewSurfaces = (int *)ri->Hunk_Alloc(sizeof(*s_worldData.viewSurfaces) * s_worldData.nummarksurfaces, h_low);

	// copy view surfaces into mark surfaces
	for (i = 0; i < s_worldData.nummarksurfaces; i++)
	{
		s_worldData.viewSurfaces[i] = s_worldData.marksurfaces[i];
	}

	// need to be synched here
	R_IssuePendingRenderCommands();

	// actually merge surfaces
	numIboIndexes = 0;
	mergedSurfIndex = 0;
	mergedSurf = s_worldData.mergedSurfaces;

	for (i = 0; i < numWorldSurfaces; i++)
	{
		msurface_t *surf1;
		VBO_t *vbo;
		IBO_t *ibo;
		glIndex_t *iboIndexes, *outIboIndexes;

		vec3_t bounds[2];

		int numSurfsToMerge;
		int numIndexes;
		int numVerts;
		int firstIndex;

		srfBspSurface_t *vboSurf;

		if (s_worldData.surfacesViewCount[i] != i)
			continue;

		surf1 = s_worldData.surfaces + i;

		// retrieve vbo
		vbo = ((srfBspSurface_t *)(surf1->data))->vbo;

		// count verts, indexes, and surfaces
		numSurfsToMerge = 0;
		numIndexes = 0;
		numVerts = 0;

		for (j = i; j < numWorldSurfaces; j++)
		{
			msurface_t *surf2;
			srfBspSurface_t *bspSurf;

			if (s_worldData.surfacesViewCount[j] != i)
				continue;

			surf2 = s_worldData.surfaces + j;

			bspSurf = (srfBspSurface_t *) surf2->data;
			numIndexes += bspSurf->numIndexes;
			numVerts += bspSurf->numVerts;
			numSurfsToMerge++;
		}

		if (numVerts == 0 || numIndexes == 0 || numSurfsToMerge < 2)
		{
			continue;
		}

		// create ibo
		ibo = tr.ibos[tr.numIBOs++] = (IBO_t*)ri->Hunk_Alloc(sizeof(*ibo), h_low);
		memset(ibo, 0, sizeof(*ibo));
		numIboIndexes = 0;

		// allocate indexes
		iboIndexes = outIboIndexes = (glIndex_t*)Z_Malloc(numIndexes * sizeof(*outIboIndexes), TAG_BSP);

		// Merge surfaces (indexes) and calculate bounds
		ClearBounds(bounds[0], bounds[1]);
		firstIndex = numIboIndexes;
		for (j = i; j < numWorldSurfaces; j++)
		{
			msurface_t *surf2;
			srfBspSurface_t *bspSurf;

			if (s_worldData.surfacesViewCount[j] != i)
				continue;

			surf2 = s_worldData.surfaces + j;

			AddPointToBounds(surf2->cullinfo.bounds[0], bounds[0], bounds[1]);
			AddPointToBounds(surf2->cullinfo.bounds[1], bounds[0], bounds[1]);

			bspSurf = (srfBspSurface_t *) surf2->data;

			for (k = 0; k < bspSurf->numIndexes; k++)
			{
				*outIboIndexes++ = bspSurf->indexes[k] + bspSurf->firstVert;
				numIboIndexes++;
			}
			break;
		}

		vboSurf = (srfBspSurface_t *)ri->Hunk_Alloc(sizeof(*vboSurf), h_low);
		memset(vboSurf, 0, sizeof(*vboSurf));

		vboSurf->surfaceType = SF_VBO_MESH;

		vboSurf->vbo = vbo;
		vboSurf->ibo = ibo;

		vboSurf->numIndexes = numIndexes;
		vboSurf->numVerts = numVerts;
		vboSurf->firstIndex = firstIndex;

		vboSurf->minIndex = *(iboIndexes + firstIndex);
		vboSurf->maxIndex = *(iboIndexes + firstIndex);

		for (j = 0; j < numIndexes; j++)
		{
			vboSurf->minIndex = MIN(vboSurf->minIndex, *(iboIndexes + firstIndex + j));
			vboSurf->maxIndex = MAX(vboSurf->maxIndex, *(iboIndexes + firstIndex + j));
		}

		VectorCopy(bounds[0], vboSurf->cullBounds[0]);
		VectorCopy(bounds[1], vboSurf->cullBounds[1]);

		VectorCopy(bounds[0], mergedSurf->cullinfo.bounds[0]);
		VectorCopy(bounds[1], mergedSurf->cullinfo.bounds[1]);

		mergedSurf->cullinfo.type = CULLINFO_BOX;
		mergedSurf->data          = (surfaceType_t *)vboSurf;
		mergedSurf->fogIndex      = surf1->fogIndex;
		mergedSurf->cubemapIndex  = surf1->cubemapIndex;
		mergedSurf->shader        = surf1->shader;

		// finish up the ibo
		qglGenBuffers(1, &ibo->indexesVBO);

		R_BindIBO(ibo);
		qglBufferData(GL_ELEMENT_ARRAY_BUFFER, numIboIndexes * sizeof(*iboIndexes), iboIndexes, ibo->iboUsage);
		R_BindNullIBO();

		GL_CheckErrors();

   		Z_Free(iboIndexes);

		// redirect view surfaces to this surf
		for (j = 0; j < numWorldSurfaces; j++)
		{
			if (s_worldData.surfacesViewCount[j] != i)
				continue;

			for (k = 0; k < s_worldData.nummarksurfaces; k++)
			{
				int *mark = s_worldData.marksurfaces + k;
				int *view = s_worldData.viewSurfaces + k;

				if (*mark == j)
					*view = -(mergedSurfIndex + 1);
			}
		}

		mergedSurfIndex++;
		mergedSurf++;
	}

	endTime = ri->Milliseconds();

	ri->Printf(PRINT_ALL, "Processed %d surfaces into %d merged, %d unmerged in %5.2f seconds\n", 
		numWorldSurfaces, numMergedSurfaces, numUnmergedSurfaces, (endTime - startTime) / 1000.0f);

	// reset viewcounts
	for (i = 0; i < numWorldSurfaces; i++)
	{
		s_worldData.surfacesViewCount[i] = -1;
	}
}


static void R_CalcVertexLightDirs( void )
{
	int i, k;

	for(k = 0; k < s_worldData.numsurfaces /* s_worldData.numWorldSurfaces */; k++)
	{
		msurface_t *surface = &s_worldData.surfaces[k];
		srfBspSurface_t *bspSurf = (srfBspSurface_t *) surface->data;

		switch(bspSurf->surfaceType)
		{
			case SF_FACE:
			case SF_GRID:
			case SF_TRIANGLES:

				for(i = 0; i < bspSurf->numVerts; i++)
					R_LightDirForPoint( bspSurf->verts[i].xyz, bspSurf->verts[i].lightdir, bspSurf->verts[i].normal, &s_worldData );

				break;

			default:
				break;
		}
	}
}

void StripMaps( const char *in, char *out, int destsize )
{
	const char *dot = strrchr(in, '_'), *slash;
	if (dot && (!(slash = strrchr(in, '/')) || slash < dot))
		destsize = (destsize < dot-in+1 ? destsize : dot-in+1);

	if ( in == out && destsize > 1 )
		out[destsize-1] = '\0';
	else
		Q_strncpyz(out, in, destsize);
}


/*
=================
RE_LoadWorldMap

Called directly from cgame
=================
*/

void RE_LoadWorldMap( const char *name ) {
	int			i;
	dheader_t	*header;
	union {
		byte *b;
		void *v;
	} buffer;
	byte		*startMarker;

	if ( tr.worldMapLoaded ) {
		ri->Error( ERR_DROP, "ERROR: attempted to redundantly load world map" );
	}

	MAP_WATER_LEVEL = 131072.0;

	{// Set currentMapName string...
		using namespace std;

		string str = name;
		string str2 = "maps/";

		// Find the word in the string
		std::string::size_type pos = str.find(str2);

		// If we find the word we replace it.
		if(pos != std::string::npos)
		{
			str.replace(pos, str2.size(), "");
		}

		COM_StripExtension(str.c_str(), currentMapName, sizeof(currentMapName));
	}

	// set default map light scale
	tr.mapLightScale  = 1.0f;
	tr.sunShadowScale = 0.5f;

	// set default sun direction to be used if it isn't
	// overridden by a shader
	tr.sunDirection[0] = 0.45f;
	tr.sunDirection[1] = 0.3f;
	tr.sunDirection[2] = 0.9f;

	VectorNormalize( tr.sunDirection );

	// set default autoexposure settings
	tr.autoExposureMinMax[0] = -2.0f;
	tr.autoExposureMinMax[1] = 2.0f;

	// set default tone mapping settings
	tr.toneMinAvgMaxLevel[0] = -8.0f;
	tr.toneMinAvgMaxLevel[1] = -2.0f;
	tr.toneMinAvgMaxLevel[2] = 0.0f;

	tr.worldMapLoaded = qtrue;

	// load it
    ri->FS_ReadFile( name, &buffer.v );
	if ( !buffer.b ) {
		ri->Error (ERR_DROP, "RE_LoadWorldMap: %s not found", name);
	}

	// clear tr.world so if the level fails to load, the next
	// try will not look at the partially loaded version
	tr.world = NULL;

	Com_Memset( &s_worldData, 0, sizeof( s_worldData ) );
	Q_strncpyz( s_worldData.name, name, sizeof( s_worldData.name ) );

	Q_strncpyz( s_worldData.baseName, COM_SkipPath( s_worldData.name ), sizeof( s_worldData.name ) );
	COM_StripExtension(s_worldData.baseName, s_worldData.baseName, sizeof(s_worldData.baseName));

	startMarker = (byte *)ri->Hunk_Alloc(0, h_low);
	c_gridVerts = 0;

	header = (dheader_t *)buffer.b;
	fileBase = (byte *)header;

	i = LittleLong (header->version);
	if ( i != BSP_VERSION ) {
		ri->Error (ERR_DROP, "RE_LoadWorldMap: %s has wrong version number (%i should be %i)", 
			name, i, BSP_VERSION);
	}

	// swap all the lumps
	for (i=0 ; i<sizeof(dheader_t)/4 ; i++) {
		((int *)header)[i] = LittleLong ( ((int *)header)[i]);
	}

	// load into heap
	R_LoadEntities( &header->lumps[LUMP_ENTITIES] );
	R_LoadShaders( &header->lumps[LUMP_SHADERS] );
	R_LoadLightmaps( &header->lumps[LUMP_LIGHTMAPS], &header->lumps[LUMP_SURFACES] );
	R_LoadPlanes (&header->lumps[LUMP_PLANES]);
	R_LoadFogs( &header->lumps[LUMP_FOGS], &header->lumps[LUMP_BRUSHES], &header->lumps[LUMP_BRUSHSIDES] );
	R_LoadSurfaces( &header->lumps[LUMP_SURFACES], &header->lumps[LUMP_DRAWVERTS], &header->lumps[LUMP_DRAWINDEXES] );
	R_LoadMarksurfaces (&header->lumps[LUMP_LEAFSURFACES]);
	R_LoadNodesAndLeafs (&header->lumps[LUMP_NODES], &header->lumps[LUMP_LEAFS]);
	R_LoadSubmodels (&header->lumps[LUMP_MODELS]);
	R_LoadVisibility( &header->lumps[LUMP_VISIBILITY] );
	R_LoadLightGrid( &header->lumps[LUMP_LIGHTGRID] );
	R_LoadLightGridArray( &header->lumps[LUMP_LIGHTARRAY] );
	
	// determine vertex light directions
	R_CalcVertexLightDirs();

	// determine which parts of the map are in sunlight
#if 0
	if (0)
	{
		world_t	*w;

		uint8_t *primaryLightGrid, *data;
		int lightGridSize;
		int i;

		w = &s_worldData;

		lightGridSize = w->lightGridBounds[0] * w->lightGridBounds[1] * w->lightGridBounds[2];
		primaryLightGrid = (uint8_t *)Z_Malloc(lightGridSize * sizeof(*primaryLightGrid), TAG_GENERAL);

		memset(primaryLightGrid, 0, lightGridSize * sizeof(*primaryLightGrid));

		data = w->lightGridData;
		for (i = 0; i < lightGridSize; i++, data += 8)
		{
			int lat, lng;
			vec3_t gridLightDir, gridLightCol;

			// skip samples in wall
			if (!(data[0]+data[1]+data[2]+data[3]+data[4]+data[5]) )
				continue;

			gridLightCol[0] = ByteToFloat(data[3]);
			gridLightCol[1] = ByteToFloat(data[4]);
			gridLightCol[2] = ByteToFloat(data[5]);
			(void)gridLightCol; // Suppress unused-but-set-variable warning

			lat = data[7];
			lng = data[6];
			lat *= (FUNCTABLE_SIZE/256);
			lng *= (FUNCTABLE_SIZE/256);

			// decode X as cos( lat ) * sin( long )
			// decode Y as sin( lat ) * sin( long )
			// decode Z as cos( long )

			gridLightDir[0] = tr.sinTable[(lat+(FUNCTABLE_SIZE/4))&FUNCTABLE_MASK] * tr.sinTable[lng];
			gridLightDir[1] = tr.sinTable[lat] * tr.sinTable[lng];
			gridLightDir[2] = tr.sinTable[(lng+(FUNCTABLE_SIZE/4))&FUNCTABLE_MASK];

			// FIXME: magic number for determining if light direction is close enough to sunlight
			if (DotProduct(gridLightDir, tr.sunDirection) > 0.75f)
			{
				primaryLightGrid[i] = 1;
			}
			else
			{
				primaryLightGrid[i] = 255;
			}
		}

		if (0)
		{
			int i;
			byte *buffer = (byte *)Z_Malloc(w->lightGridBounds[0] * w->lightGridBounds[1] * 3 + 18, TAG_GENERAL);
			byte *out;
			uint8_t *in;
			char fileName[MAX_QPATH];
			
			Com_Memset (buffer, 0, 18);
			buffer[2] = 2;		// uncompressed type
			buffer[12] = w->lightGridBounds[0] & 255;
			buffer[13] = w->lightGridBounds[0] >> 8;
			buffer[14] = w->lightGridBounds[1] & 255;
			buffer[15] = w->lightGridBounds[1] >> 8;
			buffer[16] = 24;	// pixel size

			in = primaryLightGrid;
			for (i = 0; i < w->lightGridBounds[2]; i++)
			{
				int j;

				sprintf(fileName, "primarylg%d.tga", i);

				out = buffer + 18;
				for (j = 0; j < w->lightGridBounds[0] * w->lightGridBounds[1]; j++)
				{
					if (*in == 1)
					{
						*out++ = 255;
						*out++ = 255;
						*out++ = 255;
					}
					else if (*in == 255)
					{
						*out++ = 64;
						*out++ = 64;
						*out++ = 64;
					}
					else
					{
						*out++ = 0;
						*out++ = 0;
						*out++ = 0;
					}
					in++;
				}

				ri->FS_WriteFile(fileName, buffer, w->lightGridBounds[0] * w->lightGridBounds[1] * 3 + 18);
			}

			Z_Free(buffer);
		}

		for (i = 0; i < w->numWorldSurfaces; i++)
		{
			msurface_t *surf = w->surfaces + i;
			cullinfo_t *ci = &surf->cullinfo;

			if(ci->type & CULLINFO_PLANE)
			{
				if (DotProduct(ci->plane.normal, tr.sunDirection) <= 0.0f)
				{
					//ri->Printf(PRINT_ALL, "surface %d is not oriented towards sunlight\n", i);
					continue;
				}
			}

			if(ci->type & CULLINFO_BOX)
			{
				int ibounds[2][3], x, y, z, goodSamples, numSamples;
				vec3_t lightOrigin;

				VectorSubtract( ci->bounds[0], w->lightGridOrigin, lightOrigin );

				ibounds[0][0] = floor(lightOrigin[0] * w->lightGridInverseSize[0]);
				ibounds[0][1] = floor(lightOrigin[1] * w->lightGridInverseSize[1]);
				ibounds[0][2] = floor(lightOrigin[2] * w->lightGridInverseSize[2]);

				VectorSubtract( ci->bounds[1], w->lightGridOrigin, lightOrigin );

				ibounds[1][0] = ceil(lightOrigin[0] * w->lightGridInverseSize[0]);
				ibounds[1][1] = ceil(lightOrigin[1] * w->lightGridInverseSize[1]);
				ibounds[1][2] = ceil(lightOrigin[2] * w->lightGridInverseSize[2]);

				ibounds[0][0] = CLAMP(ibounds[0][0], 0, w->lightGridSize[0]);
				ibounds[0][1] = CLAMP(ibounds[0][1], 0, w->lightGridSize[1]);
				ibounds[0][2] = CLAMP(ibounds[0][2], 0, w->lightGridSize[2]);

				ibounds[1][0] = CLAMP(ibounds[1][0], 0, w->lightGridSize[0]);
				ibounds[1][1] = CLAMP(ibounds[1][1], 0, w->lightGridSize[1]);
				ibounds[1][2] = CLAMP(ibounds[1][2], 0, w->lightGridSize[2]);

				/*
				ri->Printf(PRINT_ALL, "surf %d bounds (%f %f %f)-(%f %f %f) ibounds (%d %d %d)-(%d %d %d)\n", i,
					ci->bounds[0][0], ci->bounds[0][1], ci->bounds[0][2],
					ci->bounds[1][0], ci->bounds[1][1], ci->bounds[1][2],
					ibounds[0][0], ibounds[0][1], ibounds[0][2],
					ibounds[1][0], ibounds[1][1], ibounds[1][2]);
				*/

				goodSamples = 0;
				numSamples = 0;
				for (x = ibounds[0][0]; x <= ibounds[1][0]; x++)
				{
					for (y = ibounds[0][1]; y <= ibounds[1][1]; y++)
					{
						for (z = ibounds[0][2]; z <= ibounds[1][2]; z++)
						{
							uint8_t primaryLight = primaryLightGrid[x * 8 + y * 8 * w->lightGridBounds[0] + z * 8 * w->lightGridBounds[0] * w->lightGridBounds[2]];

							if (primaryLight == 0)
								continue;

							numSamples++;

							if (primaryLight == 1)
								goodSamples++;
						}
					}
				}

				// FIXME: magic number for determining whether object is mostly in sunlight
				if (goodSamples > numSamples * 0.75f)
				{
					//ri->Printf(PRINT_ALL, "surface %d is in sunlight\n", i);
					//surf->primaryLight = 1;
				}
			}
		}

		Z_Free(primaryLightGrid);
	}
#endif

	// load cubemaps
	if (r_cubeMapping->integer >= 1)
	{
		R_LoadCubemapEntities("misc_cubemap");
		
		if (!tr.numCubemaps)
		{
			// use deathmatch spawn points as cubemaps
			//R_LoadCubemapEntities("info_player_deathmatch");
			// UQ1: Warzone can do better!
			R_LoadCubemapWaypoints(); // NOTE: Also sets up water plane and glow postions at the same time... Can skip R_SetupMapGlowsAndWaterPlane()
		}

		if (tr.numCubemaps)
		{
			R_AssignCubemapsToWorldSurfaces();
		}
	}
	else
	{// Cubemaps disabled, need to set up water plane and glow postions anyway...
		R_SetupMapGlowsAndWaterPlane();
	}

	// create static VBOS from the world
	R_CreateWorldVBOs();
	if (r_mergeLeafSurfaces->integer)
	{
		R_MergeLeafSurfaces();
	}

	s_worldData.dataSize = (byte *)ri->Hunk_Alloc(0, h_low) - startMarker;

	// only set tr.world now that we know the entire level has loaded properly
	tr.world = &s_worldData;

	// make sure the VBO glState entries are safe
	R_BindNullVBO();
	R_BindNullIBO();

	R_LoadMapInfo();

	// Render all cubemaps
	if (r_cubeMapping->integer >= 1 && tr.numCubemaps)
	{
		R_RenderAllCubemaps();
	}

    ri->FS_FreeFile( buffer.v );
}
