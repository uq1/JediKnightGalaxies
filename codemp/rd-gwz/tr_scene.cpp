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

int			r_firstSceneDrawSurf;

int			r_numdlights;
int			r_firstSceneDlight;

int64_t		r_numentities;
int			r_firstSceneEntity;

int			r_numpolys;
int			r_firstScenePoly;

int			r_numpolyverts;


extern qboolean MATRIX_UPDATE;
extern qboolean CLOSE_LIGHTS_UPDATE;

extern void RB_UpdateMatrixes(void);


/*
====================
R_InitNextFrame

====================
*/
void R_InitNextFrame(void) {
	backEndData->commands.used = 0;

	r_firstSceneDrawSurf = 0;

	r_numdlights = 0;
	r_firstSceneDlight = 0;

	r_numentities = 0;
	r_firstSceneEntity = 0;

	r_numpolys = 0;
	r_firstScenePoly = 0;

	r_numpolyverts = 0;
}


/*
====================
RE_ClearScene

====================
*/
void RE_ClearScene(void) {
	r_firstSceneDlight = r_numdlights;
	r_firstSceneEntity = r_numentities;
	r_firstScenePoly = r_numpolys;
}

/*
===========================================================================

DISCRETE POLYS

===========================================================================
*/

/*
=====================
R_AddPolygonSurfaces

Adds all the scene's polys into this view's drawsurf list
=====================
*/
void R_AddPolygonSurfaces(void) {
	int			i;
	shader_t	*sh;
	srfPoly_t	*poly;
	int		fogMask;

	tr.currentEntityNum = REFENTITYNUM_WORLD;
	tr.shiftedEntityNum = tr.currentEntityNum << QSORT_REFENTITYNUM_SHIFT;
	fogMask = !((tr.refdef.rdflags & RDF_NOFOG) == 0);

	poly = tr.refdef.polys;

	for (i = 0; i < tr.refdef.numPolys; i++)
	{
		if (poly)
		{
			sh = R_GetShaderByHandle(poly->hShader);
			R_AddDrawSurf((surfaceType_t *)poly, sh, poly->fogIndex & fogMask, qfalse, R_IsPostRenderEntity(tr.currentEntityNum, tr.currentEntity), 0 /* cubemapIndex */);
		}

		poly++;
	}
}

/*
=====================
RE_AddPolyToScene

=====================
*/
void RE_AddPolyToScene(qhandle_t hShader, int numVerts, const polyVert_t *verts, int numPolys) {
	srfPoly_t	*poly;
	int			i, j;
	int			fogIndex;
	fog_t		*fog;
	vec3_t		bounds[2];

	if (!tr.registered) {
		return;
	}

	if (!hShader) {
		// This isn't a useful warning, and an hShader of zero isn't a null shader, it's
		// the default shader.
		//ri->Printf( PRINT_WARNING, "WARNING: RE_AddPolyToScene: NULL poly shader\n");
		//return;
	}

	for (j = 0; j < numPolys; j++) {
		if (r_numpolyverts + numVerts > max_polyverts || r_numpolys >= max_polys) {
			/*
			NOTE TTimo this was initially a PRINT_WARNING
			but it happens a lot with high fighting scenes and particles
			since we don't plan on changing the const and making for room for those effects
			simply cut this message to developer only
			*/
			ri->Printf(PRINT_DEVELOPER, "WARNING: RE_AddPolyToScene: r_max_polys or r_max_polyverts reached\n");
			return;
		}

		poly = &backEndData->polys[r_numpolys];
		poly->surfaceType = SF_POLY;
		poly->hShader = hShader;
		poly->numVerts = numVerts;
		poly->verts = &backEndData->polyVerts[r_numpolyverts];

		Com_Memcpy(poly->verts, &verts[numVerts*j], numVerts * sizeof(*verts));

		// done.
		r_numpolys++;
		r_numpolyverts += numVerts;

		// if no world is loaded
		if (tr.world == NULL) {
			fogIndex = 0;
		}
		// see if it is in a fog volume
		else if (tr.world->numfogs == 1) {
			fogIndex = 0;
		}
		else {
			// find which fog volume the poly is in
			VectorCopy(poly->verts[0].xyz, bounds[0]);
			VectorCopy(poly->verts[0].xyz, bounds[1]);
			for (i = 1; i < poly->numVerts; i++) {
				AddPointToBounds(poly->verts[i].xyz, bounds[0], bounds[1]);
			}
			for (fogIndex = 1; fogIndex < tr.world->numfogs; fogIndex++) {
				fog = &tr.world->fogs[fogIndex];
				if (bounds[1][0] >= fog->bounds[0][0]
					&& bounds[1][1] >= fog->bounds[0][1]
					&& bounds[1][2] >= fog->bounds[0][2]
					&& bounds[0][0] <= fog->bounds[1][0]
					&& bounds[0][1] <= fog->bounds[1][1]
					&& bounds[0][2] <= fog->bounds[1][2]) {
					break;
				}
			}
			if (fogIndex == tr.world->numfogs) {
				fogIndex = 0;
			}
		}
		poly->fogIndex = fogIndex;
	}
}


