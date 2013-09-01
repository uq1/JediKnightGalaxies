// Copyright (C) 1999-2000 Id Software, Inc.
//
/**********************************************************************
	UI_ATOMS.C

	User interface building blocks and support functions.
**********************************************************************/
#include "ui_local.h"

qboolean		m_entersound;		// after a frame, so caching won't disrupt the sound

void QDECL UI_Printf( const char *msg, ... ) {
	va_list		argptr;
	char		text[1024] = {0};

	va_start( argptr, msg );
	Q_vsnprintf( text, sizeof( text ), msg, argptr );
	va_end( argptr );

	trap_Print( text );
}

void QDECL UI_Error( const char *msg, ... ) {
	va_list		argptr;
	char		text[1024] = {0};

	va_start (argptr, msg);
	Q_vsnprintf( text, sizeof( text ), msg, argptr );
	va_end (argptr);

	trap_Error( text );
}

void QDECL Com_Error( int level, const char *error, ... ) {
	va_list		argptr;
	char		text[4096] = {0};

	va_start (argptr, error);
	Q_vsnprintf( text, sizeof( text ), error, argptr );
	va_end (argptr);

	trap_Error( va("%s", text) );
}

void QDECL Com_Printf( const char *msg, ... ) {
	va_list		argptr;
	char		text[4096] = {0};
	int ret;

	va_start (argptr, msg);
	ret = Q_vsnprintf (text, sizeof( text ), msg, argptr);
	va_end (argptr);

	if ( ret == -1 )
		trap_Print( "Com_Printf: overflow of 4096 bytes buffer\n" );
	else
		trap_Print( va("%s", text) );
}

qboolean newUI = qfalse;


/*
=================
UI_ClampCvar
=================
*/
float UI_ClampCvar( float min, float max, float value )
{
	if ( value < min ) return min;
	if ( value > max ) return max;
	return value;
}

/*
=================
UI_StartDemoLoop
=================
*/
void UI_StartDemoLoop( void ) {
	trap_Cmd_ExecuteText( EXEC_APPEND, "d1\n" );
}


char *UI_Argv( int arg ) {
	static char	buffer[MAX_STRING_CHARS];

	trap_Argv( arg, buffer, sizeof( buffer ) );

	return buffer;
}


char *UI_Cvar_VariableString( const char *var_name ) {
	static char	buffer[MAX_STRING_CHARS];

	trap_Cvar_VariableStringBuffer( var_name, buffer, sizeof( buffer ) );

	return buffer;
}

static void	UI_Cache_f() {
	int i;
	Display_CacheAll();
	if (trap_Argc() == 2) {
		for (i = 0; i < uiInfo.q3HeadCount; i++)
		{
			trap_Print( va("model %s\n", uiInfo.q3HeadNames[i]) );
		}
	}
}

#include "json/cJSON.h"
#include "jkg_ui_auxlib.h"

// TEST
int testMasterFinalFunc (asyncTask_t *task) {
	cJSON *data = (cJSON *)task->finalData;
	
	if (task->errorCode == 0) {
		Com_Printf("Test successful! (bounce: %i - %i)\n", cJSON_ToInteger(cJSON_GetObjectItem(data, "bounce")), trap_Milliseconds());
	} else {
		Com_Printf("Test failed!\n");
	}
	return 0;
}

int termsMasterFinalFunc (asyncTask_t *task) {
	cJSON *data = (cJSON *)task->finalData;
	
	if (task->errorCode == 0) {
		Com_Printf("Terms of use:\n%s\n", cJSON_ToStringOpt(cJSON_GetObjectItem(data, "message"), ""));
	} else {
		Com_Printf("Could not obtain terms of use!\n");
	}
	return 0;
}

int registerFinalFunc (asyncTask_t *task) {
	cJSON *data = (cJSON *)task->finalData;
	
	if (task->errorCode == 0) {
		if (cJSON_ToInteger(cJSON_GetObjectItem(data, "errorCode"))) {
			Com_Printf("Registration failed: %s\n", cJSON_ToString(cJSON_GetObjectItem(data, "message")));
		} else {
			Com_Printf("Registration successful\n");
		}
	} else {
		Com_Printf("Registration request failed\n");
	}
	return 0;
}

int loginFinalFunc (asyncTask_t *task) {
	int errorcode;
	cJSON *data = (cJSON *)task->finalData;
	
	if (task->errorCode == 0) {
		errorcode = cJSON_ToInteger(cJSON_GetObjectItem(data, "errorCode"));
		if (errorcode == 0) {
			Com_Printf("Login successful!\n");
		} else if (errorcode == 1) {
			Com_Printf("Bad username or password!\n");
		} else if (errorcode == 7) {
			Com_Printf("Could not access database, please try again in a moment!\n");
		} else if (errorcode == 8) {
			Com_Printf("Account is not yet activated!\n");
		}
	} else {
		Com_Printf("Login request failed\n");
	}
	return 0;
}




