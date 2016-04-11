//       ____ ___________________   ___           ____  __ _______   ___  ________  ___ ______________
//      |    |\_   _____/\______ \ |   |         |    |/ _|\      \ |   |/  _____/ /   |   \__    ___/
//      |    | |    __)_  |    |  \|   |         |      <  /   |   \|   /   \  ___/    ~    \|    |   
//  /\__|    | |        \ |    `   \   |         |    |  \/    |    \   \    \_\  \    Y    /|    |   
//  \________|/_______  //_______  /___|         |____|__ \____|__  /___|\______  /\___|_  / |____|   
//                    \/         \/                      \/       \/            \/       \/           
//                         ________    _____   ____       _____  ____  ___ ______________ _________   
//                        /  _____/   /  _  \ |    |     /  _  \ \   \/  /|   \_   _____//   _____/   
//                       /   \  ___  /  /_\  \|    |    /  /_\  \ \     / |   ||    __)_ \_____  \    
//                       \    \_\  \/    |    \    |___/    |    \/     \ |   ||        \/        \   
//                        \______  /\____|__  /_______ \____|__  /___/\  \|___/_______  /_______  /   
//                               \/         \/        \/	   \/	   \_/			  \/        \/ (c)
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


