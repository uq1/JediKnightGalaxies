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

// bg_slidemove.c -- part of bg_pmove functionality

#include "qcommon/q_shared.h"
#include "bg_public.h"
#include "bg_local.h"

#if defined(_GAME)
	#include "g_local.h"
#elif defined(IN_UI)
	#include "../ui/ui_local.h"
#elif defined(_CGAME)
	#include "../cgame/cg_local.h"
#endif

extern bgEntity_t *pm_entSelf;

qboolean PM_GroundSlideOkay( float zNormal )
{
	if ( zNormal > 0 )
	{
		if ( pm->ps->velocity[2] > 0 )
		{
			if ( pm->ps->legsAnim == BOTH_WALL_RUN_RIGHT
				|| pm->ps->legsAnim == BOTH_WALL_RUN_LEFT 
				|| pm->ps->legsAnim == BOTH_WALL_RUN_RIGHT_STOP
				|| pm->ps->legsAnim == BOTH_WALL_RUN_LEFT_STOP 
				|| pm->ps->legsAnim == BOTH_FORCEWALLRUNFLIP_START 
				|| pm->ps->legsAnim == BOTH_FORCELONGLEAP_START
				|| pm->ps->legsAnim == BOTH_FORCELONGLEAP_ATTACK
				|| pm->ps->legsAnim == BOTH_FORCELONGLEAP_LAND
				|| BG_InReboundJump( pm->ps->legsAnim ))
			{
				return qfalse;
			}
		}
	}
	return qtrue;
}

/*
===============
qboolean PM_ClientImpact( trace_t *trace, qboolean damageSelf )

===============
*/
#ifdef _GAME
extern void Client_CheckImpactBBrush( gentity_t *self, gentity_t *other );
qboolean PM_ClientImpact( trace_t *trace )
{
	//don't try to predict this
	gentity_t	*traceEnt;
	int			otherEntityNum = trace->entityNum;

	if ( !pm_entSelf )
	{
		return qfalse;
	}

	if ( otherEntityNum >= ENTITYNUM_WORLD )
	{
		return qfalse;
	}

	traceEnt = &g_entities[otherEntityNum];

	if( VectorLength( pm->ps->velocity ) >= 100 
		&& pm_entSelf->s.NPC_class != CLASS_VEHICLE
		&& pm->ps->lastOnGround+100 < level.time )
		//&& pm->ps->groundEntityNum == ENTITYNUM_NONE )
	{
		Client_CheckImpactBBrush( (gentity_t *)(pm_entSelf), &g_entities[otherEntityNum] );
	}

	if ( !traceEnt 
		|| !(traceEnt->r.contents&pm->tracemask) )
	{//it's dead or not in my way anymore, don't clip against it
		return qtrue;
	}

	return qfalse;
}
#endif

