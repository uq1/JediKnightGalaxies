// bg_ammo.h
// Contains data for the ammo.json table
// (c) 2013 Jedi Knight Galaxies

#pragma once

#include "qcommon/q_shared.h"

#define MAX_AMMO_NAME	32
#define MAX_AMMO_TYPES	256

template <typename T>
struct complexAmmoOverride {
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
		std::pair<qboolean, int> means;
		std::pair<qboolean, int> splashmeans;

		complexAmmoOverride<int> damage;
		complexAmmoOverride<int> projectiles;
		complexAmmoOverride<double> splashRange;
		complexAmmoOverride<double> collisionSize;
		complexAmmoOverride<double> recoil;
		complexAmmoOverride<int> ammocost;
		complexAmmoOverride<int> fireDelay;
		complexAmmoOverride<int> bounces;
		complexAmmoOverride<int> accuracyRatingBase;
		complexAmmoOverride<int> accuracyRatingPerShot;
		complexAmmoOverride<double> knockback;


		std::pair<qboolean, int> hitscan;

	} overrides;

	// visual overrides, only on the client
	struct {
		std::pair<qboolean, std::string> crosshairShader;

		// "fire"
		complexAmmoOverride<double> muzzleLightIntensity;
		std::pair<qboolean, vec3_t> muzzleLightColor;
		std::pair<qboolean, std::string> chargingEffect;
		std::pair<qboolean, std::string> muzzleEffect;
		std::pair<qboolean, std::vector<std::string>> fireSound;

		struct {
			std::pair<qboolean, std::string> shader;
			complexAmmoOverride<double> minSize;
			complexAmmoOverride<double> maxSize;
			complexAmmoOverride<int> lifeTime;
		} traceline;

		struct {
			std::pair<qboolean, std::string> projectileModel;
			std::pair<qboolean, std::string> projectileEffect;
			std::pair<qboolean, std::string> runSound;

			complexAmmoOverride<double> lightIntensity;
			std::pair<qboolean, vec3_t> lightColor;

			std::pair<qboolean, std::string> deathEffect;
			std::pair<qboolean, std::string> impactEffect;
			std::pair<qboolean, std::string> deflectEffect;
		} projectile;

	} visualOverrides;
};

void BG_InitializeAmmo ( void );
ammo_t *BG_GetAmmo ( const char *ammoName );
ammo_t *BG_GetAmmo ( const int ammoIndex );
void BG_GetAllAmmoSubstitutions(int ammoIndex, std::vector<ammo_t*>& outSubs);
bool BG_WeaponAcceptsAlternateAmmo(int weapon, int variation);
qboolean BG_AmmoIsBasedOn(int ammoTypeIndex, int basedOnIndex);

void JKG_ApplyAmmoOverride(int& value, const complexAmmoOverride<int>& field);
void JKG_ApplyAmmoOverride(double& value, const complexAmmoOverride<double>& field);
void JKG_ApplySimpleAmmoOverride(int& value, const std::pair<qboolean, int>& field);
void JKG_ApplySimpleAmmoOverride(double& value, const std::pair<qboolean, double>& field);
qboolean JKG_SimpleAmmoOverridePresent(const std::pair<qboolean, int>& field);
qboolean JKG_SimpleAmmoOverridePresent(const std::pair<qboolean, double>& field);
qboolean JKG_SimpleAmmoOverridePresent(const std::pair<qboolean, std::string>& field);
qboolean JKG_SimpleAmmoOverridePresent(const std::pair<qboolean, std::vector<std::string>>& field);
qboolean JKG_SimpleAmmoOverridePresent(const std::pair<qboolean, vec3_t>& field);

extern ammo_t ammoTable[MAX_AMMO_TYPES];
extern int numAmmoLoaded;
