#include <json/cJSON.h>
#include "bg_ammo.h"

#define AMMO_NAMEBUFFER	8192
#define AMMO_FILEBUFFER	16386
#define AMMO_ERRBUFFER	1024

#if defined(_GAME)
	#include "g_local.h"
#elif defined(_CGAME)
	#include "cgame/cg_local.h"
#elif defined(IN_UI)
	#include "ui/ui_local.h"
#endif

ammo_t ammoTable[MAX_AMMO_TYPES];
int numAmmoLoaded = 0;

/*
============================
BG_GetAmmo

Gets a specific ammo type by name
============================
*/
// Overload 1: Get by string reference
ammo_t *BG_GetAmmo(const char *ammoName) {
	for (int i = 0; i < numAmmoLoaded; i++) {
		if (!Q_stricmp(ammoName, ammoTable[i].name)) {
			return &ammoTable[i];
		}
	}
	return nullptr;
}

// Overload 2: Get by integer
ammo_t *BG_GetAmmo(const int ammoIndex) {
	if (ammoIndex < 0 || ammoIndex >= MAX_AMMO_TYPES) {
		return nullptr;
	}
	return &ammoTable[ammoIndex];
}

/*
============================
BG_GiveAmmo

Not technically a BG function, it's a utility to prevent overflow
============================
*/
#ifdef _GAME
void BG_GiveAmmo(gentity_t* ent, ammo_t* ammo, qboolean max, int amount) {
	if (max) {
		ent->client->ammoTable[ammo->ammoIndex] = ammo->ammoMax;
	}
	else {
		ent->client->ammoTable[ammo->ammoIndex] += amount;
		if (ent->client->ammoTable[ammo->ammoIndex] > ammo->ammoMax) {
			ent->client->ammoTable[ammo->ammoIndex] = ammo->ammoMax;
		}
	}
}
#endif

/*
==============================
BG_GetRefillAmmoCost

Gets the cost of refilling ammo, given an array of ammo and a weapon
==============================
*/
int BG_GetRefillAmmoCost(int* ammo, weaponData_t* wp)
{
	int totalCost = 0;
	for (int i = 0; i < wp->numFiringModes; i++)
	{
		ammo_t* ammoType = wp->firemodes[i].ammoDefault;
		if (ammoType == nullptr)
		{
			continue;	// bad linkage - shouldn't happen
		}

		int ammoCount = ammoType->ammoMax - ammo[ammoType->ammoIndex];
		totalCost += ammoCount * ammoType->pricePerUnit;
	}
	return totalCost;
}

/*
============================
BG_GetAllAmmoSubstitutions

Gets all substitutions for a specifc ammo type
Also includes the original ammo type in the list
============================
*/
void BG_GetAllAmmoSubstitutions(int ammoIndex, std::vector<ammo_t*>& outSubs) {
	if (ammoIndex < 0 || ammoIndex >= numAmmoLoaded) {
		return;
	}

	outSubs.clear();

	ammo_t* find = &ammoTable[ammoIndex];
	for (int i = 0; i < numAmmoLoaded; i++) {
		ammo_t* thisAmmo = &ammoTable[i];
		if (thisAmmo->pSub == find) {
			outSubs.push_back(thisAmmo);
		}
	}
}

/*
============================
BG_WeaponAcceptsAlternateAmmo

Returns true if the weapon accepts alternate ammo, false otherwise
============================
*/
qboolean BG_WeaponAcceptsAlternateAmmo(int weapon, int variation) {
	weaponData_t* wp = GetWeaponData(weapon, variation);

	switch (weapon) {
		case WP_SABER:
		case WP_NONE:
		case WP_MELEE:
			return qfalse;
		default:
			break;
	}

	if (wp->firemodes[0].clipSize <= 0) {
		return qfalse;
	}

	if (wp->firemodes[0].useQuantity) {
		return qfalse;
	}

	return qtrue;
}

/*
============================
BG_AmmoIsBasedOn

Returns true if the ammo in arg 1 is based on (can be substituted for) the ammo in arg 2
============================
*/
qboolean BG_AmmoIsBasedOn(int ammoTypeIndex, int basedOnIndex)
{
	std::vector<ammo_t*> ammoTypes;
	BG_GetAllAmmoSubstitutions(basedOnIndex, ammoTypes);

	for (auto it = ammoTypes.begin(); it != ammoTypes.end(); ++it)
	{
		if ((*it)->ammoIndex == ammoTypeIndex)
		{
			return qtrue;
		}
	}

	return qfalse;
}

