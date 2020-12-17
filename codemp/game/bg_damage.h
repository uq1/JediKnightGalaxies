// bg_damage.h
// Contains all of the code pertaining to custom damage types (meansOfDamage.json)
// (c) 2016 Jedi Knight Galaxies

#pragma once

#include "qcommon/q_shared.h"
#include <vector>
#include <string>

#define MAX_OBIT_LENGTH			64
#define MAX_MEANSOFDAMAGE_NAME	64

struct meansOfDamage_t {
	char ref[MAX_MEANSOFDAMAGE_NAME];
	char inventoryName[MAX_QPATH];

	qboolean hitmarkerExplosives;
	qboolean disintegrate;

	struct {
		qboolean noDamagePlums;

		qboolean overrideDamagePlum;
		vec4_t overrideDamagePlumColor;

		qboolean overrideShieldDamagePlum;
		vec4_t overrideShieldDamagePlumColor;

		qboolean overrideLowDamagePlum;
		vec4_t overrideLowDamagePlumColor;
	} plums;

	struct {
		qboolean ignoreArmor;	// if true, this damage type goes through armor
		qboolean ignoreShield;	// if true, this damage types goes through shields
		qboolean dodgeable;		// if true, this damage type can be dodged
		qboolean shieldBlocks;	// if true, this damage type is blocked entirely by shields
		qboolean isEMP;	// if true, this damage is electric based (can effect electric equipment)
		float armor;
		float shield;
		float organic;
		float droid;
	} modifiers;

	struct {
		qboolean canDismember;
		qboolean blowChunks;
	} dismemberment;

	struct {
		char killMessage[MAX_OBIT_LENGTH];
		char deathMessage[MAX_OBIT_LENGTH];

		union {
			char message[MAX_OBIT_LENGTH];
			struct {
				char male[MAX_OBIT_LENGTH];
				char female[MAX_OBIT_LENGTH];
				char neuter[MAX_OBIT_LENGTH];
			} gendered;
		} suicideMessage;
		qboolean genderedStringsPresent;
	} killfeed;
};

void JKG_LoadMeansOfDamage();
meansOfDamage_t* JKG_GetMeansOfDamage(const std::string& ref);
meansOfDamage_t* JKG_GetMeansOfDamage(int num);
int JKG_GetMeansOfDamageIndex(const std::string& ref);

extern std::vector<meansOfDamage_t> allMeansOfDamage;