/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
#include "tr_local.h"
#include "glext.h"

backEndData_t	*backEndData;
backEndState_t	backEnd;


static float	s_flipMatrix[16] = {
	// convert from our coordinate system (looking down X)
	// to OpenGL's coordinate system (looking down -Z)
	0, 0, -1, 0,
	-1, 0, 0, 0,
	0, 1, 0, 0,
	0, 0, 0, 1
};

void RB_SwapFBOs ( FBO_t **currentFbo, FBO_t **currentOutFbo)
{
	FBO_t *temp = *currentFbo;
	*currentFbo = *currentOutFbo;
	*currentOutFbo = temp;
}

/*
** GL_Bind
*/
void GL_Bind( image_t *image ) {
	int texnum;

	if ( !image ) {
		ri->Printf( PRINT_WARNING, "GL_Bind: NULL image\n" );
		texnum = tr.defaultImage->texnum;
	} else {
		texnum = image->texnum;
	}

	if ( r_nobind->integer && tr.dlightImage ) {		// performance evaluation option
		texnum = tr.dlightImage->texnum;
	}

	if ( glState.currenttextures[glState.currenttmu] != texnum ) {
		if ( image ) {
			image->frameUsed = tr.frameCount;
		}
		glState.currenttextures[glState.currenttmu] = texnum;
		if (image && image->flags & IMGFLAG_CUBEMAP)
			qglBindTexture( GL_TEXTURE_CUBE_MAP, texnum );
		else
			qglBindTexture( GL_TEXTURE_2D, texnum );
	}
}

/*
** GL_SelectTexture
*/
void GL_SelectTexture( int unit )
{
	if ( glState.currenttmu == unit )
	{
		return;
	}

	if (!(unit >= 0 && unit <= 31))
		ri->Error( ERR_DROP, "GL_SelectTexture: unit = %i", unit );

	qglActiveTexture(GL_TEXTURE0 + unit);

	glState.currenttmu = unit;
}

/*
** GL_BindToTMU
*/
void GL_BindToTMU( image_t *image, int tmu )
{
	int		texnum;
	int     oldtmu = glState.currenttmu;

	if (!image)
		texnum = 0;
	else
		texnum = image->texnum;

	if ( glState.currenttextures[tmu] != texnum ) {
		GL_SelectTexture( tmu );
		if (image)
			image->frameUsed = tr.frameCount;
		glState.currenttextures[tmu] = texnum;

		if (image && (image->flags & IMGFLAG_CUBEMAP))
			qglBindTexture( GL_TEXTURE_CUBE_MAP, texnum );
		else
			qglBindTexture( GL_TEXTURE_2D, texnum );
		GL_SelectTexture( oldtmu );
	}
}


/*
** GL_Cull
*/
void GL_Cull( int cullType ) {
	if ( glState.faceCulling == cullType ) {
		return;
	}

	glState.faceCulling = cullType;

	if ( backEnd.projection2D )
	{
		return;
	}

	if ( cullType == CT_TWO_SIDED ) 
	{
		qglDisable( GL_CULL_FACE );
	} 
	else 
	{
		qboolean cullFront;
		qglEnable( GL_CULL_FACE );

		cullFront = (qboolean)(cullType == CT_FRONT_SIDED);
		if ( backEnd.viewParms.isMirror )
		{
			cullFront = (qboolean)(!cullFront);
		}

		if ( backEnd.currentEntity && backEnd.currentEntity->mirrored )
		{
			cullFront = (qboolean)(!cullFront);
		}

		qglCullFace( cullFront ? GL_FRONT : GL_BACK );
	}
}

/*
** GL_TexEnv
*/
void GL_TexEnv( int env )
{
	if ( env == glState.texEnv[glState.currenttmu] )
	{
		return;
	}

	glState.texEnv[glState.currenttmu] = env;


	switch ( env )
	{
	case GL_MODULATE:
		qglTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
		break;
	case GL_REPLACE:
		qglTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE );
		break;
	case GL_DECAL:
		qglTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL );
		break;
	case GL_ADD:
		qglTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_ADD );
		break;
	default:
		ri->Error( ERR_DROP, "GL_TexEnv: invalid env '%d' passed", env );
		break;
	}
}

/*
** GL_State
**
** This routine is responsible for setting the most commonly changed state
** in Q3.
*/
void GL_State( uint32_t stateBits )
{
	uint32_t diff = stateBits ^ glState.glStateBits;

	if ( !diff )
	{
		return;
	}

	//
	// check depthFunc bits
	//
	if ( diff & GLS_DEPTHFUNC_BITS )
	{
		if ( stateBits & GLS_DEPTHFUNC_EQUAL )
		{
			qglDepthFunc( GL_EQUAL );
		}
		else if ( stateBits & GLS_DEPTHFUNC_GREATER)
		{
			qglDepthFunc( GL_GREATER );
		}
		else
		{
			qglDepthFunc( GL_LEQUAL );
		}
	}

	//
	// check blend bits
	//
	if ( diff & ( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS ) )
	{
		GLenum srcFactor = GL_ONE, dstFactor = GL_ONE;

		if ( stateBits & ( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS ) )
		{
			switch ( stateBits & GLS_SRCBLEND_BITS )
			{
			case GLS_SRCBLEND_ZERO:
				srcFactor = GL_ZERO;
				break;
			case GLS_SRCBLEND_ONE:
				srcFactor = GL_ONE;
				break;
			case GLS_SRCBLEND_DST_COLOR:
				srcFactor = GL_DST_COLOR;
				break;
			case GLS_SRCBLEND_ONE_MINUS_DST_COLOR:
				srcFactor = GL_ONE_MINUS_DST_COLOR;
				break;
			case GLS_SRCBLEND_SRC_ALPHA:
				srcFactor = GL_SRC_ALPHA;
				break;
			case GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA:
				srcFactor = GL_ONE_MINUS_SRC_ALPHA;
				break;
			case GLS_SRCBLEND_DST_ALPHA:
				srcFactor = GL_DST_ALPHA;
				break;
			case GLS_SRCBLEND_ONE_MINUS_DST_ALPHA:
				srcFactor = GL_ONE_MINUS_DST_ALPHA;
				break;
			case GLS_SRCBLEND_ALPHA_SATURATE:
				srcFactor = GL_SRC_ALPHA_SATURATE;
				break;
			default:
				ri->Error( ERR_DROP, "GL_State: invalid src blend state bits" );
				break;
			}

			switch ( stateBits & GLS_DSTBLEND_BITS )
			{
			case GLS_DSTBLEND_ZERO:
				dstFactor = GL_ZERO;
				break;
			case GLS_DSTBLEND_ONE:
				dstFactor = GL_ONE;
				break;
			case GLS_DSTBLEND_SRC_COLOR:
				dstFactor = GL_SRC_COLOR;
				break;
			case GLS_DSTBLEND_ONE_MINUS_SRC_COLOR:
				dstFactor = GL_ONE_MINUS_SRC_COLOR;
				break;
			case GLS_DSTBLEND_SRC_ALPHA:
				dstFactor = GL_SRC_ALPHA;
				break;
			case GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA:
				dstFactor = GL_ONE_MINUS_SRC_ALPHA;
				break;
			case GLS_DSTBLEND_DST_ALPHA:
				dstFactor = GL_DST_ALPHA;
				break;
			case GLS_DSTBLEND_ONE_MINUS_DST_ALPHA:
				dstFactor = GL_ONE_MINUS_DST_ALPHA;
				break;
			default:
				ri->Error( ERR_DROP, "GL_State: invalid dst blend state bits" );
				break;
			}

			qglEnable( GL_BLEND );
			qglBlendFunc( srcFactor, dstFactor );
		}
		else
		{
			qglDisable( GL_BLEND );
		}
	}

	//
	// check depthmask
	//
	if ( diff & GLS_DEPTHMASK_TRUE )
	{
		if ( stateBits & GLS_DEPTHMASK_TRUE )
		{
			qglDepthMask( GL_TRUE );
		}
		else
		{
			qglDepthMask( GL_FALSE );
		}
	}

	//
	// fill/line mode
	//
	if ( diff & GLS_POLYMODE_LINE )
	{
		if ( stateBits & GLS_POLYMODE_LINE )
		{
			qglPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
		}
		else
		{
			qglPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
		}
	}

	//
	// depthtest
	//
	if ( diff & GLS_DEPTHTEST_DISABLE )
	{
		if ( stateBits & GLS_DEPTHTEST_DISABLE )
		{
			qglDisable( GL_DEPTH_TEST );
		}
		else
		{
			qglEnable( GL_DEPTH_TEST );
		}
	}

	//
	// alpha test
	//
	if ( diff & GLS_ATEST_BITS )
	{
		switch ( stateBits & GLS_ATEST_BITS )
		{
		case 0:
			qglDisable( GL_ALPHA_TEST );
			break;
		case GLS_ATEST_GT_0:
			qglEnable( GL_ALPHA_TEST );
			qglAlphaFunc( GL_GREATER, 0.0f );
			break;
		case GLS_ATEST_LT_128:
			qglEnable( GL_ALPHA_TEST );
			qglAlphaFunc( GL_LESS, 0.5f );
			break;
		case GLS_ATEST_GE_128:
			qglEnable( GL_ALPHA_TEST );
			qglAlphaFunc( GL_GEQUAL, 0.5f );
			break;
		case GLS_ATEST_GE_192:
			qglEnable( GL_ALPHA_TEST );
			qglAlphaFunc( GL_GEQUAL, 0.75f );
			break;
		default:
			assert( 0 );
			break;
		}
	}

	glState.glStateBits = stateBits;
}

/*
matrix_t *InitTranslationTransform(float x, float y, float z)
{
	matrix_t m[16];
    m[0][0] = 1.0f; m[0][1] = 0.0f; m[0][2] = 0.0f; m[0][3] = x;
    m[1][0] = 0.0f; m[1][1] = 1.0f; m[1][2] = 0.0f; m[1][3] = y;
    m[2][0] = 0.0f; m[2][1] = 0.0f; m[2][2] = 1.0f; m[2][3] = z;
    m[3][0] = 0.0f; m[3][1] = 0.0f; m[3][2] = 0.0f; m[3][3] = 1.0f;
	return m;
}


matrix_t *InitCameraTransform(const vec3_t Target, const vec3_t Up)
{
	matrix_t m[16];
    vec3_t N;
	VectorCopy(Target, N);
    VectorNormalize(N);
    vec3_t U;
	VectorCopy(Up, U);
    VectorNormalize(U);
	CrossProduct(U, N, U);
    vec3_t V;
	CrossProduct(N, U, V);

    m[0][0] = U[0];   m[0][1] = U[1];   m[0][2] = U[3];   m[0][3] = 0.0f;
    m[1][0] = V[0];   m[1][1] = V[1];   m[1][2] = V[3];   m[1][3] = 0.0f;
    m[2][0] = N[0];   m[2][1] = N[1];   m[2][2] = N[3];   m[2][3] = 0.0f;
    m[3][0] = 0.0f;  m[3][1] = 0.0f;  m[3][2] = 0.0f;  m[3][3] = 1.0f;
	return m;
}
*/

void myInverseMatrix (float m[16], float src[16])
{

  float m11, m12, m13, m14, m21, m22, m23, m24; // minors of src matrix
  float m31, m32, m33, m34, m41, m42, m43, m44; // minors of src matrix
  float determinent;

  if (m != NULL && src != NULL) {
    // Finding minors of src matrix
    m11 = src[5] * (src[10] * src[15] - src[11] * src[14])
        - src[9] * (src[6] * src[15] - src[7] * src[14])
        + src[13] * (src[6] * src[11] - src[7] * src[10]);
    m12 = src[1] * (src[10] * src[15] - src[11] * src[14])
        - src[9] * (src[2] * src[15] - src[3] * src[14])
        + src[13] * (src[2] * src[11] - src[3] * src[10]);
    m13 = src[1] * (src[6] * src[15] - src[7] * src[14])
        - src[5] * (src[2] * src[15] - src[3] * src[14])
        + src[13] * (src[2] * src[7] - src[3] * src[6]);
    m14 = src[1] * (src[6] * src[11] - src[7] * src[10])
        - src[5] * (src[2] * src[11] - src[3] * src[10])
        + src[9] * (src[2] * src[7] - src[3] * src[6]);
    m21 = src[4] * (src[10] * src[15] - src[11] * src[14])
        - src[8] * (src[6] * src[15] - src[7] * src[14])
        + src[12] * (src[6] * src[11] - src[7] * src[10]);
    m22 = src[0] * (src[10] * src[15] - src[11] * src[14])
        - src[8] * (src[2] * src[15] - src[3] * src[14])
        + src[12] * (src[2] * src[11] - src[3] * src[10]);
    m23 = src[0] * (src[6] * src[15] - src[7] * src[14])
        - src[4] * (src[2] * src[15] - src[3] * src[14])
        + src[12] * (src[2] * src[7] - src[3] * src[6]);
    m24 = src[0] * (src[6] * src[11] - src[7] * src[10])
        - src[4] * (src[2] * src[11] - src[3] * src[10])
        + src[8] * (src[2] * src[7] - src[3] * src[6]);
    m31 = src[4] * (src[9] * src[15] - src[11] * src[13])
        - src[8] * (src[5] * src[15] - src[7] * src[13])
        + src[12] * (src[5] * src[11] - src[7] * src[9]);
    m32 = src[0] * (src[9] * src[15] - src[11] * src[13])
        - src[8] * (src[1] * src[15] - src[3] * src[13])
        + src[12] * (src[1] * src[11] - src[3] * src[9]);
    m33 = src[0] * (src[5] * src[15] - src[7] * src[13])
        - src[4] * (src[1] * src[15] - src[3] * src[13])
        + src[12] * (src[1] * src[7] - src[3] * src[5]);
    m34 = src[0] * (src[5] * src[11] - src[7] * src[9])
        - src[4] * (src[1] * src[11] - src[3] * src[9])
        + src[8] * (src[1] * src[7] - src[3] * src[5]);
    m41 = src[4] * (src[9] * src[14] - src[10] * src[13])
        - src[8] * (src[5] * src[14] - src[6] * src[13])
        + src[12] * (src[5] * src[10] - src[6] * src[9]);
    m42 = src[0] * (src[9] * src[14] - src[10] * src[13])
        - src[8] * (src[1] * src[14] - src[2] * src[13])
        + src[12] * (src[1] * src[10] - src[2] * src[9]);
    m43 = src[0] * (src[5] * src[14] - src[6] * src[13])
        - src[4] * (src[1] * src[14] - src[2] * src[13])
        + src[12] * (src[1] * src[6] - src[2] * src[5]);
    m44 = src[0] * (src[5] * src[10] - src[6] * src[9])
        - src[4] * (src[1] * src[10] - src[2] * src[9])
        + src[8] * (src[1] * src[6] - src[2] * src[5]);

    // calculate the determinent
    determinent = src[0] * m11 - src[4] * m12 + src[8] * m13 - src[12] * m14;
    if (determinent != 0) {
      m[0] = m11 / determinent;
      m[1] = -m12 / determinent;
      m[2] = m13 / determinent;
      m[3] = -m14 / determinent;
      m[4] = -m21 / determinent;
      m[5] = m22 / determinent;
      m[6] = -m23 / determinent;
      m[7] = m24 / determinent;
      m[8] = m31 / determinent;
      m[9] = -m32 / determinent;
      m[10] = m33 / determinent;
      m[11] = -m34 / determinent;
      m[12] = -m41 / determinent;
      m[13] = m42 / determinent;
      m[14] = -m43 / determinent;
      m[15] = m44 / determinent;
    } else {
      fprintf (stderr, "myInverseMatrix() error: no inverse matrix "
          "exists.\n");
    }
  } else {
    fprintf (stderr, "myInverseMatrix() error: matrix pointer is null.\n");
  }
}

void SWAP(float *A, float *B, float *c)
{
	c = A;
	A = B;
	B = c;
}

void RealTransposeMatrix(const float m[16], float invOut[16])
{
	float t;

	memcpy(invOut, m, sizeof(float)*16);

	SWAP(&invOut[1],&invOut[4],&t);
	SWAP(&invOut[2],&invOut[8],&t);
	SWAP(&invOut[6],&invOut[9],&t);
	SWAP(&invOut[3],&invOut[12],&t);
	SWAP(&invOut[7],&invOut[13],&t);
	SWAP(&invOut[11],&invOut[14],&t);
}

