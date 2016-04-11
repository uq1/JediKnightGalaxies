#pragma once

#include "game/bg_items.h"
#include "game/bg_weapons.h"
#include "qcommon/q_shared.h"

extern vmCvar_t ui_inventoryFilter;

#define MAX_XML_BUFFER_SIZE	4096

typedef enum {
	JKGIFILTER_ALL,
	JKGIFILTER_ARMOR,
	JKGIFILTER_WEAPONS,
	JKGIFILTER_CONSUMABLES,
	JKGIFILTER_MISC,
} jkgFilterItems_t;

#pragma region XML
typedef enum {
	CONTAINCAT_PRODUCTIONINFO=1,
	CONTAINCAT_USAGE=2,
	CONTAINCAT_TECHSPEC=3,
} containgCats_t;

static int lastCat;
static int containingCat;
static int lastDepth2;
static char    *last_content;
static qboolean useAutoFill;
static weaponDataGrab_t autoFill;
static char formatStr[64];

int JKG_Inventory_ExamineMenuNumForItemType(const int itemType);
#pragma endregion

/* Structure Definitions */
typedef struct
{

    // LOOK AT ME, I'M A COMMENT.
    // Okay, I have your attention now. If you edit this struct, make sure you
    // update the struct in cgame/jkg_cg_items.h too.


	//oboi, clientside time!
	//Basic Item Characteristics
	char displayName[MAX_ITEM_NAME];
	char itemIcon[MAX_QPATH];
	char xml[MAX_QPATH];
	unsigned int itemID;
	jkgItemType_t itemType;
	unsigned int weight;
	unsigned int cost;

	//pSpell data needed by clientside
	unsigned int pSpell[MAX_PSPELLS];
	int affector[MAX_PSPELLS];
	unsigned int duration[MAX_PSPELLS];

	//Equipment/ACI information
	unsigned int weapon;
	unsigned int variation;
	int varID;
} cgItemData_t;

typedef struct
{

    // LOOK AT ME, I'M A COMMENT.
    // Okay, I have your attention now. If you edit this struct, make sure you
    // update the struct in cgame/jkg_cg_items.h too.

	//FIXME: Share this struct across cgame/ui

	cgItemData_t *id;
	unsigned int itemQuality;

	//pSpell stuff
	unsigned int amount[MAX_PSPELLS]; //NOTENOTE: Need to make a variable which holds all final values of stats

	//ACI stuff
	qboolean equipped;
} cgItemInstance_t;

void JKG_Shop_OpenDialog(char **args);
void JKG_Shop_CloseDialog ( char **args );
void JKG_Shop_ItemSelect(char **args);
void JKG_Shop_ArrowPrev(char **args);
void JKG_Shop_ArrowNext(char **args);
void JKG_Shop_Update(char **args);
void JKG_Shop_UpdateShopStuff(int filterVal);
void JKG_Shop_ClearFocus(char **args);
void JKG_Shop_BuyConfirm_Yes(char **args);
void JKG_Shop_BuyConfirm_No(char **args);
void JKG_Shop_UpdateCreditDisplay(void);
void JKG_Shop_BuyConfirm_Display(char **args);
void JKG_Shop_OpenInventoryMenu(char **args);
void JKG_Shop_UpdateNotify(int msg);
void JKG_Inventory_CloseFromShop( char **args );

void JKG_Shop_RestoreShopMenu(void);

void JKG_Shop_ArrowPrevClean(void);
void JKG_Shop_ArrowNextClean(void);

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