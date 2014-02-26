cmake_minimum_required(VERSION 2.8.8)

# Make sure the user is not executing this script directly
if(NOT InOpenJK)
	message(FATAL_ERROR "Use the top-level cmake script!")
endif(NOT InOpenJK)

set(MPCGameIncludeDirectories "${MPDir}" "${MPDir}/libraries")
if(WIN32)
	set(MPCGameLibraries "odbc32" "odbccp32") # what are these even?
endif(WIN32)
set(MPCGameDefines ${MPSharedDefines} "CGAME" "MISSIONPACK" )
set(MPCGameGameFiles
	"${MPDir}/game/animtable.cpp"
	"${MPDir}/game/bg_ammo.cpp"
	"${MPDir}/game/bg_constants.cpp"
	"${MPDir}/game/bg_g2_utils.cpp"
	"${MPDir}/game/bg_items.cpp"
	"${MPDir}/game/bg_misc.cpp"
	"${MPDir}/game/bg_panimate.cpp"
	"${MPDir}/game/bg_pmove.cpp"
	"${MPDir}/game/bg_saber.cpp"
	"${MPDir}/game/bg_saberLoad.cpp"
	"${MPDir}/game/bg_saga.h"
	"${MPDir}/game/bg_slidemove.cpp"
	"${MPDir}/game/bg_vehicleLoad.cpp"
	"${MPDir}/game/bg_vmove.cpp"
	"${MPDir}/game/bg_weapons.cpp"
	"${MPDir}/game/bg_weapons_load.cpp"
	"${MPDir}/game/NPC_Animal.cpp"
	"${MPDir}/game/NPC_Fighter.cpp"
	"${MPDir}/game/NPC_Speeder.cpp"
	"${MPDir}/game/NPC_Walker.cpp"
	"${MPDir}/game/jkg_gangwars.cpp"
	"${MPDir}/game/anims.h"
	"${MPDir}/game/bg_ammo.h"
	"${MPDir}/game/bg_items.h"
	"${MPDir}/game/bg_local.h"
	"${MPDir}/game/bg_public.h"
	"${MPDir}/game/bg_strap.h"
	"${MPDir}/game/bg_weapons.h"
	"${MPDir}/game/surfaceflags.h"
	"${MPDir}/game/z_global_defines.h"
	)
source_group("game" FILES ${MPCGameGameFiles})
set(MPCGameFiles ${MPCGameFiles} ${MPCGameGameFiles})

set(MPCGameCgameFiles
	"${MPDir}/cgame/cg_cinematics.cpp"
	"${MPDir}/cgame/cg_consolecmds.cpp"
	"${MPDir}/cgame/cg_crossover.cpp"
	"${MPDir}/cgame/cg_draw.cpp"
	"${MPDir}/cgame/cg_drawtools.cpp"
	"${MPDir}/cgame/cg_effects.cpp"
	"${MPDir}/cgame/cg_ents.cpp"
	"${MPDir}/cgame/cg_event.cpp"
	"${MPDir}/cgame/cg_info.cpp"
	"${MPDir}/cgame/cg_light.cpp"
	"${MPDir}/cgame/cg_localents.cpp"
	"${MPDir}/cgame/cg_main.cpp"
	"${MPDir}/cgame/cg_marks.cpp"
	"${MPDir}/cgame/cg_newDraw.cpp"
	"${MPDir}/cgame/cg_players.cpp"
	"${MPDir}/cgame/cg_playerstate.cpp"
	"${MPDir}/cgame/cg_predict.cpp"
	"${MPDir}/cgame/cg_scoreboard.cpp"
	"${MPDir}/cgame/cg_servercmds.cpp"
	"${MPDir}/cgame/cg_snapshot.cpp"
	"${MPDir}/cgame/cg_spawn.cpp"
	"${MPDir}/cgame/cg_strap.cpp"
	"${MPDir}/cgame/cg_syscalls.cpp"
	"${MPDir}/cgame/cg_turret.cpp"
	"${MPDir}/cgame/cg_view.cpp"
	"${MPDir}/cgame/cg_weaponinit.cpp"
	"${MPDir}/cgame/cg_weapons.cpp"
	"${MPDir}/cgame/fx_blaster.cpp"
	"${MPDir}/cgame/fx_bowcaster.cpp"
	"${MPDir}/cgame/fx_bryarpistol.cpp"
	"${MPDir}/cgame/fx_demp2.cpp"
	"${MPDir}/cgame/fx_disruptor.cpp"
	"${MPDir}/cgame/fx_flechette.cpp"
	"${MPDir}/cgame/fx_force.cpp"
	"${MPDir}/cgame/fx_heavyrepeater.cpp"
	"${MPDir}/cgame/fx_rocketlauncher.cpp"
	"${MPDir}/cgame/jkg_autowaypoint.cpp"
	"${MPDir}/cgame/jkg_cg_damagetypes.cpp"
	"${MPDir}/cgame/jkg_cg_items.cpp"
	"${MPDir}/cgame/jkg_cg_weaponholster.cpp"
	"${MPDir}/cgame/jkg_chatbox.cpp"
	"${MPDir}/cgame/jkg_chatcmds.cpp"
	"${MPDir}/cgame/jkg_cinbuild.cpp"
	"${MPDir}/cgame/jkg_custombounds.cpp"
	"${MPDir}/cgame/jkg_hud.cpp"
	"${MPDir}/cgame/jkg_minimaps.cpp"
	"${MPDir}/cgame/jkg_splinepaths.cpp"
	"${MPDir}/cgame/jkg_wpindicators.cpp"
	"${MPDir}/cgame/animtable.h"
	"${MPDir}/cgame/cg_lights.h"
	"${MPDir}/cgame/cg_local.h"
	"${MPDir}/cgame/cg_public.h"
	"${MPDir}/cgame/cg_weapons.h"
	"${MPDir}/cgame/cg_xcvar.h"
	"${MPDir}/cgame/fx_local.h"
	"${MPDir}/cgame/holocronicons.h"
	"${MPDir}/cgame/jkg_cg_auxlib.h"
	"${MPDir}/cgame/jkg_cg_damagetypes.h"
	"${MPDir}/cgame/jkg_cg_items.h"
	"${MPDir}/cgame/jkg_chatcmds.h"
	"${MPDir}/cgame/jkg_eshader.h"
	"${MPDir}/cgame/jkg_hud.h"
	"${MPDir}/cgame/jkg_navmesh_visualiser.h"
	"${MPDir}/cgame/jkg_patcher.h"
	"${CMAKE_SOURCE_DIR}/JKGalaxies/ui/menudef.h" #Fixme: Don't like the way JKGalaxies folder is included. 
	)