bool RealInvertMatrix(const float m[16], float invOut[16])
{
    float inv[16], det;
    int i;

    inv[0] = m[5]  * m[10] * m[15] - 
             m[5]  * m[11] * m[14] - 
             m[9]  * m[6]  * m[15] + 
             m[9]  * m[7]  * m[14] +
             m[13] * m[6]  * m[11] - 
             m[13] * m[7]  * m[10];

    inv[4] = -m[4]  * m[10] * m[15] + 
              m[4]  * m[11] * m[14] + 
              m[8]  * m[6]  * m[15] - 
              m[8]  * m[7]  * m[14] - 
              m[12] * m[6]  * m[11] + 
              m[12] * m[7]  * m[10];

    inv[8] = m[4]  * m[9] * m[15] - 
             m[4]  * m[11] * m[13] - 
             m[8]  * m[5] * m[15] + 
             m[8]  * m[7] * m[13] + 
             m[12] * m[5] * m[11] - 
             m[12] * m[7] * m[9];

    inv[12] = -m[4]  * m[9] * m[14] + 
               m[4]  * m[10] * m[13] +
               m[8]  * m[5] * m[14] - 
               m[8]  * m[6] * m[13] - 
               m[12] * m[5] * m[10] + 
               m[12] * m[6] * m[9];

    inv[1] = -m[1]  * m[10] * m[15] + 
              m[1]  * m[11] * m[14] + 
              m[9]  * m[2] * m[15] - 
              m[9]  * m[3] * m[14] - 
              m[13] * m[2] * m[11] + 
              m[13] * m[3] * m[10];

    inv[5] = m[0]  * m[10] * m[15] - 
             m[0]  * m[11] * m[14] - 
             m[8]  * m[2] * m[15] + 
             m[8]  * m[3] * m[14] + 
             m[12] * m[2] * m[11] - 
             m[12] * m[3] * m[10];

    inv[9] = -m[0]  * m[9] * m[15] + 
              m[0]  * m[11] * m[13] + 
              m[8]  * m[1] * m[15] - 
              m[8]  * m[3] * m[13] - 
              m[12] * m[1] * m[11] + 
              m[12] * m[3] * m[9];

    inv[13] = m[0]  * m[9] * m[14] - 
              m[0]  * m[10] * m[13] - 
              m[8]  * m[1] * m[14] + 
              m[8]  * m[2] * m[13] + 
              m[12] * m[1] * m[10] - 
              m[12] * m[2] * m[9];

    inv[2] = m[1]  * m[6] * m[15] - 
             m[1]  * m[7] * m[14] - 
             m[5]  * m[2] * m[15] + 
             m[5]  * m[3] * m[14] + 
             m[13] * m[2] * m[7] - 
             m[13] * m[3] * m[6];

    inv[6] = -m[0]  * m[6] * m[15] + 
              m[0]  * m[7] * m[14] + 
              m[4]  * m[2] * m[15] - 
              m[4]  * m[3] * m[14] - 
              m[12] * m[2] * m[7] + 
              m[12] * m[3] * m[6];

    inv[10] = m[0]  * m[5] * m[15] - 
              m[0]  * m[7] * m[13] - 
              m[4]  * m[1] * m[15] + 
              m[4]  * m[3] * m[13] + 
              m[12] * m[1] * m[7] - 
              m[12] * m[3] * m[5];

    inv[14] = -m[0]  * m[5] * m[14] + 
               m[0]  * m[6] * m[13] + 
               m[4]  * m[1] * m[14] - 
               m[4]  * m[2] * m[13] - 
               m[12] * m[1] * m[6] + 
               m[12] * m[2] * m[5];

    inv[3] = -m[1] * m[6] * m[11] + 
              m[1] * m[7] * m[10] + 
              m[5] * m[2] * m[11] - 
              m[5] * m[3] * m[10] - 
              m[9] * m[2] * m[7] + 
              m[9] * m[3] * m[6];

    inv[7] = m[0] * m[6] * m[11] - 
             m[0] * m[7] * m[10] - 
             m[4] * m[2] * m[11] + 
             m[4] * m[3] * m[10] + 
             m[8] * m[2] * m[7] - 
             m[8] * m[3] * m[6];

    inv[11] = -m[0] * m[5] * m[11] + 
               m[0] * m[7] * m[9] + 
               m[4] * m[1] * m[11] - 
               m[4] * m[3] * m[9] - 
               m[8] * m[1] * m[7] + 
               m[8] * m[3] * m[5];

    inv[15] = m[0] * m[5] * m[10] - 
              m[0] * m[6] * m[9] - 
              m[4] * m[1] * m[10] + 
              m[4] * m[2] * m[9] + 
              m[8] * m[1] * m[6] - 
              m[8] * m[2] * m[5];

    det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];

    if (det == 0)
        return false;

    det = 1.0 / det;

    for (i = 0; i < 16; i++)
        invOut[i] = inv[i] * det;

    return true;
}

void GL_SetProjectionMatrix(matrix_t matrix)
{
	Matrix16Copy(matrix, glState.projection);
	Matrix16Multiply(glState.projection, glState.modelview, glState.modelviewProjection);
	RealInvertMatrix/*Matrix16SimpleInverse*/( glState.projection, glState.invProjection);
	RealInvertMatrix/*Matrix16SimpleInverse*/( glState.modelviewProjection, glState.invEyeProjection);

	/*
	matrix_t CameraTranslationTrans, CameraRotateTrans;

    //InitTranslationTransform(&CameraTranslationTrans, -m_camera.Pos.x, -m_camera.Pos.y, -m_camera.Pos.z);
	Matrix16Translation( backEnd.viewParms.ori.origin, CameraTranslationTrans );
	Matrix16Translation( backEnd.refdef.viewaxis, CameraRotateTrans );
    //Matrix16SimpleInverse( CameraTranslationTrans, CameraTranslationTrans);

	Matrix16Multiply(CameraRotateTrans, CameraTranslationTrans, glState.viewTrans);
	*/
}

void GL_SetModelviewMatrix(matrix_t matrix)
{
	Matrix16Copy(matrix, glState.modelview);
	Matrix16Multiply(glState.projection, glState.modelview, glState.modelviewProjection);
	RealInvertMatrix/*Matrix16SimpleInverse*/( glState.projection, glState.invProjection);
	RealInvertMatrix/*Matrix16SimpleInverse*/( glState.modelviewProjection, glState.invEyeProjection);
}


/*
================
RB_Hyperspace

A player has predicted a teleport, but hasn't arrived yet
================
*/
static void RB_Hyperspace( void ) {
	float		c;

	if ( !backEnd.isHyperspace ) {
		// do initialization shit
	}

	c = ( backEnd.refdef.time & 255 ) / 255.0f;
	qglClearColor( c, c, c, 1 );
	qglClear( GL_COLOR_BUFFER_BIT );

	backEnd.isHyperspace = qtrue;
}


void SetViewportAndScissor( void ) {
	GL_SetProjectionMatrix( backEnd.viewParms.projectionMatrix );

	// set the window clipping
	qglViewport( backEnd.viewParms.viewportX, backEnd.viewParms.viewportY, 
		backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight );
	qglScissor( backEnd.viewParms.viewportX, backEnd.viewParms.viewportY, 
		backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight );
}

/*
=================
RB_BeginDrawingView

Any mirrored or portaled views have already been drawn, so prepare
to actually render the visible surfaces for this view
=================
*/
extern qboolean SUN_VISIBLE;
extern float MAP_WATER_LEVEL;

void RB_ClearWaterPositionMap ( void )
{
#ifdef __USE_WATERMAP__
	if (r_glslWater->integer && MAP_WATER_LEVEL > -131072.0)
	{
		FBO_Bind(tr.waterFbo);
		qglColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		qglClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		qglClear( GL_COLOR_BUFFER_BIT );
		FBO_Bind(NULL);
		qglColorMask(!backEnd.colorMask[0], !backEnd.colorMask[1], !backEnd.colorMask[2], !backEnd.colorMask[3]);

		FBO_Bind(tr.waterFbo2);
		qglColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		qglClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		qglClear( GL_COLOR_BUFFER_BIT );
		FBO_Bind(NULL);
		qglColorMask(!backEnd.colorMask[0], !backEnd.colorMask[1], !backEnd.colorMask[2], !backEnd.colorMask[3]);
	}
#endif //__USE_WATERMAP__
}

#ifdef __ORIGINAL_OCCLUSION__
extern void RB_InitOcclusionFrame(void);
#endif //__ORIGINAL_OCCLUSION__

void RB_BeginDrawingView (void) {
	int clearBits = 0;

#ifdef __ORIGINAL_OCCLUSION__
	RB_InitOcclusionFrame();
#endif //__ORIGINAL_OCCLUSION__
	RB_ClearWaterPositionMap();

	// sync with gl if needed
#ifdef __USE_QGL_FINISH__
	if ( r_finish->integer == 1 && !glState.finishCalled ) {
		qglFinish ();
		glState.finishCalled = qtrue;
	}
	if ( r_finish->integer == 0 ) {
		glState.finishCalled = qtrue;
	}
#else if __USE_QGL_FLUSH__
	if ( r_finish->integer == 1 && !glState.finishCalled ) {
		qglFlush ();
		glState.finishCalled = qtrue;
	}
	if ( r_finish->integer == 0 ) {
		glState.finishCalled = qtrue;
	}
#endif //__USE_QGL_FINISH__

	// we will need to change the projection matrix before drawing
	// 2D images again
	backEnd.projection2D = qfalse;

	// FIXME: HUGE HACK: render to the screen fbo if we've already postprocessed the frame and aren't drawing more world
	// drawing more world check is in case of double renders, such as skyportals
	if (backEnd.viewParms.targetFbo == NULL)
	{
		if (!tr.renderFbo || (backEnd.framePostProcessed && (backEnd.refdef.rdflags & RDF_NOWORLDMODEL)))
		{
			FBO_Bind(NULL);
		}
		else
		{
			FBO_Bind(tr.renderFbo);
		}
	}
	else
	{
		FBO_Bind(backEnd.viewParms.targetFbo);

		// FIXME: hack for cubemap testing
		if (tr.renderCubeFbo != NULL && backEnd.viewParms.targetFbo == tr.renderCubeFbo)
		{
			//qglFramebufferTexture2D(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + backEnd.viewParms.targetFboLayer, backEnd.viewParms.targetFbo->colorImage[0]->texnum, 0);
			qglFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + backEnd.viewParms.targetFboLayer, tr.cubemaps[backEnd.viewParms.targetFboCubemapIndex]->texnum, 0);
		}
	}

	//
	// set the modelview matrix for the viewer
	//
	SetViewportAndScissor();

	// ensures that depth writes are enabled for the depth clear
	GL_State( GLS_DEFAULT );
	// clear relevant buffers
	clearBits = GL_DEPTH_BUFFER_BIT;

	if ( r_clear->integer )
	{
		clearBits |= GL_COLOR_BUFFER_BIT;
	}

	if ( r_measureOverdraw->integer || r_shadows->integer == 2 )
	{
		clearBits |= GL_STENCIL_BUFFER_BIT;
	}

	if ( r_fastsky->integer && !( backEnd.refdef.rdflags & RDF_NOWORLDMODEL ) )
	{
		clearBits |= GL_COLOR_BUFFER_BIT;	// FIXME: only if sky shaders have been used
#ifdef _DEBUG
		qglClearColor( 0.8f, 0.7f, 0.4f, 1.0f );	// FIXME: get color of sky
#else
		qglClearColor( 0.0f, 0.0f, 0.0f, 1.0f );	// FIXME: get color of sky
#endif
	}

	// clear to white for shadow maps
	if (backEnd.viewParms.flags & VPF_SHADOWMAP)
	{
		clearBits |= GL_COLOR_BUFFER_BIT;
		qglClearColor( 1.0f, 1.0f, 1.0f, 1.0f );
	}
	
	// clear to black for cube maps
	if (tr.renderCubeFbo != NULL && backEnd.viewParms.targetFbo == tr.renderCubeFbo)
	{
		clearBits |= GL_COLOR_BUFFER_BIT;
		qglClearColor( 0.0f, 0.0f, 0.0f, 1.0f );
	}

	qglClear( clearBits );

	if (backEnd.viewParms.targetFbo == NULL)
	{
		// Clear the glow target
		float black[] = {0.0f, 0.0f, 0.0f, 1.0f};
		qglClearBufferfv (GL_COLOR, 1, black);
	}

	if ( ( backEnd.refdef.rdflags & RDF_HYPERSPACE ) )
	{
		RB_Hyperspace();
		return;
	}
	else
	{
		backEnd.isHyperspace = qfalse;
	}

	glState.faceCulling = -1;		// force face culling to set next time

	// we will only draw a sun if there was sky rendered in this view
	backEnd.skyRenderedThisView = qfalse;
	SUN_VISIBLE = qfalse;

	// clip to the plane of the portal
	if ( backEnd.viewParms.isPortal ) {
#if 0
		float	plane[4];
		double	plane2[4];

		plane[0] = backEnd.viewParms.portalPlane.normal[0];
		plane[1] = backEnd.viewParms.portalPlane.normal[1];
		plane[2] = backEnd.viewParms.portalPlane.normal[2];
		plane[3] = backEnd.viewParms.portalPlane.dist;

		plane2[0] = DotProduct (backEnd.viewParms.ori.axis[0], plane);
		plane2[1] = DotProduct (backEnd.viewParms.ori.axis[1], plane);
		plane2[2] = DotProduct (backEnd.viewParms.ori.axis[2], plane);
		plane2[3] = DotProduct (plane, backEnd.viewParms.ori.origin) - plane[3];
#endif
		GL_SetModelviewMatrix( s_flipMatrix );
	}
}

#define	MAC_EVENT_PUMP_MSEC		5


#ifdef __DEPTH_PREPASS_OCCLUSION__
#define MAX_OCCLUDION_QUERIES 32768
int occlusionQueryTotal = 0;
GLuint occlusionQuery[MAX_OCCLUDION_QUERIES] = { -1 };
qboolean occlusionQueryOccluded[MAX_OCCLUDION_QUERIES] = { qfalse };
drawSurf_t *occlusionQueryDrawSurf[MAX_OCCLUDION_QUERIES] = { NULL };

void RB_UpdateOccludedList(void)
{
	int i;
	int occluded = 0;
	int visible = 0;
	int noResult = 0;

	qglFinish();

	// first, check any outstanding queries
	for (i = 0; i < occlusionQueryTotal; i++)
	{
		GLuint result;

		qglGetQueryObjectuiv(occlusionQuery[i], GL_QUERY_RESULT_AVAILABLE, &result);

		if (result)
		{
			qglGetQueryObjectuiv(occlusionQuery[i], GL_QUERY_RESULT, &result);

			//ri->Printf(PRINT_ALL, "surface %i has %d samples!\n", i, result);

			if (result <= 0)
			{
				occlusionQueryOccluded[i] = qtrue;
				occluded++;
			}
			else
			{
				occlusionQueryOccluded[i] = qfalse;
				visible++;
			}
		}
		else
		{
			occlusionQueryOccluded[i] = qfalse;
			noResult++;
		}
	}

	ri->Printf(PRINT_WARNING, "%i queries. %i occluded. %i visible. %i noResult.\n", occlusionQueryTotal, occluded, visible, noResult);
	occlusionQueryTotal = 0;
}
#endif //__DEPTH_PREPASS_OCCLUSION__

/*
==================
RB_RenderDrawSurfList
==================
*/

