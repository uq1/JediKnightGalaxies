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
// tr_init.c -- functions that are not called every frame

#include "tr_local.h"
#include "ghoul2/g2_local.h"

bool g_bDynamicGlowSupported = false;		// Not used. Put here to keep *_glimp from whining at us. --eez

glconfig_t  glConfig;
glconfigExt_t glConfigExt;
glRefConfig_t glRefConfig;
window_t	window;

float       displayAspect = 0.0f;

glstate_t	glState;

static void GfxInfo_f( void );
static void GfxMemInfo_f( void );

cvar_t *r_debugContext;
cvar_t	*r_arb_buffer_storage;

cvar_t	*r_superSampleMultiplier;
cvar_t	*r_instanceCloudReductionCulling;
cvar_t	*r_tesselation;
cvar_t	*r_tesselationLevel;
cvar_t	*r_tesselationAlpha;

cvar_t	*r_foliage;
cvar_t	*r_foliagePasses;
cvar_t	*r_foliageDistance;
cvar_t	*r_foliageDensity;
cvar_t	*r_foliageShadows;

cvar_t	*r_pebbles;
cvar_t	*r_pebblesPasses;
cvar_t	*r_pebblesDistance;

cvar_t	*r_rotatex;
cvar_t	*r_rotatey;
cvar_t	*r_rotatez;

cvar_t	*r_materialDebug;
cvar_t	*r_genericShaderDebug;

cvar_t	*r_weather;
cvar_t	*r_surfaceSprites;
cvar_t	*r_surfaceWeather;

cvar_t	*r_windSpeed;
cvar_t	*r_windAngle;
cvar_t	*r_windGust;
cvar_t	*r_windDampFactor;
cvar_t	*r_windPointForce;
cvar_t	*r_windPointX;
cvar_t	*r_windPointY;

cvar_t	*se_language;

cvar_t	*r_flareSize;
cvar_t	*r_flareFade;
cvar_t	*r_flareCoeff;

cvar_t	*r_displayRefresh;

cvar_t	*r_verbose;
cvar_t	*r_ignore;

cvar_t	*r_detailTextures;

cvar_t	*r_znear;
cvar_t	*r_zproj;
cvar_t	*r_stereoSeparation;

cvar_t	*r_skipBackEnd;

cvar_t	*r_stereo;
cvar_t	*r_anaglyphMode;

cvar_t	*r_greyscale;

cvar_t	*r_ignorehwgamma;
cvar_t	*r_measureOverdraw;

cvar_t	*r_inGameVideo;
cvar_t	*r_fastsky;
cvar_t	*r_drawSun;
cvar_t	*r_dynamiclight;

cvar_t	*r_lodbias;
cvar_t	*r_lodscale;
cvar_t	*r_autolodscalevalue;

cvar_t	*r_norefresh;
cvar_t	*r_drawentities;
cvar_t	*r_drawworld;
cvar_t	*r_speeds;
cvar_t	*r_fullbright;
cvar_t	*r_novis;
cvar_t	*r_nocull;
cvar_t	*r_entityCull;
cvar_t	*r_fovCull;
cvar_t	*r_facePlaneCull;
cvar_t	*r_showcluster;
cvar_t	*r_nocurves;

cvar_t	*r_allowExtensions;

cvar_t	*r_ext_compressed_textures;
cvar_t	*r_ext_multitexture;
cvar_t	*r_ext_compiled_vertex_array;
cvar_t	*r_ext_texture_env_add;
cvar_t	*r_ext_texture_filter_anisotropic;
cvar_t	*r_ext_preferred_tc_method;

cvar_t  *r_entitySurfaceMerge;

cvar_t  *r_occlusion;
cvar_t  *r_occlusionDebug;
cvar_t  *r_ext_draw_range_elements;
cvar_t  *r_ext_multi_draw_arrays;
cvar_t  *r_ext_texture_float;
cvar_t  *r_arb_half_float_pixel;
cvar_t  *r_ext_framebuffer_multisample;
cvar_t  *r_arb_seamless_cube_map;
cvar_t  *r_arb_vertex_type_2_10_10_10_rev;

cvar_t  *r_lazyFrustum;
cvar_t  *r_cacheVisibleSurfaces;

cvar_t  *r_mergeMultidraws;
cvar_t  *r_mergeLeafSurfaces;

cvar_t  *r_cameraExposure;

cvar_t  *r_hdr;
cvar_t  *r_floatLightmap;
cvar_t  *r_postProcess;

cvar_t  *r_toneMap;
cvar_t  *r_forceToneMap;
cvar_t  *r_forceToneMapMin;
cvar_t  *r_forceToneMapAvg;
cvar_t  *r_forceToneMapMax;

cvar_t  *r_autoExposure;
cvar_t  *r_forceAutoExposure;
cvar_t  *r_forceAutoExposureMin;
cvar_t  *r_forceAutoExposureMax;

cvar_t  *r_srgb;

cvar_t  *r_depthPrepass;
cvar_t  *r_ssao;

cvar_t  *r_normalMapping;
cvar_t  *r_normalMapQuality;
cvar_t  *r_specularMapping;
cvar_t  *r_deluxeMapping;
cvar_t  *r_parallaxMapping;
cvar_t  *r_cubeMapping;
cvar_t  *r_cubeMapSize;
cvar_t  *r_deluxeSpecular;
cvar_t  *r_specularIsMetallic;
cvar_t  *r_baseNormalX;
cvar_t  *r_baseNormalY;
cvar_t  *r_baseParallax;
cvar_t  *r_baseSpecular;
cvar_t  *r_baseGloss;
cvar_t  *r_mergeLightmaps;
cvar_t  *r_dlightMode;
cvar_t  *r_pshadowDist;
cvar_t  *r_imageUpsample;
cvar_t  *r_imageUpsampleMaxSize;
cvar_t  *r_imageUpsampleType;
cvar_t  *r_genNormalMaps;
cvar_t  *r_dlightShadows;
cvar_t  *r_forceSun;
cvar_t  *r_forceSunMapLightScale;
cvar_t  *r_forceSunLightScale;
cvar_t  *r_forceSunAmbientScale;
cvar_t  *r_sunlightMode;
cvar_t  *r_sunlightSpecular;
cvar_t  *r_drawSunRays;
cvar_t  *r_shadowBlurPasses;
cvar_t  *r_shadowFilter;
cvar_t  *r_shadowMapSize;
cvar_t  *r_shadowCascadeZNear;
cvar_t  *r_shadowCascadeZFar;
cvar_t  *r_shadowCascadeZBias;
cvar_t	*r_ignoreDstAlpha;

cvar_t	*r_ignoreGLErrors;
cvar_t	*r_logFile;

cvar_t	*r_stencilbits;
cvar_t	*r_depthbits;
cvar_t	*r_colorbits;
cvar_t	*r_texturebits;
cvar_t  *r_ext_multisample;

cvar_t	*r_drawBuffer;
cvar_t	*r_lightmap;
cvar_t	*r_vertexLight;
cvar_t	*r_uiFullScreen;
cvar_t	*r_shadows;
cvar_t	*r_flares;
cvar_t	*r_mode;
cvar_t	*r_nobind;
cvar_t	*r_singleShader;
cvar_t	*r_roundImagesDown;
cvar_t	*r_colorMipLevels;
cvar_t	*r_picmip;
cvar_t	*r_showtris;
cvar_t	*r_showsky;
cvar_t	*r_shownormals;
cvar_t	*r_finish;
cvar_t	*r_clear;
cvar_t	*r_swapInterval;
cvar_t	*r_markcount;
cvar_t	*r_textureMode;
cvar_t	*r_offsetFactor;
cvar_t	*r_offsetUnits;
cvar_t	*r_gamma;
cvar_t	*r_intensity;
cvar_t	*r_lockpvs;
cvar_t	*r_noportals;
cvar_t	*r_portalOnly;

cvar_t	*r_subdivisions;
cvar_t	*r_lodCurveError;

cvar_t	*r_fullscreen;
cvar_t  *r_noborder;
cvar_t	*r_centerWindow;

cvar_t	*r_customwidth;
cvar_t	*r_customheight;
cvar_t	*r_customPixelAspect;

cvar_t	*r_overBrightBits;
cvar_t	*r_mapOverBrightBits;

cvar_t	*r_debugSurface;
cvar_t	*r_simpleMipMaps;

cvar_t	*r_showImages;

cvar_t	*r_ambientScale;
cvar_t	*r_directedScale;
cvar_t	*r_debugLight;
cvar_t	*r_debugSort;
cvar_t	*r_printShaders;
cvar_t	*r_saveFontData;


//
// UQ1: Added...
//
cvar_t	*r_parallaxScale;
cvar_t	*r_blinnPhong;
cvar_t  *r_skynum;
cvar_t  *r_volumeLightStrength;
cvar_t	*r_disableGfxDirEnhancement;
cvar_t	*r_cubemapCullRange;
cvar_t	*r_cubemapCullFalloffMult;
cvar_t	*r_glslWater;
cvar_t	*r_waterWaveHeight;
cvar_t	*r_waterWaveDensity;
cvar_t  *r_grassLength;
cvar_t  *r_grassWaveSpeed;
cvar_t  *r_grassWaveSize;
cvar_t	*r_fog;
cvar_t	*r_multithread;
cvar_t	*r_multithread2;
cvar_t	*r_testvar;
cvar_t	*r_steepParallaxEyeX;
cvar_t	*r_steepParallaxEyeY;
cvar_t	*r_steepParallaxEyeZ;
cvar_t	*r_bloom;
cvar_t	*r_bloomPasses;
cvar_t	*r_bloomDarkenPower;
cvar_t	*r_bloomScale;
cvar_t	*r_lensflare;
cvar_t	*r_anamorphic;
cvar_t	*r_anamorphicDarkenPower;
cvar_t  *r_ssgi;
cvar_t  *r_ssgiWidth;
cvar_t  *r_ssgiSamples;
cvar_t	*r_depth;
cvar_t	*r_depthParallax;
cvar_t	*r_depthParallaxScale;
cvar_t	*r_depthParallaxMultiplier;
cvar_t	*r_depthParallaxEyeX;
cvar_t	*r_depthParallaxEyeY;
cvar_t	*r_depthParallaxEyeZ;
cvar_t	*r_depthPasses;
cvar_t	*r_depthScale;
cvar_t	*r_darkexpand;
cvar_t  *r_truehdr;
cvar_t  *r_magicdetail;
cvar_t  *r_magicdetailStrength;
cvar_t  *r_dof;
cvar_t  *r_fOff1X;
cvar_t  *r_fOff1Y;
cvar_t  *r_fOff2X;
cvar_t  *r_fOff2Y;
cvar_t  *r_fOff3X;
cvar_t  *r_fOff3Y;
cvar_t  *r_testvalue0;
cvar_t  *r_testvalue1;
cvar_t  *r_testvalue2;
cvar_t  *r_testvalue3;
cvar_t  *r_esharpening;
cvar_t  *r_esharpening2;
cvar_t  *r_multipost;
cvar_t  *r_textureClean;
cvar_t  *r_textureCleanSigma;
cvar_t  *r_textureCleanBSigma;
cvar_t  *r_textureCleanMSize;
cvar_t  *r_imageBasedLighting;
cvar_t  *r_sss;
cvar_t  *r_sssMinRange;
cvar_t  *r_sssMaxRange;
cvar_t  *r_rbm;
cvar_t  *r_rbmStrength;
cvar_t  *r_hbao;
cvar_t  *r_colorCorrection;
cvar_t  *r_deferredLighting;
cvar_t	*r_steepParallax;
cvar_t  *r_trueAnaglyph;
cvar_t  *r_trueAnaglyphSeparation;
cvar_t  *r_trueAnaglyphRed;
cvar_t  *r_trueAnaglyphGreen;
cvar_t  *r_trueAnaglyphBlue;
cvar_t  *r_trueAnaglyphPower;
cvar_t  *r_trueAnaglyphMinDistance;
cvar_t  *r_trueAnaglyphMaxDistance;
cvar_t  *r_trueAnaglyphParallax;
cvar_t  *r_vibrancy;
cvar_t  *r_fxaa;
cvar_t  *r_underwater;
cvar_t  *r_distanceBlur;
cvar_t  *r_fogPost;
cvar_t  *r_dayNightCycleSpeed;
cvar_t  *r_testshader;
cvar_t  *r_testshaderValue1;
cvar_t  *r_testshaderValue2;
cvar_t  *r_testshaderValue3;
cvar_t  *r_testshaderValue4;
cvar_t  *r_testshaderValue5;
cvar_t  *r_testshaderValue6;
cvar_t  *r_testshaderValue7;
cvar_t  *r_testshaderValue8;
cvar_t  *r_testshaderValue9;

//
// UQ1: End Added...
//


#ifdef _DEBUG
cvar_t	*r_noPrecacheGLA;
#endif

cvar_t	*r_noServerGhoul2;
cvar_t	*r_Ghoul2AnimSmooth=0;
cvar_t	*r_Ghoul2UnSqashAfterSmooth=0;
//cvar_t	*r_Ghoul2UnSqash;
//cvar_t	*r_Ghoul2TimeBase=0; from single player
//cvar_t	*r_Ghoul2NoLerp;
//cvar_t	*r_Ghoul2NoBlend;
//cvar_t	*r_Ghoul2BlendMultiplier=0;

cvar_t	*broadsword=0;
cvar_t	*broadsword_kickbones=0;
cvar_t	*broadsword_kickorigin=0;
cvar_t	*broadsword_playflop=0;
cvar_t	*broadsword_dontstopanim=0;
cvar_t	*broadsword_waitforshot=0;
cvar_t	*broadsword_smallbbox=0;
cvar_t	*broadsword_extra1=0;
cvar_t	*broadsword_extra2=0;

cvar_t	*broadsword_effcorr=0;
cvar_t	*broadsword_ragtobase=0;
cvar_t	*broadsword_dircap=0;

cvar_t	*r_marksOnTriangleMeshes;

cvar_t	*r_aviMotionJpegQuality;
cvar_t	*r_screenshotJpegQuality;