/*
=================
UI_ConsoleCommand
=================
*/
qboolean UI_ConsoleCommand( int realTime ) {
	char	*cmd;

	uiInfo.uiDC.frameTime = realTime - uiInfo.uiDC.realTime;
	uiInfo.uiDC.realTime = realTime;

	cmd = UI_Argv( 0 );

	// ensure minimum menu data is available
	//Menu_Cache();

// JKG - Only allow these commands if we're using a debug compile
// We don't want players fuckin around with these
#ifdef _DEBUG

	if ( Q_stricmp (cmd, "ui_report") == 0 ) {
		UI_Report();
		return qtrue;
	}
	/*if ( Q_stricmp (cmd, "ui_testmaster") == 0) {
		JKG_GLUI_Task_Test(testMasterFinalFunc);
		return qtrue;
	}

	if ( Q_stricmp (cmd, "ui_testterms") == 0) {
		JKG_GLUI_Task_GetTermsOfUse(termsMasterFinalFunc);
		return qtrue;
	}

	if ( Q_stricmp (cmd, "ui_testregister") == 0 ) {
		char username[32];
		char password[32];
		char email[64];

		trap_Argv(1, username, 32);
		trap_Argv(2, password, 32);
		trap_Argv(3, email, 32);

		JKG_GLUI_Task_RegisterUser(username, password, email, registerFinalFunc);
		return qtrue;
	}

	if ( Q_stricmp (cmd, "ui_testlogin") == 0 ) {
		char username[32];
		char password[32];

		trap_Argv(1, username, 32);
		trap_Argv(2, password, 32);

		JKG_GLUI_Task_Login(username, password, loginFinalFunc);
		return qtrue;
	}*/

	
	

	if ( Q_stricmp (cmd, "ui_report") == 0 ) {
		UI_Report();
		return qtrue;
	}
	
	if ( Q_stricmp (cmd, "ui_load") == 0 ) {
		UI_Load();
		return qtrue;
	}

	if ( Q_stricmp (cmd, "ui_openmenu" ) == 0 ) 
	{
		//if ( trap_Cvar_VariableValue ( "developer" ) )
		{
			Menus_CloseAll();
			if (Menus_ActivateByName(UI_Argv(1)))
			{
				trap_Key_SetCatcher( KEYCATCH_UI );
			}
			return qtrue;
		}
	}

	if ( Q_stricmp (cmd, "ui_cache") == 0 ) {
		UI_Cache_f();
		return qtrue;
	}

	if ( Q_stricmp (cmd, "ui_teamOrders") == 0 ) {
		//UI_TeamOrdersMenu_f();
		return qtrue;
	}


#endif // _DEBUG

	return qfalse;
}

/*
=================
UI_Shutdown
=================
*/
void UI_Shutdown( void ) {
}


void UI_DrawNamedPic( float x, float y, float width, float height, const char *picname ) {
	qhandle_t	hShader;

	hShader = trap_R_RegisterShaderNoMip( picname );
	trap_R_DrawStretchPic( x, y, width, height, 0, 0, 1, 1, hShader );
}

void UI_DrawHandlePic( float x, float y, float w, float h, qhandle_t hShader ) {
	float	s0;
	float	s1;
	float	t0;
	float	t1;

	if( w < 0 ) {	// flip about vertical
		w  = -w;
		s0 = 1;
		s1 = 0;
	}
	else {
		s0 = 0;
		s1 = 1;
	}

	if( h < 0 ) {	// flip about horizontal
		h  = -h;
		t0 = 1;
		t1 = 0;
	}
	else {
		t0 = 0;
		t1 = 1;
	}
	
	trap_R_DrawStretchPic( x, y, w, h, s0, t0, s1, t1, hShader );
}

/*
================
UI_FillRect

Coordinates are 640*480 virtual values
=================
*/
void UI_FillRect( float x, float y, float width, float height, const float *color ) {
	trap_R_SetColor( color );

	trap_R_DrawStretchPic( x, y, width, height, 0, 0, 0, 0, uiInfo.uiDC.whiteShader );

	trap_R_SetColor( NULL );
}

void UI_DrawSides(float x, float y, float w, float h) {
	float size = 1 / uiInfo.uiDC.xscale;
	trap_R_DrawStretchPic( x, y, size, h, 0, 0, 0, 0, uiInfo.uiDC.whiteShader );
	trap_R_DrawStretchPic( x + w - size, y, size, h, 0, 0, 0, 0, uiInfo.uiDC.whiteShader );
}

void UI_DrawTopBottom(float x, float y, float w, float h) {
	float size = 1 / uiInfo.uiDC.yscale;
	trap_R_DrawStretchPic( x, y, w, size, 0, 0, 0, 0, uiInfo.uiDC.whiteShader );
	trap_R_DrawStretchPic( x, y + h - size, w, size, 0, 0, 0, 0, uiInfo.uiDC.whiteShader );
}
/*
================
UI_DrawRect

Coordinates are 640*480 virtual values
=================
*/
void UI_DrawRect( float x, float y, float width, float height, const float *color ) {
	trap_R_SetColor( color );

  UI_DrawTopBottom(x, y, width, height);
  UI_DrawSides(x, y, width, height);

	trap_R_SetColor( NULL );
}

void UI_SetColor( const float *rgba ) {
	trap_R_SetColor( rgba );
}

void UI_UpdateScreen( void ) {
	trap_UpdateScreen();
}


void UI_DrawTextBox (int x, int y, int width, int lines)
{
	UI_FillRect( x + BIGCHAR_WIDTH/2, y + BIGCHAR_HEIGHT/2, ( width + 1 ) * BIGCHAR_WIDTH, ( lines + 1 ) * BIGCHAR_HEIGHT, colorBlack );
	UI_DrawRect( x + BIGCHAR_WIDTH/2, y + BIGCHAR_HEIGHT/2, ( width + 1 ) * BIGCHAR_WIDTH, ( lines + 1 ) * BIGCHAR_HEIGHT, colorWhite );
}

qboolean UI_CursorInRect (int x, int y, int width, int height)
{
	if (uiInfo.uiDC.cursorx < x ||
		uiInfo.uiDC.cursory < y ||
		uiInfo.uiDC.cursorx > x+width ||
		uiInfo.uiDC.cursory > y+height)
		return qfalse;

	return qtrue;
}
