// Copyright (C) 2011 Jedi Knight Galaxies
// jkg_equip.c: Handles weapon/armor equipping procedures.
// File by eezstreet

#include "g_local.h"

void initACI(gclient_t *client)
{
    memset (&client->coreStats.ACISlots, 0, sizeof (client->coreStats.ACISlots));
    client->coreStats.aciSlotsUsed = 0;
}

void JKG_EquipItem(gentity_t *ent, int iNum)
{
	if(!ent->client)
		return;

	if ( iNum < 0 || iNum >= ent->inventory->size() )
	{
		trap->SendServerCommand(ent->client->ps.clientNum, "print \"Invalid inventory slot.\n\"");
	    return;
	}

	if((*ent->inventory)[iNum].equipped)
	{
		//trap->SendServerCommand(ent->client->ps.clientNum, "print \"That item is already equipped.\n\"");
		return;
	}

	auto item = (*ent->inventory)[iNum];
	if (item.id->itemType == ITEM_WEAPON)
	{
	    int prevEquipped = -1;

		for (auto it = ent->inventory->begin(); it != ent->inventory->end(); ++it) {
			if (!it->id) {
				continue;
			}
			if (it->id->itemType == ITEM_WEAPON && it->equipped) {
				it->equipped = false;
				prevEquipped = it - ent->inventory->begin();
				break;
			}
		}

		(*ent->inventory)[iNum].equipped = true;
	    trap->SendServerCommand (ent->s.number, va ("ieq %d %d", iNum, prevEquipped));
		trap->SendServerCommand (ent->s.number, va ("chw %d", item.id->weaponData.varID));
	}
	else if (item.id->itemType == ITEM_ARMOR){
	    // Unequip the armor which is currently equipped at the slot the new armor will use.
	    int prevEquipped = -1;

		for (auto it = ent->inventory->begin(); it != ent->inventory->end(); ++it) {
			if (iNum == it - ent->inventory->begin()) {
				continue;
			}

			if (it->id->itemType == ITEM_ARMOR && it->equipped && item.id->armorData.armorSlot == it->id->armorData.armorSlot) {
				it->equipped = false;
				prevEquipped = it - ent->inventory->begin();
				break;
			}
		}

		(*ent->inventory)[iNum].equipped = true;
		ent->client->armorItems[item.id->armorData.armorSlot] = iNum;

		trap->SendServerCommand( ent->s.number, va("ieq %d %d", iNum, prevEquipped ));
		trap->SendServerCommand( -1, va("aequi %i %i %i", ent->client->ps.clientNum, item.id->armorData.armorSlot, item.id->armorData.armorID));
	}
	else
	{
		trap->SendServerCommand(ent->client->ps.clientNum, "print \"You cannot equip that item.\n\"");
	}
}

void JKG_UnequipItem(gentity_t *ent, int iNum)
{
	if(!ent->client)
		return;

	if ( iNum < 0 || iNum >= ent->inventory->size() )
	{
		trap->SendServerCommand(ent->client->ps.clientNum, "print \"Invalid inventory slot.\n\"");
	    return;
	}

	if(!(*ent->inventory)[iNum].equipped)
	{
		//trap->SendServerCommand(ent->client->ps.clientNum, "print \"That item is not equipped.\n\"");
		return;
	}

	if((*ent->inventory)[iNum].id->itemType == ITEM_WEAPON)
	{
		(*ent->inventory)[iNum].equipped = qfalse;
	    trap->SendServerCommand (ent->s.number, va ("iueq %i", iNum));
	    trap->SendServerCommand (ent->s.number, "chw 0");
	}
	else if((*ent->inventory)[iNum].id->itemType == ITEM_ARMOR)
	{
		(*ent->inventory)[iNum].equipped = qfalse;
		ent->client->armorItems[(*ent->inventory)[iNum].id->armorData.armorSlot] = 0;
		trap->SendServerCommand (ent->s.number, va ("iueq %i", iNum));
		trap->SendServerCommand(-1, va("aequi %i %i 0", ent->client->ps.clientNum, (*ent->inventory)[iNum].id->armorData.armorSlot));
	}
	else
	{
		//trap->SendServerCommand(ent->client->ps.clientNum, "print \"You cannot unequip that item.\n\"");
	}
}