#ifndef __MERGE_MODEL_SURFACES2__
void RB_RenderDrawSurfList( drawSurf_t *drawSurfs, int numDrawSurfs, qboolean inQuery ) {
	int				i, max_threads_used = 0;
	float			originalTime;
	FBO_t*			fbo = NULL;
	shader_t		*oldShader = NULL;
	int			oldFogNum = -1;
	int			oldEntityNum = -1;
	int			oldDlighted = 0;
	int			oldPostRender = 0;
	int             oldCubemapIndex = -1;
	int				oldDepthRange = 0;
	uint		oldSort = (uint) -1;
	qboolean		CUBEMAPPING = qfalse;
	float			depth[2];
	int				numOccluded = 0;

	if (r_cubeMapping->integer >= 1) CUBEMAPPING = qtrue;

	if (((backEnd.refdef.rdflags & RDF_BLUR) || (tr.viewParms.flags & VPF_SHADOWPASS) || backEnd.depthFill /*|| (backEnd.viewParms.flags & VPF_DEPTHSHADOW)*/)) CUBEMAPPING = qfalse;

	// draw everything
	backEnd.currentEntity = &tr.worldEntity;

	depth[0] = 0.f;
	depth[1] = 1.f;

	backEnd.pc.c_surfaces += numDrawSurfs;

#ifdef __DEPTH_PREPASS_OCCLUSION__
	if (r_occlusion->integer && !backEnd.depthFill && !(backEnd.viewParms.flags & VPF_DEPTHSHADOW) && !inQuery)
	{
		RB_UpdateOccludedList();
	}
#endif //__DEPTH_PREPASS_OCCLUSION__

	// save original time for entity shader offsets
	originalTime = backEnd.refdef.floatTime;

	fbo = glState.currentFBO;

	for (i = 0 ; i < numDrawSurfs ; ++i) 
	{
		drawSurf_t		*drawSurf;
		shader_t		*shader = NULL;
		int			fogNum;
		int			entityNum;
		int			dlighted;
		int			postRender;
		int             cubemapIndex, newCubemapIndex;
		int				depthRange;

		if (backEnd.depthFill && shader && shader->sort != SS_OPAQUE && shader->sort != SS_SEE_THROUGH) continue; // UQ1: No point thinking any more on this one...

		drawSurf = &drawSurfs[i];

		if (!drawSurf->surface) continue;

#ifdef __DEPTH_PREPASS_OCCLUSION__
		if (r_occlusion->integer && !inQuery && !backEnd.depthFill && !(backEnd.viewParms.flags & VPF_DEPTHSHADOW) && backEnd.currentEntity != &tr.worldEntity) {
			if (occlusionQueryOccluded[i])
			{
				if (occlusionQueryDrawSurf[i] == drawSurf)
				{
					numOccluded++;
					continue;
				}
				/*else
				{
					ri->Printf(PRINT_WARNING, "Occlusion query drawSurf %i does not match render drawSurf. cubemapIndex %s. sort %s. surface %s.\n", i,
						drawSurf->cubemapIndex != occlusionQueryDrawSurf[i]->cubemapIndex ? "DIFFERENT" : "SAME",
						drawSurf->sort != occlusionQueryDrawSurf[i]->sort ? "DIFFERENT" : "SAME",
						drawSurf->surface != occlusionQueryDrawSurf[i]->surface ? "DIFFERENT" : "SAME");
				}*/
			}
		}
#endif //__DEPTH_PREPASS_OCCLUSION__

		// Don't care about these...
		dlighted = oldDlighted = 0;
		fogNum = oldFogNum = 0;

		if (!CUBEMAPPING)
		{
			newCubemapIndex = 0;
		}
		else
		{
			if (r_cubeMapping->integer >= 1)
			{
				newCubemapIndex = drawSurf->cubemapIndex;
			}
			else
			{
				newCubemapIndex = 0;
			}

			if (newCubemapIndex > 0)
			{// Let's see if we can swap with a close cubemap and merge them...
				
				if (Distance(tr.refdef.vieworg, tr.cubemapOrigins[newCubemapIndex-1]) > r_cubemapCullRange->value * r_cubemapCullFalloffMult->value)
				{// Too far away to care about cubemaps... Allow merge...
					newCubemapIndex = 0;
				}
			}
		}

		if ( drawSurf->sort == oldSort 
#ifndef __LAZY_CUBEMAP__
			&& (!CUBEMAPPING || newCubemapIndex == oldCubemapIndex) 
#endif //__LAZY_CUBEMAP__
			) 
		{// fast path, same as previous sort
#ifdef __DEPTH_PREPASS_OCCLUSION__
			if (r_occlusion->integer && inQuery && backEnd.depthFill && !(backEnd.viewParms.flags & VPF_DEPTHSHADOW) && backEnd.currentEntity != &tr.worldEntity)
			{
				qglGenQueries(1, &occlusionQuery[i]);
				qglBeginQuery(GL_SAMPLES_PASSED, occlusionQuery[i]);
				occlusionQueryTotal = numDrawSurfs;
				occlusionQueryDrawSurf[i] = drawSurf;
				rb_surfaceTable[*drawSurf->surface](drawSurf->surface);
				qglEndQuery(GL_SAMPLES_PASSED);
			}
			else
			{
				occlusionQueryOccluded[i] = qfalse;
				rb_surfaceTable[*drawSurf->surface](drawSurf->surface);
			}
#else //!__DEPTH_PREPASS_OCCLUSION__
			rb_surfaceTable[ *drawSurf->surface ]( drawSurf->surface );
#endif //__DEPTH_PREPASS_OCCLUSION__
			continue;
		}

		oldSort = drawSurf->sort;
		R_DecomposeSort( drawSurf->sort, &entityNum, &shader, &fogNum, &postRender );

		cubemapIndex = newCubemapIndex;

#ifdef __MERGE_MORE__
		if ( shader != NULL && !shader->entityMergable)
			shader->entityMergable = qtrue;
#endif //__MERGE_MORE__

		//
		// change the tess parameters if needed
		// a "entityMergable" shader is a shader that can have surfaces from seperate
		// entities merged into a single batch, like smoke and blood puff sprites
		if ( shader != NULL 
			&& ( shader != oldShader 
				//|| fogNum != oldFogNum 
				//|| dlighted != oldDlighted 
				|| postRender != oldPostRender 
#ifndef __LAZY_CUBEMAP__
				|| (CUBEMAPPING && cubemapIndex != oldCubemapIndex) 
#endif //__LAZY_CUBEMAP__
				|| ( entityNum != oldEntityNum && !shader->entityMergable ) ) ) 
		{
			if (oldShader != NULL) 
			{
				RB_EndSurface();
			}

			RB_BeginSurface( shader, fogNum, cubemapIndex );

			backEnd.pc.c_surfBatches++;
			oldShader = shader;
			oldFogNum = fogNum;
			oldDlighted = dlighted;
			oldPostRender = postRender;
			
			oldCubemapIndex = cubemapIndex;
		}

		//
		// change the modelview matrix if needed
		//
		if ( entityNum != oldEntityNum ) 
		{
			qboolean sunflare = qfalse;
			depthRange = 0;

			if ( entityNum != REFENTITYNUM_WORLD ) 
			{
				backEnd.currentEntity = &backEnd.refdef.entities[entityNum];
				backEnd.refdef.floatTime = originalTime - backEnd.currentEntity->e.shaderTime;
				// we have to reset the shaderTime as well otherwise image animations start
				// from the wrong frame
				tess.shaderTime = backEnd.refdef.floatTime - tess.shader->timeOffset;

				// set up the transformation matrix
				R_RotateForEntity( backEnd.currentEntity, &backEnd.viewParms, &backEnd.ori );

				if ( backEnd.currentEntity->needDlights ) 
				{// set up the dynamic lighting if needed
					R_TransformDlights( backEnd.refdef.num_dlights, backEnd.refdef.dlights, &backEnd.ori );
				}

				if ( backEnd.currentEntity->e.renderfx & RF_NODEPTH ) 
				{// No depth at all, very rare but some things for seeing through walls
					depthRange = 2;
				}
				else if ( backEnd.currentEntity->e.renderfx & RF_DEPTHHACK ) 
				{// hack the depth range to prevent view model from poking into walls
					depthRange = 1;
				}
			} else {
				backEnd.currentEntity = &tr.worldEntity;
				backEnd.refdef.floatTime = originalTime;
				backEnd.ori = backEnd.viewParms.world;
				// we have to reset the shaderTime as well otherwise image animations on
				// the world (like water) continue with the wrong frame
				tess.shaderTime = backEnd.refdef.floatTime - tess.shader->timeOffset;
				R_TransformDlights( backEnd.refdef.num_dlights, backEnd.refdef.dlights, &backEnd.ori );
			}

			GL_SetModelviewMatrix( backEnd.ori.modelMatrix );

			//
			// change depthrange. Also change projection matrix so first person weapon does not look like coming
			// out of the screen.
			//
			if (oldDepthRange != depthRange)
			{
				switch ( depthRange ) {
				default:
				case 0:
					if(backEnd.viewParms.stereoFrame != STEREO_CENTER)
					{
						GL_SetProjectionMatrix( backEnd.viewParms.projectionMatrix );
					}

					if( !sunflare )
						qglDepthRange( 0.0f, 1.0f );

					depth[0] = 0;
					depth[1] = 1;
					break;

				case 1:
					if(backEnd.viewParms.stereoFrame != STEREO_CENTER)
					{
						viewParms_t temp = backEnd.viewParms;

						R_SetupProjection(&temp, r_znear->value, 0, qfalse);

						GL_SetProjectionMatrix( temp.projectionMatrix );
					}

					if ( !oldDepthRange )
 							qglDepthRange( 0.0f, 0.3f );

					break;

				case 2:
					if(backEnd.viewParms.stereoFrame != STEREO_CENTER)
					{
						viewParms_t temp = backEnd.viewParms;

						R_SetupProjection(&temp, r_znear->value, 0, qfalse);

						GL_SetProjectionMatrix( temp.projectionMatrix );
					}

					if ( !oldDepthRange )
 							qglDepthRange( 0.0f, 0.0f );

					break;
				}

				oldDepthRange = depthRange;
			}

			oldEntityNum = entityNum;
		}

		// add the triangles for this surface
#ifdef __DEPTH_PREPASS_OCCLUSION__
		if (r_occlusion->integer && inQuery && backEnd.depthFill && !(backEnd.viewParms.flags & VPF_DEPTHSHADOW) && backEnd.currentEntity != &tr.worldEntity)
		{
			qglGenQueries(1, &occlusionQuery[i]);
			qglBeginQuery(GL_SAMPLES_PASSED, occlusionQuery[i]);
			occlusionQueryTotal = numDrawSurfs;
			occlusionQueryDrawSurf[i] = drawSurf;
			rb_surfaceTable[*drawSurf->surface](drawSurf->surface);
			qglEndQuery(GL_SAMPLES_PASSED);
		}
		else
		{
			occlusionQueryOccluded[i] = qfalse;
			rb_surfaceTable[*drawSurf->surface](drawSurf->surface);
		}
#else //!__DEPTH_PREPASS_OCCLUSION__
		rb_surfaceTable[*drawSurf->surface](drawSurf->surface);
#endif //__DEPTH_PREPASS_OCCLUSION__
	}

	backEnd.refdef.floatTime = originalTime;

	// draw the contents of the last shader batch
	if (oldShader != NULL) {
		RB_EndSurface();
	}

#ifndef __DEPTH_PREPASS_OCCLUSION__
	if (inQuery) {
		qglEndQuery(GL_SAMPLES_PASSED);
	}
#endif //__DEPTH_PREPASS_OCCLUSION__

	FBO_Bind(fbo);

	// go back to the world modelview matrix

	GL_SetModelviewMatrix( backEnd.viewParms.world.modelMatrix );

	// Restore depth range for subsequent rendering
	qglDepthRange( 0.0f, 1.0f );

	/*
#ifdef __DEPTH_PREPASS_OCCLUSION__
	if (r_occlusion->integer && inQuery)
	{
		ri->Printf(PRINT_WARNING, "%i draw surfaces. %i occlusion queries.\n", numDrawSurfs, occlusionQueryTotal);
	}
	else if (r_occlusion->integer && !inQuery)
	{
		ri->Printf(PRINT_WARNING, "%i draw surfaces. %i occluded.\n", numDrawSurfs, numOccluded);
	}
	else if (r_occlusion->integer && !backEnd.depthFill)
	{
		ri->Printf(PRINT_WARNING, "%i draw surfaces. (non depthFill).\n", numDrawSurfs);
	}
#endif //__DEPTH_PREPASS_OCCLUSION__
	*/

#ifdef __DEBUG_MERGE__
	ri->Printf(PRINT_WARNING, "TOTAL %i, NUM_MERGED %i, NUM_CUBE_MERGED %i, NUM_CUBE_DELETED %i, NUM_FAST_PATH %i, NUM_NULL_SHADERS %i, NUM_SHADER_FAILS %i, NUM_POSTRENDER_FAILS %i, NUM_CUBEMAP_FAILS %i, NUM_MERGABLE_FAILS %i.\n"
		, NUM_TOTAL, NUM_MERGED, NUM_CUBE_MERGED, NUM_CUBE_DELETED, NUM_FAST_PATH, NUM_NULL_SHADERS, NUM_SHADER_FAILS, NUM_POSTRENDER_FAILS, NUM_CUBEMAP_FAILS, NUM_MERGABLE_FAILS);
#endif //__DEBUG_MERGE__
}
#else //__MERGE_MODEL_SURFACES2__
#define MAX_SHADER_MERGED_GROUPS 2048
#define MAX_SHADER_MERGED_SURFACES 16550

typedef struct sortedDrawInfo_s {
	int				shaderIndex = -1;
	qboolean		isPostRender = qfalse;
	int				cubemapIndex = 0;
	int				sort = 0;
	shader_t		*shader = NULL;
	int				numDrawSurfs = 0;
	drawSurf_t		*drawSurf[MAX_SHADER_MERGED_SURFACES];
	int				entityNum[MAX_SHADER_MERGED_SURFACES];
} sortedDrawInfo_t;

sortedDrawInfo_t sortedDrawInfos[MAX_SHADER_MERGED_GROUPS];

