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
#include "tr_occlusion.h"

int NUM_WORLD_FOV_CULLS = 0;
int NUM_WORLDMERGED_FOV_CULLS = 0;

extern bool TR_WorldToScreen(vec3_t worldCoord, float *x, float *y);
extern void TR_AxisToAngles(const vec3_t axis[3], vec3_t angles);

qboolean R_CULL_InFOV(vec3_t spot, vec3_t from)
{
	//if (tr.viewParms.flags & VPF_SHADOWPASS) return qtrue; // so object still can generate shadows...

	vec3_t	deltaVector, angles, deltaAngles;
	vec3_t	fromAnglesCopy;
	vec3_t	fromAngles;
	//int hFOV = tr.refdef.fov_x + 5.0;
	//int vFOV = tr.refdef.fov_y + 5.0;
	int hFOV = 100;//backEnd.refdef.fov_x + 5.0;//100;
	int vFOV = 140;//backEnd.refdef.fov_y + 5.0;//100;

	if (tr.viewParms.flags & VPF_SHADOWPASS || backEnd.depthFill)
		TR_AxisToAngles(tr.refdef.viewaxis, fromAngles);
	else
		TR_AxisToAngles(backEnd.refdef.viewaxis/*tr.refdef.viewaxis*/, fromAngles);

	VectorSubtract(spot, from, deltaVector);
	vectoangles(deltaVector, angles);
	VectorCopy(fromAngles, fromAnglesCopy);

	deltaAngles[PITCH] = AngleDelta(fromAnglesCopy[PITCH], angles[PITCH]);
	deltaAngles[YAW] = AngleDelta(fromAnglesCopy[YAW], angles[YAW]);

	if (fabs(deltaAngles[PITCH]) <= vFOV && fabs(deltaAngles[YAW]) <= hFOV)
	{
		return qtrue;
	}

	return qfalse;
}

qboolean R_Box_In_FOV(vec3_t mins, vec3_t maxs)
{
	vec3_t edge, edge2;

	VectorSet(edge, maxs[0], mins[1], maxs[2]);
	VectorSet(edge2, mins[0], maxs[1], maxs[2]);

	if (!R_CULL_InFOV(mins, tr.refdef.vieworg)
		&& !R_CULL_InFOV(maxs, tr.refdef.vieworg)
		&& !R_CULL_InFOV(edge, tr.refdef.vieworg)
		&& !R_CULL_InFOV(edge2, tr.refdef.vieworg))
		return qfalse;

	return qtrue;
}

static qboolean	R_FovCullSurface(msurface_t *surf)
{
	if (r_fovCull->integer && backEnd.viewParms.targetFbo != tr.renderCubeFbo /*&& backEnd.depthFill*/)
	{
#if 1
		vec3_t bounds[2];

		bounds[0][0] = surf->cullinfo.bounds[0][0];
		bounds[0][1] = surf->cullinfo.bounds[1][1];
		bounds[0][2] = surf->cullinfo.bounds[0][2];

		bounds[1][0] = surf->cullinfo.bounds[1][0];
		bounds[1][1] = surf->cullinfo.bounds[0][1];
		bounds[1][2] = surf->cullinfo.bounds[1][2];

		if (Distance(surf->cullinfo.bounds[0], tr.refdef.vieworg) < 256
			|| Distance(surf->cullinfo.bounds[1], tr.refdef.vieworg) < 256
			|| Distance(bounds[0], tr.refdef.vieworg) < 256
			|| Distance(bounds[1], tr.refdef.vieworg) < 256)
		{// Don't cull close stuff, ever...
			return qfalse;
		}

		/*if (!R_CULL_InFOV(surf->cullinfo.bounds[0], tr.refdef.vieworg)
			&& !R_CULL_InFOV(surf->cullinfo.bounds[1], tr.refdef.vieworg)
			&& !R_CULL_InFOV(bounds[0], tr.refdef.vieworg)
			&& !R_CULL_InFOV(bounds[1], tr.refdef.vieworg))
			{// No corner of this surface is on screen...
			NUM_WORLD_FOV_CULLS++;
			return qtrue;
			}*/

		if (!R_CULL_InFOV(surf->cullinfo.bounds[0], backEnd.refdef.vieworg)
			&& !R_CULL_InFOV(surf->cullinfo.bounds[1], backEnd.refdef.vieworg)
			&& !R_CULL_InFOV(bounds[0], backEnd.refdef.vieworg)
			&& !R_CULL_InFOV(bounds[1], backEnd.refdef.vieworg))
		{
			NUM_WORLD_FOV_CULLS++;
			return qtrue;
		}

		/*if (!R_inPVS( tr.refdef.vieworg, surf->cullinfo.bounds[0], tr.refdef.areamask )
			&& !R_inPVS( tr.refdef.vieworg, surf->cullinfo.bounds[1], tr.refdef.areamask )
			&& !R_inPVS( tr.refdef.vieworg, bounds[0], tr.refdef.areamask )
			&& !R_inPVS( tr.refdef.vieworg, bounds[1], tr.refdef.areamask ))
			{
			// Not in PVS? Cull the bitch!
			NUM_WORLD_FOV_CULLS++;
			return qtrue;
			}*/
#else
		if (!R_Box_In_FOV(surf->cullinfo.bounds[0], surf->cullinfo.bounds[1]))
			return qtrue;
#endif
	}

	return qfalse;
}

