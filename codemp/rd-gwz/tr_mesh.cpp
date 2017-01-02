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
// tr_mesh.c: triangle model functions

#include "tr_local.h"

float ProjectRadius( float r, vec3_t location )
{
	float pr;
	float dist;
	float c;
	vec3_t	p;
	float	projected[4];

	c = DotProduct( tr.viewParms.ori.axis[0], tr.viewParms.ori.origin );
	dist = DotProduct( tr.viewParms.ori.axis[0], location ) - c;

	if ( dist <= 0 )
		return 0;

	p[0] = 0;
	p[1] = fabs( r );
	p[2] = -dist;

	projected[0] = p[0] * tr.viewParms.projectionMatrix[0] + 
		           p[1] * tr.viewParms.projectionMatrix[4] +
				   p[2] * tr.viewParms.projectionMatrix[8] +
				   tr.viewParms.projectionMatrix[12];

	projected[1] = p[0] * tr.viewParms.projectionMatrix[1] + 
		           p[1] * tr.viewParms.projectionMatrix[5] +
				   p[2] * tr.viewParms.projectionMatrix[9] +
				   tr.viewParms.projectionMatrix[13];

	projected[2] = p[0] * tr.viewParms.projectionMatrix[2] + 
		           p[1] * tr.viewParms.projectionMatrix[6] +
				   p[2] * tr.viewParms.projectionMatrix[10] +
				   tr.viewParms.projectionMatrix[14];

	projected[3] = p[0] * tr.viewParms.projectionMatrix[3] + 
		           p[1] * tr.viewParms.projectionMatrix[7] +
				   p[2] * tr.viewParms.projectionMatrix[11] +
				   tr.viewParms.projectionMatrix[15];


	pr = projected[1] / projected[3];

	if ( pr > 1.0f )
		pr = 1.0f;

	return pr;
}

/*
=============
R_CullModel
=============
*/
static int R_CullModel( mdvModel_t *model, trRefEntity_t *ent ) {
	vec3_t		bounds[2];
	mdvFrame_t	*oldFrame, *newFrame;
	int			i;

	// compute frame pointers
	newFrame = model->frames + ent->e.frame;
	oldFrame = model->frames + ent->e.oldframe;

	// cull bounding sphere ONLY if this is not an upscaled entity
	if ( !ent->e.nonNormalizedAxes )
	{
		if ( ent->e.frame == ent->e.oldframe )
		{
			switch ( R_CullLocalPointAndRadius( newFrame->localOrigin, newFrame->radius ) )
			{
			case CULL_OUT:
				tr.pc.c_sphere_cull_md3_out++;
				return CULL_OUT;

			case CULL_IN:
				tr.pc.c_sphere_cull_md3_in++;
				return CULL_IN;

			case CULL_CLIP:
				tr.pc.c_sphere_cull_md3_clip++;
				break;
			}
		}
		else
		{
			int sphereCull, sphereCullB;

			sphereCull  = R_CullLocalPointAndRadius( newFrame->localOrigin, newFrame->radius );
			if ( newFrame == oldFrame ) {
				sphereCullB = sphereCull;
			} else {
				sphereCullB = R_CullLocalPointAndRadius( oldFrame->localOrigin, oldFrame->radius );
			}

			if ( sphereCull == sphereCullB )
			{
				if ( sphereCull == CULL_OUT )
				{
					tr.pc.c_sphere_cull_md3_out++;
					return CULL_OUT;
				}
				else if ( sphereCull == CULL_IN )
				{
					tr.pc.c_sphere_cull_md3_in++;
					return CULL_IN;
				}
				else
				{
					tr.pc.c_sphere_cull_md3_clip++;
				}
			}
		}
	}
	
	// calculate a bounding box in the current coordinate system
	for (i = 0 ; i < 3 ; i++) {
		bounds[0][i] = oldFrame->bounds[0][i] < newFrame->bounds[0][i] ? oldFrame->bounds[0][i] : newFrame->bounds[0][i];
		bounds[1][i] = oldFrame->bounds[1][i] > newFrame->bounds[1][i] ? oldFrame->bounds[1][i] : newFrame->bounds[1][i];
	}

	switch ( R_CullLocalBox( bounds ) )
	{
	case CULL_IN:
		tr.pc.c_box_cull_md3_in++;
		return CULL_IN;
	case CULL_CLIP:
		tr.pc.c_box_cull_md3_clip++;
		return CULL_CLIP;
	case CULL_OUT:
	default:
		tr.pc.c_box_cull_md3_out++;
		return CULL_OUT;
	}
}


