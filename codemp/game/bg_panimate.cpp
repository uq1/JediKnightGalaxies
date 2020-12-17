/*
===========================================================================
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

// BG_PAnimate.c

#include "qcommon/q_shared.h"
#include "bg_public.h"
#include "bg_local.h"
#include "anims.h"
#include "cgame/animtable.h"
#include "ghoul2/G2.h"

#if defined(_GAME)
	#include "g_local.h"
#elif defined(IN_UI)
	#include "../ui/ui_local.h"
#elif defined(_CGAME)
	#include "../cgame/cg_local.h"
#endif

extern saberInfo_t *BG_MySaber( int clientNum, int saberNum );
/*
==============================================================================
BEGIN: Animation utility functions (sequence checking)
==============================================================================
*/
//Called regardless of pm validity:

// VVFIXME - Most of these functions are totally stateless and stupid. Don't
// need multiple copies of this, but it's much easier (and less likely to
// break in the future) if I keep separate namespace versions now.

qboolean BG_SaberStanceAnim( int anim )
{
	switch ( anim )
	{
	case BOTH_STAND1://not really a saberstance anim, actually... "saber off" stance
	case BOTH_STAND2://single-saber, medium style
	case BOTH_SABERFAST_STANCE://single-saber, fast style
	case BOTH_SABERSLOW_STANCE://single-saber, strong style
	case BOTH_SABERSTAFF_STANCE://saber staff style
	case BOTH_SABERDUAL_STANCE://dual saber style
		return qtrue;
		break;
	}
	return qfalse;
}

qboolean BG_CrouchAnim( int anim )
{
	switch ( anim )
	{
	case BOTH_SIT1:				//# Normal chair sit.
	case BOTH_SIT2:				//# Lotus position.
	case BOTH_SIT3:				//# Sitting in tired position: elbows on knees
	case BOTH_CROUCH1:			//# Transition from standing to crouch
	case BOTH_CROUCH1IDLE:		//# Crouching idle
	case BOTH_CROUCH1WALK:		//# Walking while crouched
	case BOTH_CROUCH1WALKBACK:	//# Walking while crouched
	case BOTH_CROUCH2TOSTAND1:	//# going from crouch2 to stand1
	case BOTH_CROUCH3:			//# Desann crouching down to Kyle (cin 9)
	case BOTH_KNEES1:			//# Tavion on her knees
	case BOTH_CROUCHATTACKBACK1://FIXME: not if in middle of anim?
	case BOTH_ROLL_STAB:
		return qtrue;
		break;
	}
	return qfalse;
}

qboolean BG_InSpecialJump( int anim )
{
	switch ( (anim) )
	{
	case BOTH_WALL_RUN_RIGHT:
	case BOTH_WALL_RUN_RIGHT_STOP:
	case BOTH_WALL_RUN_RIGHT_FLIP:
	case BOTH_WALL_RUN_LEFT:
	case BOTH_WALL_RUN_LEFT_STOP:
	case BOTH_WALL_RUN_LEFT_FLIP:
	case BOTH_WALL_FLIP_RIGHT:
	case BOTH_WALL_FLIP_LEFT:
	case BOTH_FLIP_BACK1:
	case BOTH_FLIP_BACK2:
	case BOTH_FLIP_BACK3:
	case BOTH_WALL_FLIP_BACK1:
	case BOTH_BUTTERFLY_LEFT:
	case BOTH_BUTTERFLY_RIGHT:
	case BOTH_BUTTERFLY_FL1:
	case BOTH_BUTTERFLY_FR1:
	case BOTH_FJSS_TR_BL:
	case BOTH_FJSS_TL_BR:
	case BOTH_FORCELEAP2_T__B_:
	case BOTH_JUMPFLIPSLASHDOWN1://#
	case BOTH_JUMPFLIPSTABDOWN://#
	case BOTH_JUMPATTACK6:
	case BOTH_JUMPATTACK7:
	case BOTH_ARIAL_LEFT:
	case BOTH_ARIAL_RIGHT:
	case BOTH_ARIAL_F1:
	case BOTH_CARTWHEEL_LEFT:
	case BOTH_CARTWHEEL_RIGHT:

	case BOTH_FORCELONGLEAP_START:
	case BOTH_FORCELONGLEAP_ATTACK:
	case BOTH_FORCEWALLRUNFLIP_START:
	case BOTH_FORCEWALLRUNFLIP_END:
	case BOTH_FORCEWALLRUNFLIP_ALT:
	case BOTH_FLIP_ATTACK7:
	case BOTH_FLIP_HOLD7:
	case BOTH_FLIP_LAND:
	case BOTH_A7_SOULCAL:
		return qtrue;
	}
	if ( BG_InReboundJump( anim ) )
	{
		return qtrue;
	}
	if ( BG_InReboundHold( anim ) )
	{
		return qtrue;
	}
	if ( BG_InReboundRelease( anim ) )
	{
		return qtrue;
	}
	if ( BG_InBackFlip( anim ) )
	{
		return qtrue;
	}
	return qfalse;
}

qboolean BG_InSaberStandAnim( int anim )
{
	switch ( (anim) )
	{
	case BOTH_SABERFAST_STANCE:
	case BOTH_STAND2:
	case BOTH_SABERSLOW_STANCE:
	case BOTH_SABERDUAL_STANCE:
	case BOTH_SABERSTAFF_STANCE:
		return qtrue;
	default:
		return qfalse;
	}
}

qboolean BG_InReboundJump( int anim )
{
	switch ( anim )
	{
	case BOTH_FORCEWALLREBOUND_FORWARD:
	case BOTH_FORCEWALLREBOUND_LEFT:
	case BOTH_FORCEWALLREBOUND_BACK:
	case BOTH_FORCEWALLREBOUND_RIGHT:
		return qtrue;
		break;
	}
	return qfalse;
}

qboolean BG_InReboundHold( int anim )
{
	switch ( anim )
	{
	case BOTH_FORCEWALLHOLD_FORWARD:
	case BOTH_FORCEWALLHOLD_LEFT:
	case BOTH_FORCEWALLHOLD_BACK:
	case BOTH_FORCEWALLHOLD_RIGHT:
		return qtrue;
		break;
	}
	return qfalse;
}

qboolean BG_InReboundRelease( int anim )
{
	switch ( anim )
	{
	case BOTH_FORCEWALLRELEASE_FORWARD:
	case BOTH_FORCEWALLRELEASE_LEFT:
	case BOTH_FORCEWALLRELEASE_BACK:
	case BOTH_FORCEWALLRELEASE_RIGHT:
		return qtrue;
		break;
	}
	return qfalse;
}

qboolean BG_InBackFlip( int anim )
{
	switch ( anim )
	{
	case BOTH_FLIP_BACK1:
	case BOTH_FLIP_BACK2:
	case BOTH_FLIP_BACK3:
		return qtrue;
		break;
	}
	return qfalse;
}

qboolean BG_DirectFlippingAnim( int anim )
{
	switch ( (anim) )
	{
	case BOTH_FLIP_F:			//# Flip forward
	case BOTH_FLIP_B:			//# Flip backwards
	case BOTH_FLIP_L:			//# Flip left
	case BOTH_FLIP_R:			//# Flip right
		return qtrue;
	}

	return qfalse;
}

qboolean BG_SaberInAttackPure( int move )
{
	if ( move >= LS_A_TL2BR && move <= LS_A_T2B )
	{
		return qtrue;
	}
	return qfalse;
}
qboolean BG_SaberInAttack( int move )
{
	if ( move >= LS_A_TL2BR && move <= LS_A_T2B )
	{
		return qtrue;
	}
	switch ( move )
	{
	case LS_A_BACK:
	case LS_A_BACK_CR:
	case LS_A_BACKSTAB:
	case LS_ROLL_STAB:
	case LS_A_LUNGE:
	case LS_A_JUMP_T__B_:
	case LS_A_FLIP_STAB:
	case LS_A_FLIP_SLASH:
	case LS_JUMPATTACK_DUAL:
	case LS_JUMPATTACK_ARIAL_LEFT:
	case LS_JUMPATTACK_ARIAL_RIGHT:
	case LS_JUMPATTACK_CART_LEFT:
	case LS_JUMPATTACK_CART_RIGHT:
	case LS_JUMPATTACK_STAFF_LEFT:
	case LS_JUMPATTACK_STAFF_RIGHT:
	case LS_BUTTERFLY_LEFT:
	case LS_BUTTERFLY_RIGHT:
	case LS_A_BACKFLIP_ATK:
	case LS_SPINATTACK_DUAL:
	case LS_SPINATTACK:
	case LS_LEAP_ATTACK:
	case LS_SWOOP_ATTACK_RIGHT:
	case LS_SWOOP_ATTACK_LEFT:
	case LS_TAUNTAUN_ATTACK_RIGHT:
	case LS_TAUNTAUN_ATTACK_LEFT:
	case LS_KICK_F:
	case LS_KICK_B:
	case LS_KICK_R:
	case LS_KICK_L:
	case LS_KICK_S:
	case LS_KICK_BF:
	case LS_KICK_RL:
	case LS_KICK_F_AIR:
	case LS_KICK_B_AIR:
	case LS_KICK_R_AIR:
	case LS_KICK_L_AIR:
	case LS_STABDOWN:
	case LS_STABDOWN_STAFF:
	case LS_STABDOWN_DUAL:
	case LS_DUAL_SPIN_PROTECT:
	case LS_STAFF_SOULCAL:
	case LS_A1_SPECIAL:
	case LS_A2_SPECIAL:
	case LS_A3_SPECIAL:
	case LS_UPSIDE_DOWN_ATTACK:
	case LS_PULL_ATTACK_STAB:
	case LS_PULL_ATTACK_SWING:
	case LS_SPINATTACK_ALORA:
	case LS_DUAL_FB:
	case LS_DUAL_LR:
	case LS_HILT_BASH:
		return qtrue;
	}
	return qfalse;
}

qboolean BG_SaberInKata( int saberMove )
{
	switch ( saberMove )
	{
	case LS_A1_SPECIAL:
	case LS_A2_SPECIAL:
	case LS_A3_SPECIAL:
	case LS_DUAL_SPIN_PROTECT:
	case LS_STAFF_SOULCAL:
		return qtrue;
	}
	return qfalse;
}

qboolean BG_InKataAnim(int anim)
{
	switch (anim)
	{
	case BOTH_A6_SABERPROTECT:
	case BOTH_A7_SOULCAL:
	case BOTH_A1_SPECIAL:
	case BOTH_A2_SPECIAL:
	case BOTH_A3_SPECIAL:
		return qtrue;
	}
	return qfalse;
}

//[CoOp]
qboolean PM_LockedAnim( int anim )
{//anims that can *NEVER* be overridden, regardless
	switch ( anim )
	{
	case BOTH_KYLE_PA_1:
	case BOTH_KYLE_PA_2:
	case BOTH_KYLE_PA_3:
	case BOTH_PLAYER_PA_1:
	case BOTH_PLAYER_PA_2:
	case BOTH_PLAYER_PA_3:
	case BOTH_PLAYER_PA_3_FLY:
	case BOTH_TAVION_SCEPTERGROUND:
	case BOTH_TAVION_SWORDPOWER:
	case BOTH_SCEPTER_START:
	case BOTH_SCEPTER_HOLD:
	case BOTH_SCEPTER_STOP:
	//grabbed by wampa
	case BOTH_GRABBED:	//#
	case BOTH_RELEASED:	//#	when Wampa drops player, transitions into fall on back
	case BOTH_HANG_IDLE:	//#
	case BOTH_HANG_ATTACK:	//#
	case BOTH_HANG_PAIN:	//#
		return qtrue;
	}
	return qfalse;
}
//[/CoOp]

qboolean BG_SaberInSpecial( int move )
{
	switch( move )
	{
	case LS_A_BACK:
	case LS_A_BACK_CR:
	case LS_A_BACKSTAB:
	case LS_ROLL_STAB:
	case LS_A_LUNGE:
	case LS_A_JUMP_T__B_:
	case LS_A_FLIP_STAB:
	case LS_A_FLIP_SLASH:
	case LS_JUMPATTACK_DUAL:
	case LS_JUMPATTACK_ARIAL_LEFT:
	case LS_JUMPATTACK_ARIAL_RIGHT:
	case LS_JUMPATTACK_CART_LEFT:
	case LS_JUMPATTACK_CART_RIGHT:
	case LS_JUMPATTACK_STAFF_LEFT:
	case LS_JUMPATTACK_STAFF_RIGHT:
	case LS_BUTTERFLY_LEFT:
	case LS_BUTTERFLY_RIGHT:
	case LS_A_BACKFLIP_ATK:
	case LS_SPINATTACK_DUAL:
	case LS_SPINATTACK:
	case LS_LEAP_ATTACK:
	case LS_SWOOP_ATTACK_RIGHT:
	case LS_SWOOP_ATTACK_LEFT:
	case LS_TAUNTAUN_ATTACK_RIGHT:
	case LS_TAUNTAUN_ATTACK_LEFT:
	case LS_KICK_F:
	case LS_KICK_B:
	case LS_KICK_R:
	case LS_KICK_L:
	case LS_KICK_S:
	case LS_KICK_BF:
	case LS_KICK_RL:
	case LS_KICK_F_AIR:
	case LS_KICK_B_AIR:
	case LS_KICK_R_AIR:
	case LS_KICK_L_AIR:
	case LS_STABDOWN:
	case LS_STABDOWN_STAFF:
	case LS_STABDOWN_DUAL:
	case LS_DUAL_SPIN_PROTECT:
	case LS_STAFF_SOULCAL:
	case LS_A1_SPECIAL:
	case LS_A2_SPECIAL:
	case LS_A3_SPECIAL:
	case LS_UPSIDE_DOWN_ATTACK:
	case LS_PULL_ATTACK_STAB:
	case LS_PULL_ATTACK_SWING:
	case LS_SPINATTACK_ALORA:
	case LS_DUAL_FB:
	case LS_DUAL_LR:
	case LS_HILT_BASH:
		return qtrue;
	}
	return qfalse;
}

qboolean BG_KickMove( int move )
{
	switch( move )
	{
	case LS_KICK_F:
	case LS_KICK_B:
	case LS_KICK_R:
	case LS_KICK_L:
	case LS_KICK_S:
	case LS_KICK_BF:
	case LS_KICK_RL:
	case LS_KICK_F_AIR:
	case LS_KICK_B_AIR:
	case LS_KICK_R_AIR:
	case LS_KICK_L_AIR:
	case LS_HILT_BASH:
		return qtrue;
	}
	return qfalse;
}

qboolean BG_SaberInIdle( int move )
{
	switch ( move )
	{
	case LS_NONE:
	case LS_READY:
	case LS_DRAW:
	case LS_PUTAWAY:
		return qtrue;
	}
	return qfalse;
}

qboolean BG_InExtraDefenseSaberMove( int move )
{
	switch ( move )
	{
	case LS_SPINATTACK_DUAL:
	case LS_SPINATTACK:
	case LS_DUAL_SPIN_PROTECT:
	case LS_STAFF_SOULCAL:
	case LS_A1_SPECIAL:
	case LS_A2_SPECIAL:
	case LS_A3_SPECIAL:
	case LS_JUMPATTACK_DUAL:
		return qtrue;
	}
	return qfalse;
}

qboolean BG_FlippingAnim( int anim )
{
	switch ( anim )
	{
	case BOTH_FLIP_F:			//# Flip forward
	case BOTH_FLIP_B:			//# Flip backwards
	case BOTH_FLIP_L:			//# Flip left
	case BOTH_FLIP_R:			//# Flip right
	case BOTH_WALL_RUN_RIGHT_FLIP:
	case BOTH_WALL_RUN_LEFT_FLIP:
	case BOTH_WALL_FLIP_RIGHT:
	case BOTH_WALL_FLIP_LEFT:
	case BOTH_FLIP_BACK1:
	case BOTH_FLIP_BACK2:
	case BOTH_FLIP_BACK3:
	case BOTH_WALL_FLIP_BACK1:
	//Not really flips, but...
	case BOTH_WALL_RUN_RIGHT:
	case BOTH_WALL_RUN_LEFT:
	case BOTH_WALL_RUN_RIGHT_STOP:
	case BOTH_WALL_RUN_LEFT_STOP:
	case BOTH_BUTTERFLY_LEFT:
	case BOTH_BUTTERFLY_RIGHT:
	case BOTH_BUTTERFLY_FL1:
	case BOTH_BUTTERFLY_FR1:
	//
	case BOTH_ARIAL_LEFT:
	case BOTH_ARIAL_RIGHT:
	case BOTH_ARIAL_F1:
	case BOTH_CARTWHEEL_LEFT:
	case BOTH_CARTWHEEL_RIGHT:
	case BOTH_JUMPFLIPSLASHDOWN1:
	case BOTH_JUMPFLIPSTABDOWN:
	case BOTH_JUMPATTACK6:
	case BOTH_JUMPATTACK7:
	//JKA
	case BOTH_FORCEWALLRUNFLIP_END:
	case BOTH_FORCEWALLRUNFLIP_ALT:
	case BOTH_FLIP_ATTACK7:
	case BOTH_A7_SOULCAL:
		return qtrue;
	}
	return qfalse;
}

qboolean BG_SpinningSaberAnim( int anim )
{
	switch ( anim )
	{
	//level 1 - FIXME: level 1 will have *no* spins
	case BOTH_T1_BR_BL:
	case BOTH_T1__R__L:
	case BOTH_T1__R_BL:
	case BOTH_T1_TR_BL:
	case BOTH_T1_BR_TL:
	case BOTH_T1_BR__L:
	case BOTH_T1_TL_BR:
	case BOTH_T1__L_BR:
	case BOTH_T1__L__R:
	case BOTH_T1_BL_BR:
	case BOTH_T1_BL__R:
	case BOTH_T1_BL_TR:
	//level 2
	case BOTH_T2_BR__L:
	case BOTH_T2_BR_BL:
	case BOTH_T2__R_BL:
	case BOTH_T2__L_BR:
	case BOTH_T2_BL_BR:
	case BOTH_T2_BL__R:
	//level 3
	case BOTH_T3_BR__L:
	case BOTH_T3_BR_BL:
	case BOTH_T3__R_BL:
	case BOTH_T3__L_BR:
	case BOTH_T3_BL_BR:
	case BOTH_T3_BL__R:
	//level 4
	case BOTH_T4_BR__L:
	case BOTH_T4_BR_BL:
	case BOTH_T4__R_BL:
	case BOTH_T4__L_BR:
	case BOTH_T4_BL_BR:
	case BOTH_T4_BL__R:
	//level 5
	case BOTH_T5_BR_BL:
	case BOTH_T5__R__L:
	case BOTH_T5__R_BL:
	case BOTH_T5_TR_BL:
	case BOTH_T5_BR_TL:
	case BOTH_T5_BR__L:
	case BOTH_T5_TL_BR:
	case BOTH_T5__L_BR:
	case BOTH_T5__L__R:
	case BOTH_T5_BL_BR:
	case BOTH_T5_BL__R:
	case BOTH_T5_BL_TR:
	//level 6
	case BOTH_T6_BR_TL:
	case BOTH_T6__R_TL:
	case BOTH_T6__R__L:
	case BOTH_T6__R_BL:
	case BOTH_T6_TR_TL:
	case BOTH_T6_TR__L:
	case BOTH_T6_TR_BL:
	case BOTH_T6_T__TL:
	case BOTH_T6_T__BL:
	case BOTH_T6_TL_BR:
	case BOTH_T6__L_BR:
	case BOTH_T6__L__R:
	case BOTH_T6_TL__R:
	case BOTH_T6_TL_TR:
	case BOTH_T6__L_TR:
	case BOTH_T6__L_T_:
	case BOTH_T6_BL_T_:
	case BOTH_T6_BR__L:
	case BOTH_T6_BR_BL:
	case BOTH_T6_BL_BR:
	case BOTH_T6_BL__R:
	case BOTH_T6_BL_TR:
	//level 7
	case BOTH_T7_BR_TL:
	case BOTH_T7_BR__L:
	case BOTH_T7_BR_BL:
	case BOTH_T7__R__L:
	case BOTH_T7__R_BL:
	case BOTH_T7_TR__L:
	case BOTH_T7_T___R:
	case BOTH_T7_TL_BR:
	case BOTH_T7__L_BR:
	case BOTH_T7__L__R:
	case BOTH_T7_BL_BR:
	case BOTH_T7_BL__R:
	case BOTH_T7_BL_TR:
	case BOTH_T7_TL_TR:
	case BOTH_T7_T__BR:
	case BOTH_T7__L_TR:
	case BOTH_V7_BL_S7:
	//special
	//case BOTH_A2_STABBACK1:
	case BOTH_ATTACK_BACK:
	case BOTH_CROUCHATTACKBACK1:
	case BOTH_BUTTERFLY_LEFT:
	case BOTH_BUTTERFLY_RIGHT:
	case BOTH_FJSS_TR_BL:
	case BOTH_FJSS_TL_BR:
	case BOTH_JUMPFLIPSLASHDOWN1:
	case BOTH_JUMPFLIPSTABDOWN:
		return qtrue;
	}
	return qfalse;
}

qboolean BG_SaberInSpecialAttack( int anim )
{
	switch ( anim )
	{
	case BOTH_A2_STABBACK1:
	case BOTH_ATTACK_BACK:
	case BOTH_CROUCHATTACKBACK1:
	case BOTH_ROLL_STAB:
	case BOTH_BUTTERFLY_LEFT:
	case BOTH_BUTTERFLY_RIGHT:
	case BOTH_BUTTERFLY_FL1:
	case BOTH_BUTTERFLY_FR1:
	case BOTH_FJSS_TR_BL:
	case BOTH_FJSS_TL_BR:
	case BOTH_LUNGE2_B__T_:
	case BOTH_FORCELEAP2_T__B_:
	case BOTH_JUMPFLIPSLASHDOWN1://#
	case BOTH_JUMPFLIPSTABDOWN://#
	case BOTH_JUMPATTACK6:
	case BOTH_JUMPATTACK7:
	case BOTH_SPINATTACK6:
	case BOTH_SPINATTACK7:
	case BOTH_FORCELONGLEAP_ATTACK:
	case BOTH_VS_ATR_S:
	case BOTH_VS_ATL_S:
	case BOTH_VT_ATR_S:
	case BOTH_VT_ATL_S:
	case BOTH_A7_KICK_F:
	case BOTH_A7_KICK_B:
	case BOTH_A7_KICK_R:
	case BOTH_A7_KICK_L:
	case BOTH_A7_KICK_S:
	case BOTH_A7_KICK_BF:
	case BOTH_A7_KICK_RL:
	case BOTH_A7_KICK_F_AIR:
	case BOTH_A7_KICK_B_AIR:
	case BOTH_A7_KICK_R_AIR:
	case BOTH_A7_KICK_L_AIR:
	case BOTH_STABDOWN:
	case BOTH_STABDOWN_STAFF:
	case BOTH_STABDOWN_DUAL:
	case BOTH_A6_SABERPROTECT:
	case BOTH_A7_SOULCAL:
	case BOTH_A1_SPECIAL:
	case BOTH_A2_SPECIAL:
	case BOTH_A3_SPECIAL:
	case BOTH_FLIP_ATTACK7:
	case BOTH_PULL_IMPALE_STAB:
	case BOTH_PULL_IMPALE_SWING:
	case BOTH_ALORA_SPIN_SLASH:
	case BOTH_A6_FB:
	case BOTH_A6_LR:	
	case BOTH_A7_HILT:
		return qtrue;
	}
	return qfalse;
}

