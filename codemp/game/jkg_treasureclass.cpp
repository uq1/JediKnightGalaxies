#include "g_local.h"
#include <json/cJSON.h>

std::unordered_map<std::string, TreasureClass*> mTreasureRegistry;
std::vector<std::pair<std::string, TreasureOdds**>> vUnresolvedReferences;

static unsigned uiTotalTCsLoaded = 0;
static bool bLastFileFailed = false;

extern unsigned int JKG_Easy_GetItemIDFromInternal(const char *internalName);
extern itemData_t itemLookupTable[MAX_ITEM_TABLE_SIZE];

// Generate a treasure class from a JSON entry
TreasureClass::TreasureClass(void* json) : bValid(true) {
	cJSON* child = nullptr;

	// Load basic information
	child = cJSON_GetObjectItem(json, "info");
	if (child == nullptr) {
		bValid = false;
		return;
	} else {
		cJSON* info = nullptr;
		info = cJSON_GetObjectItem(child, "name");
		if (info == nullptr) {
			Com_Printf("TreasureClass without a name! Not loading.\n");
			bValid = false;
			return;
		} else {
			sName = cJSON_ToString(info);
			std::transform(sName.begin(), sName.end(), sName.begin(), ::tolower); // Make it lowercase
			if (mTreasureRegistry.find(sName) == mTreasureRegistry.end()) {
				Com_Printf(S_COLOR_YELLOW "WARNING: Overwriting existing treasure class %s\n", sName.c_str());
				mTreasureRegistry.erase(sName);
			}
		}

		info = cJSON_GetObjectItem(child, "numPicks");
		numPicks = cJSON_ToIntegerOpt(info, 1);
	}

	// Load info on treasure
	child = cJSON_GetObjectItem(json, "treasure");
	if (child != nullptr) {
		// Treasure classes without treasure are valid for scripting purposes (for later when I add GLua tools)
		cJSON* iterator = nullptr;
		for (iterator = cJSON_GetFirstItem(child); iterator != nullptr; iterator = cJSON_GetNextItem(iterator)) {
			std::string name = cJSON_GetItemKey(iterator);
			if (name.length() == 0) {
				Com_Printf(S_COLOR_YELLOW "WARNING in TreasureClass %s: Treasure without a search field!\n", sName.c_str());
				continue;
			}
			if (name == sName) {
				Com_Printf(S_COLOR_YELLOW "WARNING: TreasureClass %s uses itself as a pick. Removed.\n", sName.c_str());
				continue;
			}
			unsigned itemID = JKG_Easy_GetItemIDFromInternal(name.c_str());
			TreasureOdds* tcOdds = new TreasureOdds();
			if (itemID != -1) {
				// We're referring to a specific item
				tcOdds->bTC = false;
				tcOdds->Treasure.itm = &itemLookupTable[itemID];
			} else {
				// We're referring to another treasure class. Check and make sure that it exists.
				tcOdds->bTC = true;
				auto search = mTreasureRegistry.find(name);
				if (search == mTreasureRegistry.end()) {
					// Couldn't find it, so add it to a list of unresolved references
					tcOdds->bUnresolved = true;
					vUnresolvedReferences.push_back(std::make_pair(name, &tcOdds));
				} else {
					// Found it, so go ahead and change the pointer
					tcOdds->Treasure.tc = search->second;
				}
			}
			cJSON* odds = cJSON_GetObjectItem(iterator, "odds");
			tcOdds->odds = cJSON_ToIntegerOpt(odds, 1);
		}
	}
}

// Create multiple treasure classes from one file
TreasureClass::TreasureClass(const char *sFileName) : bValid(false) {
	fileHandle_t FILE;
	int len = trap->FS_Open(sFileName, &FILE, FS_READ);
	if (len <= 0 || !FILE) {
		Com_Printf("Couldn't load treasure class: %s\n", sFileName);
		bLastFileFailed = true;
		return;
	}
	// Load it
	char* buffer = new char[len + 1];
	trap->FS_Read(buffer, len, FILE);
	buffer[len] = '\0';
	trap->FS_Close(FILE);

	// Stuff it into the JSON parser
	char error[MAX_STRING_CHARS];
	cJSON* json = cJSON_ParsePooled(buffer, error, sizeof(error));
	cJSON* iterator = nullptr;
	if (json == nullptr) {
		Com_Printf("Couldn't load TC @%s: %s\n", sFileName, error);
		bLastFileFailed = true;
		return;
	}

	for (iterator = cJSON_GetFirstItem(json); iterator != nullptr; iterator = cJSON_GetNextItem(iterator)) {
		// Each item on the lowest level represents a treasure class definition
		TreasureClass* tc = new TreasureClass(iterator);
		if (!tc->IsValid()) {
			delete tc;
		}
		else {
			mTreasureRegistry[tc->GetName()] = tc;
		}
	}

	// Cleanup
	delete[] buffer;
	bLastFileFailed = false;
}

