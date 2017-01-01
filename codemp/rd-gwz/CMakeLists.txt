#============================================================================
# Copyright (C) 2013 - 2015, OpenJK contributors
# 
# This file is part of the OpenJK source code.
# 
# OpenJK is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, see <http://www.gnu.org/licenses/>.
#============================================================================

cmake_minimum_required(VERSION 2.8.8)

# Make sure the user is not executing this script directly
if(NOT InOpenJK)
	message(FATAL_ERROR "Use the top-level cmake script!")
endif(NOT InOpenJK)


# Check for dependencies first

set(MPWarzoneRendererIncludeDirectories ${MPDir})
set(MPWarzoneRendererIncludeDirectories
	${MPWarzoneRendererIncludeDirectories}
	${SharedDir}
	${MPDir}
	"${MPDir}/rd-gwz"
	)

set(MPWarzoneRendererFiles
  "${MPDir}/client/fast_mutex.h"
	"${MPDir}/client/tinythread.cpp"
	"${MPDir}/client/tinythread.h"
	"${MPDir}/qcommon/inifile.h"
	"${MPDir}/qcommon/inifile.cpp"
	"${MPDir}/rd-gwz/iqm.h"
	"${MPDir}/rd-gwz/qgl.h"
	"${MPDir}/rd-gwz/glext.h"
	"${MPDir}/rd-gwz/G2_API.cpp"
	"${MPDir}/rd-gwz/G2_bolts.cpp"
	"${MPDir}/rd-gwz/G2_bones.cpp"
	"${MPDir}/rd-gwz/G2_misc.cpp"
	"${MPDir}/rd-gwz/G2_surfaces.cpp"
	"${MPDir}/rd-gwz/tr_animation.cpp"
	"${MPDir}/rd-gwz/tr_backend.cpp"
	"${MPDir}/rd-gwz/tr_bsp.cpp"
	"${MPDir}/rd-gwz/tr_cache.cpp"
	"${MPDir}/rd-gwz/tr_cmds.cpp"
	"${MPDir}/rd-gwz/tr_curve.cpp"
	"${MPDir}/rd-gwz/tr_decals.cpp"
	"${MPDir}/rd-gwz/tr_extensions.cpp"
	"${MPDir}/rd-gwz/tr_extramath.cpp"
	"${MPDir}/rd-gwz/tr_extramath.h"
	"${MPDir}/rd-gwz/tr_extratypes.h"
	"${MPDir}/rd-gwz/tr_fbo.cpp"
	"${MPDir}/rd-gwz/tr_fbo.h"
	"${MPDir}/rd-gwz/tr_flares.cpp"
	"${MPDir}/rd-gwz/tr_ghoul2.cpp"
	"${MPDir}/rd-gwz/tr_glsl.cpp"
	"${MPDir}/rd-gwz/tr_image.cpp"
	"${MPDir}/rd-gwz/tr_init.cpp"
	"${MPDir}/rd-gwz/tr_light.cpp"
	"${MPDir}/rd-gwz/tr_local.h"
	"${MPDir}/rd-gwz/tr_main.cpp"
	"${MPDir}/rd-gwz/tr_marks.cpp"
	"${MPDir}/rd-gwz/tr_mapping.cpp"
	"${MPDir}/rd-gwz/tr_mesh.cpp"
	"${MPDir}/rd-gwz/tr_model.cpp"
	"${MPDir}/rd-gwz/tr_model_iqm.cpp"
	"${MPDir}/rd-gwz/tr_occlusion.cpp"
	"${MPDir}/rd-gwz/tr_ocean.cpp"
	"${MPDir}/rd-gwz/tr_ocean.h"
	"${MPDir}/rd-gwz/tr_postprocess.cpp"
	"${MPDir}/rd-gwz/tr_postprocess.h"
	"${MPDir}/rd-gwz/tr_scene.cpp"
	"${MPDir}/rd-gwz/tr_shade.cpp"
	"${MPDir}/rd-gwz/tr_shade_calc.cpp"
	"${MPDir}/rd-gwz/tr_shader.cpp"
	"${MPDir}/rd-gwz/tr_shadows.cpp"
	"${MPDir}/rd-gwz/tr_skin.cpp"
	"${MPDir}/rd-gwz/tr_sky.cpp"
	"${MPDir}/rd-gwz/tr_subs.cpp"
	"${MPDir}/rd-gwz/tr_surface.cpp"
	"${MPDir}/rd-gwz/tr_vbo.cpp"
	"${MPDir}/rd-gwz/tr_world.cpp"
	)
source_group("renderer" FILES ${MPWarzoneRendererFiles})