//=================================================================================

//#define __PVS_CULL__ // UQ1: Testing...

/*
=====================
RE_AddRefEntityToScene

=====================
*/
void RE_AddRefEntityToScene(const refEntity_t *ent) {
	vec3_t cross;

	if (!tr.registered) {
		return;
	}
	if (r_numentities >= MAX_REFENTITIES) {
		ri->Printf(PRINT_DEVELOPER, "RE_AddRefEntityToScene: Dropping refEntity, reached MAX_REFENTITIES\n");
		return;
	}
	if (Q_isnan(ent->origin[0]) || Q_isnan(ent->origin[1]) || Q_isnan(ent->origin[2])) {
		static qboolean firstTime = qtrue;
		if (firstTime) {
			firstTime = qfalse;
			ri->Printf(PRINT_WARNING, "RE_AddRefEntityToScene passed a refEntity which has an origin with a NaN component\n");
		}
		return;
	}
	if ((int)ent->reType < 0 || ent->reType >= RT_MAX_REF_ENTITY_TYPE) {
		ri->Error(ERR_DROP, "RE_AddRefEntityToScene: bad reType %i", ent->reType);
	}

#ifdef __PVS_CULL__
	{
		byte mask;

		if (!ent->ignoreCull && !R_inPVS( tr.refdef.vieworg, ent->origin, &mask )) {
			return;
		}
	}
#endif //__PVS_CULL__

	backEndData->entities[r_numentities].e = *ent;
	backEndData->entities[r_numentities].lightingCalculated = qfalse;

	CrossProduct(ent->axis[0], ent->axis[1], cross);
	backEndData->entities[r_numentities].mirrored = (qboolean)(DotProduct(ent->axis[2], cross) < 0.f);

	r_numentities++;
}

/*
=====================
RE_AddMiniRefEntityToScene

1:1 with how vanilla does it --eez
=====================
*/
void RE_AddMiniRefEntityToScene(const miniRefEntity_t *miniRefEnt) {
	refEntity_t entity;
	if (!tr.registered)
		return;
	if (!miniRefEnt)
		return;

#ifdef __PVS_CULL__
	{
		byte mask;

		if (!R_inPVS( tr.refdef.vieworg, miniRefEnt->origin, &mask )) {
			//PVS_CULL_COUNT++;
			return;
		}
	}
#endif //__PVS_CULL__

	memset(&entity, 0, sizeof(entity));
	memcpy(&entity, miniRefEnt, sizeof(*miniRefEnt));
	RE_AddRefEntityToScene(&entity);
}


/*
=====================
RE_AddDynamicLightToScene

=====================
*/
void RE_AddDynamicLightToScene(const vec3_t org, float intensity, float r, float g, float b, int additive, qboolean isGlowBased) {
	dlight_t	*dl;

	if (!tr.registered) {
		return;
	}
	if (r_numdlights >= MAX_DLIGHTS) {
		return;
	}
	/*if ( intensity <= 0 ) {
		return;
		}*/ // UQ1: negative now means volumetric
	//assert(0);
	dl = &backEndData->dlights[r_numdlights++];
	VectorCopy(org, dl->origin);
	dl->radius = intensity;
	dl->color[0] = r;
	dl->color[1] = g;
	dl->color[2] = b;
	dl->additive = additive;
	dl->isGlowBased = isGlowBased;
}