/*
==================
PM_SlideMove

Returns qtrue if the velocity was clipped in some way
==================
*/
#define	MAX_CLIP_PLANES	5
qboolean	PM_SlideMove( qboolean gravity ) {
	int			bumpcount, numbumps;
	vec3_t		dir;
	float		d;
	int			numplanes;
	vec3_t		normal, planes[MAX_CLIP_PLANES];
	vec3_t		primal_velocity;
	vec3_t		clipVelocity;
	int			i, j, k;
	trace_t	trace;
	vec3_t		end;
	float		time_left;
	float		into;
	vec3_t		endVelocity;
	vec3_t		endClipVelocity;
	//qboolean	damageSelf = qtrue;
	
	numbumps = 4;

	VectorCopy (pm->ps->velocity, primal_velocity);

	if ( gravity ) {
		VectorCopy( pm->ps->velocity, endVelocity );
		endVelocity[2] -= pm->ps->gravity * pml.frametime;
		pm->ps->velocity[2] = ( pm->ps->velocity[2] + endVelocity[2] ) * 0.5;
		primal_velocity[2] = endVelocity[2];
		if ( pml.groundPlane ) {
			if ( PM_GroundSlideOkay( pml.groundTrace.plane.normal[2] ) )
			{// slide along the ground plane
				PM_ClipVelocity (pm->ps->velocity, pml.groundTrace.plane.normal, 
					pm->ps->velocity, OVERCLIP );
			}
		}
	}

	time_left = pml.frametime;

	// never turn against the ground plane
	if ( pml.groundPlane ) {
		numplanes = 1;
		VectorCopy( pml.groundTrace.plane.normal, planes[0] );
		if ( !PM_GroundSlideOkay( planes[0][2] ) )
		{
			planes[0][2] = 0;
			VectorNormalize( planes[0] );
		}
	} else {
		numplanes = 0;
	}

	// never turn against original velocity
	VectorNormalize2( pm->ps->velocity, planes[numplanes] );
	numplanes++;

	for ( bumpcount=0 ; bumpcount < numbumps ; bumpcount++ ) {

		// calculate position we are trying to move to
		VectorMA( pm->ps->origin, time_left, pm->ps->velocity, end );

		// see if we can make it there
		pm->trace ( &trace, pm->ps->origin, pm->mins, pm->maxs, end, pm->ps->clientNum, pm->tracemask);

		if (trace.allsolid) {
			// entity is completely trapped in another solid
			pm->ps->velocity[2] = 0;	// don't build up falling damage, but allow sideways acceleration
			return qtrue;
		}

		if (trace.fraction > 0) {
			// actually covered some distance
			VectorCopy (trace.endpos, pm->ps->origin);
		}

		if (trace.fraction == 1) {
			 break;		// moved the entire distance
		}

		// save entity for contact
		PM_AddTouchEnt( trace.entityNum );

#ifdef _GAME
		if ( PM_ClientImpact( &trace ) )
		{
			continue;
		}
#endif

		time_left -= time_left * trace.fraction;

		if (numplanes >= MAX_CLIP_PLANES) {
			// this shouldn't really happen
			VectorClear( pm->ps->velocity );
			return qtrue;
		}

		VectorCopy( trace.plane.normal, normal );

		if ( !PM_GroundSlideOkay( normal[2] ) )
		{//wall-running
			//never push up off a sloped wall
			normal[2] = 0;
			VectorNormalize( normal );
		}
		//
		// if this is the same plane we hit before, nudge velocity
		// out along it, which fixes some epsilon issues with
		// non-axial planes
		//
		if ( !(pm->ps->pm_flags&PMF_STUCK_TO_WALL) )
		{//no sliding if stuck to wall!
			for ( i = 0 ; i < numplanes ; i++ ) {
				if ( VectorCompare( normal, planes[i] ) ) {//DotProduct( normal, planes[i] ) > 0.99 ) {
					VectorAdd( normal, pm->ps->velocity, pm->ps->velocity );
					break;
				}
			}
			if ( i < numplanes ) {
				continue;
			}
		}
		VectorCopy (normal, planes[numplanes]);
		numplanes++;

		//
		// modify velocity so it parallels all of the clip planes
		//

		// find a plane that it enters
		for ( i = 0 ; i < numplanes ; i++ ) {
			into = DotProduct( pm->ps->velocity, planes[i] );
			if ( into >= 0.1 ) {
				continue;		// move doesn't interact with the plane
			}

			// see how hard we are hitting things
			if ( -into > pml.impactSpeed ) {
				pml.impactSpeed = -into;
			}

			// slide along the plane
			PM_ClipVelocity (pm->ps->velocity, planes[i], clipVelocity, OVERCLIP );

			// slide along the plane
			PM_ClipVelocity (endVelocity, planes[i], endClipVelocity, OVERCLIP );

			// see if there is a second plane that the new move enters
			for ( j = 0 ; j < numplanes ; j++ ) {
				if ( j == i ) {
					continue;
				}
				if ( DotProduct( clipVelocity, planes[j] ) >= 0.1 ) {
					continue;		// move doesn't interact with the plane
				}

				// try clipping the move to the plane
				PM_ClipVelocity( clipVelocity, planes[j], clipVelocity, OVERCLIP );
				PM_ClipVelocity( endClipVelocity, planes[j], endClipVelocity, OVERCLIP );

				// see if it goes back into the first clip plane
				if ( DotProduct( clipVelocity, planes[i] ) >= 0 ) {
					continue;
				}

				// slide the original velocity along the crease
				CrossProduct (planes[i], planes[j], dir);
				VectorNormalize( dir );
				d = DotProduct( dir, pm->ps->velocity );
				VectorScale( dir, d, clipVelocity );

				CrossProduct (planes[i], planes[j], dir);
				VectorNormalize( dir );
				d = DotProduct( dir, endVelocity );
				VectorScale( dir, d, endClipVelocity );

				// see if there is a third plane the the new move enters
				for ( k = 0 ; k < numplanes ; k++ ) {
					if ( k == i || k == j ) {
						continue;
					}
					if ( DotProduct( clipVelocity, planes[k] ) >= 0.1 ) {
						continue;		// move doesn't interact with the plane
					}

					// stop dead at a triple plane interaction
					VectorClear( pm->ps->velocity );
					return qtrue;
				}
			}

			// if we have fixed all interactions, try another move
			VectorCopy( clipVelocity, pm->ps->velocity );
			VectorCopy( endClipVelocity, endVelocity );
			break;
		}
	}

	if ( gravity ) {
		VectorCopy( endVelocity, pm->ps->velocity );
	}

	// don't change velocity if in a timer (FIXME: is this correct?)
	if ( pm->ps->pm_time ) {
		VectorCopy( primal_velocity, pm->ps->velocity );
	}

	return ( bumpcount != 0 );
}

