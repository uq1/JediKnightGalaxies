#include "../game/bg_items.h"
#include "../game/bg_weapons.h"
#include "ui_local.h"
#include "jkg_inventory.h"
#include <algorithm>

using namespace std;

/* Global variables */
static bool bLeftSelected = false;		/* Is the left side (or the right side) selected */
static int nSelected = -1;				/* The currently selected element */

static size_t nNumberInventoryItems;	/* The number of inventory items */
static size_t nNumberShopItems;			/* The number of shop items */
static size_t nNumberUnfilteredIItems;	/* The total number of inventory items before filtering */
static size_t nNumberUnfilteredSItems;	/* The total number of shop items before filtering */

static vector<pair<int, itemInstance_t*>> vInventoryItems;	/* The inventory items, after filtering */
static vector<pair<int, itemInstance_t*>> vShopItems;		/* The shop items, after filtering */

static size_t nInventoryScroll = 0;		// How far we've scrolled in the menu
static size_t nShopScroll = 0;			// How far we've scrolled in the menu

// This function constructs both the inventory and shop lists
void JKG_ConstructShopLists() {
	vInventoryItems.clear();
	vShopItems.clear();

	if (cgImports == nullptr) {
		// This gets called when the game starts, because ui_inventoryFilter gets modified
		return;
	}

	nNumberUnfilteredIItems = *(size_t*)cgImports->InventoryDataRequest(0);
	nNumberUnfilteredSItems = *(size_t*)cgImports->InventoryDataRequest(5);

	itemInstance_t* pAllInventoryItems = (itemInstance_t*)cgImports->InventoryDataRequest(1);
	itemInstance_t* pAllShopItems = (itemInstance_t*)cgImports->InventoryDataRequest(4);

	//
	// Filter the shop and inventory items based on the value of the cvar
	//
	for (int i = 0; i < nNumberUnfilteredIItems; i++) {
		itemInstance_t* pThisItem = &pAllInventoryItems[i];
		if (ui_inventoryFilter.integer == JKGIFILTER_ARMOR && pThisItem->id->itemType != ITEM_ARMOR) {
			continue;
		}
		else if (ui_inventoryFilter.integer == JKGIFILTER_WEAPONS && pThisItem->id->itemType != ITEM_WEAPON) {
			continue;
		}
		else if (ui_inventoryFilter.integer == JKGIFILTER_CONSUMABLES && pThisItem->id->itemType != ITEM_CONSUMABLE) {
			continue;
		}
		else if (ui_inventoryFilter.integer == JKGIFILTER_MISC) {
			continue; // FIXME
		}
		vInventoryItems.push_back(make_pair(i, pThisItem));
	}

	for (int i = 0; i < nNumberUnfilteredSItems; i++) {
		itemInstance_t* pThisItem = &pAllShopItems[i];
		if (ui_inventoryFilter.integer == JKGIFILTER_ARMOR && pThisItem->id->itemType != ITEM_ARMOR) {
			continue;
		}
		else if (ui_inventoryFilter.integer == JKGIFILTER_WEAPONS && pThisItem->id->itemType != ITEM_WEAPON) {
			continue;
		}
		else if (ui_inventoryFilter.integer == JKGIFILTER_CONSUMABLES && pThisItem->id->itemType != ITEM_CONSUMABLE) {
			continue;
		}
		else if (ui_inventoryFilter.integer == JKGIFILTER_MISC) {
			continue;
		}
		vShopItems.push_back(make_pair(i, pThisItem));
	}

	//
	// Sort the lists of items
	//
	if (ui_inventorySortMode.integer == 0) {
		// If it's 0, then we're sorting by item name
		sort(vInventoryItems.begin(), vInventoryItems.end(),
			[](const pair<int, itemInstance_t*>& a, const pair<int, itemInstance_t*>& b) -> bool {
			if (ui_inventorySortType.integer) {
				return Q_stricmp(UI_GetStringEdString2(a.second->id->displayName), UI_GetStringEdString2(b.second->id->displayName));
			}
			else {
				return Q_stricmp(UI_GetStringEdString2(b.second->id->displayName), UI_GetStringEdString2(b.second->id->displayName));
			}
		});
	}
	else if (ui_inventorySortMode.integer == 1) {
		// If it's 1, then we're sorting by price
		sort(vInventoryItems.begin(), vInventoryItems.end(),
			[](const pair<int, itemInstance_t*>& a, const pair<int, itemInstance_t*>& b) -> bool {
			if (ui_inventorySortType.integer) {
				return a.second->id->baseCost > b.second->id->baseCost;
			}
			else {
				return a.second->id->baseCost < b.second->id->baseCost;
			}
		});
	}

	if (ui_shopSortMode.integer == 0) {
		// If it's 0, then we're sorting by item name
		sort(vShopItems.begin(), vInventoryItems.end(),
			[](const pair<int, itemInstance_t*>& a, const pair<int, itemInstance_t*>& b) -> bool {
			if (ui_inventorySortType.integer) {
				return Q_stricmp(UI_GetStringEdString2(a.second->id->displayName), UI_GetStringEdString2(b.second->id->displayName));
			}
			else {
				return Q_stricmp(UI_GetStringEdString2(b.second->id->displayName), UI_GetStringEdString2(b.second->id->displayName));
			}
		});
	}
	else if (ui_shopSortMode.integer == 1) {
		// If it's 1, then we're sorting by price
		sort(vShopItems.begin(), vInventoryItems.end(),
			[](const pair<int, itemInstance_t*>& a, const pair<int, itemInstance_t*>& b) -> bool {
			if (ui_inventorySortType.integer) {
				return a.second->id->baseCost > b.second->id->baseCost;
			}
			else {
				return a.second->id->baseCost < b.second->id->baseCost;
			}
		});
	}

	// Reset the selection/scroll if it goes out of bounds
	nNumberInventoryItems = vInventoryItems.size();
	nNumberShopItems = vShopItems.size();
	if (bLeftSelected && nSelected >= nNumberInventoryItems) {
		nSelected = -1;
	}
	else if (!bLeftSelected && nSelected >= nNumberShopItems) {
		nSelected = -1;
	}

	if (nInventoryScroll >= nNumberInventoryItems) {
		nInventoryScroll = 0;
	}

	if (nShopScroll >= nNumberShopItems) {
		nShopScroll = 0;
	}
}

