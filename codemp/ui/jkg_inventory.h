#pragma once

#include "game/bg_items.h"
#include "game/bg_weapons.h"
#include "qcommon/q_shared.h"

extern vmCvar_t ui_inventoryFilter;

typedef enum {
	JKGIFILTER_ALL,
	JKGIFILTER_WEAPONS,
	JKGIFILTER_ARMOR,
	JKGIFILTER_CONSUMABLES,
	JKGIFILTER_MISC,
} jkgFilterItems_t;


void JKG_Inventory_CheckACIKeyStroke(int key);
void JKG_Inventory_UpdateNotify(int msg);

void JKG_Inventory_OwnerDraw_CreditsText(itemDef_t* item);
void JKG_Inventory_OwnerDraw_ItemIcon(itemDef_t* item, int ownerDrawID);
void JKG_Inventory_OwnerDraw_ItemName(itemDef_t* item, int ownerDrawID);
void JKG_Inventory_OwnerDraw_SelHighlight(itemDef_t* item, int ownerDrawID);
void JKG_Inventory_OwnerDraw_SelItemIcon(itemDef_t* item);
void JKG_Inventory_OwnerDraw_SelItemName(itemDef_t* item);
void JKG_Inventory_OwnerDraw_SelItemDesc(itemDef_t* item, int ownerDrawID);
void JKG_Inventory_OwnerDraw_ItemTagTop(itemDef_t* item, int ownerDrawID);
void JKG_Inventory_OwnerDraw_ItemTagBottom(itemDef_t* item, int ownerDrawID);
void JKG_Inventory_OwnerDraw_Interact(itemDef_t* item, int ownerDrawID);
void JKG_Inventory_OwnerDraw_Interact_Button(int ownerDrawID, int key);
void JKG_Inventory_SelectItem(char** args);
void JKG_Inventory_ArrowUp(char** args);
void JKG_Inventory_ArrowDown(char** args);
void JKG_Inventory_Use(char** args);
void JKG_Inventory_Destroy(char** args);
void JKG_Inventory_ACISlot(char** args);
void JKG_Inventory_ACISlotAuto(char** args);
void JKG_Inventory_ACIRemove(char** args);
void JKG_Inventory_EquipArmor(char** args);
void JKG_Inventory_UnequipArmor(char** args);
void JKG_Inventory_Open(char** args);