/*
============================
JKG_ApplyAmmoOverride

Applies an ammo override (if present)
============================
*/
void JKG_ApplyAmmoOverride(int& value, const ammoOverride<int>& field) {
	if (!field.bIsPresent) {
		return;
	}

	if (field.bSet) {
		value = field.set;
	}
	if (field.bAdd) {
		value += field.add;
	}
	if (field.multiply) {
		value *= field.multiply;
	}
}

void JKG_ApplyAmmoOverride(double& value, const ammoOverride<double>& field) {
	if (!field.bIsPresent) {
		return;
	}

	if (field.bSet) {
		value = field.set;
	}
	if (field.bAdd) {
		value += field.add;
	}
	if (field.multiply) {
		value *= field.multiply;
	}
}

/*
============================
JKG_ParseAmmoOverride_Int

Parses a single ammo override
============================
*/
static void JKG_ParseAmmoOverride_Int(cJSON* json, const char* name, ammoOverride<int>& field) {
	cJSON* node = cJSON_GetObjectItem(json, name);
	if (!node) {
		field.bIsPresent = qfalse;
	}
	else {
		cJSON* child;

		field.bIsPresent = qtrue;
		field.bAdd = qfalse;
		field.bSet = qfalse;
		field.bMultiply = qfalse;

		child = cJSON_GetObjectItem(node, "set");
		if (child) {
			field.bSet = qtrue;
			field.set = cJSON_ToInteger(child);
		}

		child = cJSON_GetObjectItem(node, "add");
		if (child) {
			field.bAdd = qtrue;
			field.add = cJSON_ToInteger(child);
		}

		child = cJSON_GetObjectItem(node, "multiply");
		if (child) {
			field.bMultiply = qtrue;
			field.multiply = cJSON_ToNumber(child);
		}
	}
}

/*
============================
JKG_ParseAmmoOverride_Float

Parses a single ammo override
============================
*/
static void JKG_ParseAmmoOverride_Float(cJSON* json, const char* name, ammoOverride<double>& field) {
	cJSON* node = cJSON_GetObjectItem(json, name);
	if (!node) {
		field.bIsPresent = qfalse;
	}
	else {
		cJSON* child;

		field.bIsPresent = qtrue;
		field.bAdd = qfalse;
		field.bSet = qfalse;
		field.bMultiply = qfalse;

		child = cJSON_GetObjectItem(node, "set");
		if (child) {
			field.bSet = qtrue;
			field.set = cJSON_ToNumber(child);
		}

		child = cJSON_GetObjectItem(node, "add");
		if (child) {
			field.bAdd = qtrue;
			field.add = cJSON_ToNumber(child);
		}

		child = cJSON_GetObjectItem(node, "multiply");
		if (child) {
			field.bMultiply = qtrue;
			field.multiply = cJSON_ToNumber(child);
		}
	}
}

/*
============================
JKG_ParseAmmoOverride_Means

Parses a means of death override
============================
*/
static void JKG_ParseAmmoOverride_Means(cJSON* json, const char* name, ammoOverride<int>& field) {
	cJSON* node = cJSON_GetObjectItem(json, name);
	if (!node) {
		field.bIsPresent = qfalse;
	}
	else {
		cJSON* child;

		field.bIsPresent = qtrue;
		field.bMultiply = qfalse;
		field.bAdd = qfalse;

		child = cJSON_GetObjectItem(json, "set");
		if (child) {
			field.bSet = qtrue;
			field.set = JKG_GetMeansOfDamageIndex(cJSON_ToString(child));
		}
		else {
			field.bSet = qfalse;
		}
	}
}

/*
============================
JKG_ParseAmmoOverrides

Overrides are things such as changing damage, effects, ...
============================
*/
static void JKG_ParseAmmoOverrides(ammo_t* ammo, cJSON* json) {
	// means, splashmeans, debuffs all have special handling
	
	JKG_ParseAmmoOverride_Means(json, "means", ammo->overrides.means);
	JKG_ParseAmmoOverride_Means(json, "splashmeans", ammo->overrides.splashmeans);


	JKG_ParseAmmoOverride_Int(json, "damage", ammo->overrides.damage);
	JKG_ParseAmmoOverride_Int(json, "projectiles", ammo->overrides.projectiles);
	JKG_ParseAmmoOverride_Int(json, "clipSize", ammo->overrides.clipSize);
	JKG_ParseAmmoOverride_Float(json, "splashRange", ammo->overrides.splashRange);
	JKG_ParseAmmoOverride_Float(json, "collisionSize", ammo->overrides.collisionSize);
	JKG_ParseAmmoOverride_Float(json, "recoil", ammo->overrides.recoil);
	JKG_ParseAmmoOverride_Int(json, "ammocost", ammo->overrides.ammocost);
	JKG_ParseAmmoOverride_Int(json, "fireDelay", ammo->overrides.fireDelay);
	JKG_ParseAmmoOverride_Int(json, "bounces", ammo->overrides.bounces);

}