void JKG_ShieldEquipped(gentity_t* ent, int shieldItemNumber, qboolean playSound) {
	if (shieldItemNumber < 0 || shieldItemNumber >= ent->inventory->size()) {
		trap->SendServerCommand(ent - g_entities, "print \"Invalid inventory index.\n\"");
		return;
	}

	if (ent->client->shieldEquipped) {
		// Already have a shield equipped. Mark the other shield as not being equipped.
		for (auto it = ent->inventory->begin(); it != ent->inventory->end(); ++it) {
			if (it->equipped && it->id->itemType == ITEM_SHIELD) {
				it->equipped = qfalse;
			}
		}
	}

	itemInstance_t* item = &(*ent->inventory)[shieldItemNumber];
	if (ent->client->ps.stats[STAT_MAX_SHIELD] == ent->client->ps.stats[STAT_SHIELD] && ent->client->ps.stats[STAT_SHIELD] != 0) {
		// If we're at max shield, and upgrading capacity, increase our shield amount to match
		ent->client->ps.stats[STAT_SHIELD] = item->id->shieldData.capacity;
	}
	item->equipped = qtrue;
	ent->client->ps.stats[STAT_MAX_SHIELD] = item->id->shieldData.capacity;
	ent->client->shieldEquipped = qtrue;
	ent->client->shieldRechargeLast = ent->client->shieldRegenLast = level.time;
	ent->client->shieldRechargeTime = item->id->shieldData.cooldown;
	ent->client->shieldRegenTime = item->id->shieldData.regenrate;

	if (playSound && item->id->shieldData.equippedSoundEffect[0]) {
		G_Sound(ent, CHAN_AUTO, G_SoundIndex(item->id->shieldData.equippedSoundEffect));
	}
}

void JKG_ShieldUnequipped(gentity_t* ent) {
	if (ent->client->shieldEquipped) {
		for (auto it = ent->inventory->begin(); it != ent->inventory->end(); ++it) {
			if (it->equipped && it->id->itemType == ITEM_SHIELD) {
				it->equipped = qfalse;
			}
		}
	}

	ent->client->ps.stats[STAT_MAX_SHIELD] = 0;
	ent->client->shieldEquipped = qfalse;
	ent->client->shieldRechargeLast = ent->client->shieldRegenLast = level.time;
	ent->client->shieldRegenTime = ent->client->shieldRechargeTime = 0;
}

void JKG_JetpackEquipped(gentity_t* ent, int jetpackItemNumber) {
	if (jetpackItemNumber < 0 || jetpackItemNumber >= ent->inventory->size()) {
		trap->SendServerCommand(ent - g_entities, "print \"Invalid item number.\n\"");
		return;
	}

	itemInstance_t* item = &(*ent->inventory)[jetpackItemNumber];
	if (item->id->itemType != ITEM_JETPACK) {
		trap->SendServerCommand(ent - g_entities, "print \"That item is not a jetpack.\n\"");
		return;
	}

	// Unequip the previous jetpack first
	JKG_JetpackUnequipped(ent);

	item->equipped = qtrue;

	ent->client->pItemJetpack = &item->id->jetpackData;
	ent->client->ps.jetpack = ent->client->pItemJetpack->pJetpackData - jetpackTable + 1;
}

void JKG_JetpackUnequipped(gentity_t* ent) {
	// Iterate through the inventory and remove the jetpack that is equipped
	for (auto it = ent->inventory->begin(); it != ent->inventory->end(); it++) {
		if (it->equipped && it->id->itemType == ITEM_JETPACK) {
			it->equipped = qfalse;
		}
	}

	ent->client->pItemJetpack = nullptr;
	ent->client->ps.jetpack = 0;
}

#define JETPACK_TOGGLE_TIME		200
void Jetpack_Off(gentity_t *ent)
{ //create effects?
	assert(ent && ent->client);

	ent->client->ps.eFlags &= ~EF_JETPACK_ACTIVE;

	jetpackData_t* jet = &jetpackTable[ent->client->ps.jetpack - 1];
	G_Sound(ent, CHAN_AUTO, G_SoundIndex(/*"sound/jkg/jetpack/jetoff" "sound/boba/JETON"*/ jet->visuals.deactivateSound));
}

void Jetpack_On(gentity_t *ent)
{ //create effects?
	assert(ent && ent->client);

	if (ent->client->ps.fd.forceGripBeingGripped >= level.time)
	{ //can't turn on during grip interval
		return;
	}

	if (ent->client->ps.fallingToDeath)
	{ //too late!
		return;
	}

	jetpackData_t* jet = &jetpackTable[ent->client->ps.jetpack - 1];
	G_Sound(ent, CHAN_AUTO, G_SoundIndex(/*"sound/jkg/jetpack/jeton" "sound/boba/JETON"*/ jet->visuals.activateSound));

	ent->client->ps.eFlags |= EF_JETPACK_ACTIVE;
}

void ItemUse_Jetpack(gentity_t *ent)
{
	assert(ent && ent->client);

	if (ent->client->jetPackToggleTime >= level.time)
	{
		return;
	}

	if (ent->health <= 0 ||
		ent->client->ps.stats[STAT_HEALTH] <= 0 ||
		(ent->client->ps.eFlags & EF_DEAD) ||
		ent->client->ps.pm_type == PM_DEAD)
	{ //can't use it when dead under any circumstances.
		return;
	}

	if (!ent->client->ps.jetpack) {
		// they don't have a jetpack equipped
		return;
	}

	if (!(ent->client->ps.eFlags & EF_JETPACK_ACTIVE) &&
		ent->client->ps.jetpackFuel < 5)
	{ //too low on fuel to start it up
		return;
	}

	if (ent->client->ps.eFlags & EF_JETPACK_ACTIVE)
	{
		Jetpack_Off(ent);
	}
	else
	{
		Jetpack_On(ent);
	}

	ent->client->jetPackToggleTime = level.time + JETPACK_TOGGLE_TIME;
}