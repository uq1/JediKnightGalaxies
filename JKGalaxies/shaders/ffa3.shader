// ///////////////////////////////////

// //// World ////////////////////////

// ///////////////////////////////////

textures/jkg_tatooine/sky
{
	qer_editorimage	textures/skies/sky
	surfaceparm	sky
	surfaceparm	noimpact
	surfaceparm	nomarks
	surfaceparm	nodlight
	q3map_SunExt 1 .7 .55 220 80 54 1 10
	q3map_SunExt 1 .9 .9 250 65 62 1 10
	q3map_nofog
	notc
	q3map_nolightmap
	skyParms	textures/skies/desert 1024 -
}

textures/jkg_tatooine/fog
{
	qer_editorimage	textures/fogs/fog.tga
	surfaceparm	nonsolid
	surfaceparm	nonopaque
	surfaceparm	fog
	surfaceparm	trans
	q3map_nolightmap
	fogparms	( 0.75 0.65 0.5 ) 9000.0
}

textures/jkg_tatooine/jundland_fog
{
	qer_editorimage	textures/fogs/fog.tga
	surfaceparm	nonsolid
	surfaceparm	nonopaque
	surfaceparm	fog
	surfaceparm	trans
	q3map_nolightmap
	fogparms	( 0.75 0.65 0.5 ) 55300.0
}

// ///////////////////////////////////

// //// Decals ///////////////////////

// ///////////////////////////////////

textures/jkg_tatooine/dmg1
{
	qer_editorimage	textures/jkg_tatooine/dmg1
	qer_trans	0.8
	surfaceparm	nomarks
	surfaceparm	trans
	polygonOffset
	sort	6
    {
        map textures/jkg_tatooine/dmg1
        alphaFunc GE128
    }
    {
        map $lightmap
        blendFunc GL_DST_COLOR GL_ZERO
        depthFunc equal
    }
}

textures/jkg_tatooine/dmg2
{
	qer_editorimage	textures/jkg_tatooine/dmg2
	qer_trans	0.8
	surfaceparm	nomarks
	surfaceparm	trans
	polygonOffset
	sort	6
    {
        map textures/jkg_tatooine/dmg2
        alphaFunc GE128
    }
    {
        map $lightmap
        blendFunc GL_DST_COLOR GL_ZERO
        depthFunc equal
    }
}

textures/jkg_tatooine/dmg3
{
	qer_editorimage	textures/jkg_tatooine/dmg3
	qer_trans	0.8
	surfaceparm	nomarks
	surfaceparm	trans
	polygonOffset
	sort	6
    {
        map textures/jkg_tatooine/dmg3
        alphaFunc GE128
    }
    {
        map $lightmap
        blendFunc GL_DST_COLOR GL_ZERO
        depthFunc equal
    }
}

textures/jkg_tatooine/dmg4
{
	qer_editorimage	textures/jkg_tatooine/dmg4
	qer_trans	0.8
	surfaceparm	nomarks
	surfaceparm	trans
	polygonOffset
	sort	6
    {
        map textures/jkg_tatooine/dmg4
        alphaFunc GE128
    }
    {
        map $lightmap
        blendFunc GL_DST_COLOR GL_ZERO
        depthFunc equal
    }
}

textures/jkg_tatooine/window
{
	qer_editorimage	textures/jkg_tatooine/window
	qer_trans	0.8
	surfaceparm	trans
	polygonOffset
	sort	6
    {
        map textures/jkg_tatooine/window
        alphaFunc GE128
    }
    {
        map $lightmap
        blendFunc GL_DST_COLOR GL_ZERO
        depthFunc equal
    }
}

textures/jkg_tatooine/grate
{
	qer_editorimage	textures/jkg_tatooine/grate
	qer_trans	0.8
	q3map_alphashadow
    {
        map textures/jkg_tatooine/grate
        alphaFunc GE128
        blendFunc GL_ONE GL_ZERO
        depthWrite
        rgbGen identity
    }
    {
        map $lightmap
        blendFunc GL_DST_COLOR GL_ZERO
        depthFunc equal
        rgbGen identity
    }
}

// ///////////////////////////////////

// //// Lights ///////////////////////

// ///////////////////////////////////

textures/jkg_tatooine/under_grate
{
	qer_editorimage	textures/jkg_tatooine/under_grate
	q3map_surfacelight	400
	q3map_lightRGB .96 .92 .55
	q3map_nolightmap
    {
        map textures/jkg_tatooine/under_grate
    }
}

// ///////////////////////////////////

// //// Walls ////////////////////////

// ///////////////////////////////////

textures/jkg_tatooine/chrome
{
	qer_editorimage	textures/jkg_tatooine/metal1
	qer_trans	0.8
	surfaceparm	nomarks
	surfaceparm	trans
	polygonOffset
    {
        map textures/colors/white
        rgbGen const ( 0.400000 0.400000 0.400000 )
    }
    {
        map textures/common/env_chrome
        blendFunc GL_ONE GL_ONE
        tcGen environment
    }
    {
        map $lightmap
        blendFunc GL_DST_COLOR GL_ZERO
    }
}