inv_t& TreasureClass::GenerateLoot(inv_t& in, unsigned numPicks) {
	if (totalChance == 0) {
		Com_Printf(S_COLOR_YELLOW "WARNING: Tried to generate loot from TC %s but no treasure is assigned to it!\n", sName.c_str());
		return in;
	}
	for (unsigned i = 0; i < numPicks; i++) {
		int num = Q_irand(0, totalChance - 1);
		int portion = 0;
		// Find the treasure class that fits with this
		for (auto it = vTreasure.begin(); it != vTreasure.end(); ++it) {
			portion += it->odds;
			if (portion >= num) {
				if (it->bTC) {
					it->Treasure.tc->GenerateLoot(in);
				} else {
					itemData_t* ptItemData = it->Treasure.itm;
					JKG_A_RollItem(ptItemData->itemID, 0, &in);
				}
				break;
			}
		}
	}
	return in;
}

void TreasureClass::EvaluateOdds() {
	// This just makes sure that we don't have any unresolved references that could be mucking up the odds
	for (auto it = vTreasure.begin(); it != vTreasure.end(); ++it) {
		if (it->bUnresolved) {
			totalChance -= it->odds;
			it = vTreasure.erase(it);
		}
	}
}

void TreasureClass::AddTo(const char* sRef, unsigned odds) {
	AddToNoEvaluate(sRef, odds);
	EvaluateOdds();
}

void TreasureClass::AddToNoEvaluate(const char* sRef, unsigned odds) {
	int itemID = JKG_Easy_GetItemIDFromInternal(sRef);
	TreasureOdds ptOdds;

	ptOdds.odds = odds;
	if (itemID == -1) {
		// It might be a reference to a treasure class.
		auto search = mTreasureRegistry.find(sRef);
		if (search == mTreasureRegistry.end()) {
			Com_Printf("Couldn't add reference ''%s'' to TC %s (does not exist)\n", sRef, sName.c_str());
			return;
		}
		ptOdds.bTC = true;
		ptOdds.bUnresolved = false;
		ptOdds.Treasure.tc = search->second;
	}
	else {
		ptOdds.bTC = false;
		ptOdds.bUnresolved = false;
		ptOdds.Treasure.itm = &itemLookupTable[itemID];
	}
	vTreasure.push_back(ptOdds);
}

/////////////////////////////////////////////////////////////////////////////////
//
//	Serverside init code
//
/////////////////////////////////////////////////////////////////////////////////

void InitTreasureClasses() {
	char treasureFiles[8192];
	int numFiles = trap->FS_GetFileList("ext_data/treasureclass", ".tc", treasureFiles, sizeof(treasureFiles));
	const char* treasureFile = treasureFiles;
	int numSucceeded = 0, numFailed = 0;

	/////////////////////////////////////////////////
	Com_Printf("Loading treasure classes...\n");
	for (int i = 0; i < numFiles; i++) {
		TreasureClass* newTreasure = new TreasureClass(treasureFile);
		if (bLastFileFailed) {
			numFailed++;
		} else {
			numSucceeded++;
		}
		delete newTreasure;
		treasureFile += strlen(treasureFile);
	}
	/////////////////////////////////////////////////
	Com_Printf("Resolving treasure class references...\n");
	int numResolved = 0, numUnresolved = 0;
	for (auto it = vUnresolvedReferences.begin(); it != vUnresolvedReferences.end(); it++) {
		auto search = mTreasureRegistry.find(it->first);
		if (search == mTreasureRegistry.end()) {
			Com_Printf(S_COLOR_YELLOW "Unresolved reference %s!", it->first.c_str());
			numUnresolved++;
			continue;
		}
		TreasureOdds** tcOdds = it->second;
		(*tcOdds)->Treasure.tc = search->second;
		(*tcOdds)->bUnresolved = false;
		numResolved++;
	}
	vUnresolvedReferences.clear();
	Com_Printf("Resolved %i treasure class references (%i unresolved)\n", numResolved, numUnresolved);
	/////////////////////////////////////////////////
	Com_Printf("Recalculating treasure odds...");
	for (auto it = mTreasureRegistry.begin(); it != mTreasureRegistry.end(); ++it) {
		TreasureClass* ptTC = it->second;
		ptTC->EvaluateOdds();
	}
	Com_Printf("done!\n");
	/////////////////////////////////////////////////
	Com_Printf("Loaded %i treasure class files successfully (%i failed)\n", numSucceeded, numFailed);
}

void ShutdownTreasureClasses() {
	for (auto it = mTreasureRegistry.begin(); it != mTreasureRegistry.end(); ++it) {
		delete it->second;
	}
	mTreasureRegistry.clear();
}