qboolean BG_KickingAnim( int anim )
{
	switch ( anim )
	{
	case BOTH_A7_KICK_F:
	case BOTH_A7_KICK_B:
	case BOTH_A7_KICK_R:
	case BOTH_A7_KICK_L:
	case BOTH_A7_KICK_S:
	case BOTH_A7_KICK_BF:
	case BOTH_A7_KICK_RL:
	case BOTH_A7_KICK_F_AIR:
	case BOTH_A7_KICK_B_AIR:
	case BOTH_A7_KICK_R_AIR:
	case BOTH_A7_KICK_L_AIR:
	case BOTH_A7_HILT:
	//NOT kicks, but do kick traces anyway
	case BOTH_GETUP_BROLL_B:
	case BOTH_GETUP_BROLL_F:
	case BOTH_GETUP_FROLL_B:
	case BOTH_GETUP_FROLL_F:
		return qtrue;
	}
	return qfalse;
}

int BG_InGrappleMove(int anim)
{
	switch (anim)
	{
	case BOTH_KYLE_GRAB:
	case BOTH_KYLE_MISS:
		return 1; //grabbing at someone
	case BOTH_KYLE_PA_1:
	case BOTH_KYLE_PA_2:
		return 2; //beating the shit out of someone
	case BOTH_PLAYER_PA_1:
	case BOTH_PLAYER_PA_2:
	case BOTH_PLAYER_PA_FLY:
		return 3; //getting the shit beaten out of you
	}

	return 0;
}

int BG_BrokenParryForAttack( int move, int stance )
{
	//Our attack was knocked away by a knockaway parry
	//FIXME: need actual anims for this
	//FIXME: need to know which side of the saber was hit!  For now, we presume the saber gets knocked away from the center
	switch ( SaberStances[stance].moves[move].startingQuadrant )
	{
	case Q_B:
		return LS_V1_B_;
	case Q_BR:
		return LS_V1_BR;
	case Q_R:
		return LS_V1__R;
	case Q_TR:
		return LS_V1_TR;
	case Q_T:
		return LS_V1_T_;
	case Q_TL:
		return LS_V1_TL;
	case Q_L:
		return LS_V1__L;
	case Q_BL:
		return LS_V1_BL;
	}
	return LS_NONE;
}

int BG_BrokenParryForParry( int move )
{
	//FIXME: need actual anims for this
	//FIXME: need to know which side of the saber was hit!  For now, we presume the saber gets knocked away from the center
	switch ( move )
	{
	case LS_PARRY_UP:
		//Hmm... since we don't know what dir the hit came from, randomly pick knock down or knock back
		if ( Q_irand( 0, 1 ) )
		{
			return LS_H1_B_;
		}
		else
		{
			return LS_H1_T_;
		}
		break;
	case LS_PARRY_UR:
		return LS_H1_TR;
	case LS_PARRY_UL:
		return LS_H1_TL;
	case LS_PARRY_LR:
		return LS_H1_BR;
	case LS_PARRY_LL:
		return LS_H1_BL;
	case LS_READY:
		return LS_H1_B_;//???
	}
	return LS_NONE;
}

int BG_KnockawayForParry( int move )
{
	//FIXME: need actual anims for this
	//FIXME: need to know which side of the saber was hit!  For now, we presume the saber gets knocked away from the center
	switch ( move )
	{
	case BLOCKED_TOP://LS_PARRY_UP:
		return LS_K1_T_;//push up
	case BLOCKED_UPPER_RIGHT://LS_PARRY_UR:
	default://case LS_READY:
		return LS_K1_TR;//push up, slightly to right
	case BLOCKED_UPPER_LEFT://LS_PARRY_UL:
		return LS_K1_TL;//push up and to left
	case BLOCKED_LOWER_RIGHT://LS_PARRY_LR:
		return LS_K1_BR;//push down and to left
	case BLOCKED_LOWER_LEFT://LS_PARRY_LL:
		return LS_K1_BL;//push down and to right
	}
	//return LS_NONE;
}

qboolean BG_InRoll( playerState_t *ps, int anim )
{
	switch ( (anim) )
	{
	case BOTH_GETUP_BROLL_B:
	case BOTH_GETUP_BROLL_F:
	case BOTH_GETUP_BROLL_L:
	case BOTH_GETUP_BROLL_R:
	case BOTH_GETUP_FROLL_B:
	case BOTH_GETUP_FROLL_F:
	case BOTH_GETUP_FROLL_L:
	case BOTH_GETUP_FROLL_R:
	case BOTH_ROLL_F:
	case BOTH_ROLL_B:
	case BOTH_ROLL_R:
	case BOTH_ROLL_L:
		if ( ps->legsTimer > 0 )
		{
			return qtrue;
		}
		break;
	}
	return qfalse;
}

qboolean BG_InSpecialDeathAnim( int anim )
{
	switch( anim )
	{
	case BOTH_DEATH_ROLL:		//# Death anim from a roll
	case BOTH_DEATH_FLIP:		//# Death anim from a flip
	case BOTH_DEATH_SPIN_90_R:	//# Death anim when facing 90 degrees right
	case BOTH_DEATH_SPIN_90_L:	//# Death anim when facing 90 degrees left
	case BOTH_DEATH_SPIN_180:	//# Death anim when facing backwards
	case BOTH_DEATH_LYING_UP:	//# Death anim when lying on back
	case BOTH_DEATH_LYING_DN:	//# Death anim when lying on front
	case BOTH_DEATH_FALLING_DN:	//# Death anim when falling on face
	case BOTH_DEATH_FALLING_UP:	//# Death anim when falling on back
	case BOTH_DEATH_CROUCHED:	//# Death anim when crouched
		return qtrue;
	default:
		return qfalse;
	}
}

qboolean BG_InDeathAnim ( int anim )
{//Purposely does not cover stumbledeath and falldeath...
	switch( anim )
	{
	case BOTH_DEATH1:		//# First Death anim
	case BOTH_DEATH2:			//# Second Death anim
	case BOTH_DEATH3:			//# Third Death anim
	case BOTH_DEATH4:			//# Fourth Death anim
	case BOTH_DEATH5:			//# Fifth Death anim
	case BOTH_DEATH6:			//# Sixth Death anim
	case BOTH_DEATH7:			//# Seventh Death anim
	case BOTH_DEATH8:			//# 
	case BOTH_DEATH9:			//# 
	case BOTH_DEATH10:			//# 
	case BOTH_DEATH11:			//#
	case BOTH_DEATH12:			//# 
	case BOTH_DEATH13:			//# 
	case BOTH_DEATH14:			//# 
	case BOTH_DEATH14_UNGRIP:	//# Desann's end death (cin #35)
	case BOTH_DEATH14_SITUP:		//# Tavion sitting up after having been thrown (cin #23)
	case BOTH_DEATH15:			//# 
	case BOTH_DEATH16:			//# 
	case BOTH_DEATH17:			//# 
	case BOTH_DEATH18:			//# 
	case BOTH_DEATH19:			//# 
	case BOTH_DEATH20:			//# 
	case BOTH_DEATH21:			//# 
	case BOTH_DEATH22:			//# 
	case BOTH_DEATH23:			//# 
	case BOTH_DEATH24:			//# 
	case BOTH_DEATH25:			//# 

	case BOTH_DEATHFORWARD1:		//# First Death in which they get thrown forward
	case BOTH_DEATHFORWARD2:		//# Second Death in which they get thrown forward
	case BOTH_DEATHFORWARD3:		//# Tavion's falling in cin# 23
	case BOTH_DEATHBACKWARD1:	//# First Death in which they get thrown backward
	case BOTH_DEATHBACKWARD2:	//# Second Death in which they get thrown backward

	case BOTH_DEATH1IDLE:		//# Idle while close to death
	case BOTH_LYINGDEATH1:		//# Death to play when killed lying down
	case BOTH_STUMBLEDEATH1:		//# Stumble forward and fall face first death
	case BOTH_FALLDEATH1:		//# Fall forward off a high cliff and splat death - start
	case BOTH_FALLDEATH1INAIR:	//# Fall forward off a high cliff and splat death - loop
	case BOTH_FALLDEATH1LAND:	//# Fall forward off a high cliff and splat death - hit bottom
	//# #sep case BOTH_ DEAD POSES # Should be last frame of corresponding previous anims
	case BOTH_DEAD1:				//# First Death finished pose
	case BOTH_DEAD2:				//# Second Death finished pose
	case BOTH_DEAD3:				//# Third Death finished pose
	case BOTH_DEAD4:				//# Fourth Death finished pose
	case BOTH_DEAD5:				//# Fifth Death finished pose
	case BOTH_DEAD6:				//# Sixth Death finished pose
	case BOTH_DEAD7:				//# Seventh Death finished pose
	case BOTH_DEAD8:				//# 
	case BOTH_DEAD9:				//# 
	case BOTH_DEAD10:			//# 
	case BOTH_DEAD11:			//#
	case BOTH_DEAD12:			//# 
	case BOTH_DEAD13:			//# 
	case BOTH_DEAD14:			//# 
	case BOTH_DEAD15:			//# 
	case BOTH_DEAD16:			//# 
	case BOTH_DEAD17:			//# 
	case BOTH_DEAD18:			//# 
	case BOTH_DEAD19:			//# 
	case BOTH_DEAD20:			//# 
	case BOTH_DEAD21:			//# 
	case BOTH_DEAD22:			//# 
	case BOTH_DEAD23:			//# 
	case BOTH_DEAD24:			//# 
	case BOTH_DEAD25:			//# 
	case BOTH_DEADFORWARD1:		//# First thrown forward death finished pose
	case BOTH_DEADFORWARD2:		//# Second thrown forward death finished pose
	case BOTH_DEADBACKWARD1:		//# First thrown backward death finished pose
	case BOTH_DEADBACKWARD2:		//# Second thrown backward death finished pose
	case BOTH_LYINGDEAD1:		//# Killed lying down death finished pose
	case BOTH_STUMBLEDEAD1:		//# Stumble forward death finished pose
	case BOTH_FALLDEAD1LAND:		//# Fall forward and splat death finished pose
	//# #sep case BOTH_ DEAD TWITCH/FLOP # React to being shot from death poses
	case BOTH_DEADFLOP1:		//# React to being shot from First Death finished pose
	case BOTH_DEADFLOP2:		//# React to being shot from Second Death finished pose
	case BOTH_DISMEMBER_HEAD1:	//#
	case BOTH_DISMEMBER_TORSO1:	//#
	case BOTH_DISMEMBER_LLEG:	//#
	case BOTH_DISMEMBER_RLEG:	//#
	case BOTH_DISMEMBER_RARM:	//#
	case BOTH_DISMEMBER_LARM:	//#
		return qtrue;
	default:
		return BG_InSpecialDeathAnim( anim );
	}
}

qboolean BG_InKnockDownOnly( int anim )
{
	switch ( anim )
	{
	case BOTH_KNOCKDOWN1:
	case BOTH_KNOCKDOWN2:
	case BOTH_KNOCKDOWN3:
	case BOTH_KNOCKDOWN4:
	case BOTH_KNOCKDOWN5:
		return qtrue;
	}
	return qfalse;
}

qboolean BG_InSaberLockOld( int anim )
{
	switch ( anim )
	{
	case BOTH_BF2LOCK:
	case BOTH_BF1LOCK:
	case BOTH_CWCIRCLELOCK:
	case BOTH_CCWCIRCLELOCK:
		return qtrue;
	}
	return qfalse;
}

qboolean BG_InSaberLock( int anim ) 
{
	switch ( anim )
	{
	case BOTH_LK_S_DL_S_L_1:		//lock if I'm using single vs. a dual
	case BOTH_LK_S_DL_T_L_1:		//lock if I'm using single vs. a dual
	case BOTH_LK_S_ST_S_L_1:		//lock if I'm using single vs. a staff
	case BOTH_LK_S_ST_T_L_1:		//lock if I'm using single vs. a staff
	case BOTH_LK_S_S_S_L_1:		//lock if I'm using single vs. a single and I initiated
	case BOTH_LK_S_S_T_L_1:		//lock if I'm using single vs. a single and I initiated
	case BOTH_LK_DL_DL_S_L_1:	//lock if I'm using dual vs. dual and I initiated
	case BOTH_LK_DL_DL_T_L_1:	//lock if I'm using dual vs. dual and I initiated
	case BOTH_LK_DL_ST_S_L_1:	//lock if I'm using dual vs. a staff
	case BOTH_LK_DL_ST_T_L_1:	//lock if I'm using dual vs. a staff
	case BOTH_LK_DL_S_S_L_1:		//lock if I'm using dual vs. a single
	case BOTH_LK_DL_S_T_L_1:		//lock if I'm using dual vs. a single
	case BOTH_LK_ST_DL_S_L_1:	//lock if I'm using staff vs. dual
	case BOTH_LK_ST_DL_T_L_1:	//lock if I'm using staff vs. dual
	case BOTH_LK_ST_ST_S_L_1:	//lock if I'm using staff vs. a staff and I initiated
	case BOTH_LK_ST_ST_T_L_1:	//lock if I'm using staff vs. a staff and I initiated
	case BOTH_LK_ST_S_S_L_1:		//lock if I'm using staff vs. a single
	case BOTH_LK_ST_S_T_L_1:		//lock if I'm using staff vs. a single
	case BOTH_LK_S_S_S_L_2:
	case BOTH_LK_S_S_T_L_2:
	case BOTH_LK_DL_DL_S_L_2:
	case BOTH_LK_DL_DL_T_L_2:
	case BOTH_LK_ST_ST_S_L_2:
	case BOTH_LK_ST_ST_T_L_2:
		return qtrue;
	default:
		return BG_InSaberLockOld( anim );
	}
}

//Called only where pm is valid (not all require pm, but some do):
qboolean PM_InCartwheel( int anim )
{
	switch ( anim )
	{
	case BOTH_ARIAL_LEFT:
	case BOTH_ARIAL_RIGHT:
	case BOTH_ARIAL_F1:
	case BOTH_CARTWHEEL_LEFT:
	case BOTH_CARTWHEEL_RIGHT:
		return qtrue;
	}
	return qfalse;
}

qboolean BG_InKnockDownOnGround( playerState_t *ps )
{
	switch ( ps->legsAnim )
	{
	case BOTH_KNOCKDOWN1:
	case BOTH_KNOCKDOWN2:
	case BOTH_KNOCKDOWN3:
	case BOTH_KNOCKDOWN4:
	case BOTH_KNOCKDOWN5:
	case BOTH_RELEASED:
		return qtrue;
	case BOTH_GETUP1:
	case BOTH_GETUP2:
	case BOTH_GETUP3:
	case BOTH_GETUP4:
	case BOTH_GETUP5:
	case BOTH_GETUP_CROUCH_F1:
	case BOTH_GETUP_CROUCH_B1:
	case BOTH_FORCE_GETUP_F1:
	case BOTH_FORCE_GETUP_F2:
	case BOTH_FORCE_GETUP_B1:
	case BOTH_FORCE_GETUP_B2:
	case BOTH_FORCE_GETUP_B3:
	case BOTH_FORCE_GETUP_B4:
	case BOTH_FORCE_GETUP_B5:
	case BOTH_FORCE_GETUP_B6:
		if ( BG_AnimLength( 0, (animNumber_t)ps->legsAnim ) - ps->legsTimer < 500 )
		{//at beginning of getup anim
			return qtrue;
		}
		break;
	case BOTH_GETUP_BROLL_B:
	case BOTH_GETUP_BROLL_F:
	case BOTH_GETUP_BROLL_L:
	case BOTH_GETUP_BROLL_R:
	case BOTH_GETUP_FROLL_B:
	case BOTH_GETUP_FROLL_F:
	case BOTH_GETUP_FROLL_L:
	case BOTH_GETUP_FROLL_R:
		if ( BG_AnimLength( 0, (animNumber_t)ps->legsAnim ) - ps->legsTimer < 500 )
		{//at beginning of getup anim
			return qtrue;
		}
		break;
	case BOTH_LK_DL_ST_T_SB_1_L:
		if ( ps->legsTimer < 1000 )
		{
			return qtrue;
		}
		break;
	case BOTH_PLAYER_PA_3_FLY:
		if ( ps->legsTimer < 300 )
		{
			return qtrue;
		}
		break;
	}
	return qfalse;
}

qboolean BG_StabDownAnim( int anim )
{
	switch ( anim )
	{
	case BOTH_STABDOWN:
	case BOTH_STABDOWN_STAFF:
	case BOTH_STABDOWN_DUAL:
		return qtrue;
	}
	return qfalse;
}

int PM_SaberBounceForAttack( int stance, int move )
{
	switch ( SaberStances[stance].moves[move].startingQuadrant )
	{
	case Q_B:
	case Q_BR:
		return LS_B1_BR;
	case Q_R:
		return LS_B1__R;
	case Q_TR:
		return LS_B1_TR;
	case Q_T:
		return LS_B1_T_;
	case Q_TL:
		return LS_B1_TL;
	case Q_L:
		return LS_B1__L;
	case Q_BL:
		return LS_B1_BL;
	}
	return LS_NONE;
}

int PM_SaberDeflectionForQuad( int quad )
{
	switch ( quad )
	{
	case Q_B:
		return LS_D1_B_;
	case Q_BR:
		return LS_D1_BR;
	case Q_R:
		return LS_D1__R;
	case Q_TR:
		return LS_D1_TR;
	case Q_T:
		return LS_D1_T_;
	case Q_TL:
		return LS_D1_TL;
	case Q_L:
		return LS_D1__L;
	case Q_BL:
		return LS_D1_BL;
	}
	return LS_NONE;
}

qboolean PM_SaberInDeflect( int move )
{
	if ( move >= LS_D1_BR && move <= LS_D1_B_ )
	{
		return qtrue;
	}
	return qfalse;
}

qboolean PM_SaberInParry( int move )
{
	if ( move >= LS_PARRY_UP && move <= LS_PARRY_LL )
	{
		return qtrue;
	}
	return qfalse;
}

qboolean PM_SaberInKnockaway( int move )
{
	if ( move >= LS_K1_T_ && move <= LS_K1_BL )
	{
		return qtrue;
	}
	return qfalse;
}

qboolean PM_SaberInReflect( int move )
{
	if ( move >= LS_REFLECT_UP && move <= LS_REFLECT_LL )
	{
		return qtrue;
	}
	return qfalse;
}

qboolean PM_SaberInStart( int move )
{
	if ( move >= LS_S_TL2BR && move <= LS_S_T2B )
	{
		return qtrue;
	}
	return qfalse;
}

qboolean PM_SaberInReturn( int move )
{
	if ( move >= LS_R_TL2BR && move <= LS_R_T2B )
	{
		return qtrue;
	}
	return qfalse;
}

qboolean BG_SaberInReturn( int move )
{
	return PM_SaberInReturn( move );
}

qboolean PM_InSaberAnim( int anim )
{
	if ( (anim) >= BOTH_A1_T__B_ && (anim) <= BOTH_H1_S1_BR )
	{
		return qtrue;
	}
	return qfalse;
}


//[KnockdownSys]
//[SPPortCompete]
qboolean PM_InForceGetUp( playerState_t *ps )
{//racc - Are we in a getup animation that uses the Force?
	switch ( ps->legsAnim )
	{
	case BOTH_FORCE_GETUP_F1:
	case BOTH_FORCE_GETUP_F2:
	case BOTH_FORCE_GETUP_B1:
	case BOTH_FORCE_GETUP_B2:
	case BOTH_FORCE_GETUP_B3:
	case BOTH_FORCE_GETUP_B4:
	case BOTH_FORCE_GETUP_B5:
	case BOTH_FORCE_GETUP_B6:
	case BOTH_GETUP_BROLL_B:
	case BOTH_GETUP_BROLL_F:
	case BOTH_GETUP_BROLL_L:
	case BOTH_GETUP_BROLL_R:
	case BOTH_GETUP_FROLL_B:
	case BOTH_GETUP_FROLL_F:
	case BOTH_GETUP_FROLL_L:
	case BOTH_GETUP_FROLL_R:
		if ( ps->legsTimer )
		{
			return qtrue;
		}
		break;
	}
	return qfalse;
}

qboolean PM_InGetUp( playerState_t *ps )
{//racc - player in getup animation.
	switch ( ps->legsAnim )
	{
	case BOTH_GETUP1:
	case BOTH_GETUP2:
	case BOTH_GETUP3:
	case BOTH_GETUP4:
	case BOTH_GETUP5:
	case BOTH_GETUP_CROUCH_F1:
	case BOTH_GETUP_CROUCH_B1:
	case BOTH_GETUP_BROLL_B:
	case BOTH_GETUP_BROLL_F:
	case BOTH_GETUP_BROLL_L:
	case BOTH_GETUP_BROLL_R:
	case BOTH_GETUP_FROLL_B:
	case BOTH_GETUP_FROLL_F:
	case BOTH_GETUP_FROLL_L:
	case BOTH_GETUP_FROLL_R:
		if ( ps->legsTimer )
		{
			return qtrue;
		}
		break;
	}
	return PM_InForceGetUp(ps);
}
//[/KnockdownSys]


qboolean PM_InKnockDown( playerState_t *ps )
{
	//[KnockdownSys] 
	//Porting in SP Code for this stuff.
	switch ( ps->legsAnim )
	{
	case BOTH_KNOCKDOWN1:
	case BOTH_KNOCKDOWN2:
	case BOTH_KNOCKDOWN3:
	case BOTH_KNOCKDOWN4:
	case BOTH_KNOCKDOWN5:
	//special anims:
	case BOTH_RELEASED:
		return qtrue;
	case BOTH_LK_DL_ST_T_SB_1_L:
		if ( ps->legsTimer < 550 )
		{
			return qtrue;
		}
		break;
	case BOTH_PLAYER_PA_3_FLY:
		if ( ps->legsTimer < 300 )
		{
			return qtrue;
		}
		break;
	default:
		return PM_InGetUp( ps );
	}
	return qfalse;
}

