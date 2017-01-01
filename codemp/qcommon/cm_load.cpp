/*
===========================================================================
Copyright (C) 1999 - 2005, Id Software, Inc.
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
Copyright (C) 2013 - 2015, OpenJK contributors

This file is part of the OpenJK source code.

OpenJK is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, see <http://www.gnu.org/licenses/>.
===========================================================================
*/

// cmodel.c -- model loading
#include "cm_local.h"
#include "qcommon/qfiles.h"

void *ShaderData;
uint32_t ShaderDataCount;
void *LeafsData;
uint32_t LeafsDataCount;
void *LeafBrushesData;
uint32_t LeafBrushesDataCount;
void *LeafSurfacesData;
uint32_t LeafSurfacesDataCount;
void *PlanesData;
uint32_t PlanesDataCount;
void *BrushSidesData;
uint32_t BrushSidesDataCount;
void *BrushesData;
uint32_t BrushesDataCount;
void *SubmodelsData;
uint32_t SubmodelsDataCount;
void *NodesData;
uint32_t NodesDataCount;
void *EntityStringData;
uint32_t EntityStringDataCount;
void *VisibilityData;
uint32_t VisibilityDataClusterCount;
uint32_t VisibilityDataClusterBytesCount;
void *PatchesData;
uint32_t PatchesDataCount;

#ifdef BSPC

#include "../bspc/l_qfiles.h"

void SetPlaneSignbits (cplane_t *out) {
	int	bits, j;

	// for fast box on planeside test
	bits = 0;
	for (j=0 ; j<3 ; j++) {
		if (out->normal[j] < 0) {
			bits |= 1<<j;
		}
	}
	out->signbits = bits;
}
#endif //BSPC

// to allow boxes to be treated as brush models, we allocate
// some extra indexes along with those needed by the map
#define	BOX_BRUSHES		1
#define	BOX_SIDES		6
#define	BOX_LEAFS		2
#define	BOX_PLANES		12

#define	LL(x) x=LittleLong(x)


clipMap_t	cmg; //rwwRMG - changed from cm
int			c_pointcontents;
int			c_traces, c_brush_traces, c_patch_traces;


byte		*cmod_base;

#ifndef BSPC
cvar_t		*cm_noAreas;
cvar_t		*cm_noCurves;
cvar_t		*cm_playerCurveClip;
cvar_t		*cm_extraVerbose;
#endif

cmodel_t	box_model;
cplane_t	*box_planes;
cbrush_t	*box_brush;



void	CM_InitBoxHull (void);
void	CM_FloodAreaConnections (clipMap_t &cm);

//rwwRMG - added:
clipMap_t	SubBSP[MAX_SUB_BSP];
int			NumSubBSP, TotalSubModels;

/*
===============================================================================

					MAP LOADING

===============================================================================
*/

qboolean StringsContainWord(const char *heystack, const char *heystack2, char *needle)
{
	if (StringContainsWord(heystack, needle)) return qtrue;
	if (StringContainsWord(heystack2, needle)) return qtrue;
	return qfalse;
}

qboolean IsKnownShinyMap2(const char *heystack)
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

qboolean IsKnownShinyMap(const char *heystack)
{
	if (IsKnownShinyMap2(heystack))
	{
		return qtrue;
	}

	return qfalse;
}