/*
=====================
RE_AddLightToScene

=====================
*/
void RE_AddLightToScene(const vec3_t org, float intensity, float r, float g, float b) {
	RE_AddDynamicLightToScene(org, intensity, r, g, b, qfalse, qfalse);
}

/*
=====================
RE_AddAdditiveLightToScene

=====================
*/
void RE_AddAdditiveLightToScene(const vec3_t org, float intensity, float r, float g, float b) {
	RE_AddDynamicLightToScene(org, intensity, r, g, b, qtrue, qfalse);
}

#ifdef __DAY_NIGHT__
int DAY_NIGHT_UPDATE_TIME = 0;

float DAY_NIGHT_SUN_DIRECTION = 0.0;
float DAY_NIGHT_CURRENT_TIME = 0.0;
float DAY_NIGHT_AMBIENT_SCALE = 0.0;
vec4_t DAY_NIGHT_AMBIENT_COLOR_ORIGINAL;
vec4_t DAY_NIGHT_AMBIENT_COLOR_CURRENT;

void RB_UpdateDayNightCycle()
{
	int nowTime = ri->Milliseconds();

	if (DAY_NIGHT_UPDATE_TIME == 0)
	{// Init stuff...
		VectorCopy4(tr.refdef.sunAmbCol, DAY_NIGHT_AMBIENT_COLOR_ORIGINAL);
		DAY_NIGHT_CURRENT_TIME = 9.0 / 24.0; // Start at 9am...
	}

	if (DAY_NIGHT_UPDATE_TIME < nowTime)
	{
		vec4_t sunColor;
		float Time24h = DAY_NIGHT_CURRENT_TIME*24.0;

		DAY_NIGHT_CURRENT_TIME += r_dayNightCycleSpeed->value;

		if (Time24h > 24.0)
		{
			DAY_NIGHT_CURRENT_TIME = 0.0;
			Time24h = 0.0;
		}


		// We need to match up real world 24 type time to sun direction... Offset...
		float adjustedTime24h = Time24h + 4.0;
		if (adjustedTime24h > 24.0) adjustedTime24h = adjustedTime24h - 24.0;

		DAY_NIGHT_SUN_DIRECTION = ((adjustedTime24h / 24.0) - 0.5) * 6.283185307179586476925286766559;


		if (Time24h < 6.0 || Time24h > 22.0)
		{// Night time...
			VectorSet4(sunColor, 0.0, 0.0, 0.0, 0.0);
			sunColor[0] *= DAY_NIGHT_AMBIENT_COLOR_ORIGINAL[0];
			sunColor[1] *= DAY_NIGHT_AMBIENT_COLOR_ORIGINAL[1];
			sunColor[2] *= DAY_NIGHT_AMBIENT_COLOR_ORIGINAL[2];
			sunColor[3] = 1.0;
		}
		else
		{// Day time...
			if (Time24h < 8.0)
			{// Morning color... More red/yellow...
				DAY_NIGHT_AMBIENT_SCALE = 8.0 - Time24h;
				VectorSet4(sunColor, 1.0, (0.5 - (DAY_NIGHT_AMBIENT_SCALE * 0.5)) + 0.5, 1.0 - DAY_NIGHT_AMBIENT_SCALE, 1.0);
				sunColor[0] *= DAY_NIGHT_AMBIENT_COLOR_ORIGINAL[0];
				sunColor[1] *= DAY_NIGHT_AMBIENT_COLOR_ORIGINAL[1];
				sunColor[2] *= DAY_NIGHT_AMBIENT_COLOR_ORIGINAL[2];
				sunColor[3] = 1.0;
			}
			else if (Time24h > 18.0)
			{// Evening color... More red/yellow...
				DAY_NIGHT_AMBIENT_SCALE = Time24h - 18.0;
				VectorSet4(sunColor, 1.0, (0.5 - (DAY_NIGHT_AMBIENT_SCALE * 0.5)) + 0.5, 1.0 - DAY_NIGHT_AMBIENT_SCALE, 1.0);
				sunColor[0] *= DAY_NIGHT_AMBIENT_COLOR_ORIGINAL[0];
				sunColor[1] *= DAY_NIGHT_AMBIENT_COLOR_ORIGINAL[1];
				sunColor[2] *= DAY_NIGHT_AMBIENT_COLOR_ORIGINAL[2];
				sunColor[3] = 1.0;
			}
			else
			{// Full bright - day...
				VectorCopy4(DAY_NIGHT_AMBIENT_COLOR_ORIGINAL, sunColor);
				sunColor[3] = 1.0;
			}
		}

		VectorCopy4(sunColor, DAY_NIGHT_AMBIENT_COLOR_CURRENT);

		//ri->Printf(PRINT_WARNING, "Day/Night timer is %f. Sun dir %f.\n", Time24h, DAY_NIGHT_SUN_DIRECTION);

		DAY_NIGHT_UPDATE_TIME = nowTime + 50;
	}

	VectorCopy4(DAY_NIGHT_AMBIENT_COLOR_CURRENT, tr.refdef.sunAmbCol);
	VectorCopy4(tr.refdef.sunAmbCol, tr.refdef.sunCol);

	float a = 0.3;
	float b = DAY_NIGHT_SUN_DIRECTION;
	tr.sunDirection[0] = cos(a) * cos(b);
	tr.sunDirection[1] = sin(a) * cos(b);
	tr.sunDirection[2] = sin(b);
}