cvar_t	*r_maxpolys;
int		max_polys;
cvar_t	*r_maxpolyverts;
int		max_polyverts;

cvar_t	*r_dynamicGlow;
cvar_t	*r_dynamicGlowPasses;
cvar_t	*r_dynamicGlowDelta;
cvar_t	*r_dynamicGlowIntensity;
cvar_t	*r_dynamicGlowSoft;

extern void	RB_SetGL2D (void);
void R_Splash()
{
	image_t *pImage = R_FindImageFile("menu/splash", IMGTYPE_COLORALPHA, IMGFLAG_NONE);
	extern void	RB_SetGL2D(void);
	RB_SetGL2D();
	if (pImage)
	{//invalid paths?
		GL_Bind(pImage);
	}
	GL_State(GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO);

	const int width = 640;
	const int height = 480;
	const float x1 = 320 - width / 2;
	const float x2 = 320 + width / 2;
	const float y1 = 240 - height / 2;
	const float y2 = 240 + height / 2;


	qglBegin(GL_TRIANGLE_STRIP);
	qglTexCoord2f(0, 0);
	qglVertex2f(x1, y1);
	qglTexCoord2f(1, 0);
	qglVertex2f(x2, y1);
	qglTexCoord2f(0, 1);
	qglVertex2f(x1, y2);
	qglTexCoord2f(1, 1);
	qglVertex2f(x2, y2);
	qglEnd();

	ri->WIN_Present(&window);
}


/*
** GLW_CheckForExtension

Cannot use strstr directly to differentiate between (for eg) reg_combiners and reg_combiners2
*/
bool GL_CheckForExtension(const char *ext)
{
	const char *ptr = Q_stristr(glConfigExt.originalExtensionString, ext);
	if (ptr == NULL)
		return false;
	ptr += strlen(ext);
	return ((*ptr == ' ') || (*ptr == '\0'));  // verify it's complete string.
}


void GLW_InitTextureCompression(void)
{
	bool newer_tc, old_tc;

	// Check for available tc methods.
	newer_tc = GL_CheckForExtension("ARB_texture_compression") && GL_CheckForExtension("EXT_texture_compression_s3tc");
	old_tc = GL_CheckForExtension("GL_S3_s3tc");

	if (old_tc)
	{
		Com_Printf("...GL_S3_s3tc available\n");
	}

	if (newer_tc)
	{
		Com_Printf("...GL_EXT_texture_compression_s3tc available\n");
	}

	if (!r_ext_compressed_textures->value)
	{
		// Compressed textures are off
		glConfig.textureCompression = TC_NONE;
		Com_Printf("...ignoring texture compression\n");
	}
	else if (!old_tc && !newer_tc)
	{
		// Requesting texture compression, but no method found
		glConfig.textureCompression = TC_NONE;
		Com_Printf("...no supported texture compression method found\n");
		Com_Printf(".....ignoring texture compression\n");
	}
	else
	{
		// some form of supported texture compression is avaiable, so see if the user has a preference
		if (r_ext_preferred_tc_method->integer == TC_NONE)
		{
			// No preference, so pick the best
			if (newer_tc)
			{
				Com_Printf("...no tc preference specified\n");
				Com_Printf(".....using GL_EXT_texture_compression_s3tc\n");
				glConfig.textureCompression = TC_S3TC_DXT;
			}
			else
			{
				Com_Printf("...no tc preference specified\n");
				Com_Printf(".....using GL_S3_s3tc\n");
				glConfig.textureCompression = TC_S3TC;
			}
		}
		else
		{
			// User has specified a preference, now see if this request can be honored
			if (old_tc && newer_tc)
			{
				// both are avaiable, so we can use the desired tc method
				if (r_ext_preferred_tc_method->integer == TC_S3TC)
				{
					Com_Printf("...using preferred tc method, GL_S3_s3tc\n");
					glConfig.textureCompression = TC_S3TC;
				}
				else
				{
					Com_Printf("...using preferred tc method, GL_EXT_texture_compression_s3tc\n");
					glConfig.textureCompression = TC_S3TC_DXT;
				}
			}
			else
			{
				// Both methods are not available, so this gets trickier
				if (r_ext_preferred_tc_method->integer == TC_S3TC)
				{
					// Preferring to user older compression
					if (old_tc)
					{
						Com_Printf("...using GL_S3_s3tc\n");
						glConfig.textureCompression = TC_S3TC;
					}
					else
					{
						// Drat, preference can't be honored
						Com_Printf("...preferred tc method, GL_S3_s3tc not available\n");
						Com_Printf(".....falling back to GL_EXT_texture_compression_s3tc\n");
						glConfig.textureCompression = TC_S3TC_DXT;
					}
				}
				else
				{
					// Preferring to user newer compression
					if (newer_tc)
					{
						Com_Printf("...using GL_EXT_texture_compression_s3tc\n");
						glConfig.textureCompression = TC_S3TC_DXT;
					}
					else
					{
						// Drat, preference can't be honored
						Com_Printf("...preferred tc method, GL_EXT_texture_compression_s3tc not available\n");
						Com_Printf(".....falling back to GL_S3_s3tc\n");
						glConfig.textureCompression = TC_S3TC;
					}
				}
			}
		}
	}
}

// Truncates the GL extensions string by only allowing up to 'maxExtensions' extensions in the string.
static const char *TruncateGLExtensionsString(const char *extensionsString, int maxExtensions)
{
	const char *p = extensionsString;
	const char *q;
	int numExtensions = 0;
	size_t extensionsLen = strlen(extensionsString);

	char *truncatedExtensions;

	while ((q = strchr(p, ' ')) != NULL && numExtensions <= maxExtensions)
	{
		p = q + 1;
		numExtensions++;
	}

	if (q != NULL)
	{
		// We still have more extensions. We'll call this the end

		extensionsLen = p - extensionsString - 1;
	}

	truncatedExtensions = (char *)Z_Malloc(extensionsLen + 1, TAG_GENERAL);
	Q_strncpyz(truncatedExtensions, extensionsString, extensionsLen + 1);

	return truncatedExtensions;
}

static const char *GetGLExtensionsString()
{
	GLint numExtensions;
	glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions);
	size_t extensionStringLen = 0;

	for (int i = 0; i < numExtensions; i++)
	{
		extensionStringLen += strlen((const char *)qglGetStringi(GL_EXTENSIONS, i)) + 1;
	}

	char *extensionString = (char *)Z_Malloc(extensionStringLen + 1, TAG_GENERAL);
	char *p = extensionString;
	for (int i = 0; i < numExtensions; i++)
	{
		const char *extension = (const char *)qglGetStringi(GL_EXTENSIONS, i);
		while (*extension != '\0')
			*p++ = *extension++;

		*p++ = ' ';
	}

	*p = '\0';
	assert((p - extensionString) == extensionStringLen);

	return extensionString;
}


/*
** InitOpenGL
**
** This function is responsible for initializing a valid OpenGL subsystem.  This
** is done by calling GLimp_Init (which gives us a working OGL subsystem) then
** setting variables, checking GL constants, and reporting the gfx system config
** to the user.
*/
static void InitOpenGL( void )
{
	//
	// initialize OS specific portions of the renderer
	//
	// GLimp_Init directly or indirectly references the following cvars:
	//		- r_fullscreen
	//		- r_mode
	//		- r_(color|depth|stencil)bits
	//		- r_ignorehwgamma
	//		- r_gamma
	//

	if (glConfig.vidWidth == 0)
	{
		windowDesc_t windowDesc = {};
		memset(&glConfig, 0, sizeof(glConfig));

		windowDesc.api = GRAPHICS_API_OPENGL;
		windowDesc.gl.majorVersion = 3;
		windowDesc.gl.minorVersion = 2;
		windowDesc.gl.profile = GLPROFILE_CORE;
		if (r_debugContext->integer)
			windowDesc.gl.contextFlags = GLCONTEXT_DEBUG;

		window = ri->WIN_Init(&windowDesc, &glConfig);

		GLimp_InitCoreFunctions();

		Com_Printf("GL_RENDERER: %s\n", (char *)qglGetString(GL_RENDERER));

		// get our config strings
		glConfig.vendor_string = (const char *)qglGetString(GL_VENDOR);
		glConfig.renderer_string = (const char *)qglGetString(GL_RENDERER);
		glConfig.version_string = (const char *)qglGetString(GL_VERSION);
		glConfig.extensions_string = GetGLExtensionsString();

		glConfigExt.originalExtensionString = glConfig.extensions_string;
		glConfig.extensions_string = TruncateGLExtensionsString(glConfigExt.originalExtensionString, 128);

		// OpenGL driver constants
		qglGetIntegerv(GL_MAX_TEXTURE_SIZE, &glConfig.maxTextureSize);

		// Determine GPU IHV
		if (Q_stristr(glConfig.vendor_string, "ATI Technologies Inc."))
		{
			glRefConfig.hardwareVendor = IHV_AMD;
		}
		else if (Q_stristr(glConfig.vendor_string, "NVIDIA"))
		{
			glRefConfig.hardwareVendor = IHV_NVIDIA;
		}
		else if (Q_stristr(glConfig.vendor_string, "INTEL"))
		{
			glRefConfig.hardwareVendor = IHV_INTEL;
		}
		else
		{
			glRefConfig.hardwareVendor = IHV_UNKNOWN;
		}

		// stubbed or broken drivers may have reported 0...
		glConfig.maxTextureSize = Q_max(0, glConfig.maxTextureSize);

		// initialize extensions
		GLimp_InitExtensions();

		// Create the default VAO
		GLuint vao;
		qglGenVertexArrays(1, &vao);
		qglBindVertexArray(vao);
		tr.globalVao = vao;

		// set default state
		GL_SetDefaultState();

		R_Splash();	//get something on screen asap
	}
	else
	{
		// set default state
		GL_SetDefaultState();
	}
}

/*
==================
GL_CheckErrors
==================
*/
void GL_CheckErrs( char *file, int line ) {
	int		err;
	char	s[64];

	err = qglGetError();
	if ( err == GL_NO_ERROR ) {
		return;
	}
	if ( r_ignoreGLErrors->integer ) {
		return;
	}
	switch( err ) {
		case GL_INVALID_ENUM:
			strcpy( s, "GL_INVALID_ENUM" );
			break;
		case GL_INVALID_VALUE:
			strcpy( s, "GL_INVALID_VALUE" );
			break;
		case GL_INVALID_OPERATION:
			strcpy( s, "GL_INVALID_OPERATION" );
			break;
		case GL_STACK_OVERFLOW:
			strcpy( s, "GL_STACK_OVERFLOW" );
			break;
		case GL_STACK_UNDERFLOW:
			strcpy( s, "GL_STACK_UNDERFLOW" );
			break;
		case GL_OUT_OF_MEMORY:
			strcpy( s, "GL_OUT_OF_MEMORY" );
			break;
		default:
			Com_sprintf( s, sizeof(s), "%i", err);
			break;
	}

	ri->Error( ERR_FATAL, "GL_CheckErrors: %s in %s at line %d", s , file, line);
}


/*
** R_GetModeInfo
*/
typedef struct vidmode_s
{
	const char *description;
	int width, height;
	float pixelAspect;		// pixel width / height
} vidmode_t;

vidmode_t r_vidModes[] =
{
	{ "Mode  0: 320x240",		320,	240,	1 },
	{ "Mode  1: 400x300",		400,	300,	1 },
	{ "Mode  2: 512x384",		512,	384,	1 },
	{ "Mode  3: 640x480",		640,	480,	1 },
	{ "Mode  4: 800x600",		800,	600,	1 },
	{ "Mode  5: 960x720",		960,	720,	1 },
	{ "Mode  6: 1024x768",		1024,	768,	1 },
	{ "Mode  7: 1152x864",		1152,	864,	1 },
	{ "Mode  8: 1280x1024",		1280,	1024,	1 },
	{ "Mode  9: 1600x1200",		1600,	1200,	1 },
	{ "Mode 10: 2048x1536",		2048,	1536,	1 },
	{ "Mode 11: 856x480 (wide)",856,	480,	1 }
};
static int	s_numVidModes = ARRAY_LEN( r_vidModes );

qboolean R_GetModeInfo( int *width, int *height, int mode ) {
	vidmode_t	*vm;
	float			pixelAspect;

	if ( mode < -1 ) {
		return qfalse;
	}
	if ( mode >= s_numVidModes ) {
		return qfalse;
	}

	if ( mode == -1 ) {
		*width = r_customwidth->integer;
		*height = r_customheight->integer;
		pixelAspect = r_customPixelAspect->value;
	} else {
		vm = &r_vidModes[mode];

		*width  = vm->width;
		*height = vm->height;
		pixelAspect = vm->pixelAspect;
	}

	return qtrue;
}

/*
** R_ModeList_f
*/
static void R_ModeList_f( void )
{
	int i;

	ri->Printf( PRINT_ALL, "\n" );
	for ( i = 0; i < s_numVidModes; i++ )
	{
		ri->Printf( PRINT_ALL, "%s\n", r_vidModes[i].description );
	}
	ri->Printf( PRINT_ALL, "\n" );
}


/*
==============================================================================

						SCREEN SHOTS

NOTE TTimo
some thoughts about the screenshots system:
screenshots get written in fs_homepath + fs_gamedir
vanilla q3 .. baseq3/screenshots/ *.tga
team arena .. missionpack/screenshots/ *.tga

two commands: "screenshot" and "screenshotJPEG"
we use statics to store a count and start writing the first screenshot/screenshot????.tga (.jpg) available
(with FS_FileExists / FS_FOpenFileWrite calls)
FIXME: the statics don't get a reinit between fs_game changes

==============================================================================
*/

/*
==================
RB_ReadPixels

Reads an image but takes care of alignment issues for reading RGB images.

Reads a minimum offset for where the RGB data starts in the image from
integer stored at pointer offset. When the function has returned the actual
offset was written back to address offset. This address will always have an
alignment of packAlign to ensure efficient copying.

Stores the length of padding after a line of pixels to address padlen

Return value must be freed with ri->Hunk_FreeTempMemory()
==================
*/