void JKG_ShopInventorySortChanged() {
	JKG_ConstructShopLists();
}

// The script that gets run when a shop arrow button is pressed
void JKG_ShopArrow(char** args) {
	const char* side = args[0];
	const char* direction = args[1];
	int count = atoi(args[2]);

	if (count <= 0) {
		trap->Print("shop_arrow called with 0 or negative scroll value\n");
		return;
	}

	if (!Q_stricmp(direction, "up")) {
		count *= -1;
	}

	if (!Q_stricmp(side, "left")) {
		if (nNumberInventoryItems > 0 && nInventoryScroll + count >= nNumberInventoryItems) {
			nInventoryScroll = nNumberInventoryItems - 1;
		}
		else if (nInventoryScroll + count < 0) {
			nInventoryScroll = 0;
		}
		else {
			nInventoryScroll += count;
		}
	}
	else if (!Q_stricmp(side, "right")) {
		if (nNumberShopItems > 0 && nShopScroll + count >= nNumberShopItems) {
			nShopScroll = nNumberShopItems - 1;
		}
		else if (nNumberShopItems + count < 0) {
			nShopScroll = 0;
		}
		else {
			nShopScroll += count;
		}
	}
	else {
		trap->Print("Unknown side '%s' used for shop_arrow jkgscript\n", side);
		return;
	}

	JKG_ConstructShopLists();
}