/*
================
R_CullSurface

Tries to cull surfaces before they are lighted or
added to the sorting list.
================
*/
static qboolean	R_CullSurface(msurface_t *surf) {
#if 0
	if ( r_nocull->integer || surf->cullinfo.type == CULLINFO_NONE) {
		return qfalse;
	}

	if ( *surf->data == SF_GRID && r_nocurves->integer ) {
		return qtrue;
	}

	if ( *surf->data != SF_FACE && *surf->data != SF_TRIANGLES && *surf->data != SF_POLY && *surf->data != SF_VBO_MESH && *surf->data != SF_GRID )
	{
		return qtrue;
	}

	// plane cull
	if ( *surf->data == SF_FACE && r_facePlaneCull->integer )
	{
		float d = DotProduct( tr.ori.viewOrigin, surf->cullinfo.plane.normal ) - surf->cullinfo.plane.dist;

		// shadowmaps draw back surfaces
		if ( tr.viewParms.flags & (VPF_SHADOWMAP | VPF_DEPTHSHADOW) )
		{
			if (surf->shader->cullType == CT_FRONT_SIDED)
			{
				surf->shader->cullType = CT_BACK_SIDED;
			}
			else
			{
				surf->shader->cullType = CT_FRONT_SIDED;
			}
		}

		// do proper cull for orthographic projection
		if (tr.viewParms.flags & VPF_ORTHOGRAPHIC) {
			d = DotProduct(tr.viewParms.ori.axis[0], surf->cullinfo.plane.normal);
			if ( surf->shader->cullType == CT_FRONT_SIDED ) {
				if (d > 0)
					return qtrue;
			} else {
				if (d < 0)
					return qtrue;
			}
			return qfalse;
		}

		// don't cull exactly on the plane, because there are levels of rounding
		// through the BSP, ICD, and hardware that may cause pixel gaps if an
		// epsilon isn't allowed here
		if ( surf->shader->cullType == CT_FRONT_SIDED )
		{
			if ( d < -8.0f )
			{
				return qtrue;
			}
		}
		else if ( surf->shader->cullType == CT_BACK_SIDED )
		{
			if ( d > 8.0f )
			{
				return qtrue;
			}
		}
		else if (surf->shader->cullType == CT_TWO_SIDED)
		{
			return qfalse;
		}
		else
		{
			return qfalse;
		}
	}

	if (surf->cullinfo.type & CULLINFO_SPHERE)
	{
		int 	sphereCull;

		if ( tr.currentEntityNum != REFENTITYNUM_WORLD ) {
			sphereCull = R_CullLocalPointAndRadius( surf->cullinfo.localOrigin, surf->cullinfo.radius );
		} else {
			sphereCull = R_CullPointAndRadius( surf->cullinfo.localOrigin, surf->cullinfo.radius );
		}

		if ( sphereCull == CULL_OUT )
		{
			return qtrue;
		}
	}

	if (surf->cullinfo.type & CULLINFO_BOX)
	{
		int boxCull;

		if ( tr.currentEntityNum != REFENTITYNUM_WORLD ) {
			boxCull = R_CullLocalBox( surf->cullinfo.bounds );
		} else {
			boxCull = R_CullBox( surf->cullinfo.bounds );
		}

		if ( boxCull == CULL_OUT )
		{
			return qtrue;
		}
	}

	return qfalse;
#else
	if (r_nocull->integer || surf->cullinfo.type == CULLINFO_NONE) {
		return qfalse;
	}

	if (*surf->data == SF_GRID && r_nocurves->integer) {
		return qtrue;
	}

	if (surf->cullinfo.type & CULLINFO_PLANE)
	{
		// Only true for SF_FACE, so treat like its own function
		float			d;
		cullType_t ct;

		if (!r_facePlaneCull->integer) {
			return qfalse;
		}

		ct = surf->shader->cullType;

		if (ct == CT_TWO_SIDED)
		{
			return qfalse;
		}

		// don't cull for depth shadow
		/*
		if ( tr.viewParms.flags & VPF_DEPTHSHADOW )
		{
		return qfalse;
		}
		*/

		// shadowmaps draw back surfaces
		if (tr.viewParms.flags & (VPF_SHADOWMAP | VPF_DEPTHSHADOW))
		{
			if (ct == CT_FRONT_SIDED)
			{
				ct = CT_BACK_SIDED;
			}
			else
			{
				ct = CT_FRONT_SIDED;
			}
		}

		// do proper cull for orthographic projection
		if (tr.viewParms.flags & VPF_ORTHOGRAPHIC) {
			d = DotProduct(tr.viewParms.ori.axis[0], surf->cullinfo.plane.normal);
			if (ct == CT_FRONT_SIDED) {
				if (d > 0)
					return qtrue;
			}
			else {
				if (d < 0)
					return qtrue;
			}
			return qfalse;
		}

		d = DotProduct(tr.ori.viewOrigin, surf->cullinfo.plane.normal);

		// don't cull exactly on the plane, because there are levels of rounding
		// through the BSP, ICD, and hardware that may cause pixel gaps if an
		// epsilon isn't allowed here
		if (ct == CT_FRONT_SIDED) {
			if (d < surf->cullinfo.plane.dist - 8) {
				return qtrue;
			}
		}
		else {
			if (d > surf->cullinfo.plane.dist + 8) {
				return qtrue;
			}
		}

		return qfalse;
	}

	if (surf->cullinfo.type & CULLINFO_SPHERE)
	{
		int 	sphereCull;

		if (tr.currentEntityNum != REFENTITYNUM_WORLD) {
			sphereCull = R_CullLocalPointAndRadius(surf->cullinfo.localOrigin, surf->cullinfo.radius);
		}
		else {
			sphereCull = R_CullPointAndRadius(surf->cullinfo.localOrigin, surf->cullinfo.radius);
		}

		if (sphereCull == CULL_OUT)
		{
			return qtrue;
		}
	}

	if (surf->cullinfo.type & CULLINFO_BOX)
	{
		int boxCull;

		if (tr.currentEntityNum != REFENTITYNUM_WORLD) {
			boxCull = R_CullLocalBox(surf->cullinfo.bounds);
		}
		else {
			boxCull = R_CullBox(surf->cullinfo.bounds);
		}

		if (boxCull == CULL_OUT)
		{
			return qtrue;
		}
	}

	return qfalse;
#endif
}

/*
====================
R_DlightSurface

The given surface is going to be drawn, and it touches a leaf
that is touched by one or more dlights, so try to throw out
more dlights if possible.
====================
*/
#if 0
static int R_DlightSurface( msurface_t *surf, int dlightBits ) {
	float       d;
	int         i;
	dlight_t    *dl;

	if ( surf->cullinfo.type & CULLINFO_PLANE )
	{
		for ( i = 0 ; i < tr.refdef.num_dlights ; i++ ) {
			if ( ! ( dlightBits & ( 1 << i ) ) ) {
				continue;
			}
			dl = &tr.refdef.dlights[i];
			d = DotProduct( dl->origin, surf->cullinfo.plane.normal ) - surf->cullinfo.plane.dist;
			if ( d < -dl->radius || d > dl->radius ) {
				// dlight doesn't reach the plane
				dlightBits &= ~( 1 << i );
			}
		}
	}

	if ( surf->cullinfo.type & CULLINFO_BOX )
	{
		for ( i = 0 ; i < tr.refdef.num_dlights ; i++ ) {
			if ( ! ( dlightBits & ( 1 << i ) ) ) {
				continue;
			}
			dl = &tr.refdef.dlights[i];
			if ( dl->origin[0] - dl->radius > surf->cullinfo.bounds[1][0]
				|| dl->origin[0] + dl->radius < surf->cullinfo.bounds[0][0]
				|| dl->origin[1] - dl->radius > surf->cullinfo.bounds[1][1]
				|| dl->origin[1] + dl->radius < surf->cullinfo.bounds[0][1]
				|| dl->origin[2] - dl->radius > surf->cullinfo.bounds[1][2]
				|| dl->origin[2] + dl->radius < surf->cullinfo.bounds[0][2] ) {
				// dlight doesn't reach the bounds
				dlightBits &= ~( 1 << i );
			}
		}
	}

	if ( surf->cullinfo.type & CULLINFO_SPHERE )
	{
		for ( i = 0 ; i < tr.refdef.num_dlights ; i++ ) {
			if ( ! ( dlightBits & ( 1 << i ) ) ) {
				continue;
			}
			dl = &tr.refdef.dlights[i];
			if (!SpheresIntersect(dl->origin, dl->radius, surf->cullinfo.localOrigin, surf->cullinfo.radius))
			{
				// dlight doesn't reach the bounds
				dlightBits &= ~( 1 << i );
			}
		}
	}

	switch(*surf->data)
	{
	case SF_FACE:
	case SF_GRID:
	case SF_TRIANGLES:
	case SF_VBO_MESH:
		((srfBspSurface_t *)surf->data)->dlightBits = dlightBits;
		break;

	default:
		dlightBits = 0;
		break;
	}

	if ( dlightBits ) {
		tr.pc.c_dlightSurfaces++;
	} else {
		tr.pc.c_dlightSurfacesCulled++;
	}

	return dlightBits;
}
#endif