/*
=================
R_ComputeLOD

=================
*/
int R_ComputeLOD( trRefEntity_t *ent ) {
	float radius;
	float flod, lodscale;
	float projectedRadius;
	mdvFrame_t *frame;
	mdrHeader_t *mdr;
	mdrFrame_t *mdrframe;
	int lod;

	if ( tr.currentModel->numLods < 2 )
	{
		// model has only 1 LOD level, skip computations and bias
		lod = 0;
	}
	else
	{
		// multiple LODs exist, so compute projected bounding sphere
		// and use that as a criteria for selecting LOD

		if(tr.currentModel->type == MOD_MDR)
		{
			int frameSize;
			mdr = tr.currentModel->data.mdr;
			frameSize = (size_t) (&((mdrFrame_t *)0)->bones[mdr->numBones]);
			
			mdrframe = (mdrFrame_t *) ((byte *) mdr + mdr->ofsFrames + frameSize * ent->e.frame);
			
			radius = RadiusFromBounds(mdrframe->bounds[0], mdrframe->bounds[1]);
		}
		else
		{
			//frame = ( md3Frame_t * ) ( ( ( unsigned char * ) tr.currentModel->md3[0] ) + tr.currentModel->md3[0]->ofsFrames );
			frame = tr.currentModel->data.mdv[0]->frames;

			frame += ent->e.frame;

			radius = RadiusFromBounds( frame->bounds[0], frame->bounds[1] );
		}

		if ( ( projectedRadius = ProjectRadius( radius, ent->e.origin ) ) != 0 )
		{
			lodscale = (r_lodscale->value+r_autolodscalevalue->integer);
			if (lodscale > 20) lodscale = 20;
			flod = 1.0f - projectedRadius * lodscale;
		}
		else
		{
			// object intersects near view plane, e.g. view weapon
			flod = 0;
		}

		flod *= tr.currentModel->numLods;
		lod = Q_ftol(flod);

		if ( lod < 0 )
		{
			lod = 0;
		}
		else if ( lod >= tr.currentModel->numLods )
		{
			lod = tr.currentModel->numLods - 1;
		}
	}

	lod += r_lodbias->integer;
	
	if ( lod >= tr.currentModel->numLods )
		lod = tr.currentModel->numLods - 1;
	if ( lod < 0 )
		lod = 0;

	return lod;
}

/*
=================
R_ComputeFogNum

=================
*/
int R_ComputeFogNum( mdvModel_t *model, trRefEntity_t *ent ) {
	int				i, j;
	fog_t			*fog;
	mdvFrame_t		*mdvFrame;
	vec3_t			localOrigin;

	if ( tr.refdef.rdflags & RDF_NOWORLDMODEL ) {
		return 0;
	}

	// FIXME: non-normalized axis issues
	mdvFrame = model->frames + ent->e.frame;
	VectorAdd( ent->e.origin, mdvFrame->localOrigin, localOrigin );
	for ( i = 1 ; i < tr.world->numfogs ; i++ ) {
		fog = &tr.world->fogs[i];
		for ( j = 0 ; j < 3 ; j++ ) {
			if ( localOrigin[j] - mdvFrame->radius >= fog->bounds[1][j] ) {
				break;
			}
			if ( localOrigin[j] + mdvFrame->radius <= fog->bounds[0][j] ) {
				break;
			}
		}
		if ( j == 3 ) {
			return i;
		}
	}

	return 0;
}

