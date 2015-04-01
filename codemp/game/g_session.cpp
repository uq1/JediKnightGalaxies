/*
===========================================================================
Copyright (C) 1999 - 2005, Id Software, Inc.
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
Copyright (C) 2005 - 2015, ioquake3 contributors
Copyright (C) 2013 - 2015, OpenJK contributors

This file is part of the OpenJK source code.

OpenJK is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, see <http://www.gnu.org/licenses/>.
===========================================================================
*/

#include "g_local.h"

/*
=======================================================================

  SESSION DATA

Session data is the only data that stays persistant across level loads
and tournament restarts.
=======================================================================
*/

// Jedi Knight Galaxies
// Alrighty, we're recoding this system completely.
// Instead of using cvars (lol wtf), we're gonna save the session data into a allocation :o
clientSession_t *g_sess; //[MAX_CLIENTS] // Store session data in here

/*
================
G_WriteClientSessionData

Called on game shutdown
================
*/
void G_WriteClientSessionData( gclient_t *client ) {
	
	g_sess[client - level.clients] = client->sess;
	
	/*
	const char	*s;
	const char	*var;
	int			i = 0;
	char		siegeClass[64];
	char		saberType[64];
	char		saber2Type[64];

	strcpy(siegeClass, client->sess.siegeClass);

	while (siegeClass[i])
	{ //sort of a hack.. we don't want spaces by siege class names have spaces so convert them all to unused chars
		if (siegeClass[i] == ' ')
		{
			siegeClass[i] = 1;
		}

		i++;
	}

	if (!siegeClass[0])
	{ //make sure there's at least something
		strcpy(siegeClass, "none");
	}

	//Do the same for the saber
	strcpy(saberType, client->sess.saberType);

	i = 0;
	while (saberType[i])
	{
		if (saberType[i] == ' ')
		{
			saberType[i] = 1;
		}

		i++;
	}

	strcpy(saber2Type, client->sess.saber2Type);

	i = 0;
	while (saber2Type[i])
	{
		if (saber2Type[i] == ' ')
		{
			saber2Type[i] = 1;
		}

		i++;
	}

	s = va("%i %i %i %i %i %i %i %i %i %i %i %i %s %s %s %s", 
		client->sess.sessionTeam,
		client->sess.spectatorTime,
		client->sess.spectatorState,
		client->sess.spectatorClient,
		client->sess.wins,
		client->sess.losses,
		client->sess.teamLeader,
		client->sess.setForce,
		client->sess.saberLevel,
		client->sess.selectedFP,
		client->sess.duelTeam,
		client->sess.siegeDesiredTeam,
		siegeClass,
		saberType,
		saber2Type,
		//Jedi Knight Galaxies
		client->sess.IP
		);

	var = va( "session%i", client - level.clients );

	trap->Cvar_Set( var, s );
	*/

}

/*
================
G_ReadSessionData

Called on a reconnect
================
*/
void G_ReadSessionData( gclient_t *client ) {
	client->sess = g_sess[client - level.clients];
	/*
	char	s[MAX_STRING_CHARS];
	const char	*var;
	int			i = 0;

	// bk001205 - format
	int teamLeader;
	int spectatorState;
	int sessionTeam;

	var = va( "session%i", client - level.clients );
	trap->Cvar_VariableStringBuffer( var, s, sizeof(s) );

	sscanf( s, "%i %i %i %i %i %i %i %i %i %i %i %i %s %s %s %s",
		&sessionTeam,                 // bk010221 - format
		&client->sess.spectatorTime,
		&spectatorState,              // bk010221 - format
		&client->sess.spectatorClient,
		&client->sess.wins,
		&client->sess.losses,
		&teamLeader,                   // bk010221 - format
		&client->sess.setForce,
		&client->sess.saberLevel,
		&client->sess.selectedFP,
		&client->sess.duelTeam,
		&client->sess.siegeDesiredTeam,
		&client->sess.siegeClass,
		&client->sess.saberType,
		&client->sess.saber2Type,
		//Jedi Knight Galaxies
		&client->sess.IP
		);

	while (client->sess.siegeClass[i])
	{ //convert back to spaces from unused chars, as session data is written that way.
		if (client->sess.siegeClass[i] == 1)
		{
			client->sess.siegeClass[i] = ' ';
		}

		i++;
	}

	i = 0;
	//And do the same for the saber type
	while (client->sess.saberType[i])
	{
		if (client->sess.saberType[i] == 1)
		{
			client->sess.saberType[i] = ' ';
		}

		i++;
	}

	i = 0;
	while (client->sess.saber2Type[i])
	{
		if (client->sess.saber2Type[i] == 1)
		{
			client->sess.saber2Type[i] = ' ';
		}

		i++;
	}

	// bk001205 - format issues
	client->sess.sessionTeam = (team_t)sessionTeam;
	client->sess.spectatorState = (spectatorState_t)spectatorState;
	client->sess.teamLeader = (qboolean)teamLeader;
	*/

	client->ps.fd.saberAnimLevel = client->sess.saberLevel;
	client->ps.fd.saberDrawAnimLevel = client->sess.saberLevel;
	client->ps.fd.forcePowerSelected = client->sess.selectedFP;
}