/*
====================
R_PshadowSurface

Just like R_DlightSurface, cull any we can
====================
*/
static int R_PshadowSurface(msurface_t *surf, int pshadowBits) {
#ifdef __PSHADOWS__
	float       d;
	int         i;
	pshadow_t    *ps;

	if ( surf->cullinfo.type & CULLINFO_PLANE )
	{
		for ( i = 0 ; i < tr.refdef.num_pshadows ; i++ ) {
			if ( ! ( pshadowBits & ( 1 << i ) ) ) {
				continue;
			}
			ps = &tr.refdef.pshadows[i];
			d = DotProduct( ps->lightOrigin, surf->cullinfo.plane.normal ) - surf->cullinfo.plane.dist;
			if ( d < -ps->lightRadius || d > ps->lightRadius ) {
				// pshadow doesn't reach the plane
				pshadowBits &= ~( 1 << i );
			}
		}
	}

	if ( surf->cullinfo.type & CULLINFO_BOX )
	{
		for ( i = 0 ; i < tr.refdef.num_pshadows ; i++ ) {
			if ( ! ( pshadowBits & ( 1 << i ) ) ) {
				continue;
			}
			ps = &tr.refdef.pshadows[i];
			if ( ps->lightOrigin[0] - ps->lightRadius > surf->cullinfo.bounds[1][0]
				|| ps->lightOrigin[0] + ps->lightRadius < surf->cullinfo.bounds[0][0]
				|| ps->lightOrigin[1] - ps->lightRadius > surf->cullinfo.bounds[1][1]
				|| ps->lightOrigin[1] + ps->lightRadius < surf->cullinfo.bounds[0][1]
				|| ps->lightOrigin[2] - ps->lightRadius > surf->cullinfo.bounds[1][2]
				|| ps->lightOrigin[2] + ps->lightRadius < surf->cullinfo.bounds[0][2]
				|| BoxOnPlaneSide(surf->cullinfo.bounds[0], surf->cullinfo.bounds[1], &ps->cullPlane) == 2 ) {
				// pshadow doesn't reach the bounds
				pshadowBits &= ~( 1 << i );
			}
		}
	}

	if ( surf->cullinfo.type & CULLINFO_SPHERE )
	{
		for ( i = 0 ; i < tr.refdef.num_pshadows ; i++ ) {
			if ( ! ( pshadowBits & ( 1 << i ) ) ) {
				continue;
			}
			ps = &tr.refdef.pshadows[i];
			if (!SpheresIntersect(ps->viewOrigin, ps->viewRadius, surf->cullinfo.localOrigin, surf->cullinfo.radius)
				|| DotProduct( surf->cullinfo.localOrigin, ps->cullPlane.normal ) - ps->cullPlane.dist < -surf->cullinfo.radius)
			{
				// pshadow doesn't reach the bounds
				pshadowBits &= ~( 1 << i );
			}
		}
	}

	switch(*surf->data)
	{
	case SF_FACE:
	case SF_GRID:
	case SF_TRIANGLES:
	case SF_VBO_MESH:
		((srfBspSurface_t *)surf->data)->pshadowBits = pshadowBits;
		break;

	default:
		pshadowBits = 0;
		break;
	}

	if ( pshadowBits ) {
		//tr.pc.c_dlightSurfaces++;
	}

	return pshadowBits;
#else
	return 0;
#endif
}

/*
======================
R_AddWorldSurface
======================
*/
static void R_AddWorldSurface(msurface_t *surf, int dlightBits, int pshadowBits, qboolean dontCache) {
	// FIXME: bmodel fog?
	int cubemapIndex = 0;

	// try to cull before dlighting or adding
	if (R_CullSurface(surf) || R_FovCullSurface(surf)) {
		return;
	}

	// check for dlighting
	/*if ( dlightBits ) {
		dlightBits = R_DlightSurface( surf, dlightBits );
		dlightBits = ( dlightBits != 0 );
		}*/

#ifdef __PSHADOWS__
	// check for pshadows
	if ( pshadowBits ) {
		pshadowBits = R_PshadowSurface( surf, pshadowBits);
		pshadowBits = ( pshadowBits != 0 );
	}
#endif

	if ((backEnd.refdef.rdflags & RDF_BLUR) || (tr.viewParms.flags & VPF_SHADOWPASS) || backEnd.depthFill /*|| (backEnd.viewParms.flags & VPF_DEPTHSHADOW)*/)
		cubemapIndex = 0;
	else if (surf->cubemapIndex >= 1 && Distance(tr.refdef.vieworg, tr.cubemapOrigins[surf->cubemapIndex - 1]) < r_cubemapCullRange->value * r_cubemapCullFalloffMult->value)
		cubemapIndex = surf->cubemapIndex;
	else
		cubemapIndex = 0;

#ifdef __ORIGINAL_OCCLUSION__
	if (!r_occlusion->integer || (tr.viewParms.flags & VPF_SHADOWPASS))
#endif //__ORIGINAL_OCCLUSION__
	{
		R_AddDrawSurf(surf->data, surf->shader, surf->fogIndex, dlightBits, R_IsPostRenderEntity(tr.currentEntityNum, tr.currentEntity), cubemapIndex);
	}
#ifdef __ORIGINAL_OCCLUSION__
	else
	{
		if (dontCache)
		{
			R_AddDrawSurf(surf->data, surf->shader, surf->fogIndex, dlightBits, R_IsPostRenderEntity(tr.currentEntityNum, tr.currentEntity), cubemapIndex);
		}
		else
		{
			int scene = tr.viewParms.isPortal ? 1 : 0;

			if (tr.world->numVisibleSurfaces[scene] == MAX_DRAWSURFS)
			{
				ri->Printf(PRINT_ALL, "R_AddWorldSurface(): MAX_DRAWSURFS hit\n");
				return;
			}
			tr.world->visibleSurfaces[scene][tr.world->numVisibleSurfaces[scene]++] = surf;
		}
	}
#endif //__ORIGINAL_OCCLUSION__
}

/*
=============================================================

BRUSH MODELS

=============================================================
*/

/*
=================
R_AddBrushModelSurfaces
=================
*/
void R_AddBrushModelSurfaces(trRefEntity_t *ent) {
	bmodel_t	*bmodel;
	int			clip;
	model_t		*pModel;
	int			i;

	pModel = R_GetModelByHandle(ent->e.hModel);

	bmodel = pModel->data.bmodel;

	clip = R_CullLocalBox(bmodel->bounds);
	if (clip == CULL_OUT) {
		return;
	}

	R_SetupEntityLighting(&tr.refdef, ent);
	R_DlightBmodel(bmodel);

	for (i = 0; i < bmodel->numSurfaces; i++) {
		int surf = bmodel->firstSurface + i;

		if (tr.world->surfacesViewCount[surf] != tr.viewCount)
		{
			tr.world->surfacesViewCount[surf] = tr.viewCount;
			R_AddWorldSurface(tr.world->surfaces + surf, tr.currentEntity->needDlights, 0, qtrue);
		}
	}
}

void RE_SetRangedFog(float range)
{
	tr.rangedFog = range;
}

/*
=============================================================

WORLD MODEL

=============================================================
*/

int R_BoxOnPlaneSide(vec3_t emins, vec3_t emaxs, struct cplane_s *p)
{
	float	dist[2];
	int		sides, b, i;

	// fast axial cases
	if (p->type < 3)
	{
		if (p->dist <= emins[p->type])
			return 1;
		if (p->dist >= emaxs[p->type])// && emins[p->type] != 0) // UQ1: added  && emins[p->type] != 0  to fix a Q3 missing sky bug. It obviously is not being set somewhere...
			return 2;
		return 3;
	}

	// general case
	dist[0] = dist[1] = 0;
	if (p->signbits < 8) // >= 8: default case is original code (dist[0]=dist[1]=0)
	{
		for (i = 0; i < 3; i++)
		{
			b = (p->signbits >> i) & 1;
			dist[b] += p->normal[i] * emaxs[i];
			dist[!b] += p->normal[i] * emins[i];
		}
	}

	sides = 0;
	if (dist[0] >= p->dist)
		sides = 1;
	if (dist[1] < p->dist)// && emins[p->type] != 0) // UQ1: added  && emins[p->type] != 0  to fix a Q3 missing sky bug. It obviously is not being set somewhere...
		sides |= 2;

	return sides;
}