void R_MergeMD3Surfaces(trRefEntity_t *ent, mdvModel_t *model, int fogNum, int cubemapIndex)
{
#define MAX_MODEL_SURFS 8192

	int i, j, k;
	int mergedSurfIndex = 0;
	int numUnmergedSurfaces = 0;
	int numIboIndexes = 0;
	int startTime, endTime;
	msurface_t *mergedSurf = NULL;
	
	int			numMergedSurfaces = 0;
	msurface_t	mergedSurfaces[MAX_MODEL_SURFS];
	int         mergedSurfacesViewCount = 0;

	int         surfacesViewCount[MAX_MODEL_SURFS];

	int			nummarksurfaces = 0;
	mdvSurface_t	marksurfaces[MAX_MODEL_SURFS];


	startTime = ri->Milliseconds();

	// use viewcount to keep track of mergers
	for (i = 0; i < model->numSurfaces; i++)
	{
		surfacesViewCount[i] = -1;
	}

	// mark matching surfaces
	for (j = 0; j < model->numSurfaces; j++)
	{
		mdvSurface_t *surf1;
		shader_t *shader1;
		int surfNum1;

		surfNum1 = j;

		if (surfacesViewCount[surfNum1] != -1)
			continue;

		surf1 = model->surfaces + surfNum1;

		if ( ent->e.customShader ) {
			shader1 = R_GetShaderByHandle( ent->e.customShader );
		} else if ( ent->e.customSkin > 0 && ent->e.customSkin < tr.numSkins ) {
			skin_t *skin;
			int		j;

			skin = R_GetSkinByHandle( ent->e.customSkin );

			// match the surface name to something in the skin file
			shader1 = tr.defaultShader;
			for ( j = 0 ; j < skin->numSurfaces ; j++ ) {
				// the names have both been lowercased
				if ( !strcmp( skin->surfaces[j]->name, surf1->name ) ) {
					shader1 = (shader_t *)skin->surfaces[j]->shader;
					break;
				}
			}
		} else {
			shader1 = tr.shaders[ surf1->shaderIndexes[ ent->e.skinNum % surf1->numShaderIndexes ] ];
		}

		if(shader1->isSky)
			continue;

		if(shader1->isPortal)
			continue;

		if(ShaderRequiresCPUDeforms(shader1))
			continue;
		
		surfacesViewCount[surfNum1] = surfNum1;

		for (k = j + 1; k < model->numSurfaces; k++)
		{
			mdvSurface_t *surf2;
			shader_t *shader2;
			int surfNum2;

			surfNum2 = k;

			if (surfacesViewCount[surfNum2] != -1)
				continue;

			surf2 = model->surfaces + surfNum2;

			if ( ent->e.customShader ) {
				shader2 = R_GetShaderByHandle( ent->e.customShader );
			} else if ( ent->e.customSkin > 0 && ent->e.customSkin < tr.numSkins ) {
				skin_t *skin;
				int		j;

				skin = R_GetSkinByHandle( ent->e.customSkin );

				// match the surface name to something in the skin file
				shader2 = tr.defaultShader;
				for ( j = 0 ; j < skin->numSurfaces ; j++ ) {
					// the names have both been lowercased
					if ( !strcmp( skin->surfaces[j]->name, surf2->name ) ) {
						shader2 = (shader_t *)skin->surfaces[j]->shader;
						break;
					}
				}
			} else {
				shader2 = tr.shaders[ surf2->shaderIndexes[ ent->e.skinNum % surf2->numShaderIndexes ] ];
			}

			if (shader1 && shader2 && shader1->stages[0] && shader2->stages[0] && ( r_glslWater->integer && shader1->stages[0]->isWater && shader2->stages[0]->isWater))
			{// UQ1: All water can be safely merged I believe...
				surfacesViewCount[surfNum2] = surfNum1;
				continue;
			}
			else if (shader1 != shader2
				// Merge matching shader names...
				&& stricmp(shader1->name, shader2->name))
			{
				continue;
			}

			surfacesViewCount[surfNum2] = surfNum1;
		}
	}

	// don't add surfaces that don't merge to any others to the merged list
	for (i = 0; i < model->numSurfaces; i++)
	{
		qboolean merges = qfalse;

		if (surfacesViewCount[i] != i)
			continue;

		for (j = 0; j < model->numSurfaces; j++)
		{
			if (j == i)
				continue;

			if (surfacesViewCount[j] == i)
			{
				merges = qtrue;
				break;
			}
		}

		if (!merges)
			surfacesViewCount[i] = -1;
	}

	// count merged/unmerged surfaces
	numMergedSurfaces = 0;
	numUnmergedSurfaces = 0;

	for (i = 0; i < model->numSurfaces; i++)
	{
		if (surfacesViewCount[i] == i)
		{
			numMergedSurfaces++;
		}
		else if (surfacesViewCount[i] == -1)
		{
			numUnmergedSurfaces++;
		}
	}

	// Allocate merged surfaces
	//s_worldData.mergedSurfaces = (msurface_t *)ri->Hunk_Alloc(sizeof(*s_worldData.mergedSurfaces) * numMergedSurfaces, h_low);
	//s_worldData.mergedSurfacesViewCount = (int *)ri->Hunk_Alloc(sizeof(*s_worldData.mergedSurfacesViewCount) * numMergedSurfaces, h_low);
	//numMergedSurfaces = numMergedSurfaces;
	
	// view surfaces are like mark surfaces, except negative ones represent merged surfaces
	// -1 represents 0, -2 represents 1, and so on
	//viewSurfaces = (int *)ri->Hunk_Alloc(sizeof(*s_worldData.viewSurfaces) * s_worldData.nummarksurfaces, h_low);

	int viewSurfaces[MAX_MODEL_SURFS];

	// copy view surfaces into mark surfaces
	for (i = 0; i < model->numSurfaces; i++)
	{
		viewSurfaces[i] = (int &)marksurfaces[i]; // UQ1: Hmmm????
	}

	// need to be synched here
	//R_IssuePendingRenderCommands();

	// actually merge surfaces
	numIboIndexes = 0;
	mergedSurfIndex = 0;
	mergedSurf = mergedSurfaces;

	for (i = 0; i < model->numSurfaces; i++)
	{
		mdvSurface_t *surf1;
		VBO_t *vbo;
		IBO_t *ibo = model->vboSurfaces->ibo;
		glIndex_t *iboIndexes, *outIboIndexes;

		vec3_t bounds[2];

		int numSurfsToMerge;
		int numIndexes;
		int numVerts;
		int firstIndex;

		srfBspSurface_t *vboSurf;

		if (surfacesViewCount[i] != i)
			continue;

		surf1 = model->surfaces + i;
		
		// retrieve vbo
		vbo = surf1->model->vboSurfaces->vbo;//((srfBspSurface_t *)(surf1->data))->vbo;

		// count verts, indexes, and surfaces
		numSurfsToMerge = 0;
		numIndexes = 0;
		numVerts = 0;

		for (j = i; j < model->numSurfaces; j++)
		{
			mdvSurface_t *surf2;
			srfBspSurface_t *bspSurf;

			if (surfacesViewCount[j] != i)
				continue;

			surf2 = model->surfaces + j;

			//bspSurf = (srfBspSurface_t *) surf2->data;
			bspSurf = (srfBspSurface_t *) surf2->model->vboSurfaces->vbo;
			numIndexes += bspSurf->numIndexes;
			numVerts += bspSurf->numVerts;
			numSurfsToMerge++;
		}

		if (numVerts == 0 || numIndexes == 0 || numSurfsToMerge < 2)
		{
			continue;
		}
		
		// create ibo
		//ibo = tr.ibos[tr.numIBOs++] = (IBO_t*)ri->Hunk_Alloc(sizeof(*ibo), h_low);
		memset(ibo, 0, sizeof(*ibo));
		numIboIndexes = 0;
		
		// allocate indexes
		iboIndexes = outIboIndexes = (glIndex_t*)ibo->indexesSize;//(glIndex_t*)Z_Malloc(numIndexes * sizeof(*outIboIndexes), TAG_BSP);

		// Merge surfaces (indexes) and calculate bounds
		ClearBounds(bounds[0], bounds[1]);
		firstIndex = numIboIndexes;

		for (j = i; j < model->numSurfaces; j++)
		{
			mdvSurface_t *surf2;
			srfBspSurface_t *bspSurf;

			if (surfacesViewCount[j] != i)
				continue;

			surf2 = model->surfaces + j;

			bspSurf = (srfBspSurface_t *) surf2->model->vboSurfaces->vbo;

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
		mergedSurf->fogIndex      = fogNum;
		mergedSurf->cubemapIndex  = cubemapIndex;
		//mergedSurf->shader        = surf1->shader;

		if ( ent->e.customShader ) {
			mergedSurf->shader = R_GetShaderByHandle( ent->e.customShader );
		} else if ( ent->e.customSkin > 0 && ent->e.customSkin < tr.numSkins ) {
			skin_t *skin;
			int		j;

			skin = R_GetSkinByHandle( ent->e.customSkin );

			// match the surface name to something in the skin file
			mergedSurf->shader = tr.defaultShader;
			for ( j = 0 ; j < skin->numSurfaces ; j++ ) {
				// the names have both been lowercased
				if ( !strcmp( skin->surfaces[j]->name, surf1->name ) ) {
					mergedSurf->shader = (shader_t *)skin->surfaces[j]->shader;
					break;
				}
			}
		} else {
			mergedSurf->shader = tr.shaders[ surf1->shaderIndexes[ ent->e.skinNum % surf1->numShaderIndexes ] ];
		}

		// finish up the ibo
		qglGenBuffers(1, &ibo->indexesVBO);

		R_BindIBO(ibo);
		qglBufferData(GL_ELEMENT_ARRAY_BUFFER, numIboIndexes * sizeof(*iboIndexes), iboIndexes, ibo->iboUsage);
		R_BindNullIBO();

		GL_CheckErrors();

   		Z_Free(iboIndexes);
		Z_Free(vboSurf);

		// redirect view surfaces to this surf
		for (j = 0; j < model->numSurfaces; j++)
		{
			if (surfacesViewCount[j] != i)
				continue;

			for (k = 0; k < model->numSurfaces; k++)
			{
				int *mark = (int *)(marksurfaces + k);
				int *view = viewSurfaces + k;

				if (*mark == j)
					*view = -(mergedSurfIndex + 1);
			}
		}

		mergedSurfIndex++;
		mergedSurf++;
	}

	endTime = ri->Milliseconds();

	ri->Printf(PRINT_ALL, "Processed %d surfaces into %d merged, %d unmerged in %5.2f seconds\n", 
		model->numSurfaces, numMergedSurfaces, numUnmergedSurfaces, (endTime - startTime) / 1000.0f);

	for (j = 0; j < numMergedSurfaces; j++)
	{
		R_AddDrawSurf(mergedSurfaces[i].data, mergedSurfaces[i].shader, fogNum, qfalse, R_IsPostRenderEntity (tr.currentEntityNum, ent), cubemapIndex );
	}

	for (j = 0; j < model->numSurfaces; j++)
	{
		if (surfacesViewCount[j] == -1)
		{
			srfVBOMDVMesh_t *vboSurface = &model->vboSurfaces[i];
			shader_t		*shader = NULL;

			if ( ent->e.customShader ) {
				shader = R_GetShaderByHandle( ent->e.customShader );
			} else if ( ent->e.customSkin > 0 && ent->e.customSkin < tr.numSkins ) {
				skin_t *skin;
				int		j;

				skin = R_GetSkinByHandle( ent->e.customSkin );

				// match the surface name to something in the skin file
				shader = tr.defaultShader;
				for ( j = 0 ; j < skin->numSurfaces ; j++ ) {
					// the names have both been lowercased
					if ( !strcmp( skin->surfaces[j]->name, model->surfaces[j].name ) ) {
						shader = (shader_t *)skin->surfaces[j]->shader;
						break;
					}
				}
				if (shader == tr.defaultShader) {
					ri->Printf( PRINT_DEVELOPER, "WARNING: no shader for surface %s in skin %s\n", model->surfaces[j].name, skin->name);
				}
				else if (shader->defaultShader) {
					ri->Printf( PRINT_DEVELOPER, "WARNING: shader %s in skin %s not found\n", model->surfaces[j].name, skin->name);
				}
			} else {
				shader = tr.shaders[ model->surfaces[j].shaderIndexes[ ent->e.skinNum % model->surfaces[j].numShaderIndexes ] ];
			}

			R_AddDrawSurf((surfaceType_t *)vboSurface, shader, fogNum, qfalse, R_IsPostRenderEntity (tr.currentEntityNum, ent), cubemapIndex );
		}
	}
}

/*
=================
R_AddMD3Surfaces

=================
*/
void R_AddMD3Surfaces( trRefEntity_t *ent ) {
	int				i;
	mdvModel_t		*model = NULL;
	mdvSurface_t	*surface = NULL;
	shader_t		*shader = NULL;
	int				cull;
	int				lod;
	int64_t			fogNum;
	int             cubemapIndex;
	qboolean		personalModel;

	// don't add third_person objects if not in a portal
	personalModel = (qboolean)((ent->e.renderfx & RF_THIRD_PERSON) && !(tr.viewParms.isPortal 
	                 || (tr.viewParms.flags & (VPF_SHADOWMAP | VPF_DEPTHSHADOW))));

	if(personalModel) return; // Seems to never draw in this code, why waste time?

	if ( ent->e.renderfx & RF_WRAP_FRAMES ) {
		ent->e.frame %= tr.currentModel->data.mdv[0]->numFrames;
		ent->e.oldframe %= tr.currentModel->data.mdv[0]->numFrames;
	}

	//
	// Validate the frames so there is no chance of a crash.
	// This will write directly into the entity structure, so
	// when the surfaces are rendered, they don't need to be
	// range checked again.
	//
	if ( (ent->e.frame >= tr.currentModel->data.mdv[0]->numFrames) 
		|| (ent->e.frame < 0)
		|| (ent->e.oldframe >= tr.currentModel->data.mdv[0]->numFrames)
		|| (ent->e.oldframe < 0) ) {
			ri->Printf( PRINT_DEVELOPER, "R_AddMD3Surfaces: no such frame %d to %d for '%s'\n",
				ent->e.oldframe, ent->e.frame,
				tr.currentModel->name );
			ent->e.frame = 0;
			ent->e.oldframe = 0;
	}

	//
	// compute LOD
	//
	lod = R_ComputeLOD( ent );

	model = tr.currentModel->data.mdv[lod];

	//
	// cull the entire model if merged bounding box of both frames
	// is outside the view frustum.
	//
	cull = R_CullModel ( model, ent );
	if ( cull == CULL_OUT ) {
		return;
	}

	//
	// set up lighting now that we know we aren't culled
	//
	if ( !personalModel || r_shadows->integer > 1 ) {
		R_SetupEntityLighting( &tr.refdef, ent );
	}

	//
	// see if we are in a fog volume
	//
	fogNum = R_ComputeFogNum( model, ent );

	if ((backEnd.refdef.rdflags & RDF_BLUR) || (tr.viewParms.flags & VPF_SHADOWPASS) || backEnd.depthFill /*|| (backEnd.viewParms.flags & VPF_DEPTHSHADOW)*/)
		cubemapIndex = 0;
	else if (r_cubeMapping->integer >= 2)
		cubemapIndex = R_CubemapForPoint(ent->e.origin);
	else
		cubemapIndex = 0;

	if (cubemapIndex-1 < 0 || Distance(tr.refdef.vieworg, tr.cubemapOrigins[cubemapIndex-1]) > r_cubemapCullRange->value * r_cubemapCullFalloffMult->value)
		cubemapIndex = 0;

//#define __MERGE_MD3_TEST__

#ifdef __MERGE_MD3_TEST__
	// This should be done at load, but, I have no idea what i'm doing... *sigh*
	R_MergeMD3Surfaces(ent, model, fogNum, cubemapIndex);
#else //!__MERGE_MD3_TEST__
	//
	// draw all surfaces
	//
	surface = model->surfaces;
	for ( i = 0 ; i < model->numSurfaces ; i++ ) {

		if ( ent->e.customShader ) {
			shader = R_GetShaderByHandle( ent->e.customShader );
		} else if ( ent->e.customSkin > 0 && ent->e.customSkin < tr.numSkins ) {
			skin_t *skin;
			int		j;

			skin = R_GetSkinByHandle( ent->e.customSkin );

			// match the surface name to something in the skin file
			shader = tr.defaultShader;
			for ( j = 0 ; j < skin->numSurfaces ; j++ ) {
				// the names have both been lowercased
				if ( !strcmp( skin->surfaces[j]->name, surface->name ) ) {
					shader = (shader_t *)skin->surfaces[j]->shader;
					break;
				}
			}
			if (shader == tr.defaultShader) {
				ri->Printf( PRINT_DEVELOPER, "WARNING: no shader for surface %s in skin %s\n", surface->name, skin->name);
			}
			else if (shader->defaultShader) {
				ri->Printf( PRINT_DEVELOPER, "WARNING: shader %s in skin %s not found\n", shader->name, skin->name);
			}
		//} else if ( surface->numShaders <= 0 ) {
			//shader = tr.defaultShader;
		} else {
			//md3Shader = (md3Shader_t *) ( (byte *)surface + surface->ofsShaders );
			//md3Shader += ent->e.skinNum % surface->numShaders;
			//shader = tr.shaders[ md3Shader->shaderIndex ];
			shader = tr.shaders[ surface->shaderIndexes[ ent->e.skinNum % surface->numShaderIndexes ] ];
		}

		// don't add third_person objects if not viewing through a portal
		if(!personalModel)
		{
			srfVBOMDVMesh_t *vboSurface = &model->vboSurfaces[i];
			R_AddDrawSurf((surfaceType_t *)vboSurface, shader, fogNum, qfalse, R_IsPostRenderEntity (tr.currentEntityNum, ent), cubemapIndex );
		}

		surface++;
	}
#endif //__MERGE_MD3_TEST__
}