byte *RB_ReadPixels(int x, int y, int width, int height, size_t *offset, int *padlen)
{
	byte *buffer, *bufstart;
	int padwidth, linelen;
	GLint packAlign;

	qglGetIntegerv(GL_PACK_ALIGNMENT, &packAlign);

	linelen = width * 3;
	padwidth = PAD(linelen, packAlign);

	// Allocate a few more bytes so that we can choose an alignment we like
	buffer = (byte *)ri->Hunk_AllocateTempMemory(padwidth * height + *offset + packAlign - 1);

	bufstart = (byte*)(PADP((intptr_t) buffer + *offset, packAlign));
	qglReadPixels(x, y, width, height, GL_RGB, GL_UNSIGNED_BYTE, bufstart);

	*offset = bufstart - buffer;
	*padlen = padwidth - linelen;

	return buffer;
}

/*
==================
RB_TakeScreenshot
==================
*/
void RB_TakeScreenshot(int x, int y, int width, int height, char *fileName)
{
	byte *allbuf, *buffer;
	byte *srcptr, *destptr;
	byte *endline, *endmem;
	byte temp;

	int linelen, padlen;
	size_t offset = 18, memcount;

	FBO_Bind(tr.renderFbo);

	allbuf = RB_ReadPixels(x, y, width, height, &offset, &padlen);
	buffer = allbuf + offset - 18;

	Com_Memset (buffer, 0, 18);
	buffer[2] = 2;		// uncompressed type
	buffer[12] = width & 255;
	buffer[13] = width >> 8;
	buffer[14] = height & 255;
	buffer[15] = height >> 8;
	buffer[16] = 24;	// pixel size

	// swap rgb to bgr and remove padding from line endings
	linelen = width * 3;

	srcptr = destptr = allbuf + offset;
	endmem = srcptr + (linelen + padlen) * height;

	while(srcptr < endmem)
	{
		endline = srcptr + linelen;

		while(srcptr < endline)
		{
			temp = srcptr[0];
			*destptr++ = srcptr[2];
			*destptr++ = srcptr[1];
			*destptr++ = temp;

			srcptr += 3;
		}

		// Skip the pad
		srcptr += padlen;
	}

	memcount = linelen * height;

	// gamma correct
	if(glConfig.deviceSupportsGamma)
		R_GammaCorrect(allbuf + offset, memcount);

	ri->FS_WriteFile(fileName, buffer, memcount + 18);

	ri->Hunk_FreeTempMemory(allbuf);
}

/*
==================
R_TakeScreenshotPNG
==================
*/
void RB_TakeScreenshotPNG( int x, int y, int width, int height, char *fileName ) {
	byte *buffer;
	size_t offset = 0, memcount;
	int padlen;

	FBO_Bind(tr.renderFbo);

	buffer = RB_ReadPixels( x, y, width, height, &offset, &padlen );
	memcount = (width * 3 + padlen) * height;

	// gamma correct
	if(glConfig.deviceSupportsGamma)
		R_GammaCorrect(buffer + offset, memcount);

	RE_SavePNG( fileName, buffer, width, height, 3 );
	ri->Hunk_FreeTempMemory( buffer );
}

/*
==================
RB_TakeScreenshotJPEG
==================
*/

void RB_TakeScreenshotJPEG(int x, int y, int width, int height, char *fileName)
{
	byte *buffer;
	size_t offset = 0, memcount;
	int padlen;

	FBO_Bind(tr.renderFbo);

	buffer = RB_ReadPixels(x, y, width, height, &offset, &padlen);
	memcount = (width * 3 + padlen) * height;

	// gamma correct
	if(glConfig.deviceSupportsGamma)
		R_GammaCorrect(buffer + offset, memcount);

	RE_SaveJPG(fileName, r_screenshotJpegQuality->integer, width, height, buffer + offset, padlen);
	ri->Hunk_FreeTempMemory(buffer);
}

/*
==================
RB_TakeScreenshotCmd
==================
*/
const void *RB_TakeScreenshotCmd( const void *data ) {
	const screenshotCommand_t	*cmd;

	cmd = (const screenshotCommand_t *)data;

	// finish any 2D drawing if needed
	if(tess.numIndexes)
		RB_EndSurface();

	FBO_Bind(tr.renderFbo);

	switch( cmd->format ) {
		case SSF_JPEG:
			RB_TakeScreenshotJPEG( cmd->x, cmd->y, cmd->width, cmd->height, cmd->fileName );
			break;
		case SSF_TGA:
			RB_TakeScreenshot( cmd->x, cmd->y, cmd->width, cmd->height, cmd->fileName );
			break;
		case SSF_PNG:
			RB_TakeScreenshotPNG( cmd->x, cmd->y, cmd->width, cmd->height, cmd->fileName );
			break;
	}

	return (const void *)(cmd + 1);
}

/*
==================
R_TakeScreenshot
==================
*/
void R_TakeScreenshot( int x, int y, int width, int height, char *name, screenshotFormat_t format ) {
	static char	fileName[MAX_OSPATH]; // bad things if two screenshots per frame?
	screenshotCommand_t	*cmd;

	cmd = (screenshotCommand_t *)R_GetCommandBuffer( sizeof( *cmd ) );
	if ( !cmd ) {
		return;
	}
	cmd->commandId = RC_SCREENSHOT;

	cmd->x = x;
	cmd->y = y;
	cmd->width = width;
	cmd->height = height;
	Q_strncpyz( fileName, name, sizeof(fileName) );
	cmd->fileName = fileName;
	cmd->format = format;
}

/*
==================
R_ScreenshotFilename
==================
*/
void R_ScreenshotFilename( char *buf, int bufSize, const char *ext ) {
	time_t rawtime;
	char timeStr[32] = {0}; // should really only reach ~19 chars

	time( &rawtime );
	strftime( timeStr, sizeof( timeStr ), "%Y-%m-%d_%H-%M-%S", localtime( &rawtime ) ); // or gmtime

	Com_sprintf( buf, bufSize, "screenshots/shot%s%s", timeStr, ext );
}

/*
====================
R_LevelShot

levelshots are specialized 256*256 thumbnails for
the menu system, sampled down from full screen distorted images
====================
*/
#define LEVELSHOTSIZE 256
static void R_LevelShot( void ) {
	char		checkname[MAX_OSPATH];
	byte		*buffer;
	byte		*source, *allsource;
	byte		*src, *dst;
	size_t		offset = 0;
	int			padlen;
	int			x, y;
	int			r, g, b;
	float		xScale, yScale;
	int			xx, yy;

	Com_sprintf( checkname, sizeof(checkname), "levelshots/%s.tga", tr.world->baseName );

	allsource = RB_ReadPixels(0, 0, glConfig.vidWidth * r_superSampleMultiplier->value, glConfig.vidHeight * r_superSampleMultiplier->value, &offset, &padlen);
	source = allsource + offset;

	buffer = (byte *)ri->Hunk_AllocateTempMemory(LEVELSHOTSIZE * LEVELSHOTSIZE*3 + 18);
	Com_Memset (buffer, 0, 18);
	buffer[2] = 2;		// uncompressed type
	buffer[12] = LEVELSHOTSIZE & 255;
	buffer[13] = LEVELSHOTSIZE >> 8;
	buffer[14] = LEVELSHOTSIZE & 255;
	buffer[15] = LEVELSHOTSIZE >> 8;
	buffer[16] = 24;	// pixel size

	int vidWidth = glConfig.vidWidth * r_superSampleMultiplier->value;

	// resample from source
	xScale = (glConfig.vidWidth * r_superSampleMultiplier->value) / (4.0*LEVELSHOTSIZE);
	yScale = (glConfig.vidHeight * r_superSampleMultiplier->value) / (3.0*LEVELSHOTSIZE);

	for ( y = 0 ; y < LEVELSHOTSIZE ; y++ ) {
		for ( x = 0 ; x < LEVELSHOTSIZE ; x++ ) {
			r = g = b = 0;
			for ( yy = 0 ; yy < 3 ; yy++ ) {
				for ( xx = 0 ; xx < 4 ; xx++ ) {
					src = source + 3 * ( vidWidth * (int)( (y*3+yy)*yScale ) + (int)( (x*4+xx)*xScale ) );
					r += src[0];
					g += src[1];
					b += src[2];
				}
			}
			dst = buffer + 18 + 3 * ( y * LEVELSHOTSIZE + x );
			dst[0] = b / 12;
			dst[1] = g / 12;
			dst[2] = r / 12;
		}
	}

	// gamma correct
	if ( ( tr.overbrightBits > 0 ) && glConfig.deviceSupportsGamma ) {
		R_GammaCorrect( buffer + 18, LEVELSHOTSIZE * LEVELSHOTSIZE * 3 );
	}

	ri->FS_WriteFile( checkname, buffer, LEVELSHOTSIZE * LEVELSHOTSIZE*3 + 18 );

	ri->Hunk_FreeTempMemory( buffer );
	ri->Hunk_FreeTempMemory( allsource );

	ri->Printf( PRINT_ALL, "Wrote %s\n", checkname );
}

/*
==================
R_ScreenShotTGA_f

screenshot
screenshot [silent]
screenshot [levelshot]
screenshot [filename]

Doesn't print the pacifier message if there is a second arg
==================
*/
void R_ScreenShotTGA_f (void) {
	char checkname[MAX_OSPATH] = {0};
	qboolean silent = qfalse;

	if ( !strcmp( ri->Cmd_Argv(1), "levelshot" ) ) {
		R_LevelShot();
		return;
	}

	if ( !strcmp( ri->Cmd_Argv(1), "silent" ) )
		silent = qtrue;

	if ( ri->Cmd_Argc() == 2 && !silent ) {
		// explicit filename
		Com_sprintf( checkname, sizeof( checkname ), "screenshots/%s.tga", ri->Cmd_Argv( 1 ) );
	}
	else {
		// timestamp the file
		R_ScreenshotFilename( checkname, sizeof( checkname ), ".tga" );

		if ( ri->FS_FileExists( checkname ) ) {
			Com_Printf( "ScreenShot: Couldn't create a file\n");
			return;
 		}
	}

	R_TakeScreenshot( 0, 0, glConfig.vidWidth * r_superSampleMultiplier->value, glConfig.vidHeight * r_superSampleMultiplier->value, checkname, SSF_TGA );

	if ( !silent )
		ri->Printf (PRINT_ALL, "Wrote %s\n", checkname);
}

void R_ScreenShotPNG_f (void) {
	char checkname[MAX_OSPATH] = {0};
	qboolean silent = qfalse;

	if ( !strcmp( ri->Cmd_Argv(1), "levelshot" ) ) {
		R_LevelShot();
		return;
	}

	if ( !strcmp( ri->Cmd_Argv(1), "silent" ) )
		silent = qtrue;

	if ( ri->Cmd_Argc() == 2 && !silent ) {
		// explicit filename
		Com_sprintf( checkname, sizeof( checkname ), "screenshots/%s.png", ri->Cmd_Argv( 1 ) );
	}
	else {
		// timestamp the file
		R_ScreenshotFilename( checkname, sizeof( checkname ), ".png" );

		if ( ri->FS_FileExists( checkname ) ) {
			Com_Printf( "ScreenShot: Couldn't create a file\n");
			return;
 		}
	}

	R_TakeScreenshot( 0, 0, glConfig.vidWidth * r_superSampleMultiplier->value, glConfig.vidHeight * r_superSampleMultiplier->value, checkname, SSF_PNG );

	if ( !silent )
		ri->Printf (PRINT_ALL, "Wrote %s\n", checkname);
}

void R_ScreenShotJPEG_f (void) {
	char checkname[MAX_OSPATH] = {0};
	qboolean silent = qfalse;

	if ( !strcmp( ri->Cmd_Argv(1), "levelshot" ) ) {
		R_LevelShot();
		return;
	}

	if ( !strcmp( ri->Cmd_Argv(1), "silent" ) )
		silent = qtrue;

	if ( ri->Cmd_Argc() == 2 && !silent ) {
		// explicit filename
		Com_sprintf( checkname, sizeof( checkname ), "screenshots/%s.jpg", ri->Cmd_Argv( 1 ) );
	}
	else {
		// timestamp the file
		R_ScreenshotFilename( checkname, sizeof( checkname ), ".jpg" );

		if ( ri->FS_FileExists( checkname ) ) {
			Com_Printf( "ScreenShot: Couldn't create a file\n");
			return;
 		}
	}

	R_TakeScreenshot( 0, 0, glConfig.vidWidth * r_superSampleMultiplier->value, glConfig.vidHeight * r_superSampleMultiplier->value, checkname, SSF_JPEG );

	if ( !silent )
		ri->Printf (PRINT_ALL, "Wrote %s\n", checkname);
}

//============================================================================

/*
==================
RB_TakeVideoFrameCmd
==================
*/
const void *RB_TakeVideoFrameCmd( const void *data )
{
	const videoFrameCommand_t	*cmd;
	byte				*cBuf;
	size_t				memcount, linelen;
	int				padwidth, avipadwidth, padlen, avipadlen;
	GLint packAlign;

	// finish any 2D drawing if needed
	if(tess.numIndexes)
		RB_EndSurface();

	cmd = (const videoFrameCommand_t *)data;

	qglGetIntegerv(GL_PACK_ALIGNMENT, &packAlign);

	linelen = cmd->width * 3;

	// Alignment stuff for glReadPixels
	padwidth = PAD(linelen, packAlign);
	padlen = padwidth - linelen;
	// AVI line padding
	avipadwidth = PAD(linelen, AVI_LINE_PADDING);
	avipadlen = avipadwidth - linelen;

	cBuf = (byte*)(PADP(cmd->captureBuffer, packAlign));

	qglReadPixels(0, 0, cmd->width, cmd->height, GL_RGB,
		GL_UNSIGNED_BYTE, cBuf);

	memcount = padwidth * cmd->height;

	// gamma correct
	if(glConfig.deviceSupportsGamma)
		R_GammaCorrect(cBuf, memcount);

	if(cmd->motionJpeg)
	{
		memcount = RE_SaveJPGToBuffer(cmd->encodeBuffer, linelen * cmd->height,
			r_aviMotionJpegQuality->integer,
			cmd->width, cmd->height, cBuf, padlen);
		ri->CL_WriteAVIVideoFrame(cmd->encodeBuffer, memcount);
	}
	else
	{
		byte *lineend, *memend;
		byte *srcptr, *destptr;

		srcptr = cBuf;
		destptr = cmd->encodeBuffer;
		memend = srcptr + memcount;

		// swap R and B and remove line paddings
		while(srcptr < memend)
		{
			lineend = srcptr + linelen;
			while(srcptr < lineend)
			{
				*destptr++ = srcptr[2];
				*destptr++ = srcptr[1];
				*destptr++ = srcptr[0];
				srcptr += 3;
			}

			Com_Memset(destptr, '\0', avipadlen);
			destptr += avipadlen;

			srcptr += padlen;
		}

		ri->CL_WriteAVIVideoFrame(cmd->encodeBuffer, avipadwidth * cmd->height);
	}

	return (const void *)(cmd + 1);
}