/*
================
R_RecursiveWorldNode
================
*/
static void R_RecursiveWorldNode(mnode_t *node, int planeBits, int dlightBits, int pshadowBits) {
	do {
		int			newDlights[2];
		unsigned int newPShadows[2];

		// if the node wasn't marked as potentially visible, exit
		// pvs is skipped for depth shadows
		if (!(tr.viewParms.flags & VPF_DEPTHSHADOW) && node->visCounts[tr.visIndex] != tr.visCounts[tr.visIndex]) {
			return;
		}

		// if the bounding volume is outside the frustum, nothing
		// inside can be visible OPTIMIZE: don't do this all the way to leafs?

		if (!r_nocull->integer) {
			int		r;

			if (planeBits & 1) {
				r = R_BoxOnPlaneSide(node->mins, node->maxs, &tr.viewParms.frustum[0]);
				if (r == 2) {
					return;						// culled
				}
				if (r == 1) {
					planeBits &= ~1;			// all descendants will also be in front
				}
			}

			if (planeBits & 2) {
				r = R_BoxOnPlaneSide(node->mins, node->maxs, &tr.viewParms.frustum[1]);
				if (r == 2) {
					return;						// culled
				}
				if (r == 1) {
					planeBits &= ~2;			// all descendants will also be in front
				}
			}

			if (planeBits & 4) {
				r = R_BoxOnPlaneSide(node->mins, node->maxs, &tr.viewParms.frustum[2]);
				if (r == 2) {
					return;						// culled
				}
				if (r == 1) {
					planeBits &= ~4;			// all descendants will also be in front
				}
			}

			if (planeBits & 8) {
				r = R_BoxOnPlaneSide(node->mins, node->maxs, &tr.viewParms.frustum[3]);
				if (r == 2) {
					return;						// culled
				}
				if (r == 1) {
					planeBits &= ~8;			// all descendants will also be in front
				}
			}

			if (planeBits & 16) {
				r = R_BoxOnPlaneSide(node->mins, node->maxs, &tr.viewParms.frustum[4]);
				if (r == 2) {
					return;						// culled
				}
				if (r == 1) {
					planeBits &= ~16;			// all descendants will also be in front
				}
			}
		}

		if (node->contents != -1) {
			break;
		}

		// node is just a decision point, so go down both sides
		// since we don't care about sort orders, just go positive to negative

		// determine which dlights are needed
#if 0
		newDlights[0] = 0;
		newDlights[1] = 0;
		if ( dlightBits ) {
			int	i;

			for ( i = 0 ; i < tr.refdef.num_dlights ; i++ ) {
				dlight_t	*dl;
				float		dist;

				if ( dlightBits & ( 1 << i ) ) {
					dl = &tr.refdef.dlights[i];
					dist = DotProduct( dl->origin, node->plane->normal ) - node->plane->dist;

					if ( dist > -dl->radius ) {
						newDlights[0] |= ( 1 << i );
					}
					if ( dist < dl->radius ) {
						newDlights[1] |= ( 1 << i );
					}
				}
			}
		}
#else
		newDlights[0] = 0;
		newDlights[1] = 0;
#endif

#ifdef __PSHADOWS__
		newPShadows[0] = 0;
		newPShadows[1] = 0;
		if ( pshadowBits ) {
			int	i;

			for ( i = 0 ; i < tr.refdef.num_pshadows ; i++ ) {
				pshadow_t	*shadow;
				float		dist;

				if ( pshadowBits & ( 1 << i ) ) {
					shadow = &tr.refdef.pshadows[i];
					dist = DotProduct( shadow->lightOrigin, node->plane->normal ) - node->plane->dist;

					if ( dist > -shadow->lightRadius ) {
						newPShadows[0] |= ( 1 << i );
					}
					if ( dist < shadow->lightRadius ) {
						newPShadows[1] |= ( 1 << i );
					}
				}
			}
		}
#else
		newPShadows[0] = 0;
		newPShadows[1] = 0;
#endif

		// recurse down the children, front side first
		R_RecursiveWorldNode(node->children[0], planeBits, newDlights[0], newPShadows[0]);

		// tail recurse
		node = node->children[1];
		dlightBits = newDlights[1];
		pshadowBits = newPShadows[1];
	} while (1);

	{
		// leaf node, so add mark surfaces
		int			c;
		int surf, *view;

		tr.pc.c_leafs++;

		// add to z buffer bounds
		if (node->mins[0] < tr.viewParms.visBounds[0][0]) {
			tr.viewParms.visBounds[0][0] = node->mins[0];
		}
		if (node->mins[1] < tr.viewParms.visBounds[0][1]) {
			tr.viewParms.visBounds[0][1] = node->mins[1];
		}
		if (node->mins[2] < tr.viewParms.visBounds[0][2]) {
			tr.viewParms.visBounds[0][2] = node->mins[2];
		}

		if (node->maxs[0] > tr.viewParms.visBounds[1][0]) {
			tr.viewParms.visBounds[1][0] = node->maxs[0];
		}
		if (node->maxs[1] > tr.viewParms.visBounds[1][1]) {
			tr.viewParms.visBounds[1][1] = node->maxs[1];
		}
		if (node->maxs[2] > tr.viewParms.visBounds[1][2]) {
			tr.viewParms.visBounds[1][2] = node->maxs[2];
		}

#ifdef __ORIGINAL_OCCLUSION__
		if (r_occlusion->integer)
		{// Occlusion culling...
			if (node->nummarksurfaces > 0)
			{// No point adding if its got no actual surfaces in it...
				int scene = tr.viewParms.isPortal ? 1 : 0;
				tr.world->visibleLeafs[scene][tr.world->numVisibleLeafs[scene]++] = node;
				//node->occluded[scene] = qfalse;
			}
			return;
		}
#endif //__ORIGINAL_OCCLUSION__

		// add merged and unmerged surfaces
		if (tr.world->viewSurfaces && !r_nocurves->integer)
			view = tr.world->viewSurfaces + node->firstmarksurface;
		else
			view = tr.world->marksurfaces + node->firstmarksurface;

		c = node->nummarksurfaces;

		while (c--)
		{
			// just mark it as visible, so we don't jump out of the cache derefencing the surface
			surf = *view;
			if (surf < 0)
			{
				if (tr.world->mergedSurfacesViewCount[-surf - 1] != tr.viewCount)
				{
					tr.world->mergedSurfacesViewCount[-surf - 1] = tr.viewCount;
					//tr.world->mergedSurfacesDlightBits[-surf - 1] = dlightBits;
#ifdef __PSHADOWS__
					tr.world->mergedSurfacesPshadowBits[-surf - 1] = pshadowBits;
#endif
				}
				else
				{
					//tr.world->mergedSurfacesDlightBits[-surf - 1] |= dlightBits;
#ifdef __PSHADOWS__
					tr.world->mergedSurfacesPshadowBits[-surf - 1] |= pshadowBits;
#endif
				}
			}
			else
			{
				if (tr.world->surfacesViewCount[surf] != tr.viewCount)
				{
					tr.world->surfacesViewCount[surf] = tr.viewCount;
					//tr.world->surfacesDlightBits[surf] = dlightBits;
#ifdef __PSHADOWS__
					tr.world->surfacesPshadowBits[surf] = pshadowBits;
#endif
				}
				else
				{
					//tr.world->surfacesDlightBits[surf] |= dlightBits;
#ifdef __PSHADOWS__
					tr.world->surfacesPshadowBits[surf] |= pshadowBits;
#endif
				}
			}
			view++;
		}
	}
}

/*
===============
R_PointInLeaf
===============
*/
static mnode_t *R_PointInLeaf(const vec3_t p) {
	mnode_t		*node;
	float		d;
	cplane_t	*plane;

	if (!tr.world) {
		ri->Error(ERR_DROP, "R_PointInLeaf: bad model");
	}

	node = tr.world->nodes;
	while (1) {
		if (node->contents != -1) {
			break;
		}
		plane = node->plane;
		d = DotProduct(p, plane->normal) - plane->dist;
		if (d > 0) {
			node = node->children[0];
		}
		else {
			node = node->children[1];
		}
	}

	return node;
}