void RB_RenderDrawSurfList(drawSurf_t *drawSurfs, int numDrawSurfs, qboolean inQuery) {
	int				i, max_threads_used = 0;
	int				j = 0;
	float			originalTime;
	FBO_t*			fbo = NULL;
	shader_t		*oldShader = NULL;
	int				oldEntityNum = -1;
	int				oldPostRender = 0;
	int             oldCubemapIndex = -1;
	int				oldDepthRange = 0;
	int				oldSort = (int)-1;
	qboolean		CUBEMAPPING = qfalse;
	float			depth[2];

	if (r_cubeMapping->integer >= 1) CUBEMAPPING = qtrue;

	if (((backEnd.refdef.rdflags & RDF_BLUR) || (tr.viewParms.flags & VPF_SHADOWPASS) || backEnd.depthFill /*|| (backEnd.viewParms.flags & VPF_DEPTHSHADOW)*/)) CUBEMAPPING = qfalse;

	// draw everything
	backEnd.currentEntity = &tr.worldEntity;

	depth[0] = 0.f;
	depth[1] = 1.f;

	backEnd.pc.c_surfaces += numDrawSurfs;

	// save original time for entity shader offsets
	originalTime = backEnd.refdef.floatTime;

	fbo = glState.currentFBO;

	if (!r_entitySurfaceMerge->integer)
	{
#ifdef __DEBUG_MERGE__
		int numShaderChanges = 0;
		int numShaderDraws = 0;
#endif //__DEBUG_MERGE__

		// First draw world normally...
		for (i = 0; i < numDrawSurfs; ++i)
		{
			int			zero = 0;
			drawSurf_t		*drawSurf;
			shader_t		*shader = NULL;
			int			entityNum;
			int			postRender;
			int             cubemapIndex, newCubemapIndex;
			int				depthRange;

			if (backEnd.depthFill && shader && shader->sort != SS_OPAQUE && shader->sort != SS_SEE_THROUGH) continue; // UQ1: No point thinking any more on this one...

			drawSurf = &drawSurfs[i];

			if (!drawSurf->surface) continue;

			if (!CUBEMAPPING)
			{
				newCubemapIndex = 0;
			}
			else
			{
				if (r_cubeMapping->integer >= 1)
				{
					newCubemapIndex = drawSurf->cubemapIndex;
				}
				else
				{
					newCubemapIndex = 0;
				}

				if (newCubemapIndex > 0)
				{// Let's see if we can swap with a close cubemap and merge them...

					if (Distance(tr.refdef.vieworg, tr.cubemapOrigins[newCubemapIndex - 1]) > r_cubemapCullRange->value * r_cubemapCullFalloffMult->value)
					{// Too far away to care about cubemaps... Allow merge...
						newCubemapIndex = 0;
					}
				}
			}

			if (drawSurf->sort == oldSort 
#ifndef __LAZY_CUBEMAP__
				&& (!CUBEMAPPING || newCubemapIndex == oldCubemapIndex)
#endif //__LAZY_CUBEMAP__
				)
			{// fast path, same as previous sort
				rb_surfaceTable[*drawSurf->surface](drawSurf->surface);
#ifdef __DEBUG_MERGE__
				numShaderDraws++;
#endif //__DEBUG_MERGE__
				continue;
			}

			oldSort = drawSurf->sort;
			R_DecomposeSort(drawSurf->sort, &entityNum, &shader, &zero, &postRender);

			cubemapIndex = newCubemapIndex;

			//
			// change the tess parameters if needed
			// a "entityMergable" shader is a shader that can have surfaces from seperate
			// entities merged into a single batch, like smoke and blood puff sprites
			if (shader != NULL
				&& (shader != oldShader
					|| postRender != oldPostRender
#ifndef __LAZY_CUBEMAP__
					|| (CUBEMAPPING && cubemapIndex != oldCubemapIndex)
#endif //__LAZY_CUBEMAP__
					))
			{
				if (oldShader != NULL)
				{
					RB_EndSurface();
				}

				RB_BeginSurface(shader, 0, cubemapIndex);

				backEnd.pc.c_surfBatches++;
				oldShader = shader;
				oldPostRender = postRender;
				oldCubemapIndex = cubemapIndex;
#ifdef __DEBUG_MERGE__
				numShaderChanges++;
#endif //__DEBUG_MERGE__
			}

			//
			// change the modelview matrix if needed
			//
			if (entityNum != oldEntityNum)
			{
				qboolean sunflare = qfalse;
				depthRange = 0;

				// we have to reset the shaderTime as well otherwise image animations start
				// from the wrong frame
				tess.shaderTime = backEnd.refdef.floatTime - tess.shader->timeOffset;

				if (entityNum != REFENTITYNUM_WORLD)
				{
					backEnd.currentEntity = &backEnd.refdef.entities[entityNum];
					backEnd.refdef.floatTime = originalTime - backEnd.currentEntity->e.shaderTime;

					// set up the transformation matrix
					R_RotateForEntity(backEnd.currentEntity, &backEnd.viewParms, &backEnd.ori);

					if (backEnd.currentEntity->needDlights)
					{// set up the dynamic lighting if needed
						R_TransformDlights(backEnd.refdef.num_dlights, backEnd.refdef.dlights, &backEnd.ori);
					}

					if (backEnd.currentEntity->e.renderfx & RF_NODEPTH)
					{// No depth at all, very rare but some things for seeing through walls
						depthRange = 2;
					}
					else if (backEnd.currentEntity->e.renderfx & RF_DEPTHHACK)
					{// hack the depth range to prevent view model from poking into walls
						depthRange = 1;
					}
				}
				else {
					backEnd.currentEntity = &tr.worldEntity;
					backEnd.refdef.floatTime = originalTime;
					backEnd.ori = backEnd.viewParms.world;
					// we have to reset the shaderTime as well otherwise image animations on
					// the world (like water) continue with the wrong frame
					R_TransformDlights(backEnd.refdef.num_dlights, backEnd.refdef.dlights, &backEnd.ori);
				}

				GL_SetModelviewMatrix(backEnd.ori.modelMatrix);

				//
				// change depthrange. Also change projection matrix so first person weapon does not look like coming
				// out of the screen.
				//
				if (oldDepthRange != depthRange)
				{
					switch (depthRange) {
					default:
					case 0:
						if (backEnd.viewParms.stereoFrame != STEREO_CENTER)
						{
							GL_SetProjectionMatrix(backEnd.viewParms.projectionMatrix);
						}

						if (!sunflare)
							qglDepthRange(0.0f, 1.0f);

						depth[0] = 0;
						depth[1] = 1;
						break;

					case 1:
						if (backEnd.viewParms.stereoFrame != STEREO_CENTER)
						{
							viewParms_t temp = backEnd.viewParms;

							R_SetupProjection(&temp, r_znear->value, 0, qfalse);

							GL_SetProjectionMatrix(temp.projectionMatrix);
						}

						if (!oldDepthRange)
							qglDepthRange(0.0f, 0.3f);

						break;

					case 2:
						if (backEnd.viewParms.stereoFrame != STEREO_CENTER)
						{
							viewParms_t temp = backEnd.viewParms;

							R_SetupProjection(&temp, r_znear->value, 0, qfalse);

							GL_SetProjectionMatrix(temp.projectionMatrix);
						}

						if (!oldDepthRange)
							qglDepthRange(0.0f, 0.0f);

						break;
					}

					oldDepthRange = depthRange;
				}

				oldEntityNum = entityNum;
			}

			// add the triangles for this surface
			rb_surfaceTable[*drawSurf->surface](drawSurf->surface);
#ifdef __DEBUG_MERGE__
			numShaderDraws++;
#endif //__DEBUG_MERGE__
		}

#ifdef __DEBUG_MERGE__
		ri->Printf(PRINT_ALL, "%i total surface draws. %i shader changes.\n", numShaderDraws, numShaderChanges);
#endif //__DEBUG_MERGE__
	}
	else
	{
#ifdef __DEBUG_MERGE__
		int numShaderChanges = 0;
		int numShaderDraws = 0;
#endif //__DEBUG_MERGE__

		while (1)
		{// Add to the sorted list...
			if (j >= MAX_SHADER_MERGED_GROUPS)
			{
				if (r_entitySurfaceMerge->integer > 2)
					ri->Printf(PRINT_WARNING, "MAX_SHADER_MERGED_GROUPS\n");
				break;
			}

			sortedDrawInfo_t *sorted = &sortedDrawInfos[j];

			if (sorted->shaderIndex == -1) break;

			sorted->shaderIndex = -1;
			sorted->numDrawSurfs = 0;

			j++;
		}

		int numTotal = 0;
		int numSorted = 0;

		j = 0;

		for (i = 0; i < numDrawSurfs; ++i)
		{
			int			zero = 0;
			shader_t		*shader = NULL;
			int             cubemapIndex, newCubemapIndex;
			drawSurf_t		*drawSurf;
			int			entityNum;
			int			postRender;
			int				depthRange;

			if (backEnd.depthFill && shader && shader->sort != SS_OPAQUE && shader->sort != SS_SEE_THROUGH) continue; // UQ1: No point thinking any more on this one...

			drawSurf = &drawSurfs[i];

			if (!drawSurf->surface) continue;

			if (!CUBEMAPPING)
			{
				newCubemapIndex = 0;
			}
			else
			{
				if (r_cubeMapping->integer >= 1)
				{
					newCubemapIndex = drawSurf->cubemapIndex;
				}
				else
				{
					newCubemapIndex = 0;
				}

				if (newCubemapIndex > 0)
				{// Let's see if we can swap with a close cubemap and merge them...

					if (Distance(tr.refdef.vieworg, tr.cubemapOrigins[newCubemapIndex - 1]) > r_cubemapCullRange->value * r_cubemapCullFalloffMult->value)
					{// Too far away to care about cubemaps... Allow merge...
						newCubemapIndex = 0;
					}
				}
			}

			R_DecomposeSort(drawSurf->sort, &entityNum, &shader, &zero, &postRender);

			//
			// Non-world surfaces can be merged and drawn after...
			//

			if (entityNum != REFENTITYNUM_WORLD)
			{
				j = 0;

				while (1)
				{// Add to the sorted list...
					if (j >= MAX_SHADER_MERGED_GROUPS)
					{
						if (r_entitySurfaceMerge->integer > 2)
							ri->Printf(PRINT_WARNING, "MAX_SHADER_MERGED_GROUPS\n");
						break;
					}

					sortedDrawInfo_t *sorted = &sortedDrawInfos[j];

					if (sorted->shaderIndex == -1)
					{// Create a new slot...
						sorted->shaderIndex = shader->index;
						sorted->shader = shader;
						sorted->cubemapIndex = newCubemapIndex;
						sorted->sort = drawSurf->sort;
						//sorted->entityNum = entityNum;
						sorted->isPostRender = (qboolean)postRender;

						if (sorted->numDrawSurfs + 1 >= MAX_SHADER_MERGED_SURFACES)
						{
							if (r_entitySurfaceMerge->integer > 2)
								ri->Printf(PRINT_WARNING, "MAX_SHADER_MERGED_SURFACES\n");
							break;
						}

						sorted->entityNum[sorted->numDrawSurfs] = entityNum;
						sorted->drawSurf[sorted->numDrawSurfs] = drawSurf;
						sorted->numDrawSurfs++;
						numTotal++;
						numSorted++;
						break;
					}

					if (sorted->shaderIndex == shader->index
#ifndef __LAZY_CUBEMAP__
						&& sorted->cubemapIndex == newCubemapIndex
#endif //__LAZY_CUBEMAP__
						&& sorted->isPostRender == (qboolean)postRender
						/*&& sorted->sort == drawSurf->sort*/)
					{// Add it to this slot's list...

						//sorted->sort = drawSurf->sort;
						if (sorted->numDrawSurfs + 1 >= MAX_SHADER_MERGED_SURFACES)
						{
							if (r_entitySurfaceMerge->integer > 2)
								ri->Printf(PRINT_WARNING, "MAX_SHADER_MERGED_SURFACES\n");
							break;
						}

						sorted->entityNum[sorted->numDrawSurfs] = entityNum;
						sorted->drawSurf[sorted->numDrawSurfs] = drawSurf;
						sorted->numDrawSurfs++;
						numTotal++;
						break;
					}

					j++;
				}

				continue;
			}

			//
			// World surfaces are already pre-merged at map load...
			//

			if (drawSurf->sort == oldSort 
#ifndef __LAZY_CUBEMAP__
				&& (!CUBEMAPPING || newCubemapIndex == oldCubemapIndex)
#endif //__LAZY_CUBEMAP__
				)
			{// fast path, same as previous sort
				rb_surfaceTable[*drawSurf->surface](drawSurf->surface);
#ifdef __DEBUG_MERGE__
				numShaderDraws++;
#endif //__DEBUG_MERGE__
				continue;
			}

			oldSort = drawSurf->sort;
			R_DecomposeSort(drawSurf->sort, &entityNum, &shader, &zero, &postRender);

			cubemapIndex = newCubemapIndex;

			//
			// change the tess parameters if needed
			// a "entityMergable" shader is a shader that can have surfaces from seperate
			// entities merged into a single batch, like smoke and blood puff sprites
			if (shader != NULL
				&& (shader != oldShader
					|| postRender != oldPostRender
#ifndef __LAZY_CUBEMAP__
					|| (CUBEMAPPING && cubemapIndex != oldCubemapIndex)
#endif //__LAZY_CUBEMAP__
					))
			{
				if (oldShader != NULL)
				{
					RB_EndSurface();
				}

				RB_BeginSurface(shader, 0, cubemapIndex);

				backEnd.pc.c_surfBatches++;
				oldShader = shader;
				oldPostRender = postRender;
				oldCubemapIndex = cubemapIndex;
#ifdef __DEBUG_MERGE__
				numShaderChanges++;
#endif //__DEBUG_MERGE__
			}

			//
			// change the modelview matrix if needed
			//
			if (entityNum != oldEntityNum)
			{
				qboolean sunflare = qfalse;
				depthRange = 0;

				// we have to reset the shaderTime as well otherwise image animations start
				// from the wrong frame
				tess.shaderTime = backEnd.refdef.floatTime - tess.shader->timeOffset;

				if (entityNum != REFENTITYNUM_WORLD)
				{
					backEnd.currentEntity = &backEnd.refdef.entities[entityNum];
					backEnd.refdef.floatTime = originalTime - backEnd.currentEntity->e.shaderTime;

					// set up the transformation matrix
					R_RotateForEntity(backEnd.currentEntity, &backEnd.viewParms, &backEnd.ori);

					if (backEnd.currentEntity->needDlights)
					{// set up the dynamic lighting if needed
						R_TransformDlights(backEnd.refdef.num_dlights, backEnd.refdef.dlights, &backEnd.ori);
					}

					if (backEnd.currentEntity->e.renderfx & RF_NODEPTH)
					{// No depth at all, very rare but some things for seeing through walls
						depthRange = 2;
					}
					else if (backEnd.currentEntity->e.renderfx & RF_DEPTHHACK)
					{// hack the depth range to prevent view model from poking into walls
						depthRange = 1;
					}
				}
				else {
					backEnd.currentEntity = &tr.worldEntity;
					backEnd.refdef.floatTime = originalTime;
					backEnd.ori = backEnd.viewParms.world;
					// we have to reset the shaderTime as well otherwise image animations on
					// the world (like water) continue with the wrong frame
					R_TransformDlights(backEnd.refdef.num_dlights, backEnd.refdef.dlights, &backEnd.ori);
				}

				GL_SetModelviewMatrix(backEnd.ori.modelMatrix);

				//
				// change depthrange. Also change projection matrix so first person weapon does not look like coming
				// out of the screen.
				//
				if (oldDepthRange != depthRange)
				{
					switch (depthRange) {
					default:
					case 0:
						if (backEnd.viewParms.stereoFrame != STEREO_CENTER)
						{
							GL_SetProjectionMatrix(backEnd.viewParms.projectionMatrix);
						}

						if (!sunflare)
							qglDepthRange(0.0f, 1.0f);

						depth[0] = 0;
						depth[1] = 1;
						break;

					case 1:
						if (backEnd.viewParms.stereoFrame != STEREO_CENTER)
						{
							viewParms_t temp = backEnd.viewParms;

							R_SetupProjection(&temp, r_znear->value, 0, qfalse);

							GL_SetProjectionMatrix(temp.projectionMatrix);
						}

						if (!oldDepthRange)
							qglDepthRange(0.0f, 0.3f);

						break;

					case 2:
						if (backEnd.viewParms.stereoFrame != STEREO_CENTER)
						{
							viewParms_t temp = backEnd.viewParms;

							R_SetupProjection(&temp, r_znear->value, 0, qfalse);

							GL_SetProjectionMatrix(temp.projectionMatrix);
						}

						if (!oldDepthRange)
							qglDepthRange(0.0f, 0.0f);

						break;
					}

					oldDepthRange = depthRange;
				}

				oldEntityNum = entityNum;
			}

			// add the triangles for this surface
			rb_surfaceTable[*drawSurf->surface](drawSurf->surface);
#ifdef __DEBUG_MERGE__
			numShaderDraws++;
#endif //__DEBUG_MERGE__
		}

		if (r_entitySurfaceMerge->integer > 1)
			ri->Printf(PRINT_ALL, "%i draw surfs were merged into %i sorted lists.\n", numTotal, numSorted);

		j = 0;

		// Draw sorted entity surface list...
		while (1)
		{// Add to the sorted list...
			if (j >= MAX_SHADER_MERGED_GROUPS)
			{
				if (r_entitySurfaceMerge->integer > 2)
					ri->Printf(PRINT_WARNING, "MAX_SHADER_MERGED_GROUPS2\n");
				break;
			}

			sortedDrawInfo_t *sorted = &sortedDrawInfos[j];

			if (sorted->shaderIndex == -1)
				break;

			for (i = 0; i < sorted->numDrawSurfs; ++i)
			{
				int			zero = 0;
				drawSurf_t		*drawSurf;
				shader_t		*shader = NULL;
				int			entityNum;
				int			postRender;
				int             cubemapIndex, newCubemapIndex;
				int				depthRange;

				if (backEnd.depthFill && shader && shader->sort != SS_OPAQUE && shader->sort != SS_SEE_THROUGH) continue; // UQ1: No point thinking any more on this one...

				drawSurf = sorted->drawSurf[i];

				if (!drawSurf->surface) continue;

				entityNum = sorted->entityNum[i];
				newCubemapIndex = sorted->cubemapIndex;
				shader = sorted->shader;
				postRender = sorted->isPostRender;
				
				if (drawSurf->sort == oldSort 
#ifndef __LAZY_CUBEMAP__
					&& (!CUBEMAPPING || newCubemapIndex == oldCubemapIndex)
#endif //__LAZY_CUBEMAP__
					)
				{// fast path, same as previous sort
					rb_surfaceTable[*drawSurf->surface](drawSurf->surface);
#ifdef __DEBUG_MERGE__
					numShaderDraws++;
#endif //__DEBUG_MERGE__
					continue;
				}

				oldSort = drawSurf->sort;
				//R_DecomposeSort(drawSurf->sort, &entityNum, &shader, &zero, &postRender);

				cubemapIndex = newCubemapIndex;

				//
				// change the tess parameters if needed
				// a "entityMergable" shader is a shader that can have surfaces from seperate
				// entities merged into a single batch, like smoke and blood puff sprites
				if (shader != NULL
					&& (shader != oldShader
						|| postRender != oldPostRender
#ifndef __LAZY_CUBEMAP__
						|| (CUBEMAPPING && cubemapIndex != oldCubemapIndex)
#endif //__LAZY_CUBEMAP__
						))
				{
					if (oldShader != NULL)
					{
						RB_EndSurface();
					}

					RB_BeginSurface(shader, 0, cubemapIndex);

					backEnd.pc.c_surfBatches++;
					oldShader = shader;
					oldPostRender = postRender;
					oldCubemapIndex = cubemapIndex;
#ifdef __DEBUG_MERGE__
					numShaderChanges++;
#endif //__DEBUG_MERGE__
				}

				//
				// change the modelview matrix if needed
				//
				if (entityNum != oldEntityNum)
				{
					qboolean sunflare = qfalse;
					depthRange = 0;

					// we have to reset the shaderTime as well otherwise image animations start
					// from the wrong frame
					tess.shaderTime = backEnd.refdef.floatTime - tess.shader->timeOffset;

					if (entityNum != REFENTITYNUM_WORLD)
					{
						backEnd.currentEntity = &backEnd.refdef.entities[entityNum];
						backEnd.refdef.floatTime = originalTime - backEnd.currentEntity->e.shaderTime;

						// set up the transformation matrix
						R_RotateForEntity(backEnd.currentEntity, &backEnd.viewParms, &backEnd.ori);

						if (backEnd.currentEntity->needDlights)
						{// set up the dynamic lighting if needed
							R_TransformDlights(backEnd.refdef.num_dlights, backEnd.refdef.dlights, &backEnd.ori);
						}

						if (backEnd.currentEntity->e.renderfx & RF_NODEPTH)
						{// No depth at all, very rare but some things for seeing through walls
							depthRange = 2;
						}
						else if (backEnd.currentEntity->e.renderfx & RF_DEPTHHACK)
						{// hack the depth range to prevent view model from poking into walls
							depthRange = 1;
						}
					}
					else {
						backEnd.currentEntity = &tr.worldEntity;
						backEnd.refdef.floatTime = originalTime;
						backEnd.ori = backEnd.viewParms.world;
						// we have to reset the shaderTime as well otherwise image animations on
						// the world (like water) continue with the wrong frame
						R_TransformDlights(backEnd.refdef.num_dlights, backEnd.refdef.dlights, &backEnd.ori);
					}

					GL_SetModelviewMatrix(backEnd.ori.modelMatrix);

					//
					// change depthrange. Also change projection matrix so first person weapon does not look like coming
					// out of the screen.
					//
					if (oldDepthRange != depthRange)
					{
						switch (depthRange) {
						default:
						case 0:
							if (backEnd.viewParms.stereoFrame != STEREO_CENTER)
							{
								GL_SetProjectionMatrix(backEnd.viewParms.projectionMatrix);
							}

							if (!sunflare)
								qglDepthRange(0.0f, 1.0f);

							depth[0] = 0;
							depth[1] = 1;
							break;

						case 1:
							if (backEnd.viewParms.stereoFrame != STEREO_CENTER)
							{
								viewParms_t temp = backEnd.viewParms;

								R_SetupProjection(&temp, r_znear->value, 0, qfalse);

								GL_SetProjectionMatrix(temp.projectionMatrix);
							}

							if (!oldDepthRange)
								qglDepthRange(0.0f, 0.3f);

							break;

						case 2:
							if (backEnd.viewParms.stereoFrame != STEREO_CENTER)
							{
								viewParms_t temp = backEnd.viewParms;

								R_SetupProjection(&temp, r_znear->value, 0, qfalse);

								GL_SetProjectionMatrix(temp.projectionMatrix);
							}

							if (!oldDepthRange)
								qglDepthRange(0.0f, 0.0f);

							break;
						}

						oldDepthRange = depthRange;
					}

					oldEntityNum = entityNum;
				}

				// add the triangles for this surface
				rb_surfaceTable[*drawSurf->surface](drawSurf->surface);
#ifdef __DEBUG_MERGE__
				numShaderDraws++;
#endif //__DEBUG_MERGE__
			}

			j++;
		}

#ifdef __DEBUG_MERGE__
		ri->Printf(PRINT_ALL, "%i total surface draws. %i shader changes.\n", numShaderDraws, numShaderChanges);
#endif //__DEBUG_MERGE__
	}

	backEnd.refdef.floatTime = originalTime;

	// draw the contents of the last shader batch
	if (oldShader != NULL) {
		RB_EndSurface();
	}

	if (inQuery) {
		qglEndQuery(GL_SAMPLES_PASSED);
	}

	FBO_Bind(fbo);

	// go back to the world modelview matrix

	GL_SetModelviewMatrix(backEnd.viewParms.world.modelMatrix);

	// Restore depth range for subsequent rendering
	qglDepthRange(0.0f, 1.0f);
}
#endif //__MERGE_MODEL_SURFACES2__


/*
============================================================================

RENDER BACK END FUNCTIONS

============================================================================
*/

/*
================
RB_SetGL2D

================
*/
void	RB_SetGL2D (void) {
	matrix_t matrix;
	int width, height;

	if (backEnd.projection2D && backEnd.last2DFBO == glState.currentFBO)
		return;

	backEnd.projection2D = qtrue;
	backEnd.last2DFBO = glState.currentFBO;

	if (glState.currentFBO)
	{
		width = glState.currentFBO->width;
		height = glState.currentFBO->height;
	}
	else
	{
		width = glConfig.vidWidth * r_superSampleMultiplier->value;
		height = glConfig.vidHeight * r_superSampleMultiplier->value;
	}

	// set 2D virtual screen size
	qglViewport( 0, 0, width, height );
	qglScissor( 0, 0, width, height );

	Matrix16Ortho(0, 640, 480, 0, 0, 1, matrix);
	GL_SetProjectionMatrix(matrix);
	Matrix16Identity(matrix);
	GL_SetModelviewMatrix(matrix);

	GL_State( GLS_DEPTHTEST_DISABLE |
			  GLS_SRCBLEND_SRC_ALPHA |
			  GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );

	qglDisable( GL_CULL_FACE );
	qglDisable( GL_CLIP_PLANE0 );

	// set time for 2D shaders
	backEnd.refdef.time = ri->Milliseconds();
	backEnd.refdef.floatTime = backEnd.refdef.time * 0.001f;

	// reset color scaling
	backEnd.refdef.colorScale = 1.0f;
}