/*
============================
JKG_ParseSingleAmmo

Reads a single ammo entry from a .ammo file
============================
*/
static qboolean JKG_ParseSingleAmmo(cJSON* json) {
	const char* name = cJSON_GetItemKey(json);
	ammo_t* ammo = &ammoTable[numAmmoLoaded];
	cJSON* child;

	ammo->ammoIndex = numAmmoLoaded;
	Q_strncpyz(ammo->name, name, sizeof(ammo->name));

	Q_strncpyz(ammo->shortname, cJSON_ToStringOpt(cJSON_GetObjectItem(json, "shortname"), ""), sizeof(ammo->shortname));

	// Parse substitutes (we need to establish the link later)
	ammo->pSub = nullptr;
	child = cJSON_GetObjectItem(json, "basedOn");
	if (child) {
		Q_strncpyz(ammo->substitute, cJSON_ToString(child), sizeof(ammo->substitute));
	}
	else {
		Q_strncpyz(ammo->substitute, ammo->name, sizeof(ammo->substitute));
	}

	child = cJSON_GetObjectItem(json, "max");
	ammo->ammoMax = cJSON_ToIntegerOpt(child, 999);

	child = cJSON_GetObjectItem(json, "pricePerUnit");
	ammo->pricePerUnit = cJSON_ToNumberOpt(child, 1.0);

	child = cJSON_GetObjectItem(json, "overrides");
	if (child) {
		JKG_ParseAmmoOverrides(ammo, child);
	}

	return qtrue;
}

/*
============================
JKG_LoadAmmo

Loads an individual jetpack (.jet) file.
Called on both the client and the server.
============================
*/
static qboolean JKG_LoadAmmo(const char* fileName, const char* dir) {
	int fileLen;
	fileHandle_t f;
	char fileBuffer[AMMO_FILEBUFFER];
	char error[AMMO_ERRBUFFER];

	fileLen = trap->FS_Open(va("%s%s", dir, fileName), &f, FS_READ);

	if (!f || fileLen == -1) {
		Com_Printf(S_COLOR_RED "Could not read %s\n", fileName);
		return qfalse;
	}

	if (fileLen == 0) {
		trap->FS_Close(f);
		Com_Printf(S_COLOR_RED "%s is blank\n", fileName);
		return qfalse;
	}

	if ((fileLen + 1) >= AMMO_FILEBUFFER) {
		trap->FS_Close(f);
		Com_Printf(S_COLOR_RED "%s is too big - max file size %d bytes (%.2f kb)\n", fileName, AMMO_FILEBUFFER, AMMO_FILEBUFFER / 1024.0f);
		return qfalse;
	}

	trap->FS_Read(&fileBuffer, fileLen, f);
	fileBuffer[fileLen] = '\0';
	trap->FS_Close(f);

	cJSON* json = cJSON_ParsePooled(fileBuffer, error, AMMO_ERRBUFFER);
	if (!json) {
		Com_Printf(S_COLOR_RED "%s: %s\n", fileName, error);
		return qfalse;
	}

	// Read each node in the json file from top level as an ammo_t
	cJSON* child;
	for (child = cJSON_GetFirstItem(json); child; child = cJSON_GetNextItem(child)) {
		if (numAmmoLoaded >= MAX_AMMO_TYPES) {
			Com_Printf("MAX_AMMO_TYPES exceeded\n");
			break;
		}
		if (JKG_ParseSingleAmmo(child)) {
			numAmmoLoaded++;
		}
	}

	cJSON_Delete(json);
	return qtrue;
}

/*
========================
BG_InitializeAmmo

Initializes the ammo subsystem.
========================
*/
void BG_InitializeAmmo ( void )
{
	char ammoFiles[AMMO_NAMEBUFFER] {0};
	const char* ammo;
	const char* ammoDir = "ext_data/ammo/";
	int failed = 0;
	int numFiles;

	numFiles = trap->FS_GetFileList(ammoDir, ".ammo", ammoFiles, sizeof(ammoFiles));
	ammo = ammoFiles;

	Com_Printf("------- Ammo -------\n");

	for (int i = 0; i < numFiles; i++) {
		if (!JKG_LoadAmmo(ammo, ammoDir)) {
			failed++;
			ammo += strlen(ammo) + 1;
			continue;
		}

		ammo += strlen(ammo) + 1;
	}

	Com_Printf("Ammo: %d successful, %d failed.\n", numAmmoLoaded, failed);
	Com_Printf("-------------------------------------\n");

	// Link up all of the substitutes
	for (int i = 0; i < numAmmoLoaded; i++) {
		ammoTable[i].pSub = BG_GetAmmo(ammoTable[i].substitute);
	}
}