extern void R_LocalPointToWorld(const vec3_t local, vec3_t world);
extern void R_WorldToLocal(const vec3_t world, vec3_t local);
#endif //__DAY_NIGHT__

extern void RB_AddGlowShaderLights(void);
extern void RB_UpdateCloseLights();

void RE_BeginScene(const refdef_t *fd)
{
	Com_Memcpy(tr.refdef.text, fd->text, sizeof(tr.refdef.text));

	tr.refdef.x = fd->x;
	tr.refdef.y = fd->y;
	tr.refdef.width = fd->width;
	tr.refdef.height = fd->height;
	tr.refdef.fov_x = fd->fov_x;
	tr.refdef.fov_y = fd->fov_y;

	VectorCopy(fd->vieworg, tr.refdef.vieworg);
	VectorCopy(fd->viewaxis[0], tr.refdef.viewaxis[0]);
	VectorCopy(fd->viewaxis[1], tr.refdef.viewaxis[1]);
	VectorCopy(fd->viewaxis[2], tr.refdef.viewaxis[2]);

	tr.refdef.time = fd->time;
	tr.refdef.rdflags = fd->rdflags;

	// copy the areamask data over and note if it has changed, which
	// will force a reset of the visible leafs even if the view hasn't moved
	tr.refdef.areamaskModified = qfalse;
	if (!(tr.refdef.rdflags & RDF_NOWORLDMODEL)) {
		int		areaDiff;
		int		i;

		// compare the area bits
		areaDiff = 0;
		for (i = 0; i < MAX_MAP_AREA_BYTES / 4; i++) {
			areaDiff |= ((int *)tr.refdef.areamask)[i] ^ ((int *)fd->areamask)[i];
			((int *)tr.refdef.areamask)[i] = ((int *)fd->areamask)[i];
		}

		if (areaDiff) {
			// a door just opened or something
			tr.refdef.areamaskModified = qtrue;
		}
	}

	tr.refdef.sunDir[3] = 0.0f;
	tr.refdef.sunCol[3] = 1.0f;
	tr.refdef.sunAmbCol[3] = 1.0f;

	VectorNormalize(tr.sunDirection);

	VectorCopy(tr.sunDirection, tr.refdef.sunDir);

	if ((tr.refdef.rdflags & RDF_NOWORLDMODEL) || !(r_depthPrepass->value)){
		tr.refdef.colorScale = 1.0f;
		VectorSet(tr.refdef.sunCol, 0, 0, 0);
		VectorSet(tr.refdef.sunAmbCol, 0, 0, 0);
	}
	else
	{
		float colorScale = 1.0;

		if (r_forceSun->integer)
			colorScale = (r_forceSun->integer) ? r_forceSunMapLightScale->value : tr.mapLightScale;
		else if (r_sunlightMode->integer >= 2)
			colorScale = 0.75;

		tr.refdef.colorScale = colorScale;

		if (r_sunlightMode->integer >= 1)
		{
			float ambCol = 1.0;

			if (r_forceSun->integer || r_dlightShadows->integer)
				ambCol = (r_forceSun->integer) ? r_forceSunAmbientScale->value : tr.sunShadowScale;
			else if (r_sunlightMode->integer >= 2)
				ambCol = 0.75;

#ifndef __DAY_NIGHT__
			tr.refdef.sunCol[0] =
				tr.refdef.sunCol[1] =
				tr.refdef.sunCol[2] = 1.0f;

			tr.refdef.sunAmbCol[0] =
				tr.refdef.sunAmbCol[1] =
				tr.refdef.sunAmbCol[2] = ambCol;
#else //__DAY_NIGHT__
			if (DAY_NIGHT_CYCLE_ENABLED)
			{
				/*
				tr.refdef.sunAmbCol[0] *= ambCol;
				tr.refdef.sunAmbCol[1] *= ambCol;
				tr.refdef.sunAmbCol[2] *= ambCol;
				*/

				RB_UpdateDayNightCycle();
				VectorNormalize(tr.sunDirection);
				VectorCopy(tr.sunDirection, tr.refdef.sunDir);

				tr.refdef.sunCol[0] =
					tr.refdef.sunCol[1] =
					tr.refdef.sunCol[2] = 1.0f;

				tr.refdef.sunAmbCol[0] =
					tr.refdef.sunAmbCol[1] =
					tr.refdef.sunAmbCol[2] = ambCol;
			}
			else
			{
				tr.refdef.sunCol[0] =
					tr.refdef.sunCol[1] =
					tr.refdef.sunCol[2] = 1.0f;

				tr.refdef.sunAmbCol[0] =
					tr.refdef.sunAmbCol[1] =
					tr.refdef.sunAmbCol[2] = ambCol;
			}
#endif //__DAY_NIGHT__
		}
		else
		{
			float scale = pow(2.0f, r_mapOverBrightBits->integer - tr.overbrightBits - 8);
			if (r_sunlightMode->integer/*r_forceSun->integer || r_dlightShadows->integer*/)
			{
				VectorScale(tr.sunLight, scale * r_forceSunLightScale->value, tr.refdef.sunCol);
				VectorScale(tr.sunLight, scale * r_forceSunAmbientScale->value, tr.refdef.sunAmbCol);
			}
			else
			{
				VectorScale(tr.sunLight, scale, tr.refdef.sunCol);
				VectorScale(tr.sunLight, scale * tr.sunShadowScale, tr.refdef.sunAmbCol);
			}
		}
	}

	if (r_forceAutoExposure->integer)
	{
		tr.refdef.autoExposureMinMax[0] = r_forceAutoExposureMin->value;
		tr.refdef.autoExposureMinMax[1] = r_forceAutoExposureMax->value;
	}
	else
	{
		tr.refdef.autoExposureMinMax[0] = tr.autoExposureMinMax[0];
		tr.refdef.autoExposureMinMax[1] = tr.autoExposureMinMax[1];
	}

	if (r_forceToneMap->integer)
	{
		tr.refdef.toneMinAvgMaxLinear[0] = pow(2, r_forceToneMapMin->value);
		tr.refdef.toneMinAvgMaxLinear[1] = pow(2, r_forceToneMapAvg->value);
		tr.refdef.toneMinAvgMaxLinear[2] = pow(2, r_forceToneMapMax->value);
	}
	else
	{
		tr.refdef.toneMinAvgMaxLinear[0] = pow(2, tr.toneMinAvgMaxLevel[0]);
		tr.refdef.toneMinAvgMaxLinear[1] = pow(2, tr.toneMinAvgMaxLevel[1]);
		tr.refdef.toneMinAvgMaxLinear[2] = pow(2, tr.toneMinAvgMaxLevel[2]);
	}

	// Makro - copy exta info if present
	if (fd->rdflags & RDF_EXTRA) {
		const refdefex_t* extra = (const refdefex_t*)(fd + 1);

		tr.refdef.blurFactor = extra->blurFactor;

		if (fd->rdflags & RDF_SUNLIGHT)
		{
			VectorCopy(extra->sunDir, tr.refdef.sunDir);
			VectorCopy(extra->sunCol, tr.refdef.sunCol);
			VectorCopy(extra->sunAmbCol, tr.refdef.sunAmbCol);
		}
	}
	else
	{
		tr.refdef.blurFactor = 0.0f;
	}

	// derived info

	tr.refdef.floatTime = tr.refdef.time * 0.001f;

	tr.refdef.numDrawSurfs = r_firstSceneDrawSurf;
	tr.refdef.drawSurfs = backEndData->drawSurfs;

	tr.refdef.num_entities = r_numentities - r_firstSceneEntity;
	tr.refdef.entities = &backEndData->entities[r_firstSceneEntity];

	tr.refdef.num_dlights = r_numdlights - r_firstSceneDlight;
	tr.refdef.dlights = &backEndData->dlights[r_firstSceneDlight];

	//#ifdef USING_ENGINE_GLOW_LIGHTCOLORS_SEARCH
	backEnd.refdef.dlights = tr.refdef.dlights;
	backEnd.refdef.num_dlights = tr.refdef.num_dlights;

	RB_AddGlowShaderLights();
	RB_UpdateCloseLights();
	//#endif //USING_ENGINE_GLOW_LIGHTCOLORS_SEARCH

	// Add the decals here because decals add polys and we need to ensure
	// that the polys are added before the the renderer is prepared
	if ( !(tr.refdef.rdflags & RDF_NOWORLDMODEL) )
		R_AddDecals();

	tr.refdef.numPolys = r_numpolys - r_firstScenePoly;
	tr.refdef.polys = &backEndData->polys[r_firstScenePoly];

#ifdef __PSHADOWS__
	tr.refdef.num_pshadows = 0;
	tr.refdef.pshadows = &backEndData->pshadows[0];
#endif

	// turn off dynamic lighting globally by clearing all the
	// dlights if it needs to be disabled or if vertex lighting is enabled
	if ((/*r_dynamiclight->integer == 0 ||*/ r_vertexLight->integer == 1)) {
		tr.refdef.num_dlights = 0;
	}

	// a single frame may have multiple scenes draw inside it --
	// a 3D game view, 3D status bar renderings, 3D menus, etc.
	// They need to be distinguished by the light flare code, because
	// the visibility state for a given surface may be different in
	// each scene / view.
	tr.frameSceneNum++;
	tr.sceneCount++;
}