/*
=============
RE_StretchRaw

FIXME: not exactly backend
Stretches a raw 32 bit power of 2 bitmap image over the given screen rectangle.
Used for cinematics.
=============
*/
void RE_StretchRaw (int x, int y, int w, int h, int cols, int rows, const byte *data, int client, qboolean dirty) {
	int			i, j;
	int			start, end;
	vec4_t quadVerts[4];
	vec2_t texCoords[4];

	if ( !tr.registered ) {
		return;
	}
	R_IssuePendingRenderCommands();

	if ( tess.numIndexes ) {
		RB_EndSurface();
	}

	// we definately want to sync every frame for the cinematics
#ifdef __USE_QGL_FINISH__
	qglFinish();
#else if __USE_QGL_FLUSH__
	qglFlush();
#endif //__USE_QGL_FINISH__

	start = 0;
	if ( r_speeds->integer ) {
		start = ri->Milliseconds();
	}

	// make sure rows and cols are powers of 2
	for ( i = 0 ; ( 1 << i ) < cols ; i++ ) {
	}
	for ( j = 0 ; ( 1 << j ) < rows ; j++ ) {
	}
	if ( ( 1 << i ) != cols || ( 1 << j ) != rows) {
		ri->Error (ERR_DROP, "Draw_StretchRaw: size not a power of 2: %i by %i", cols, rows);
	}

	RE_UploadCinematic (cols, rows, data, client, dirty);

	if ( r_speeds->integer ) {
		end = ri->Milliseconds();
		ri->Printf( PRINT_ALL, "qglTexSubImage2D %i, %i: %i msec\n", cols, rows, end - start );
	}

	// FIXME: HUGE hack
	if (!tr.renderFbo || backEnd.framePostProcessed)
	{
		FBO_Bind(NULL);
	}
	else
	{
		FBO_Bind(tr.renderFbo);
	}

	RB_SetGL2D();

	VectorSet4(quadVerts[0], x,     y,     0.0f, 1.0f);
	VectorSet4(quadVerts[1], x + w, y,     0.0f, 1.0f);
	VectorSet4(quadVerts[2], x + w, y + h, 0.0f, 1.0f);
	VectorSet4(quadVerts[3], x,     y + h, 0.0f, 1.0f);

	VectorSet2(texCoords[0], 0.5f / cols,          0.5f / rows);
	VectorSet2(texCoords[1], (cols - 0.5f) / cols, 0.5f / rows);
	VectorSet2(texCoords[2], (cols - 0.5f) / cols, (rows - 0.5f) / rows);
	VectorSet2(texCoords[3], 0.5f / cols,          (rows - 0.5f) / rows);

	GLSL_BindProgram(&tr.textureColorShader);
	
	GLSL_SetUniformMatrix16(&tr.textureColorShader, UNIFORM_MODELVIEWPROJECTIONMATRIX, glState.modelviewProjection);
	GLSL_SetUniformVec4(&tr.textureColorShader, UNIFORM_COLOR, colorWhite);

	RB_InstantQuad2(quadVerts, texCoords);
}

void RE_UploadCinematic (int cols, int rows, const byte *data, int client, qboolean dirty) {

	GL_Bind( tr.scratchImage[client] );

	// if the scratchImage isn't in the format we want, specify it as a new texture
	if ( cols != tr.scratchImage[client]->width || rows != tr.scratchImage[client]->height ) {
		tr.scratchImage[client]->width = tr.scratchImage[client]->uploadWidth = cols;
		tr.scratchImage[client]->height = tr.scratchImage[client]->uploadHeight = rows;
		qglTexImage2D( GL_TEXTURE_2D, 0, GL_RGB8, cols, rows, 0, GL_RGBA, GL_UNSIGNED_BYTE, data );
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );	
	} else {
		if (dirty) {
			// otherwise, just subimage upload it so that drivers can tell we are going to be changing
			// it and don't try and do a texture compression
			qglTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, cols, rows, GL_RGBA, GL_UNSIGNED_BYTE, data );
		}
	}
}


/*
=============
RB_SetColor

=============
*/
const void	*RB_SetColor( const void *data ) {
	const setColorCommand_t	*cmd;

	cmd = (const setColorCommand_t *)data;

	backEnd.color2D[0] = cmd->color[0] * 255;
	backEnd.color2D[1] = cmd->color[1] * 255;
	backEnd.color2D[2] = cmd->color[2] * 255;
	backEnd.color2D[3] = cmd->color[3] * 255;

	return (const void *)(cmd + 1);
}

/*
=============
RB_StretchPic
=============
*/
const void *RB_StretchPic ( const void *data ) {
	const stretchPicCommand_t	*cmd;
	shader_t *shader;
	int		numVerts, numIndexes;

	cmd = (const stretchPicCommand_t *)data;

	// FIXME: HUGE hack
	if (!tr.renderFbo || backEnd.framePostProcessed)
	{
		FBO_Bind(NULL);
	}
	else
	{
		FBO_Bind(tr.renderFbo);
	}

	RB_SetGL2D();

	shader = cmd->shader;
	if ( shader != tess.shader ) {
		if ( tess.numIndexes ) {
			RB_EndSurface();
		}
		backEnd.currentEntity = &backEnd.entity2D;
		RB_BeginSurface( shader, 0, 0 );
	}

	RB_CHECKOVERFLOW( 4, 6 );
	numVerts = tess.numVertexes;
	numIndexes = tess.numIndexes;

	tess.numVertexes += 4;
	tess.numIndexes += 6;

	tess.indexes[ numIndexes ] = numVerts + 3;
	tess.indexes[ numIndexes + 1 ] = numVerts + 0;
	tess.indexes[ numIndexes + 2 ] = numVerts + 2;
	tess.indexes[ numIndexes + 3 ] = numVerts + 2;
	tess.indexes[ numIndexes + 4 ] = numVerts + 0;
	tess.indexes[ numIndexes + 5 ] = numVerts + 1;

	{
		vec4_t color;

		VectorScale4(backEnd.color2D, 1.0f / 255.0f, color);

		VectorCopy4(color, tess.vertexColors[numVerts]);
		VectorCopy4(color, tess.vertexColors[numVerts + 1]);
		VectorCopy4(color, tess.vertexColors[numVerts + 2]);
		VectorCopy4(color, tess.vertexColors[numVerts + 3]);
	}


	tess.xyz[ numVerts ][0] = cmd->x;
	tess.xyz[ numVerts ][1] = cmd->y;
	tess.xyz[ numVerts ][2] = 0;

	tess.texCoords[ numVerts ][0][0] = cmd->s1;
	tess.texCoords[ numVerts ][0][1] = cmd->t1;

	tess.xyz[ numVerts + 1 ][0] = cmd->x + cmd->w;
	tess.xyz[ numVerts + 1 ][1] = cmd->y;
	tess.xyz[ numVerts + 1 ][2] = 0;

	tess.texCoords[ numVerts + 1 ][0][0] = cmd->s2;
	tess.texCoords[ numVerts + 1 ][0][1] = cmd->t1;

	tess.xyz[ numVerts + 2 ][0] = cmd->x + cmd->w;
	tess.xyz[ numVerts + 2 ][1] = cmd->y + cmd->h;
	tess.xyz[ numVerts + 2 ][2] = 0;

	tess.texCoords[ numVerts + 2 ][0][0] = cmd->s2;
	tess.texCoords[ numVerts + 2 ][0][1] = cmd->t2;

	tess.xyz[ numVerts + 3 ][0] = cmd->x;
	tess.xyz[ numVerts + 3 ][1] = cmd->y + cmd->h;
	tess.xyz[ numVerts + 3 ][2] = 0;

	tess.texCoords[ numVerts + 3 ][0][0] = cmd->s1;
	tess.texCoords[ numVerts + 3 ][0][1] = cmd->t2;

	return (const void *)(cmd + 1);
}

/*
=============
RB_DrawRotatePic
=============
*/
const void *RB_RotatePic ( const void *data ) 
{
	const rotatePicCommand_t	*cmd;
	shader_t *shader;

	cmd = (const rotatePicCommand_t *)data;

	// FIXME: HUGE hack
	if (!tr.renderFbo || backEnd.framePostProcessed)
	{
		FBO_Bind(NULL);
	}
	else
	{
		FBO_Bind(tr.renderFbo);
	}

	RB_SetGL2D();

	shader = cmd->shader;
	if ( shader != tess.shader ) {
		if ( tess.numIndexes ) {
			RB_EndSurface();
		}
		backEnd.currentEntity = &backEnd.entity2D;
		RB_BeginSurface( shader, 0, 0 );
	}

	RB_CHECKOVERFLOW( 4, 6 );
	int numVerts = tess.numVertexes;
	int numIndexes = tess.numIndexes;

	float angle = DEG2RAD(cmd->a);
	float s = sinf(angle);
	float c = cosf(angle);

	matrix3_t m = {
		{ c, s, 0.0f },
		{ -s, c, 0.0f },
		{ cmd->x + cmd->w, cmd->y, 1.0f }
	};

	tess.numVertexes += 4;
	tess.numIndexes += 6;

	tess.indexes[ numIndexes ] = numVerts + 3;
	tess.indexes[ numIndexes + 1 ] = numVerts + 0;
	tess.indexes[ numIndexes + 2 ] = numVerts + 2;
	tess.indexes[ numIndexes + 3 ] = numVerts + 2;
	tess.indexes[ numIndexes + 4 ] = numVerts + 0;
	tess.indexes[ numIndexes + 5 ] = numVerts + 1;

	{
		vec4_t color;

		VectorScale4(backEnd.color2D, 1.0f / 255.0f, color);

		VectorCopy4(color, tess.vertexColors[numVerts]);
		VectorCopy4(color, tess.vertexColors[numVerts + 1]);
		VectorCopy4(color, tess.vertexColors[numVerts + 2]);
		VectorCopy4(color, tess.vertexColors[numVerts + 3]);
	}

	tess.xyz[numVerts][0] = m[0][0] * (-cmd->w) + m[2][0];
	tess.xyz[numVerts][1] = m[0][1] * (-cmd->w) + m[2][1];
	tess.xyz[numVerts][2] = 0;

	tess.texCoords[numVerts][0][0] = cmd->s1;
	tess.texCoords[numVerts][0][1] = cmd->t1;

	tess.xyz[numVerts + 1][0] = m[2][0];
	tess.xyz[numVerts + 1][1] = m[2][1];
	tess.xyz[numVerts + 1][2] = 0;

	tess.texCoords[numVerts + 1][0][0] = cmd->s2;
	tess.texCoords[numVerts + 1][0][1] = cmd->t1;

	tess.xyz[numVerts + 2][0] = m[1][0] * (cmd->h) + m[2][0];
	tess.xyz[numVerts + 2][1] = m[1][1] * (cmd->h) + m[2][1];
	tess.xyz[numVerts + 2][2] = 0;

	tess.texCoords[numVerts + 2][0][0] = cmd->s2;
	tess.texCoords[numVerts + 2][0][1] = cmd->t2;

	tess.xyz[numVerts + 3][0] = m[0][0] * (-cmd->w) + m[1][0] * (cmd->h) + m[2][0];
	tess.xyz[numVerts + 3][1] = m[0][1] * (-cmd->w) + m[1][1] * (cmd->h) + m[2][1];
	tess.xyz[numVerts + 3][2] = 0;

	tess.texCoords[numVerts + 3][0][0] = cmd->s1;
	tess.texCoords[numVerts + 3][0][1] = cmd->t2;

	return (const void *)(cmd + 1);
}

/*
=============
RB_DrawRotatePic2
=============
*/
const void *RB_RotatePic2 ( const void *data ) 
{
	const rotatePicCommand_t	*cmd;
	shader_t *shader;

	cmd = (const rotatePicCommand_t *)data;

	// FIXME: HUGE hack
	if (!tr.renderFbo || backEnd.framePostProcessed)
	{
		FBO_Bind(NULL);
	}
	else
	{
		FBO_Bind(tr.renderFbo);
	}

	RB_SetGL2D();

	shader = cmd->shader;
	if ( shader != tess.shader ) {
		if ( tess.numIndexes ) {
			RB_EndSurface();
		}
		backEnd.currentEntity = &backEnd.entity2D;
		RB_BeginSurface( shader, 0, 0 );
	}

	RB_CHECKOVERFLOW( 4, 6 );
	int numVerts = tess.numVertexes;
	int numIndexes = tess.numIndexes;

	float angle = DEG2RAD(cmd->a);
	float s = sinf(angle);
	float c = cosf(angle);

	matrix3_t m = {
		{ c, s, 0.0f },
		{ -s, c, 0.0f },
		{ cmd->x, cmd->y, 1.0f }
	};

	tess.numVertexes += 4;
	tess.numIndexes += 6;

	tess.indexes[ numIndexes ] = numVerts + 3;
	tess.indexes[ numIndexes + 1 ] = numVerts + 0;
	tess.indexes[ numIndexes + 2 ] = numVerts + 2;
	tess.indexes[ numIndexes + 3 ] = numVerts + 2;
	tess.indexes[ numIndexes + 4 ] = numVerts + 0;
	tess.indexes[ numIndexes + 5 ] = numVerts + 1;

	{
		vec4_t color;

		VectorScale4(backEnd.color2D, 1.0f / 255.0f, color);

		VectorCopy4(color, tess.vertexColors[numVerts]);
		VectorCopy4(color, tess.vertexColors[numVerts + 1]);
		VectorCopy4(color, tess.vertexColors[numVerts + 2]);
		VectorCopy4(color, tess.vertexColors[numVerts + 3]);
	}

	tess.xyz[numVerts][0] = m[0][0] * (-cmd->w * 0.5f) + m[1][0] * (-cmd->h * 0.5f) + m[2][0];
	tess.xyz[numVerts][1] = m[0][1] * (-cmd->w * 0.5f) + m[1][1] * (-cmd->h * 0.5f) + m[2][1];
	tess.xyz[numVerts][2] = 0;

	tess.texCoords[numVerts][0][0] = cmd->s1;
	tess.texCoords[numVerts][0][1] = cmd->t1;

	tess.xyz[numVerts + 1][0] = m[0][0] * (cmd->w * 0.5f) + m[1][0] * (-cmd->h * 0.5f) + m[2][0];
	tess.xyz[numVerts + 1][1] = m[0][1] * (cmd->w * 0.5f) + m[1][1] * (-cmd->h * 0.5f) + m[2][1];
	tess.xyz[numVerts + 1][2] = 0;

	tess.texCoords[numVerts + 1][0][0] = cmd->s2;
	tess.texCoords[numVerts + 1][0][1] = cmd->t1;

	tess.xyz[numVerts + 2][0] = m[0][0] * (cmd->w * 0.5f) + m[1][0] * (cmd->h * 0.5f) + m[2][0];
	tess.xyz[numVerts + 2][1] = m[0][1] * (cmd->w * 0.5f) + m[1][1] * (cmd->h * 0.5f) + m[2][1];
	tess.xyz[numVerts + 2][2] = 0;

	tess.texCoords[numVerts + 2][0][0] = cmd->s2;
	tess.texCoords[numVerts + 2][0][1] = cmd->t2;

	tess.xyz[numVerts + 3][0] = m[0][0] * (-cmd->w * 0.5f) + m[1][0] * (cmd->h * 0.5f) + m[2][0];
	tess.xyz[numVerts + 3][1] = m[0][1] * (-cmd->w * 0.5f) + m[1][1] * (cmd->h * 0.5f) + m[2][1];
	tess.xyz[numVerts + 3][2] = 0;

	tess.texCoords[numVerts + 3][0][0] = cmd->s1;
	tess.texCoords[numVerts + 3][0][1] = cmd->t2;

	return (const void *)(cmd + 1);
}

/*
=============
RB_DrawSurfs

=============
*/