//
// The icon that shows up for each item button
// 
void JKG_ShopIconLeft(itemDef_t* item, int nOwnerDrawID) {
	if (nInventoryScroll + nOwnerDrawID >= nNumberInventoryItems) {
		// Don't draw it if there isn't an item in the slot
		return;
	}

	itemInstance_t* pThisItem = vInventoryItems[nInventoryScroll + nOwnerDrawID].second;
	qhandle_t shader = trap->R_RegisterShaderNoMip(pThisItem->id->visuals.itemIcon);
	trap->R_DrawStretchPic(item->window.rect.x, item->window.rect.y, item->window.rect.w, item->window.rect.h,
		0, 0, 1, 1, shader);
}

void JKG_ShopIconRight(itemDef_t* item, int nOwnerDrawID) {
	if (nShopScroll + nOwnerDrawID >= nNumberShopItems) {
		// Don't draw it if there isn't an item in the slot
		return;
	}

	itemInstance_t* pThisItem = vShopItems[nShopScroll + nOwnerDrawID].second;
	qhandle_t shader = trap->R_RegisterShaderNoMip(pThisItem->id->visuals.itemIcon);
	trap->R_DrawStretchPic(item->window.rect.x, item->window.rect.y, item->window.rect.w, item->window.rect.h,
		0, 0, 1, 1, shader);
}

//
// The selection highlight that shows on the item we have selected
//
void JKG_Shop_InventorySelection(itemDef_t* item, int nOwnerDrawID) {
	if (!bLeftSelected) {
		return; // An item on the left hand side is not selected.
	}
	if (nSelected != nInventoryScroll + nOwnerDrawID) {
		return; // The item we have selected is not this one.
	}
	trap->R_DrawStretchPic(item->window.rect.x, item->window.rect.y, item->window.rect.w, item->window.rect.h,
		0, 0, 1, 1, item->window.background);
}

void JKG_Shop_ShopSelection(itemDef_t* item, int nOwnerDrawID) {
	if (bLeftSelected) {
		return; // An item on the right hand side is not selected.
	}
	if (nSelected != nShopScroll + nOwnerDrawID) {
		return; // The item we have selected is not this one.
	}
	trap->R_DrawStretchPic(item->window.rect.x, item->window.rect.y, item->window.rect.w, item->window.rect.h,
		0, 0, 1, 1, item->window.background);
}

//
// The name of the item that shows on each button
//
extern void Item_Text_Paint(itemDef_t *item);
void JKG_Shop_InventoryItemName(itemDef_t* item, int nOwnerDrawID) {
	if (nInventoryScroll + nOwnerDrawID >= nNumberInventoryItems) {
		memset(item->text, 0, sizeof(item->text));
		Item_Text_Paint(item); // FIXME: should we really be trying to paint a blank string?
		return; // There isn't an item in this slot.
	}
	itemInstance_t* pItem = vInventoryItems[nInventoryScroll + nOwnerDrawID].second;
	Q_strncpyz(item->text, pItem->id->displayName, sizeof(item->text));
	Item_Text_Paint(item);
}

void JKG_Shop_ShopItemName(itemDef_t* item, int nOwnerDrawID) {
	if (nShopScroll + nOwnerDrawID >= nNumberShopItems) {
		memset(item->text, 0, sizeof(item->text));
		Item_Text_Paint(item); // FIXME: should we really be trying to paint a blank string?
		return; // There isn't an item in this slot.
	}
	itemInstance_t* pItem = vShopItems[nShopScroll + nOwnerDrawID].second;
	Q_strncpyz(item->text, pItem->id->displayName, sizeof(item->text));
	Item_Text_Paint(item);
}

//
// The action that gets performed when we select an item
//

void JKG_Shop_SelectLeft(char** args) {
	int id = atoi(args[0]);
	if (id + nInventoryScroll >= nNumberInventoryItems) {
		nSelected = -1;
		return;
	}
	bLeftSelected = true;
	nSelected = nInventoryScroll + id;
}

void JKG_Shop_SelectRight(char** args) {
	int id = atoi(args[0]);
	if (id + nShopScroll >= nNumberShopItems) {
		nSelected = -1;
		return;
	}
	bLeftSelected = false;
	nSelected = nShopScroll + id;
}