void RE_EndScene()
{
	// the next scene rendered in this frame will tack on after this one
	r_firstSceneDrawSurf = tr.refdef.numDrawSurfs;
	r_firstSceneEntity = r_numentities;
	r_firstSceneDlight = r_numdlights;
	r_firstScenePoly = r_numpolys;

	qglClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	qglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	qglClearDepth(1.0f);
}

/*
@@@@@@@@@@@@@@@@@@@@@
RE_RenderScene

Draw a 3D view into a part of the window, then return
to 2D drawing.

Rendering a scene may require multiple views to be rendered
to handle mirrors,
@@@@@@@@@@@@@@@@@@@@@
*/

extern void RB_AdvanceOverlaySway(void);

int NEXT_SHADOWMAP_UPDATE[3] = { 0 };

void RE_RenderScene(const refdef_t *fd) {
	viewParms_t		parms;
	int				startTime;

	RB_AdvanceOverlaySway();

	if (!tr.registered) {
		return;
	}

	GLimp_LogComment("====== RE_RenderScene =====\n");

	if (r_norefresh->integer) {
		return;
	}

	startTime = ri->Milliseconds();

	if (!tr.world && !(fd->rdflags & RDF_NOWORLDMODEL)) {
		ri->Error(ERR_DROP, "R_RenderScene: NULL worldmodel");
	}

	MATRIX_UPDATE = qtrue;
	CLOSE_LIGHTS_UPDATE = qtrue;
	RE_BeginScene(fd);

	// SmileTheory: playing with shadow mapping
#if 0
	if (!( fd->rdflags & RDF_NOWORLDMODEL ) && tr.refdef.num_dlights && r_dlightMode->integer >= 2)
	{
		R_RenderDlightCubemaps(fd);
	}
#endif

#ifdef __PSHADOWS__
	/* playing with more shadows */
	if(!( fd->rdflags & RDF_NOWORLDMODEL ) && r_shadows->integer == 4)
	{
		R_RenderPshadowMaps(fd);
	}
#endif

#ifdef __DYNAMIC_SHADOWS__
	if (!(fd->rdflags & RDF_NOWORLDMODEL))
	{
		R_RenderDlightShadowMaps(fd, 0);
		R_RenderDlightShadowMaps(fd, 1);
		R_RenderDlightShadowMaps(fd, 2);
	}
#endif //__DYNAMIC_SHADOWS__

	// playing with even more shadows
	if (!(fd->rdflags & RDF_NOWORLDMODEL)
		&& (r_sunlightMode->integer >= 2 || r_forceSun->integer || tr.sunShadows)
		&& !backEnd.depthFill)
	{
		if (r_sunlightMode->integer == 2)
		{// Update distance shadows on timers...
			//int nowTime = ri->Milliseconds();

			//if (nowTime >= NEXT_SHADOWMAP_UPDATE[0])
			{// Close shadows - fast updates...
				//NEXT_SHADOWMAP_UPDATE[0] = nowTime + 20;
				R_RenderSunShadowMaps(fd, 0);
			}
		}
		else if (r_sunlightMode->integer == 3)
		{// Update distance shadows on timers...
			int nowTime = ri->Milliseconds();

			if (nowTime >= NEXT_SHADOWMAP_UPDATE[0])
			{// Close shadows - fast updates...
				NEXT_SHADOWMAP_UPDATE[0] = nowTime + 20;
				R_RenderSunShadowMaps(fd, 0);
			}
			else if (nowTime >= NEXT_SHADOWMAP_UPDATE[1])
			{// Distant shadows - slower updates...
				NEXT_SHADOWMAP_UPDATE[1] = nowTime + 2000;//500;
				R_RenderSunShadowMaps(fd, 1);
			}
		}
		else
		{
			R_RenderSunShadowMaps(fd, 0);
			R_RenderSunShadowMaps(fd, 1);
			R_RenderSunShadowMaps(fd, 2);
		}
	}

	// playing with cube maps
	// this is where dynamic cubemaps would be rendered
	if (0) //(!( fd->rdflags & RDF_NOWORLDMODEL ))
	{
		int i, j;

		for (i = 0; i < tr.numCubemaps; i++)
		{
			for (j = 0; j < 6; j++)
			{
				R_RenderCubemapSide(i, j, qtrue);
			}
		}
	}

	// setup view parms for the initial view
	//
	// set up viewport
	// The refdef takes 0-at-the-top y coordinates, so
	// convert to GL's 0-at-the-bottom space
	//
	Com_Memset(&parms, 0, sizeof(parms));
	parms.viewportX = tr.refdef.x;
	parms.viewportY = glConfig.vidHeight - (tr.refdef.y + tr.refdef.height);
	parms.viewportWidth = tr.refdef.width;
	parms.viewportHeight = tr.refdef.height;
	parms.isPortal = qfalse;

	parms.fovX = tr.refdef.fov_x;
	parms.fovY = tr.refdef.fov_y;

	parms.stereoFrame = tr.refdef.stereoFrame;

	VectorCopy(fd->vieworg, parms.ori.origin);
	VectorCopy(fd->viewaxis[0], parms.ori.axis[0]);
	VectorCopy(fd->viewaxis[1], parms.ori.axis[1]);
	VectorCopy(fd->viewaxis[2], parms.ori.axis[2]);

	VectorCopy(fd->vieworg, parms.pvsOrigin);

	if (!(fd->rdflags & RDF_NOWORLDMODEL)
		&& r_depthPrepass->value
		&& (r_sunlightMode->integer >= 2 /*|| r_sunlightSpecular->integer*/ || r_forceSun->integer || tr.sunShadows))
	{
		parms.flags = VPF_USESUNLIGHT;
	}

	parms.maxEntityRange = 512000;

	MATRIX_UPDATE = qtrue;
	CLOSE_LIGHTS_UPDATE = qtrue;
	R_RenderView( &parms );

	if(!( fd->rdflags & RDF_NOWORLDMODEL ))
		R_AddPostProcessCmd();

#if 0
	if (r_weather->integer)
	{
		extern void RE_RenderWorldEffects(void);
		RE_RenderWorldEffects();
	}
#endif

	RE_EndScene();

	tr.frontEndMsec += ri->Milliseconds() - startTime;
}