//============================================================================

/*
** GL_SetDefaultState
*/
void GL_SetDefaultState( void )
{
	qglClearDepth(1.0f);

	qglCullFace(GL_FRONT);

	// initialize downstream texture unit if we're running
	// in a multitexture environment
	GL_SelectTexture(1);
	GL_TextureMode(r_textureMode->string);
	GL_SelectTexture(0);

	GL_TextureMode(r_textureMode->string);

	//qglShadeModel( GL_SMOOTH );
	qglDepthFunc(GL_LEQUAL);

	Com_Memset(&glState, 0, sizeof(glState));

	//
	// make sure our GL state vector is set correctly
	//
	glState.glStateBits = GLS_DEPTHTEST_DISABLE | GLS_DEPTHMASK_TRUE;
	glState.maxDepth = 1.0f;
	qglDepthRange(0.0f, 1.0f);

	qglUseProgram(0);

	qglBindBuffer(GL_ARRAY_BUFFER, 0);
	qglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	qglPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	qglDepthMask(GL_TRUE);
	qglDisable(GL_DEPTH_TEST);
	qglEnable(GL_SCISSOR_TEST);
	qglDisable(GL_CULL_FACE);
	qglDisable(GL_BLEND);

	qglEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
}

/*
================
R_PrintLongString

Workaround for ri->Printf's 1024 characters buffer limit.
================
*/
void R_PrintLongString(const char *string) {
	char buffer[1024];
	const char *p;
	int size = strlen(string);

	p = string;
	while(size > 0)
	{
		Q_strncpyz(buffer, p, sizeof (buffer) );
		ri->Printf( PRINT_ALL, "%s", buffer );
		p += 1023;
		size -= 1023;
	}
}

/*
================
GfxInfo_f
================
*/
static void GfxInfo_f( void )
{
	const char *enablestrings[] =
	{
		"disabled",
		"enabled"
	};
	const char *fsstrings[] =
	{
		"windowed",
		"fullscreen"
	};

	ri->Printf( PRINT_ALL, "\nGL_VENDOR: %s\n", glConfig.vendor_string );
	ri->Printf( PRINT_ALL, "GL_RENDERER: %s\n", glConfig.renderer_string );
	ri->Printf( PRINT_ALL, "GL_VERSION: %s\n", glConfig.version_string );
	ri->Printf( PRINT_ALL, "GL_EXTENSIONS: " );
	R_PrintLongString( glConfigExt.originalExtensionString );
	ri->Printf( PRINT_ALL, "\n" );
	ri->Printf( PRINT_ALL, "GL_MAX_TEXTURE_SIZE: %d\n", glConfig.maxTextureSize );
	//ri->Printf( PRINT_ALL, "GL_MAX_TEXTURE_UNITS_ARB: %d\n", glConfig.numTextureUnits );
	ri->Printf( PRINT_ALL, "\nPIXELFORMAT: color(%d-bits) Z(%d-bit) stencil(%d-bits)\n", glConfig.colorBits, glConfig.depthBits, glConfig.stencilBits );
	ri->Printf( PRINT_ALL, "MODE: %d, %d x %d %s hz:", r_mode->integer, glConfig.vidWidth * r_superSampleMultiplier->value, glConfig.vidHeight * r_superSampleMultiplier->value, fsstrings[r_fullscreen->integer == 1] );
	if ( glConfig.displayFrequency )
	{
		ri->Printf( PRINT_ALL, "%d\n", glConfig.displayFrequency );
	}
	else
	{
		ri->Printf( PRINT_ALL, "N/A\n" );
	}
	if ( glConfig.deviceSupportsGamma )
	{
		ri->Printf( PRINT_ALL, "GAMMA: hardware w/ %d overbright bits\n", tr.overbrightBits );
	}
	else
	{
		ri->Printf( PRINT_ALL, "GAMMA: software w/ %d overbright bits\n", tr.overbrightBits );
	}

	ri->Printf( PRINT_ALL, "texturemode: %s\n", r_textureMode->string );
	ri->Printf( PRINT_ALL, "picmip: %d\n", r_picmip->integer );
	ri->Printf( PRINT_ALL, "texture bits: %d\n", r_texturebits->integer );
	ri->Printf( PRINT_ALL, "multitexture: %s\n", enablestrings[qglActiveTexture != 0] );
	ri->Printf( PRINT_ALL, "compiled vertex arrays: %s\n", enablestrings[qglLockArraysEXT != 0 ] );
	ri->Printf( PRINT_ALL, "texenv add: %s\n", enablestrings[glConfig.textureEnvAddAvailable != 0] );
	ri->Printf( PRINT_ALL, "compressed textures: %s\n", enablestrings[glConfig.textureCompression!=TC_NONE] );
	if ( r_vertexLight->integer )
	{
		ri->Printf( PRINT_ALL, "HACK: using vertex lightmap approximation\n" );
	}
	if ( r_displayRefresh ->integer ) {
		ri->Printf( PRINT_ALL, "Display refresh set to %d\n", r_displayRefresh->integer );
	}
	if ( r_finish->integer ) {
		ri->Printf( PRINT_ALL, "Forcing glFinish\n" );
	}

	ri->Printf( PRINT_ALL, "Dynamic Glow: %s\n", enablestrings[r_dynamicGlow->integer != 0] );
}

/*
================
GfxMemInfo_f
================
*/
void GfxMemInfo_f( void )
{
	switch (glRefConfig.memInfo)
	{
		case MI_NONE:
		{
			ri->Printf(PRINT_ALL, "No extension found for GPU memory info.\n");
		}
		break;
		case MI_NVX:
		{
			int value;

			qglGetIntegerv(GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX, &value);
			ri->Printf(PRINT_ALL, "GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX: %ikb\n", value);

			qglGetIntegerv(GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX, &value);
			ri->Printf(PRINT_ALL, "GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX: %ikb\n", value);

			qglGetIntegerv(GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX, &value);
			ri->Printf(PRINT_ALL, "GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX: %ikb\n", value);

			qglGetIntegerv(GL_GPU_MEMORY_INFO_EVICTION_COUNT_NVX, &value);
			ri->Printf(PRINT_ALL, "GPU_MEMORY_INFO_EVICTION_COUNT_NVX: %i\n", value);

			qglGetIntegerv(GL_GPU_MEMORY_INFO_EVICTED_MEMORY_NVX, &value);
			ri->Printf(PRINT_ALL, "GPU_MEMORY_INFO_EVICTED_MEMORY_NVX: %ikb\n", value);
		}
		break;
		case MI_ATI:
		{
			// GL_ATI_meminfo
			int value[4];

			qglGetIntegerv(GL_VBO_FREE_MEMORY_ATI, &value[0]);
			ri->Printf(PRINT_ALL, "VBO_FREE_MEMORY_ATI: %ikb total %ikb largest aux: %ikb total %ikb largest\n", value[0], value[1], value[2], value[3]);

			qglGetIntegerv(GL_TEXTURE_FREE_MEMORY_ATI, &value[0]);
			ri->Printf(PRINT_ALL, "TEXTURE_FREE_MEMORY_ATI: %ikb total %ikb largest aux: %ikb total %ikb largest\n", value[0], value[1], value[2], value[3]);

			qglGetIntegerv(GL_RENDERBUFFER_FREE_MEMORY_ATI, &value[0]);
			ri->Printf(PRINT_ALL, "RENDERBUFFER_FREE_MEMORY_ATI: %ikb total %ikb largest aux: %ikb total %ikb largest\n", value[0], value[1], value[2], value[3]);
		}
		break;
	}
}

#ifdef _WIN32
#define SWAPINTERVAL_FLAGS CVAR_ARCHIVE
#else
#define SWAPINTERVAL_FLAGS CVAR_ARCHIVE | CVAR_LATCH
#endif