/*
==============
R_ClusterPVS
==============
*/
static const byte *R_ClusterPVS(int cluster) {
	if (!tr.world->vis || cluster < 0 || cluster >= tr.world->numClusters) {
		return NULL;
	}

	return tr.world->vis + cluster * tr.world->clusterBytes;
}

/*
=================
R_inPVS
=================
*/
#define __PVS__
qboolean R_inPVS(const vec3_t p1, const vec3_t p2, byte *mask) {
#ifdef __PVS__
	int		leafnum;
	int		cluster;

	leafnum = ri->CM_PointLeafnum(p1);
	cluster = ri->CM_LeafCluster(leafnum);

	//agh, the damn snapshot mask doesn't work for this
	mask = (byte *)ri->CM_ClusterPVS(cluster);

	leafnum = ri->CM_PointLeafnum(p2);
	cluster = ri->CM_LeafCluster(leafnum);
	if (mask && (!(mask[cluster >> 3] & (1 << (cluster & 7)))))
		return qfalse;

	return qtrue;
#else //!__PVS__
	return qtrue;
#endif //__PVS__
}

/*
===============
R_MarkLeaves

Mark the leaves and nodes that are in the PVS for the current
cluster
===============
*/
static void R_MarkLeaves(void) {
	const byte	*vis;
	mnode_t	*leaf, *parent;
	int		i;
	int		cluster;

	// lockpvs lets designers walk around to determine the
	// extent of the current pvs
	if (r_lockpvs->integer) {
		return;
	}

	// current viewcluster
	leaf = R_PointInLeaf(tr.viewParms.pvsOrigin);
	cluster = leaf->cluster;

	// if the cluster is the same and the area visibility matrix
	// hasn't changed, we don't need to mark everything again
	for (i = 0; i < MAX_VISCOUNTS; i++)
	{
		// if the areamask or r_showcluster was modified, invalidate all visclusters
		// this caused doors to open into undrawn areas
		if (tr.refdef.areamaskModified || r_showcluster->modified)
		{
			tr.visClusters[i] = -2;
		}
		else if (tr.visClusters[i] == cluster)
		{
			if (tr.visClusters[i] != tr.visClusters[tr.visIndex] && r_showcluster->integer)
			{
				ri->Printf(PRINT_ALL, "found cluster:%i  area:%i  index:%i\n", cluster, leaf->area, i);
			}
			tr.visIndex = i;
			return;
		}
	}

	tr.visIndex = (tr.visIndex + 1) % MAX_VISCOUNTS;
	tr.visCounts[tr.visIndex]++;
	tr.visClusters[tr.visIndex] = cluster;

	if (r_showcluster->modified || r_showcluster->integer) {
		r_showcluster->modified = qfalse;
		if (r_showcluster->integer) {
			ri->Printf(PRINT_ALL, "cluster:%i  area:%i\n", cluster, leaf->area);
		}
	}

	vis = R_ClusterPVS(tr.visClusters[tr.visIndex]);

	for (i = 0, leaf = tr.world->nodes; i < tr.world->numnodes; i++, leaf++) {
		cluster = leaf->cluster;
		if (cluster < 0 || cluster >= tr.world->numClusters) {
			continue;
		}

		// check general pvs
		if (vis && !(vis[cluster >> 3] & (1 << (cluster & 7)))) {
			continue;
		}

		// check for door connection
		if ((tr.refdef.areamask[leaf->area >> 3] & (1 << (leaf->area & 7)))) {
			continue;		// not visible
		}

		parent = leaf;
		do {
			if (parent->visCounts[tr.visIndex] == tr.visCounts[tr.visIndex])
				break;
			parent->visCounts[tr.visIndex] = tr.visCounts[tr.visIndex];
			parent = parent->parent;
		} while (parent);
	}
}

qboolean G_BoxInBounds(vec3_t point, vec3_t mins, vec3_t maxs, vec3_t boundsMins, vec3_t boundsMaxs)
{
	vec3_t boxMins;
	vec3_t boxMaxs;

	VectorAdd(point, mins, boxMins);
	VectorAdd(point, maxs, boxMaxs);

	if (boxMaxs[0] > boundsMaxs[0])
		return qfalse;

	if (boxMaxs[1] > boundsMaxs[1])
		return qfalse;

	if (boxMaxs[2] > boundsMaxs[2])
		return qfalse;

	if (boxMins[0] < boundsMins[0])
		return qfalse;

	if (boxMins[1] < boundsMins[1])
		return qfalse;

	if (boxMins[2] < boundsMins[2])
		return qfalse;

	//box is completely contained within bounds
	return qtrue;
}

#ifdef __RENDERER_FOLIAGE__
void R_FoliageQuadStamp(vec4_t quadVerts[4])
{
	vec2_t texCoords[4];

	VectorSet2(texCoords[0], 0.0f, 0.0f);
	VectorSet2(texCoords[1], 1.0f, 0.0f);
	VectorSet2(texCoords[2], 1.0f, 1.0f);
	VectorSet2(texCoords[3], 0.0f, 1.0f);

	GLSL_BindProgram(&tr.textureColorShader);

	GLSL_SetUniformMatrix16(&tr.textureColorShader, UNIFORM_MODELVIEWPROJECTIONMATRIX, glState.modelviewProjection);
	GLSL_SetUniformVec4(&tr.textureColorShader, UNIFORM_COLOR, colorWhite);

	RB_InstantQuad2(quadVerts, texCoords);
}

extern int64_t		r_numentities;
extern int			r_firstSceneEntity;

extern void TR_AxisToAngles ( const vec3_t axis[3], vec3_t angles );
extern void R_WorldToLocal (const vec3_t world, vec3_t local);

static void R_FoliageQuad2( vec3_t surfOrigin, cplane_t plane, vec3_t bounds[2], float scale ) {
	vec3_t	left, up;
	float	radius;
	//float	color[4];

	// calculate the xyz locations for the four corners
	radius = 24.0 * scale;

	vec3_t mins, maxs, newOrg, angles, vfwd, vright, vup;

	VectorSet(mins, bounds[0][0], bounds[0][1], bounds[0][2]);
	VectorSet(maxs, bounds[1][0], bounds[1][1], bounds[1][2]);

	if (mins[0] > maxs[0])
	{
		float temp;
		temp = mins[0];
		mins[0] = maxs[0];
		maxs[0] = temp;
	}

	if (mins[1] > maxs[1])
	{
		float temp;
		temp = mins[1];
		mins[1] = maxs[1];
		maxs[1] = temp;
	}

	if (mins[2] > maxs[2])
	{
		float temp;
		temp = mins[2];
		mins[2] = maxs[2];
		maxs[2] = temp;
	}

	float length = maxs[0] - mins[0];
	mins[0] = -(length/2.0);
	maxs[0] = (length/2.0);

	length = maxs[1] - mins[1];
	mins[1] = -(length/2.0);
	maxs[1] = (length/2.0);

	length = maxs[2] - mins[2];
	mins[2] = -(length/2.0);
	maxs[2] = (length/2.0);

	//AngleVectors(plane.normal, vfwd, vright, vup);
	matrix3_t axis;
	VectorCopy( plane.normal, axis[0] );
	PerpendicularVector( axis[1], axis[0] );
	CrossProduct( axis[0], axis[1], axis[2] );

	VectorCopy( axis[1], left );
	VectorCopy( axis[2], up );

	VectorScale( left, radius, left );
	VectorScale( up, radius, up );

	TR_AxisToAngles(axis, angles);
	AngleVectors (angles, vfwd, vright, vup);

	//ri->Printf(PRINT_WARNING, "mins %f %f %f, maxs %f %f %f, plane %f %f %f\n", mins[0], mins[1], mins[2], maxs[0], maxs[1], maxs[2], plane.normal[0], plane.normal[1], plane.normal[2]);

	bool Ynerf = true;

	for (float offsetX = mins[0]; offsetX < maxs[0]; offsetX += 32.0)
	{
		Ynerf = !Ynerf;

		float ms1 = mins[1];
		if (Ynerf) ms1 += 16.0;

		for (float offsetY = ms1; offsetY < maxs[1]; offsetY += 32.0)
		{
			qhandle_t shader = RE_RegisterShader("models/pop/foliages/sch_weed_a.tga");

			VectorCopy(surfOrigin, newOrg);
			VectorMA( newOrg, offsetX, vright, newOrg );
			VectorMA( newOrg, offsetY, vup, newOrg );

			refEntity_t re;
			memset( &re, 0, sizeof( re ) );
			VectorCopy(newOrg, re.origin);

			re.reType = RT_GRASS;

			re.radius = radius;
			re.customShader = shader;
			re.shaderRGBA[0] = 255;
			re.shaderRGBA[1] = 255;
			re.shaderRGBA[2] = 255;
			re.shaderRGBA[3] = 255;

			//re.origin[2] += re.radius;

			angles[PITCH] = angles[ROLL] = 0.0f;
			angles[YAW] = 0.0f;

			VectorCopy(angles, re.angles);
			AnglesToAxis(angles, re.axis);

			//if (Distance(tr.refdef.vieworg, newOrg) < 64) ri->Printf(PRINT_WARNING, "org %f %f %f\n", newOrg[0], newOrg[1], newOrg[2]);

			RE_AddRefEntityToScene( &re );

			tr.refdef.num_entities = r_numentities - r_firstSceneEntity;
			tr.refdef.entities = &backEndData->entities[r_firstSceneEntity];
		}
	}
}