qboolean PM_PainAnim( int anim )
{
	switch ( (anim) )
	{
		case BOTH_PAIN1:				//# First take pain anim
		case BOTH_PAIN2:				//# Second take pain anim
		case BOTH_PAIN3:				//# Third take pain anim
		case BOTH_PAIN4:				//# Fourth take pain anim
		case BOTH_PAIN5:				//# Fifth take pain anim - from behind
		case BOTH_PAIN6:				//# Sixth take pain anim - from behind
		case BOTH_PAIN7:				//# Seventh take pain anim - from behind
		case BOTH_PAIN8:				//# Eigth take pain anim - from behind
		case BOTH_PAIN9:				//# 
		case BOTH_PAIN10:			//# 
		case BOTH_PAIN11:			//# 
		case BOTH_PAIN12:			//# 
		case BOTH_PAIN13:			//# 
		case BOTH_PAIN14:			//# 
		case BOTH_PAIN15:			//# 
		case BOTH_PAIN16:			//# 
		case BOTH_PAIN17:			//# 
		case BOTH_PAIN18:			//# 
		return qtrue;
	}
	return qfalse;
}

qboolean PM_JumpingAnim( int anim )
{
	switch ( (anim) )
	{
		case BOTH_JUMP1:				//# Jump - wind-up and leave ground
		case BOTH_INAIR1:			//# In air loop (from jump)
		case BOTH_LAND1:				//# Landing (from in air loop)
		case BOTH_LAND2:				//# Landing Hard (from a great height)
		case BOTH_JUMPBACK1:			//# Jump backwards - wind-up and leave ground
		case BOTH_INAIRBACK1:		//# In air loop (from jump back)
		case BOTH_LANDBACK1:			//# Landing backwards(from in air loop)
		case BOTH_JUMPLEFT1:			//# Jump left - wind-up and leave ground
		case BOTH_INAIRLEFT1:		//# In air loop (from jump left)
		case BOTH_LANDLEFT1:			//# Landing left(from in air loop)
		case BOTH_JUMPRIGHT1:		//# Jump right - wind-up and leave ground
		case BOTH_INAIRRIGHT1:		//# In air loop (from jump right)
		case BOTH_LANDRIGHT1:		//# Landing right(from in air loop)
		case BOTH_FORCEJUMP1:				//# Jump - wind-up and leave ground
		case BOTH_FORCEINAIR1:			//# In air loop (from jump)
		case BOTH_FORCELAND1:				//# Landing (from in air loop)
		case BOTH_FORCEJUMPBACK1:			//# Jump backwards - wind-up and leave ground
		case BOTH_FORCEINAIRBACK1:		//# In air loop (from jump back)
		case BOTH_FORCELANDBACK1:			//# Landing backwards(from in air loop)
		case BOTH_FORCEJUMPLEFT1:			//# Jump left - wind-up and leave ground
		case BOTH_FORCEINAIRLEFT1:		//# In air loop (from jump left)
		case BOTH_FORCELANDLEFT1:			//# Landing left(from in air loop)
		case BOTH_FORCEJUMPRIGHT1:		//# Jump right - wind-up and leave ground
		case BOTH_FORCEINAIRRIGHT1:		//# In air loop (from jump right)
		case BOTH_FORCELANDRIGHT1:		//# Landing right(from in air loop)
		return qtrue;
	}
	return qfalse;
}

qboolean PM_LandingAnim( int anim )
{
	switch ( (anim) )
	{
		case BOTH_LAND1:				//# Landing (from in air loop)
		case BOTH_LAND2:				//# Landing Hard (from a great height)
		case BOTH_LANDBACK1:			//# Landing backwards(from in air loop)
		case BOTH_LANDLEFT1:			//# Landing left(from in air loop)
		case BOTH_LANDRIGHT1:		//# Landing right(from in air loop)
		case BOTH_FORCELAND1:		//# Landing (from in air loop)
		case BOTH_FORCELANDBACK1:	//# Landing backwards(from in air loop)
		case BOTH_FORCELANDLEFT1:	//# Landing left(from in air loop)
		case BOTH_FORCELANDRIGHT1:	//# Landing right(from in air loop)
		return qtrue;
	}
	return qfalse;
}

qboolean PM_SpinningAnim( int anim )
{
	return BG_SpinningSaberAnim( anim );
}

qboolean PM_InOnGroundAnim ( int anim )
{
	switch( anim )
	{
	case BOTH_DEAD1:
	case BOTH_DEAD2:
	case BOTH_DEAD3:
	case BOTH_DEAD4:
	case BOTH_DEAD5:
	case BOTH_DEADFORWARD1:
	case BOTH_DEADBACKWARD1:
	case BOTH_DEADFORWARD2:
	case BOTH_DEADBACKWARD2:
	case BOTH_LYINGDEATH1:
	case BOTH_LYINGDEAD1:
	case BOTH_SLEEP1:			//# laying on back-rknee up-rhand on torso
	case BOTH_KNOCKDOWN1:		//# 
	case BOTH_KNOCKDOWN2:		//# 
	case BOTH_KNOCKDOWN3:		//# 
	case BOTH_KNOCKDOWN4:		//# 
	case BOTH_KNOCKDOWN5:		//# 
	case BOTH_GETUP1:
	case BOTH_GETUP2:
	case BOTH_GETUP3:
	case BOTH_GETUP4:
	case BOTH_GETUP5:
	case BOTH_GETUP_CROUCH_F1:
	case BOTH_GETUP_CROUCH_B1:
	case BOTH_FORCE_GETUP_F1:
	case BOTH_FORCE_GETUP_F2:
	case BOTH_FORCE_GETUP_B1:
	case BOTH_FORCE_GETUP_B2:
	case BOTH_FORCE_GETUP_B3:
	case BOTH_FORCE_GETUP_B4:
	case BOTH_FORCE_GETUP_B5:
	case BOTH_FORCE_GETUP_B6:
	case BOTH_GETUP_BROLL_B:
	case BOTH_GETUP_BROLL_F:
	case BOTH_GETUP_BROLL_L:
	case BOTH_GETUP_BROLL_R:
	case BOTH_GETUP_FROLL_B:
	case BOTH_GETUP_FROLL_F:
	case BOTH_GETUP_FROLL_L:
	case BOTH_GETUP_FROLL_R:
		return qtrue;
	}

	return qfalse;
}

qboolean PM_InRollComplete( playerState_t *ps, int anim )
{
	switch ( (anim) )
	{
	case BOTH_ROLL_F:
	case BOTH_ROLL_B:
	case BOTH_ROLL_R:
	case BOTH_ROLL_L:
		if ( ps->legsTimer < 1 )
		{
			return qtrue;
		}
		break;
	}
	return qfalse;
}

qboolean PM_CanRollFromSoulCal( playerState_t *ps )
{
	if ( ps->legsAnim == BOTH_A7_SOULCAL
		&& ps->legsTimer < 700
		&& ps->legsTimer > 250 )
	{
		return qtrue;
	}
	return qfalse;
}

qboolean BG_SuperBreakLoseAnim( int anim )
{
	switch ( anim )
	{
	case BOTH_LK_S_DL_S_SB_1_L:	//super break I lost
	case BOTH_LK_S_DL_T_SB_1_L:	//super break I lost
	case BOTH_LK_S_ST_S_SB_1_L:	//super break I lost
	case BOTH_LK_S_ST_T_SB_1_L:	//super break I lost
	case BOTH_LK_S_S_S_SB_1_L:	//super break I lost
	case BOTH_LK_S_S_T_SB_1_L:	//super break I lost
	case BOTH_LK_DL_DL_S_SB_1_L:	//super break I lost
	case BOTH_LK_DL_DL_T_SB_1_L:	//super break I lost
	case BOTH_LK_DL_ST_S_SB_1_L:	//super break I lost
	case BOTH_LK_DL_ST_T_SB_1_L:	//super break I lost
	case BOTH_LK_DL_S_S_SB_1_L:	//super break I lost
	case BOTH_LK_DL_S_T_SB_1_L:	//super break I lost
	case BOTH_LK_ST_DL_S_SB_1_L:	//super break I lost
	case BOTH_LK_ST_DL_T_SB_1_L:	//super break I lost
	case BOTH_LK_ST_ST_S_SB_1_L:	//super break I lost
	case BOTH_LK_ST_ST_T_SB_1_L:	//super break I lost
	case BOTH_LK_ST_S_S_SB_1_L:	//super break I lost
	case BOTH_LK_ST_S_T_SB_1_L:	//super break I lost
		return qtrue;
	}
	return qfalse;
}

qboolean BG_SuperBreakWinAnim( int anim )
{
	switch ( anim )
	{
	case BOTH_LK_S_DL_S_SB_1_W:	//super break I won
	case BOTH_LK_S_DL_T_SB_1_W:	//super break I won
	case BOTH_LK_S_ST_S_SB_1_W:	//super break I won
	case BOTH_LK_S_ST_T_SB_1_W:	//super break I won
	case BOTH_LK_S_S_S_SB_1_W:	//super break I won
	case BOTH_LK_S_S_T_SB_1_W:	//super break I won
	case BOTH_LK_DL_DL_S_SB_1_W:	//super break I won
	case BOTH_LK_DL_DL_T_SB_1_W:	//super break I won
	case BOTH_LK_DL_ST_S_SB_1_W:	//super break I won
	case BOTH_LK_DL_ST_T_SB_1_W:	//super break I won
	case BOTH_LK_DL_S_S_SB_1_W:	//super break I won
	case BOTH_LK_DL_S_T_SB_1_W:	//super break I won
	case BOTH_LK_ST_DL_S_SB_1_W:	//super break I won
	case BOTH_LK_ST_DL_T_SB_1_W:	//super break I won
	case BOTH_LK_ST_ST_S_SB_1_W:	//super break I won
	case BOTH_LK_ST_ST_T_SB_1_W:	//super break I won
	case BOTH_LK_ST_S_S_SB_1_W:	//super break I won
	case BOTH_LK_ST_S_T_SB_1_W:	//super break I won
		return qtrue;
	}
	return qfalse;
}


qboolean BG_SaberLockBreakAnim( int anim )
{
	switch ( anim )
	{
	case BOTH_BF1BREAK:
	case BOTH_BF2BREAK:
	case BOTH_CWCIRCLEBREAK:
	case BOTH_CCWCIRCLEBREAK:
	case BOTH_LK_S_DL_S_B_1_L:	//normal break I lost
	case BOTH_LK_S_DL_S_B_1_W:	//normal break I won
	case BOTH_LK_S_DL_T_B_1_L:	//normal break I lost
	case BOTH_LK_S_DL_T_B_1_W:	//normal break I won
	case BOTH_LK_S_ST_S_B_1_L:	//normal break I lost
	case BOTH_LK_S_ST_S_B_1_W:	//normal break I won
	case BOTH_LK_S_ST_T_B_1_L:	//normal break I lost
	case BOTH_LK_S_ST_T_B_1_W:	//normal break I won
	case BOTH_LK_S_S_S_B_1_L:	//normal break I lost
	case BOTH_LK_S_S_S_B_1_W:	//normal break I won
	case BOTH_LK_S_S_T_B_1_L:	//normal break I lost
	case BOTH_LK_S_S_T_B_1_W:	//normal break I won
	case BOTH_LK_DL_DL_S_B_1_L:	//normal break I lost
	case BOTH_LK_DL_DL_S_B_1_W:	//normal break I won
	case BOTH_LK_DL_DL_T_B_1_L:	//normal break I lost
	case BOTH_LK_DL_DL_T_B_1_W:	//normal break I won
	case BOTH_LK_DL_ST_S_B_1_L:	//normal break I lost
	case BOTH_LK_DL_ST_S_B_1_W:	//normal break I won
	case BOTH_LK_DL_ST_T_B_1_L:	//normal break I lost
	case BOTH_LK_DL_ST_T_B_1_W:	//normal break I won
	case BOTH_LK_DL_S_S_B_1_L:	//normal break I lost
	case BOTH_LK_DL_S_S_B_1_W:	//normal break I won
	case BOTH_LK_DL_S_T_B_1_L:	//normal break I lost
	case BOTH_LK_DL_S_T_B_1_W:	//normal break I won
	case BOTH_LK_ST_DL_S_B_1_L:	//normal break I lost
	case BOTH_LK_ST_DL_S_B_1_W:	//normal break I won
	case BOTH_LK_ST_DL_T_B_1_L:	//normal break I lost
	case BOTH_LK_ST_DL_T_B_1_W:	//normal break I won
	case BOTH_LK_ST_ST_S_B_1_L:	//normal break I lost
	case BOTH_LK_ST_ST_S_B_1_W:	//normal break I won
	case BOTH_LK_ST_ST_T_B_1_L:	//normal break I lost
	case BOTH_LK_ST_ST_T_B_1_W:	//normal break I won
	case BOTH_LK_ST_S_S_B_1_L:	//normal break I lost
	case BOTH_LK_ST_S_S_B_1_W:	//normal break I won
	case BOTH_LK_ST_S_T_B_1_L:	//normal break I lost
	case BOTH_LK_ST_S_T_B_1_W:	//normal break I won
		return qtrue;
	}
	return (BG_SuperBreakLoseAnim(anim)||BG_SuperBreakWinAnim(anim));
}

qboolean BG_KnockDownAnim( int anim )
{//racc - is this a "normal" knockdown animation?
	switch ( anim )
	{
	case BOTH_KNOCKDOWN1:
	case BOTH_KNOCKDOWN2:
	case BOTH_KNOCKDOWN3:
	case BOTH_KNOCKDOWN4:
	case BOTH_KNOCKDOWN5:
		return qtrue;
		break;
	}
	return qfalse;
}


qboolean PM_KnockDownAnimExtended( int anim )
{//racc - check anim to see if it's one of the getting-knockdowned/have-been-knockeddown ones.  
	//This doesn't include any of the knockdown getup animations
	switch ( anim )
	{
	case BOTH_KNOCKDOWN1:
	case BOTH_KNOCKDOWN2:
	case BOTH_KNOCKDOWN3:
	case BOTH_KNOCKDOWN4:
	case BOTH_KNOCKDOWN5:
	//special anims:
	case BOTH_RELEASED:
	case BOTH_LK_DL_ST_T_SB_1_L:
	case BOTH_PLAYER_PA_3_FLY:
		return qtrue;
		break;
	}
	return qfalse;
}

qboolean BG_FullBodyTauntAnim( int anim )
{
	switch ( anim )
	{
	case BOTH_GESTURE1:
	case BOTH_DUAL_TAUNT:
	case BOTH_STAFF_TAUNT:
	case BOTH_BOW:
	case BOTH_MEDITATE:
	case BOTH_SHOWOFF_FAST:
	case BOTH_SHOWOFF_MEDIUM:
	case BOTH_SHOWOFF_STRONG:
	case BOTH_SHOWOFF_DUAL:
	case BOTH_SHOWOFF_STAFF:
	case BOTH_VICTORY_FAST:
	case BOTH_VICTORY_MEDIUM:
	case BOTH_VICTORY_STRONG:
	case BOTH_VICTORY_DUAL:
	case BOTH_VICTORY_STAFF:
		return qtrue;
	}
	return qfalse;
}


/*
=============
BG_AnimLength

Get the "length" of an anim given the local anim index (which skeleton)
and anim number. Obviously does not take things like the length of the
anim while force speeding (as an example) and whatnot into account.
=============
*/
int BG_AnimLength( int index, animNumber_t anim )
{
	if (anim >= MAX_ANIMATIONS)
	{
		return -1;
	}
	
	return bgAllAnims[index].anims[anim].numFrames * fabs((float)(bgAllAnims[index].anims[anim].frameLerp));
}

//just use whatever pm->animations is
int PM_AnimLength( int index, animNumber_t anim )
{
	if (anim >= MAX_ANIMATIONS || !pm->animations)
	{
		return -1;
	}

	return pm->animations[anim].numFrames * fabs((float)(pm->animations[anim].frameLerp));
}

void PM_DebugLegsAnim(int anim)
{
	int oldAnim = (pm->ps->legsAnim);
	int newAnim = (anim);

	if (oldAnim < MAX_TOTALANIMATIONS && oldAnim >= BOTH_DEATH1 &&
		newAnim < MAX_TOTALANIMATIONS && newAnim >= BOTH_DEATH1)
	{
		Com_Printf("OLD: %s\n", animTable[oldAnim]);
		Com_Printf("NEW: %s\n", animTable[newAnim]);
	}
}

qboolean PM_SaberInTransition( int move )
{
	if ( move >= LS_T1_BR__R && move <= LS_T1_BL__L )
	{
		return qtrue;
	}
	return qfalse;
}

qboolean BG_SaberInTransitionAny( int move )
{
	if ( PM_SaberInStart( move ) )
	{
		return qtrue;
	}
	else if ( PM_SaberInTransition( move ) )
	{
		return qtrue;
	}
	else if ( PM_SaberInReturn( move ) )
	{
		return qtrue;
	}
	return qfalse;
}


int PM_DualStanceForSingleStance(int anim)
{
	switch (anim)
	{
	case BOTH_P1_S1_T_:
		return BOTH_P6_S6_T_;
	case BOTH_P1_S1_BL:
		return BOTH_P6_S6_BL;
	case BOTH_P1_S1_BR:
		return BOTH_P6_S6_BR;
	case BOTH_P1_S1_TL:
		return BOTH_P6_S6_TL;
	case BOTH_P1_S1_TR:
		return BOTH_P6_S6_TR;
	case BOTH_SABERDUAL_STANCE:
		return BOTH_H6_S6_BR;
	case BOTH_STAND2:
		return BOTH_SABERDUAL_STANCE;
	}
	return anim;
}

int PM_StaffStanceForSingleStance(int anim)
{
	switch (anim)
	{
	case BOTH_STAND2:
	case BOTH_SABERFAST_STANCE:
		return BOTH_P7_S7_TL;
	case BOTH_P7_S7_T_:
		return BOTH_P7_S7_TR;
	}
	return anim;
}


qboolean BG_SabersOff(playerState_t *ps)
{
	if (!ps->saberHolstered)
	{
		return qfalse;
	}
	if (SaberStances[ps->fd.saberAnimLevel].isDualsOnly
		|| SaberStances[ps->fd.saberAnimLevel].isStaffOnly ||
		SaberStances[ps->fd.saberAnimLevel].isDualsFriendly ||
		SaberStances[ps->fd.saberAnimLevel].isStaffFriendly)
	{
		if (ps->saberHolstered < 2)
		{
			return qfalse;
		}
	}
	return qtrue;
}

int PM_GetSaberStance(void)
{
	int anim = BOTH_STAND2;
	saberInfo_t *saber1 = BG_MySaber(pm->ps->clientNum, 0);
	saberInfo_t *saber2 = BG_MySaber(pm->ps->clientNum, 1);

	if (!pm->ps->saberEntityNum)
	{ //lost it
		return BOTH_STAND1;
	}

	if (BG_SabersOff(pm->ps))
	{
		return BG_GetWeaponDataByIndex(pm->ps->weaponId)->anims.ready.torsoAnim;
	}

	if (saber1
		&& saber1->readyAnim != -1)
	{
		return saber1->readyAnim;
	}

	if (saber2
		&& saber2->readyAnim != -1)
	{
		return saber2->readyAnim;
	}

	if (!(pm->ps->saberActionFlags & (1 << SAF_BLOCKING))
		&& !(pm->ps->weaponstate == WEAPON_DROPPING
		|| pm->ps->weaponstate == WEAPON_RAISING)/* && bAllowRelaxedAnim*/ &&
		!pm->cmd.upmove && !pm->cmd.forwardmove && !pm->cmd.rightmove &&
		pm->ps->torsoAnim != BOTH_STAND1TO2 &&
		pm->ps->torsoAnim != BOTH_STAND2TO1)
	{

		return SaberStances[pm->ps->fd.saberAnimLevel].offensiveStanceAnim;
	}

	if (pm->ps->saberActionFlags & (1 << SAF_BLOCKING) && pm->cmd.upmove <= 0)
	{
		if ((pm->cmd.forwardmove != 0 || pm->cmd.rightmove != 0))
		{
			// Use a directional set of animations for blocking, which readies us for the parry --eez
			if (pm->cmd.forwardmove < 0)
			{
				// moving backward/sideways
				anim = BOTH_P7_S7_T_;
			}
			else if (pm->cmd.forwardmove != 0)
			{
				anim = BOTH_P1_S1_T_; // use top as default, since most stances face that way anyway --eez
			}
			if (pm->cmd.rightmove != 0)
			{
				// moving sideways
				if (pm->cmd.rightmove > 0)	// moving right
				{
					if (anim == BOTH_P1_S1_T_)	// going forward
					{
						anim = BOTH_P1_S1_BL;
					}
					else if (anim == BOTH_P7_S7_T_)	// going backward
					{
						anim = BOTH_P1_S1_T_;
					}
					else								// just straight right
					{
						anim = BOTH_P1_S1_TR;
					}
				}
				else						// moving left
				{
					if (anim == BOTH_P1_S1_T_)
					{
						// forward
						anim = BOTH_P1_S1_BR;
					}
					else if (anim == BOTH_P7_S7_T_)	// going backward
					{
						anim = BOTH_P7_S7_T_;
					}
					else
					{
						anim = BOTH_P1_S1_TL;
					}
				}
			}
		}
		if (pm->ps->saberActionFlags & (1 << SAF_PROJBLOCKING))
		{
			return SaberStances[pm->ps->fd.saberAnimLevel].projectileBlockAnim;
		}
		if (saber1
			&& saber2
			&& !pm->ps->saberHolstered)
		{
			return PM_DualStanceForSingleStance(anim);
		}
		else if (SaberStances[pm->ps->fd.saberAnimLevel].isStaffOnly)
		{
			return PM_StaffStanceForSingleStance(anim);
		}
		if (anim != BOTH_STAND2)
		{ // so stances are still distinguished --eez
			return anim;
		}
	}
	anim = SaberStances[pm->ps->fd.saberAnimLevel].moves[LS_READY].anim;
	return anim;
}

/*
===================
PM_AnimateJetpack

===================
*/
void PM_AnimateJetpack(void) {
	if (pm->cmd.rightmove > 0)
	{
		PM_ContinueLegsAnim(BOTH_FORCEJUMPRIGHT1);
	}
	else if (pm->cmd.rightmove < 0)
	{
		PM_ContinueLegsAnim(BOTH_FORCEJUMPLEFT1);
	}
	else if (pm->cmd.forwardmove > 0)
	{
		if (pm->cmd.buttons & BUTTON_SPRINT) {
			PM_ContinueLegsAnim(BOTH_FORCELONGLEAP_START);
		}
		else if(pm->ps->legsAnim == BOTH_FORCELONGLEAP_START || pm->ps->legsAnim == BOTH_FORCELONGLEAP_LAND) {
			PM_ContinueLegsAnim(BOTH_FORCELONGLEAP_LAND);
		}
		else {
			PM_ContinueLegsAnim(BOTH_FORCEJUMP1);
		}
	}
	else if (pm->cmd.forwardmove < 0)
	{
		if (pm->ps->legsAnim == BOTH_FORCELONGLEAP_START || pm->ps->legsAnim == BOTH_FORCELONGLEAP_LAND) {
			PM_ContinueLegsAnim(BOTH_FORCELONGLEAP_LAND);
		}
		else {
			PM_ContinueLegsAnim(BOTH_FORCEJUMPBACK1);
		}
	}
	else
	{
		PM_ContinueLegsAnim(BOTH_INAIR1);
	}
}