qboolean HaveSurfaceType(int surfaceFlags)
{
	switch (surfaceFlags & MATERIAL_MASK)
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

int GetMaterialType(const char *name, int materialType)
{
	if (StringContainsWord(name, "gfx/2d")
		|| StringContainsWord(name, "gfx/console")
		|| StringContainsWord(name, "gfx/colors")
		|| StringContainsWord(name, "gfx/digits")
		|| StringContainsWord(name, "gfx/hud")
		|| StringContainsWord(name, "gfx/jkg")
		|| StringContainsWord(name, "gfx/menu"))
		return MATERIAL_NONE;

	if (!HaveSurfaceType(materialType))
	{
		//
		// Special cases - where we are pretty sure we want lots of specular and reflection...
		//
		if (StringsContainWord(name, name, "plastic") || StringsContainWord(name, name, "stormtrooper") || StringsContainWord(name, name, "snowtrooper") || StringsContainWord(name, name, "medpac") || StringsContainWord(name, name, "bacta") || StringsContainWord(name, name, "helmet") || StringsContainWord(name, name, "feather"))
			return MATERIAL_PLASTIC;
		else if (StringsContainWord(name, name, "/ships/") || StringsContainWord(name, name, "engine") || StringsContainWord(name, name, "mp/flag"))
			return MATERIAL_SOLIDMETAL;//MATERIAL_PLASTIC;
		else if (StringsContainWord(name, name, "wing") || StringsContainWord(name, name, "xwbody") || StringsContainWord(name, name, "tie_"))
			return MATERIAL_SOLIDMETAL;//MATERIAL_PLASTIC;
		else if (StringsContainWord(name, name, "ship") || StringsContainWord(name, name, "shuttle") || StringsContainWord(name, name, "falcon"))
			return MATERIAL_SOLIDMETAL;//MATERIAL_PLASTIC;
		else if (StringsContainWord(name, name, "freight") || StringsContainWord(name, name, "transport") || StringsContainWord(name, name, "crate"))
			return MATERIAL_SOLIDMETAL;//MATERIAL_PLASTIC;
		else if (StringsContainWord(name, name, "container") || StringsContainWord(name, name, "barrel") || StringsContainWord(name, name, "train"))
			return MATERIAL_SOLIDMETAL;//MATERIAL_PLASTIC;
		else if (StringsContainWord(name, name, "crane") || StringsContainWord(name, name, "plate") || StringsContainWord(name, name, "cargo"))
			return MATERIAL_SOLIDMETAL;//MATERIAL_PLASTIC;
		else if (StringsContainWord(name, name, "ship_"))
			return MATERIAL_SOLIDMETAL;//MATERIAL_PLASTIC;
		else if (!StringsContainWord(name, name, "trainer") && StringsContainWord(name, name, "train"))
			return MATERIAL_SOLIDMETAL;
		else if (StringsContainWord(name, name, "reborn") || StringsContainWord(name, name, "trooper"))
			return MATERIAL_ARMOR;
		else if (StringsContainWord(name, name, "boba") || StringsContainWord(name, name, "pilot"))
			return MATERIAL_ARMOR;
		else if (StringsContainWord(name, name, "water") && !StringsContainWord(name, name, "splash") && !StringsContainWord(name, name, "drip") && !StringsContainWord(name, name, "ripple") && !StringsContainWord(name, name, "bubble") && !StringsContainWord(name, name, "woosh") && !StringsContainWord(name, name, "underwater") && !StringsContainWord(name, name, "bottom"))
		{
			return MATERIAL_WATER;
		}
		else if (StringsContainWord(name, name, "grass") || StringsContainWord(name, name, "foliage") || StringsContainWord(name, name, "yavin/ground") || StringsContainWord(name, name, "mp/s_ground") || StringsContainWord(name, name, "volcano/terrain") || StringsContainWord(name, name, "bay/terrain") || StringsContainWord(name, name, "towers/terrain") || StringsContainWord(name, name, "yavinassault/terrain"))
			return MATERIAL_SHORTGRASS;
		//
		// Stuff we can be pretty sure of...
		//
		else if (StringsContainWord(name, name, "concrete"))
			return MATERIAL_CONCRETE;
		else if (StringsContainWord(name, name, "models/weapon") && StringsContainWord(name, name, "saber") && !StringsContainWord(name, name, "glow"))
			return MATERIAL_HOLLOWMETAL; // UQ1: Using hollowmetal for weapons to force low parallax setting...
		else if (StringsContainWord(name, name, "/weapon") || StringsContainWord(name, name, "scope") || StringsContainWord(name, name, "blaster") || StringsContainWord(name, name, "pistol") || StringsContainWord(name, name, "thermal") || StringsContainWord(name, name, "bowcaster") || StringsContainWord(name, name, "cannon") || StringsContainWord(name, name, "saber") || StringsContainWord(name, name, "rifle") || StringsContainWord(name, name, "rocket"))
			return MATERIAL_HOLLOWMETAL; // UQ1: Using hollowmetal for weapons to force low parallax setting...
		else if (StringsContainWord(name, name, "metal") || StringsContainWord(name, name, "pipe") || StringsContainWord(name, name, "shaft") || StringsContainWord(name, name, "jetpack") || StringsContainWord(name, name, "antenna") || StringsContainWord(name, name, "xwing") || StringsContainWord(name, name, "tie_") || StringsContainWord(name, name, "raven") || StringsContainWord(name, name, "falcon") || StringsContainWord(name, name, "engine") || StringsContainWord(name, name, "elevator") || StringsContainWord(name, name, "evaporator") || StringsContainWord(name, name, "airpur") || StringsContainWord(name, name, "gonk") || StringsContainWord(name, name, "droid") || StringsContainWord(name, name, "cart") || StringsContainWord(name, name, "vent") || StringsContainWord(name, name, "tank") || StringsContainWord(name, name, "transformer") || StringsContainWord(name, name, "generator") || StringsContainWord(name, name, "grate") || StringsContainWord(name, name, "rack") || StringsContainWord(name, name, "mech") || StringsContainWord(name, name, "turbolift") || StringsContainWord(name, name, "grate") || StringsContainWord(name, name, "tube") || StringsContainWord(name, name, "coil") || StringsContainWord(name, name, "vader_trim") || StringsContainWord(name, name, "newfloor_vjun") || StringsContainWord(name, name, "bay_beam"))
			return MATERIAL_SOLIDMETAL;
		else if (StringsContainWord(name, name, "eye"))
			return MATERIAL_GLASS;
		else if (StringsContainWord(name, name, "textures/byss/") && !StringsContainWord(name, name, "glow") && !StringsContainWord(name, name, "glw") && !StringsContainWord(name, name, "static") && !StringsContainWord(name, name, "isd") && !StringsContainWord(name, name, "power") && !StringsContainWord(name, name, "env_") && !StringsContainWord(name, name, "byss_switch"))
			return MATERIAL_SOLIDMETAL; // special for byss shiny
		else if (StringsContainWord(name, name, "textures/vjun/") && !StringsContainWord(name, name, "glow") && !StringsContainWord(name, name, "glw") && !StringsContainWord(name, name, "static") && !StringsContainWord(name, name, "light") && !StringsContainWord(name, name, "env_") && !StringsContainWord(name, name, "_env") && !StringsContainWord(name, name, "switch_off") && !StringsContainWord(name, name, "switch_on") && !StringsContainWord(name, name, "screen") && !StringsContainWord(name, name, "blend") && !StringsContainWord(name, name, "o_ground") && !StringsContainWord(name, name, "_onoffg") && !StringsContainWord(name, name, "_onoffr") && !StringsContainWord(name, name, "console"))
			return MATERIAL_SOLIDMETAL; // special for vjun shiny
		else if (StringsContainWord(name, name, "sand"))
			return MATERIAL_SAND;
		else if (StringsContainWord(name, name, "gravel"))
			return MATERIAL_GRAVEL;
		else if ((StringsContainWord(name, name, "dirt") || StringsContainWord(name, name, "ground")) && !StringsContainWord(name, name, "menus/main_background"))
			return MATERIAL_DIRT;
		else if (IsKnownShinyMap(name) && StringsContainWord(name, name, "stucco"))
			return MATERIAL_TILES;
		else if (StringsContainWord(name, name, "rift") && StringsContainWord(name, name, "piller"))
			return MATERIAL_MARBLE;
		else if (StringsContainWord(name, name, "stucco") || StringsContainWord(name, name, "piller") || StringsContainWord(name, name, "sith_jp"))
			return MATERIAL_CONCRETE;
		else if (StringsContainWord(name, name, "marbl") || StringsContainWord(name, name, "teeth"))
			return MATERIAL_MARBLE;
		else if (StringsContainWord(name, name, "snow"))
			return MATERIAL_SNOW;
		else if (StringsContainWord(name, name, "hood") || StringsContainWord(name, name, "robe") || StringsContainWord(name, name, "cloth") || StringsContainWord(name, name, "pants"))
			return MATERIAL_FABRIC;
		else if (StringsContainWord(name, name, "hair") || StringsContainWord(name, name, "chewbacca")) // use carpet
			return MATERIAL_FABRIC;//MATERIAL_CARPET; Just because it has a bit of parallax and suitable specular...
		else if (StringsContainWord(name, name, "armor") || StringsContainWord(name, name, "armour"))
			return MATERIAL_ARMOR;
		else if (StringsContainWord(name, name, "flesh") || StringsContainWord(name, name, "body") || StringsContainWord(name, name, "leg") || StringsContainWord(name, name, "hand") || StringsContainWord(name, name, "head") || StringsContainWord(name, name, "hips") || StringsContainWord(name, name, "torso") || StringsContainWord(name, name, "tentacles") || StringsContainWord(name, name, "face") || StringsContainWord(name, name, "arms"))
			return MATERIAL_FLESH;
		else if (StringsContainWord(name, name, "players") && (StringsContainWord(name, name, "skirt") || StringsContainWord(name, name, "boots") || StringsContainWord(name, name, "accesories") || StringsContainWord(name, name, "accessories") || StringsContainWord(name, name, "vest") || StringsContainWord(name, name, "holster") || StringsContainWord(name, name, "cap")))
			return MATERIAL_FABRIC;
		else if (StringsContainWord(name, name, "canvas"))
			return MATERIAL_CANVAS;
		else if (StringsContainWord(name, name, "rock"))
			return MATERIAL_ROCK;
		else if (StringsContainWord(name, name, "rubber"))
			return MATERIAL_RUBBER;
		else if (StringsContainWord(name, name, "carpet"))
			return MATERIAL_CARPET;
		else if (StringsContainWord(name, name, "plaster"))
			return MATERIAL_PLASTER;
		else if (StringsContainWord(name, name, "computer") || StringsContainWord(name, name, "console") || StringsContainWord(name, name, "button") || StringsContainWord(name, name, "terminal") || StringsContainWord(name, name, "switch") || StringsContainWord(name, name, "panel") || StringsContainWord(name, name, "control"))
			return MATERIAL_COMPUTER;
		else if (StringsContainWord(name, name, "fabric"))
			return MATERIAL_FABRIC;
		else if (StringsContainWord(name, name, "tree") || StringsContainWord(name, name, "leaf") || StringsContainWord(name, name, "leaves") || StringsContainWord(name, name, "fern") || StringsContainWord(name, name, "vine"))
			return MATERIAL_GREENLEAVES;
		else if (StringsContainWord(name, name, "wood") && !StringsContainWord(name, name, "street"))
			return MATERIAL_SOLIDWOOD;
		else if (StringsContainWord(name, name, "mud"))
			return MATERIAL_MUD;
		else if (StringsContainWord(name, name, "ice"))
			return MATERIAL_ICE;
		else if ((StringsContainWord(name, name, "grass") || StringsContainWord(name, name, "foliage")) && (StringsContainWord(name, name, "long") || StringsContainWord(name, name, "tall") || StringsContainWord(name, name, "thick")))
			return MATERIAL_LONGGRASS;
		else if (StringsContainWord(name, name, "grass") || StringsContainWord(name, name, "foliage"))
			return MATERIAL_SHORTGRASS;
		else if (IsKnownShinyMap(name) && StringsContainWord(name, name, "floor"))
			return MATERIAL_TILES;
		else if (!StringsContainWord(name, name, "players") && (StringsContainWord(name, name, "bespin") || StringsContainWord(name, name, "_cc")))
			return MATERIAL_MARBLE;
		else if (!StringsContainWord(name, name, "players") && (StringsContainWord(name, name, "coruscant") || StringsContainWord(name, name, "/rooftop/") || StringsContainWord(name, name, "/nar_") || StringsContainWord(name, name, "/imperial/")))
			return MATERIAL_TILES;
		else if (StringsContainWord(name, name, "floor"))
			return MATERIAL_CONCRETE;
		else if (StringsContainWord(name, name, "textures/mp/") && !StringsContainWord(name, name, "glow") && !StringsContainWord(name, name, "glw") && !StringsContainWord(name, name, "static") && !StringsContainWord(name, name, "light") && !StringsContainWord(name, name, "env_") && !StringsContainWord(name, name, "_env") && !StringsContainWord(name, name, "underside") && !StringsContainWord(name, name, "blend") && !StringsContainWord(name, name, "t_pit") && !StringsContainWord(name, name, "desert") && !StringsContainWord(name, name, "cliff") && !StringsContainWord(name, name, "t_pit"))
			return MATERIAL_SOLIDMETAL; // special for mp shiny
		else if (IsKnownShinyMap(name) && StringsContainWord(name, name, "frame"))
			return MATERIAL_SOLIDMETAL;
		else if (IsKnownShinyMap(name) && StringsContainWord(name, name, "wall"))
			return MATERIAL_SOLIDMETAL;
		else if (StringsContainWord(name, name, "wall") || StringsContainWord(name, name, "underside"))
			return MATERIAL_CONCRETE;
		else if (IsKnownShinyMap(name) && StringsContainWord(name, name, "door"))
			return MATERIAL_SOLIDMETAL;
		else if (StringsContainWord(name, name, "door"))
			return MATERIAL_CONCRETE;
		else if (IsKnownShinyMap(name) && StringsContainWord(name, name, "ground"))
			return MATERIAL_TILES; // dunno about this one
		else if (StringsContainWord(name, name, "ground"))
			return MATERIAL_CONCRETE;
		else if (StringsContainWord(name, name, "desert"))
			return MATERIAL_CONCRETE;
		else if (IsKnownShinyMap(name) && (StringsContainWord(name, name, "tile") || StringsContainWord(name, name, "lift")))
			return MATERIAL_SOLIDMETAL;
		else if (StringsContainWord(name, name, "tile") || StringsContainWord(name, name, "lift"))
			return MATERIAL_TILES;
		else if (StringsContainWord(name, name, "glass") || StringsContainWord(name, name, "light") || StringsContainWord(name, name, "screen") || StringsContainWord(name, name, "lamp") || StringsContainWord(name, name, "crystal"))
			return MATERIAL_GLASS;
		else if (StringsContainWord(name, name, "flag"))
			return MATERIAL_FABRIC;
		else if (StringsContainWord(name, name, "column") || StringsContainWord(name, name, "stone") || StringsContainWord(name, name, "statue"))
			return MATERIAL_MARBLE;
		// Extra backup - backup stuff. Used when nothing better found...
		else if (StringsContainWord(name, name, "red") || StringsContainWord(name, name, "blue") || StringsContainWord(name, name, "yellow") || StringsContainWord(name, name, "white") || StringsContainWord(name, name, "monitor"))
			return MATERIAL_PLASTIC;
		else if (StringsContainWord(name, name, "yavin") && (StringsContainWord(name, name, "trim") || StringsContainWord(name, name, "step") || StringsContainWord(name, name, "pad")))
			return MATERIAL_ROCK;
		else if (!StringsContainWord(name, name, "players") && (StringsContainWord(name, name, "deathstar") || StringsContainWord(name, name, "imperial") || StringsContainWord(name, name, "shuttle") || StringsContainWord(name, name, "destroyer")))
			return MATERIAL_TILES;
		else if (!StringsContainWord(name, name, "players") && StringsContainWord(name, name, "dantooine"))
			return MATERIAL_MARBLE;
		else if (StringsContainWord(name, name, "outside"))
			return MATERIAL_CONCRETE; // Outside, assume concrete...
		else if (StringsContainWord(name, name, "out") && (StringsContainWord(name, name, "trim") || StringsContainWord(name, name, "step") || StringsContainWord(name, name, "pad")))
			return MATERIAL_CONCRETE; // Outside, assume concrete...
		else if (StringsContainWord(name, name, "out") && (StringsContainWord(name, name, "frame") || StringsContainWord(name, name, "wall") || StringsContainWord(name, name, "round") || StringsContainWord(name, name, "crate") || StringsContainWord(name, name, "trim") || StringsContainWord(name, name, "support") || StringsContainWord(name, name, "step") || StringsContainWord(name, name, "pad") || StringsContainWord(name, name, "weapon") || StringsContainWord(name, name, "gun")))
			return MATERIAL_CONCRETE; // Outside, assume concrete...
		else if (StringsContainWord(name, name, "frame") || StringsContainWord(name, name, "wall") || StringsContainWord(name, name, "round") || StringsContainWord(name, name, "crate") || StringsContainWord(name, name, "trim") || StringsContainWord(name, name, "support") || StringsContainWord(name, name, "step") || StringsContainWord(name, name, "pad") || StringsContainWord(name, name, "weapon") || StringsContainWord(name, name, "gun"))
			return MATERIAL_CONCRETE;
		else if (StringsContainWord(name, name, "yavin"))
			return MATERIAL_ROCK; // On yavin maps, assume rock for anything else...
		else if (StringsContainWord(name, name, "black") || StringsContainWord(name, name, "boon") || StringsContainWord(name, name, "items") || StringsContainWord(name, name, "shield"))
			return MATERIAL_PLASTIC;
		else if (StringsContainWord(name, name, "refract") || StringsContainWord(name, name, "reflect"))
			return MATERIAL_PLASTIC;
		else if (StringsContainWord(name, name, "map_objects"))
			return MATERIAL_SOLIDMETAL; // hmmm, maybe... testing...
		else if (StringsContainWord(name, name, "rodian"))
			return MATERIAL_FLESH;
		else if (IsKnownShinyMap(name)) // Chances are it's shiny...
			return MATERIAL_TILES;
	}
	else
	{
		//
		// Special cases - where we are pretty sure we want lots of specular and reflection... Override!
		//
		if (StringsContainWord(name, name, "plastic") || StringsContainWord(name, name, "stormtrooper") || StringsContainWord(name, name, "snowtrooper") || StringsContainWord(name, name, "medpac") || StringsContainWord(name, name, "bacta") || StringsContainWord(name, name, "helmet"))
			return MATERIAL_PLASTIC;
		else if (StringsContainWord(name, name, "/ships/") || StringsContainWord(name, name, "engine") || StringsContainWord(name, name, "mp/flag"))
			return MATERIAL_SOLIDMETAL;//MATERIAL_PLASTIC;
		else if (StringsContainWord(name, name, "wing") || StringsContainWord(name, name, "xwbody") || StringsContainWord(name, name, "tie_"))
			return MATERIAL_SOLIDMETAL;//MATERIAL_PLASTIC;
		else if (StringsContainWord(name, name, "ship") || StringsContainWord(name, name, "shuttle") || StringsContainWord(name, name, "falcon"))
			return MATERIAL_SOLIDMETAL;//MATERIAL_PLASTIC;
		else if (StringsContainWord(name, name, "freight") || StringsContainWord(name, name, "transport") || StringsContainWord(name, name, "crate"))
			return MATERIAL_SOLIDMETAL;//MATERIAL_PLASTIC;
		else if (StringsContainWord(name, name, "container") || StringsContainWord(name, name, "barrel") || StringsContainWord(name, name, "train"))
			return MATERIAL_SOLIDMETAL;//MATERIAL_PLASTIC;
		else if (StringsContainWord(name, name, "crane") || StringsContainWord(name, name, "plate") || StringsContainWord(name, name, "cargo"))
			return MATERIAL_SOLIDMETAL;//MATERIAL_PLASTIC;
		else if (StringsContainWord(name, name, "ship_"))
			return MATERIAL_SOLIDMETAL;//MATERIAL_PLASTIC;
		else if (StringsContainWord(name, name, "reborn") || StringsContainWord(name, name, "trooper"))
			return MATERIAL_ARMOR;
		else if (StringsContainWord(name, name, "boba") || StringsContainWord(name, name, "pilot"))
			return MATERIAL_ARMOR;
		else if (StringsContainWord(name, name, "grass") || (StringsContainWord(name, name, "foliage") && !StringsContainWord(name, name, "billboard")) || StringsContainWord(name, name, "yavin/ground") || StringsContainWord(name, name, "mp/s_ground") || StringsContainWord(name, name, "yavinassault/terrain"))
			return MATERIAL_SHORTGRASS;
	}

	if (StringsContainWord(name, name, "common/water") && !StringsContainWord(name, name, "splash") && !StringsContainWord(name, name, "drip") && !StringsContainWord(name, name, "ripple") && !StringsContainWord(name, name, "bubble") && !StringsContainWord(name, name, "woosh") && !StringsContainWord(name, name, "underwater") && !StringsContainWord(name, name, "bottom"))
	{
		return MATERIAL_WATER;
	}
	else if (!StringsContainWord(name, name, "billboard") &&
		(StringsContainWord(name, name, "grass") || StringsContainWord(name, name, "foliage") || StringsContainWord(name, name, "yavin/ground")
			|| StringsContainWord(name, name, "mp/s_ground") || StringsContainWord(name, name, "yavinassault/terrain")
			|| StringsContainWord(name, name, "tree") || StringsContainWord(name, name, "plant") || StringsContainWord(name, name, "bush")
			|| StringsContainWord(name, name, "shrub") || StringsContainWord(name, name, "leaf") || StringsContainWord(name, name, "leaves")
			|| StringsContainWord(name, name, "branch") || StringsContainWord(name, name, "flower") || StringsContainWord(name, name, "weed")))
	{// Always greenleaves... No parallax...
		return MATERIAL_GREENLEAVES;
	}
	else if (StringsContainWord(name, name, "plastic") || StringsContainWord(name, name, "trooper") || StringsContainWord(name, name, "medpack"))
		if (!(materialType & MATERIAL_PLASTIC)) return MATERIAL_PLASTIC;
		else if (StringsContainWord(name, name, "grass") || (StringsContainWord(name, name, "foliage") && !StringsContainWord(name, name, "billboard")) || StringsContainWord(name, name, "yavin/ground")
			|| StringsContainWord(name, name, "mp/s_ground") || StringsContainWord(name, name, "yavinassault/terrain"))
			if (!(materialType & MATERIAL_SHORTGRASS)) return MATERIAL_SHORTGRASS;

	return MATERIAL_NONE;
}

/*
=================
CMod_LoadShaders
=================
*/
static void CMod_LoadShaders(lump_t *l, clipMap_t &cm)
{
	dshader_t	*in;
	int			i, count;
	CCMShader	*out;

	in = (dshader_t *)(cmod_base + l->fileofs);
	if (l->filelen % sizeof(*in)) {
		Com_Error(ERR_DROP, "CMod_LoadShaders: funny lump size");
	}
	count = l->filelen / sizeof(*in);

	if (count < 1) {
		Com_Error(ERR_DROP, "Map with no shaders");
	}
	cm.shaders = (CCMShader *)Hunk_Alloc((1 + count) * sizeof(*cm.shaders), h_high);
	cm.numShaders = count;

	ShaderData = cm.shaders;
	ShaderDataCount = cm.numShaders;

	out = cm.shaders;
	for (i = 0; i < count; i++, in++, out++)
	{
		Q_strncpyz(out->shader, in->shader, MAX_QPATH);
		out->contentFlags = LittleLong(in->contentFlags);
		out->surfaceFlags = LittleLong(in->surfaceFlags);

		if (in->shader && (StringContainsWord(in->shader, "warzone/tree") || StringContainsWord(in->shader, "warzone\\tree")))
		{// LOL WTF HAX!!! :)
			if (StringContainsWord(in->shader, "bark")
				|| StringContainsWord(in->shader, "trunk")
				|| StringContainsWord(in->shader, "giant_tree")
				|| StringContainsWord(in->shader, "vine01"))
			{
				in->contentFlags |= (CONTENTS_SOLID | CONTENTS_OPAQUE);
				out->contentFlags = LittleLong(in->contentFlags);
				out->surfaceFlags = LittleLong(in->surfaceFlags);
			}
		}

		if (!HaveSurfaceType(out->surfaceFlags))
			out->surfaceFlags = LittleLong(GetMaterialType(in->shader, (in->surfaceFlags & MATERIAL_MASK)));

		if (in->shader && (StringContainsWord(in->shader, "skies/") || StringContainsWord(in->shader, "sky")))
		{// LOL WTF HAX!!! :)
			in->contentFlags |= (CONTENTS_SOLID | CONTENTS_OPAQUE);
			in->surfaceFlags |= SURF_SKY;
			out->contentFlags = LittleLong(in->contentFlags);
			out->surfaceFlags = LittleLong(in->surfaceFlags);
			Com_Printf("Set SURF_SKY for %s.\n", in->shader);
		}
	}
}


/*
=================
CMod_LoadSubmodels
=================
*/
void CMod_LoadSubmodels(lump_t *l, clipMap_t &cm) {
	dmodel_t	*in;
	cmodel_t	*out;
	int			i, j, count;
	int			*indexes;

	in = (dmodel_t *)(cmod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Com_Error(ERR_DROP, "CMod_LoadSubmodels: funny lump size");
	count = l->filelen / sizeof(*in);

	if (count < 1)
		Com_Error(ERR_DROP, "Map with no models");
	cm.cmodels = (struct cmodel_s *)Hunk_Alloc(count * sizeof(*cm.cmodels), h_high);
	cm.numSubModels = count;

	SubmodelsData = cm.cmodels;
	SubmodelsDataCount = cm.numSubModels;

	if (count > MAX_SUBMODELS) {
		Com_Error(ERR_DROP, "MAX_SUBMODELS exceeded");
	}

	for (i = 0; i<count; i++, in++, out++)
	{
		out = &cm.cmodels[i];

		for (j = 0; j<3; j++)
		{	// spread the mins / maxs by a pixel
			out->mins[j] = LittleFloat(in->mins[j]) - 1;
			out->maxs[j] = LittleFloat(in->maxs[j]) + 1;
		}

		//rwwRMG - sof2 doesn't have to add this &cm == &cmg check.
		//Are they getting leaf data elsewhere? (the reason this needs to be done is
		//in sub bsp instances the first brush model isn't necessary a world model and might be
		//real architecture)
		if (i == 0 && &cm == &cmg) {
			out->firstNode = 0;
			continue;	// world model doesn't need other info
		}

		// make a "leaf" just to hold the model's brushes and surfaces
		out->firstNode = -1;

		// make a "leaf" just to hold the model's brushes and surfaces
		out->leaf.numLeafBrushes = LittleLong(in->numBrushes);
		indexes = (int *)Hunk_Alloc(out->leaf.numLeafBrushes * 4, h_high);
		out->leaf.firstLeafBrush = indexes - cm.leafbrushes;
		for (j = 0; j < out->leaf.numLeafBrushes; j++) {
			indexes[j] = LittleLong(in->firstBrush) + j;
		}

		out->leaf.numLeafSurfaces = LittleLong(in->numSurfaces);
		indexes = (int *)Hunk_Alloc(out->leaf.numLeafSurfaces * 4, h_high);
		out->leaf.firstLeafSurface = indexes - cm.leafsurfaces;
		for (j = 0; j < out->leaf.numLeafSurfaces; j++) {
			indexes[j] = LittleLong(in->firstSurface) + j;
		}
	}
}


/*
=================
CMod_LoadNodes

=================
*/
void CMod_LoadNodes(lump_t *l, clipMap_t &cm) {
	dnode_t		*in;
	int			child;
	cNode_t		*out;
	int			i, j, count;

	in = (dnode_t *)(cmod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Com_Error(ERR_DROP, "MOD_LoadBmodel: funny lump size");
	count = l->filelen / sizeof(*in);

	if (count < 1)
		Com_Error(ERR_DROP, "Map has no nodes");
	cm.nodes = (cNode_t *)Hunk_Alloc(count * sizeof(*cm.nodes), h_high);
	cm.numNodes = count;

	NodesData = cm.nodes;
	NodesDataCount = cm.numNodes;

	out = cm.nodes;

	for (i = 0; i<count; i++, out++, in++)
	{
		out->plane = cm.planes + LittleLong(in->planeNum);
		for (j = 0; j<2; j++)
		{
			child = LittleLong(in->children[j]);
			out->children[j] = child;
		}
	}

}

/*
=================
CM_BoundBrush

=================
*/
void CM_BoundBrush(cbrush_t *b) {
	b->bounds[0][0] = -b->sides[0].plane->dist;
	b->bounds[1][0] = b->sides[1].plane->dist;

	b->bounds[0][1] = -b->sides[2].plane->dist;
	b->bounds[1][1] = b->sides[3].plane->dist;

	b->bounds[0][2] = -b->sides[4].plane->dist;
	b->bounds[1][2] = b->sides[5].plane->dist;
}


/*
=================
CMod_LoadBrushes

=================
*/
void CMod_LoadBrushes(lump_t *l, clipMap_t	&cm) {
	dbrush_t	*in;
	cbrush_t	*out;
	int			i, count;

	in = (dbrush_t *)(cmod_base + l->fileofs);
	if (l->filelen % sizeof(*in)) {
		Com_Error(ERR_DROP, "MOD_LoadBmodel: funny lump size");
	}
	count = l->filelen / sizeof(*in);

	cm.brushes = (cbrush_t *)Hunk_Alloc((BOX_BRUSHES + count) * sizeof(*cm.brushes), h_high);
	cm.numBrushes = count;

	BrushesData = cm.brushes;
	BrushesDataCount = cm.numBrushes;

	out = cm.brushes;

	for (i = 0; i<count; i++, out++, in++) {
		out->sides = cm.brushsides + LittleLong(in->firstSide);
		out->numsides = LittleLong(in->numSides);

		out->shaderNum = LittleLong(in->shaderNum);
		if (out->shaderNum < 0 || out->shaderNum >= cm.numShaders) {
			Com_Error(ERR_DROP, "CMod_LoadBrushes: bad shaderNum: %i", out->shaderNum);
		}
		out->contents = cm.shaders[out->shaderNum].contentFlags;

		CM_BoundBrush(out);
	}

}

/*
=================
CMod_LoadLeafs
=================
*/
void CMod_LoadLeafs(lump_t *l, clipMap_t &cm)
{
	int			i;
	cLeaf_t		*out;
	dleaf_t 	*in;
	int			count;

	in = (dleaf_t *)(cmod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Com_Error(ERR_DROP, "MOD_LoadBmodel: funny lump size");
	count = l->filelen / sizeof(*in);

	if (count < 1)
		Com_Error(ERR_DROP, "Map with no leafs");

	cm.leafs = (cLeaf_t *)Hunk_Alloc((BOX_LEAFS + count) * sizeof(*cm.leafs), h_high);
	cm.numLeafs = count;

	LeafsData = cm.leafs;
	LeafsDataCount = cm.numLeafs;

	out = cm.leafs;
	for (i = 0; i<count; i++, in++, out++)
	{
		out->cluster = LittleLong(in->cluster);
		out->area = LittleLong(in->area);
		out->firstLeafBrush = LittleLong(in->firstLeafBrush);
		out->numLeafBrushes = LittleLong(in->numLeafBrushes);
		out->firstLeafSurface = LittleLong(in->firstLeafSurface);
		out->numLeafSurfaces = LittleLong(in->numLeafSurfaces);

		if (out->cluster >= cm.numClusters)
			cm.numClusters = out->cluster + 1;
		if (out->area >= cm.numAreas)
			cm.numAreas = out->area + 1;
	}

	cm.areas = (cArea_t *)Hunk_Alloc(cm.numAreas * sizeof(*cm.areas), h_high);
	cm.areaPortals = (int *)Hunk_Alloc(cm.numAreas * cm.numAreas * sizeof(*cm.areaPortals), h_high);
}

/*
=================
CMod_LoadPlanes
=================
*/
void CMod_LoadPlanes(lump_t *l, clipMap_t &cm)
{
	int			i, j;
	cplane_t	*out;
	dplane_t 	*in;
	int			count;
	int			bits;

	in = (dplane_t *)(cmod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Com_Error(ERR_DROP, "MOD_LoadBmodel: funny lump size");
	count = l->filelen / sizeof(*in);

	if (count < 1)
		Com_Error(ERR_DROP, "Map with no planes");
	cm.planes = (struct cplane_s *)Hunk_Alloc((BOX_PLANES + count) * sizeof(*cm.planes), h_high);
	cm.numPlanes = count;

	PlanesData = cm.planes;
	PlanesDataCount = cm.numPlanes;

	out = cm.planes;

	for (i = 0; i<count; i++, in++, out++)
	{
		bits = 0;
		for (j = 0; j<3; j++)
		{
			out->normal[j] = LittleFloat(in->normal[j]);
			if (out->normal[j] < 0)
				bits |= 1 << j;
		}

		out->dist = LittleFloat(in->dist);
		out->type = PlaneTypeForNormal(out->normal);
		out->signbits = bits;
	}
}

/*
=================
CMod_LoadLeafBrushes
=================
*/
void CMod_LoadLeafBrushes(lump_t *l, clipMap_t	&cm)
{
	int			i;
	int			*out;
	int		 	*in;
	int			count;

	in = (int *)(cmod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Com_Error(ERR_DROP, "MOD_LoadBmodel: funny lump size");
	count = l->filelen / sizeof(*in);

	cm.leafbrushes = (int *)Hunk_Alloc((count + BOX_BRUSHES) * sizeof(*cm.leafbrushes), h_high);
	cm.numLeafBrushes = count;

	LeafBrushesData = cm.leafbrushes;
	LeafBrushesDataCount = cm.numLeafBrushes;

	out = cm.leafbrushes;

	for (i = 0; i<count; i++, in++, out++) {
		*out = LittleLong(*in);
	}
}

/*
=================
CMod_LoadLeafSurfaces
=================
*/
void CMod_LoadLeafSurfaces(lump_t *l, clipMap_t &cm)
{
	int			i;
	int			*out;
	int		 	*in;
	int			count;

	in = (int *)(cmod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Com_Error(ERR_DROP, "MOD_LoadBmodel: funny lump size");
	count = l->filelen / sizeof(*in);

	cm.leafsurfaces = (int *)Hunk_Alloc(count * sizeof(*cm.leafsurfaces), h_high);
	cm.numLeafSurfaces = count;

	LeafSurfacesData = cm.leafsurfaces;
	LeafSurfacesDataCount = cm.numLeafSurfaces;

	out = cm.leafsurfaces;

	for (i = 0; i<count; i++, in++, out++) {
		*out = LittleLong(*in);
	}
}

/*
=================
CMod_LoadBrushSides
=================
*/
void CMod_LoadBrushSides(lump_t *l, clipMap_t &cm)
{
	int				i;
	cbrushside_t	*out;
	dbrushside_t 	*in;
	int				count;
	int				num;

	in = (dbrushside_t *)(cmod_base + l->fileofs);
	if (l->filelen % sizeof(*in)) {
		Com_Error(ERR_DROP, "MOD_LoadBmodel: funny lump size");
	}
	count = l->filelen / sizeof(*in);

	cm.brushsides = (cbrushside_t *)Hunk_Alloc((BOX_SIDES + count) * sizeof(*cm.brushsides), h_high);
	cm.numBrushSides = count;

	BrushSidesData = cm.brushsides;
	BrushSidesDataCount = cm.numBrushSides;

	out = cm.brushsides;

	for (i = 0; i<count; i++, in++, out++) {
		num = LittleLong(in->planeNum);
		out->plane = &cm.planes[num];
		out->shaderNum = LittleLong(in->shaderNum);
		if (out->shaderNum < 0 || out->shaderNum >= cm.numShaders) {
			Com_Error(ERR_DROP, "CMod_LoadBrushSides: bad shaderNum: %i", out->shaderNum);
		}
	}
}


/*
=================
CMod_LoadEntityString
=================
*/
void CMod_LoadEntityString(lump_t *l, clipMap_t &cm) {
	cm.entityString = (char *)Hunk_Alloc(l->filelen, h_high);
	cm.numEntityChars = l->filelen;
	Com_Memcpy(cm.entityString, cmod_base + l->fileofs, l->filelen);

	EntityStringData = cm.entityString;
	EntityStringDataCount = cm.numEntityChars;
}

/*
=================
CMod_LoadVisibility
=================
*/
#define	VIS_HEADER	8
void CMod_LoadVisibility(lump_t *l, clipMap_t &cm) {
	int		len;
	byte	*buf;

	len = l->filelen;
	if (!len) {
		cm.clusterBytes = (cm.numClusters + 31) & ~31;
		cm.visibility = (unsigned char *)Hunk_Alloc(cm.clusterBytes, h_high);
		Com_Memset(cm.visibility, 255, cm.clusterBytes);
		return;
	}
	buf = cmod_base + l->fileofs;

	cm.vised = qtrue;
	cm.visibility = (unsigned char *)Hunk_Alloc(len, h_high);
	cm.numClusters = LittleLong(((int *)buf)[0]);
	cm.clusterBytes = LittleLong(((int *)buf)[1]);
	Com_Memcpy(cm.visibility, buf + VIS_HEADER, len - VIS_HEADER);

	VisibilityData = cm.visibility;
	VisibilityDataClusterCount = cm.numClusters;
	VisibilityDataClusterBytesCount = cm.clusterBytes;
}

//==================================================================


/*
=================
CMod_LoadPatches
=================
*/
#define	MAX_PATCH_VERTS		1024
void CMod_LoadPatches(lump_t *surfs, lump_t *verts, clipMap_t &cm) {
	drawVert_t	*dv, *dv_p;
	dsurface_t	*in;
	int			count;
	int			i, j;
	int			c;
	cPatch_t	*patch;
	vec3_t		points[MAX_PATCH_VERTS];
	int			width, height;
	int			shaderNum;

	in = (dsurface_t *)(cmod_base + surfs->fileofs);
	if (surfs->filelen % sizeof(*in))
		Com_Error(ERR_DROP, "MOD_LoadBmodel: funny lump size");
	cm.numSurfaces = count = surfs->filelen / sizeof(*in);
	cm.surfaces = (cPatch_t **)Hunk_Alloc(cm.numSurfaces * sizeof(cm.surfaces[0]), h_high);

	PatchesData = cm.surfaces;
	PatchesDataCount = cm.numSurfaces;

	dv = (drawVert_t *)(cmod_base + verts->fileofs);
	if (verts->filelen % sizeof(*dv))
		Com_Error(ERR_DROP, "MOD_LoadBmodel: funny lump size");

	// scan through all the surfaces, but only load patches,
	// not planar faces
	for (i = 0; i < count; i++, in++) {
		if (LittleLong(in->surfaceType) != MST_PATCH) {
			continue;		// ignore other surfaces
		}
		// FIXME: check for non-colliding patches

		cm.surfaces[i] = patch = (cPatch_t *)Hunk_Alloc(sizeof(*patch), h_high);

		// load the full drawverts onto the stack
		width = LittleLong(in->patchWidth);
		height = LittleLong(in->patchHeight);
		c = width * height;
		if (c > MAX_PATCH_VERTS) {
			Com_Error(ERR_DROP, "ParseMesh: MAX_PATCH_VERTS");
		}

		dv_p = dv + LittleLong(in->firstVert);
		for (j = 0; j < c; j++, dv_p++) {
			points[j][0] = LittleFloat(dv_p->xyz[0]);
			points[j][1] = LittleFloat(dv_p->xyz[1]);
			points[j][2] = LittleFloat(dv_p->xyz[2]);
		}

		shaderNum = LittleLong(in->shaderNum);
		patch->contents = cm.shaders[shaderNum].contentFlags;
		patch->surfaceFlags = cm.shaders[shaderNum].surfaceFlags;

		// create the internal facet structure
		patch->pc = CM_GeneratePatchCollide(width, height, points);
	}
}

//==================================================================

/*
==================
CM_LoadMap

Loads in the map and all submodels
==================
*/
void *gpvCachedMapDiskImage = NULL;
char  gsCachedMapDiskImage[MAX_QPATH];
qboolean gbUsingCachedMapDataRightNow = qfalse;	// if true, signifies that you can't delete this at the moment!! (used during z_malloc()-fail recovery attempt)

// called in response to a "devmapbsp blah" or "devmapall blah" command, do NOT use inside CM_Load unless you pass in qtrue
//
// new bool return used to see if anything was freed, used during z_malloc failure re-try
//
qboolean CM_DeleteCachedMap(qboolean bGuaranteedOkToDelete)
{
	qboolean bActuallyFreedSomething = qfalse;

	if (bGuaranteedOkToDelete || !gbUsingCachedMapDataRightNow)
	{
		// dump cached disk image...
		//
		if (gpvCachedMapDiskImage)
		{
			Z_Free(	gpvCachedMapDiskImage );
					gpvCachedMapDiskImage = NULL;

			bActuallyFreedSomething = qtrue;
		}
		gsCachedMapDiskImage[0] = '\0';

		// force map loader to ignore cached internal BSP structures for next level CM_LoadMap() call...
		//
		cmg.name[0] = '\0';
	}

	return bActuallyFreedSomething;
}





static void CM_LoadMap_Actual( const char *name, qboolean clientload, int *checksum, clipMap_t &cm )
{ //rwwRMG - function needs heavy modification
	int				*buf;
	dheader_t		header;
	static unsigned	last_checksum;
	char			origName[MAX_OSPATH];
	void			*newBuff = 0;

	if ( !name || !name[0] ) {
		Com_Error( ERR_DROP, "CM_LoadMap: NULL name" );
	}

#ifndef BSPC
	cm_noAreas = Cvar_Get ("cm_noAreas", "0", CVAR_CHEAT);
	cm_noCurves = Cvar_Get ("cm_noCurves", "0", CVAR_CHEAT);
	cm_playerCurveClip = Cvar_Get ("cm_playerCurveClip", "1", CVAR_ARCHIVE|CVAR_CHEAT );
	cm_extraVerbose = Cvar_Get ("cm_extraVerbose", "0", CVAR_TEMP );
#endif
	Com_DPrintf( "CM_LoadMap( %s, %i )\n", name, clientload );

	if ( !strcmp( cm.name, name ) && clientload ) {
		if ( checksum )
			*checksum = last_checksum;
		return;
	}

	strcpy(origName, name);

	if (&cm == &cmg)
	{
		// free old stuff
		CM_ClearMap();
		CM_ClearLevelPatches();
	}

	// free old stuff
	Com_Memset( &cm, 0, sizeof( cm ) );

	if ( !name[0] ) {
		cm.numLeafs = 1;
		cm.numClusters = 1;
		cm.numAreas = 1;
		cm.cmodels = (struct cmodel_s *)Hunk_Alloc( sizeof( *cm.cmodels ), h_high );
		if ( checksum )
			*checksum = 0;
		return;
	}

	//
	// load the file
	//
	//rww - Doesn't this sort of defeat the purpose? We're clearing it even if the map is the same as the last one!
	//Not touching it though in case I'm just overlooking something.
	if (gpvCachedMapDiskImage && &cm == &cmg)	// MP code: this'll only be NZ if we got an ERR_DROP during last map load,
	{							//	so it's really just a safety measure.
		Z_Free(	gpvCachedMapDiskImage);
				gpvCachedMapDiskImage = NULL;
	}

#ifndef BSPC
	//
	// load the file into a buffer that we either discard as usual at the bottom, or if we've got enough memory
	//	then keep it long enough to save the renderer re-loading it (if not dedicated server),
	//	then discard it after that...
	//
	buf = NULL;
	fileHandle_t h;
	const int iBSPLen = FS_FOpenFileRead( name, &h, qfalse );
	if (h)
	{
		newBuff = Z_Malloc( iBSPLen, TAG_BSP_DISKIMAGE );
		FS_Read( newBuff, iBSPLen, h);
		FS_FCloseFile( h );

		buf = (int*) newBuff;	// so the rest of the code works as normal
		if (&cm == &cmg)
		{
			gpvCachedMapDiskImage = newBuff;
			newBuff = 0;
		}

		// carry on as before...
		//
	}
#else
	const int iBSPLen = LoadQuakeFile((quakefile_t *) name, (void **)&buf);
#endif

	if ( !buf ) {
		Com_Error (ERR_DROP, "Couldn't load %s", name);
	}

	last_checksum = LittleLong (Com_BlockChecksum (buf, iBSPLen));
	if ( checksum )
		*checksum = last_checksum;

	header = *(dheader_t *)buf;
	for (size_t i=0 ; i<sizeof(dheader_t)/4 ; i++) {
		((int *)&header)[i] = LittleLong ( ((int *)&header)[i]);
	}

	if ( header.version != BSP_VERSION ) {
		Z_Free(	gpvCachedMapDiskImage);
				gpvCachedMapDiskImage = NULL;

		Com_Error (ERR_DROP, "CM_LoadMap: %s has wrong version number (%i should be %i)"
		, name, header.version, BSP_VERSION );
	}

	cmod_base = (byte *)buf;

	// load into heap
	CMod_LoadShaders( &header.lumps[LUMP_SHADERS], cm );
	CMod_LoadLeafs (&header.lumps[LUMP_LEAFS], cm);
	CMod_LoadLeafBrushes (&header.lumps[LUMP_LEAFBRUSHES], cm);
	CMod_LoadLeafSurfaces (&header.lumps[LUMP_LEAFSURFACES], cm);
	CMod_LoadPlanes (&header.lumps[LUMP_PLANES], cm);
	CMod_LoadBrushSides (&header.lumps[LUMP_BRUSHSIDES], cm);
	CMod_LoadBrushes (&header.lumps[LUMP_BRUSHES], cm);
	CMod_LoadSubmodels (&header.lumps[LUMP_MODELS], cm);
	CMod_LoadNodes (&header.lumps[LUMP_NODES], cm);
	CMod_LoadEntityString (&header.lumps[LUMP_ENTITIES], cm );
	CMod_LoadVisibility( &header.lumps[LUMP_VISIBILITY], cm );
	CMod_LoadPatches( &header.lumps[LUMP_SURFACES], &header.lumps[LUMP_DRAWVERTS], cm );

	TotalSubModels += cm.numSubModels;

	if (&cm == &cmg)
	{
		// Load in the shader text - return instantly if already loaded
		CM_InitBoxHull ();
	}

#ifndef BSPC	// I hope we can lose this crap soon
	//
	// if we've got enough memory, and it's not a dedicated-server, then keep the loaded map binary around
	//	for the renderer to chew on... (but not if this gets ported to a big-endian machine, because some of the
	//	map data will have been Little-Long'd, but some hasn't).
	//
	if (Sys_LowPhysicalMemory()
		|| com_dedicated->integer
//		|| we're on a big-endian machine
		)
	{
		Z_Free(	gpvCachedMapDiskImage );
				gpvCachedMapDiskImage = NULL;
	}
	else
	{
		// ... do nothing, and let the renderer free it after it's finished playing with it...
		//
	}
#else
	FS_FreeFile (buf);
#endif

	CM_FloodAreaConnections (cm);

	// allow this to be cached if it is loaded by the server
	if ( !clientload ) {
		Q_strncpyz( cm.name, origName, sizeof( cm.name ) );
	}
}


// need a wrapper function around this because of multiple returns, need to ensure bool is correct...
//
void CM_LoadMap( const char *name, qboolean clientload, int *checksum )
{
	gbUsingCachedMapDataRightNow = qtrue;	// !!!!!!!!!!!!!!!!!!

		CM_LoadMap_Actual( name, clientload, checksum, cmg );

	gbUsingCachedMapDataRightNow = qfalse;	// !!!!!!!!!!!!!!!!!!
}



/*
==================
CM_ClearMap
==================
*/
void CM_ClearMap( void )
{
	int		i;

	Com_Memset( &cmg, 0, sizeof( cmg ) );
	CM_ClearLevelPatches();

	for(i = 0; i < NumSubBSP; i++)
	{
		memset(&SubBSP[i], 0, sizeof(SubBSP[0]));
	}
	NumSubBSP = 0;
	TotalSubModels = 0;
}

/*
==================
CM_ClipHandleToModel
==================
*/
cmodel_t	*CM_ClipHandleToModel( clipHandle_t handle, clipMap_t **clipMap ) {
	int		i;
	int		count;

	if ( handle < 0 )
	{
		Com_Error( ERR_DROP, "CM_ClipHandleToModel: bad handle %i", handle );
	}
	if ( handle < cmg.numSubModels )
	{
		if (clipMap)
		{
			*clipMap = &cmg;
		}
		return &cmg.cmodels[handle];
	}
	if ( handle == BOX_MODEL_HANDLE )
	{
		if (clipMap)
		{
			*clipMap = &cmg;
		}
		return &box_model;
	}

	count = cmg.numSubModels;
	for(i = 0; i < NumSubBSP; i++)
	{
		if (handle < count + SubBSP[i].numSubModels)
		{
			if (clipMap)
			{
				*clipMap = &SubBSP[i];
			}
			return &SubBSP[i].cmodels[handle - count];
		}
		count += SubBSP[i].numSubModels;
	}

	if ( handle < MAX_SUBMODELS )
	{
		Com_Error( ERR_DROP, "CM_ClipHandleToModel: bad handle (count: %i) < (handle: %i) < (max: %i)",
			cmg.numSubModels, handle, MAX_SUBMODELS );
	}
	Com_Error( ERR_DROP, "CM_ClipHandleToModel: bad handle %i", handle + MAX_SUBMODELS );

	return NULL;
}

/*
==================
CM_InlineModel
==================
*/
clipHandle_t	CM_InlineModel( int index ) {
	if ( index < 0 || index >= TotalSubModels ) {
		Com_Error( ERR_DROP, "CM_InlineModel: bad number: %d >= %d (may need to re-BSP map?)", index, TotalSubModels );
	}
	return index;
}

int		CM_NumInlineModels( void ) {
	return cmg.numSubModels;
}

char	*CM_EntityString( void ) {
	return cmg.entityString;
}

char *CM_SubBSPEntityString( int index )
{
	return SubBSP[index].entityString;
}

int		CM_LeafCluster( int leafnum ) {
	if (leafnum < 0 || leafnum >= cmg.numLeafs) {
		Com_Error (ERR_DROP, "CM_LeafCluster: bad number");
	}
	return cmg.leafs[leafnum].cluster;
}

int		CM_LeafArea( int leafnum ) {
	if ( leafnum < 0 || leafnum >= cmg.numLeafs ) {
		Com_Error (ERR_DROP, "CM_LeafArea: bad number");
	}
	return cmg.leafs[leafnum].area;
}

//=======================================================================


/*
===================
CM_InitBoxHull

Set up the planes and nodes so that the six floats of a bounding box
can just be stored out and get a proper clipping hull structure.
===================
*/
void CM_InitBoxHull (void)
{
	int			i;
	int			side;
	cplane_t	*p;
	cbrushside_t	*s;

	box_planes = &cmg.planes[cmg.numPlanes];

	box_brush = &cmg.brushes[cmg.numBrushes];
	box_brush->numsides = 6;
	box_brush->sides = cmg.brushsides + cmg.numBrushSides;
	box_brush->contents = CONTENTS_BODY;

	box_model.firstNode = -1;
	box_model.leaf.numLeafBrushes = 1;
//	box_model.leaf.firstLeafBrush = cmg.numBrushes;
	box_model.leaf.firstLeafBrush = cmg.numLeafBrushes;
	cmg.leafbrushes[cmg.numLeafBrushes] = cmg.numBrushes;

	for (i=0 ; i<6 ; i++)
	{
		side = i&1;

		// brush sides
		s = &cmg.brushsides[cmg.numBrushSides+i];
		s->plane = 	cmg.planes + (cmg.numPlanes+i*2+side);
		s->shaderNum = cmg.numShaders;

		// planes
		p = &box_planes[i*2];
		p->type = i>>1;
		p->signbits = 0;
		VectorClear (p->normal);
		p->normal[i>>1] = 1;

		p = &box_planes[i*2+1];
		p->type = 3 + (i>>1);
		p->signbits = 0;
		VectorClear (p->normal);
		p->normal[i>>1] = -1;

		SetPlaneSignbits( p );
	}
}

/*
===================
CM_TempBoxModel

To keep everything totally uniform, bounding boxes are turned into small
BSP trees instead of being compared directly.
Capsules are handled differently though.
===================
*/
clipHandle_t CM_TempBoxModel( const vec3_t mins, const vec3_t maxs, int capsule ) {

	VectorCopy( mins, box_model.mins );
	VectorCopy( maxs, box_model.maxs );

	if ( capsule ) {
		return CAPSULE_MODEL_HANDLE;
	}

	box_planes[0].dist = maxs[0];
	box_planes[1].dist = -maxs[0];
	box_planes[2].dist = mins[0];
	box_planes[3].dist = -mins[0];
	box_planes[4].dist = maxs[1];
	box_planes[5].dist = -maxs[1];
	box_planes[6].dist = mins[1];
	box_planes[7].dist = -mins[1];
	box_planes[8].dist = maxs[2];
	box_planes[9].dist = -maxs[2];
	box_planes[10].dist = mins[2];
	box_planes[11].dist = -mins[2];

	VectorCopy( mins, box_brush->bounds[0] );
	VectorCopy( maxs, box_brush->bounds[1] );

	return BOX_MODEL_HANDLE;
}

/*
===================
CM_ModelBounds
===================
*/
void CM_ModelBounds( clipHandle_t model, vec3_t mins, vec3_t maxs ) {
	cmodel_t	*cmod;

	cmod = CM_ClipHandleToModel( model );
	VectorCopy( cmod->mins, mins );
	VectorCopy( cmod->maxs, maxs );
}

int CM_LoadSubBSP(const char *name, qboolean clientload)
{
	int		i;
	int		checksum;
	int		count;

	count = cmg.numSubModels;
	for(i = 0; i < NumSubBSP; i++)
	{
		if (!Q_stricmp(name, SubBSP[i].name))
		{
			return count;
		}
		count += SubBSP[i].numSubModels;
	}

	if (NumSubBSP == MAX_SUB_BSP)
	{
		Com_Error (ERR_DROP, "CM_LoadSubBSP: too many unique sub BSPs");
	}

	CM_LoadMap_Actual(name, clientload, &checksum, SubBSP[NumSubBSP] );
	NumSubBSP++;

	return count;
}

int CM_FindSubBSP(int modelIndex)
{
	int		i;
	int		count;

	count = cmg.numSubModels;
	if (modelIndex < count)
	{	// belongs to the main bsp
		return -1;
	}

	for(i = 0; i < NumSubBSP; i++)
	{
		count += SubBSP[i].numSubModels;
		if (modelIndex < count)
		{
			return i;
		}
	}
	return -1;
}

void CM_GetWorldBounds ( vec3_t mins, vec3_t maxs )
{
	VectorCopy ( cmg.cmodels[0].mins, mins );
	VectorCopy ( cmg.cmodels[0].maxs, maxs );
}

int CM_ModelContents_Actual( clipHandle_t model, clipMap_t *cm )
{
	cmodel_t	*cmod;
	int			contents = 0;
	int			i;

	if (!cm)
	{
		cm = &cmg;
	}

	cmod = CM_ClipHandleToModel( model, &cm );

	//MCG ADDED - return the contents, too

	for ( i = 0; i < cmod->leaf.numLeafBrushes; i++ )
	{
		int brushNum = cm->leafbrushes[cmod->leaf.firstLeafBrush + i];
		contents |= cm->brushes[brushNum].contents;
	}

	for ( i = 0; i < cmod->leaf.numLeafSurfaces; i++ )
	{
		int surfaceNum = cm->leafsurfaces[cmod->leaf.firstLeafSurface + i];
		if ( cm->surfaces[surfaceNum] != NULL )
		{//HERNH?  How could we have a null surf within our cmod->leaf.numLeafSurfaces?
			contents |= cm->surfaces[surfaceNum]->contents;
		}
	}

	return contents;
}

int CM_ModelContents(  clipHandle_t model, int subBSPIndex )
{
	if (subBSPIndex < 0)
	{
		return CM_ModelContents_Actual(model, NULL);
	}

	return CM_ModelContents_Actual(model, &SubBSP[subBSPIndex]);
}