textures/jkg_tatooine/mudwall1_outside
{
	q3map_shadeangle	45
	qer_editorimage	textures/jkg_tatooine/mudwall1
	q3map_lightmapsamplesize	8
    {
        map textures/jkg_tatooine/mudwall1
    }
    {
        map $lightmap
        blendFunc GL_DST_COLOR GL_ZERO
    }
}

textures/jkg_tatooine/mudwall1_high_angle
{
	q3map_shadeangle	150
	qer_editorimage	textures/jkg_tatooine/mudwall1
	q3map_lightmapsamplesize	8
    {
        map textures/jkg_tatooine/mudwall1
    }
    {
        map $lightmap
        blendFunc GL_DST_COLOR GL_ZERO
    }
}

textures/jkg_tatooine/mudwall1_base
{
	q3map_shadeangle	45
	qer_editorimage	textures/jkg_tatooine/mudwall1_base
    {
        map textures/jkg_tatooine/mudwall1_base
    }
    {
        map $lightmap
        blendFunc GL_DST_COLOR GL_ZERO
    }
}

textures/jkg_tatooine/metal1
{
	q3map_shadeangle	45
	qer_editorimage	textures/jkg_tatooine/metal1
	q3map_material	HollowMetal
    {
        map textures/jkg_tatooine/metal1
    }
    {
        map textures/jkg_tatooine/metal1_spec
        blendFunc GL_SRC_ALPHA GL_ONE
        alphaGen lightingSpecular
    }
    {
        map $lightmap
        blendFunc GL_DST_COLOR GL_ZERO
    }
}

textures/jkg_tatooine/vent_s
{
	q3map_Offset 0.25
    {
        map textures/jkg_tatooine/vent_s
    }
    {
        map $lightmap
        blendFunc GL_DST_COLOR GL_ZERO
    }
}

textures/jkg_tatooine/vent_l
{
	q3map_Offset 0.25
    {
        map textures/jkg_tatooine/vent_s
    }
    {
        map $lightmap
        blendFunc GL_DST_COLOR GL_ZERO
    }
}

textures/jkg_tatooine/sand1
{
	q3map_shadeangle	120
	qer_editorimage	textures/jkg_tatooine/sand1
	q3map_material	Sand
    {
        map textures/jkg_tatooine/sand1
    }
    {
        map $lightmap
        blendFunc GL_DST_COLOR GL_ZERO
    }
}

textures/jkg_tatooine/sand_to_steps
{
// q3map_lightmapMergable

	q3map_shadeangle	45
	qer_editorimage	textures/jkg_tatooine/sand1
	q3map_lightmapsamplesize	8
	q3map_material	Sand
    {
        map textures/jkg_tatooine/sand_trodden
        rgbGen identity
    }
    {
// rgbGen identity

        map textures/jkg_tatooine/sand1
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        alphaGen vertex
    }
    {
// rgbGen identity

        map $lightmap
        blendFunc GL_DST_COLOR GL_ZERO
    }
}

models/spaceport/spaceport_objects
{
	q3map_shadeangle	45
    {
        map models/spaceport/spaceport_objects
        blendFunc GL_ONE GL_ZERO
        rgbGen identity
    }
    {
        map $lightmap
        blendFunc GL_DST_COLOR GL_ZERO
        depthWrite
    }
}

models/spaceport/main_building
{
	q3map_shadeangle	45
	q3map_lightmapsamplesize	8
    {
        map textures/jkg_tatooine/mudwall1
        tcMod scale 10 10
    }
    {
        map $lightmap
        blendFunc GL_DST_COLOR GL_ZERO
        depthWrite
    }
}

models/spaceport/house_building
{
	q3map_shadeangle	45
	q3map_lightmapsamplesize	8
    {
        map textures/jkg_tatooine/mudwall1
        tcMod scale 10 10
    }
    {
        map $lightmap
        blendFunc GL_DST_COLOR GL_ZERO
        depthWrite
    }
}

models/spaceport/arch_building
{
	q3map_shadeangle	45
	q3map_lightmapsamplesize	8
    {
        map textures/jkg_tatooine/mudwall1
        tcMod scale 10 10
    }
    {
        map $lightmap
        blendFunc GL_DST_COLOR GL_ZERO
        depthWrite
    }
}

models/spaceport/stair_building
{
	q3map_shadeangle	45
	q3map_lightmapsamplesize	8
    {
        map textures/jkg_tatooine/mudwall1
        tcMod scale 10 10
    }
    {
        map $lightmap
        blendFunc GL_DST_COLOR GL_ZERO
        depthWrite
    }
}

models/map_objects/desert/vent_grime
{
	surfaceparm	nomarks
	surfaceparm	trans
	q3map_nolightmap
	q3map_alphashadow
    {
        map models/map_objects/desert/vent_grime
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        rgbGen vertex
    }
}

