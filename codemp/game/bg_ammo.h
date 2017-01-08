// bg_ammo.h
// Contains data for the ammo.json table
// (c) 2013 Jedi Knight Galaxies

#pragma once

#include "qcommon/q_shared.h"

#define MAX_AMMO_NAME	32
#define MAX_AMMO_TYPES	256

struct ammo_t {
	char name[MAX_AMMO_NAME];
	char shortname[MAX_AMMO_NAME];
	char substitute[MAX_AMMO_NAME];
	int ammoIndex;
	int ammoMax;
	float pricePerUnit;

	ammo_t* pSub;	// pointer to the substitute..may point to itself!!

	struct {
		// Change means of damage on the weapon
		qboolean overrideMeans;
		int means;

		// The interpretation of this can be a bit confusing.
		// If modifyDamage is set but not overrideDamage, multiply damage by the modifier.
		// If modifyDamage and overrideDamage are both set, add damage
		// If overrideDamage is set but not modifyDamage, set the damage
		// If neither is set, then we don't do anything to damage
		qboolean overrideDamage;
		qboolean modifyDamage;
		union {
			int damage;
			float modifier;
		};

		// Change the debuffs? It sets, not adds.
		qboolean changeDebuffs;
		int newDebuffs;
	} overrides;
};

void BG_InitializeAmmo ( void );
ammo_t *BG_GetAmmo ( const char *ammoName );
ammo_t *BG_GetAmmo ( const int ammoIndex );
void BG_GetAllAmmoSubstitutions(int ammoIndex, std::vector<ammo_t*>& outSubs);

extern ammo_t ammoTable[MAX_AMMO_TYPES];
extern int numAmmoLoaded;