source_group("cgame" FILES ${MPCGameCgameFiles})
set(MPCGameFiles ${MPCGameFiles} ${MPCGameCgameFiles})

set(MPCGameCommonFiles
	"${MPDir}/qcommon/q_math.cpp"
	"${MPDir}/qcommon/q_shared.cpp"
	"${MPDir}/qcommon/disablewarnings.h"
	"${MPDir}/qcommon/q_shared.h"
	"${MPDir}/qcommon/qfiles.h"
	"${MPDir}/qcommon/tags.h"
	)
source_group("common" FILES ${MPCGameCommonFiles})
set(MPCGameFiles ${MPCGameFiles} ${MPCGameCommonFiles})

set(MPCGameUiFiles
	"${MPDir}/ui/ui_shared.cpp"
	"${MPDir}/ui/keycodes.h"
	"${MPDir}/ui/menudef.h"
	"${MPDir}/ui/ui_shared.h"
	)
source_group("ui" FILES ${MPCGameUiFiles})
set(MPCGameFiles ${MPCGameFiles} ${MPCGameUiFiles})

set(MPCGameRendererFiles
	"${MPDir}/rd-common/tr_types.h"
	)
source_group("rd-common" FILES ${MPCGameRendererFiles})
set(MPCGameFiles ${MPCGameFiles} ${MPCGameRendererFiles})

set(MPCGameGFiles
	"${MPDir}/ghoul2/G2.h"
	)
source_group("ghoul2" FILES ${MPCGameG2Files})
set(MPCGameFiles ${MPCGameFiles} ${MPCGameG2Files})

set(MPCGameEncodingLibrary
	"${MPDir}/libraries/encoding/base128.cpp"
	"${MPDir}/libraries/encoding/base128.h"
	"${MPDir}/libraries/encoding/bitstream.c"
	"${MPDir}/libraries/encoding/bitstream.h"
	)
source_group("Libraries\\Encoding" FILES ${MPCGameEncodingLibrary})
set(MPCGameFiles ${MPCGameFiles} ${MPCGameEncodingLibrary})

set(MPCGameJSONLibrary
	"${MPDir}/libraries/json/cJSON.cpp"
	"${MPDir}/libraries/json/cJSON.h"
	)
source_group("Libraries\\JSON" FILES ${MPCGameJSONLibrary})
set(MPCGameFiles ${MPCGameFiles} ${MPCGameJSONLibrary})
	
add_library(${MPCGame} SHARED ${MPCGameFiles})

if(NOT WIN32)
	# remove "lib" prefix for .so/.dylib files
	set_target_properties(${MPCGame} PROPERTIES PREFIX "")
endif(NOT WIN32)

if(WIN32)
	install(TARGETS ${MPCGame} RUNTIME DESTINATION "JKGalaxies")
else(WIN32)
	install(TARGETS ${MPCGame} LIBRARY DESTINATION "JKGalaxies")
endif(WIN32)
set_target_properties(${MPCGame} PROPERTIES COMPILE_DEFINITIONS_RELWITHDEBINFO "${MPCGameDefines};${ReleaseDefines}")
set_target_properties(${MPCGame} PROPERTIES COMPILE_DEFINITIONS_MINSIZEREL "${MPCGameDefines};${ReleaseDefines}")
set_target_properties(${MPCGame} PROPERTIES COMPILE_DEFINITIONS_RELEASE "${MPCGameDefines};${ReleaseDefines}")
set_target_properties(${MPCGame} PROPERTIES COMPILE_DEFINITIONS_DEBUG "${MPCGameDefines};${DebugDefines}")
set_target_properties(${MPCGame} PROPERTIES INCLUDE_DIRECTORIES "${MPCGameIncludeDirectories}")
set_target_properties(${MPCGame} PROPERTIES PROJECT_LABEL "Client Side Module (cgame)")
#set_target_properties(${MPCGame} PROPERTIES LINK_FLAGS "/NODEFAULTLIB:MSVCRT")
# no libraries used
if(MPCGameLibraries)
	target_link_libraries(${MPCGame} ${MPCGameLibraries})
endif(MPCGameLibraries)