/*
=======================
Checks for each specific type of animation:
	* Walking
	* Running
	* Swimming
	* Rolling
	* Slope

=======================
*/
qboolean BG_WalkingAnim(int anim)
{
	switch (anim)
	{
	case BOTH_WALK1:				//# Normal walk
	case BOTH_WALK2:				//# Normal walk with saber
	case BOTH_WALK_STAFF:			//# Normal walk with staff
	case BOTH_WALK_DUAL:			//# Normal walk with staff
	case BOTH_WALK5:				//# Tavion taunting Kyle (cin 22)
	case BOTH_WALK6:				//# Slow walk for Luke (cin 12)
	case BOTH_WALK7:				//# Fast walk
	case BOTH_WALKBACK1:			//# Walk1 backwards
	case BOTH_WALKBACK2:			//# Walk2 backwards
	case BOTH_WALKBACK_STAFF:		//# Walk backwards with staff
	case BOTH_WALKBACK_DUAL:		//# Walk backwards with dual
	case BOTH_FEMALEEWALK:			//# JKG: Female walking
	case BOTH_FEMALEEWALKBACK:		//# JKG: Femake walking (backwards)
		return qtrue;
	}
	return qfalse;
}

qboolean BG_RunningAnim(int anim)
{
	switch ((anim))
	{
	case BOTH_RUN1:
	case BOTH_RUN2:
	case BOTH_RUN_STAFF:
	case BOTH_RUN_DUAL:
	case BOTH_RUNBACK1:
	case BOTH_RUNBACK2:
	case BOTH_RUNBACK_STAFF:
	case BOTH_RUNBACK_DUAL:
	case BOTH_RUN1START:			//# Start into full run1
	case BOTH_RUN1STOP:			//# Stop from full run1
	case BOTH_RUNSTRAFE_LEFT1:	//# Sidestep left: should loop
	case BOTH_RUNSTRAFE_RIGHT1:	//# Sidestep right: should loop
	case BOTH_FEMALERUN:
		return qtrue;
	}
	return qfalse;
}

qboolean BG_SwimmingAnim(int anim)
{
	switch (anim)
	{
	case BOTH_SWIM_IDLE1:		//# Swimming Idle 1
	case BOTH_SWIMFORWARD:		//# Swim forward loop
	case BOTH_SWIMBACKWARD:		//# Swim backward loop
		return qtrue;
	}
	return qfalse;
}

qboolean BG_RollingAnim(int anim)
{
	switch (anim)
	{
	case BOTH_ROLL_F:			//# Roll forward
	case BOTH_ROLL_B:			//# Roll backward
	case BOTH_ROLL_L:			//# Roll left
	case BOTH_ROLL_R:			//# Roll right
	case BOTH_GETUP_BROLL_B:
	case BOTH_GETUP_BROLL_F:
	case BOTH_GETUP_BROLL_L:
	case BOTH_GETUP_BROLL_R:
	case BOTH_GETUP_FROLL_B:
	case BOTH_GETUP_FROLL_F:
	case BOTH_GETUP_FROLL_L:
	case BOTH_GETUP_FROLL_R:
		return qtrue;
	}
	return qfalse;
}

qboolean BG_InSlopeAnim(int anim)
{
	switch (anim)
	{
	case LEGS_LEFTUP1:			//# On a slope with left foot 4 higher than right
	case LEGS_LEFTUP2:			//# On a slope with left foot 8 higher than right
	case LEGS_LEFTUP3:			//# On a slope with left foot 12 higher than right
	case LEGS_LEFTUP4:			//# On a slope with left foot 16 higher than right
	case LEGS_LEFTUP5:			//# On a slope with left foot 20 higher than right
	case LEGS_RIGHTUP1:			//# On a slope with RIGHT foot 4 higher than left
	case LEGS_RIGHTUP2:			//# On a slope with RIGHT foot 8 higher than left
	case LEGS_RIGHTUP3:			//# On a slope with RIGHT foot 12 higher than left
	case LEGS_RIGHTUP4:			//# On a slope with RIGHT foot 16 higher than left
	case LEGS_RIGHTUP5:			//# On a slope with RIGHT foot 20 higher than left
	case LEGS_S1_LUP1:
	case LEGS_S1_LUP2:
	case LEGS_S1_LUP3:
	case LEGS_S1_LUP4:
	case LEGS_S1_LUP5:
	case LEGS_S1_RUP1:
	case LEGS_S1_RUP2:
	case LEGS_S1_RUP3:
	case LEGS_S1_RUP4:
	case LEGS_S1_RUP5:
	case LEGS_S3_LUP1:
	case LEGS_S3_LUP2:
	case LEGS_S3_LUP3:
	case LEGS_S3_LUP4:
	case LEGS_S3_LUP5:
	case LEGS_S3_RUP1:
	case LEGS_S3_RUP2:
	case LEGS_S3_RUP3:
	case LEGS_S3_RUP4:
	case LEGS_S3_RUP5:
	case LEGS_S4_LUP1:
	case LEGS_S4_LUP2:
	case LEGS_S4_LUP3:
	case LEGS_S4_LUP4:
	case LEGS_S4_LUP5:
	case LEGS_S4_RUP1:
	case LEGS_S4_RUP2:
	case LEGS_S4_RUP3:
	case LEGS_S4_RUP4:
	case LEGS_S4_RUP5:
	case LEGS_S5_LUP1:
	case LEGS_S5_LUP2:
	case LEGS_S5_LUP3:
	case LEGS_S5_LUP4:
	case LEGS_S5_LUP5:
	case LEGS_S5_RUP1:
	case LEGS_S5_RUP2:
	case LEGS_S5_RUP3:
	case LEGS_S5_RUP4:
	case LEGS_S5_RUP5:
		return qtrue;
	}
	return qfalse;
}

/*
=================
PM_AdjustStandAnimForSlope

=================
*/

#define	SLOPE_RECALC_INT 100

extern void PM_FootSlopeTrace(float *pDiff, float *pInterval);
qboolean PM_AdjustStandAnimForSlope(void)
{
	float	diff;
	float	interval;
	int		destAnim;
	int		legsAnim;
#define SLOPERECALCVAR pm->ps->slopeRecalcTime //this is purely convenience

	if (!pm->ghoul2)
	{ //probably just changed models and not quite in sync yet
		return qfalse;
	}

	if (pm->g2Bolts_LFoot == -1 || pm->g2Bolts_RFoot == -1)
	{//need these bolts!
		return qfalse;
	}

	//step 1: find the 2 foot tags
	PM_FootSlopeTrace(&diff, &interval);

	//step 4: based on difference, choose one of the left/right slope-match intervals
	if (diff >= interval * 5)
	{
		destAnim = LEGS_LEFTUP5;
	}
	else if (diff >= interval * 4)
	{
		destAnim = LEGS_LEFTUP4;
	}
	else if (diff >= interval * 3)
	{
		destAnim = LEGS_LEFTUP3;
	}
	else if (diff >= interval * 2)
	{
		destAnim = LEGS_LEFTUP2;
	}
	else if (diff >= interval)
	{
		destAnim = LEGS_LEFTUP1;
	}
	else if (diff <= interval*-5)
	{
		destAnim = LEGS_RIGHTUP5;
	}
	else if (diff <= interval*-4)
	{
		destAnim = LEGS_RIGHTUP4;
	}
	else if (diff <= interval*-3)
	{
		destAnim = LEGS_RIGHTUP3;
	}
	else if (diff <= interval*-2)
	{
		destAnim = LEGS_RIGHTUP2;
	}
	else if (diff <= interval*-1)
	{
		destAnim = LEGS_RIGHTUP1;
	}
	else
	{
		return qfalse;
	}

	legsAnim = pm->ps->legsAnim;
	//adjust for current legs anim
	switch (legsAnim)
	{
	case BOTH_STAND1:

	case LEGS_S1_LUP1:
	case LEGS_S1_LUP2:
	case LEGS_S1_LUP3:
	case LEGS_S1_LUP4:
	case LEGS_S1_LUP5:
	case LEGS_S1_RUP1:
	case LEGS_S1_RUP2:
	case LEGS_S1_RUP3:
	case LEGS_S1_RUP4:
	case LEGS_S1_RUP5:
		destAnim = LEGS_S1_LUP1 + (destAnim - LEGS_LEFTUP1);
		break;
	case BOTH_STAND2:
	case BOTH_SABERFAST_STANCE:
	case BOTH_SABERSLOW_STANCE:
	case BOTH_CROUCH1IDLE:
	case BOTH_CROUCH1:
	case LEGS_LEFTUP1:			//# On a slope with left foot 4 higher than right
	case LEGS_LEFTUP2:			//# On a slope with left foot 8 higher than right
	case LEGS_LEFTUP3:			//# On a slope with left foot 12 higher than right
	case LEGS_LEFTUP4:			//# On a slope with left foot 16 higher than right
	case LEGS_LEFTUP5:			//# On a slope with left foot 20 higher than right
	case LEGS_RIGHTUP1:			//# On a slope with RIGHT foot 4 higher than left
	case LEGS_RIGHTUP2:			//# On a slope with RIGHT foot 8 higher than left
	case LEGS_RIGHTUP3:			//# On a slope with RIGHT foot 12 higher than left
	case LEGS_RIGHTUP4:			//# On a slope with RIGHT foot 16 higher than left
	case LEGS_RIGHTUP5:			//# On a slope with RIGHT foot 20 higher than left
		//fine
		break;
	case BOTH_STAND3:
	case LEGS_S3_LUP1:
	case LEGS_S3_LUP2:
	case LEGS_S3_LUP3:
	case LEGS_S3_LUP4:
	case LEGS_S3_LUP5:
	case LEGS_S3_RUP1:
	case LEGS_S3_RUP2:
	case LEGS_S3_RUP3:
	case LEGS_S3_RUP4:
	case LEGS_S3_RUP5:
		destAnim = LEGS_S3_LUP1 + (destAnim - LEGS_LEFTUP1);
		break;
	case BOTH_STAND4:
	case LEGS_S4_LUP1:
	case LEGS_S4_LUP2:
	case LEGS_S4_LUP3:
	case LEGS_S4_LUP4:
	case LEGS_S4_LUP5:
	case LEGS_S4_RUP1:
	case LEGS_S4_RUP2:
	case LEGS_S4_RUP3:
	case LEGS_S4_RUP4:
	case LEGS_S4_RUP5:
		destAnim = LEGS_S4_LUP1 + (destAnim - LEGS_LEFTUP1);
		break;
	case BOTH_STAND5:
	case LEGS_S5_LUP1:
	case LEGS_S5_LUP2:
	case LEGS_S5_LUP3:
	case LEGS_S5_LUP4:
	case LEGS_S5_LUP5:
	case LEGS_S5_RUP1:
	case LEGS_S5_RUP2:
	case LEGS_S5_RUP3:
	case LEGS_S5_RUP4:
	case LEGS_S5_RUP5:
		destAnim = LEGS_S5_LUP1 + (destAnim - LEGS_LEFTUP1);
		break;
	case BOTH_STAND6:
	default:
		return qfalse;
	}

	//step 5: based on the chosen interval and the current legsAnim, pick the correct anim
	//step 6: increment/decrement to the dest anim, not instant
	if ((legsAnim >= LEGS_LEFTUP1 && legsAnim <= LEGS_LEFTUP5)
		|| (legsAnim >= LEGS_S1_LUP1 && legsAnim <= LEGS_S1_LUP5)
		|| (legsAnim >= LEGS_S3_LUP1 && legsAnim <= LEGS_S3_LUP5)
		|| (legsAnim >= LEGS_S4_LUP1 && legsAnim <= LEGS_S4_LUP5)
		|| (legsAnim >= LEGS_S5_LUP1 && legsAnim <= LEGS_S5_LUP5))
	{//already in left-side up
		if (destAnim > legsAnim && SLOPERECALCVAR < pm->cmd.serverTime)
		{
			legsAnim++;
			SLOPERECALCVAR = pm->cmd.serverTime + SLOPE_RECALC_INT;
		}
		else if (destAnim < legsAnim && SLOPERECALCVAR < pm->cmd.serverTime)
		{
			legsAnim--;
			SLOPERECALCVAR = pm->cmd.serverTime + SLOPE_RECALC_INT;
		}
		else //if (SLOPERECALCVAR < pm->cmd.serverTime)
		{
			legsAnim = destAnim;
		}

		destAnim = legsAnim;
	}
	else if ((legsAnim >= LEGS_RIGHTUP1 && legsAnim <= LEGS_RIGHTUP5)
		|| (legsAnim >= LEGS_S1_RUP1 && legsAnim <= LEGS_S1_RUP5)
		|| (legsAnim >= LEGS_S3_RUP1 && legsAnim <= LEGS_S3_RUP5)
		|| (legsAnim >= LEGS_S4_RUP1 && legsAnim <= LEGS_S4_RUP5)
		|| (legsAnim >= LEGS_S5_RUP1 && legsAnim <= LEGS_S5_RUP5))
	{//already in right-side up
		if (destAnim > legsAnim && SLOPERECALCVAR < pm->cmd.serverTime)
		{
			legsAnim++;
			SLOPERECALCVAR = pm->cmd.serverTime + SLOPE_RECALC_INT;
		}
		else if (destAnim < legsAnim && SLOPERECALCVAR < pm->cmd.serverTime)
		{
			legsAnim--;
			SLOPERECALCVAR = pm->cmd.serverTime + SLOPE_RECALC_INT;
		}
		else //if (SLOPERECALCVAR < pm->cmd.serverTime)
		{
			legsAnim = destAnim;
		}

		destAnim = legsAnim;
	}
	else
	{//in a stand of some sort?
		switch (legsAnim)
		{
		case BOTH_STAND1:
		case TORSO_WEAPONREADY1:
		case TORSO_WEAPONREADY2:
		case TORSO_WEAPONREADY3:
		case TORSO_WEAPONREADY10:

			if (destAnim >= LEGS_S1_LUP1 && destAnim <= LEGS_S1_LUP5)
			{//going into left side up
				destAnim = LEGS_S1_LUP1;
				SLOPERECALCVAR = pm->cmd.serverTime + SLOPE_RECALC_INT;
			}
			else if (destAnim >= LEGS_S1_RUP1 && destAnim <= LEGS_S1_RUP5)
			{//going into right side up
				destAnim = LEGS_S1_RUP1;
				SLOPERECALCVAR = pm->cmd.serverTime + SLOPE_RECALC_INT;
			}
			else
			{//will never get here
				return qfalse;
			}
			break;
		case BOTH_STAND2:
		case BOTH_SABERFAST_STANCE:
		case BOTH_SABERSLOW_STANCE:
		case BOTH_CROUCH1IDLE:
			if (destAnim >= LEGS_LEFTUP1 && destAnim <= LEGS_LEFTUP5)
			{//going into left side up
				destAnim = LEGS_LEFTUP1;
				SLOPERECALCVAR = pm->cmd.serverTime + SLOPE_RECALC_INT;
			}
			else if (destAnim >= LEGS_RIGHTUP1 && destAnim <= LEGS_RIGHTUP5)
			{//going into right side up
				destAnim = LEGS_RIGHTUP1;
				SLOPERECALCVAR = pm->cmd.serverTime + SLOPE_RECALC_INT;
			}
			else
			{//will never get here
				return qfalse;
			}
			break;
		case BOTH_STAND3:
			if (destAnim >= LEGS_S3_LUP1 && destAnim <= LEGS_S3_LUP5)
			{//going into left side up
				destAnim = LEGS_S3_LUP1;
				SLOPERECALCVAR = pm->cmd.serverTime + SLOPE_RECALC_INT;
			}
			else if (destAnim >= LEGS_S3_RUP1 && destAnim <= LEGS_S3_RUP5)
			{//going into right side up
				destAnim = LEGS_S3_RUP1;
				SLOPERECALCVAR = pm->cmd.serverTime + SLOPE_RECALC_INT;
			}
			else
			{//will never get here
				return qfalse;
			}
			break;
		case BOTH_STAND4:
			if (destAnim >= LEGS_S4_LUP1 && destAnim <= LEGS_S4_LUP5)
			{//going into left side up
				destAnim = LEGS_S4_LUP1;
				SLOPERECALCVAR = pm->cmd.serverTime + SLOPE_RECALC_INT;
			}
			else if (destAnim >= LEGS_S4_RUP1 && destAnim <= LEGS_S4_RUP5)
			{//going into right side up
				destAnim = LEGS_S4_RUP1;
				SLOPERECALCVAR = pm->cmd.serverTime + SLOPE_RECALC_INT;
			}
			else
			{//will never get here
				return qfalse;
			}
			break;
		case BOTH_STAND5:
			if (destAnim >= LEGS_S5_LUP1 && destAnim <= LEGS_S5_LUP5)
			{//going into left side up
				destAnim = LEGS_S5_LUP1;
				SLOPERECALCVAR = pm->cmd.serverTime + SLOPE_RECALC_INT;
			}
			else if (destAnim >= LEGS_S5_RUP1 && destAnim <= LEGS_S5_RUP5)
			{//going into right side up
				destAnim = LEGS_S5_RUP1;
				SLOPERECALCVAR = pm->cmd.serverTime + SLOPE_RECALC_INT;
			}
			else
			{//will never get here
				return qfalse;
			}
			break;
		case BOTH_STAND6:
		default:
			return qfalse;
			break;
		}
	}
	//step 7: set the anim
	//PM_SetAnim( SETANIM_LEGS, destAnim, SETANIM_FLAG_NORMAL);
	PM_ContinueLegsAnim(destAnim);

	return qtrue;
}


/*
=================
PM_LegsSlopeBackTransition
rww - slowly back out of slope leg anims, to prevent skipping between slope anims and general jittering
=================
*/
int PM_LegsSlopeBackTransition(int desiredAnim)
{
	int anim = pm->ps->legsAnim;
	int resultingAnim = desiredAnim;

	switch (anim)
	{
	case LEGS_LEFTUP2:			//# On a slope with left foot 8 higher than right
	case LEGS_LEFTUP3:			//# On a slope with left foot 12 higher than right
	case LEGS_LEFTUP4:			//# On a slope with left foot 16 higher than right
	case LEGS_LEFTUP5:			//# On a slope with left foot 20 higher than right
	case LEGS_RIGHTUP2:			//# On a slope with RIGHT foot 8 higher than left
	case LEGS_RIGHTUP3:			//# On a slope with RIGHT foot 12 higher than left
	case LEGS_RIGHTUP4:			//# On a slope with RIGHT foot 16 higher than left
	case LEGS_RIGHTUP5:			//# On a slope with RIGHT foot 20 higher than left
	case LEGS_S1_LUP2:
	case LEGS_S1_LUP3:
	case LEGS_S1_LUP4:
	case LEGS_S1_LUP5:
	case LEGS_S1_RUP2:
	case LEGS_S1_RUP3:
	case LEGS_S1_RUP4:
	case LEGS_S1_RUP5:
	case LEGS_S3_LUP2:
	case LEGS_S3_LUP3:
	case LEGS_S3_LUP4:
	case LEGS_S3_LUP5:
	case LEGS_S3_RUP2:
	case LEGS_S3_RUP3:
	case LEGS_S3_RUP4:
	case LEGS_S3_RUP5:
	case LEGS_S4_LUP2:
	case LEGS_S4_LUP3:
	case LEGS_S4_LUP4:
	case LEGS_S4_LUP5:
	case LEGS_S4_RUP2:
	case LEGS_S4_RUP3:
	case LEGS_S4_RUP4:
	case LEGS_S4_RUP5:
	case LEGS_S5_LUP2:
	case LEGS_S5_LUP3:
	case LEGS_S5_LUP4:
	case LEGS_S5_LUP5:
	case LEGS_S5_RUP2:
	case LEGS_S5_RUP3:
	case LEGS_S5_RUP4:
	case LEGS_S5_RUP5:
		if (pm->ps->slopeRecalcTime < pm->cmd.serverTime)
		{
			resultingAnim = anim - 1;
			pm->ps->slopeRecalcTime = pm->cmd.serverTime + 8;//SLOPE_RECALC_INT;
		}
		else
		{
			resultingAnim = anim;
		}
		VectorClear(pm->ps->velocity);
		break;
	}

	return resultingAnim;
}

/*
========================
PM_WeaponAnimate

========================
*/
qboolean PM_CanSetWeaponAnims(void)
{
	if (pm->ps->m_iVehicleNum)
	{
		return qfalse;
	}

	if (BG_IsSprinting(pm->ps, &pm->cmd, qtrue))
	{
		return qfalse;
	}

	return qtrue;
}

