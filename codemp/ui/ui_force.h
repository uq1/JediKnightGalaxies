#pragma once

#include "qcommon/qfiles.h"

#define NUM_FORCE_STAR_IMAGES  9
#define FORCE_NONJEDI	0
#define FORCE_JEDI		1

extern int uiSaberColorShaders[NUM_SABER_COLORS];
// Dots above or equal to a given rank carry a certain color.

qboolean UI_SkinColor_HandleKey(int flags, float *special, int key, int num, int min, int max, int type);
