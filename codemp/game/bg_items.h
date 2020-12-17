// bg_items.h
// Loader for the .itm and .loot (obsolete) formats
// (c) 2013 Jedi Knight Galaxies

#pragma once

#include "qcommon/q_shared.h"
#include "bg_jetpacks.h"
#include "bg_armor.h"

#ifdef _GAME
typedef struct gentity_s gentity_t;
#endif

#define MAX_ITEM_TABLE_SIZE     (65535)
#define MAX_ITEM_FILE_LENGTH    (16384)
#define MAX_ITEM_NAME			(64)
#define MAX_INVENTORY_ITEMS		(256)

#define MAX_ACI_SLOTS			(10)
#define MAX_ARMOR_PIECES		1024

/*
 * Enumerations
*/

typedef enum jkgItemType_e
{
    ITEM_UNKNOWN,
    ITEM_WEAPON,
    ITEM_ARMOR,
    ITEM_CLOTHING,		// Classified as "armor" in the filter
	ITEM_CONSUMABLE,
	ITEM_SHIELD,		// Classified as "armor" in the filter
	ITEM_JETPACK,		// Classified as "armor" in the filter
	ITEM_AMMO,			// Not acquirable by the player.

	NUM_ITEM_TYPES
} jkgItemType_t;

typedef enum
{
	ARMTYPE_LIGHT,
	ARMTYPE_MEDIUM,
	ARMTYPE_HEAVY,
	ARMTYPE_MAX
} armorTypes_t;

typedef enum
{
	ARMSLOT_HEAD,
	ARMSLOT_NECK,
	ARMSLOT_TORSO,
	ARMSLOT_ROBE,
	ARMSLOT_LEGS,
	ARMSLOT_GLOVES,
	ARMSLOT_BOOTS,
	ARMSLOT_SHOULDER,
	ARMSLOT_IMPLANTS,
	ARMSLOT_MAX
} armorSlots_t;

typedef enum
{
	IPT_ADD,		// Added an item to the inventory
	IPT_REM,		// Removed an item from the inventory
	IPT_ADDACI,		// Added an item to the inventory (and it is being forced to the ACI)
	IPT_CLEAR,		// Clears the client inventory
	IPT_OPEN,		// Tells the client to open their inventory
	IPT_QUANT,		// An item's quantity has been changed
	IPT_RESET,		// Reset the player's inventory (usually only done after a vid_restart)
	IPT_EQUIP,		// Equipped an item
	IPT_UNEQUIP,	// Unequipped an item
	IPT_NULL		// null packet
} itemPacketType_t;

typedef enum
{
	IPT_TRADE,			// Update the list of items that are being traded.
	IPT_TRADEOPEN,		// Same as above, except that it also forces the Trade menu to open.
	IPT_TRADETRANS,		// We have carried out a trade, tell the client that we are done.
	IPT_TRADESINGLE,	// We are picking a single item from a vendor or container
	IPT_TRADECANCEL,	// The trade was cancelled.
	IPT_TRADEREADY,		// The other party has indicated that they are okay with the trade.
	IPT_TRADECREDITS,	// The other party has entered the number of credits that they want to send.
} itemTradePacketType_t;

/*
 * Item-specific structures
 */

// Weapons
typedef struct {
	unsigned int weapon;
	unsigned int variation;
	int varID;
} itemWeaponData_t;

// Armor
typedef struct {
	char ref[MAX_QPATH];
	armorData_t* pArm;
} itemArmorData_t;

// Consumables
#define MAX_CONSUMABLE_SCRIPTNAME	32
typedef struct {
	char consumeScript[MAX_CONSUMABLE_SCRIPTNAME];
	int consumeAmount;
} itemConsumableData_t;

// Shields
#define SHIELD_DEFAULT_CAPACITY	25
#define SHIELD_DEFAULT_COOLDOWN	10000
#define SHIELD_DEFAULT_REGEN	100
typedef struct {
	int capacity;	// Total amount that the shield can absorb
	int cooldown;	// Time between when we received a hit and when the shield will start recharging
	int regenrate;	// Time (ms) it takes to recharge one shield unit

	char rechargeSoundEffect[MAX_QPATH];	// Sound that plays once we start charging
	char brokenSoundEffect[MAX_QPATH];		// Sound that plays once the shield is broken
	char equippedSoundEffect[MAX_QPATH];	// Sound that plays once the shield is equipped
	char chargedSoundEffect[MAX_QPATH];		// Sound that plays once a shield is finished charging
	char malfunctionSoundEffect[MAX_QPATH];	// Sound that plays if the shield has malfunctioned --futuza: to be added later
} itemShieldData_t;