qboolean PM_WeaponAnimate(void)
{
	if (!PM_CanSetWeaponAnims()) {
		return qtrue;
	}

	if (PM_CanSetWeaponAnims() && BG_IsSprinting(pm->ps, &pm->cmd, qtrue))
	{
		// FIXME: ummm...what
		if (pm->ps->weapon == WP_DET_PACK)
		{
			if (pm->ps->weapon == WP_THERMAL)
			{
				if ((pm->ps->torsoAnim) == GetWeaponData(pm->ps->weapon, pm->ps->weaponVariation)->anims.firing.torsoAnim &&
					(pm->ps->weaponTime - 200) <= 0)
				{
					PM_StartTorsoAnim(GetWeaponData(pm->ps->weapon, pm->ps->weaponVariation)->anims.ready.torsoAnim);
				}
			}
			else
			{
				if ((pm->ps->torsoAnim) == GetWeaponData(pm->ps->weapon, pm->ps->weaponVariation)->anims.firing.torsoAnim &&
					(pm->ps->weaponTime - 700) <= 0)
				{
					PM_StartTorsoAnim(GetWeaponData(pm->ps->weapon, pm->ps->weaponVariation)->anims.ready.torsoAnim);
				}
			}
		}
	}
	else if (pm->ps->forceHandExtend == HANDEXTEND_WEAPONREADY)
	{ //reset into weapon stance
		if (pm->ps->weapon != WP_SABER && pm->ps->weapon != WP_MELEE)
		{ //saber handles its own anims
			if (pm->ps->weapon == WP_DISRUPTOR && pm->ps->zoomMode == 1)
			{
				//PM_StartTorsoAnim( TORSO_WEAPONREADY4 );
				PM_StartTorsoAnim(TORSO_RAISEWEAP1);
			}
			else
			{
				if (pm->ps->weapon == WP_EMPLACED_GUN)
				{
					PM_StartTorsoAnim(BOTH_GUNSIT1);
				}
				else
				{
					//PM_StartTorsoAnim( WeaponReadyAnim[pm->ps->weapon] );
					PM_StartTorsoAnim(TORSO_RAISEWEAP1);
				}
			}
		}

		//we now go into a weapon raise anim after every force hand extend.
		//this is so that my holster-view-weapon-when-hand-extend stuff works.
		pm->ps->weaponstate = WEAPON_RAISING;
		pm->ps->weaponTime += 250;

		pm->ps->forceHandExtend = HANDEXTEND_NONE;
	}
	else if (pm->ps->forceHandExtend != HANDEXTEND_NONE)
	{ //nothing else should be allowed to happen during this time, including weapon fire
		int desiredAnim = 0;
		qboolean seperateOnTorso = qfalse;
		qboolean playFullBody = qfalse;
		int desiredOnTorso = 0;

		switch (pm->ps->forceHandExtend)
		{
		case HANDEXTEND_FORCEPUSH:
			desiredAnim = BOTH_FORCEPUSH;
			break;
		case HANDEXTEND_FORCEPULL:
			desiredAnim = BOTH_FORCEPULL;
			break;
		case HANDEXTEND_FORCE_HOLD:
			if ((pm->ps->fd.forcePowersActive&(1 << FP_GRIP)))
			{//gripping
				desiredAnim = BOTH_FORCEGRIP_HOLD;
			}
			else if ((pm->ps->fd.forcePowersActive&(1 << FP_LIGHTNING)))
			{//lightning
				if (pm->ps->weapon == WP_MELEE
					&& pm->ps->activeForcePass > FORCE_LEVEL_2)
				{//2-handed lightning
					desiredAnim = BOTH_FORCE_2HANDEDLIGHTNING_HOLD;
				}
				else
				{
					desiredAnim = BOTH_FORCELIGHTNING_HOLD;
				}
			}
			else if ((pm->ps->fd.forcePowersActive&(1 << FP_DRAIN)))
			{//draining
				desiredAnim = BOTH_FORCEGRIP_HOLD;
			}
			else
			{//???
				desiredAnim = BOTH_FORCEGRIP_HOLD;
			}
			break;
		case HANDEXTEND_SABERPULL:
			desiredAnim = BOTH_SABERPULL;
			break;
		case HANDEXTEND_CHOKE:
			desiredAnim = BOTH_CHOKE3; //left-handed choke
			break;
		case HANDEXTEND_DODGE:
			desiredAnim = pm->ps->forceDodgeAnim;
			break;
		case HANDEXTEND_KNOCKDOWN:
			if (pm->ps->forceDodgeAnim)
			{
				if (pm->ps->forceDodgeAnim > 4)
				{ //this means that we want to play a sepereate anim on the torso
					int originalDAnim = pm->ps->forceDodgeAnim - 8; //-8 is the original legs anim
					if (originalDAnim == 2)
					{
						desiredAnim = BOTH_FORCE_GETUP_B1;
					}
					else if (originalDAnim == 3)
					{
						desiredAnim = BOTH_FORCE_GETUP_B3;
					}
					else
					{
						desiredAnim = BOTH_GETUP1;
					}

					//now specify the torso anim
					seperateOnTorso = qtrue;
					desiredOnTorso = BOTH_FORCEPUSH;
				}
				else if (pm->ps->forceDodgeAnim == 2)
				{
					desiredAnim = BOTH_FORCE_GETUP_B1;
				}
				else if (pm->ps->forceDodgeAnim == 3)
				{
					desiredAnim = BOTH_FORCE_GETUP_B3;
				}
				else
				{
					desiredAnim = BOTH_GETUP1;
				}
			}
			else
			{
				desiredAnim = BOTH_KNOCKDOWN1;
			}
			break;
		case HANDEXTEND_DUELCHALLENGE:
			desiredAnim = BOTH_ENGAGETAUNT;
			break;
		case HANDEXTEND_TAUNT:
			desiredAnim = pm->ps->forceDodgeAnim;
			if (desiredAnim != BOTH_ENGAGETAUNT
				&& VectorCompare(pm->ps->velocity, vec3_origin)
				&& pm->ps->groundEntityNum != ENTITYNUM_NONE)
			{
				playFullBody = qtrue;
			}
			break;
		case HANDEXTEND_PRETHROW:
			desiredAnim = BOTH_A3_TL_BR;
			playFullBody = qtrue;
			break;
		case HANDEXTEND_POSTTHROW:
			desiredAnim = BOTH_D3_TL___;
			playFullBody = qtrue;
			break;
		case HANDEXTEND_PRETHROWN:
			desiredAnim = BOTH_KNEES1;
			playFullBody = qtrue;
			break;
		case HANDEXTEND_POSTTHROWN:
			if (pm->ps->forceDodgeAnim)
			{
				desiredAnim = BOTH_FORCE_GETUP_F2;
			}
			else
			{
				desiredAnim = BOTH_KNOCKDOWN5;
			}
			playFullBody = qtrue;
			break;
		case HANDEXTEND_DRAGGING:
			desiredAnim = BOTH_B1_BL___;
			break;
		case HANDEXTEND_JEDITAUNT:
			desiredAnim = BOTH_GESTURE1;
			break;
		default:
			desiredAnim = BOTH_FORCEPUSH;
			break;
		}

		if (!seperateOnTorso)
		{ //of seperateOnTorso, handle it after setting the legs
			PM_SetAnim(SETANIM_TORSO, desiredAnim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			pm->ps->torsoTimer = 1;
		}

		if (playFullBody)
		{ //sorry if all these exceptions are getting confusing. This one just means play on both legs and torso.
			PM_SetAnim(SETANIM_BOTH, desiredAnim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			pm->ps->legsTimer = pm->ps->torsoTimer = 1;
		}
		else if (pm->ps->forceHandExtend == HANDEXTEND_DODGE || pm->ps->forceHandExtend == HANDEXTEND_KNOCKDOWN ||
			(pm->ps->forceHandExtend == HANDEXTEND_CHOKE && pm->ps->groundEntityNum == ENTITYNUM_NONE))
		{ //special case, play dodge anim on whole body, choke anim too if off ground
			if (seperateOnTorso)
			{
				PM_SetAnim(SETANIM_LEGS, desiredAnim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				pm->ps->legsTimer = 1;

				PM_SetAnim(SETANIM_TORSO, desiredOnTorso, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				pm->ps->torsoTimer = 1;
			}
			else
			{
				PM_SetAnim(SETANIM_LEGS, desiredAnim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				pm->ps->legsTimer = 1;
			}
		}

		return qfalse;
	}
	return qtrue;
}

qboolean BG_KnockdownAnim(int anim)
{
	switch (anim)
	{
	case BOTH_KNOCKDOWN1:
	case BOTH_KNOCKDOWN2:
	case BOTH_KNOCKDOWN3:
	case BOTH_KNOCKDOWN4:
	case BOTH_KNOCKDOWN5:
	case BOTH_GETUP1:
	case BOTH_GETUP2:
	case BOTH_GETUP3:
	case BOTH_GETUP4:
	case BOTH_GETUP5:
	case BOTH_FORCE_GETUP_F1:
	case BOTH_FORCE_GETUP_F2:
	case BOTH_FORCE_GETUP_B1:
	case BOTH_FORCE_GETUP_B2:
	case BOTH_FORCE_GETUP_B3:
	case BOTH_FORCE_GETUP_B4:
	case BOTH_FORCE_GETUP_B5:
	case BOTH_GETUP_BROLL_B:
	case BOTH_GETUP_BROLL_F:
	case BOTH_GETUP_BROLL_L:
	case BOTH_GETUP_BROLL_R:
	case BOTH_GETUP_FROLL_B:
	case BOTH_GETUP_FROLL_F:
	case BOTH_GETUP_FROLL_L:
	case BOTH_GETUP_FROLL_R:
		return qtrue;
	}
	return qfalse;
}

qboolean BG_InRollES(entityState_t *ps, int anim)
{
	switch ((anim))
	{
	case BOTH_ROLL_F:
	case BOTH_ROLL_B:
	case BOTH_ROLL_R:
	case BOTH_ROLL_L:
		return qtrue;
	}
	return qfalse;
}


void BG_IK_MoveArm(void *ghoul2, int lHandBolt, int time, entityState_t *ent, int basePose, vec3_t desiredPos, qboolean *ikInProgress,
	vec3_t origin, vec3_t angles, vec3_t scale, int blendTime, qboolean forceHalt)
{
	mdxaBone_t lHandMatrix;
	vec3_t lHand;
	vec3_t torg;
	float distToDest;

	if (!ghoul2)
	{
		return;
	}

	assert(bgHumanoidAnimations[basePose].firstFrame > 0);

	if (!*ikInProgress && !forceHalt)
	{
		int baseposeAnim = basePose;
		sharedSetBoneIKStateParams_t ikP;

		//restrict the shoulder joint
		//VectorSet(ikP.pcjMins,-50.0f,-80.0f,-15.0f);
		//VectorSet(ikP.pcjMaxs,15.0f,40.0f,15.0f);

		//for now, leaving it unrestricted, but restricting elbow joint.
		//This lets us break the arm however we want in order to fling people
		//in throws, and doesn't look bad.
		VectorSet(ikP.pcjMins, 0, 0, 0);
		VectorSet(ikP.pcjMaxs, 0, 0, 0);

		//give the info on our entity.
		ikP.blendTime = blendTime;
		VectorCopy(origin, ikP.origin);
		VectorCopy(angles, ikP.angles);
		ikP.angles[PITCH] = 0;
		ikP.pcjOverrides = 0;
		ikP.radius = 10.0f;
		VectorCopy(scale, ikP.scale);

		//base pose frames for the limb
		ikP.startFrame = bgHumanoidAnimations[baseposeAnim].firstFrame + bgHumanoidAnimations[baseposeAnim].numFrames;
		ikP.endFrame = bgHumanoidAnimations[baseposeAnim].firstFrame + bgHumanoidAnimations[baseposeAnim].numFrames;

		ikP.forceAnimOnBone = qfalse; //let it use existing anim if it's the same as this one.

		//we want to call with a null bone name first. This will init all of the
		//ik system stuff on the g2 instance, because we need ragdoll effectors
		//in order for our pcj's to know how to angle properly.
		if (!trap->G2API_SetBoneIKState(ghoul2, time, NULL, IKS_DYNAMIC, &ikP))
		{
			assert(!"Failed to init IK system for g2 instance!");
		}

		//Now, create our IK bone state.
		if (trap->G2API_SetBoneIKState(ghoul2, time, "lhumerus", IKS_DYNAMIC, &ikP))
		{
			//restrict the elbow joint
			VectorSet(ikP.pcjMins, -90.0f, -20.0f, -20.0f);
			VectorSet(ikP.pcjMaxs, 30.0f, 20.0f, -20.0f);

			if (trap->G2API_SetBoneIKState(ghoul2, time, "lradius", IKS_DYNAMIC, &ikP))
			{ //everything went alright.
				*ikInProgress = qtrue;
			}
		}
	}

	if (*ikInProgress && !forceHalt)
	{ //actively update our ik state.
		sharedIKMoveParams_t ikM;
		sharedRagDollUpdateParams_t tuParms;
		vec3_t tAngles;

		//set the argument struct up
		VectorCopy(desiredPos, ikM.desiredOrigin); //we want the bone to move here.. if possible

		VectorCopy(angles, tAngles);
		tAngles[PITCH] = tAngles[ROLL] = 0;

		trap->G2API_GetBoltMatrix(ghoul2, 0, lHandBolt, &lHandMatrix, tAngles, origin, time, 0, scale);
		//Get the point position from the matrix.
		lHand[0] = lHandMatrix.matrix[0][3];
		lHand[1] = lHandMatrix.matrix[1][3];
		lHand[2] = lHandMatrix.matrix[2][3];

		VectorSubtract(lHand, desiredPos, torg);
		distToDest = VectorLength(torg);

		//closer we are, more we want to keep updated.
		//if we're far away we don't want to be too fast or we'll start twitching all over.
		if (distToDest < 2)
		{ //however if we're this close we want very precise movement
			ikM.movementSpeed = 0.4f;
		}
		else if (distToDest < 16)
		{
			ikM.movementSpeed = 0.9f;//8.0f;
		}
		else if (distToDest < 32)
		{
			ikM.movementSpeed = 0.8f;//4.0f;
		}
		else if (distToDest < 64)
		{
			ikM.movementSpeed = 0.7f;//2.0f;
		}
		else
		{
			ikM.movementSpeed = 0.6f;
		}
		VectorCopy(origin, ikM.origin); //our position in the world.

		ikM.boneName[0] = 0;
		if (trap->G2API_IKMove(ghoul2, time, &ikM))
		{
			//now do the standard model animate stuff with ragdoll update params.
			VectorCopy(angles, tuParms.angles);
			tuParms.angles[PITCH] = 0;

			VectorCopy(origin, tuParms.position);
			VectorCopy(scale, tuParms.scale);

			tuParms.me = ent->number;
			VectorClear(tuParms.velocity);

			trap->G2API_AnimateG2Models(ghoul2, time, &tuParms);
		}
		else
		{
			*ikInProgress = qfalse;
		}
	}
	else if (*ikInProgress)
	{ //kill it
		float cFrame, animSpeed;
		int sFrame, eFrame, flags;

		trap->G2API_SetBoneIKState(ghoul2, time, "lhumerus", IKS_NONE, NULL);
		trap->G2API_SetBoneIKState(ghoul2, time, "lradius", IKS_NONE, NULL);

		//then reset the angles/anims on these PCJs
		trap->G2API_SetBoneAngles(ghoul2, 0, "lhumerus", vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, NULL, 0, time);
		trap->G2API_SetBoneAngles(ghoul2, 0, "lradius", vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, NULL, 0, time);

		//Get the anim/frames that the pelvis is on exactly, and match the left arm back up with them again.
		trap->G2API_GetBoneAnim(ghoul2, "pelvis", (const int)time, &cFrame, &sFrame, &eFrame, &flags, &animSpeed, 0, 0);
		trap->G2API_SetBoneAnim(ghoul2, 0, "lhumerus", sFrame, eFrame, flags, animSpeed, time, sFrame, 300);
		trap->G2API_SetBoneAnim(ghoul2, 0, "lradius", sFrame, eFrame, flags, animSpeed, time, sFrame, 300);

		//And finally, get rid of all the ik state effector data by calling with null bone name (similar to how we init it).
		trap->G2API_SetBoneIKState(ghoul2, time, NULL, IKS_NONE, NULL);

		*ikInProgress = qfalse;
	}
}


//Adjust the head/neck desired angles
void BG_UpdateLookAngles(int lookingDebounceTime, vec3_t lastHeadAngles, int time, vec3_t lookAngles, float lookSpeed, float minPitch, float maxPitch, float minYaw, float maxYaw, float minRoll, float maxRoll)
{
	static const float fFrameInter = 0.1f;
	static vec3_t oldLookAngles;
	static vec3_t lookAnglesDiff;
	static int ang;

	if (lookingDebounceTime > time)
	{
		//clamp so don't get "Exorcist" effect
		if (lookAngles[PITCH] > maxPitch)
		{
			lookAngles[PITCH] = maxPitch;
		}
		else if (lookAngles[PITCH] < minPitch)
		{
			lookAngles[PITCH] = minPitch;
		}
		if (lookAngles[YAW] > maxYaw)
		{
			lookAngles[YAW] = maxYaw;
		}
		else if (lookAngles[YAW] < minYaw)
		{
			lookAngles[YAW] = minYaw;
		}
		if (lookAngles[ROLL] > maxRoll)
		{
			lookAngles[ROLL] = maxRoll;
		}
		else if (lookAngles[ROLL] < minRoll)
		{
			lookAngles[ROLL] = minRoll;
		}

		//slowly lerp to this new value
		//Remember last headAngles
		VectorCopy(lastHeadAngles, oldLookAngles);
		VectorSubtract(lookAngles, oldLookAngles, lookAnglesDiff);

		for (ang = 0; ang < 3; ang++)
		{
			lookAnglesDiff[ang] = AngleNormalize180(lookAnglesDiff[ang]);
		}

		if (VectorLengthSquared(lookAnglesDiff))
		{
			lookAngles[PITCH] = AngleNormalize180(oldLookAngles[PITCH] + (lookAnglesDiff[PITCH] * fFrameInter*lookSpeed));
			lookAngles[YAW] = AngleNormalize180(oldLookAngles[YAW] + (lookAnglesDiff[YAW] * fFrameInter*lookSpeed));
			lookAngles[ROLL] = AngleNormalize180(oldLookAngles[ROLL] + (lookAnglesDiff[ROLL] * fFrameInter*lookSpeed));
		}
	}
	//Remember current lookAngles next time
	VectorCopy(lookAngles, lastHeadAngles);
}

//for setting visual look (headturn) angles
static void BG_G2ClientNeckAngles(void *ghoul2, int time, const vec3_t lookAngles, vec3_t headAngles, vec3_t neckAngles, vec3_t thoracicAngles, vec3_t headClampMinAngles, vec3_t headClampMaxAngles)
{
	vec3_t	lA;
	VectorCopy(lookAngles, lA);
	//clamp the headangles (which should now be relative to the cervical (neck) angles
	if (lA[PITCH] < headClampMinAngles[PITCH])
	{
		lA[PITCH] = headClampMinAngles[PITCH];
	}
	else if (lA[PITCH] > headClampMaxAngles[PITCH])
	{
		lA[PITCH] = headClampMaxAngles[PITCH];
	}

	if (lA[YAW] < headClampMinAngles[YAW])
	{
		lA[YAW] = headClampMinAngles[YAW];
	}
	else if (lA[YAW] > headClampMaxAngles[YAW])
	{
		lA[YAW] = headClampMaxAngles[YAW];
	}

	if (lA[ROLL] < headClampMinAngles[ROLL])
	{
		lA[ROLL] = headClampMinAngles[ROLL];
	}
	else if (lA[ROLL] > headClampMaxAngles[ROLL])
	{
		lA[ROLL] = headClampMaxAngles[ROLL];
	}

	//split it up between the neck and cranium
	if (thoracicAngles[PITCH])
	{//already been set above, blend them
		thoracicAngles[PITCH] = (thoracicAngles[PITCH] + (lA[PITCH] * 0.4f)) * 0.5f;
	}
	else
	{
		thoracicAngles[PITCH] = lA[PITCH] * 0.4f;
	}
	if (thoracicAngles[YAW])
	{//already been set above, blend them
		thoracicAngles[YAW] = (thoracicAngles[YAW] + (lA[YAW] * 0.1f)) * 0.5f;
	}
	else
	{
		thoracicAngles[YAW] = lA[YAW] * 0.1f;
	}
	if (thoracicAngles[ROLL])
	{//already been set above, blend them
		thoracicAngles[ROLL] = (thoracicAngles[ROLL] + (lA[ROLL] * 0.1f)) * 0.5f;
	}
	else
	{
		thoracicAngles[ROLL] = lA[ROLL] * 0.1f;
	}

	neckAngles[PITCH] = lA[PITCH] * 0.2f;
	neckAngles[YAW] = lA[YAW] * 0.3f;
	neckAngles[ROLL] = lA[ROLL] * 0.3f;

	headAngles[PITCH] = lA[PITCH] * 0.4f;
	headAngles[YAW] = lA[YAW] * 0.6f;
	headAngles[ROLL] = lA[ROLL] * 0.6f;

	trap->G2API_SetBoneAngles(ghoul2, 0, "cranium", headAngles, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, 0, 0, time);
	trap->G2API_SetBoneAngles(ghoul2, 0, "cervical", neckAngles, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, 0, 0, time);
	trap->G2API_SetBoneAngles(ghoul2, 0, "thoracic", thoracicAngles, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, 0, 0, time);

	for (int i = 0; i < MAX_ARMOR; i++)
	{
		trap->G2API_SetBoneAngles(ghoul2, 4 + i, "cranium", headAngles, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, 0, 0, time);
		trap->G2API_SetBoneAngles(ghoul2, 4 + i, "cervical", neckAngles, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, 0, 0, time);
		trap->G2API_SetBoneAngles(ghoul2, 4 + i, "thoracic", thoracicAngles, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, 0, 0, time);
	}
}

//rww - Finally decided to convert all this stuff to BG form.
static void BG_G2ClientSpineAngles(void *ghoul2, int motionBolt, vec3_t cent_lerpOrigin, vec3_t cent_lerpAngles, entityState_t *cent,
	int time, vec3_t viewAngles, int ciLegs, int ciTorso, const vec3_t angles, vec3_t thoracicAngles,
	vec3_t ulAngles, vec3_t llAngles, vec3_t modelScale, float *tPitchAngle, float *tYawAngle, int *corrTime)
{
	qboolean doCorr = qfalse;

	//*tPitchAngle = viewAngles[PITCH];
	viewAngles[YAW] = AngleDelta(cent_lerpAngles[YAW], angles[YAW]);
	//*tYawAngle = viewAngles[YAW];

	if (!BG_FlippingAnim(cent->legsAnim) &&
		!BG_SpinningSaberAnim(cent->legsAnim) &&
		!BG_SpinningSaberAnim(cent->torsoAnim) &&
		!BG_InSpecialJump(cent->legsAnim) &&
		!BG_InSpecialJump(cent->torsoAnim) &&
		!BG_InDeathAnim(cent->legsAnim) &&
		!BG_InDeathAnim(cent->torsoAnim) &&
		!BG_InRollES(cent, cent->legsAnim) &&
		!BG_RollingAnim(cent->legsAnim) &&
		!BG_SaberInSpecial(cent->saberMove) &&
		!BG_SaberInSpecialAttack(cent->torsoAnim) &&
		!BG_SaberInSpecialAttack(cent->legsAnim) &&

		!BG_KnockdownAnim(cent->torsoAnim) &&
		!BG_KnockdownAnim(cent->legsAnim) &&
		!BG_KnockdownAnim(ciTorso) &&
		!BG_KnockdownAnim(ciLegs) &&

		!BG_FlippingAnim(ciLegs) &&
		!BG_SpinningSaberAnim(ciLegs) &&
		!BG_SpinningSaberAnim(ciTorso) &&
		!BG_InSpecialJump(ciLegs) &&
		!BG_InSpecialJump(ciTorso) &&
		!BG_InDeathAnim(ciLegs) &&
		!BG_InDeathAnim(ciTorso) &&
		!BG_SaberInSpecialAttack(ciTorso) &&
		!BG_SaberInSpecialAttack(ciLegs) &&

		!(cent->eFlags & EF_DEAD) &&
		(cent->legsAnim) != (cent->torsoAnim) &&
		(ciLegs) != (ciTorso))
	{
		doCorr = qtrue;
	}

	if (doCorr)
	{//FIXME: no need to do this if legs and torso on are same frame
		//adjust for motion offset
		mdxaBone_t	boltMatrix;
		vec3_t		motionFwd, motionAngles;
		vec3_t		motionRt, tempAng;
		int			ang;

		trap->G2API_GetBoltMatrix_NoRecNoRot(ghoul2, 0, motionBolt, &boltMatrix, vec3_origin, cent_lerpOrigin, time, 0, modelScale);
		//BG_GiveMeVectorFromMatrix( &boltMatrix, NEGATIVE_Y, motionFwd );
		motionFwd[0] = -boltMatrix.matrix[0][1];
		motionFwd[1] = -boltMatrix.matrix[1][1];
		motionFwd[2] = -boltMatrix.matrix[2][1];

		vectoangles(motionFwd, motionAngles);

		//BG_GiveMeVectorFromMatrix( &boltMatrix, NEGATIVE_X, motionRt );
		motionRt[0] = -boltMatrix.matrix[0][0];
		motionRt[1] = -boltMatrix.matrix[1][0];
		motionRt[2] = -boltMatrix.matrix[2][0];

		vectoangles(motionRt, tempAng);
		motionAngles[ROLL] = -tempAng[PITCH];

		for (ang = 0; ang < 3; ang++)
		{
			viewAngles[ang] = AngleNormalize180(viewAngles[ang] - AngleNormalize180(motionAngles[ang]));
		}
	}

	//distribute the angles differently up the spine
	//NOTE: each of these distributions must add up to 1.0f
	thoracicAngles[PITCH] = viewAngles[PITCH] * 0.20f;
	llAngles[PITCH] = viewAngles[PITCH] * 0.40f;
	ulAngles[PITCH] = viewAngles[PITCH] * 0.40f;

	thoracicAngles[YAW] = viewAngles[YAW] * 0.20f;
	ulAngles[YAW] = viewAngles[YAW] * 0.35f;
	llAngles[YAW] = viewAngles[YAW] * 0.45f;

	thoracicAngles[ROLL] = viewAngles[ROLL] * 0.20f;
	ulAngles[ROLL] = viewAngles[ROLL] * 0.35f;
	llAngles[ROLL] = viewAngles[ROLL] * 0.45f;
}


//I apologize for this function
qboolean BG_InRoll2(entityState_t *es)
{
	switch ((es->legsAnim))
	{
	case BOTH_GETUP_BROLL_B:
	case BOTH_GETUP_BROLL_F:
	case BOTH_GETUP_BROLL_L:
	case BOTH_GETUP_BROLL_R:
	case BOTH_GETUP_FROLL_B:
	case BOTH_GETUP_FROLL_F:
	case BOTH_GETUP_FROLL_L:
	case BOTH_GETUP_FROLL_R:
	case BOTH_ROLL_F:
	case BOTH_ROLL_B:
	case BOTH_ROLL_R:
	case BOTH_ROLL_L:
		return qtrue;
	}
	return qfalse;
}


/*
==================
BG_SwingAngles
==================
*/
float BG_SwingAngles(float destination, float swingTolerance, float clampTolerance,
	float speed, float *angle, qboolean *swinging, int frametime) {
	float	swing;
	float	move;
	float	scale;

	if (!*swinging) {
		// see if a swing should be started
		swing = AngleSubtract(*angle, destination);
		if (swing > swingTolerance || swing < -swingTolerance) {
			*swinging = qtrue;
		}
	}

	if (!*swinging) {
		return 0;
	}

	// modify the speed depending on the delta
	// so it doesn't seem so linear
	swing = AngleSubtract(destination, *angle);
	scale = fabs(swing);
	if (scale < swingTolerance * 0.5f) {
		scale = 0.5f;
	}
	else if (scale < swingTolerance) {
		scale = 1.0f;
	}
	else {
		scale = 2.0f;
	}

	// swing towards the destination angle
	if (swing >= 0) {
		move = frametime * scale * speed;
		if (move >= swing) {
			move = swing;
			*swinging = qfalse;
		}
		*angle = AngleMod(*angle + move);
	}
	else if (swing < 0) {
		move = frametime * scale * -speed;
		if (move <= swing) {
			move = swing;
			*swinging = qfalse;
		}
		*angle = AngleMod(*angle + move);
	}

	// clamp to no more than tolerance
	swing = AngleSubtract(destination, *angle);
	if (swing > clampTolerance) {
		*angle = AngleMod(destination - (clampTolerance - 1));
	}
	else if (swing < -clampTolerance) {
		*angle = AngleMod(destination + (clampTolerance - 1));
	}

	return swing;
}

void BG_G2PlayerAngles(void *ghoul2, int motionBolt, entityState_t *cent, int time, vec3_t cent_lerpOrigin,
	vec3_t cent_lerpAngles, vec3_t legs[3], vec3_t legsAngles, qboolean *tYawing,
	qboolean *tPitching, qboolean *lYawing, float *tYawAngle, float *tPitchAngle,
	float *lYawAngle, int frametime, vec3_t turAngles, vec3_t modelScale, int ciLegs,
	int ciTorso, int *corrTime, vec3_t lookAngles, vec3_t lastHeadAngles, int lookTime,
	entityState_t *emplaced, int *crazySmoothFactor, int saberAnimFlags)
{
	int					adddir = 0;
	static int			dir;
	static int			i;
	//static int			movementOffsets[8] = { 0, 22, 45, -22, 0, 22, -45, -22 };
	float				degrees_negative = 0;
	float				degrees_positive = 0;
	static float		dif;
	static float		dest;
	static float		speed; //, speed_dif, speed_desired;
	static const float	lookSpeed = 1.5f;
#ifdef BONE_BASED_LEG_ANGLES
	static float		legBoneYaw;
#endif
	static vec3_t		eyeAngles;
	static vec3_t		neckAngles;
	static vec3_t		velocity;
	static vec3_t		torsoAngles, headAngles;
	static vec3_t		velPos, velAng;
	static vec3_t		ulAngles, llAngles, viewAngles, angles, thoracicAngles = { 0, 0, 0 };
	static vec3_t		headClampMinAngles = { -25, -55, -10 }, headClampMaxAngles = { 50, 50, 10 };

	if (cent->forceFrame || BG_SaberLockBreakAnim(cent->legsAnim) || BG_SaberLockBreakAnim(cent->torsoAnim))
	{ //a vehicle or riding a vehicle - in either case we don't need to be in here
		vec3_t forcedAngles;

		VectorClear(forcedAngles);
		forcedAngles[YAW] = cent_lerpAngles[YAW];
		forcedAngles[ROLL] = cent_lerpAngles[ROLL];
		AnglesToAxis(forcedAngles, legs);
		VectorCopy(forcedAngles, legsAngles);
		//JAC: turAngles should be updated as well.
		VectorCopy(legsAngles, turAngles);

		if (cent->number < MAX_CLIENTS)
		{
			trap->G2API_SetBoneAngles(ghoul2, 0, "lower_lumbar", vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, 0, 0, time);
			trap->G2API_SetBoneAngles(ghoul2, 0, "upper_lumbar", vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, 0, 0, time);
			trap->G2API_SetBoneAngles(ghoul2, 0, "cranium", vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, 0, 0, time);
			trap->G2API_SetBoneAngles(ghoul2, 0, "thoracic", vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, 0, 0, time);
			trap->G2API_SetBoneAngles(ghoul2, 0, "cervical", vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, 0, 0, time);

			for (i = 0; i < MAX_ARMOR; i++) {
				trap->G2API_SetBoneAngles(ghoul2, 4 + i, "lower_lumbar", vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, 0, 0, time);
				trap->G2API_SetBoneAngles(ghoul2, 4 + i, "upper_lumbar", vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, 0, 0, time);
				trap->G2API_SetBoneAngles(ghoul2, 4 + i, "cranium", vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, 0, 0, time);
				trap->G2API_SetBoneAngles(ghoul2, 4 + i, "thoracic", vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, 0, 0, time);
				trap->G2API_SetBoneAngles(ghoul2, 4 + i, "cervical", vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, 0, 0, time);
			}
		}
		return;
	}

	if ((time + 2000) < *corrTime)
	{
		*corrTime = 0;
	}

	VectorCopy(cent_lerpAngles, headAngles);
	headAngles[YAW] = AngleMod(headAngles[YAW]);
	VectorClear(legsAngles);
	VectorClear(torsoAngles);
	// --------- yaw -------------

	// allow yaw to drift a bit
	if (((cent->legsAnim) != BOTH_STAND1) ||
		(cent->torsoAnim) != GetWeaponData(cent->weapon, cent->weaponVariation)->anims.ready.torsoAnim)
	{
		// if not standing still, always point all in the same direction
		//cent->pe.torso.yawing = qtrue;	// always center
		*tYawing = qtrue;
		//cent->pe.torso.pitching = qtrue;	// always center
		*tPitching = qtrue;
		//cent->pe.legs.yawing = qtrue;	// always center
		*lYawing = qtrue;
	}

	// adjust legs for movement dir
	if (cent->eFlags & EF_DEAD) {
		// don't let dead bodies twitch
		dir = 0;
	}
	else {
		dir = cent->angles2[YAW];
		if (dir < 0 || dir > 7) {
			Com_Error(ERR_DROP, "Bad player movement angle (%i)", dir);
		}
	}

	torsoAngles[YAW] = headAngles[YAW];

	//for now, turn torso instantly and let the legs swing to follow
	*tYawAngle = torsoAngles[YAW];

	// --------- pitch -------------

	VectorCopy(cent->pos.trDelta, velocity);

	if (BG_InRoll2(cent))
	{ //don't affect angles based on vel then
		VectorClear(velocity);
	}
	else if (cent->weapon == WP_SABER &&
		BG_SaberInSpecial(cent->saberMove))
	{
		VectorClear(velocity);
	}

	speed = VectorNormalize(velocity);

	if (!speed)
	{
		torsoAngles[YAW] = headAngles[YAW];
	}

	// only show a fraction of the pitch angle in the torso
	if (headAngles[PITCH] > 180) {
		dest = (-360 + headAngles[PITCH]) * 0.75f;
	}
	else {
		dest = headAngles[PITCH] * 0.75f;
	}

	BG_SwingAngles(dest, 15.0f, 30.0f, 0.1f, tPitchAngle, tPitching, frametime);
	torsoAngles[PITCH] = *tPitchAngle;

	// --------- roll -------------

	if (speed) {
		vec3_t	axis[3];
		float	side;

		speed *= 0.05f;

		AnglesToAxis(legsAngles, axis);
		side = speed * DotProduct(velocity, axis[1]);
		legsAngles[ROLL] -= side;

		side = speed * DotProduct(velocity, axis[0]);
		legsAngles[PITCH] += side;
	}

	//legsAngles[YAW] = headAngles[YAW] + (movementOffsets[ dir ]*speed_dif);

	//rww - crazy velocity-based leg angle calculation
	legsAngles[YAW] = headAngles[YAW];
	velPos[0] = cent_lerpOrigin[0] + velocity[0];
	velPos[1] = cent_lerpOrigin[1] + velocity[1];
	velPos[2] = cent_lerpOrigin[2];// + velocity[2];

	if (cent->groundEntityNum == ENTITYNUM_NONE ||
		cent->forceFrame ||
		(cent->weapon == WP_EMPLACED_GUN && emplaced))
	{ //off the ground, no direction-based leg angles (same if in saberlock)
		VectorCopy(cent_lerpOrigin, velPos);
	}

	VectorSubtract(cent_lerpOrigin, velPos, velAng);

	if (!VectorCompare(velAng, vec3_origin))
	{
		vectoangles(velAng, velAng);

		if (velAng[YAW] <= legsAngles[YAW])
		{
			degrees_negative = (legsAngles[YAW] - velAng[YAW]);
			degrees_positive = (360 - legsAngles[YAW]) + velAng[YAW];
		}
		else
		{
			degrees_negative = legsAngles[YAW] + (360 - velAng[YAW]);
			degrees_positive = (velAng[YAW] - legsAngles[YAW]);
		}

		if (degrees_negative < degrees_positive)
		{
			dif = degrees_negative;
			adddir = 0;
		}
		else
		{
			dif = degrees_positive;
			adddir = 1;
		}

		if (dif > 90)
		{
			dif = (180 - dif);
		}

		if (dif > 60)
		{
			dif = 60;
		}

		//Slight hack for when playing is running backward
		if (dir == 3 || dir == 5)
		{
			dif = -dif;
		}

		if (adddir)
		{
			legsAngles[YAW] -= dif;
		}
		else
		{
			legsAngles[YAW] += dif;
		}
	}

	BG_SwingAngles(legsAngles[YAW], /*40*/0.0f, 90.0f, 0.65f, lYawAngle, lYawing, frametime);
	legsAngles[YAW] = *lYawAngle;

	/*
	// pain twitch
	CG_AddPainTwitch( cent, torsoAngles );
	*/

	legsAngles[ROLL] = 0;
	torsoAngles[ROLL] = 0;

	//	VectorCopy(legsAngles, turAngles);

	// pull the angles back out of the hierarchial chain
	AnglesSubtract(headAngles, torsoAngles, headAngles);
	AnglesSubtract(torsoAngles, legsAngles, torsoAngles);

	legsAngles[PITCH] = 0;

	if (cent->heldByClient)
	{ //keep the base angles clear when doing the IK stuff, it doesn't compensate for it.
		//rwwFIXMEFIXME: Store leg angles off and add them to all the fed in angles for G2 functions?
		VectorClear(legsAngles);
		legsAngles[YAW] = cent_lerpAngles[YAW];
	}

#ifdef BONE_BASED_LEG_ANGLES
	legBoneYaw = legsAngles[YAW];
	VectorClear(legsAngles);
	legsAngles[YAW] = cent_lerpAngles[YAW];
#endif

	VectorCopy(legsAngles, turAngles);

	AnglesToAxis(legsAngles, legs);

	VectorCopy(cent_lerpAngles, viewAngles);
	viewAngles[YAW] = viewAngles[ROLL] = 0;
	viewAngles[PITCH] *= 0.5f;

	VectorSet(angles, 0, legsAngles[1], 0);

	angles[0] = legsAngles[0];
	if (angles[0] > 30)
	{
		angles[0] = 30;
	}
	else if (angles[0] < -30)
	{
		angles[0] = -30;
	}

	if (cent->weapon == WP_EMPLACED_GUN &&
		emplaced)
	{ //if using an emplaced gun, then we want to make sure we're angled to "hold" it right
		vec3_t facingAngles;
		vec3_t savedAngles;

		VectorSubtract(emplaced->pos.trBase, cent_lerpOrigin, facingAngles);
		vectoangles(facingAngles, facingAngles);

		if (emplaced->weapon == WP_NONE)
		{ //e-web
			VectorCopy(facingAngles, legsAngles);
			AnglesToAxis(legsAngles, legs);
		}
		else
		{ //misc emplaced
			float dif = AngleSubtract(cent_lerpAngles[YAW], facingAngles[YAW]);

			VectorSet(facingAngles, -16.0f, -dif, 0.0f);

			if (cent->legsAnim == BOTH_STRAFE_LEFT1 || cent->legsAnim == BOTH_STRAFE_RIGHT1)
			{ //try to adjust so it doesn't look wrong
				if (crazySmoothFactor)
				{ //want to smooth a lot during this because it chops around and looks like ass
					*crazySmoothFactor = time + 1000;
				}

				BG_G2ClientSpineAngles(ghoul2, motionBolt, cent_lerpOrigin, cent_lerpAngles, cent, time,
					viewAngles, ciLegs, ciTorso, angles, thoracicAngles, ulAngles, llAngles, modelScale,
					tPitchAngle, tYawAngle, corrTime);
				trap->G2API_SetBoneAngles(ghoul2, 0, "lower_lumbar", llAngles, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, 0, 0, time);
				trap->G2API_SetBoneAngles(ghoul2, 0, "upper_lumbar", ulAngles, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, 0, 0, time);
				trap->G2API_SetBoneAngles(ghoul2, 0, "cranium", vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, 0, 0, time);

				for (i = 0; i < MAX_ARMOR; i++) {
					trap->G2API_SetBoneAngles(ghoul2, 4 + i, "lower_lumbar", llAngles, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, 0, 0, time);
					trap->G2API_SetBoneAngles(ghoul2, 4 + i, "upper_lumbar", ulAngles, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, 0, 0, time);
					trap->G2API_SetBoneAngles(ghoul2, 4 + i, "cranium", vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, 0, 0, time);
				}

				VectorAdd(facingAngles, thoracicAngles, facingAngles);

				if (cent->legsAnim == BOTH_STRAFE_LEFT1)
				{ //this one needs some further correction
					facingAngles[YAW] -= 32.0f;
				}
			}
			else
			{
				trap->G2API_SetBoneAngles(ghoul2, 0, "cranium", vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, 0, 0, time);
				for (i = 0; i < MAX_ARMOR; i++) {
					trap->G2API_SetBoneAngles(ghoul2, 4 + i, "cranium", vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, 0, 0, time);
				}
			}

			VectorCopy(facingAngles, savedAngles);
			VectorScale(facingAngles, 0.6f, facingAngles);
			trap->G2API_SetBoneAngles(ghoul2, 0, "lower_lumbar", vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, 0, 0, time);
			for (i = 0; i < MAX_ARMOR; i++)
				trap->G2API_SetBoneAngles(ghoul2, 4 + i, "lower_lumbar", vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, 0, 0, time);
			VectorScale(facingAngles, 0.8f, facingAngles);
			trap->G2API_SetBoneAngles(ghoul2, 0, "upper_lumbar", facingAngles, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, 0, 0, time);
			for (i = 0; i < MAX_ARMOR; i++)
				trap->G2API_SetBoneAngles(ghoul2, 4 + i, "upper_lumbar", facingAngles, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, 0, 0, time);
			VectorScale(facingAngles, 0.8f, facingAngles);
			trap->G2API_SetBoneAngles(ghoul2, 0, "thoracic", facingAngles, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, 0, 0, time);
			for (i = 0; i < MAX_ARMOR; i++)
				trap->G2API_SetBoneAngles(ghoul2, 4 + i, "thoracic", facingAngles, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, 0, 0, time);
			//Now we want the head angled toward where we are facing
			VectorSet(facingAngles, 0.0f, dif, 0.0f);
			VectorScale(facingAngles, 0.6f, facingAngles);
			trap->G2API_SetBoneAngles(ghoul2, 0, "cervical", facingAngles, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, 0, 0, time);
			for (i = 0; i < MAX_ARMOR; i++)
				trap->G2API_SetBoneAngles(ghoul2, 4 + i, "cervical", facingAngles, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, 0, 0, time);

			return; //don't have to bother with the rest then
		}
	}

	BG_G2ClientSpineAngles(ghoul2, motionBolt, cent_lerpOrigin, cent_lerpAngles, cent, time,
		viewAngles, ciLegs, ciTorso, angles, thoracicAngles, ulAngles, llAngles, modelScale,
		tPitchAngle, tYawAngle, corrTime);

	VectorCopy(cent_lerpAngles, eyeAngles);

	for (i = 0; i < 3; i++)
	{
		lookAngles[i] = AngleNormalize180(lookAngles[i]);
		eyeAngles[i] = AngleNormalize180(eyeAngles[i]);
	}
	AnglesSubtract(lookAngles, eyeAngles, lookAngles);

	BG_UpdateLookAngles(lookTime, lastHeadAngles, time, lookAngles, lookSpeed, -50.0f, 50.0f, -70.0f, 70.0f, -30.0f, 30.0f);

	BG_G2ClientNeckAngles(ghoul2, time, lookAngles, headAngles, neckAngles, thoracicAngles, headClampMinAngles, headClampMaxAngles);

	trap->G2API_SetBoneAngles(ghoul2, 0, "lower_lumbar", llAngles, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, 0, 0, time);
	trap->G2API_SetBoneAngles(ghoul2, 0, "upper_lumbar", ulAngles, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, 0, 0, time);
	trap->G2API_SetBoneAngles(ghoul2, 0, "thoracic", thoracicAngles, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, 0, 0, time);
	for (i = 0; i < MAX_ARMOR; i++) {
		trap->G2API_SetBoneAngles(ghoul2, 4 + i, "lower_lumbar", llAngles, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, 0, 0, time);
		trap->G2API_SetBoneAngles(ghoul2, 4 + i, "upper_lumbar", ulAngles, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, 0, 0, time);
		trap->G2API_SetBoneAngles(ghoul2, 4 + i, "thoracic", thoracicAngles, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, 0, 0, time);
	}
}

void BG_G2ATSTAngles(void *ghoul2, int time, vec3_t cent_lerpAngles)
{//																							up			right		fwd
	trap->G2API_SetBoneAngles(ghoul2, 0, "thoracic", cent_lerpAngles, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, 0, 0, time);
}

/*
==============================================================================
END: Animation utility functions (sequence checking)
==============================================================================
*/

void BG_FlipPart(playerState_t *ps, int part)
{
	if (part == SETANIM_TORSO)
	{
		if (ps->torsoFlip)
		{
			ps->torsoFlip = qfalse;
		}
		else
		{
			ps->torsoFlip = qtrue;
		}
	}
	else if (part == SETANIM_LEGS)
	{
		if (ps->legsFlip)
		{
			ps->legsFlip = qfalse;
		}
		else
		{
			ps->legsFlip = qtrue;
		}
	}
}

qboolean	BGPAFtextLoaded = qfalse;
animation_t	bgHumanoidAnimations[MAX_TOTALANIMATIONS]; //humanoid animations are the only ones that are statically allocated.

bgLoadedAnim_t bgAllAnims[MAX_ANIM_FILES];
int bgNumAllAnims = NUM_RESERVED_ANIMSETS; //start off at 3, because 0 will always be assigned to humanoid, and 1 will always be rockettrooper, AND 2 IS FOR JKG MWAHAHAHAA

//ALWAYS call on game/cgame init
void BG_InitAnimsets(void)
{
	memset(&bgAllAnims, 0, sizeof(bgAllAnims));
	BGPAFtextLoaded = qfalse;	// VVFIXME - The PC doesn't seem to need this, but why?
}

//ALWAYS call on game/cgame shutdown
void BG_ClearAnimsets(void)
{
}

animation_t *BG_AnimsetAlloc(void)
{
	assert (bgNumAllAnims < MAX_ANIM_FILES);
	bgAllAnims[bgNumAllAnims].anims = (animation_t *) malloc(sizeof(animation_t)*MAX_TOTALANIMATIONS);

	return bgAllAnims[bgNumAllAnims].anims;
}

void BG_AnimsetFree(animation_t *animset)
{
}

#ifndef _GAME //none of this is actually needed serverside. Could just be moved to cgame code but it's here since it
			   //used to tie in a lot with the anim loading stuff.
stringID_table_t animEventTypeTable[MAX_ANIM_EVENTS+1] = 
{
	ENUM2STRING(AEV_SOUND),			//# animID AEV_SOUND framenum soundpath randomlow randomhi chancetoplay
	ENUM2STRING(AEV_FOOTSTEP),		//# animID AEV_FOOTSTEP framenum footstepType
	ENUM2STRING(AEV_EFFECT),		//# animID AEV_EFFECT framenum effectpath boltName
	ENUM2STRING(AEV_FIRE),			//# animID AEV_FIRE framenum altfire chancetofire
	ENUM2STRING(AEV_MOVE),			//# animID AEV_MOVE framenum forwardpush rightpush uppush
	ENUM2STRING(AEV_SOUNDCHAN),		//# animID AEV_SOUNDCHAN framenum CHANNEL soundpath randomlow randomhi chancetoplay 
	ENUM2STRING(AEV_SABER_SWING),	//# animID AEV_SABER_SWING framenum CHANNEL randomlow randomhi chancetoplay 
	ENUM2STRING(AEV_SABER_SPIN),	//# animID AEV_SABER_SPIN framenum CHANNEL chancetoplay 
	//must be terminated
	{ NULL,-1 }
};

stringID_table_t footstepTypeTable[NUM_FOOTSTEP_TYPES+1] = 
{
	ENUM2STRING(FOOTSTEP_R),
	ENUM2STRING(FOOTSTEP_L),
	ENUM2STRING(FOOTSTEP_HEAVY_R),
	ENUM2STRING(FOOTSTEP_HEAVY_L),
	//must be terminated
	{ NULL,-1 }
};

int CheckAnimFrameForEventType( animevent_t *animEvents, int keyFrame, animEventType_t eventType )
{
	int i;

	for ( i = 0; i < MAX_ANIM_EVENTS; i++ )
	{
		if ( animEvents[i].keyFrame == keyFrame )
		{//there is an animevent on this frame already
			if ( animEvents[i].eventType == eventType )
			{//and it is of the same type
				return i;
			}
		}
	}
	//nope
	return -1;
}

void ParseAnimationEvtBlock(const char *aeb_filename, animevent_t *animEvents, animation_t *animations, int *i,const char **text_p) 
{
	const char		*token;
	int				num, n, animNum, keyFrame, lowestVal, highestVal, curAnimEvent, lastAnimEvent = 0;
	animEventType_t	eventType;
	char			stringData[MAX_QPATH];

	// get past starting bracket
	while(1) 
	{
		token = COM_Parse( text_p );
		if ( !Q_stricmp( token, "{" ) ) 
		{
			break;
		}
	}

	assert (animations != NULL);

	//NOTE: instead of a blind increment, increase the index
	//			this way if we have an event on an anim that already
	//			has an event of that type, it stomps it

	// read information for each frame
	while ( 1 ) 
	{
		if ( lastAnimEvent >= MAX_ANIM_EVENTS )
		{
			Com_Error( ERR_DROP, "ParseAnimationEvtBlock: number events in animEvent file %s > MAX_ANIM_EVENTS(%i)", aeb_filename, MAX_ANIM_EVENTS );
			return;
		}
		// Get base frame of sequence
		token = COM_Parse( text_p );
		if ( !token || !token[0]) 
		{
			break;
		}

		if ( !Q_stricmp( token, "}" ) )		// At end of block 
		{
			break;
		}

		//Compare to same table as animations used 
		//	so we don't have to use actual numbers for animation first frames,
		//	just need offsets.
		//This way when animation numbers change, this table won't have to be updated,
		//	at least not much.
		animNum = GetIDForString(animTable, token);
		if(animNum == -1)
		{//Unrecognized ANIM ENUM name, or we're skipping this line, keep going till you get a good one
			Com_Printf(S_COLOR_YELLOW"WARNING: Unknown token %s in animEvent file %s\n", token, aeb_filename );
			while (token[0])
			{
				token = COM_ParseExt( text_p, qfalse );	//returns empty string when next token is EOL
			}
			continue;
		}

		if ( animations[animNum].numFrames == 0 )
		{//we don't use this anim
			Com_Printf(S_COLOR_YELLOW"WARNING: %s animevents.cfg: anim %s not used by this model\n", aeb_filename, token);
			//skip this entry
			SkipRestOfLine( text_p );
			continue;
		}

		token = COM_Parse( text_p );
		eventType = (animEventType_t)GetIDForString(animEventTypeTable, token);
		if ( eventType == AEV_NONE || eventType == (animEventType_t)-1 )
		{//Unrecognized ANIM EVENT TYOE, or we're skipping this line, keep going till you get a good one
			//Com_Printf(S_COLOR_YELLOW"WARNING: Unknown token %s in animEvent file %s\n", token, aeb_filename );
			continue;
		}

		//set our start frame
		keyFrame = animations[animNum].firstFrame;
		// Get offset to frame within sequence
		token = COM_Parse( text_p );
		if ( !token ) 
		{
			break;
		}
		keyFrame += atoi( token );

		//see if this frame already has an event of this type on it, if so, overwrite it
		curAnimEvent = CheckAnimFrameForEventType( animEvents, keyFrame, eventType );
		if ( curAnimEvent == -1 )
		{//this anim frame doesn't already have an event of this type on it
			curAnimEvent = lastAnimEvent;
		}

		//now that we know which event index we're going to plug the data into, start doing it
		animEvents[curAnimEvent].eventType = eventType;
		animEvents[curAnimEvent].keyFrame = keyFrame;

		//now read out the proper data based on the type
		switch ( animEvents[curAnimEvent].eventType )
		{
		case AEV_SOUNDCHAN:		//# animID AEV_SOUNDCHAN framenum CHANNEL soundpath randomlow randomhi chancetoplay
			token = COM_Parse( text_p );
			if ( !token ) 
			{
				break;
			}
			if ( Q_stricmp( token, "CHAN_VOICE_ATTEN" ) == 0 )
			{
				animEvents[curAnimEvent].eventData[AED_SOUNDCHANNEL] = CHAN_VOICE_ATTEN;
			}
			else if ( Q_stricmp( token, "CHAN_VOICE_GLOBAL" ) == 0 )
			{
				animEvents[curAnimEvent].eventData[AED_SOUNDCHANNEL] = CHAN_VOICE_GLOBAL;
			}
			else if ( Q_stricmp( token, "CHAN_ANNOUNCER" ) == 0 )
			{
				animEvents[curAnimEvent].eventData[AED_SOUNDCHANNEL] = CHAN_ANNOUNCER;
			}
			else if ( Q_stricmp( token, "CHAN_BODY" ) == 0 )
			{
				animEvents[curAnimEvent].eventData[AED_SOUNDCHANNEL] = CHAN_BODY;
			}
			else if ( Q_stricmp( token, "CHAN_WEAPON" ) == 0 )
			{
				animEvents[curAnimEvent].eventData[AED_SOUNDCHANNEL] = CHAN_WEAPON;
			}
			else if ( Q_stricmp( token, "CHAN_VOICE" ) == 0 )
			{
				animEvents[curAnimEvent].eventData[AED_SOUNDCHANNEL] = CHAN_VOICE;
			} 
			else
			{
				animEvents[curAnimEvent].eventData[AED_SOUNDCHANNEL] = CHAN_AUTO;
			}
			//fall through to normal sound
		case AEV_SOUND:			//# animID AEV_SOUND framenum soundpath randomlow randomhi chancetoplay
			//get soundstring
			token = COM_Parse( text_p );
			if ( !token ) 
			{
				break;
			}		
			strcpy(stringData, token);
			//get lowest value
			token = COM_Parse( text_p );
			if ( !token ) 
			{//WARNING!  BAD TABLE!
				break;
			}
			lowestVal = atoi( token );
			//get highest value
			token = COM_Parse( text_p );
			if ( !token ) 
			{//WARNING!  BAD TABLE!
				break;
			}
			highestVal = atoi( token );
			//Now precache all the sounds
			//NOTE: If we can be assured sequential handles, we can store sound indices
			//		instead of strings, unfortunately, if these sounds were previously
			//		registered, we cannot be guaranteed sequential indices.  Thus an array
			if(lowestVal && highestVal)
			{
				//assert(highestVal - lowestVal < MAX_RANDOM_ANIM_SOUNDS);
				if ((highestVal-lowestVal) >= MAX_RANDOM_ANIM_SOUNDS)
				{
					highestVal = lowestVal + (MAX_RANDOM_ANIM_SOUNDS-1);
				}
				for ( n = lowestVal, num = AED_SOUNDINDEX_START; n <= highestVal && num <= AED_SOUNDINDEX_END; n++, num++ )
				{
					if (stringData[0] == '*')
					{ //FIXME? Would be nice to make custom sounds work with animEvents.
						animEvents[curAnimEvent].eventData[num] = 0;
					}
					else
					{
						animEvents[curAnimEvent].eventData[num] = trap->S_RegisterSound( va( stringData, n ) );
					}
				}
				animEvents[curAnimEvent].eventData[AED_SOUND_NUMRANDOMSNDS] = num - 1;
			}
			else
			{
				if (stringData[0] == '*')
				{ //FIXME? Would be nice to make custom sounds work with animEvents.
					animEvents[curAnimEvent].eventData[AED_SOUNDINDEX_START] = 0;
				}
				else
				{
					animEvents[curAnimEvent].eventData[AED_SOUNDINDEX_START] = trap->S_RegisterSound( stringData );
				}
#ifndef FINAL_BUILD
				if ( !animEvents[curAnimEvent].eventData[AED_SOUNDINDEX_START] &&
					 stringData[0] != '*')
				{//couldn't register it - file not found
					Com_Printf( S_COLOR_RED "ParseAnimationSndBlock: sound %s does not exist (animevents.cfg %s)!\n", stringData, aeb_filename );
				}
#endif
				animEvents[curAnimEvent].eventData[AED_SOUND_NUMRANDOMSNDS] = 0;
			}
			//get probability
			token = COM_Parse( text_p );
			if ( !token ) 
			{//WARNING!  BAD TABLE!
				break;
			}
			animEvents[curAnimEvent].eventData[AED_SOUND_PROBABILITY] = atoi( token );

			//last part - cheat and check and see if it's a special overridable saber sound we know of...
			if ( !Q_stricmpn( "sound/weapons/saber/saberhup", stringData, 28 ) )
			{//a saber swing
				animEvents[curAnimEvent].eventType = AEV_SABER_SWING;
				animEvents[curAnimEvent].eventData[AED_SABER_SWING_SABERNUM] = 0;//since we don't know which one they meant if we're hacking this, always use first saber
				animEvents[curAnimEvent].eventData[AED_SABER_SWING_PROBABILITY] = animEvents[curAnimEvent].eventData[AED_SOUND_PROBABILITY];
				if ( lowestVal < 4 )
				{//fast swing
					animEvents[curAnimEvent].eventData[AED_SABER_SWING_TYPE] = 0;//SWING_FAST;
				}
				else if ( lowestVal < 7 )
				{//medium swing
					animEvents[curAnimEvent].eventData[AED_SABER_SWING_TYPE] = 1;//SWING_MEDIUM;
				}
				else
				{//strong swing
					animEvents[curAnimEvent].eventData[AED_SABER_SWING_TYPE] = 2;//SWING_STRONG;
				}
			}
			else if ( !Q_stricmpn( "sound/weapons/saber/saberspin", stringData, 29 ) )
			{//a saber spin
				animEvents[curAnimEvent].eventType = AEV_SABER_SPIN;
				animEvents[curAnimEvent].eventData[AED_SABER_SPIN_SABERNUM] = 0;//since we don't know which one they meant if we're hacking this, always use first saber
				animEvents[curAnimEvent].eventData[AED_SABER_SPIN_PROBABILITY] = animEvents[curAnimEvent].eventData[AED_SOUND_PROBABILITY];
				if ( stringData[29] == 'o' )
				{//saberspinoff
					animEvents[curAnimEvent].eventData[AED_SABER_SPIN_TYPE] = 0;
				}
				else if ( stringData[29] == '1' )
				{//saberspin1
					animEvents[curAnimEvent].eventData[AED_SABER_SPIN_TYPE] = 2;
				}
				else if ( stringData[29] == '2' )
				{//saberspin2
					animEvents[curAnimEvent].eventData[AED_SABER_SPIN_TYPE] = 3;
				}
				else if ( stringData[29] == '3' )
				{//saberspin3
					animEvents[curAnimEvent].eventData[AED_SABER_SPIN_TYPE] = 4;
				}
				else if ( stringData[29] == '%' )
				{//saberspin%d
					animEvents[curAnimEvent].eventData[AED_SABER_SPIN_TYPE] = 5;
				}
				else
				{//just plain saberspin
					animEvents[curAnimEvent].eventData[AED_SABER_SPIN_TYPE] = 1;
				}
			}
			break;
		case AEV_FOOTSTEP:		//# animID AEV_FOOTSTEP framenum footstepType
			//get footstep type
			token = COM_Parse( text_p );
			if ( !token ) 
			{
				break;
			}		
			animEvents[curAnimEvent].eventData[AED_FOOTSTEP_TYPE] = GetIDForString(footstepTypeTable, token);
			//get probability
			token = COM_Parse( text_p );
			if ( !token ) 
			{//WARNING!  BAD TABLE!
				break;
			}
			animEvents[curAnimEvent].eventData[AED_FOOTSTEP_PROBABILITY] = atoi( token );
			break;
		case AEV_EFFECT:		//# animID AEV_EFFECT framenum effectpath boltName
			//get effect index
			token = COM_Parse( text_p );
			if ( !token ) 
			{
				break;
			}
			animEvents[curAnimEvent].eventData[AED_EFFECTINDEX] = trap->FX_RegisterEffect( token );
			//get bolt index
			token = COM_Parse( text_p );
			if ( !token ) 
			{
				break;
			}		
			if ( Q_stricmp( "none", token ) != 0 && Q_stricmp( "NULL", token ) != 0 )
			{//actually are specifying a bolt to use
				Q_strncpyz(animEvents[curAnimEvent].stringData, token, sizeof(animEvents[curAnimEvent].stringData));
			}
			//NOTE: this string will later be used to add a bolt and store the index, as below:
			//animEvent->eventData[AED_BOLTINDEX] = gi.G2API_AddBolt( &cent->gent->ghoul2[cent->gent->playerModel], animEvent->stringData );
			//get probability
			token = COM_Parse( text_p );
			if ( !token ) 
			{//WARNING!  BAD TABLE!
				break;
			}
			animEvents[curAnimEvent].eventData[AED_EFFECT_PROBABILITY] = atoi( token );
			break;
		case AEV_FIRE:			//# animID AEV_FIRE framenum altfire chancetofire
			//get altfire
			token = COM_Parse( text_p );
			if ( !token ) 
			{//WARNING!  BAD TABLE!
				break;
			}
			animEvents[curAnimEvent].eventData[AED_FIRE_ALT] = atoi( token );
			//get probability
			token = COM_Parse( text_p );
			if ( !token ) 
			{//WARNING!  BAD TABLE!
				break;
			}
			animEvents[curAnimEvent].eventData[AED_FIRE_PROBABILITY] = atoi( token );
			break;
		case AEV_MOVE:			//# animID AEV_MOVE framenum forwardpush rightpush uppush
			//get forward push
			token = COM_Parse( text_p );
			if ( !token ) 
			{//WARNING!  BAD TABLE!
				break;
			}
			animEvents[curAnimEvent].eventData[AED_MOVE_FWD] = atoi( token );
			//get right push
			token = COM_Parse( text_p );
			if ( !token ) 
			{//WARNING!  BAD TABLE!
				break;
			}
			animEvents[curAnimEvent].eventData[AED_MOVE_RT] = atoi( token );
			//get upwards push
			token = COM_Parse( text_p );
			if ( !token ) 
			{//WARNING!  BAD TABLE!
				break;
			}
			animEvents[curAnimEvent].eventData[AED_MOVE_UP] = atoi( token );
			break;
		default:				//unknown?
			SkipRestOfLine( text_p );
			continue;
			break;
		}

		if ( curAnimEvent == lastAnimEvent )
		{
			lastAnimEvent++;
		}
	}	
}

/*
======================
BG_ParseAnimationEvtFile

Read a configuration file containing animation events
models/players/kyle/animevents.cfg, etc

This file's presence is not required

======================
*/
bgLoadedEvents_t bgAllEvents[MAX_ANIM_FILES];
int bgNumAnimEvents = 1;
static int bg_animParseIncluding = 0;
int BG_ParseAnimationEvtFile( const char *as_filename, int animFileIndex, int eventFileIndex ) 
{
	const char	*text_p;
	int			len;
	const char	*token;
	char		text[80000];
	char		sfilename[MAX_QPATH];
	fileHandle_t	f;
	int			i, j, upper_i, lower_i;
	int				usedIndex = -1;
	animevent_t	*legsAnimEvents;
	animevent_t	*torsoAnimEvents;
	animation_t		*animations;
	int				forcedIndex;
	
	assert(animFileIndex < MAX_ANIM_FILES);
	assert(eventFileIndex < MAX_ANIM_FILES);

	if (eventFileIndex == -1)
	{
		forcedIndex = 0;
	}
	else
	{
		forcedIndex = eventFileIndex;
	}

	if (bg_animParseIncluding <= 0)
	{ //if we should be parsing an included file, skip this part
		if ( bgAllEvents[forcedIndex].eventsParsed )
		{//already cached this one
			return forcedIndex;
		}
	}

	legsAnimEvents = bgAllEvents[forcedIndex].legsAnimEvents;
	torsoAnimEvents = bgAllEvents[forcedIndex].torsoAnimEvents;
	animations = bgAllAnims[animFileIndex].anims;

	if (bg_animParseIncluding <= 0)
	{ //if we should be parsing an included file, skip this part
		//Go through and see if this filename is already in the table.
		i = 0;
		while (i < bgNumAnimEvents && forcedIndex != 0)
		{
			if (!Q_stricmp(as_filename, bgAllEvents[i].filename))
			{ //looks like we have it already.
				return i;
			}
			i++;
		}
	}

	// Load and parse animevents.cfg file
	Com_sprintf( sfilename, sizeof( sfilename ), "%sanimevents.cfg", as_filename );

	if (bg_animParseIncluding <= 0)
	{ //should already be done if we're including
		//initialize anim event array
		for( i = 0; i < MAX_ANIM_EVENTS; i++ )
		{
			//Type of event
			torsoAnimEvents[i].eventType = AEV_NONE;
			legsAnimEvents[i].eventType = AEV_NONE;
			//Frame to play event on
			torsoAnimEvents[i].keyFrame = -1;	
			legsAnimEvents[i].keyFrame = -1;
			//we allow storage of one string, temporarily (in case we have to look up an index later, then make sure to set stringData to NULL so we only do the look-up once)
			torsoAnimEvents[i].stringData[0] = '\0';
			legsAnimEvents[i].stringData[0] = '\0';
			//Unique IDs, can be soundIndex of sound file to play OR effect index or footstep type, etc.
			for ( j = 0; j < AED_ARRAY_SIZE; j++ )
			{
				torsoAnimEvents[i].eventData[j] = -1;
				legsAnimEvents[i].eventData[j] = -1;
			}
		}
	}

	// load the file
	len = trap->FS_Open( sfilename, &f, FS_READ );
	if ( len <= 0 ) 
	{//no file
		goto fin;
	}
	if ( len >= sizeof( text ) - 1 ) 
	{
		trap->FS_Close(f);
#ifndef FINAL_BUILD
		Com_Error(ERR_DROP, "File %s too long\n", sfilename );
#else
		Com_Printf( "File %s too long\n", sfilename );
#endif
		goto fin;
	}

	trap->FS_Read( text, len, f );
	text[len] = 0;
	trap->FS_Close( f );

	// parse the text
	text_p = text;
	upper_i =0;
	lower_i =0;

	COM_BeginParseSession ("BG_ParseAnimationEvtFile");

	// read information for batches of sounds (UPPER or LOWER)
	while ( 1 ) 
	{
		// Get base frame of sequence
		token = COM_Parse( &text_p );
		if ( !token || !token[0] ) 
		{
			break;
		}

		if ( !Q_stricmp(token,"include") )	// grab from another animevents.cfg
		{//NOTE: you REALLY should NOT do this after the main block of UPPERSOUNDS and LOWERSOUNDS
			const char	*include_filename = COM_Parse( &text_p );
			if ( include_filename != NULL )
			{
				char fullIPath[MAX_QPATH];
				strcpy(fullIPath, va("models/players/%s/", include_filename));
				bg_animParseIncluding++;
				BG_ParseAnimationEvtFile( fullIPath, animFileIndex, forcedIndex );
				bg_animParseIncluding--;
			}
		}

		if ( !Q_stricmp(token,"UPPEREVENTS") )	// A batch of upper sounds
		{
			ParseAnimationEvtBlock( as_filename, torsoAnimEvents, animations, &upper_i, &text_p ); 
		}

		else if ( !Q_stricmp(token,"LOWEREVENTS") )	// A batch of lower sounds
		{
			ParseAnimationEvtBlock( as_filename, legsAnimEvents, animations, &lower_i, &text_p ); 
		}
	}

	usedIndex = forcedIndex;
fin:
	//Mark this anim set so that we know we tried to load he sounds, don't care if the load failed
	if (bg_animParseIncluding <= 0)
	{ //if we should be parsing an included file, skip this part
		bgAllEvents[forcedIndex].eventsParsed = qtrue;
		strcpy(bgAllEvents[forcedIndex].filename, as_filename);
		if (forcedIndex)
		{
			bgNumAnimEvents++;
		}
	}

	return usedIndex;
}
#endif

/*
======================
BG_ParseAnimationFile

Read a configuration file containing animation coutns and rates
models/players/visor/animation.cfg, etc

======================
*/
int BG_ParseAnimationFile(const char *filename, animation_t *animset, qboolean isHumanoid) 
{
#if 0
	char		*text_p;
	char		*token;
	float		fps;
	int			skip;
	int			animNum;
#endif
	int			len;
	int			i;
	int			usedIndex = -1;
	int			nextIndex = bgNumAllAnims;
	qboolean	dynAlloc = qfalse;
	qboolean	wasLoaded = qfalse;
	char		BGPAFtext[100000];
	fileHandle_t	f;
	
	BGPAFtext[0] = '\0';

	if (!isHumanoid)
	{
		i = 0;
		while (i < bgNumAllAnims)
		{ //see if it's been loaded already
			if (!Q_stricmp(bgAllAnims[i].filename, filename))
			{
				animset = bgAllAnims[i].anims;
				return i; //alright, we already have it.
			}
			i++;
		}

		//Looks like it has not yet been loaded. Allocate space for the anim set if we need to, and continue along.
		if (!animset)
		{
			if (strstr(filename, "players/_humanoid/"))
			{ //then use the static humanoid set.
				animset = bgHumanoidAnimations;
				nextIndex = 0;
			}
			else if (strstr(filename, "players/rockettrooper/"))
			{ //rockettrooper always index 1
				nextIndex = 1;
				animset = BG_AnimsetAlloc();
				dynAlloc = qtrue; //so we know to free this memory in case we have to return early. Don't want any leaks.

				if (!animset)
				{
					assert(!"Anim set alloc failed!");
					return -1;
				}
			}
			else if ( strstr (filename, "players/_humanoidJKG/") )
			{
			    nextIndex = 2; // JKG is always 2
			    animset = BG_AnimsetAlloc();
			    dynAlloc = qtrue;
			    
			    if ( !animset )
			    {
			        assert (!"Anim set allocation failed!");
			        return -1;
			    }
			}
			else
			{
				animset = BG_AnimsetAlloc();
				dynAlloc = qtrue; //so we know to free this memory in case we have to return early. Don't want any leaks.

				if (!animset)
				{
					assert(!"Anim set alloc failed!");
					return -1;
				}
			}
		}
	}
#ifdef _DEBUG
	else
	{
		assert(animset);
	}
#endif

	// load the file
	if (!BGPAFtextLoaded || !isHumanoid)
	{ //rww - We are always using the same animation config now. So only load it once.
		len = trap->FS_Open( filename, &f, FS_READ );
		if ( (len <= 0) || (len >= sizeof( BGPAFtext ) - 1) ) 
		{
			if (dynAlloc)
			{
				BG_AnimsetFree(animset);
			}
			if (len > 0)
			{
				Com_Error(ERR_DROP, "%s exceeds the allowed game-side animation buffer!", filename);
			}
			return -1;
		}

		trap->FS_Read( BGPAFtext, len, f );
		BGPAFtext[len] = 0;
		trap->FS_Close( f );
	}
	else
	{
		if (dynAlloc)
		{
			assert(!"Should not have allocated dynamically for humanoid");
			BG_AnimsetFree(animset);
		}
		return 0; //humanoid index
	}

    BG_ParseGenericAnimationFile (animset, MAX_ANIMATIONS, animTable, filename, BGPAFtext);
    
/*
#ifdef _DEBUG
	//Check the array, and print the ones that have nothing in them.
	for(i = 0; i < MAX_ANIMATIONS; i++)
	{	
		if (animTable[i].name != NULL)		// This animation reference exists.
		{
			if (animset[i].firstFrame <= 0 && animset[i].numFrames <=0)
			{	// This is an empty animation reference.
				Com_Printf("***ANIMTABLE reference #%d (%s) is empty!\n", i, animTable[i].name);
			}
		}
	}
#endif // _DEBUG
*/
#ifdef CONVENIENT_ANIMATION_FILE_DEBUG_THING
	SpewDebugStuffToFile();
#endif

	wasLoaded = BGPAFtextLoaded;

	if (isHumanoid)
	{
		bgAllAnims[0].anims = animset;
		Q_strncpyz (bgAllAnims[0].filename, filename, sizeof (bgAllAnims[0].filename));
		BGPAFtextLoaded = qtrue;

		usedIndex = 0;
	}
	else
	{
		bgAllAnims[nextIndex].anims = animset;
		strcpy(bgAllAnims[nextIndex].filename, filename);

		usedIndex = bgNumAllAnims;

		if (nextIndex > 2)
		{ //don't bother increasing the number if this ended up as a humanoid/rockettrooper load, or JKG humanoid
			bgNumAllAnims++;
		}
		else
		{
			BGPAFtextLoaded = qtrue;
			usedIndex = nextIndex;
		}
	}

	/*
	if (!wasLoaded && BGPAFtextLoaded)
	{ //just loaded humanoid skel - we always want the rockettrooper to be after it, in slot 1
#ifdef _DEBUG
		assert(BG_ParseAnimationFile("models/players/rockettrooper/animation.cfg", NULL, qfalse) == 1);
#else
		BG_ParseAnimationFile("models/players/rockettrooper/animation.cfg", NULL, qfalse);
#endif
	}
	*/

	return usedIndex;
}

/*
===================
LEGS Animations
Base animation for overall body
===================
*/
static void BG_StartLegsAnim( playerState_t *ps, int anim )
{
	if ( ps->pm_type >= PM_DEAD )
	{
		//assert(!BG_InDeathAnim(anim));	//--futuza: commenting out for now, can cause issues on debug mode when using melee, rolls, and jetpacks
		//please let me know if this assert fires on you (ideally before you close/ignore it) -rww

		//vehicles are allowed to do this.. IF it's a vehicle death anim
		if (ps->clientNum < MAX_CLIENTS || anim != BOTH_VT_DEATH1)
		{
			return;
		}
	}
	if ( ps->legsTimer > 0 )
	{
		return;		// a high priority animation is running
	}

	if (ps->legsAnim == anim)
	{
		BG_FlipPart(ps, SETANIM_LEGS);
	}
#ifdef _GAME
	else if (g_entities[ps->clientNum].s.legsAnim == anim)
	{ //toggled anim to one anim then back to the one we were at previously in
		//one frame, indicating that anim should be restarted.
		BG_FlipPart(ps, SETANIM_LEGS);
	}
#endif
	ps->legsAnim = anim;

	/*
	if ( pm->debugLevel ) {
		Com_Printf("%d:  StartLegsAnim %d, on client#%d\n", pm->cmd.serverTime, anim, pm->ps->clientNum);
	}
	*/
}

void PM_ContinueLegsAnim( int anim ) {
	if ( ( pm->ps->legsAnim ) == anim ) {
		return;
	}
	if ( pm->ps->legsTimer > 0 ) {
		return;		// a high priority animation is running
	}

	BG_StartLegsAnim( pm->ps, anim );
}

void PM_ForceLegsAnim( int anim) {
	if (BG_InSpecialJump(pm->ps->legsAnim) &&
		pm->ps->legsTimer > 0 &&
		!BG_InSpecialJump(anim))
	{
		return;
	}

	if (BG_InRoll(pm->ps, pm->ps->legsAnim) &&
		pm->ps->legsTimer > 0 &&
		!BG_InRoll(pm->ps, anim))
	{
		return;
	}

	pm->ps->legsTimer = 0;
	BG_StartLegsAnim( pm->ps, anim );
}



/*
===================
TORSO Animations
Override animations for upper body
===================
*/
void BG_StartTorsoAnim( playerState_t *ps, int anim )
{
	if ( ps->pm_type >= PM_DEAD )
	{
		//assert(!BG_InDeathAnim(anim));	//--futuza: commenting out for now, can cause issues on debug mode when using melee, rolls, and jetpacks
		//please let me know if this assert fires on you (ideally before you close/ignore it) -rww
		return;
	}

	if (ps->torsoAnim == anim)
	{
		BG_FlipPart(ps, SETANIM_TORSO);
	}
#ifdef _GAME
	else if (g_entities[ps->clientNum].s.torsoAnim == anim)
	{ //toggled anim to one anim then back to the one we were at previously in
		//one frame, indicating that anim should be restarted.
		BG_FlipPart(ps, SETANIM_TORSO);
	}
#endif
	ps->torsoAnim = anim;
}

void PM_StartTorsoAnim( int anim )
{
    BG_StartTorsoAnim(pm->ps, anim);
}


/*
-------------------------
PM_SetLegsAnimTimer
-------------------------
*/
void BG_SetLegsAnimTimer(playerState_t *ps, int time)
{
	ps->legsTimer = time;

	if (ps->legsTimer < 0 && time != -1 )
	{//Cap timer to 0 if was counting down, but let it be -1 if that was intentional.  NOTENOTE Yeah this seems dumb, but it mirrors SP.
		ps->legsTimer = 0;
	}
}

void PM_SetLegsAnimTimer(int time)
{
	BG_SetLegsAnimTimer(pm->ps, time);
}

/*
-------------------------
PM_SetTorsoAnimTimer
-------------------------
*/
void BG_SetTorsoAnimTimer(playerState_t *ps, int time )
{
	ps->torsoTimer = time;

	if (ps->torsoTimer < 0 && time != -1 )
	{//Cap timer to 0 if was counting down, but let it be -1 if that was intentional.  NOTENOTE Yeah this seems dumb, but it mirrors SP.
		ps->torsoTimer = 0;
	}
}

void PM_SetTorsoAnimTimer(int time )
{
	BG_SetTorsoAnimTimer(pm->ps, time);
}

void BG_SaberStartTransAnim( int clientNum, int saberAnimLevel, int weapon, int anim, float *animSpeed, int broken, float saberMoveSwingSpeed, float saberSwingSpeed, int saberMove )
{
	if ( anim >= BOTH_A1_T__B_ && anim <= BOTH_ROLL_STAB )
	{
		if ( weapon == WP_SABER )
		{
			saberInfo_t *saber = BG_MySaber( clientNum, 0 );
			if ( saber 
				&& saber->animSpeedScale != 1.0f )
			{
				*animSpeed *= saber->animSpeedScale;
			}
			saber = BG_MySaber( clientNum, 1 );
			if ( saber
				&& saber->animSpeedScale != 1.0f )
			{
				*animSpeed *= saber->animSpeedScale;
			}
		}
	}

	if ( ( (anim) >= BOTH_T1_BR__R && 
		(anim) <= BOTH_T1_BL_TL ) ||
		( (anim) >= BOTH_T2_BR__R && 
		(anim) <= BOTH_T2_BL_TL ) ||
		( (anim) >= BOTH_T3_BR__R && 
		(anim) <= BOTH_T3_BL_TL ) )
	{
		if ( saberAnimLevel == FORCE_LEVEL_1 )
		{
			*animSpeed *= 1.5f;
		}
		else if ( saberAnimLevel == FORCE_LEVEL_3 )
		{
			*animSpeed *= 0.75f;
		}

		if (broken & (1<<BROKENLIMB_RARM))
		{
			*animSpeed *= 0.5f;
		}
		else if (broken & (1<<BROKENLIMB_LARM))
		{
			*animSpeed *= 0.65f;
		}
	}
	else if (broken && PM_InSaberAnim(anim))
	{
		if (broken & (1<<BROKENLIMB_RARM))
		{
			*animSpeed *= 0.5f;
		}
		else if (broken & (1<<BROKENLIMB_LARM))
		{
			*animSpeed *= 0.65f;
		}
	}

	if( (saberMove >= LS_A_TL2BR &&
		saberMove <= LS_A_T2B) ||
		(saberMove >= LS_S_TL2BR &&
		saberMove <= LS_T1_BL__L) )
	{
		*animSpeed *= saberSwingSpeed;
	}
}

void JKG_ReloadAnimation( int firingMode, int weaponID, int anim, animation_t *anims, float *animSpeed )
{
	const weaponData_t *wp = BG_GetWeaponDataByIndex(weaponID);
	if(wp->weaponReloadTime == 0) {
		return;
	}
	*animSpeed *= (anims[anim].numFrames-1) * fabs((float)(anims[anim].frameLerp)) / (float)wp->weaponReloadTime;
}

/*
-------------------------
PM_SetAnimFinal
-------------------------
*/
void BG_SetAnimFinal(playerState_t *ps, animation_t *animations,
					 int setAnimParts,int anim,int setAnimFlags)
{
	float editAnimSpeed = 1;

	if (!animations)
	{
		return;
	}

	assert(anim > -1);
	assert(animations[anim].firstFrame > 0 || animations[anim].numFrames > 0);

	BG_SaberStartTransAnim(ps->clientNum, ps->fd.saberAnimLevel, ps->weapon, anim, &editAnimSpeed, ps->brokenLimbs, 
		SaberStances[ps->fd.saberAnimLevel].moves[ps->saberMove].animspeedscale, ps->saberSwingSpeed, ps->saberMove);

	// Set torso anim
	if (setAnimParts & SETANIM_TORSO)
	{
		// Don't reset if it's already running the anim
		if( !(setAnimFlags & SETANIM_FLAG_RESTART) && (ps->torsoAnim) == anim )
		{
			goto setAnimLegs;
		}
		// or if a more important anim is running
		if( !(setAnimFlags & SETANIM_FLAG_OVERRIDE) && ((ps->torsoTimer > 0)||(ps->torsoTimer == -1)) )
		{	
			goto setAnimLegs;
		}

		BG_StartTorsoAnim(ps, anim);

		if (setAnimFlags & SETANIM_FLAG_HOLD)
		{
			if (setAnimFlags & SETANIM_FLAG_HOLDLESS)
			{	// Make sure to only wait in full 1/20 sec server frame intervals.

				//[BugFix2]
				//Yeah, I don't think this was working correctly before
				//int dur;
				//int speedDif;
				if((ps->saberMove >= LS_A_TL2BR &&
					ps->saberMove < LS_A_BACKSTAB) ||
					(ps->saberMove >= LS_S_TL2BR &&
					ps->saberMove <= LS_T1_BL__L))
					{
						// todo: see if i can fix this
					}

				// 

				if( editAnimSpeed > 0 )
				{
					if(animations[anim].numFrames < 2)
					{//single frame animations should just run with one frame worth of animation.
						ps->torsoTimer = fabs((float)(animations[anim].frameLerp)) * (1/editAnimSpeed);
					}
					else
					{
						ps->torsoTimer = (animations[anim].numFrames-1) * fabs((float)(animations[anim].frameLerp)) * (1/editAnimSpeed);
					}

					if(ps->torsoTimer > 1)
					{	
						//set the timer to be one unit of time less than the actual animation time so the timer will expire on the frame at which the animation finishes.
						ps->torsoTimer--;
					}
				}
				
				/*
				dur = (animations[anim].numFrames-1) * fabs((float)(animations[anim].frameLerp));
				speedDif = dur - (dur * editAnimSpeed);
				dur += speedDif;
				if (dur > 1)
				{
					ps->torsoTimer = dur-1;
				}
				else
				{
					ps->torsoTimer = fabs((float)(animations[anim].frameLerp));
				}
				*/
				//[/BugFix2]
			}
			else
			{
				ps->torsoTimer = ((animations[anim].numFrames ) * fabs((float)(animations[anim].frameLerp)));
			}

			if (ps->fd.forcePowersActive & (1 << FP_RAGE))
			{
				ps->torsoTimer /= 1.7;
			}
		}
	}

setAnimLegs:
	// Set legs anim
	if (setAnimParts & SETANIM_LEGS)
	{
		// Don't reset if it's already running the anim
		if( !(setAnimFlags & SETANIM_FLAG_RESTART) && (ps->legsAnim) == anim )
		{
			goto setAnimDone;
		}
		// or if a more important anim is running
		if( !(setAnimFlags & SETANIM_FLAG_OVERRIDE) && ((ps->legsTimer > 0)||(ps->legsTimer == -1)) )
		{	
			goto setAnimDone;
		}

		BG_StartLegsAnim(ps, anim);

		if (setAnimFlags & SETANIM_FLAG_HOLD)
		{
			if (setAnimFlags & SETANIM_FLAG_HOLDLESS)
			{	// Make sure to only wait in full 1/20 sec server frame intervals.
				//[BugFix2]
				//Yeah, I don't think this was working correctly before
				//int dur;
				//int speedDif;
				
				if( editAnimSpeed > 0 )
				{
					if(animations[anim].numFrames < 2)
					{//single frame animations should just run with one frame worth of animation.
						ps->legsTimer = fabs((float)(animations[anim].frameLerp)) * (1/editAnimSpeed);
					}
					else
					{
						ps->legsTimer = (animations[anim].numFrames-1) * fabs((float)(animations[anim].frameLerp)) * (1/editAnimSpeed);
					}

					if(ps->legsTimer > 1)
					{	
						//set the timer to be one unit of time less than the actual animation time so the timer will expire on the frame at which the animation finishes.
						ps->legsTimer--;
					}
				}
			}
			else
			{
				ps->legsTimer = ((animations[anim].numFrames ) * fabs((float)(animations[anim].frameLerp)));
			}

			if (BG_RunningAnim(anim) ||
				BG_WalkingAnim(anim)) //these guys are ok, they don't actually reference pm
			{
				if (ps->fd.forcePowersActive & (1 << FP_RAGE))
				{
					ps->legsTimer /= 1.3;
				}
				else if (ps->fd.forcePowersActive & (1 << FP_SPEED))
				{
					ps->legsTimer /= 1.7;
				}
			}
		}
	}

setAnimDone:
	return;
}

void PM_SetAnimFinal(int setAnimParts,int anim,int setAnimFlags,
					 int blendTime)		// default blendTime=350
{
	BG_SetAnimFinal(pm->ps, pm->animations, setAnimParts, anim, setAnimFlags);
}


qboolean BG_HasAnimation(int animIndex, int animation)
{
	animation_t *animations;

	//must be a valid anim number
	if ( animation < 0 || animation >= MAX_ANIMATIONS )
	{
		return qfalse;
	}

	//Must have a file index entry
	if( animIndex < 0 || animIndex > bgNumAllAnims )
		return qfalse;

	animations = bgAllAnims[animIndex].anims;

	//No frames, no anim
	if ( animations[animation].numFrames == 0 )
		return qfalse;

	//Has the sequence
	return qtrue;
}

int BG_PickAnim( int animIndex, int minAnim, int maxAnim )
{
	int anim;
	int count = 0;

	do
	{
		anim = Q_irand(minAnim, maxAnim);
		count++;
	}
	while ( !BG_HasAnimation( animIndex, anim ) && count < 1000 );

	if (count == 1000)
	{ //guess we just don't have a death anim then.
		return -1;
	}

	return anim;
}

//I want to be able to use this on a playerstate even when we are not the focus
//of a pmove too so I have ported it to true BGishness.
//Please do not reference pm in this function or any functions that it calls,
//or I will cry. -rww
void BG_SetAnim(playerState_t *ps, animation_t *animations, int setAnimParts,int anim,int setAnimFlags)
{
	if (!animations)
	{
		animations = bgAllAnims[0].anims;
	}

	if (animations[anim].firstFrame == 0 && animations[anim].numFrames == 0)
	{
		if (anim == BOTH_RUNBACK1 ||
			anim == BOTH_WALKBACK1 ||
			anim == BOTH_RUN1 ||
			anim == BOTH_FEMALEEWALK ||
			anim == BOTH_FEMALERUN)
		{ //hack for droids
			anim = BOTH_WALK2;
		}

		if (animations[anim].firstFrame == 0 && animations[anim].numFrames == 0)
		{ //still? Just return then I guess.
			return;
		}
	}

	if (setAnimFlags&SETANIM_FLAG_OVERRIDE)
	{
		if (setAnimParts & SETANIM_TORSO)
		{
			if( (setAnimFlags & SETANIM_FLAG_RESTART) || (ps->torsoAnim) != anim )
			{
				BG_SetTorsoAnimTimer(ps, 0);
			}
		}
		if (setAnimParts & SETANIM_LEGS)
		{
			if( (setAnimFlags & SETANIM_FLAG_RESTART) || (ps->legsAnim) != anim )
			{
				BG_SetLegsAnimTimer(ps, 0);
			}
		}
	}

	BG_SetAnimFinal(ps, animations, setAnimParts, anim, setAnimFlags);
}

void PM_SetAnim(int setAnimParts,int anim,int setAnimFlags)
{	
	BG_SetAnim(pm->ps, pm->animations, setAnimParts, anim, setAnimFlags);
}

//[BugFix2]
//Fixed the logic problem with the timers

//BG versions of the animation point functions

//Get the point in the animation and return a percentage of the current point in the anim between 0 and the total anim length (0.0f - 1.0f)
//This function assumes that your animation timer is set to the exact length of the animation
float BG_GetTorsoAnimPoint(playerState_t *ps, int AnimIndex)
{
	float attackAnimLength = 0;
	float currentPoint = 0;
	float animSpeedFactor = 1.0f;
	float animPercentage = 0;

	//animSpeedFactor *= ns->saberSwingSpeed;

	//Be sure to scale by the proper anim speed just as if we were going to play the animation
	BG_SaberStartTransAnim(ps->clientNum, ps->fd.saberAnimLevel, ps->weapon, ps->torsoAnim, &animSpeedFactor, ps->brokenLimbs, 
		SaberStances[ps->fd.saberAnimLevel].moves[ps->saberMove].animspeedscale, ps->saberSwingSpeed, ps->saberMove);

	if( animSpeedFactor > 0 )
	{
		if(bgAllAnims[AnimIndex].anims[ps->torsoAnim].numFrames < 2)
		{//single frame animations should just run with one frame worth of animation.
			attackAnimLength = fabs((float)(bgAllAnims[AnimIndex].anims[ps->torsoAnim].frameLerp)) * (1/animSpeedFactor);
		}
		else
		{
			attackAnimLength = (bgAllAnims[AnimIndex].anims[ps->torsoAnim].numFrames-1) * fabs((float)(bgAllAnims[AnimIndex].anims[ps->torsoAnim].frameLerp)) * (1/animSpeedFactor);
		}

		if(attackAnimLength > 1)
		{	
			//set the timer to be one unit of time less than the actual animation time so the timer will expire on the frame at which the animation finishes.
			attackAnimLength--;
		}
	}

	currentPoint = ps->torsoTimer;

	animPercentage = currentPoint/attackAnimLength;

	return animPercentage;
}


float BG_GetLegsAnimPoint(playerState_t * ps, int AnimIndex)
{
	float attackAnimLength = 0;
	float currentPoint = 0;
	float animSpeedFactor = 1.0f;
	float animPercentage = 0;

	//Be sure to scale by the proper anim speed just as if we were going to play the animation
	BG_SaberStartTransAnim(ps->clientNum, ps->fd.saberAnimLevel, ps->weapon, ps->legsAnim, &animSpeedFactor, ps->brokenLimbs, 
		SaberStances[ps->fd.saberAnimLevel].moves[ps->saberMove].animspeedscale, ps->saberSwingSpeed, ps->saberMove);

	if( animSpeedFactor > 0 )
	{
		if(bgAllAnims[AnimIndex].anims[ps->legsAnim].numFrames < 2)
		{//single frame animations should just run with one frame worth of animation.
			attackAnimLength = fabs((float)(bgAllAnims[AnimIndex].anims[ps->legsAnim].frameLerp)) * (1/animSpeedFactor);
		}
		else
		{
			attackAnimLength = (bgAllAnims[AnimIndex].anims[ps->legsAnim].numFrames-1) * fabs((float)(bgAllAnims[AnimIndex].anims[ps->legsAnim].frameLerp)) * (1/animSpeedFactor);
		}
		
		if(attackAnimLength > 1)
		{	
			//set the timer to be one unit of time less than the actual animation time so the timer will expire on the frame at which the animation finishes.
			attackAnimLength--;
		}
	}

	currentPoint = ps->legsTimer;

	animPercentage = currentPoint/attackAnimLength;

	return animPercentage;
}
//[/BugFix2]
