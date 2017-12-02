// bg_ammo.h
// Contains data for the ammo.json table
// (c) 2013 Jedi Knight Galaxies

#pragma once

#include "qcommon/q_shared.h"

#define MAX_AMMO_NAME	32
#define MAX_AMMO_TYPES	256

typedef enum {
	AOV_MEANS,
	AOV_SPLASHMEANS,
	AOV_DAMAGE,
	AOV_DEBUFFS,
	AOV_PROJECTILES,
	AOV_CLIPSIZE,
	AOV_SPLASHRANGE,
	AOV_COLLISIONSIZE,
	AOV_RECOIL,
	AOV_AMMOCOST,
	AOV_FIREDELAY,
	AOV_BOUNCES
} ammoOverrideTypes_t;

template <typename T>
struct ammoOverride {
	qboolean bIsPresent;
	qboolean bSet;
	qboolean bAdd;
	qboolean bMultiply;

	T set;
	T add;
	float multiply;
};

struct ammo_t {
	char name[MAX_AMMO_NAME];
	char shortname[MAX_AMMO_NAME];
	char substitute[MAX_AMMO_NAME];
	int ammoIndex;
	int ammoMax;
	float pricePerUnit;

	ammo_t* pSub;	// pointer to the substitute..may point to itself!!

	struct {
		// Should correspond to the ammoOverrideTypes_t enum
		ammoOverride<int> means;
		ammoOverride<int> splashmeans;
		ammoOverride<int> damage;
		ammoOverride<int> debuffs;
		ammoOverride<int> projectiles;
		ammoOverride<int> clipSize;
		ammoOverride<double> splashRange;
		ammoOverride<double> collisionSize;
		ammoOverride<double> recoil;
		ammoOverride<int> ammocost;
		ammoOverride<int> fireDelay;
		ammoOverride<int> bounces;
	} overrides;
};

void BG_InitializeAmmo ( void );
ammo_t *BG_GetAmmo ( const char *ammoName );
ammo_t *BG_GetAmmo ( const int ammoIndex );
void BG_GetAllAmmoSubstitutions(int ammoIndex, std::vector<ammo_t*>& outSubs);
bool BG_WeaponAcceptsAlternateAmmo(int weapon, int variation);

void JKG_ApplyAmmoOverride(int& value, const ammoOverride<int>& field);
void JKG_ApplyAmmoOverride(double& value, const ammoOverride<double>& field);

extern ammo_t ammoTable[MAX_AMMO_TYPES];
extern int numAmmoLoaded;