file(GLOB MPWarzoneRendererGLSLFiles "${MPDir}/rd-gwz/glsl/*.glsl")
source_group("renderer\\glsl" FILES ${MPWarzoneRendererGLSLFiles})
set(MPWarzoneRendererFiles ${MPWarzoneRendererFiles} ${MPWarzoneRendererGLSLFiles})

file(GLOB MPWarzoneRendererGLSLFiles2 "${MPDir}/rd-gwz/glsl2/*.glsl")
source_group("renderer\\glsl2" FILES ${MPWarzoneRendererGLSLFiles2})
set(MPWarzoneRendererFiles ${MPWarzoneRendererFiles} ${MPWarzoneRendererGLSLFiles2})

set(MPWarzoneRendererGhoul2Files
	"${MPDir}/rd-gwz/ghoul2/g2_local.h"
	"${MPDir}/rd-gwz/ghoul2/ghoul2_shared.h"
	"${MPDir}/rd-gwz/ghoul2/G2_gore.cpp"
	"${MPDir}/rd-gwz/ghoul2/G2_gore.h")
source_group("ghoul2" FILES ${MPWarzoneRendererGhoul2Files})
set(MPWarzoneRendererFiles ${MPWarzoneRendererFiles} ${MPWarzoneRendererGhoul2Files})

set(MPWarzoneRendererMaskedOcclusionCullingFiles
	"${MPDir}/rd-gwz/MaskedOcclusionCulling/CullingThreadpool.cpp"
	"${MPDir}/rd-gwz/MaskedOcclusionCulling/CullingThreadpool.h"
	"${MPDir}/rd-gwz/MaskedOcclusionCulling/MaskedOcclusionCulling.cpp"
	"${MPDir}/rd-gwz/MaskedOcclusionCulling/MaskedOcclusionCulling.h"
	"${MPDir}/rd-gwz/MaskedOcclusionCulling/MaskedOcclusionCullingAVX2.cpp"
	"${MPDir}/rd-gwz/MaskedOcclusionCulling/MaskedOcclusionCullingCommon.inl"
	"${MPDir}/rd-gwz/MaskedOcclusionCulling/README.md")
source_group("MaskedOcclusionCulling" FILES ${MPWarzoneRendererMaskedOcclusionCullingFiles})
set(MPWarzoneRendererFiles ${MPWarzoneRendererFiles} ${MPWarzoneRendererMaskedOcclusionCullingFiles})

set(MPWarzoneRendererCommonFiles
	"${MPDir}/rd-common/mdx_format.h"
	"${MPDir}/rd-common/tr_common.h"
	"${MPDir}/rd-common/tr_font.cpp"
	"${MPDir}/rd-common/tr_font.h"
	"${MPDir}/rd-common/tr_image_load.cpp"
	"${MPDir}/rd-common/tr_image_jpg.cpp"
	"${MPDir}/rd-common/tr_image_tga.cpp"
	"${MPDir}/rd-common/tr_image_png.cpp"
	"${MPDir}/rd-common/tr_noise.cpp"
	"${MPDir}/rd-common/tr_public.h")
source_group("rd-common" FILES ${MPWarzoneRendererCommonFiles})
set(MPWarzoneRendererFiles ${MPWarzoneRendererFiles} ${MPWarzoneRendererCommonFiles})

set(MPWarzoneRendererQcommonFiles
  "${MPDir}/qcommon/inifile.cpp"
  "${MPDir}/qcommon/inifile.h"
	"${MPDir}/qcommon/GenericParser2.cpp"
	"${MPDir}/qcommon/matcomp.cpp"
	"${MPDir}/qcommon/q_math.cpp"
	"${MPDir}/qcommon/q_shared.cpp")
source_group("qcommon" FILES ${MPWarzoneRendererQcommonFiles})
set(MPWarzoneRendererFiles ${MPWarzoneRendererFiles} ${MPWarzoneRendererQcommonFiles})

# Transparently use either bundled or system libjpeg.
list(APPEND MPWarzoneRendererIncludeDirectories ${JPEG_INCLUDE_DIR})
list(APPEND MPWarzoneRendererLibraries          ${JPEG_LIBRARIES})

# Transparently use either bundled or system libpng.  Order is important --
# libpng used zlib, so it must come before it on the linker command line.
list(APPEND MPWarzoneRendererIncludeDirectories ${PNG_INCLUDE_DIRS})
list(APPEND MPWarzoneRendererLibraries          ${PNG_LIBRARIES})

# Transparently use either bundled or system zlib.
list(APPEND MPWarzoneRendererIncludeDirectories ${ZLIB_INCLUDE_DIR})
list(APPEND MPWarzoneRendererLibraries          ${ZLIB_LIBRARIES})

# Transparently use our bundled minizip.
list(APPEND MPVanillaRendererIncludeDirectories ${MINIZIP_INCLUDE_DIRS})
list(APPEND MPVanillaRendererLibraries          ${MINIZIP_LIBRARIES})

