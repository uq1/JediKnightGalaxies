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
JKG_ParseAmmoOverrides

Overrides are things such as changing damage, effects, ...
============================
*/
static void JKG_ParseAmmoOverrides(ammo_t* ammo, cJSON* json) {
	cJSON* child;

	memset(&ammo->overrides, 0, sizeof(ammo->overrides));

	child = cJSON_GetObjectItem(json, "setDamage");
	if (child) {
		ammo->overrides.overrideDamage = qtrue;
		ammo->overrides.modifyDamage = qfalse;
		ammo->overrides.damage = cJSON_ToInteger(child);
	}
	else {
		child = cJSON_GetObjectItem(json, "addDamage");
		if (child) {
			ammo->overrides.modifyDamage = qtrue;
			ammo->overrides.overrideDamage = qtrue;
			ammo->overrides.damage = cJSON_ToInteger(child);
		}
		else {
			child = cJSON_GetObjectItem(json, "multiplyDamage");
			if (child) {
				ammo->overrides.modifyDamage = qtrue;
				ammo->overrides.overrideDamage = qfalse;
				ammo->overrides.modifier = cJSON_ToNumber(child);
			}
		}
	}

	child = cJSON_GetObjectItem(json, "setDebuffs");
	if (child) {
		ammo->overrides.changeDebuffs = qtrue;

		for (int i = 0; i < cJSON_GetArraySize(child); i++) {
			cJSON* arrayItem = cJSON_GetArrayItem(child, i);
			const char* str = cJSON_ToString(arrayItem);

			int num = GetIDForString(debuffTable, str);
			if (num == -1) {
				Com_Printf("Unknown damage type used: %s.\n", str);
			}
			else {
				ammo->overrides.newDebuffs |= (1 << num);
			}
		}
	}
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
	child = cJSON_GetObjectItem(json, "substitute");
	if (child) {
		Q_strncpyz(ammo->substitute, cJSON_ToString(child), sizeof(ammo->substitute));
	}
	else {
		Q_strncpyz(ammo->substitute, ammo->name, sizeof(ammo->substitute));
	}

	child = cJSON_GetObjectItem(json, "max");
	ammo->ammoMax = cJSON_ToIntegerOpt(child, 999);

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