const void	*RB_DrawSurfs( const void *data ) {
	const drawSurfsCommand_t	*cmd;

	// finish any 2D drawing if needed
	if ( tess.numIndexes ) {
		RB_EndSurface();
	}

	cmd = (const drawSurfsCommand_t *)data;

	backEnd.refdef = cmd->refdef;
	backEnd.viewParms = cmd->viewParms;

	// clear the z buffer, set the modelview, etc
	RB_BeginDrawingView ();

	if (backEnd.viewParms.flags & VPF_DEPTHCLAMP)
	{
		qglEnable(GL_DEPTH_CLAMP);
	}

	if (!(backEnd.refdef.rdflags & RDF_NOWORLDMODEL) && (r_depthPrepass->integer || (backEnd.viewParms.flags & VPF_DEPTHSHADOW)))
	{
		FBO_t *oldFbo = glState.currentFBO;
#ifdef __DEPTH_PREPASS_OCCLUSION__
		qboolean oldColorMask[4];
		oldColorMask[0] = (qboolean)!backEnd.colorMask[0];
		oldColorMask[1] = (qboolean)!backEnd.colorMask[1];
		oldColorMask[2] = (qboolean)!backEnd.colorMask[2];
		oldColorMask[3] = (qboolean)!backEnd.colorMask[3];
#endif //__DEPTH_PREPASS_OCCLUSION__

		backEnd.depthFill = qtrue;
		qglColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
		RB_RenderDrawSurfList( cmd->drawSurfs, cmd->numDrawSurfs, qfalse );
		qglColorMask(!backEnd.colorMask[0], !backEnd.colorMask[1], !backEnd.colorMask[2], !backEnd.colorMask[3]);
		backEnd.depthFill = qfalse;

#ifdef __DEPTH_PREPASS_OCCLUSION__
		if (r_occlusion->integer /*&& !(backEnd.viewParms.flags & VPF_DEPTHSHADOW)*/)
		{
			backEnd.depthFill = qtrue;
			qglColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
			qglDepthMask(GL_FALSE);
			qglDepthFunc(GL_LEQUAL);
			//qglDepthRange(0, 0);
			RB_RenderDrawSurfList(cmd->drawSurfs, cmd->numDrawSurfs, qtrue);
			qglColorMask(oldColorMask[0], oldColorMask[1], oldColorMask[2], oldColorMask[3]);
			qglFlush();
			backEnd.depthFill = qfalse;
		}
#endif //__DEPTH_PREPASS_OCCLUSION__

#if 0
		if (tr.msaaResolveFbo)
		{
			// If we're using multisampling, resolve the depth first
			FBO_FastBlit(tr.renderFbo, NULL, tr.msaaResolveFbo, NULL, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
		}
		else 
#endif
		if (tr.renderFbo == NULL)
		{
			// If we're rendering directly to the screen, copy the depth to a texture
			GL_BindToTMU(tr.renderDepthImage, 0);
			qglCopyTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, 0, 0, glConfig.vidWidth * r_superSampleMultiplier->value, glConfig.vidHeight * r_superSampleMultiplier->value, 0);
		}

		if (r_ssao->integer || r_hbao->integer)
		{
			// need the depth in a texture we can do GL_LINEAR sampling on, so copy it to an HDR image
			FBO_BlitFromTexture(tr.renderDepthImage, NULL, NULL, tr.hdrDepthFbo, NULL, NULL, NULL, 0);
		}

		if (r_sunlightMode->integer >= 2 && tr.screenShadowFbo && backEnd.viewParms.flags & VPF_USESUNLIGHT)
		{
			vec4_t quadVerts[4];
			vec2_t texCoords[4];
			vec4_t box;

			FBO_Bind(tr.screenShadowFbo);

			box[0] = backEnd.viewParms.viewportX      * tr.screenShadowFbo->width  / ((float)glConfig.vidWidth * r_superSampleMultiplier->value);
			box[1] = backEnd.viewParms.viewportY      * tr.screenShadowFbo->height / ((float)glConfig.vidHeight * r_superSampleMultiplier->value);
			box[2] = backEnd.viewParms.viewportWidth  * tr.screenShadowFbo->width  / ((float)glConfig.vidWidth * r_superSampleMultiplier->value);
			box[3] = backEnd.viewParms.viewportHeight * tr.screenShadowFbo->height / ((float)glConfig.vidHeight * r_superSampleMultiplier->value);

			qglViewport(box[0], box[1], box[2], box[3]);
			qglScissor(box[0], box[1], box[2], box[3]);

			box[0] = backEnd.viewParms.viewportX               / ((float)glConfig.vidWidth * r_superSampleMultiplier->value);
			box[1] = backEnd.viewParms.viewportY               / ((float)glConfig.vidHeight * r_superSampleMultiplier->value);
			box[2] = box[0] + backEnd.viewParms.viewportWidth  / ((float)glConfig.vidWidth * r_superSampleMultiplier->value);
			box[3] = box[1] + backEnd.viewParms.viewportHeight / ((float)glConfig.vidHeight * r_superSampleMultiplier->value);

			texCoords[0][0] = box[0]; texCoords[0][1] = box[3];
			texCoords[1][0] = box[2]; texCoords[1][1] = box[3];
			texCoords[2][0] = box[2]; texCoords[2][1] = box[1];
			texCoords[3][0] = box[0]; texCoords[3][1] = box[1];

			box[0] = -1.0f;
			box[1] = -1.0f;
			box[2] =  1.0f;
			box[3] =  1.0f;

			VectorSet4(quadVerts[0], box[0], box[3], 0, 1);
			VectorSet4(quadVerts[1], box[2], box[3], 0, 1);
			VectorSet4(quadVerts[2], box[2], box[1], 0, 1);
			VectorSet4(quadVerts[3], box[0], box[1], 0, 1);

			GL_State( GLS_DEPTHTEST_DISABLE );

			GLSL_BindProgram(&tr.shadowmaskShader);

			GL_BindToTMU(tr.renderDepthImage, TB_COLORMAP);
			GL_BindToTMU(tr.sunShadowDepthImage[0], TB_SHADOWMAP);
			GLSL_SetUniformMatrix16(&tr.shadowmaskShader, UNIFORM_SHADOWMVP,  backEnd.refdef.sunShadowMvp[0]);
			
			if (r_sunlightMode->integer >= 3)
			{
				GL_BindToTMU(tr.sunShadowDepthImage[1], TB_SHADOWMAP2);
				GLSL_SetUniformMatrix16(&tr.shadowmaskShader, UNIFORM_SHADOWMVP2, backEnd.refdef.sunShadowMvp[1]);
				
				if (r_sunlightMode->integer >= 4)
				{
					GL_BindToTMU(tr.sunShadowDepthImage[2], TB_SHADOWMAP3);
					GLSL_SetUniformMatrix16(&tr.shadowmaskShader, UNIFORM_SHADOWMVP3, backEnd.refdef.sunShadowMvp[2]);
				}
			}
			
			GLSL_SetUniformInt(&tr.shadowmaskShader, UNIFORM_SPECULARMAP, TB_SPECULARMAP);
			GL_BindToTMU(tr.randomImage, TB_SPECULARMAP);
			
			GLSL_SetUniformVec3(&tr.shadowmaskShader, UNIFORM_VIEWORIGIN,  backEnd.refdef.vieworg);

			{
				vec4_t viewInfo;
				vec3_t viewVector;

				float zmax = backEnd.viewParms.zFar;
				float ymax = zmax * tan(backEnd.viewParms.fovY * M_PI / 360.0f);
				float xmax = zmax * tan(backEnd.viewParms.fovX * M_PI / 360.0f);

				float zmin = r_znear->value;

				VectorScale(backEnd.refdef.viewaxis[0], zmax, viewVector);
				GLSL_SetUniformVec3(&tr.shadowmaskShader, UNIFORM_VIEWFORWARD, viewVector);
				VectorScale(backEnd.refdef.viewaxis[1], xmax, viewVector);
				GLSL_SetUniformVec3(&tr.shadowmaskShader, UNIFORM_VIEWLEFT,    viewVector);
				VectorScale(backEnd.refdef.viewaxis[2], ymax, viewVector);
				GLSL_SetUniformVec3(&tr.shadowmaskShader, UNIFORM_VIEWUP,      viewVector);

				VectorSet4(viewInfo, zmax / zmin, zmax, 0.0, 0.0);

				GLSL_SetUniformVec4(&tr.shadowmaskShader, UNIFORM_VIEWINFO, viewInfo);
			}


			RB_InstantQuad2(quadVerts, texCoords); //, color, shaderProgram, invTexRes);

			{
				FBO_t *currentFbo = tr.genericFbo3;
				FBO_t *currentOutFbo = tr.genericFbo;

				FBO_FastBlit(tr.screenShadowFbo, NULL, currentFbo, NULL, GL_COLOR_BUFFER_BIT, GL_NEAREST);
				
				// Blur some times
				float	spread = 1.0f;

				for ( int i = 0; i < r_shadowBlurPasses->integer; i++ )
				{
					//RB_GaussianBlur(currentFbo, tr.genericFbo2, currentOutFbo, spread);
					//RB_BokehBlur(currentFbo, NULL, currentOutFbo, NULL, backEnd.refdef.blurFactor);
					RB_FastBlur(currentFbo, NULL, currentOutFbo, NULL);
					RB_SwapFBOs( &currentFbo, &currentOutFbo);
					//spread += 0.6f * 0.25f;
				}

				FBO_FastBlit(currentFbo, NULL, tr.screenShadowFbo, NULL, GL_COLOR_BUFFER_BIT, GL_NEAREST);
			}
			//RB_BokehBlur(NULL, srcBox, NULL, dstBox, backEnd.refdef.blurFactor);
		}

#ifdef __DYNAMIC_SHADOWS__
		//if (backEnd.viewParms.flags & VPF_USESUNLIGHT)
		{
			if ( backEnd.refdef.num_dlights )
			{
				for ( int l = 0 ; l < backEnd.refdef.num_dlights ; l++ ) {
					dlight_t	*dl;
					float radius, scale;
					vec3_t origin;

					if (l >= MAX_DYNAMIC_SHADOWS) break; // max

					dl = &backEnd.refdef.dlights[l];
					VectorCopy( dl->transformed, origin );
					radius = dl->radius;
					scale = 1.0f / radius;

					if (!dl->activeShadows) continue;

					vec4_t quadVerts[4];
					vec2_t texCoords[4];
					vec4_t box;

					FBO_Bind(tr.screenShadowFbo);

					box[0] = backEnd.viewParms.viewportX      * tr.screenShadowFbo->width  / ((float)glConfig.vidWidth * r_superSampleMultiplier->value);
					box[1] = backEnd.viewParms.viewportY      * tr.screenShadowFbo->height / ((float)glConfig.vidHeight * r_superSampleMultiplier->value);
					box[2] = backEnd.viewParms.viewportWidth  * tr.screenShadowFbo->width  / ((float)glConfig.vidWidth * r_superSampleMultiplier->value);
					box[3] = backEnd.viewParms.viewportHeight * tr.screenShadowFbo->height / ((float)glConfig.vidHeight * r_superSampleMultiplier->value);

					qglViewport(box[0], box[1], box[2], box[3]);
					qglScissor(box[0], box[1], box[2], box[3]);

					box[0] = backEnd.viewParms.viewportX               / ((float)glConfig.vidWidth * r_superSampleMultiplier->value);
					box[1] = backEnd.viewParms.viewportY               / ((float)glConfig.vidHeight * r_superSampleMultiplier->value);
					box[2] = box[0] + backEnd.viewParms.viewportWidth  / ((float)glConfig.vidWidth * r_superSampleMultiplier->value);
					box[3] = box[1] + backEnd.viewParms.viewportHeight / ((float)glConfig.vidHeight * r_superSampleMultiplier->value);

					texCoords[0][0] = box[0]; texCoords[0][1] = box[3];
					texCoords[1][0] = box[2]; texCoords[1][1] = box[3];
					texCoords[2][0] = box[2]; texCoords[2][1] = box[1];
					texCoords[3][0] = box[0]; texCoords[3][1] = box[1];

					box[0] = -1.0f;
					box[1] = -1.0f;
					box[2] =  1.0f;
					box[3] =  1.0f;

					VectorSet4(quadVerts[0], box[0], box[3], 0, 1);
					VectorSet4(quadVerts[1], box[2], box[3], 0, 1);
					VectorSet4(quadVerts[2], box[2], box[1], 0, 1);
					VectorSet4(quadVerts[3], box[0], box[1], 0, 1);

					GL_State( GLS_DEPTHTEST_DISABLE );

					GLSL_BindProgram(&tr.shadowmaskShader);

					GL_BindToTMU(tr.renderDepthImage, TB_COLORMAP);
					GL_BindToTMU(tr.dlightShadowDepthImage[l][0], TB_SHADOWMAP);
					GL_BindToTMU(tr.dlightShadowDepthImage[l][1], TB_SHADOWMAP2);
					GL_BindToTMU(tr.dlightShadowDepthImage[l][2], TB_SHADOWMAP3);

					GLSL_SetUniformMatrix16(&tr.shadowmaskShader, UNIFORM_SHADOWMVP,  backEnd.refdef.dlightShadowMvp[l][0]);
					GLSL_SetUniformMatrix16(&tr.shadowmaskShader, UNIFORM_SHADOWMVP2, backEnd.refdef.dlightShadowMvp[l][1]);
					GLSL_SetUniformMatrix16(&tr.shadowmaskShader, UNIFORM_SHADOWMVP3, backEnd.refdef.dlightShadowMvp[l][2]);

					GLSL_SetUniformVec3(&tr.shadowmaskShader, UNIFORM_VIEWORIGIN,  backEnd.refdef.vieworg);
					{
						vec4_t viewInfo;
						vec3_t viewVector;

						float zmax = backEnd.viewParms.zFar;
						float ymax = zmax * tan(backEnd.viewParms.fovY * M_PI / 360.0f);
						float xmax = zmax * tan(backEnd.viewParms.fovX * M_PI / 360.0f);

						float zmin = r_znear->value;

						VectorScale(backEnd.refdef.viewaxis[0], zmax, viewVector);
						GLSL_SetUniformVec3(&tr.shadowmaskShader, UNIFORM_VIEWFORWARD, viewVector);
						VectorScale(backEnd.refdef.viewaxis[1], xmax, viewVector);
						GLSL_SetUniformVec3(&tr.shadowmaskShader, UNIFORM_VIEWLEFT,    viewVector);
						VectorScale(backEnd.refdef.viewaxis[2], ymax, viewVector);
						GLSL_SetUniformVec3(&tr.shadowmaskShader, UNIFORM_VIEWUP,      viewVector);

						VectorSet4(viewInfo, zmax / zmin, zmax, 0.0, 0.0);

						GLSL_SetUniformVec4(&tr.shadowmaskShader, UNIFORM_VIEWINFO, viewInfo);
					}


					RB_InstantQuad2(quadVerts, texCoords); //, color, shaderProgram, invTexRes);
				}
			}
		}
#endif //__DYNAMIC_SHADOWS__

		/*
		if (r_ssao->integer)
		{
			vec4_t quadVerts[4];
			vec2_t texCoords[4];

			FBO_Bind(tr.quarterFbo[0]);

			qglViewport(0, 0, tr.quarterFbo[0]->width, tr.quarterFbo[0]->height);
			qglScissor(0, 0, tr.quarterFbo[0]->width, tr.quarterFbo[0]->height);

			VectorSet4(quadVerts[0], -1,  1, 0, 1);
			VectorSet4(quadVerts[1],  1,  1, 0, 1);
			VectorSet4(quadVerts[2],  1, -1, 0, 1);
			VectorSet4(quadVerts[3], -1, -1, 0, 1);

			texCoords[0][0] = 0; texCoords[0][1] = 1;
			texCoords[1][0] = 1; texCoords[1][1] = 1;
			texCoords[2][0] = 1; texCoords[2][1] = 0;
			texCoords[3][0] = 0; texCoords[3][1] = 0;

			GL_State( GLS_DEPTHTEST_DISABLE );

			GLSL_BindProgram(&tr.ssaoShader);

			GL_BindToTMU(tr.hdrDepthImage, TB_COLORMAP);

			{
				vec4_t viewInfo;

				float zmax = backEnd.viewParms.zFar;
				float zmin = r_znear->value;

				VectorSet4(viewInfo, zmax / zmin, zmax, 0.0, 0.0);

				GLSL_SetUniformVec4(&tr.ssaoShader, UNIFORM_VIEWINFO, viewInfo);
			}

			{
				vec2_t screensize;
				screensize[0] = tr.quarterFbo[0]->width;
				screensize[1] = tr.quarterFbo[0]->height;

				GLSL_SetUniformVec2(&tr.ssaoShader, UNIFORM_DIMENSIONS, screensize);
			}

			RB_InstantQuad2(quadVerts, texCoords); //, color, shaderProgram, invTexRes);


			FBO_Bind(tr.quarterFbo[1]);

			qglViewport(0, 0, tr.quarterFbo[1]->width, tr.quarterFbo[1]->height);
			qglScissor(0, 0, tr.quarterFbo[1]->width, tr.quarterFbo[1]->height);

			GLSL_BindProgram(&tr.depthBlurShader[0]);

			GL_BindToTMU(tr.quarterImage[0],  TB_COLORMAP);
			GL_BindToTMU(tr.hdrDepthImage, TB_LIGHTMAP);

			{
				vec4_t viewInfo;

				float zmax = backEnd.viewParms.zFar;
				float zmin = r_znear->value;

				VectorSet4(viewInfo, zmax / zmin, zmax, 0.0, 0.0);

				GLSL_SetUniformVec4(&tr.depthBlurShader[0], UNIFORM_VIEWINFO, viewInfo);
			}

			RB_InstantQuad2(quadVerts, texCoords); //, color, shaderProgram, invTexRes);


			FBO_Bind(tr.screenSsaoFbo);

			qglViewport(0, 0, tr.screenSsaoFbo->width, tr.screenSsaoFbo->height);
			qglScissor(0, 0, tr.screenSsaoFbo->width, tr.screenSsaoFbo->height);

			GLSL_BindProgram(&tr.depthBlurShader[1]);

			GL_BindToTMU(tr.quarterImage[1],  TB_COLORMAP);
			GL_BindToTMU(tr.hdrDepthImage, TB_LIGHTMAP);

			{
				vec4_t viewInfo;

				float zmax = backEnd.viewParms.zFar;
				float zmin = r_znear->value;

				VectorSet4(viewInfo, zmax / zmin, zmax, 0.0, 0.0);

				GLSL_SetUniformVec4(&tr.depthBlurShader[1], UNIFORM_VIEWINFO, viewInfo);
			}
		}
		*/

		// reset viewport and scissor
		FBO_Bind(oldFbo);
		SetViewportAndScissor();
	}

	if (backEnd.viewParms.flags & VPF_DEPTHCLAMP)
	{
		qglDisable(GL_DEPTH_CLAMP);
	}

	if (!(backEnd.viewParms.flags & VPF_DEPTHSHADOW))
	{
		RB_RenderDrawSurfList( cmd->drawSurfs, cmd->numDrawSurfs, qfalse );

		if (r_drawSun->integer)
		{
			RB_DrawSun(0.1, tr.sunShader);
		}

		if (r_drawSunRays->integer)
		{
			FBO_t *oldFbo = glState.currentFBO;
			FBO_Bind(tr.sunRaysFbo);
			
			qglClearColor( 0.0f, 0.0f, 0.0f, 1.0f );
			qglClear( GL_COLOR_BUFFER_BIT );

			tr.sunFlareQueryActive[tr.sunFlareQueryIndex] = qtrue;
			qglBeginQuery(GL_SAMPLES_PASSED, tr.sunFlareQuery[tr.sunFlareQueryIndex]);

			RB_DrawSun(0.3, tr.sunFlareShader);

			qglEndQuery(GL_SAMPLES_PASSED);

			FBO_Bind(oldFbo);
		}

		// darken down any stencil shadows
		RB_ShadowFinish();		

		// add light flares on lights that aren't obscured
		RB_RenderFlares();

#ifdef ___WARZONE_FLASH___
		gameswf_drawflash( "interface/test.swf" );
#endif //___WARZONE_FLASH___
	}

	if (tr.renderCubeFbo != NULL && backEnd.viewParms.targetFbo == tr.renderCubeFbo)
	{
		FBO_Bind(NULL);
		GL_SelectTexture(TB_CUBEMAP);
		GL_BindToTMU(tr.cubemaps[backEnd.viewParms.targetFboCubemapIndex], TB_CUBEMAP);
		qglGenerateMipmap(GL_TEXTURE_CUBE_MAP);
		GL_SelectTexture(0);
	}

	return (const void *)(cmd + 1);
}