/*
==================
PM_StepSlideMove

==================
*/
void PM_StepSlideMove( qboolean gravity ) { 
	vec3_t		start_o, start_v;
	vec3_t		down_o, down_v;
	trace_t		trace;
	vec3_t		up, down;
	float		stepSize;
	qboolean	isGiant = qfalse;
	bgEntity_t	*pEnt;
	qboolean skipStep = qfalse;

	VectorCopy (pm->ps->origin, start_o);
	VectorCopy (pm->ps->velocity, start_v);

	if ( BG_InReboundHold( pm->ps->legsAnim ) )
	{
		gravity = qfalse;
	}

	if ( PM_SlideMove( gravity ) == 0 ) {
		return;		// we got exactly where we wanted to go first try	
	}

	pEnt = pm_entSelf;

	VectorCopy(start_o, down);
	down[2] -= STEPSIZE;
	pm->trace (&trace, start_o, pm->mins, pm->maxs, down, pm->ps->clientNum, pm->tracemask);
	VectorSet(up, 0, 0, 1);
	// never step up when you still have up velocity
	if ( pm->ps->velocity[2] > 0 
		&& (trace.fraction == 1.0 || DotProduct(trace.plane.normal, up) < 0.7)
		&& !(pm->ps->clientNum >= MAX_CLIENTS && pEnt->s.NPC_class != CLASS_VEHICLE))
	{
		return;
	}

	VectorCopy (pm->ps->origin, down_o);
	VectorCopy (pm->ps->velocity, down_v);

	VectorCopy (start_o, up);

	if (pm->ps->clientNum >= MAX_CLIENTS)
	{
		// apply ground friction, even if on ladder
		if ( pEnt && pEnt->s.NPC_class == CLASS_RANCOR )
		{//also can step up high
			up[2] += 64.0f;
			isGiant = qtrue;
		}
		else
		{// UQ1: NPCs get off easy - for the sake of lower CPU usage (routing) and looking better in general...
			up[2] += STEPSIZE*2;
		}
	}
	else
	{
#ifdef _GAME
	if ( g_entities[pm->ps->clientNum].r.svFlags & SVF_BOT )
	{// UQ1: BOTs get off easy - for the sake of lower CPU usage (routing) and looking better in general...
		up[2] += STEPSIZE*2;
	}
	else
	{
		up[2] += STEPSIZE;
	}
#else //!_GAME
		up[2] += STEPSIZE;
#endif //_GAME
	}

	// test the player position if they were a stepheight higher
	pm->trace (&trace, start_o, pm->mins, pm->maxs, up, pm->ps->clientNum, pm->tracemask);
	if ( trace.allsolid ) {
		if ( pm->debugLevel ) {
			Com_Printf("%i:bend can't step\n", c_pmove);
		}

		//Com_Printf("Failed step at 3.\n");
		return;		// can't step up
	}

	stepSize = trace.endpos[2] - start_o[2];
	// try slidemove from this position
	VectorCopy (trace.endpos, pm->ps->origin);
	VectorCopy (start_v, pm->ps->velocity);

	PM_SlideMove( gravity );

	// push down the final amount
	VectorCopy (pm->ps->origin, down);
	down[2] -= stepSize;
	pm->trace (&trace, pm->ps->origin, pm->mins, pm->maxs, down, pm->ps->clientNum, pm->tracemask);

	if ( pm->stepSlideFix )
	{
		if ( pm->ps->clientNum < MAX_CLIENTS
			&& trace.plane.normal[2] < MIN_WALK_NORMAL )
		{//normal players cannot step up slopes that are too steep to walk on!
			vec3_t stepVec;
			//okay, the step up ends on a slope that it too steep to step up onto,
			//BUT:
			//If the step looks like this:
			//  (B)\__
			//        \_____(A)
			//Then it might still be okay, so we figure out the slope of the entire move
			//from (A) to (B) and if that slope is walk-upabble, then it's okay
			VectorSubtract( trace.endpos, down_o, stepVec );
			VectorNormalize( stepVec ); 
			if ( stepVec[2] > (1.0f-MIN_WALK_NORMAL) )
			{
				skipStep = qtrue;
			}
		}
	}

	if ( !trace.allsolid 
		&& !skipStep ) //normal players cannot step up slopes that are too steep to walk on!
	{ 
		if ( pm->ps->clientNum >= MAX_CLIENTS//NPC
			&& isGiant 
			&& trace.entityNum < MAX_CLIENTS
			&& pEnt 
			&& pEnt->s.NPC_class == CLASS_RANCOR )
		{//Rancor don't step on clients
			if ( pm->stepSlideFix )
			{
				VectorCopy (down_o, pm->ps->origin);
				VectorCopy (down_v, pm->ps->velocity);
			}
			else
			{
				VectorCopy (start_o, pm->ps->origin);
				VectorCopy (start_v, pm->ps->velocity);
			}
		}
		else
		{
			VectorCopy (trace.endpos, pm->ps->origin);
			if ( pm->stepSlideFix )
			{
				if ( trace.fraction < 1.0 ) {
					PM_ClipVelocity( pm->ps->velocity, trace.plane.normal, pm->ps->velocity, OVERCLIP );
				}
			}
		}
	}
	else
	{
		if ( pm->stepSlideFix )
		{
			VectorCopy (down_o, pm->ps->origin);
			VectorCopy (down_v, pm->ps->velocity);
		}
	}
	if ( !pm->stepSlideFix )
	{
		if ( trace.fraction < 1.0 ) {
			PM_ClipVelocity( pm->ps->velocity, trace.plane.normal, pm->ps->velocity, OVERCLIP );
		}
	}

	// use the step move
	float	delta;

	delta = pm->ps->origin[2] - start_o[2];
	if ( delta > 2 ) {
		if ( delta < 7 ) {
			PM_AddEvent( EV_STEP_4 );
		} else if ( delta < 11 ) {
			PM_AddEvent( EV_STEP_8 );
		} else if ( delta < 15 ) {
			PM_AddEvent( EV_STEP_12 );
		} else {
			PM_AddEvent( EV_STEP_16 );
		}
	}
	if ( pm->debugLevel ) {
		Com_Printf("%i:stepped\n", c_pmove);
	}
}