/*
===============
R_Register
===============
*/
void R_Register( void )
{
	r_superSampleMultiplier = ri->Cvar_Get( "r_superSampleMultiplier", "1", CVAR_ARCHIVE | CVAR_LATCH );

	r_instanceCloudReductionCulling = ri->Cvar_Get( "r_instanceCloudReductionCulling", "0", CVAR_ARCHIVE | CVAR_LATCH );

	r_tesselation = ri->Cvar_Get( "r_tesselation", "0", CVAR_ARCHIVE | CVAR_LATCH );
	r_tesselationLevel = ri->Cvar_Get( "r_tesselationLevel", "7.0", CVAR_ARCHIVE );
	r_tesselationAlpha = ri->Cvar_Get( "r_tesselationAlpha", "1.0", CVAR_ARCHIVE );

	r_foliage = ri->Cvar_Get( "r_foliage", "2", CVAR_ARCHIVE | CVAR_LATCH );
	r_foliagePasses = ri->Cvar_Get( "r_foliagePasses", "2", CVAR_ARCHIVE );
	r_foliageDistance = ri->Cvar_Get( "r_foliageDistance", /*"16384.0"*/"8192.0", CVAR_ARCHIVE );
	r_foliageDensity = ri->Cvar_Get( "r_foliageDensity", "40.0", CVAR_ARCHIVE );
	r_foliageShadows = ri->Cvar_Get( "r_foliageShadows", "0", CVAR_ARCHIVE );

	r_pebbles = ri->Cvar_Get("r_pebbles", "0", CVAR_ARCHIVE | CVAR_LATCH);
	r_pebblesPasses = ri->Cvar_Get("r_pebblesPasses", "2", CVAR_ARCHIVE);
	r_pebblesDistance = ri->Cvar_Get( "r_pebblesDistance", "1536.0", CVAR_ARCHIVE );

	//
	// latched and archived variables
	//
	r_allowExtensions = ri->Cvar_Get( "r_allowExtensions", "1", CVAR_ARCHIVE | CVAR_LATCH );
	r_ext_compressed_textures = ri->Cvar_Get( "r_ext_compress_textures", "2", CVAR_ARCHIVE | CVAR_LATCH );
	r_ext_multitexture = ri->Cvar_Get( "r_ext_multitexture", "1", CVAR_ARCHIVE | CVAR_LATCH );
	r_ext_compiled_vertex_array = ri->Cvar_Get( "r_ext_compiled_vertex_array", "1", CVAR_ARCHIVE | CVAR_LATCH);
	r_ext_texture_env_add = ri->Cvar_Get( "r_ext_texture_env_add", "1", CVAR_ARCHIVE | CVAR_LATCH);
	r_ext_preferred_tc_method = ri->Cvar_Get( "r_ext_preferred_tc_method", "0", CVAR_ARCHIVE | CVAR_LATCH );

	r_entitySurfaceMerge = ri->Cvar_Get("r_entitySurfaceMerge", "1", CVAR_ARCHIVE);

	r_occlusion = ri->Cvar_Get( "r_occlusion", "0", CVAR_ARCHIVE);
	r_occlusionDebug = ri->Cvar_Get( "r_occlusionDebug", "0", CVAR_ARCHIVE);
	r_ext_draw_range_elements = ri->Cvar_Get( "r_ext_draw_range_elements", "1", CVAR_ARCHIVE | CVAR_LATCH);
	r_ext_multi_draw_arrays = ri->Cvar_Get( "r_ext_multi_draw_arrays", "1", CVAR_ARCHIVE | CVAR_LATCH);
	r_ext_texture_float = ri->Cvar_Get( "r_ext_texture_float", "1", CVAR_ARCHIVE | CVAR_LATCH);
	r_arb_half_float_pixel = ri->Cvar_Get( "r_arb_half_float_pixel", "1", CVAR_ARCHIVE | CVAR_LATCH);
	r_ext_framebuffer_multisample = ri->Cvar_Get( "r_ext_framebuffer_multisample", "0", CVAR_ARCHIVE | CVAR_LATCH);
	r_arb_seamless_cube_map = ri->Cvar_Get( "r_arb_seamless_cube_map", "0", CVAR_ARCHIVE | CVAR_LATCH);
	r_arb_vertex_type_2_10_10_10_rev = ri->Cvar_Get( "r_arb_vertex_type_2_10_10_10_rev", "1", CVAR_ARCHIVE | CVAR_LATCH);
	r_arb_buffer_storage = ri->Cvar_Get("r_arb_buffer_storage", "0", CVAR_ARCHIVE | CVAR_LATCH);
	r_ext_texture_filter_anisotropic = ri->Cvar_Get( "r_ext_texture_filter_anisotropic", "0", CVAR_ARCHIVE );
	r_debugContext = ri->Cvar_Get("r_debugContext", "0", CVAR_LATCH);

	r_lazyFrustum = ri->Cvar_Get("r_lazyFrustum", "0", CVAR_ARCHIVE);
	r_cacheVisibleSurfaces = ri->Cvar_Get("r_cacheVisibleSurfaces", "0", CVAR_ARCHIVE);

	r_dynamicGlow						= ri->Cvar_Get( "r_dynamicGlow",			"1",		CVAR_ARCHIVE );
	r_dynamicGlowPasses					= ri->Cvar_Get( "r_dynamicGlowPasses",		"5",		CVAR_ARCHIVE );
	r_dynamicGlowIntensity				= ri->Cvar_Get( "r_dynamicGlowIntensity",	"1.17",		CVAR_ARCHIVE );
	r_dynamicGlowSoft					= ri->Cvar_Get( "r_dynamicGlowSoft",		"1",		CVAR_ARCHIVE );

	r_picmip = ri->Cvar_Get ("r_picmip", "0", CVAR_ARCHIVE | CVAR_LATCH );
	ri->Cvar_CheckRange( r_picmip, 0, 16, qtrue );
	r_roundImagesDown = ri->Cvar_Get ("r_roundImagesDown", "1", CVAR_ARCHIVE | CVAR_LATCH );
	r_colorMipLevels = ri->Cvar_Get ("r_colorMipLevels", "0", CVAR_LATCH );
	r_detailTextures = ri->Cvar_Get( "r_detailtextures", "1", CVAR_ARCHIVE | CVAR_LATCH );
	r_texturebits = ri->Cvar_Get( "r_texturebits", "0", CVAR_ARCHIVE | CVAR_LATCH );
	r_colorbits = ri->Cvar_Get( "r_colorbits", "0", CVAR_ARCHIVE | CVAR_LATCH );
	r_stencilbits = ri->Cvar_Get( "r_stencilbits", "8", CVAR_ARCHIVE | CVAR_LATCH );
	r_depthbits = ri->Cvar_Get( "r_depthbits", "0", CVAR_ARCHIVE | CVAR_LATCH );
	r_ext_multisample = ri->Cvar_Get( "r_ext_multisample", "0", CVAR_ARCHIVE | CVAR_LATCH );
	ri->Cvar_CheckRange( r_ext_multisample, 0, 4, qtrue );
	r_overBrightBits = ri->Cvar_Get ("r_overBrightBits", "0", CVAR_ARCHIVE | CVAR_LATCH );
	r_ignorehwgamma = ri->Cvar_Get( "r_ignorehwgamma", "0", CVAR_ARCHIVE | CVAR_LATCH);
	r_mode = ri->Cvar_Get( "r_mode", "4", CVAR_ARCHIVE | CVAR_LATCH );
	r_fullscreen = ri->Cvar_Get( "r_fullscreen", "0", CVAR_ARCHIVE|CVAR_LATCH );
	r_noborder = ri->Cvar_Get("r_noborder", "0", CVAR_ARCHIVE|CVAR_LATCH);
	r_centerWindow = ri->Cvar_Get( "r_centerWindow", "0", CVAR_ARCHIVE|CVAR_LATCH );
	r_customwidth = ri->Cvar_Get( "r_customwidth", "1600", CVAR_ARCHIVE | CVAR_LATCH );
	r_customheight = ri->Cvar_Get( "r_customheight", "1024", CVAR_ARCHIVE | CVAR_LATCH );
	r_customPixelAspect = ri->Cvar_Get( "r_customPixelAspect", "1", CVAR_ARCHIVE | CVAR_LATCH );
	r_simpleMipMaps = ri->Cvar_Get( "r_simpleMipMaps", "1", CVAR_ARCHIVE | CVAR_LATCH );
	r_vertexLight = ri->Cvar_Get( "r_vertexLight", "0", CVAR_ARCHIVE | CVAR_LATCH );
	r_uiFullScreen = ri->Cvar_Get( "r_uifullscreen", "0", 0);
	r_subdivisions = ri->Cvar_Get ("r_subdivisions", "4", CVAR_ARCHIVE | CVAR_LATCH);
	ri->Cvar_CheckRange( r_subdivisions, 4, 80, qfalse );
	r_stereo = ri->Cvar_Get( "r_stereo", "0", CVAR_ARCHIVE | CVAR_LATCH);
	r_greyscale = ri->Cvar_Get("r_greyscale", "0", CVAR_ARCHIVE | CVAR_LATCH);
	ri->Cvar_CheckRange(r_greyscale, 0, 1, qfalse);

	r_hdr = ri->Cvar_Get( "r_hdr", "0", CVAR_ARCHIVE | CVAR_LATCH );
	r_floatLightmap = ri->Cvar_Get( "r_floatLightmap", "0", CVAR_ARCHIVE | CVAR_LATCH );
	r_postProcess = ri->Cvar_Get( "r_postProcess", "1", CVAR_ARCHIVE );

	r_toneMap = ri->Cvar_Get( "r_toneMap", "0", CVAR_ARCHIVE | CVAR_LATCH ); // UQ1: Disabled. Pointless.
	r_forceToneMap = ri->Cvar_Get( "r_forceToneMap", "0", CVAR_CHEAT );
	r_forceToneMapMin = ri->Cvar_Get( "r_forceToneMapMin", "-8.0", CVAR_CHEAT );
	r_forceToneMapAvg = ri->Cvar_Get( "r_forceToneMapAvg", "-2.0", CVAR_CHEAT );
	r_forceToneMapMax = ri->Cvar_Get( "r_forceToneMapMax", "0.0", CVAR_CHEAT );

	r_autoExposure = ri->Cvar_Get( "r_autoExposure", "0", CVAR_ARCHIVE ); // UQ1: Disabled. Pointless.
	r_forceAutoExposure = ri->Cvar_Get( "r_forceAutoExposure", "0", CVAR_CHEAT );
	r_forceAutoExposureMin = ri->Cvar_Get( "r_forceAutoExposureMin", "-2.0", CVAR_CHEAT );
	r_forceAutoExposureMax = ri->Cvar_Get( "r_forceAutoExposureMax", "2.0", CVAR_CHEAT );

	r_cameraExposure = ri->Cvar_Get( "r_cameraExposure", "0", CVAR_CHEAT );

	r_srgb = ri->Cvar_Get( "r_srgb", "0", CVAR_ARCHIVE | CVAR_LATCH );

	r_depthPrepass = ri->Cvar_Get( "r_depthPrepass", "1", CVAR_ARCHIVE );
	r_ssao = ri->Cvar_Get( "r_ssao", "1", /*CVAR_LATCH |*/ CVAR_ARCHIVE );

	r_normalMapping = ri->Cvar_Get( "r_normalMapping", "1", CVAR_ARCHIVE | CVAR_LATCH );
	r_normalMapQuality = ri->Cvar_Get( "r_normalMapQuality", "1", CVAR_ARCHIVE | CVAR_LATCH );
	r_specularMapping = ri->Cvar_Get( "r_specularMapping", "1", CVAR_ARCHIVE | CVAR_LATCH );
	r_deluxeMapping = ri->Cvar_Get( "r_deluxeMapping", "1", CVAR_ARCHIVE | CVAR_LATCH );
	r_parallaxMapping = ri->Cvar_Get( "r_parallaxMapping", "0", CVAR_ARCHIVE | CVAR_LATCH );
	r_cubeMapping = ri->Cvar_Get( "r_cubeMapping", "2", CVAR_ARCHIVE | CVAR_LATCH );
	r_cubeMapSize = ri->Cvar_Get( "r_cubeMapSize", "64", CVAR_ARCHIVE | CVAR_LATCH );
   	r_deluxeSpecular = ri->Cvar_Get( "r_deluxeSpecular", "0.3", CVAR_ARCHIVE );
   	r_specularIsMetallic = ri->Cvar_Get( "r_specularIsMetallic", "0", CVAR_ARCHIVE | CVAR_LATCH );
	r_baseNormalX = ri->Cvar_Get( "r_baseNormalX", "0.04", CVAR_ARCHIVE );
	r_baseNormalY = ri->Cvar_Get( "r_baseNormalY", "0.04", CVAR_ARCHIVE );
	r_baseParallax = ri->Cvar_Get( "r_baseParallax", "0.001", CVAR_ARCHIVE );
   	r_baseSpecular = ri->Cvar_Get( "r_baseSpecular", "0.04", CVAR_ARCHIVE );
   	r_baseGloss = ri->Cvar_Get( "r_baseGloss", "0.1", CVAR_ARCHIVE );
	r_dlightMode = ri->Cvar_Get( "r_dlightMode", "1", CVAR_ARCHIVE | CVAR_LATCH );
	r_pshadowDist = ri->Cvar_Get( "r_pshadowDist", "128", CVAR_ARCHIVE );
	r_mergeLightmaps = ri->Cvar_Get( "r_mergeLightmaps", "0", CVAR_ARCHIVE | CVAR_LATCH );
	r_imageUpsample = ri->Cvar_Get( "r_imageUpsample", "0", CVAR_ARCHIVE | CVAR_LATCH );
	r_imageUpsampleMaxSize = ri->Cvar_Get( "r_imageUpsampleMaxSize", "1024", CVAR_ARCHIVE | CVAR_LATCH );
	r_imageUpsampleType = ri->Cvar_Get( "r_imageUpsampleType", "1", CVAR_ARCHIVE | CVAR_LATCH );
	r_genNormalMaps = ri->Cvar_Get( "r_genNormalMaps", "0", CVAR_ARCHIVE | CVAR_LATCH );

	r_dlightShadows = ri->Cvar_Get( "r_dlightShadows", "0", CVAR_ARCHIVE );
	r_forceSun = ri->Cvar_Get( "r_forceSun", "0", CVAR_ARCHIVE | CVAR_LATCH );
	r_forceSunMapLightScale = ri->Cvar_Get( "r_forceSunMapLightScale", "1.0", CVAR_CHEAT );
	r_forceSunLightScale = ri->Cvar_Get( "r_forceSunLightScale", "1.0", CVAR_CHEAT );
	r_forceSunAmbientScale = ri->Cvar_Get( "r_forceSunAmbientScale", "0.5", CVAR_CHEAT );
	r_drawSunRays = ri->Cvar_Get( "r_drawSunRays", "0", CVAR_ARCHIVE | CVAR_LATCH );
	r_sunlightMode = ri->Cvar_Get( "r_sunlightMode", "1", CVAR_ARCHIVE | CVAR_LATCH );
	r_sunlightSpecular = ri->Cvar_Get( "r_sunlightSpecular", "1", CVAR_ARCHIVE );

	r_shadowBlurPasses = ri->Cvar_Get( "r_shadowBlurPasses", "0", CVAR_ARCHIVE );
	r_shadowFilter = ri->Cvar_Get( "r_shadowFilter", "2", CVAR_ARCHIVE | CVAR_LATCH );
	r_shadowMapSize = ri->Cvar_Get( "r_shadowMapSize", "2048", CVAR_ARCHIVE | CVAR_LATCH );
	r_shadowCascadeZNear = ri->Cvar_Get( "r_shadowCascadeZNear", "4", CVAR_ARCHIVE );
	r_shadowCascadeZFar = ri->Cvar_Get( "r_shadowCascadeZFar", "3072", CVAR_ARCHIVE );
	r_shadowCascadeZBias = ri->Cvar_Get( "r_shadowCascadeZBias", "-320", CVAR_ARCHIVE );
	r_ignoreDstAlpha = ri->Cvar_Get( "r_ignoreDstAlpha", "1", CVAR_ARCHIVE | CVAR_LATCH );

	//
	// UQ1: Added...
	//
	r_parallaxScale = ri->Cvar_Get( "r_parallaxScale", "3.0", CVAR_ARCHIVE );
	r_blinnPhong = ri->Cvar_Get( "r_blinnPhong", "0.5", CVAR_ARCHIVE );
	r_skynum = ri->Cvar_Get( "r_skynum", "0", CVAR_ARCHIVE );
	r_volumeLightStrength = ri->Cvar_Get( "r_volumeLightStrength", "0.15", CVAR_ARCHIVE );
	r_disableGfxDirEnhancement = ri->Cvar_Get( "r_disableGfxDirEnhancement", "0", CVAR_ARCHIVE | CVAR_LATCH );
	r_cubemapCullRange = ri->Cvar_Get( "r_cubemapCullRange", "768.0", CVAR_ARCHIVE );
	r_cubemapCullFalloffMult = ri->Cvar_Get( "r_cubemapCullFalloffMult", "1.5", CVAR_ARCHIVE );
	r_glslWater = ri->Cvar_Get( "r_glslWater", "2", CVAR_ARCHIVE );
	r_waterWaveHeight = ri->Cvar_Get( "r_waterWaveHeight", "8.0", CVAR_ARCHIVE );
	r_waterWaveDensity = ri->Cvar_Get( "r_waterWaveDensity", "0.5", CVAR_ARCHIVE );
	r_grassLength = ri->Cvar_Get( "r_grassLength", "0.4", CVAR_ARCHIVE );
	r_grassWaveSpeed = ri->Cvar_Get( "r_grassWaveSpeed", "4.0", CVAR_ARCHIVE );
	r_grassWaveSize = ri->Cvar_Get( "r_grassWaveSize", "0.5", CVAR_ARCHIVE );

	r_fog = ri->Cvar_Get( "r_fog", "1", CVAR_ARCHIVE ); // UQ1: For disabling fog to test speeds...
	r_multithread = ri->Cvar_Get( "r_multithread", "0", CVAR_ARCHIVE );
	r_multithread2 = ri->Cvar_Get( "r_multithread2", "0", CVAR_ARCHIVE );
	r_fOff1X = ri->Cvar_Get( "r_fOff1X", "-24.0", CVAR_ARCHIVE );
	r_fOff1Y = ri->Cvar_Get( "r_fOff1Y", "-52.0", CVAR_ARCHIVE );
	r_fOff2X = ri->Cvar_Get( "r_fOff2X", "28.0", CVAR_ARCHIVE );
	r_fOff2Y = ri->Cvar_Get( "r_fOff2Y", "42.0", CVAR_ARCHIVE );
	r_fOff3X = ri->Cvar_Get( "r_fOff3X", "-48.0", CVAR_ARCHIVE );
	r_fOff3Y = ri->Cvar_Get( "r_fOff3Y", "10.0", CVAR_ARCHIVE );
	r_testvar = ri->Cvar_Get( "r_testvar", "1.0", CVAR_ARCHIVE );
	r_steepParallaxEyeX = ri->Cvar_Get( "r_steepParallaxEyeX", "0.0000", CVAR_ARCHIVE );
	r_steepParallaxEyeY = ri->Cvar_Get( "r_steepParallaxEyeY", "0.0002", CVAR_ARCHIVE );
	r_steepParallaxEyeZ = ri->Cvar_Get( "r_steepParallaxEyeZ", "0.0004", CVAR_ARCHIVE );
	r_bloom = ri->Cvar_Get( "r_bloom", "1", CVAR_ARCHIVE );
	r_bloomPasses = ri->Cvar_Get( "r_bloomPasses", "1", CVAR_ARCHIVE );
	r_bloomDarkenPower = ri->Cvar_Get( "r_bloomDarkenPower", "5.0", CVAR_ARCHIVE );
	r_bloomScale = ri->Cvar_Get( "r_bloomScale", "1.5", CVAR_ARCHIVE );
	r_lensflare = ri->Cvar_Get( "r_lensflare", "0", CVAR_ARCHIVE );
	r_anamorphic = ri->Cvar_Get( "r_anamorphic", "1", CVAR_ARCHIVE );
	r_anamorphicDarkenPower = ri->Cvar_Get( "r_anamorphicDarkenPower", "256.0", CVAR_ARCHIVE );
	r_ssgi = ri->Cvar_Get( "r_ssgi", "0", CVAR_ARCHIVE );
	r_ssgiWidth = ri->Cvar_Get( "r_ssgiWidth", "12.0", CVAR_ARCHIVE );
	r_ssgiSamples = ri->Cvar_Get( "r_ssgiSamples", "4", CVAR_ARCHIVE );
	r_depthParallax = ri->Cvar_Get( "r_depthParallax", "0", CVAR_ARCHIVE );
	r_depthParallaxScale = ri->Cvar_Get( "r_depthParallaxScale", "2.0", CVAR_ARCHIVE );
	r_depthParallaxMultiplier = ri->Cvar_Get( "r_depthParallaxMultiplier", "5.0", CVAR_ARCHIVE );
	r_depthParallaxEyeX = ri->Cvar_Get( "r_depthParallaxEyeX", "0.0001", CVAR_ARCHIVE );
	r_depthParallaxEyeY = ri->Cvar_Get( "r_depthParallaxEyeY", "0.0001", CVAR_ARCHIVE );
	r_depthParallaxEyeZ = ri->Cvar_Get( "r_depthParallaxEyeZ", "0.0001", CVAR_ARCHIVE );
	r_depth = ri->Cvar_Get( "r_depth", "0", CVAR_ARCHIVE );
	r_depthPasses = ri->Cvar_Get( "r_depthPasses", "4", CVAR_ARCHIVE );
	r_depthScale = ri->Cvar_Get( "r_depthScale", "1024.0", CVAR_ARCHIVE );
	r_darkexpand = ri->Cvar_Get( "r_darkexpand", "0", CVAR_ARCHIVE );
	r_truehdr = ri->Cvar_Get( "r_truehdr", "1", CVAR_ARCHIVE );
	r_magicdetail = ri->Cvar_Get( "r_magicdetail", "1", CVAR_ARCHIVE );
	r_magicdetailStrength = ri->Cvar_Get( "r_magicdetailStrength", "0.05", CVAR_ARCHIVE );
	r_dof = ri->Cvar_Get( "r_dof", "0", CVAR_ARCHIVE );
	r_testvalue0 = ri->Cvar_Get( "r_testvalue0", "0.1", CVAR_ARCHIVE );
	r_testvalue1 = ri->Cvar_Get( "r_testvalue1", "0.005", CVAR_ARCHIVE );
	r_testvalue2 = ri->Cvar_Get( "r_testvalue2", "4.0", CVAR_ARCHIVE );
	r_testvalue3 = ri->Cvar_Get( "r_testvalue3", "4.0", CVAR_ARCHIVE );
	r_esharpening = ri->Cvar_Get( "r_esharpening", "0", CVAR_ARCHIVE );
	r_esharpening2 = ri->Cvar_Get( "r_esharpening2", "0", CVAR_ARCHIVE );
	r_fxaa = ri->Cvar_Get( "r_fxaa", "1", CVAR_ARCHIVE );
	r_underwater = ri->Cvar_Get( "r_underwater", "1", CVAR_ARCHIVE );
	r_multipost = ri->Cvar_Get( "r_multipost", "0", CVAR_ARCHIVE );
	r_textureClean = ri->Cvar_Get( "r_textureClean", "0", CVAR_ARCHIVE );
	r_textureCleanSigma = ri->Cvar_Get( "r_textureCleanSigma", "0.5", CVAR_ARCHIVE ); // 1.2
	r_textureCleanBSigma = ri->Cvar_Get( "r_textureCleanBSigma", "0.1", CVAR_ARCHIVE );
	r_textureCleanMSize = ri->Cvar_Get( "r_textureCleanMSize", "6.0", CVAR_ARCHIVE );
	r_imageBasedLighting = ri->Cvar_Get( "r_imageBasedLighting", "0", CVAR_ARCHIVE );
	r_sss = ri->Cvar_Get( "r_sss", "1", CVAR_ARCHIVE );
	r_sssMinRange = ri->Cvar_Get( "r_sssMinRange", "0.004", CVAR_ARCHIVE );
	r_sssMaxRange = ri->Cvar_Get( "r_sssMaxRange", "0.02", CVAR_ARCHIVE );
	r_rbm = ri->Cvar_Get( "r_rbm", "0", CVAR_ARCHIVE );
	r_rbmStrength = ri->Cvar_Get( "r_rbmStrength", "0.22", CVAR_ARCHIVE );
	r_hbao = ri->Cvar_Get( "r_hbao", "0", CVAR_ARCHIVE );
	r_deferredLighting = ri->Cvar_Get( "r_deferredLighting", "1", CVAR_ARCHIVE );
	r_colorCorrection = ri->Cvar_Get( "r_colorCorrection", "1", CVAR_ARCHIVE );
	r_trueAnaglyph = ri->Cvar_Get( "r_trueAnaglyph", "0", CVAR_ARCHIVE );
	r_trueAnaglyphSeparation = ri->Cvar_Get( "r_trueAnaglyphSeparation", "10.0", CVAR_ARCHIVE );
	r_trueAnaglyphRed = ri->Cvar_Get( "r_trueAnaglyphRed", "0.0", CVAR_ARCHIVE );
	r_trueAnaglyphGreen = ri->Cvar_Get( "r_trueAnaglyphGreen", "0.0", CVAR_ARCHIVE );
	r_trueAnaglyphBlue = ri->Cvar_Get( "r_trueAnaglyphBlue", "0.0", CVAR_ARCHIVE );
	r_trueAnaglyphPower = ri->Cvar_Get( "r_trueAnaglyphPower", "200.0", CVAR_ARCHIVE );
	r_trueAnaglyphMinDistance = ri->Cvar_Get( "r_trueAnaglyphMinDistance", "0.03", CVAR_ARCHIVE );
	r_trueAnaglyphMaxDistance = ri->Cvar_Get( "r_trueAnaglyphMaxDistance", "1.0", CVAR_ARCHIVE );
	r_trueAnaglyphParallax = ri->Cvar_Get( "r_trueAnaglyphParallax", "11.5", CVAR_ARCHIVE );
	r_vibrancy = ri->Cvar_Get( "r_vibrancy", "0.4", CVAR_ARCHIVE );
	r_distanceBlur = ri->Cvar_Get( "r_distanceBlur", "1", CVAR_ARCHIVE );
	r_fogPost = ri->Cvar_Get( "r_fogPost", "1", CVAR_ARCHIVE );
	r_dayNightCycleSpeed = ri->Cvar_Get( "r_dayNightCycleSpeed", "0.0001", CVAR_ARCHIVE );
	r_testshader = ri->Cvar_Get( "r_testshader", "0", CVAR_ARCHIVE );
	r_testshaderValue1 = ri->Cvar_Get( "r_testshaderValue1", "1.0", CVAR_ARCHIVE );
	r_testshaderValue2 = ri->Cvar_Get( "r_testshaderValue2", "0.0", CVAR_ARCHIVE );
	r_testshaderValue3 = ri->Cvar_Get( "r_testshaderValue3", "0.0", CVAR_ARCHIVE );
	r_testshaderValue4 = ri->Cvar_Get( "r_testshaderValue4", "1.0", CVAR_ARCHIVE );
	r_testshaderValue5 = ri->Cvar_Get( "r_testshaderValue5", "0.0", CVAR_ARCHIVE );
	r_testshaderValue6 = ri->Cvar_Get( "r_testshaderValue6", "0.0", CVAR_ARCHIVE );
	r_testshaderValue7 = ri->Cvar_Get( "r_testshaderValue7", "1.0", CVAR_ARCHIVE );
	r_testshaderValue8 = ri->Cvar_Get( "r_testshaderValue8", "0.0", CVAR_ARCHIVE );
	r_testshaderValue9 = ri->Cvar_Get( "r_testshaderValue9", "0.0", CVAR_ARCHIVE );

	//
	// UQ1: End Added...
	//

	//
	// temporary latched variables that can only change over a restart
	//
	r_fullbright = ri->Cvar_Get ("r_fullbright", "0", CVAR_ARCHIVE | CVAR_LATCH | CVAR_CHEAT );
	r_mapOverBrightBits = ri->Cvar_Get ("r_mapOverBrightBits", "0", CVAR_ARCHIVE | CVAR_LATCH );
	r_intensity = ri->Cvar_Get ("r_intensity", "1", CVAR_ARCHIVE | CVAR_LATCH );
	r_singleShader = ri->Cvar_Get ("r_singleShader", "0", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_LATCH );
	r_displayRefresh = ri->Cvar_Get( "r_displayRefresh", "0", CVAR_ARCHIVE | CVAR_LATCH );
	ri->Cvar_CheckRange( r_displayRefresh, 0, 240, qtrue );

	//
	// archived variables that can change at any time
	//
	r_lodCurveError = ri->Cvar_Get( "r_lodCurveError", "250", CVAR_ARCHIVE|CVAR_CHEAT );
	r_lodbias = ri->Cvar_Get( "r_lodbias", "0", CVAR_ARCHIVE );
	r_flares = ri->Cvar_Get ("r_flares", "0", CVAR_ARCHIVE );
	r_znear = ri->Cvar_Get( "r_znear", "2", CVAR_CHEAT ); // originally 4
	ri->Cvar_CheckRange( r_znear, 0.001f, 200, qfalse );
	r_autolodscalevalue	= ri->Cvar_Get( "r_autolodscalevalue", "0", CVAR_ROM );
	r_zproj = ri->Cvar_Get( "r_zproj", "64", CVAR_ARCHIVE );
	r_stereoSeparation = ri->Cvar_Get( "r_stereoSeparation", "64", CVAR_ARCHIVE );
	r_ignoreGLErrors = ri->Cvar_Get( "r_ignoreGLErrors", "1", CVAR_ARCHIVE );
	r_fastsky = ri->Cvar_Get( "r_fastsky", "0", CVAR_ARCHIVE );
	r_inGameVideo = ri->Cvar_Get( "r_inGameVideo", "1", CVAR_ARCHIVE );
	r_drawSun = ri->Cvar_Get( "r_drawSun", "1", CVAR_ARCHIVE );
	r_dynamiclight = ri->Cvar_Get( "r_dynamiclight", "0", CVAR_ARCHIVE );
	r_finish = ri->Cvar_Get ("r_finish", "0", CVAR_ARCHIVE);
	r_textureMode = ri->Cvar_Get( "r_textureMode", "GL_LINEAR_MIPMAP_NEAREST", CVAR_ARCHIVE );
	r_swapInterval = ri->Cvar_Get( "r_swapInterval", "0",
					SWAPINTERVAL_FLAGS );
	r_markcount = ri->Cvar_Get( "r_markcount", "100", CVAR_ARCHIVE );
	r_gamma = ri->Cvar_Get( "r_gamma", "1", CVAR_ARCHIVE );
	r_facePlaneCull = ri->Cvar_Get ("r_facePlaneCull", "1", CVAR_ARCHIVE );

	r_ambientScale = ri->Cvar_Get( "r_ambientScale", "0.6", CVAR_CHEAT );
	r_directedScale = ri->Cvar_Get( "r_directedScale", "1", CVAR_CHEAT );

	r_anaglyphMode = ri->Cvar_Get("r_anaglyphMode", "0", CVAR_ARCHIVE);
	r_mergeMultidraws = ri->Cvar_Get("r_mergeMultidraws", "1", CVAR_ARCHIVE);
	r_mergeLeafSurfaces = ri->Cvar_Get("r_mergeLeafSurfaces", "1", CVAR_ARCHIVE);

	r_rotatex							= ri->Cvar_Get( "r_rotatex",						"0",						CVAR_TEMP );
	r_rotatey							= ri->Cvar_Get( "r_rotatey",						"0",						CVAR_TEMP );
	r_rotatez							= ri->Cvar_Get( "r_rotatez",						"0",						CVAR_TEMP );

	r_materialDebug						= ri->Cvar_Get("r_materialDebug",					"0",						CVAR_ARCHIVE);
	r_genericShaderDebug				= ri->Cvar_Get("r_genericShaderDebug",				"0",						CVAR_ARCHIVE);

	r_weather							= ri->Cvar_Get( "r_weather",						"0",						CVAR_TEMP );
	r_surfaceSprites					= ri->Cvar_Get( "r_surfaceSprites",					"0",						CVAR_TEMP );
	r_surfaceWeather					= ri->Cvar_Get( "r_surfaceWeather",					"0",						CVAR_TEMP );
	r_windSpeed							= ri->Cvar_Get( "r_windSpeed",						"0",						CVAR_NONE );
	r_windAngle							= ri->Cvar_Get( "r_windAngle",						"0",						CVAR_NONE );
	r_windGust							= ri->Cvar_Get( "r_windGust",						"0",						CVAR_NONE );
	r_windDampFactor					= ri->Cvar_Get( "r_windDampFactor",					"0.1",						CVAR_NONE );
	r_windPointForce					= ri->Cvar_Get( "r_windPointForce",					"0",						CVAR_NONE );
	r_windPointX						= ri->Cvar_Get( "r_windPointX",						"0",						CVAR_NONE );
	r_windPointY						= ri->Cvar_Get( "r_windPointY",						"0",						CVAR_NONE );

	//
	// temporary variables that can change at any time
	//
	r_showImages = ri->Cvar_Get( "r_showImages", "0", CVAR_TEMP );

	r_debugLight = ri->Cvar_Get( "r_debuglight", "0", CVAR_TEMP );
	r_debugSort = ri->Cvar_Get( "r_debugSort", "0", CVAR_CHEAT );
	r_printShaders = ri->Cvar_Get( "r_printShaders", "0", 0 );
	r_saveFontData = ri->Cvar_Get( "r_saveFontData", "0", 0 );

	r_nocurves = ri->Cvar_Get ("r_nocurves", "0", CVAR_CHEAT );
	r_drawworld = ri->Cvar_Get ("r_drawworld", "1", CVAR_CHEAT );
	r_lightmap = ri->Cvar_Get ("r_lightmap", "0", 0 );
	r_portalOnly = ri->Cvar_Get ("r_portalOnly", "0", CVAR_CHEAT );

	r_flareSize = ri->Cvar_Get ("r_flareSize", "40", CVAR_CHEAT);
	r_flareFade = ri->Cvar_Get ("r_flareFade", "7", CVAR_CHEAT);
	r_flareCoeff = ri->Cvar_Get ("r_flareCoeff", FLARE_STDCOEFF, CVAR_CHEAT);

	r_skipBackEnd = ri->Cvar_Get ("r_skipBackEnd", "0", CVAR_CHEAT);

	r_measureOverdraw = ri->Cvar_Get( "r_measureOverdraw", "0", CVAR_CHEAT );
	r_lodscale = ri->Cvar_Get( "r_lodscale", "5", CVAR_CHEAT );
	r_norefresh = ri->Cvar_Get ("r_norefresh", "0", CVAR_CHEAT);
	r_drawentities = ri->Cvar_Get ("r_drawentities", "1", CVAR_CHEAT );
	r_ignore = ri->Cvar_Get( "r_ignore", "1", CVAR_CHEAT );
	r_nocull = ri->Cvar_Get ("r_nocull", "0", CVAR_CHEAT);
	r_entityCull = ri->Cvar_Get ("r_entityCull", "1", CVAR_CHEAT);
	r_fovCull = ri->Cvar_Get ("r_fovCull", "0", CVAR_CHEAT);
	r_novis = ri->Cvar_Get ("r_novis", "0", CVAR_CHEAT);
	r_showcluster = ri->Cvar_Get ("r_showcluster", "0", CVAR_CHEAT);
	r_speeds = ri->Cvar_Get ("r_speeds", "0", CVAR_CHEAT);
	r_verbose = ri->Cvar_Get( "r_verbose", "0", CVAR_CHEAT );
	r_logFile = ri->Cvar_Get( "r_logFile", "0", CVAR_CHEAT );
	r_debugSurface = ri->Cvar_Get ("r_debugSurface", "0", CVAR_CHEAT);
	r_nobind = ri->Cvar_Get ("r_nobind", "0", CVAR_CHEAT);
	r_showtris = ri->Cvar_Get ("r_showtris", "0", CVAR_CHEAT);
	r_showsky = ri->Cvar_Get ("r_showsky", "0", CVAR_CHEAT);
	r_shownormals = ri->Cvar_Get ("r_shownormals", "0", CVAR_CHEAT);
	r_clear = ri->Cvar_Get ("r_clear", "0", CVAR_CHEAT);
	r_offsetFactor = ri->Cvar_Get( "r_offsetfactor", "-1", CVAR_CHEAT );
	r_offsetUnits = ri->Cvar_Get( "r_offsetunits", "-2", CVAR_CHEAT );
	r_drawBuffer = ri->Cvar_Get( "r_drawBuffer", "GL_BACK", CVAR_CHEAT );
	r_lockpvs = ri->Cvar_Get ("r_lockpvs", "0", CVAR_CHEAT);
	r_noportals = ri->Cvar_Get ("r_noportals", "0", CVAR_CHEAT);
	r_shadows = ri->Cvar_Get( "cg_shadows", "1", 0 );

	r_marksOnTriangleMeshes = ri->Cvar_Get("r_marksOnTriangleMeshes", "0", CVAR_ARCHIVE);

	r_aviMotionJpegQuality = ri->Cvar_Get("r_aviMotionJpegQuality", "90", CVAR_ARCHIVE);
	r_screenshotJpegQuality = ri->Cvar_Get("r_screenshotJpegQuality", "90", CVAR_ARCHIVE);

	r_maxpolys = ri->Cvar_Get( "r_maxpolys", va("%d", MAX_POLYS), 0);
	r_maxpolyverts = ri->Cvar_Get( "r_maxpolyverts", va("%d", MAX_POLYVERTS), 0);

/*
Ghoul2 Insert Start
*/
#ifdef _DEBUG
	r_noPrecacheGLA						= ri->Cvar_Get( "r_noPrecacheGLA",					"0",						CVAR_CHEAT );
#endif
	r_noServerGhoul2					= ri->Cvar_Get( "r_noserverghoul2",					"0",						CVAR_CHEAT );
	r_Ghoul2AnimSmooth					= ri->Cvar_Get( "r_ghoul2animsmooth",				"0.3",						CVAR_NONE );
	r_Ghoul2UnSqashAfterSmooth			= ri->Cvar_Get( "r_ghoul2unsqashaftersmooth",		"1",						CVAR_NONE );
	broadsword							= ri->Cvar_Get( "broadsword",						"0",						CVAR_NONE );
	broadsword_kickbones				= ri->Cvar_Get( "broadsword_kickbones",				"1",						CVAR_NONE );
	broadsword_kickorigin				= ri->Cvar_Get( "broadsword_kickorigin",			"1",						CVAR_NONE );
	broadsword_dontstopanim				= ri->Cvar_Get( "broadsword_dontstopanim",			"0",						CVAR_NONE );
	broadsword_waitforshot				= ri->Cvar_Get( "broadsword_waitforshot",			"0",						CVAR_NONE );
	broadsword_playflop					= ri->Cvar_Get( "broadsword_playflop",				"1",						CVAR_NONE );
	broadsword_smallbbox				= ri->Cvar_Get( "broadsword_smallbbox",				"0",						CVAR_NONE );
	broadsword_extra1					= ri->Cvar_Get( "broadsword_extra1",				"0",						CVAR_NONE );
	broadsword_extra2					= ri->Cvar_Get( "broadsword_extra2",				"0",						CVAR_NONE );
	broadsword_effcorr					= ri->Cvar_Get( "broadsword_effcorr",				"1",						CVAR_NONE );
	broadsword_ragtobase				= ri->Cvar_Get( "broadsword_ragtobase",				"2",						CVAR_NONE );
	broadsword_dircap					= ri->Cvar_Get( "broadsword_dircap",				"64",						CVAR_NONE );
/*
Ghoul2 Insert End
*/

	se_language = ri->Cvar_Get ( "se_language", "english", CVAR_ARCHIVE | CVAR_NORESTART );

	// make sure all the commands added here are also
	// removed in R_Shutdown
	ri->Cmd_AddCommand( "imagelist", R_ImageList_f );
	ri->Cmd_AddCommand( "shaderlist", R_ShaderList_f );
	ri->Cmd_AddCommand( "skinlist", R_SkinList_f );
	ri->Cmd_AddCommand( "fontlist", R_FontList_f );
	ri->Cmd_AddCommand( "modellist", R_Modellist_f );
	ri->Cmd_AddCommand( "modelist", R_ModeList_f );
	ri->Cmd_AddCommand( "screenshot", R_ScreenShotJPEG_f );
	ri->Cmd_AddCommand( "screenshot_png", R_ScreenShotPNG_f );
	ri->Cmd_AddCommand( "screenshot_tga", R_ScreenShotTGA_f );
	ri->Cmd_AddCommand( "gfxinfo", GfxInfo_f );
	//ri->Cmd_AddCommand( "minimize", GLimp_Minimize );
	ri->Cmd_AddCommand( "gfxmeminfo", GfxMemInfo_f );
#ifdef __SURFACESPRITES__
extern void R_WorldEffect_f(void);	//TR_WORLDEFFECTS.CPP
	ri->Cmd_AddCommand( "r_we", R_WorldEffect_f );
#endif //__SURFACESPRITES__
}