/*
static void R_FoliageQuad( vec3_t surfOrigin, cplane_t plane, vec3_t bounds[2], float scale ) {
vec3_t	left, up;
float	radius;
float	color[4];

// calculate the xyz locations for the four corners
radius = 128.0;//24.0 * scale;

VectorSet4(color, 1, 1, 1, 1);

vec3_t mins, maxs, newOrg, origin, angles, vfwd, vright, vup;

VectorSet(mins, bounds[0][0], bounds[0][1], bounds[0][2]);
VectorSet(maxs, bounds[1][0], bounds[1][1], bounds[1][2]);

if (mins[0] > maxs[0])
{
float temp;
temp = mins[0];
mins[0] = maxs[0];
maxs[0] = temp;
}

if (mins[1] > maxs[1])
{
float temp;
temp = mins[1];
mins[1] = maxs[1];
maxs[1] = temp;
}

if (mins[2] > maxs[2])
{
float temp;
temp = mins[2];
mins[2] = maxs[2];
maxs[2] = temp;
}

float length = maxs[0] - mins[0];
mins[0] = -(length/2.0);
maxs[0] = (length/2.0);

length = maxs[1] - mins[1];
mins[1] = -(length/2.0);
maxs[1] = (length/2.0);

length = maxs[2] - mins[2];
mins[2] = -(length/2.0);
maxs[2] = (length/2.0);

//AngleVectors(plane.normal, vfwd, vright, vup);
matrix3_t axis;
VectorCopy( plane.normal, axis[0] );
PerpendicularVector( axis[1], axis[0] );
CrossProduct( axis[0], axis[1], axis[2] );

VectorCopy( axis[1], left );
VectorCopy( axis[2], up );

VectorScale( left, radius, left );
VectorScale( up, radius, up );

TR_AxisToAngles(axis, angles);
AngleVectors (angles, vfwd, vright, vup);

//ri->Printf(PRINT_WARNING, "mins %f %f %f, maxs %f %f %f, plane %f %f %f\n", mins[0], mins[1], mins[2], maxs[0], maxs[1], maxs[2], plane.normal[0], plane.normal[1], plane.normal[2]);

for (float offsetX = mins[0]; offsetX < maxs[0]; offsetX += 64.0)
{
for (float offsetY = mins[1]; offsetY < maxs[1]; offsetY += 64.0)
{
VectorCopy(surfOrigin, newOrg);
newOrg[2] += radius;
VectorMA( newOrg, offsetX, vright, newOrg );
VectorMA( newOrg, offsetY, vup, newOrg );
//if (Distance(tr.refdef.vieworg, newOrg) < 64) ri->Printf(PRINT_WARNING, "org %f %f %f\n", newOrg[0], newOrg[1], newOrg[2]);

R_WorldToLocal(newOrg, origin);
//VectorCopy(newOrg, origin);

RB_AddQuadStamp( origin, left, up, color );

#if 0
vec4_t quadVerts[4];
quadVerts[0][0] = origin[0] + left[0] + up[0];
quadVerts[0][1] = origin[1] + left[1] + up[1];
quadVerts[0][2] = origin[2] + left[2] + up[2];

quadVerts[1][0] = origin[0] - left[0] + up[0];
quadVerts[1][1] = origin[1] - left[1] + up[1];
quadVerts[1][2] = origin[2] - left[2] + up[2];

quadVerts[2][0] = origin[0] - left[0] - up[0];
quadVerts[2][1] = origin[1] - left[1] - up[1];
quadVerts[2][2] = origin[2] - left[2] - up[2];

quadVerts[3][0] = origin[0] + left[0] - up[0];
quadVerts[3][1] = origin[1] + left[1] - up[1];
quadVerts[3][2] = origin[2] + left[2] - up[2];
R_FoliageQuadStamp(quadVerts);
#endif
}
}
}

int			FOLIAGE_NUM_SURFACES = 0;
vec3_t		FOLIAGE_ORIGINS[65536];
vec3_t		FOLIAGE_BOUNDS[65536][2];
cplane_t	FOLIAGE_PLANES[65536];
shader_t	*FOLIAGE_SHADERS[65536];

void R_DrawFoliage (int surfaceNum) {
shader_t	*shader = FOLIAGE_SHADERS[surfaceNum];

if ((shader->surfaceFlags & MATERIAL_MASK ) == MATERIAL_SHORTGRASS)
{
R_FoliageQuad(FOLIAGE_ORIGINS[surfaceNum], FOLIAGE_PLANES[surfaceNum], FOLIAGE_BOUNDS[surfaceNum], 1.0);
}
else if ((shader->surfaceFlags & MATERIAL_MASK ) == MATERIAL_LONGGRASS)
{
R_FoliageQuad(FOLIAGE_ORIGINS[surfaceNum], FOLIAGE_PLANES[surfaceNum], FOLIAGE_BOUNDS[surfaceNum], 2.0);
}
}

void R_DrawAllFoliages ( void )
{
for (int i = 0; i < FOLIAGE_NUM_SURFACES; i++)
{
R_DrawFoliage(i);
}

ri->Printf(PRINT_WARNING, "%i foliage surfaces drawn.\n", FOLIAGE_NUM_SURFACES);
}
*/

