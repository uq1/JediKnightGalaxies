// bg_jetpacks.h
// Contains everything you could possibly need to do with jetpacks
// (c) 2016 Jedi Knight Galaxies

#pragma once

#include "qcommon/q_shared.h"
#include <vector>

#define MAX_JETPACKS	64

typedef struct {
	char	ref[MAX_QPATH];		// The name of this jetpack (internal name, for referencing)
	int		fuelCapacity;		// Total amount of fuel that can be stored
	float	fuelConsumption;	// Fuel consumption when idling
	float	thrustConsumption;	// Fuel consumption from thrusting (hold jump)
	float	fuelRegeneration;	// How much to modify fuel regeneration

	// Movement related fields
	typedef struct {
		float	hoverGravity;		// How much to modify gravity when hovering

		float	forwardMove;		// How much to modify forward move (not during thrust)
		float	backMove;			// How much to modify backward move (not during thrust)
		float	sideMove;			// How much to modify side move (not during thrust)
		float	downMove;			// How much to modify down move (not during thrust)

		float	thrustFwd;			// How much to modify forward move (in thrust)
		float	thrustBack;			// How much to modify backward move (in thrust)
		float	thrustSide;			// How much to modify side move (in thrust)
		float	thrustUp;			// How much to modify up move (in thrust)
	} jetpackMovementData_t;
	jetpackMovementData_t move;

	// Visual fields
	typedef struct {
		char	modelName[MAX_QPATH];		// The model of the jetpack

		typedef struct {
			char	boneBolt[MAX_QPATH];	// The bone to bolt to
		} jetpackBolt_t;
		std::vector <jetpackBolt_t> effectBolts;

		char	hoverEffect[MAX_QPATH];		// The effect to bolt on when hovering (not thrusting)
		char	thrustEffect[MAX_QPATH];	// The effect to bolt on when thrusting (not hovering)
		char	jetEffect[MAX_QPATH];		// The effect to play when launching the player

		char	idleSound[MAX_QPATH];		// The sound to play when hovering (not thrusting)
		char	thrustSound[MAX_QPATH];		// The sound to play when thrusting (not hovering)
		char	activateSound[MAX_QPATH];	// The sound to play when the jetpack is turned on
		char	deactivateSound[MAX_QPATH];	// The sound to play when the jetpack is turned off
	} jetpackVisualData_t;
	jetpackVisualData_t visuals;
} jetpackData_t;

extern jetpackData_t jetpackTable[MAX_JETPACKS];
extern int numLoadedJetpacks;

void JKG_LoadJetpacks();
jetpackData_t* JKG_FindJetpackByName(const char* jetpackName);