void R_InitQueries(void)
{
	if (r_drawSunRays->integer)
		qglGenQueries(ARRAY_LEN(tr.sunFlareQuery), tr.sunFlareQuery);
}

void R_ShutDownQueries(void)
{
	if (r_drawSunRays->integer)
		qglDeleteQueries(ARRAY_LEN(tr.sunFlareQuery), tr.sunFlareQuery);
}

void RE_SetLightStyle (int style, int color);

/*
===============
R_Init
===============
*/
void R_Init(void) {
	byte *ptr;
	int i;

	ri->Printf(PRINT_ALL, "----- R_Init -----\n");

	// clear all our internal state
	Com_Memset(&tr, 0, sizeof(tr));
	Com_Memset(&backEnd, 0, sizeof(backEnd));
	Com_Memset(&tess, 0, sizeof(tess));


	//
	// init function tables
	//
	for (i = 0; i < FUNCTABLE_SIZE; i++)
	{
		tr.sinTable[i] = sin(DEG2RAD(i * 360.0f / ((float)(FUNCTABLE_SIZE - 1))));
		tr.squareTable[i] = (i < FUNCTABLE_SIZE / 2) ? 1.0f : -1.0f;
		tr.sawToothTable[i] = (float)i / FUNCTABLE_SIZE;
		tr.inverseSawToothTable[i] = 1.0f - tr.sawToothTable[i];

		if (i < FUNCTABLE_SIZE / 2)
		{
			if (i < FUNCTABLE_SIZE / 4)
			{
				tr.triangleTable[i] = (float)i / (FUNCTABLE_SIZE / 4);
			}
			else
			{
				tr.triangleTable[i] = 1.0f - tr.triangleTable[i - FUNCTABLE_SIZE / 4];
			}
		}
		else
		{
			tr.triangleTable[i] = -tr.triangleTable[i - FUNCTABLE_SIZE / 2];
		}
	}

	R_InitFogTable();

	R_NoiseInit();
	R_ImageLoader_Init();
	R_Register();

	max_polys = Q_min(r_maxpolys->integer, MAX_POLYS);
	max_polyverts = Q_min(r_maxpolyverts->integer, MAX_POLYVERTS);

	ptr = (byte*)ri->Hunk_Alloc(sizeof(*backEndData) + sizeof(srfPoly_t) * max_polys + sizeof(polyVert_t) * max_polyverts, h_low);

	backEndData = (backEndData_t *)ptr;
	ptr = (byte *)(backEndData + 1);

	backEndData->polys = (srfPoly_t *)ptr;
	ptr += sizeof(*backEndData->polys) * max_polys;

	backEndData->polyVerts = (polyVert_t *)ptr;
	ptr += sizeof(*backEndData->polyVerts) * max_polyverts;

	R_InitNextFrame();

	for (int i = 0; i < MAX_LIGHT_STYLES; i++)
	{
		RE_SetLightStyle(i, -1);
	}

	InitOpenGL();

	R_InitImages();

	FBO_Init();

	int shadersStartTime = GLSL_BeginLoadGPUShaders();

#ifdef __ORIGINAL_OCCLUSION__
	//if (r_occlusion->integer)
	{
		OQ_InitOcclusionQuery();
	}
#endif //__ORIGINAL_OCCLUSION__

	R_InitVBOs();

	R_InitShaders(qfalse);

	R_InitSkins();

	R_InitFonts();

	R_ModelInit();

	R_InitDecals();

#ifdef __SURFACESPRITES__
	extern void R_InitWorldEffects(void);
	R_InitWorldEffects();
#endif //__SURFACESPRITES__

	R_InitQueries();

	GLSL_EndLoadGPUShaders(shadersStartTime);

#if defined(_DEBUG)
	GLenum err = qglGetError();
	if (err != GL_NO_ERROR)
		ri->Printf(PRINT_ALL, "glGetError() = 0x%x\n", err);
#endif

	RestoreGhoul2InfoArray();

	// print info
	GfxInfo_f();
	ri->Printf(PRINT_ALL, "----- finished R_Init -----\n");
}