/*
=============
RB_DrawBuffer

=============
*/
const void	*RB_DrawBuffer( const void *data ) {
	const drawBufferCommand_t	*cmd;

	cmd = (const drawBufferCommand_t *)data;

	// finish any 2D drawing if needed
	if(tess.numIndexes)
		RB_EndSurface();

	FBO_Bind(NULL);

	qglDrawBuffer( cmd->buffer );

	// clear screen for debugging
	if ( r_clear->integer ) {
		qglClearColor( 1, 0, 0.5, 1 );
		qglClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	}

	return (const void *)(cmd + 1);
}

/*
===============
RB_ShowImages

Draw all the images to the screen, on top of whatever
was there.  This is used to test for texture thrashing.

Also called by RE_EndRegistration
===============
*/
void RB_ShowImages( void ) {
	int		i;
	image_t	*image;
	float	x, y, w, h;
	int		start, end;

	RB_SetGL2D();

	qglClear(GL_COLOR_BUFFER_BIT);

	qglFinish();

	start = ri->Milliseconds();

	image = tr.images;
	for (i = 0; i < tr.numImages; i++, image = image->poolNext) {
		w = glConfig.vidWidth / 20;
		h = glConfig.vidHeight / 15;
		x = i % 20 * w;
		y = i / 20 * h;

		// show in proportional size in mode 2
		if (r_showImages->integer == 2) {
			w *= image->uploadWidth / 512.0f;
			h *= image->uploadHeight / 512.0f;
		}

		{
			vec4_t quadVerts[4];

			GL_Bind(image);

			VectorSet4(quadVerts[0], x, y, 0, 1);
			VectorSet4(quadVerts[1], x + w, y, 0, 1);
			VectorSet4(quadVerts[2], x + w, y + h, 0, 1);
			VectorSet4(quadVerts[3], x, y + h, 0, 1);

			RB_InstantQuad(quadVerts);
		}
	}

	qglFinish();

	end = ri->Milliseconds();
	ri->Printf(PRINT_ALL, "%i msec to draw all images\n", end - start);

}

/*
=============
RB_ColorMask

=============
*/
const void *RB_ColorMask(const void *data)
{
	const colorMaskCommand_t *cmd = (colorMaskCommand_t *)data;

	// finish any 2D drawing if needed
	if(tess.numIndexes)
		RB_EndSurface();

	// reverse color mask, so 0 0 0 0 is the default
	backEnd.colorMask[0] = (qboolean)(!cmd->rgba[0]);
	backEnd.colorMask[1] = (qboolean)(!cmd->rgba[1]);
	backEnd.colorMask[2] = (qboolean)(!cmd->rgba[2]);
	backEnd.colorMask[3] = (qboolean)(!cmd->rgba[3]);

	qglColorMask(cmd->rgba[0], cmd->rgba[1], cmd->rgba[2], cmd->rgba[3]);
	
	return (const void *)(cmd + 1);
}

/*
=============
RB_ClearDepth

=============
*/
const void *RB_ClearDepth(const void *data)
{
	const clearDepthCommand_t *cmd = (clearDepthCommand_t *)data;
	
	// finish any 2D drawing if needed
	if(tess.numIndexes)
		RB_EndSurface();

	// texture swapping test
	if (r_showImages->integer)
		RB_ShowImages();

	if (!tr.renderFbo || backEnd.framePostProcessed)
	{
		FBO_Bind(NULL);
	}
	else
	{
		FBO_Bind(tr.renderFbo);
	}

	qglClear(GL_DEPTH_BUFFER_BIT);

#if 0
	// if we're doing MSAA, clear the depth texture for the resolve buffer
	if (tr.msaaResolveFbo)
	{
		FBO_Bind(tr.msaaResolveFbo);
		qglClear(GL_DEPTH_BUFFER_BIT);
	}
#endif
	
	return (const void *)(cmd + 1);
}


/*
=============
RB_SwapBuffers

=============
*/
const void	*RB_SwapBuffers( const void *data ) {
#if 1
	const swapBuffersCommand_t	*cmd;

	// finish any 2D drawing if needed
	if ( tess.numIndexes ) {
		RB_EndSurface();
	}

	ResetGhoul2RenderableSurfaceHeap();

	// texture swapping test
	if ( r_showImages->integer ) {
		RB_ShowImages();
	}

	cmd = (const swapBuffersCommand_t *)data;

	// we measure overdraw by reading back the stencil buffer and
	// counting up the number of increments that have happened
	if ( r_measureOverdraw->integer ) {
		int i;
		long sum = 0;
		unsigned char *stencilReadback;

		stencilReadback = (unsigned char *)ri->Hunk_AllocateTempMemory( (glConfig.vidWidth * r_superSampleMultiplier->value) * (glConfig.vidHeight * r_superSampleMultiplier->value) );
		qglReadPixels( 0, 0, glConfig.vidWidth * r_superSampleMultiplier->value, glConfig.vidHeight * r_superSampleMultiplier->value, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, stencilReadback );

		for ( i = 0; i < (glConfig.vidWidth * r_superSampleMultiplier->value) * (glConfig.vidHeight * r_superSampleMultiplier->value); i++ ) {
			sum += stencilReadback[i];
		}

		backEnd.pc.c_overDraw += sum;
		ri->Hunk_FreeTempMemory( stencilReadback );
	}

	if (!backEnd.framePostProcessed)
	{
#if 0
		if (tr.msaaResolveFbo && r_hdr->integer)
		{
			// Resolving an RGB16F MSAA FBO to the screen messes with the brightness, so resolve to an RGB16F FBO first
			FBO_FastBlit(tr.renderFbo, NULL, tr.msaaResolveFbo, NULL, GL_COLOR_BUFFER_BIT, GL_NEAREST);
			FBO_FastBlit(tr.msaaResolveFbo, NULL, NULL, NULL, GL_COLOR_BUFFER_BIT, GL_NEAREST);
		}
		else 
#endif
		if (tr.renderFbo)
		{
			FBO_FastBlit(tr.renderFbo, NULL, NULL, NULL, GL_COLOR_BUFFER_BIT, GL_NEAREST);
		}
	}

#ifdef __USE_QGL_FINISH__
	if ( !glState.finishCalled ) {
		qglFinish();
	}
#else if __USE_QGL_FLUSH__
	if ( !glState.finishCalled ) {
		qglFlush();
	}
#endif //__USE_QGL_FINISH__

	GLimp_LogComment( "***************** RB_SwapBuffers *****************\n\n\n" );

	ri->WIN_Present(&window);

	backEnd.framePostProcessed = qfalse;
	backEnd.projection2D = qfalse;

	return (const void *)(cmd + 1);
#else
	const swapBuffersCommand_t	*cmd;

	// finish any 2D drawing if needed
	if (tess.numIndexes) {
		RB_EndSurface();
	}

	ResetGhoul2RenderableSurfaceHeap();

	// texture swapping test
	if (r_showImages->integer) {
		RB_ShowImages();
	}

	cmd = (const swapBuffersCommand_t *)data;

	// we measure overdraw by reading back the stencil buffer and
	// counting up the number of increments that have happened
	if (r_measureOverdraw->integer) {
		int i;
		long sum = 0;
		unsigned char *stencilReadback;

		stencilReadback = (unsigned char *)ri->Hunk_AllocateTempMemory(glConfig.vidWidth * glConfig.vidHeight);
		qglReadPixels(0, 0, glConfig.vidWidth, glConfig.vidHeight, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, stencilReadback);

		for (i = 0; i < glConfig.vidWidth * glConfig.vidHeight; i++) {
			sum += stencilReadback[i];
		}

		backEnd.pc.c_overDraw += sum;
		ri->Hunk_FreeTempMemory(stencilReadback);
	}

	if (!backEnd.framePostProcessed)
	{
		if (tr.renderFbo)
		{
			FBO_FastBlit(tr.renderFbo, NULL, NULL, NULL, GL_COLOR_BUFFER_BIT, GL_NEAREST);
		}
	}

	int frameNumber = backEndData->realFrameNumber;
	//gpuFrame_t *currentFrame = backEndData->currentFrame;

	//assert(!currentFrame->sync);
	//currentFrame->sync = qglFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);

	backEndData->realFrameNumber = frameNumber + 1;

	GLimp_LogComment("***************** RB_SwapBuffers *****************\n\n\n");

	ri->WIN_Present(&window);

	backEnd.framePostProcessed = qfalse;
	backEnd.projection2D = qfalse;

	return (const void *)(cmd + 1);
#endif
}

#ifdef __SURFACESPRITES__
extern void RB_RenderWorldEffects(void);

const void	*RB_WorldEffects( const void *data )
{
	const drawBufferCommand_t	*cmd;

	cmd = (const drawBufferCommand_t *)data;

	// Always flush the tess buffer
	if ( tess.shader && tess.numIndexes )
	{
		RB_EndSurface();
	}

	RB_RenderWorldEffects();

	if(tess.shader)
	{
		RB_BeginSurface( tess.shader, tess.fogNum, tess.cubemapIndex/*0*/ );
	}

	return (const void *)(cmd + 1);
}
#endif //__SURFACESPRITES__

/*
=============
RB_CapShadowMap

=============
*/
const void *RB_CapShadowMap(const void *data)
{
	const capShadowmapCommand_t *cmd = (const capShadowmapCommand_t *)data;

	// finish any 2D drawing if needed
	if(tess.numIndexes)
		RB_EndSurface();

	if (cmd->map != -1)
	{
		GL_SelectTexture(0);
		if (cmd->cubeSide != -1)
		{
			if (tr.shadowCubemaps[cmd->map] != NULL)
			{
				GL_Bind(tr.shadowCubemaps[cmd->map]);
				qglCopyTexSubImage2D( GL_TEXTURE_CUBE_MAP_POSITIVE_X + cmd->cubeSide, 0, 0, 0, backEnd.refdef.x, glConfig.vidHeight - ( backEnd.refdef.y + PSHADOW_MAP_SIZE ), PSHADOW_MAP_SIZE, PSHADOW_MAP_SIZE );
			}
		}
		else
		{
			if (tr.pshadowMaps[cmd->map] != NULL)
			{
				GL_Bind(tr.pshadowMaps[cmd->map]);
				qglCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, backEnd.refdef.x, glConfig.vidHeight - ( backEnd.refdef.y + PSHADOW_MAP_SIZE ), PSHADOW_MAP_SIZE, PSHADOW_MAP_SIZE );
			}
		}
	}

	return (const void *)(cmd + 1);
}


/*
=============
RB_PostProcess

=============
*/

extern qboolean FOG_POST_ENABLED;

extern void GLSL_AttachPostTextures( void );