// Jetpacks
typedef struct {
	char			ref[MAX_QPATH];	// Reference to a jetpackData_t
	jetpackData_t*	pJetpackData;	// Pointer to the actual jetpackData_t. Filled upon item load and copied to gclient_t/cg_t
} itemJetpackData_t;

// Ammo
typedef struct {
	char			ref[MAX_QPATH];	// Reference to an ammoData_t
	int				ammoIndex;		// Which ammo index of the ammo table it points to
	int				quantity;		// How much ammo to give when getting this item
} itemAmmoData_t;

// Visual item data
#ifndef _GAME
typedef struct {
	char itemIcon[MAX_QPATH];
} itemVisualData_t;
#endif

// Itemdata is the core, immutable data of an item. Two items of the same base type share the same item data.
typedef struct {
	//Basic Item Information
	char displayName[MAX_ITEM_NAME];
	char internalName[MAX_ITEM_NAME];
	int itemID;
	jkgItemType_t itemType;
	unsigned int weight;
	int maxStack;

	// Visual Data
#ifndef _GAME
	itemVisualData_t visuals;
#endif

	// Data specific to the jkgItemType
	union {
		itemWeaponData_t weaponData;
		itemArmorData_t armorData;
		itemConsumableData_t consumableData;
		itemShieldData_t shieldData;
		itemJetpackData_t jetpackData;
		itemAmmoData_t ammoData;
	};

	//Stats
	unsigned int baseCost;
} itemData_t;

// The item instance is what is kept in a player's inventory.
typedef struct {
	itemData_t* id;
	int quantity;
	bool equipped;
} itemInstance_t;

extern itemData_t* itemLookupTable;
extern int lastUsedItemID;

/*
 * Functions
 */
itemPacketType_t BG_ItemPacketFromName(const char* szPacketName);
const char* BG_ItemPacketName(const int itemPacket);
itemTradePacketType_t BG_TradePacketFromName(const char* szPacketName);
const char* BG_TradePacketName(const int itemPacket);
itemData_t *BG_GetItemByWeaponIndex(int weaponIndex);
int BG_FindItemByInternal(const char *internalName);
qboolean BG_HasWeaponItem(int clientNum, int weaponId);
itemData_t* BG_FindItemDataByName(const char* internalName);
itemInstance_t BG_ItemInstance(itemData_t* pItemData, const int quantity);
itemInstance_t BG_ItemInstance(const char* internalName, const int quantity);
itemInstance_t BG_ItemInstance(const int itemID, const int quantity);
int BG_FirstStack(const std::vector<itemInstance_t>& container, const int itemID);
int BG_NextStack(const std::vector<itemInstance_t>& container, const int itemID, const int prevStack);
void BG_LoadDefaultWeaponItems(void);
void BG_InitItems();
void BG_ShutdownItems();
void BG_PrintItemList();
#ifdef _CGAME
void BG_GiveItem(itemInstance_t item);
void BG_GiveItemNonNetworked(itemInstance_t item);
void BG_ReceivedItemPacket(itemPacketType_t packet);
void BG_RemoveItemStack(int itemStackNum);
void BG_RemoveItemNonNetworked(itemInstance_t item);
void BG_ChangeItemStackQuantity(int itemStackNum, int newQuantity);
void BG_AddItemToACI(int itemStackNum, int aciSlot);
void BG_ReceivedTradePacket(itemTradePacketType_t packet);
void BG_AdjustItemStackQuantity(int itemStack, int adjustment);
#elif _GAME
void BG_GiveItem(gentity_t* ent, itemInstance_t item, qboolean ACI = false);
void BG_GiveItemNonNetworked(gentity_t* ent, itemInstance_t item);
void BG_RemoveItemStack(gentity_t* ent, int itemStackNum);
void BG_SendItemPacket(itemPacketType_t packetType, gentity_t* ent, void* memData, int intData, int intData2);
void BG_ChangeItemStackQuantity(gentity_t* ent, int itemStackNum, int newQuantity);
qboolean BG_ConsumeItem(gentity_t* ent, int itemStackNum);
void BG_SendTradePacket(itemTradePacketType_t packetType, gentity_t* ent, gentity_t* other, void* memData, int intData, int intData2);
void BG_RemoveItemNonNetworked(gentity_t* ent, itemInstance_t item);
void BG_AdjustItemStackQuantity(gentity_t* ent, int itemStack, int adjustment);
#endif