/*
===============
RE_Shutdown
===============
*/
#ifdef __SURFACESPRITES__
extern void R_ShutdownWorldEffects(void);
#endif //__SURFACESPRITES__

void RE_Shutdown( qboolean destroyWindow, qboolean restarting ) {

	ri->Printf(PRINT_ALL, "RE_Shutdown( %i )\n", destroyWindow);

	R_ShutdownFonts();
	if (tr.registered) {
		R_IssuePendingRenderCommands();
		R_ShutDownQueries();
		FBO_Shutdown();
		R_DeleteTextures();
		R_ShutdownVBOs();
		GLSL_ShutdownGPUShaders();

		if (destroyWindow && restarting)
		{
			ri->Z_Free((void *)glConfig.extensions_string);
			ri->Z_Free((void *)glConfigExt.originalExtensionString);

			qglDeleteVertexArrays(1, &tr.globalVao);
			SaveGhoul2InfoArray();
		}
	}

	// shut down platform specific OpenGL stuff
	if (destroyWindow) {
		ri->WIN_Shutdown();
	}

	tr.registered = qfalse;
	backEndData = NULL;
}


/*
=============
RE_EndRegistration

Touch all images to make sure they are resident
=============
*/
void RE_EndRegistration( void ) {
	R_IssuePendingRenderCommands();
	if (!ri->Sys_LowPhysicalMemory()) {
		RB_ShowImages();
	}
}

// HACK
extern qboolean gG2_GBMNoReconstruct;
extern qboolean gG2_GBMUseSPMethod;
static void G2API_BoltMatrixReconstruction( qboolean reconstruct ) { gG2_GBMNoReconstruct = (qboolean)!reconstruct; }
static void G2API_BoltMatrixSPMethod( qboolean spMethod ) { gG2_GBMUseSPMethod = spMethod; }

static float GetDistanceCull( void ) { return tr.distanceCull; }

extern void R_SVModelInit( void ); //tr_model.cpp