/*
================
G_InitSessionData

Called on a first-time connect
================
*/
void G_InitSessionData( gclient_t *client, char *userinfo, qboolean isBot ) {
	clientSession_t	*sess;
	const char		*value;

	sess = &client->sess;

	client->sess.siegeDesiredTeam = TEAM_FREE;

	// initial team determination
	if ( level.gametype >= GT_TEAM ) {
		if ( g_teamAutoJoin.integer && !(g_entities[client-level.clients].r.svFlags & SVF_BOT) ) {
			sess->sessionTeam = PickTeam( -1 );
			client->ps.fd.forceDoInit = 1; //every time we change teams make sure our force powers are set right
		} else {
			// always spawn as spectator in team games
			if (!isBot)
			{
				sess->sessionTeam = TEAM_SPECTATOR;	
			}
			else
			{ //Bots choose their team on creation
				value = Info_ValueForKey( userinfo, "team" );
				if (value[0] == 'r' || value[0] == 'R')
				{
					sess->sessionTeam = TEAM_RED;
				}
				else if (value[0] == 'b' || value[0] == 'B')
				{
					sess->sessionTeam = TEAM_BLUE;
				}
				else
				{
					sess->sessionTeam = PickTeam( -1 );
				}
				client->ps.fd.forceDoInit = 1; //every time we change teams make sure our force powers are set right
			}
		}
	} else {
		value = Info_ValueForKey( userinfo, "team" );
		if ( value[0] == 's' ) {
			// a willing spectator, not a waiting-in-line
			sess->sessionTeam = TEAM_SPECTATOR;
		} else {
			switch ( level.gametype ) {
			default:
			case GT_FFA:
			case GT_SINGLE_PLAYER:
				if ( g_maxGameClients.integer > 0 && 
					level.numNonSpectatorClients >= g_maxGameClients.integer ) {
					sess->sessionTeam = TEAM_SPECTATOR;
				} else {
					sess->sessionTeam = TEAM_FREE;
				}
				break;
			case GT_DUEL:
				// if the game is full, go into a waiting mode
				if ( level.numNonSpectatorClients >= 2 ) {
					sess->sessionTeam = TEAM_SPECTATOR;
				} else {
					sess->sessionTeam = TEAM_FREE;
				}
				break;
			case GT_POWERDUEL:
				//sess->duelTeam = DUELTEAM_LONE; //default
				{
					int loners = 0;
					int doubles = 0;

					G_PowerDuelCount(&loners, &doubles, qtrue);

					if (!doubles || loners > (doubles/2))
					{
						sess->duelTeam = DUELTEAM_DOUBLE;
					}
					else
					{
						sess->duelTeam = DUELTEAM_LONE;
					}
				}
				sess->sessionTeam = TEAM_SPECTATOR;
				break;
			}
		}
	}

	sess->spectatorState = SPECTATOR_FREE;
	AddTournamentQueue(client);

	sess->siegeClass[0] = 0;

	G_WriteClientSessionData( client );
}


/*
==================
G_InitWorldSession

==================
*/
void G_InitWorldSession( void ) {
	char	tmp[MAX_STRING_CHARS];
	char	s[MAX_STRING_CHARS];
	int			gt, ptr;

	trap->Cvar_VariableStringBuffer( "session", s, sizeof(s) );
	gt = atoi( s );

	// if the gametype changed since the last session, don't use any
	// client sessions
	if ( level.gametype != gt ) {
		level.newSession = qtrue;
		trap->Print( "Gametype changed, clearing session data.\n" );
	}
	
	trap->Cvar_VariableStringBuffer("gsess", tmp, sizeof(tmp));
	ptr = atoi( tmp );
	if (!tmp[0]) {
		// Cvar doesn't exist, first startup
		memset(&g_sess, 0, sizeof(g_sess));
		trap->TrueMalloc((void**)&g_sess, sizeof(clientSession_t) * MAX_CLIENTS);
		trap->Cvar_Set("gsess", va("%i", (int)g_sess));
	} else {
		g_sess = (clientSession_t *)ptr;
	}
}

/*
==================
G_WriteSessionData

==================
*/
void G_WriteSessionData( void ) {
	int		i;

	trap->Cvar_Set( "session", va("%i", level.gametype) );

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( level.clients[i].pers.connected == CON_CONNECTED ) {
			G_WriteClientSessionData( &level.clients[i] );
		}
	}
}
