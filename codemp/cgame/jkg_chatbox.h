// jkg_chatbox.h
// JKG - Chatbox function defnitions
// Copyright (c) 2016 Jedi Knight Galaxies

#ifndef JKG_CHATBOX_H
#define JKG_CHATBOX_H

float Text_GetWidth(const char *text, int iFontIndex, float scale);		//figure out our text's width
static float ExtColor_GetLevel(char chr);
static int Text_ExtColorCodes(const char *text, vec4_t color);			//actually figures out RBG of a ^xRBG name
const char *Text_ConvertExtToNormal(const char *text);				//convert RBG colors to normal colors
void Text_DrawText(int x, int y, const char *text, const float* rgba, int iFontIndex, const int limit, float scale);	//draw text on screen

#endif