static void GetRealRes( int *w, int *h ) {
	*w = glConfig.vidWidth * r_superSampleMultiplier->value;
	*h = glConfig.vidHeight * r_superSampleMultiplier->value;
}

// STUBS, REPLACEME
qboolean stub_InitializeWireframeAutomap() { return qtrue; }

void RE_GetLightStyle(int style, color4ub_t color)
{
	if (style >= MAX_LIGHT_STYLES)
	{
	    Com_Error( ERR_FATAL, "RE_GetLightStyle: %d is out of range", style );
		return;
	}

	*(int *)color = *(int *)styleColors[style];
}

void RE_SetLightStyle(int style, int color)
{
	if (style >= MAX_LIGHT_STYLES)
	{
	    Com_Error( ERR_FATAL, "RE_SetLightStyle: %d is out of range", style );
		return;
	}

	if (*(int*)styleColors[style] != color)
	{
		*(int *)styleColors[style] = color;
	}
}

void stub_RE_GetBModelVerts (int bModel, vec3_t *vec, float *normal) {}
void stub_RE_WorldEffectCommand ( const char *cmd ){}
void stub_RE_AddWeatherZone ( const vec3_t mins, const vec3_t maxs ) {}
static void RE_SetRefractionProperties ( float distortionAlpha, float distortionStretch, qboolean distortionPrePost, qboolean distortionNegate ) { }

#ifdef __SURFACESPRITES__
extern void RE_WorldEffectCommand(const char *command);
extern void RE_AddWeatherZone(vec3_t mins, vec3_t maxs);
#endif //__SURFACESPRITES__

/*
@@@@@@@@@@@@@@@@@@@@@
GetRefAPI

@@@@@@@@@@@@@@@@@@@@@
*/
extern "C" {
Q_EXPORT refexport_t* QDECL GetRefAPI ( int apiVersion, refimport_t *rimp ) {
	static refexport_t	re;

	assert( rimp );
	ri = rimp;

	Com_Memset( &re, 0, sizeof( re ) );

	if ( apiVersion != REF_API_VERSION ) {
		ri->Printf(PRINT_ALL, "Mismatched REF_API_VERSION: expected %i, got %i\n",
			REF_API_VERSION, apiVersion );
		return NULL;
	}

	// the RE_ functions are Renderer Entry points

	re.Shutdown = RE_Shutdown;

	re.BeginRegistration = RE_BeginRegistration;
	re.RegisterModel = RE_RegisterModel;
	re.RegisterServerModel = RE_RegisterServerModel;
	re.RegisterSkin = RE_RegisterSkin;
	re.RegisterServerSkin = RE_RegisterServerSkin;
	re.RegisterShader = RE_RegisterShader;
	re.RegisterShaderNoMip = RE_RegisterShaderNoMip;
	re.ShaderNameFromIndex = RE_ShaderNameFromIndex;
	re.LoadWorld = RE_LoadWorldMap;
	re.SetWorldVisData = RE_SetWorldVisData;
	re.EndRegistration = RE_EndRegistration;

	re.BeginFrame = RE_BeginFrame;
	re.EndFrame = RE_EndFrame;

	re.MarkFragments = R_MarkFragments;
	re.LerpTag = R_LerpTag;
	re.ModelBounds = R_ModelBounds;

	re.DrawRotatePic = RE_RotatePic;
	re.DrawRotatePic2 = RE_RotatePic2;

	re.ClearScene = RE_ClearScene;
	re.ClearDecals = RE_ClearDecals;
	re.AddRefEntityToScene = RE_AddRefEntityToScene;
	re.AddMiniRefEntityToScene = RE_AddMiniRefEntityToScene;
	re.AddPolyToScene = RE_AddPolyToScene;
	re.AddDecalToScene = RE_AddDecalToScene;
	re.LightForPoint = R_LightForPoint;
	re.AddLightToScene = RE_AddLightToScene;
	re.AddAdditiveLightToScene = RE_AddAdditiveLightToScene;
	re.RenderScene = RE_RenderScene;

	re.SetColor = RE_SetColor;
	re.DrawStretchPic = RE_StretchPic;
	re.DrawStretchRaw = RE_StretchRaw;
	re.UploadCinematic = RE_UploadCinematic;

	re.RegisterFont = RE_RegisterFont;
	re.Font_StrLenPixels = RE_Font_StrLenPixels;
	re.Font_StrLenChars = RE_Font_StrLenChars;
	re.Font_HeightPixels = RE_Font_HeightPixels;
	re.Font_DrawString = RE_Font_DrawString;
	re.Language_IsAsian = Language_IsAsian;
	re.Language_UsesSpaces = Language_UsesSpaces;
	re.AnyLanguage_ReadCharFromString = AnyLanguage_ReadCharFromString;
	re.RemapShader = R_RemapShader;
	re.GetEntityToken = R_GetEntityToken;
	re.inPVS = R_inPVS;

	re.GetLightStyle = RE_GetLightStyle;
	re.SetLightStyle = RE_SetLightStyle;
	re.GetBModelVerts = stub_RE_GetBModelVerts;

	re.SetRangedFog = RE_SetRangedFog;
	re.SetRefractionProperties = RE_SetRefractionProperties;
	re.GetDistanceCull = GetDistanceCull;
	re.GetRealRes = GetRealRes;
	// R_AutomapElevationAdjustment
	re.InitializeWireframeAutomap = stub_InitializeWireframeAutomap;
#ifdef __SURFACESPRITES__
	re.AddWeatherZone = RE_AddWeatherZone;//stub_RE_AddWeatherZone;
	re.WorldEffectCommand = RE_WorldEffectCommand;//stub_RE_WorldEffectCommand;
#else //!__SURFACESPRITES__
	re.AddWeatherZone = stub_RE_AddWeatherZone;
	re.WorldEffectCommand = stub_RE_WorldEffectCommand;
#endif //__SURFACESPRITES__
	re.RegisterMedia_LevelLoadBegin = C_LevelLoadBegin;
	re.RegisterMedia_LevelLoadEnd = C_LevelLoadEnd;
	re.RegisterMedia_GetLevel = C_GetLevel;
	re.RegisterImages_LevelLoadEnd = C_Images_LevelLoadEnd;
	re.RegisterModels_LevelLoadEnd = C_Models_LevelLoadEnd;

	re.TakeVideoFrame = RE_TakeVideoFrame;

	re.InitSkins							= R_InitSkins;
	re.InitShaders							= R_InitShaders;
	re.SVModelInit							= R_SVModelInit;
	re.HunkClearCrap						= RE_HunkClearCrap;

	re.G2API_AddBolt						= G2API_AddBolt;
	re.G2API_AddBoltSurfNum					= G2API_AddBoltSurfNum;
	re.G2API_AddSurface						= G2API_AddSurface;
	re.G2API_AnimateG2ModelsRag				= G2API_AnimateG2ModelsRag;
	re.G2API_AttachEnt						= G2API_AttachEnt;
	re.G2API_AttachG2Model					= G2API_AttachG2Model;
	re.G2API_AttachInstanceToEntNum			= G2API_AttachInstanceToEntNum;
	re.G2API_AbsurdSmoothing				= G2API_AbsurdSmoothing;
	re.G2API_BoltMatrixReconstruction		= G2API_BoltMatrixReconstruction;
	re.G2API_BoltMatrixSPMethod				= G2API_BoltMatrixSPMethod;
	re.G2API_CleanEntAttachments			= G2API_CleanEntAttachments;
	re.G2API_CleanGhoul2Models				= G2API_CleanGhoul2Models;
	re.G2API_ClearAttachedInstance			= G2API_ClearAttachedInstance;
	re.G2API_CollisionDetect				= G2API_CollisionDetect;
	re.G2API_CollisionDetectCache			= G2API_CollisionDetectCache;
	re.G2API_CopyGhoul2Instance				= G2API_CopyGhoul2Instance;
	re.G2API_CopySpecificG2Model			= G2API_CopySpecificG2Model;
	re.G2API_DetachG2Model					= G2API_DetachG2Model;
	re.G2API_DoesBoneExist					= G2API_DoesBoneExist;
	re.G2API_DuplicateGhoul2Instance		= G2API_DuplicateGhoul2Instance;
	re.G2API_FreeSaveBuffer					= G2API_FreeSaveBuffer;
	re.G2API_GetAnimFileName				= G2API_GetAnimFileName;
	re.G2API_GetAnimFileNameIndex			= G2API_GetAnimFileNameIndex;
	re.G2API_GetAnimRange					= G2API_GetAnimRange;
	re.G2API_GetBoltMatrix					= G2API_GetBoltMatrix;
	re.G2API_GetBoneAnim					= G2API_GetBoneAnim;
	re.G2API_GetBoneIndex					= G2API_GetBoneIndex;
	re.G2API_GetGhoul2ModelFlags			= G2API_GetGhoul2ModelFlags;
	re.G2API_GetGLAName						= G2API_GetGLAName;
	re.G2API_GetModelName					= G2API_GetModelName;
	re.G2API_GetParentSurface				= G2API_GetParentSurface;
	re.G2API_GetRagBonePos					= G2API_GetRagBonePos;
	re.G2API_GetSurfaceIndex				= G2API_GetSurfaceIndex;
	re.G2API_GetSurfaceName					= G2API_GetSurfaceName;
	re.G2API_GetSurfaceOnOff				= G2API_GetSurfaceOnOff;
	re.G2API_GetSurfaceRenderStatus			= G2API_GetSurfaceRenderStatus;
	re.G2API_GetTime						= G2API_GetTime;
	re.G2API_Ghoul2Size						= G2API_Ghoul2Size;
	re.G2API_GiveMeVectorFromMatrix			= G2API_GiveMeVectorFromMatrix;
	re.G2API_HasGhoul2ModelOnIndex			= G2API_HasGhoul2ModelOnIndex;
	re.G2API_HaveWeGhoul2Models				= G2API_HaveWeGhoul2Models;
	re.G2API_IKMove							= G2API_IKMove;
	re.G2API_InitGhoul2Model				= G2API_InitGhoul2Model;
	re.G2API_IsGhoul2InfovValid				= G2API_IsGhoul2InfovValid;
	re.G2API_IsPaused						= G2API_IsPaused;
	re.G2API_ListBones						= G2API_ListBones;
	re.G2API_ListSurfaces					= G2API_ListSurfaces;
	re.G2API_LoadGhoul2Models				= G2API_LoadGhoul2Models;
	re.G2API_LoadSaveCodeDestructGhoul2Info	= G2API_LoadSaveCodeDestructGhoul2Info;
	re.G2API_OverrideServerWithClientData	= G2API_OverrideServerWithClientData;
	re.G2API_PauseBoneAnim					= G2API_PauseBoneAnim;
	re.G2API_PrecacheGhoul2Model			= G2API_PrecacheGhoul2Model;
	re.G2API_RagEffectorGoal				= G2API_RagEffectorGoal;
	re.G2API_RagEffectorKick				= G2API_RagEffectorKick;
	re.G2API_RagForceSolve					= G2API_RagForceSolve;
	re.G2API_RagPCJConstraint				= G2API_RagPCJConstraint;
	re.G2API_RagPCJGradientSpeed			= G2API_RagPCJGradientSpeed;
	re.G2API_RemoveBolt						= G2API_RemoveBolt;
	re.G2API_RemoveBone						= G2API_RemoveBone;
	re.G2API_RemoveGhoul2Model				= G2API_RemoveGhoul2Model;
	re.G2API_RemoveGhoul2Models				= G2API_RemoveGhoul2Models;
	re.G2API_RemoveSurface					= G2API_RemoveSurface;
	re.G2API_ResetRagDoll					= G2API_ResetRagDoll;
	re.G2API_SaveGhoul2Models				= G2API_SaveGhoul2Models;
	re.G2API_SetBoltInfo					= G2API_SetBoltInfo;
	re.G2API_SetBoneAngles					= G2API_SetBoneAngles;
	re.G2API_SetBoneAnglesIndex				= G2API_SetBoneAnglesIndex;
	re.G2API_SetBoneAnglesMatrix			= G2API_SetBoneAnglesMatrix;
	re.G2API_SetBoneAnglesMatrixIndex		= G2API_SetBoneAnglesMatrixIndex;
	re.G2API_SetBoneAnim					= G2API_SetBoneAnim;
	re.G2API_SetBoneAnimIndex				= G2API_SetBoneAnimIndex;
	re.G2API_SetBoneIKState					= G2API_SetBoneIKState;
	re.G2API_SetGhoul2ModelIndexes			= G2API_SetGhoul2ModelIndexes;
	re.G2API_SetGhoul2ModelFlags			= G2API_SetGhoul2ModelFlags;
	re.G2API_SetLodBias						= G2API_SetLodBias;
	re.G2API_SetNewOrigin					= G2API_SetNewOrigin;
	re.G2API_SetRagDoll						= G2API_SetRagDoll;
	re.G2API_SetRootSurface					= G2API_SetRootSurface;
	re.G2API_SetShader						= G2API_SetShader;
	re.G2API_SetSkin						= G2API_SetSkin;
	re.G2API_SetSurfaceOnOff				= G2API_SetSurfaceOnOff;
	re.G2API_SetTime						= G2API_SetTime;
	re.G2API_SkinlessModel					= G2API_SkinlessModel;
	re.G2API_StopBoneAngles					= G2API_StopBoneAngles;
	re.G2API_StopBoneAnglesIndex			= G2API_StopBoneAnglesIndex;
	re.G2API_StopBoneAnim					= G2API_StopBoneAnim;
	re.G2API_StopBoneAnimIndex				= G2API_StopBoneAnimIndex;

	#ifdef _G2_GORE
	re.G2API_GetNumGoreMarks				= G2API_GetNumGoreMarks;
	re.G2API_AddSkinGore					= G2API_AddSkinGore;
	re.G2API_ClearSkinGore					= G2API_ClearSkinGore;
	#endif // _SOF2

	/*
	Ghoul2 Insert End
	*/

	// Jedi Knight Galaxies
	re.OverrideShaderFrame = R_OverrideShaderFrame;

	return &re;
}
}