void R_AddFoliage (msurface_t *surf) {
	//RE_RegisterShader("models/warzone/foliage/grass01.png");
	if ((surf->shader->surfaceFlags & MATERIAL_MASK ) == MATERIAL_SHORTGRASS || (surf->shader->surfaceFlags & MATERIAL_MASK ) == MATERIAL_LONGGRASS)
	{
		vec3_t		surfOrigin;

		//shader_t	*shader = R_GetShaderByHandle(RE_RegisterShader("models/warzone/foliage/grass01.png"));

		surfOrigin[0] = (surf->cullinfo.bounds[0][0] + surf->cullinfo.bounds[1][0]) * 0.5f;
		surfOrigin[1] = (surf->cullinfo.bounds[0][1] + surf->cullinfo.bounds[1][1]) * 0.5f;
		surfOrigin[2] = (surf->cullinfo.bounds[0][2] + surf->cullinfo.bounds[1][2]) * 0.5f;

		if (Distance(surfOrigin, tr.refdef.vieworg) <= 2048)
		{
			/*
			VectorCopy(surfOrigin, FOLIAGE_ORIGINS[FOLIAGE_NUM_SURFACES]);
			VectorCopy(surf->cullinfo.bounds[0], FOLIAGE_BOUNDS[FOLIAGE_NUM_SURFACES][0]);
			VectorCopy(surf->cullinfo.bounds[1], FOLIAGE_BOUNDS[FOLIAGE_NUM_SURFACES][1]);
			FOLIAGE_PLANES[FOLIAGE_NUM_SURFACES] = surf->cullinfo.plane;
			FOLIAGE_SHADERS[FOLIAGE_NUM_SURFACES] = shader;
			FOLIAGE_NUM_SURFACES++;
			*/
			R_FoliageQuad2( surfOrigin, surf->cullinfo.plane, surf->cullinfo.bounds, 1.0 );
		}
	}
}
#endif //__RENDERER_FOLIAGE__

/*
=============
R_AddWorldSurfaces
=============
*/

#if defined(__SOFTWARE_OCCLUSION__) && defined(__THREADED_OCCLUSION2__)
extern void Occlusion_FinishThread();
#endif //defined(__SOFTWARE_OCCLUSION__) && defined(__THREADED_OCCLUSION2__)

vec3_t PREVIOUS_OCCLUSION_ORG = { -999999 };
vec3_t PREVIOUS_OCCLUSION_ANGLES = { -999999 };

