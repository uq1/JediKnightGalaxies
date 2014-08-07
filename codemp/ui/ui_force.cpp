//
/*
=======================================================================

FORCE INTERFACE 

=======================================================================
*/

// use this to get a demo build without an explicit demo build, i.e. to get the demo ui files to build
#include "ui_local.h"
#include "qcommon/qfiles.h"
#include "ui_force.h"

int uiMaxPoints = 20;

extern const char *UI_TeamName(int team);

void Menu_ShowItemByName(menuDef_t *menu, const char *p, qboolean bShow);

extern int	uiSkinColor;
extern int	uiHoldSkinColor;

int uiSaberColorShaders[NUM_SABER_COLORS];

qboolean UI_SkinColor_HandleKey(int flags, float *special, int key, int num, int min, int max, int type) 
{
  if (key == A_MOUSE1 || key == A_MOUSE2 || key == A_ENTER || key == A_KP_ENTER) 
  {
  	int i = num;

	if (key == A_MOUSE2)
	{
	    i--;
	}
	else
	{
	    i++;
	}

    if (i < min)
	{
		i = max;
	}
	else if (i > max)
	{
      i = min;
    }

    num = i;

	uiSkinColor = num;

	uiHoldSkinColor = uiSkinColor;

	UI_FeederSelection(FEEDER_Q3HEADS, uiInfo.q3SelectedHead, NULL);

    return qtrue;
  }
  return qfalse;
}