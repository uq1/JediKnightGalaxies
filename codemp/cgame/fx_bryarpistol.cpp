// Bryar Pistol Weapon Effects

#include "cg_local.h"
#include "fx_local.h"

//TURRET
/*
-------------------------
FX_TurretProjectileThink
-------------------------
*/
void FX_TurretProjectileThink(  centity_t *cent, const struct weaponInfo_s *weapon )
{
	vec3_t forward;

	if ( VectorNormalize2( cent->currentState.pos.trDelta, forward ) == 0.0f )
	{
		forward[2] = 1.0f;
	}

	trap_FX_PlayEffectID( cgs.effects.turretShotEffect, cent->lerpOrigin, forward, -1, -1 );
}