void R_AddWorldSurfaces(void) {
	int planeBits;
#ifdef __PSHADOWS__
	int pshadowBits;//, dlightBits;
#endif
	int changeFrustum = 0;
	int scene;

	if (!r_drawworld->integer) {
		return;
	}

	if (tr.refdef.rdflags & RDF_NOWORLDMODEL) {
		return;
	}

	NUM_WORLD_FOV_CULLS = 0;
	NUM_WORLDMERGED_FOV_CULLS = 0;
	/*
	#ifdef __RENDERER_FOLIAGE__
	FOLIAGE_NUM_SURFACES = 0;
	#endif //__RENDERER_FOLIAGE__
	*/

	tr.currentEntityNum = REFENTITYNUM_WORLD;
	tr.shiftedEntityNum = tr.currentEntityNum << QSORT_REFENTITYNUM_SHIFT;

	scene = tr.viewParms.isPortal ? 1 : 0;

	// determine which leaves are in the PVS / areamask
	if (!(tr.viewParms.flags & VPF_DEPTHSHADOW))
		R_MarkLeaves();

#ifdef __ORIGINAL_OCCLUSION__
	if (!r_occlusion->integer || (tr.viewParms.flags & VPF_SHADOWPASS))
#endif //__ORIGINAL_OCCLUSION__
	{
		// clear out the visible min/max
		ClearBounds(tr.viewParms.visBounds[0], tr.viewParms.visBounds[1]);
	}
#ifdef __ORIGINAL_OCCLUSION__
	else
	{
		if (!r_lazyFrustum->integer)
		{
			changeFrustum = 1;
		}
		else
		{
			changeFrustum = 0;

			if (tr.visIndex != tr.previousVisIndex[scene] || tr.refdef.areamaskModified)
			{
				tr.previousVisIndex[scene] = tr.visIndex;
				changeFrustum = 1;
			}

			// force a frustum change every 300 frames
			//if (tr.framesSincePreviousFrustum > 300)
			//{
			//changeFrustum = 1;
			//}

#if 0
			if (!changeFrustum)
			{
				vec3_t ang;
				AxisToAngles(tr.refdef.viewaxis, ang);
				if (Distance(tr.refdef.vieworg, PREVIOUS_OCCLUSION_ORG) > 64)
				{
					changeFrustum = 1;
					VectorCopy(tr.refdef.vieworg, PREVIOUS_OCCLUSION_ORG);
					VectorCopy(ang, PREVIOUS_OCCLUSION_ANGLES);
					ri->Printf(PRINT_ALL, "Frust changed because of org.\n");
				}
				else if (Distance(ang, PREVIOUS_OCCLUSION_ANGLES) > 8)
				{
					changeFrustum = 1;
					VectorCopy(tr.refdef.vieworg, PREVIOUS_OCCLUSION_ORG);
					VectorCopy(ang, PREVIOUS_OCCLUSION_ANGLES);
					ri->Printf(PRINT_ALL, "Frust changed because of ang.\n");
				}
			}
#endif
		}

#if 1
		if (!changeFrustum)
		{
			//clip new view frustum against old one
			vec3_t frustumpoints[5];
			vec3_t vec;
			float ymax, ymin, xmax, xmin, zProj;
			int i, j;

			// this next bit of code is from R_SetupProjection() in tr_main.c
			zProj = 2048; // make this extra large

			ymax = zProj * tan(tr.viewParms.fovY * M_PI / 360.0f);
			ymin = -ymax;

			xmax = zProj * tan(tr.viewParms.fovX * M_PI / 360.0f);
			xmin = -xmax;

			// create five points which represent the view pyramid
			VectorCopy(tr.viewParms.ori.origin, frustumpoints[0]);

			VectorMA(frustumpoints[0], zProj, tr.viewParms.ori.axis[0], frustumpoints[1]);
			VectorCopy(frustumpoints[1], frustumpoints[2]);
			VectorCopy(frustumpoints[1], frustumpoints[3]);
			VectorCopy(frustumpoints[1], frustumpoints[4]);

			VectorScale(tr.viewParms.ori.axis[1], xmin, vec);
			VectorAdd(vec, frustumpoints[1], frustumpoints[1]);
			VectorAdd(vec, frustumpoints[3], frustumpoints[3]);

			VectorScale(tr.viewParms.ori.axis[1], xmax, vec);
			VectorAdd(vec, frustumpoints[2], frustumpoints[2]);
			VectorAdd(vec, frustumpoints[4], frustumpoints[4]);

			VectorScale(tr.viewParms.ori.axis[2], ymin, vec);
			VectorAdd(vec, frustumpoints[1], frustumpoints[1]);
			VectorAdd(vec, frustumpoints[2], frustumpoints[2]);

			VectorScale(tr.viewParms.ori.axis[2], ymax, vec);
			VectorAdd(vec, frustumpoints[3], frustumpoints[3]);
			VectorAdd(vec, frustumpoints[4], frustumpoints[4]);

			for (i = 0; i < 5; i++)
			{
				for (j = 0; j < 4 /*FRUSTUM_PLANES*/; j++)
				{
					cplane_t *frust = &tr.previousFrustum[scene][j];
					float dot;

					dot = DotProduct(frustumpoints[i], frust->normal);

					if (dot < frust->dist - 0.001f)
					{
						// completely outside frustum
						changeFrustum = 1;
						break;
					}
				}

				if (changeFrustum)
					break;
			}
		}
#endif
	}
#endif //__ORIGINAL_OCCLUSION__

	// perform frustum culling and flag all the potentially visible surfaces
	tr.refdef.num_dlights = min(tr.refdef.num_dlights, 32);
#ifdef __PSHADOWS__
	tr.refdef.num_pshadows = min (tr.refdef.num_pshadows, 32) ;
#endif

	planeBits = (tr.viewParms.flags & VPF_FARPLANEFRUSTUM) ? 31 : 15;

	if (tr.viewParms.flags & VPF_DEPTHSHADOW)
	{
		//dlightBits = 0;
#ifdef __PSHADOWS__
		pshadowBits = 0;
#endif
	}
	else if (!(tr.viewParms.flags & VPF_SHADOWMAP))
	{
		//dlightBits = ( 1 << tr.refdef.num_dlights ) - 1;
#ifdef __PSHADOWS__
		pshadowBits = ( 1 << tr.refdef.num_pshadows ) - 1;
#endif
	}
	else
	{
		//dlightBits = ( 1 << tr.refdef.num_dlights ) - 1;
#ifdef __PSHADOWS__
		pshadowBits = 0;
#endif
	}

#ifdef __ORIGINAL_OCCLUSION__
	if (!r_occlusion->integer || (tr.viewParms.flags & VPF_SHADOWPASS))
#endif //__ORIGINAL_OCCLUSION__
	{
#ifdef __PSHADOWS__
		R_RecursiveWorldNode( tr.world->nodes, planeBits, 0/*dlightBits*/, pshadowBits);
#else //!__PSHADOWS__
		R_RecursiveWorldNode(tr.world->nodes, planeBits, 0/*dlightBits*/, 0/*pshadowBits*/);
#endif //__PSHADOWS__
		//tr.updateOcclusion[0] = qtrue;
	}
#ifdef __ORIGINAL_OCCLUSION__
	else
	{
		if (changeFrustum)
		{
			int i;

			// clear out the visible min/max
			ClearBounds(tr.viewParms.visBounds[0], tr.viewParms.visBounds[1]);

			tr.world->numVisibleLeafs[scene] = 0;

			R_RecursiveWorldNode(tr.world->nodes, /*15*/planeBits, 0, 0);

			VectorCopy(tr.viewParms.visBounds[0], tr.previousVisBounds[scene][0]);
			VectorCopy(tr.viewParms.visBounds[1], tr.previousVisBounds[scene][1]);
			for (i = 0; i < 4; i++)
			{
				VectorCopy(tr.viewParms.frustum[i].normal, tr.previousFrustum[scene][i].normal);
				tr.previousFrustum[scene][i].dist = tr.viewParms.frustum[i].dist;
				tr.previousFrustum[scene][i].type = tr.viewParms.frustum[i].type;
				tr.previousFrustum[scene][i].signbits = tr.viewParms.frustum[i].signbits;
			}
			//tr.previousFrustumTime = ri.Milliseconds();
			//tr.framesSincePreviousFrustum = 0;
			tr.updateVisibleSurfaces[scene] = qtrue;
			tr.updateOcclusion[scene] = qtrue;
			tr.changedFrustum = qtrue;
		}
		else
		{
			VectorCopy(tr.previousVisBounds[scene][0], tr.viewParms.visBounds[0]);
			VectorCopy(tr.previousVisBounds[scene][1], tr.viewParms.visBounds[1]);
			tr.changedFrustum = qfalse;
		}

#if defined(__SOFTWARE_OCCLUSION__) && defined(__THREADED_OCCLUSION2__)
		Occlusion_FinishThread();
#endif //defined(__SOFTWARE_OCCLUSION__) && defined(__THREADED_OCCLUSION2__)

		if ((!r_cacheVisibleSurfaces->integer || tr.updateVisibleSurfaces[scene]))
		{
			int i;
			int occludedCount = 0;

			tr.world->numVisibleSurfaces[scene] = 0;

			for (i = 0; i < tr.world->numVisibleLeafs[scene]; i++)
			{
				mnode_t *leaf = tr.world->visibleLeafs[scene][i];
				int c;

				if (leaf->occluded[scene])
				{
					occludedCount++;
					continue;
				}

				// add the individual surfaces
				c = leaf->nummarksurfaces;
				while (c--) {
					// the surface may have already been added if it
					// spans multiple leafs
					int *mark = (tr.world->marksurfaces + leaf->firstmarksurface + c);
					msurface_t *surf = tr.world->surfaces + *mark;
					R_AddWorldSurface(surf, 0 /*dlightBits*/, 0, r_cacheVisibleSurfaces->integer ? qfalse : qtrue);
				}
			}

			tr.updateVisibleSurfaces[scene] = qfalse;

			if (r_occlusionDebug->integer == 4)
				ri->Printf(PRINT_ALL, "OCCLUSION DEBUG: time %i. occludedCount was %i. totalCount %i.\n", backEnd.refdef.time, occludedCount, tr.world->numVisibleLeafs[scene]);
		}
		//ri.Printf(PRINT_ALL, "Change Frustum: %d\n", changeFrustum);

		if (r_cacheVisibleSurfaces->integer)
		{
			int i;

			for (i = 0; i < tr.world->numVisibleSurfaces[scene]; i++)
			{
				msurface_t *surf = tr.world->visibleSurfaces[scene][i];
				//R_AddDrawSurf( surf->data, surf->shader, surf->fogIndex, 0 /*dlightBits*/, 0, 0 );
				R_AddWorldSurface(surf, 0 /*dlightBits*/, 0, qtrue);
			}
		}

		return;
	}
#endif //__ORIGINAL_OCCLUSION__

	// now add all the potentially visible surfaces
	// also mask invisible dlights for next frame
#ifdef __ORIGINAL_OCCLUSION__
	if (!r_occlusion->integer || (tr.viewParms.flags & VPF_SHADOWPASS))
#endif //__ORIGINAL_OCCLUSION__
	{
		int i;

		//tr.refdef.dlightMask = 0;

		for (i = 0; i < tr.world->numWorldSurfaces; i++)
		{
			if (tr.world->surfacesViewCount[i] != tr.viewCount)
				continue;

#ifdef __PSHADOWS__
			R_AddWorldSurface( tr.world->surfaces + i, 0/*tr.world->surfacesDlightBits[i]*/, tr.world->surfacesPshadowBits[i], qtrue );
#else //!__PSHADOWS__
			R_AddWorldSurface(tr.world->surfaces + i, 0/*tr.world->surfacesDlightBits[i]*/, 0/*tr.world->surfacesPshadowBits[i]*/, qtrue);
#endif //__PSHADOWS__
			//tr.refdef.dlightMask |= tr.world->surfacesDlightBits[i];

#ifdef __RENDERER_FOLIAGE__
			R_AddFoliage(tr.world->surfaces + i);
#endif //__RENDERER_FOLIAGE__
		}

		for (i = 0; i < tr.world->numMergedSurfaces; i++)
		{
			if (tr.world->mergedSurfacesViewCount[i] != tr.viewCount)
				continue;

#ifdef __PSHADOWS__
			R_AddWorldSurface( tr.world->mergedSurfaces + i, 0/*tr.world->mergedSurfacesDlightBits[i]*/, tr.world->mergedSurfacesPshadowBits[i], qtrue );
#else //!__PSHADOWS__
			R_AddWorldSurface(tr.world->mergedSurfaces + i, 0/*tr.world->mergedSurfacesDlightBits[i]*/, 0/*tr.world->mergedSurfacesPshadowBits[i]*/, qtrue);
#endif //__PSHADOWS__
			//tr.refdef.dlightMask |= tr.world->mergedSurfacesDlightBits[i];

#ifdef __RENDERER_FOLIAGE__
			R_AddFoliage(tr.world->mergedSurfaces + i);
#endif //__RENDERER_FOLIAGE__
		}

		/*
		#ifdef __RENDERER_FOLIAGE__
		ri->Printf(PRINT_WARNING, "%i foliage surfaces added.\n", FOLIAGE_NUM_SURFACES);
		#endif //__RENDERER_FOLIAGE__
		*/

		//tr.refdef.dlightMask = ~tr.refdef.dlightMask;

		if (r_fovCull->integer >= 2)
			ri->Printf(PRINT_WARNING, "There are %i world and %i merged surfaces. Culled %i. %i World and %i Merged.\n", tr.world->numWorldSurfaces, tr.world->numMergedSurfaces, NUM_WORLD_FOV_CULLS + NUM_WORLDMERGED_FOV_CULLS, NUM_WORLD_FOV_CULLS, NUM_WORLDMERGED_FOV_CULLS);
	}
}