const void *RB_PostProcess(const void *data)
{
	const postProcessCommand_t *cmd = (const postProcessCommand_t *)data;
	FBO_t *srcFbo;
	vec4i_t srcBox, dstBox;
	qboolean autoExposure;

	// finish any 2D drawing if needed
	if(tess.numIndexes)
		RB_EndSurface();

	if (!r_postProcess->integer || (tr.viewParms.flags & VPF_NOPOSTPROCESS))
	{
		// do nothing
		return (const void *)(cmd + 1);
	}

	if (cmd)
	{
		backEnd.refdef = cmd->refdef;
		backEnd.viewParms = cmd->viewParms;
	}

	srcFbo = tr.renderFbo;

#if 0
	if (tr.msaaResolveFbo)
	{
		// Resolve the MSAA before anything else
		// Can't resolve just part of the MSAA FBO, so multiple views will suffer a performance hit here
		FBO_FastBlit(tr.renderFbo, NULL, tr.msaaResolveFbo, NULL, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);
		srcFbo = tr.msaaResolveFbo;

		if ( r_dynamicGlow->integer || r_ssgi->integer || r_anamorphic->integer )
		{
			FBO_FastBlitIndexed(tr.renderFbo, tr.msaaResolveFbo, 1, 1, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);
		}
	}
#endif

	dstBox[0] = backEnd.viewParms.viewportX;
	dstBox[1] = backEnd.viewParms.viewportY;
	dstBox[2] = backEnd.viewParms.viewportWidth;
	dstBox[3] = backEnd.viewParms.viewportHeight;

	if (!(backEnd.refdef.rdflags & RDF_BLUR)
		&& !(tr.viewParms.flags & VPF_SHADOWPASS)
		&& !(backEnd.viewParms.flags & VPF_DEPTHSHADOW)
		&& !backEnd.depthFill
		&& (r_dynamicGlow->integer || r_ssgi->integer || r_anamorphic->integer || r_bloom->integer))
	{
		RB_BloomDownscale(tr.glowImage, tr.glowFboScaled[0]);
		int numPasses = Com_Clampi(1, ARRAY_LEN(tr.glowFboScaled), r_dynamicGlowPasses->integer);
		for ( int i = 1; i < numPasses; i++ )
			RB_BloomDownscale2(tr.glowFboScaled[i - 1], tr.glowFboScaled[i]);
 
		for ( int i = numPasses - 2; i >= 0; i-- )
			RB_BloomUpscale(tr.glowFboScaled[i + 1], tr.glowFboScaled[i]);
	}

	srcBox[0] = backEnd.viewParms.viewportX;
	srcBox[1] = backEnd.viewParms.viewportY;
	srcBox[2] = backEnd.viewParms.viewportWidth;
	srcBox[3] = backEnd.viewParms.viewportHeight;

	if (srcFbo)
	{
		FBO_FastBlit(tr.renderFbo, NULL, tr.genericFbo3, NULL, GL_COLOR_BUFFER_BIT, GL_NEAREST);

		FBO_t *currentFbo = tr.genericFbo3;
		FBO_t *currentOutFbo = tr.genericFbo;

		//
		// UQ1: Added...
		//
		qboolean SCREEN_BLUR = qfalse;

		if (backEnd.refdef.rdflags & RDF_BLUR)
		{// Skip most of the fancy stuff when doing a blured screen...
			SCREEN_BLUR = qtrue;
		}

		if (!SCREEN_BLUR && FOG_POST_ENABLED && r_fogPost->integer)
		{
			RB_FogPostShader(currentFbo, srcBox, currentOutFbo, dstBox);
			RB_SwapFBOs( &currentFbo, &currentOutFbo);
		}

		if (!SCREEN_BLUR && r_deferredLighting->integer)
		{
			RB_DeferredLighting(currentFbo, srcBox, currentOutFbo, dstBox);
			RB_SwapFBOs(&currentFbo, &currentOutFbo);
		}

		if (!SCREEN_BLUR && r_shownormals->integer)
		{
			RB_ShowNormals(currentFbo, srcBox, currentOutFbo, dstBox);
			RB_SwapFBOs(&currentFbo, &currentOutFbo);
		}

		if (!SCREEN_BLUR && r_testshader->integer)
		{
			RB_TestShader(currentFbo, srcBox, currentOutFbo, dstBox, 0);
			RB_SwapFBOs( &currentFbo, &currentOutFbo);
		}

		if (r_colorCorrection->integer)
		{
			RB_ColorCorrection(currentFbo, srcBox, currentOutFbo, dstBox);
			RB_SwapFBOs( &currentFbo, &currentOutFbo);
		}

		/*if (r_underwater->integer && (backEnd.refdef.rdflags & RDF_UNDERWATER))
		{
			RB_Underwater(currentFbo, srcBox, currentOutFbo, dstBox);
			RB_SwapFBOs( &currentFbo, &currentOutFbo);
		}*/

		if (!SCREEN_BLUR && r_sss->integer)
		{
			RB_SSS(currentFbo, srcBox, currentOutFbo, dstBox);
			RB_SwapFBOs( &currentFbo, &currentOutFbo);
		}

		if (!SCREEN_BLUR && r_magicdetail->integer)
		{
			RB_MagicDetail(currentFbo, srcBox, currentOutFbo, dstBox);
			RB_SwapFBOs( &currentFbo, &currentOutFbo);
		}

//#define CRAZY_SLOW_GAUSSIAN_BLUR //  UQ1: Let's do scope background blur with a fast blur instead...
#ifdef CRAZY_SLOW_GAUSSIAN_BLUR
		if (SCREEN_BLUR)
		{
			// Blur some times
			float	spread = 1.0f;
			int		numPasses = 8;

			for ( int i = 0; i < numPasses; i++ )
			{
				RB_GaussianBlur(currentFbo, tr.genericFbo2, currentOutFbo, spread);
				RB_SwapFBOs( &currentFbo, &currentOutFbo);
				spread += 0.6f * 0.25f;
			}
		}
#else //!CRAZY_SLOW_GAUSSIAN_BLUR
		if (SCREEN_BLUR)
		{
			RB_FastBlur(currentFbo, srcBox, currentOutFbo, dstBox);
			RB_SwapFBOs( &currentFbo, &currentOutFbo);
		}
#endif //CRAZY_SLOW_GAUSSIAN_BLUR

		if (!SCREEN_BLUR && r_rbm->integer)
		{
			RB_RBM(currentFbo, srcBox, currentOutFbo, dstBox);
			RB_SwapFBOs( &currentFbo, &currentOutFbo);
		}

		if (!SCREEN_BLUR && r_ssao->integer)
		{
			RB_SSAO(currentFbo, srcBox, currentOutFbo, dstBox);
			RB_SwapFBOs( &currentFbo, &currentOutFbo);
		}

		if (!SCREEN_BLUR && r_hbao->integer)
		{
			RB_HBAO(currentFbo, srcBox, currentOutFbo, dstBox);
			RB_SwapFBOs( &currentFbo, &currentOutFbo);
		}

		if (!SCREEN_BLUR && r_glslWater->integer)
		{
			RB_WaterPost(currentFbo, srcBox, currentOutFbo, dstBox);
			RB_SwapFBOs( &currentFbo, &currentOutFbo);
		}

		if (!SCREEN_BLUR && r_distanceBlur->integer)
		{
			if (r_distanceBlur->integer >= 2)
			{// New HQ matso blur versions...
				RB_DistanceBlur(currentFbo, srcBox, currentOutFbo, dstBox, 2);
				RB_SwapFBOs( &currentFbo, &currentOutFbo);
				RB_DistanceBlur(currentFbo, srcBox, currentOutFbo, dstBox, 3);
				RB_SwapFBOs( &currentFbo, &currentOutFbo);
				RB_DistanceBlur(currentFbo, srcBox, currentOutFbo, dstBox, 0);
				RB_SwapFBOs( &currentFbo, &currentOutFbo);
				RB_DistanceBlur(currentFbo, srcBox, currentOutFbo, dstBox, 1);
				RB_SwapFBOs( &currentFbo, &currentOutFbo);
			}
			else
			{
				RB_DistanceBlur(currentFbo, srcBox, currentOutFbo, dstBox, -1);
				RB_SwapFBOs( &currentFbo, &currentOutFbo);
			}
		}

		if (!SCREEN_BLUR && r_fxaa->integer)
		{
			RB_FXAA(currentFbo, srcBox, currentOutFbo, dstBox);
			RB_SwapFBOs( &currentFbo, &currentOutFbo);
		}

		if (!SCREEN_BLUR && r_esharpening->integer)
		{
			RB_ESharpening(currentFbo, srcBox, currentOutFbo, dstBox);
			RB_SwapFBOs( &currentFbo, &currentOutFbo);
		}

		if (!SCREEN_BLUR && r_esharpening2->integer)
		{
			RB_ESharpening2(currentFbo, srcBox, currentOutFbo, dstBox);
			RB_SwapFBOs( &currentFbo, &currentOutFbo);
		}

		if (!SCREEN_BLUR && r_darkexpand->integer)
		{
			for (int pass = 0; pass < 2; pass++)
			{
				RB_DarkExpand(currentFbo, srcBox, currentOutFbo, dstBox);
				RB_SwapFBOs( &currentFbo, &currentOutFbo);
			}
		}

		if (!SCREEN_BLUR && r_multipost->integer)
		{
			RB_MultiPost(currentFbo, srcBox, currentOutFbo, dstBox);
			RB_SwapFBOs( &currentFbo, &currentOutFbo);
		}

		if (!SCREEN_BLUR && r_depth->integer)
		{
			// First run, before adding parallax and blooms, etc...
			for (int i = 0; i < r_depthPasses->integer / 2; i++)
			{
				RB_FakeDepth(currentFbo, srcBox, currentOutFbo, dstBox);
				RB_SwapFBOs( &currentFbo, &currentOutFbo);
			}
		}

		if (!SCREEN_BLUR && r_depthParallax->integer)
		{
			for (int i = 0; i < 4; i++)
			{
				RB_FakeDepthParallax(currentFbo, srcBox, currentOutFbo, dstBox);
				RB_SwapFBOs( &currentFbo, &currentOutFbo);
			}
		}

		if (r_truehdr->integer)
		{
			RB_HDR(currentFbo, srcBox, currentOutFbo, dstBox);
			RB_SwapFBOs( &currentFbo, &currentOutFbo);
		}

		if (!SCREEN_BLUR && r_bloom->integer)
		{
			RB_Bloom(currentFbo, srcBox, currentOutFbo, dstBox);
			RB_SwapFBOs( &currentFbo, &currentOutFbo);
		}

		if (!SCREEN_BLUR && r_dof->integer)
		{
			RB_DOF(currentFbo, srcBox, currentOutFbo, dstBox, 2);
			RB_SwapFBOs( &currentFbo, &currentOutFbo);
			RB_DOF(currentFbo, srcBox, currentOutFbo, dstBox, 3);
			RB_SwapFBOs( &currentFbo, &currentOutFbo);
			RB_DOF(currentFbo, srcBox, currentOutFbo, dstBox, 0);
			RB_SwapFBOs( &currentFbo, &currentOutFbo);
			RB_DOF(currentFbo, srcBox, currentOutFbo, dstBox, 1);
			RB_SwapFBOs( &currentFbo, &currentOutFbo);
		}

		if (!SCREEN_BLUR && r_anamorphic->integer)
		{
			RB_Anamorphic(currentFbo, srcBox, currentOutFbo, dstBox);
			RB_SwapFBOs( &currentFbo, &currentOutFbo);
		}

		if (!SCREEN_BLUR && r_ssgi->integer)
		{
			RB_SSGI(currentFbo, srcBox, currentOutFbo, dstBox);
			RB_SwapFBOs( &currentFbo, &currentOutFbo);
		}

		if (!SCREEN_BLUR && r_lensflare->integer)
		{
			RB_LensFlare(currentFbo, srcBox, currentOutFbo, dstBox);
			RB_SwapFBOs( &currentFbo, &currentOutFbo);
		}

		if (!SCREEN_BLUR && r_dynamiclight->integer)
		{
			if (RB_VolumetricLight(currentFbo, srcBox, currentOutFbo, dstBox))
				RB_SwapFBOs( &currentFbo, &currentOutFbo);
		}

		if (!SCREEN_BLUR && r_depth->integer)
		{
			// Second run, after adding parallax and blooms, etc...
			for (int i = 0; i < r_depthPasses->integer / 2; i++)
			{
				RB_FakeDepth(currentFbo, srcBox, currentOutFbo, dstBox);
				RB_SwapFBOs( &currentFbo, &currentOutFbo);
			}
		}

		if (r_vibrancy->value > 0.0)
		{
			RB_Vibrancy(currentFbo, srcBox, currentOutFbo, dstBox);
			RB_SwapFBOs( &currentFbo, &currentOutFbo);
		}

		if (r_trueAnaglyph->integer)
		{
			RB_Anaglyph(currentFbo, srcBox, currentOutFbo, dstBox);
			RB_SwapFBOs( &currentFbo, &currentOutFbo);
		}

		//FBO_Blit(currentFbo, srcBox, NULL, srcFbo, dstBox, NULL, NULL, 0);
		FBO_FastBlit(currentFbo, NULL, srcFbo, NULL, GL_COLOR_BUFFER_BIT, GL_NEAREST);

		//
		// End UQ1 Added...
		//

		if (r_hdr->integer && (r_toneMap->integer || r_forceToneMap->integer))
		{
			autoExposure = (qboolean)(r_autoExposure->integer || r_forceAutoExposure->integer);
			RB_ToneMap(srcFbo, srcBox, NULL, dstBox, autoExposure);
		}
		else if (r_cameraExposure->value == 0.0f)
		{
			FBO_FastBlit(srcFbo, srcBox, NULL, dstBox, GL_COLOR_BUFFER_BIT, GL_NEAREST);
		}
		else
		{
			vec4_t color;

			color[0] =
			color[1] =
			color[2] = pow(2, r_cameraExposure->value); //exp2(r_cameraExposure->value);
			color[3] = 1.0f;

			FBO_Blit(srcFbo, srcBox, NULL, NULL, dstBox, NULL, color, 0);
		}
	}

	if (r_drawSunRays->integer)
		RB_SunRays(NULL, srcBox, NULL, dstBox);

	//if (backEnd.refdef.blurFactor > 0.0)
	//	RB_BokehBlur(NULL, srcBox, NULL, dstBox, backEnd.refdef.blurFactor);

	/*if (srcFbo)
	{
		FBO_FastBlit(srcFbo, srcBox, tr.previousRenderFbo, dstBox, GL_COLOR_BUFFER_BIT, GL_LINEAR);
	}*/

	if (0 && r_sunlightMode->integer)
	{
		vec4i_t dstBox;
		VectorSet4(dstBox, 0, 0, 128, 128);
		FBO_BlitFromTexture(tr.sunShadowDepthImage[0], NULL, NULL, NULL, dstBox, NULL, NULL, 0);
		VectorSet4(dstBox, 128, 0, 128, 128);
		FBO_BlitFromTexture(tr.sunShadowDepthImage[1], NULL, NULL, NULL, dstBox, NULL, NULL, 0);
		VectorSet4(dstBox, 256, 0, 128, 128);
		FBO_BlitFromTexture(tr.sunShadowDepthImage[2], NULL, NULL, NULL, dstBox, NULL, NULL, 0);
	}

	if (0)
	{
		vec4i_t dstBox;
		VectorSet4(dstBox, 256, glConfig.vidHeight - 256, 256, 256);
		FBO_BlitFromTexture(tr.renderDepthImage, NULL, NULL, NULL, dstBox, NULL, NULL, 0);
		VectorSet4(dstBox, 512, glConfig.vidHeight - 256, 256, 256);
		FBO_BlitFromTexture(tr.screenShadowImage, NULL, NULL, NULL, dstBox, NULL, NULL, 0);
	}

	if (0)
	{
		vec4i_t dstBox;
		VectorSet4(dstBox, 256, glConfig.vidHeight - 256, 256, 256);
		FBO_BlitFromTexture(tr.sunRaysImage, NULL, NULL, NULL, dstBox, NULL, NULL, 0);
	}

	if (0)//tr.refdef.num_dlights && r_dlightMode->integer >= 2)
	{
		vec4i_t dstBox;
		VectorSet4(dstBox, 256, glConfig.vidHeight - 256, 256, 256);
		FBO_BlitFromTexture(tr.shadowCubemaps[0], NULL, NULL, NULL, dstBox, NULL, NULL, 0);
	}

	if (0)
	{
		vec4i_t dstBox;
		VectorSet4(dstBox, 256, glConfig.vidHeight - 256, 256, 256);
		FBO_BlitFromTexture(tr.renderFbo->colorImage[3], NULL, NULL, NULL, dstBox, NULL, NULL, 0);
	}

	/*if (1)
	{
		vec4i_t dstBox;
		VectorSet4(dstBox, 0, 0, 128, 128);
		FBO_BlitFromTexture(tr.ssgiRenderFBOImage[2], NULL, NULL, NULL, dstBox, NULL, NULL, 0);
		VectorSet4(dstBox, 128, 0, 128, 128);
		FBO_BlitFromTexture(tr.ssgiRenderFBOImage[1], NULL, NULL, NULL, dstBox, NULL, NULL, 0);
		VectorSet4(dstBox, 256, 0, 128, 128);
		FBO_BlitFromTexture(tr.bloomRenderFBOImage[2], NULL, NULL, NULL, dstBox, NULL, NULL, 0);
	}*/

#if 0
	if (r_cubeMapping->integer >= 1 && tr.numCubemaps)
	{
		vec4i_t dstBox;
		int cubemapIndex = R_CubemapForPoint( backEnd.viewParms.ori.origin );

		if (cubemapIndex)
		{
			VectorSet4(dstBox, 0, glConfig.vidHeight - 256, 256, 256);
			//FBO_BlitFromTexture(tr.renderCubeImage, NULL, NULL, NULL, dstBox, &tr.testcubeShader, NULL, 0);
			FBO_BlitFromTexture(tr.cubemaps[cubemapIndex - 1], NULL, NULL, NULL, dstBox, &tr.testcubeShader, NULL, 0);
		}
	}
#endif

#ifdef __DYNAMIC_SHADOWS__
	//if (0)
	{
		vec4i_t dstBox;
		VectorSet4(dstBox, 0, 0, 128, 128);
		FBO_BlitFromTexture(tr.dlightShadowDepthImage[0][0], NULL, NULL, NULL, dstBox, NULL, NULL, 0);
		VectorSet4(dstBox, 128, 0, 128, 128);
		FBO_BlitFromTexture(tr.dlightShadowDepthImage[0][1], NULL, NULL, NULL, dstBox, NULL, NULL, 0);
		VectorSet4(dstBox, 256, 0, 128, 128);
		FBO_BlitFromTexture(tr.dlightShadowDepthImage[0][2], NULL, NULL, NULL, dstBox, NULL, NULL, 0);

		VectorSet4(dstBox, 384, 0, 128, 128);
		FBO_BlitFromTexture(tr.dlightShadowDepthImage[1][0], NULL, NULL, NULL, dstBox, NULL, NULL, 0);
		VectorSet4(dstBox, 512, 0, 128, 128);
		FBO_BlitFromTexture(tr.dlightShadowDepthImage[1][1], NULL, NULL, NULL, dstBox, NULL, NULL, 0);
		VectorSet4(dstBox, 640, 0, 128, 128);
		FBO_BlitFromTexture(tr.dlightShadowDepthImage[1][2], NULL, NULL, NULL, dstBox, NULL, NULL, 0);
	}
#endif //__DYNAMIC_SHADOWS__

	if (r_superSampleMultiplier->value > 1.0)
	{
		VectorSet4(srcBox, 0, 0, glConfig.vidHeight * r_superSampleMultiplier->value, glConfig.vidHeight * r_superSampleMultiplier->value);
		VectorSet4(dstBox, glConfig.vidHeight * (r_superSampleMultiplier->value - 1.0), glConfig.vidHeight * (r_superSampleMultiplier->value - 1.0), glConfig.vidHeight, glConfig.vidHeight);
		FBO_FastBlit(srcFbo, srcBox, tr.genericFbo, dstBox, GL_COLOR_BUFFER_BIT, GL_LINEAR);
		FBO_FastBlit(tr.genericFbo, dstBox, srcFbo, dstBox, GL_COLOR_BUFFER_BIT, GL_LINEAR);
	}

	if (!(backEnd.refdef.rdflags & RDF_BLUR) && (r_dynamicGlow->integer != 0 || r_ssgi->integer || r_anamorphic->integer || r_bloom->integer))
	{
		// Composite the glow/bloom texture
		int blendFunc = 0;
		vec4_t color = { 1.0f, 1.0f, 1.0f, 1.0f };

		if ( r_dynamicGlow->integer == 2 )
		{
			// Debug output
			blendFunc = GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO;
		}
		else if ( r_dynamicGlowSoft->integer )
		{
			blendFunc = GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE_MINUS_SRC_COLOR;
			color[0] = color[1] = color[2] = r_dynamicGlowIntensity->value;
		}
		else
		{
			blendFunc = GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE;
			color[0] = color[1] = color[2] = r_dynamicGlowIntensity->value;
		}

		FBO_BlitFromTexture (tr.glowFboScaled[0]->colorImage[0], NULL, NULL, NULL, NULL, NULL, color, blendFunc);
	}

	backEnd.framePostProcessed = qtrue;

	return (const void *)(cmd + 1);
}

/*
====================
RB_ExecuteRenderCommands
====================
*/
void RB_ExecuteRenderCommands( const void *data ) {
	int		t1, t2;

	t1 = ri->Milliseconds ();

	while ( 1 ) {
		data = PADP(data, sizeof(void *));

		switch ( *(const int *)data ) {
		case RC_SET_COLOR:
			data = RB_SetColor( data );
			break;
		case RC_STRETCH_PIC:
			data = RB_StretchPic( data );
			break;
		case RC_ROTATE_PIC:
			data = RB_RotatePic( data );
			break;
		case RC_ROTATE_PIC2:
			data = RB_RotatePic2( data );
			break;
		case RC_DRAW_SURFS:
			data = RB_DrawSurfs( data );
			break;
		case RC_DRAW_BUFFER:
			data = RB_DrawBuffer( data );
			break;
		case RC_SWAP_BUFFERS:
			data = RB_SwapBuffers( data );
			break;
#ifdef __ORIGINAL_OCCLUSION__
		case RC_DRAW_OCCLUSION:
			data = RB_DrawOcclusion(data);
			break;
#endif //__ORIGINAL_OCCLUSION__
		case RC_SCREENSHOT:
			data = RB_TakeScreenshotCmd( data );
			break;
		case RC_VIDEOFRAME:
			data = RB_TakeVideoFrameCmd( data );
			break;
		case RC_COLORMASK:
			data = RB_ColorMask(data);
			break;
		case RC_CLEARDEPTH:
			data = RB_ClearDepth(data);
			break;
		case RC_CAPSHADOWMAP:
			data = RB_CapShadowMap(data);
			break;
#ifdef __SURFACESPRITES__
		case RC_WORLD_EFFECTS:
			data = RB_WorldEffects(data);
			break;
#endif //__SURFACESPRITES__
		case RC_POSTPROCESS:
			data = RB_PostProcess(data);
			break;
		case RC_END_OF_LIST:
		default:
			// finish any 2D drawing if needed
			if(tess.numIndexes)
				RB_EndSurface();

			// stop rendering
			t2 = ri->Milliseconds ();
			backEnd.pc.msec = t2 - t1;
			return;
		}
	}

}