if(NOT WIN32)
	find_package(OpenGL REQUIRED)
	set(MPWarzoneRendererIncludeDirectories ${MPWarzoneRendererIncludeDirectories} ${OPENGL_INCLUDE_DIR})
	set(MPWarzoneRendererLibraries ${MPWarzoneRendererLibraries} ${OPENGL_LIBRARIES})

	find_package(SDL2 REQUIRED)
	set(MPWarzoneRendererIncludeDirectories ${MPWarzoneRendererIncludeDirectories} ${SDL2_INCLUDE_DIR})
	set(MPWarzoneRendererLibraries ${MPWarzoneRendererLibraries} ${SDL2_LIBRARY})
endif(NOT WIN32)


source_group("renderer" FILES ${CMAKE_CURRENT_BINARY_DIR}/glsl_shaders.cpp)
set(MPWarzoneRendererFiles ${MPWarzoneRendererFiles} ${CMAKE_CURRENT_BINARY_DIR}/glsl_shaders.cpp)

source_group("renderer" FILES ${CMAKE_CURRENT_BINARY_DIR}/glsl_shaders2.cpp)
set(MPWarzoneRendererFiles ${MPWarzoneRendererFiles} ${CMAKE_CURRENT_BINARY_DIR}/glsl_shaders2.cpp)

find_package(OpenGL REQUIRED)
set(MPWarzoneRendererIncludeDirectories ${MPWarzoneRendererIncludeDirectories} ${OPENGL_INCLUDE_DIR})
set(MPWarzoneRendererLibraries ${MPWarzoneRendererLibraries} ${OPENGL_LIBRARIES})

set(MPWarzoneRendererIncludeDirectories ${MPWarzoneRendererIncludeDirectories} ${WarzoneLibDir})
add_library(${MPWarzoneRenderer} SHARED ${MPWarzoneRendererFiles})

if(NOT MSVC)
	# remove "lib" prefix for .so/.dylib files
	set_target_properties(${MPWarzoneRenderer} PROPERTIES PREFIX "")
endif()

if(WIN32)
	install(TARGETS ${MPWarzoneRenderer}
		RUNTIME
		DESTINATION ${JKAInstallDir}
		COMPONENT ${JKAMPClientComponent})
else(WIN32)
	if(MakeApplicationBundles AND BuildMPEngine)
		install(TARGETS ${MPWarzoneRenderer}
			LIBRARY
			DESTINATION "${JKAInstallDir}/${MPEngine}.app/Contents/MacOS/"
			COMPONENT ${JKAMPClientComponent})
	else()
		install(TARGETS ${MPWarzoneRenderer}
			LIBRARY
			DESTINATION ${JKAInstallDir}
			COMPONENT ${JKAMPClientComponent})
	endif()
endif()

set_target_properties(${MPWarzoneRenderer} PROPERTIES COMPILE_DEFINITIONS_RELWITHDEBINFO "${SharedDefines};${ReleaseDefines}")
set_target_properties(${MPWarzoneRenderer} PROPERTIES COMPILE_DEFINITIONS_MINSIZEREL "${SharedDefines};${ReleaseDefines}")
set_target_properties(${MPWarzoneRenderer} PROPERTIES COMPILE_DEFINITIONS_RELEASE "${SharedDefines};${ReleaseDefines}")
set_target_properties(${MPWarzoneRenderer} PROPERTIES COMPILE_DEFINITIONS_DEBUG "${SharedDefines};${DebugDefines}")
set_target_properties(${MPWarzoneRenderer} PROPERTIES INCLUDE_DIRECTORIES "${MPWarzoneRendererIncludeDirectories}")
set_target_properties(${MPWarzoneRenderer} PROPERTIES PROJECT_LABEL "MP Warzone Renderer (rd-gwz_x86.dll)")
target_link_libraries(${MPWarzoneRenderer} ${MPWarzoneRendererLibraries})

# GLSL shader file generator
add_executable(compact_glsl ${MPDir}/rd-gwz/glsl/compact.cpp)
add_custom_command(
	OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/glsl_shaders.cpp ${CMAKE_CURRENT_BINARY_DIR}/glsl_shaders2.cpp
	COMMAND compact_glsl ${CMAKE_CURRENT_BINARY_DIR}/glsl_shaders.cpp ${MPWarzoneRendererGLSLFiles}
	COMMAND compact_glsl ${CMAKE_CURRENT_BINARY_DIR}/glsl_shaders2.cpp ${MPWarzoneRendererGLSLFiles2}
	DEPENDS compact_glsl ${MPWarzoneRendererGLSLFiles} ${MPWarzoneRendererGLSLFiles2})
