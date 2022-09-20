/*
 * 4DVis - 4D Visualization system of RIKEN -
 * Copyright (c) 2000-2006, RIKEN, Japan,  All right reserved.
 */

#include "nvrOrthoSliceBrick.h"
using namespace NVR;
using namespace CES;

#ifdef TRACE
#define MyTRACE(x) TRACE(x)
#else
#define MyTRACE(x) fprintf(stderr, (x))
#endif

// STATIC members
NVR::Dim3 nvrOrthoSliceBrick::s_intermediate;
bool nvrOrthoSliceBrick::s_useCombine2D = true;


//-----------------------------------------------------------
//  Draw : draw with texture according to axis
//-----------------------------------------------------------
void nvrOrthoSliceBrick::Draw(const int axis) const
{
  if ( ! m_data ) return;

  if ( g_useTexObject ) {
    if ( ! GenTexIds() ) {
      fprintf(stderr, "nvrOrthoSliceBrick: can't generate texture ids\n");
      return;
    }

    if ( ! m_loaded ) {
      switch ( s_renderMode ) {
      case T2D:
	if ( ! BuildTexObj2D() ) {
	  fprintf(stderr,"nvrOrthoSliceBrick:"
		  " can't build 2D texture objects\n");
	  return;
	}
	break;
      case T3D:
	if ( ! BuildTexObj3D() ) {
	  fprintf(stderr,"nvrOrthoSliceBrick:"
		  " can't build 3D texture objects\n");
	  return;
	}
	break;
      }
    }
  } /* g_useTexObject */

  /* reset texture states */
  glDisable(GL_TEXTURE_GEN_S);
  glDisable(GL_TEXTURE_GEN_T);
  glMatrixMode(GL_TEXTURE);
  glLoadIdentity();
  glMatrixMode(GL_MODELVIEW);

  /* draw */
  switch ( s_renderMode ) {
  case T2D:
    DrawSlices2D(axis);
    break;
  case T3D:
    DrawSlices3D(axis);
    break;
  }

  return;
}


//-----------------------------------------------------------
//  DrawSlices2D : draw slices with 2D texture
//-----------------------------------------------------------
void nvrOrthoSliceBrick::DrawSlices2D(const int axis) const
{
  if ( ! m_data ) return;
  if ( s_renderMode != T2D ) return;

  // texture base color mode : Modulate|Replace
  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
  //glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
  GLERROR_CHECK;

  // disable Texture unit #1 (failsafe)
  if ( s_useCombine2D ) {
    glActiveTexture(GL_TEXTURE1);
    glDisable(T2D);
  }

  const unsigned char* slData = NULL;
  size_t slSize[2] = {0, 0};
  register size_t plSz = 0;

  if ( ! g_useTexObject ) {
    slData = (const unsigned char*)GetDataPtr(axis);
    switch ( axis ) {
    case 1: case -1:
      slSize[0] = m_reducedDims.size[1];
      slSize[1] = m_reducedDims.size[2];
      break;
    case 2: case -2:
      slSize[0] = m_reducedDims.size[2];
      slSize[1] = m_reducedDims.size[0];
      break;
    case 3: case -3:
      slSize[0] = m_reducedDims.size[0];
      slSize[1] = m_reducedDims.size[1];
      break;
    }

    //----------------- TEXTURE UNIT #0 -----------------
    glTexParameteri(s_renderMode, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(s_renderMode, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    GLERROR_CHECK;
    glTexParameteri(s_renderMode, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(s_renderMode, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    GLERROR_CHECK;
    glEnable(T2D);
    if ( s_useCombine2D ) {
      glActiveTexture(GL_TEXTURE0); GLERROR_CHECK;
#if 1
      TexEnvCombiner* pComb0 = nvrOrthoSliceBrickFactory::GetTexEnvCombiner(0);
      if ( pComb0 ) {
        pComb0->Apply(TexEnvRGB); GLERROR_CHECK;
        pComb0->Apply(TexEnvALPHA); GLERROR_CHECK;
      }
#else
      glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
#endif
    }

    //----------------- TEXTURE UNIT #1 -----------------
    if ( s_useCombine2D ) {
      glActiveTexture(GL_TEXTURE1); GLERROR_CHECK;
      glTexParameteri(s_renderMode, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(s_renderMode, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      GLERROR_CHECK;
      glTexParameteri(s_renderMode, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(s_renderMode, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      GLERROR_CHECK;
      glDisable(T2D);

      TexEnvCombiner* pComb1 = nvrOrthoSliceBrickFactory::GetTexEnvCombiner(1);
      if ( pComb1 ) {
        pComb1->Apply(TexEnvRGB);   GLERROR_CHECK;
        pComb1->Apply(TexEnvALPHA); GLERROR_CHECK;
      }
    }

    // slice plane size (only LUMINANCE and RGBA format supported)
    plSz = slSize[0] * slSize[1];
    if ( m_texFormat == NVR::LUMINANCE )
      plSz *= sizeof(unsigned char);
    else if ( m_texFormat == NVR::RGBA )
      plSz *= sizeof(unsigned int);

  } /* ! g_useTexObject */

  // for texture interplation (CONSTANT color)
  float Cc[4];
  // base intermediate slice num
  register unsigned int lsz = 0x1<<m_reduceLevel;

  /* ==================== DRAW SLICES ==================== */
  glColor4d(1., 1., 1., 1.);

  int exAxis = (g_drawDirection == BTF) ? axis : -axis;
  switch ( exAxis ) {
  case  1: // +I : draw JK plane from Imin to Imax
    {
      lsz += s_intermediate[0];
      register float x = m_bbox[0][0];
      register float dt = 1.f / (float)m_reducedDims.size[0];
      register float dx = (m_bbox[1][0] - m_bbox[0][0]) * dt;
      for ( register size_t i = 0; i < m_reducedDims.size[0]; i++ ) {
	// Base plane, just using texture-unit #0
        if ( s_useCombine2D )
	  glActiveTexture(GL_TEXTURE0);

	if ( g_useTexObject ) {
	  glBindTexture(s_renderMode, GetTexId2D(axis, i));
	  GLERROR_CHECK;
	  SetupTexUnits2D(GL_TEXTURE0);
	}
	else {
	  glTexImage2D(s_renderMode, 0,          // Level
		       g_useTexColorTable ?      // Internal
		       GL_LUMINANCE : GL_RGBA,
		       slSize[0], slSize[1],     // Size
		       0,                        // Border
		       m_texFormat,              // Format
		       GL_UNSIGNED_BYTE,         // Type
		       &slData[plSz * i]);       // Image
	  GLERROR_CHECK;
	}

	glBegin(GL_QUADS);
	glTexCoord2f(0.f, 0.f);
	glVertex3f(x, m_bbox[0][1], m_bbox[0][2]);
	glTexCoord2f(1.f, 0.f);
	glVertex3f(x, m_bbox[1][1], m_bbox[0][2]);
	glTexCoord2f(1.f, 1.f);
	glVertex3f(x, m_bbox[1][1], m_bbox[1][2]);
	glTexCoord2f(0.f, 1.f);
	glVertex3f(x, m_bbox[0][1], m_bbox[1][2]);
	glEnd();
	GLERROR_CHECK;

	// Intermediate slices, using combined texture
        if ( ! s_useCombine2D ) {x += dx; continue;}
	if ( i != m_reducedDims.size[0]-1 ) {
	  glActiveTexture(GL_TEXTURE1);

	  if ( g_useTexObject ) {
	    glBindTexture(s_renderMode, GetTexId2D(axis, i+1));
	    GLERROR_CHECK;
	    SetupTexUnits2D(GL_TEXTURE1);
	  }
	  else {
	    glTexImage2D(s_renderMode, 0,          // Level
			 g_useTexColorTable ?      // Internal
			 GL_LUMINANCE : GL_RGBA,
			 slSize[0], slSize[1],     // Size
			 0,                        // Border
			 m_texFormat,              // Format
			 GL_UNSIGNED_BYTE,         // Type
			 &slData[plSz * (i+1)]);   // Image
	    GLERROR_CHECK;
	  }
	  glEnable(T2D);

	  for ( register size_t ii = 1; ii < lsz; ii++ ) {
	    Cc[0] = Cc[1] = Cc[2] = Cc[3] = dt*ii/lsz;
	    glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, Cc);
	    GLERROR_CHECK;

	    glBegin(GL_QUADS);
	    glTexCoord2f(0.f, 0.f);
	    glVertex3f(x +dx*ii/lsz, m_bbox[0][1], m_bbox[0][2]);
	    glTexCoord2f(1.f, 0.f);
	    glVertex3f(x +dx*ii/lsz, m_bbox[1][1], m_bbox[0][2]);
	    glTexCoord2f(1.f, 1.f);
	    glVertex3f(x +dx*ii/lsz, m_bbox[1][1], m_bbox[1][2]);
	    glTexCoord2f(0.f, 1.f);
	    glVertex3f(x +dx*ii/lsz, m_bbox[0][1], m_bbox[1][2]);
	    glEnd();
	    GLERROR_CHECK;
	  } // end of (ii)
	  glDisable(T2D);
	}
	else {
	  for ( register size_t ii = 1; ii < lsz; ii++ ) {
	    glBegin(GL_QUADS);
	    glTexCoord2f(0.f, 0.f);
	    glVertex3f(x +dx*ii/lsz, m_bbox[0][1], m_bbox[0][2]);
	    glTexCoord2f(1.f, 0.f);
	    glVertex3f(x +dx*ii/lsz, m_bbox[1][1], m_bbox[0][2]);
	    glTexCoord2f(1.f, 1.f);
	    glVertex3f(x +dx*ii/lsz, m_bbox[1][1], m_bbox[1][2]);
	    glTexCoord2f(0.f, 1.f);
	    glVertex3f(x +dx*ii/lsz, m_bbox[0][1], m_bbox[1][2]);
	    glEnd();
	    GLERROR_CHECK;
	  } // end of (ii)
	}

	x += dx;
      } // end of (i)
    }
    break;
  case -1: // -I : draw JK plane from Imax to Imin
    {
      lsz += s_intermediate[0];
      register float x = m_bbox[1][0];
      register float dt = 1.f / (float)m_reducedDims.size[0];
      register float dx = (m_bbox[1][0] - m_bbox[0][0]) * dt;
      for ( register int i = m_reducedDims.size[0] -1; i >= 0; i-- ) {
	// Base plane, just using texture-unit #0
        if ( s_useCombine2D )
	  glActiveTexture(GL_TEXTURE0);

	if ( g_useTexObject ) {
	  glBindTexture(s_renderMode, GetTexId2D(axis, i));
	  GLERROR_CHECK;
	  SetupTexUnits2D(GL_TEXTURE0);
	}
	else {
	  glTexImage2D(s_renderMode, 0,          // Level
		       g_useTexColorTable ?      // Internal
		       GL_LUMINANCE : GL_RGBA,
		       slSize[0], slSize[1],     // Size
		       0,                        // Border
		       m_texFormat,              // Format
		       GL_UNSIGNED_BYTE,         // Type
		       &slData[plSz * i]);       // Image
	  GLERROR_CHECK;
	}

	glBegin(GL_QUADS);
	glTexCoord2f(0.f, 0.f);
	glVertex3f(x, m_bbox[0][1], m_bbox[0][2]);
	glTexCoord2f(0.f, 1.f);
	glVertex3f(x, m_bbox[0][1], m_bbox[1][2]);
	glTexCoord2f(1.f, 1.f);
	glVertex3f(x, m_bbox[1][1], m_bbox[1][2]);
	glTexCoord2f(1.f, 0.f);
	glVertex3f(x, m_bbox[1][1], m_bbox[0][2]);
	glEnd();
	GLERROR_CHECK;

	// Intermediate slices, using combined texture
        if ( ! s_useCombine2D ) {x -= dx; continue;}
	if ( i != 0 ) {
	  glActiveTexture(GL_TEXTURE1);

	  if ( g_useTexObject ) {
	    glBindTexture(s_renderMode, GetTexId2D(axis, i-1));
	    GLERROR_CHECK;
	    SetupTexUnits2D(GL_TEXTURE1);
	  }
	  else {
	    glTexImage2D(s_renderMode, 0,          // Level
			 g_useTexColorTable ?      // Internal
			 GL_LUMINANCE : GL_RGBA,
			 slSize[0], slSize[1],     // Size
			 0,                        // Border
			 m_texFormat,              // Format
			 GL_UNSIGNED_BYTE,         // Type
			 &slData[plSz * (i-1)]);   // Image
	    GLERROR_CHECK;
	  }
	  glEnable(T2D);

	  for ( register size_t ii = 1; ii < lsz; ii++ ) {
	    Cc[0] = Cc[1] = Cc[2] = Cc[3] = dt*ii/lsz;
	    glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, Cc);
	    GLERROR_CHECK;
	  
	    glBegin(GL_QUADS);
	    glTexCoord2f(0.f, 0.f);
	    glVertex3f(x -dx*ii/lsz, m_bbox[0][1], m_bbox[0][2]);
	    glTexCoord2f(1.f, 0.f);
	    glVertex3f(x -dx*ii/lsz, m_bbox[1][1], m_bbox[0][2]);
	    glTexCoord2f(1.f, 1.f);
	    glVertex3f(x -dx*ii/lsz, m_bbox[1][1], m_bbox[1][2]);
	    glTexCoord2f(0.f, 1.f);
	    glVertex3f(x -dx*ii/lsz, m_bbox[0][1], m_bbox[1][2]);
	    glEnd();
	    GLERROR_CHECK;
	  } // end of (ii)
	  glDisable(T2D);
	}
	else {
	  for ( register size_t ii = 1; ii < lsz; ii++ ) {
	    glBegin(GL_QUADS);
	    glTexCoord2f(0.f, 0.f);
	    glVertex3f(x -dx*ii/lsz, m_bbox[0][1], m_bbox[0][2]);
	    glTexCoord2f(1.f, 0.f);
	    glVertex3f(x -dx*ii/lsz, m_bbox[1][1], m_bbox[0][2]);
	    glTexCoord2f(1.f, 1.f);
	    glVertex3f(x -dx*ii/lsz, m_bbox[1][1], m_bbox[1][2]);
	    glTexCoord2f(0.f, 1.f);
	    glVertex3f(x -dx*ii/lsz, m_bbox[0][1], m_bbox[1][2]);
	    glEnd();
	    GLERROR_CHECK;
	  } // end of (ii)
	}

	x -= dx;
      }
    }
    break;
  case  2: // +J : draw KI plane from Jmin to Jmax
    {
      lsz += s_intermediate[1];
      register float y = m_bbox[0][1];
      register float dt = 1.f / (float)m_reducedDims.size[1];
      register float dy = (m_bbox[1][1] - m_bbox[0][1]) * dt;
      for ( register size_t j = 0; j < m_reducedDims.size[1]; j++ ) {
	// Base plane, just using texture-unit #0
        if ( s_useCombine2D )
	  glActiveTexture(GL_TEXTURE0);

	if ( g_useTexObject ) {
	  glBindTexture(s_renderMode, GetTexId2D(axis, j));
	  GLERROR_CHECK;
	  SetupTexUnits2D(GL_TEXTURE0);
	}
	else {
	  glTexImage2D(s_renderMode, 0,          // Level
		       g_useTexColorTable ?      // Internal
		       GL_LUMINANCE : GL_RGBA,
		       slSize[0], slSize[1],     // Size
		       0,                        // Border
		       m_texFormat,              // Format
		       GL_UNSIGNED_BYTE,         // Type
		       &slData[plSz * j]);       // Image
	  GLERROR_CHECK;
	}

	glBegin(GL_QUADS);
	glTexCoord2f(0.f, 0.f);
	glVertex3f(m_bbox[0][0], y, m_bbox[0][2]);
	glTexCoord2f(1.f, 0.f);
	glVertex3f(m_bbox[0][0], y, m_bbox[1][2]);
	glTexCoord2f(1.f, 1.f);
	glVertex3f(m_bbox[1][0], y, m_bbox[1][2]);
	glTexCoord2f(0.f, 1.f);
	glVertex3f(m_bbox[1][0], y, m_bbox[0][2]);
	glEnd();
	GLERROR_CHECK;

	// Intermediate slices, using combined texture
        if ( ! s_useCombine2D ) {y += dy; continue;}
	if ( j != m_reducedDims.size[1]-1 ) {
	  glActiveTexture(GL_TEXTURE1);

	  if ( g_useTexObject ) {
	    glBindTexture(s_renderMode, GetTexId2D(axis, j+1));
	    GLERROR_CHECK;
	    SetupTexUnits2D(GL_TEXTURE1);
	  }
	  else {
	    glTexImage2D(s_renderMode, 0,          // Level
			 g_useTexColorTable ?      // Internal
			 GL_LUMINANCE : GL_RGBA,
			 slSize[0], slSize[1],     // Size
			 0,                        // Border
			 m_texFormat,              // Format
			 GL_UNSIGNED_BYTE,         // Type
			 &slData[plSz * (j+1)]);   // Image
	    GLERROR_CHECK;
	  }
	  glEnable(T2D);

	  for ( register size_t jj = 1; jj < lsz; jj++ ) {
	    Cc[0] = Cc[1] = Cc[2] = Cc[3] = dt*jj/lsz;
	    glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, Cc);
	    GLERROR_CHECK;
	    
	    glBegin(GL_QUADS);
	    glTexCoord2f(0.f, 0.f);
	    glVertex3f(m_bbox[0][0], y +dy*jj/lsz, m_bbox[0][2]);
	    glTexCoord2f(1.f, 0.f);
	    glVertex3f(m_bbox[0][0], y +dy*jj/lsz, m_bbox[1][2]);
	    glTexCoord2f(1.f, 1.f);
	    glVertex3f(m_bbox[1][0], y +dy*jj/lsz, m_bbox[1][2]);
	    glTexCoord2f(0.f, 1.f);
	    glVertex3f(m_bbox[1][0], y +dy*jj/lsz, m_bbox[0][2]);
	    glEnd();
	    GLERROR_CHECK;
	  } // end of (jj)
	  glDisable(T2D);
	}
	else {
	  for ( register size_t jj = 1; jj < lsz; jj++ ) {
	    glBegin(GL_QUADS);
	    glTexCoord2f(0.f, 0.f);
	    glVertex3f(m_bbox[0][0], y +dy*jj/lsz, m_bbox[0][2]);
	    glTexCoord2f(1.f, 0.f);
	    glVertex3f(m_bbox[0][0], y +dy*jj/lsz, m_bbox[1][2]);
	    glTexCoord2f(1.f, 1.f);
	    glVertex3f(m_bbox[1][0], y +dy*jj/lsz, m_bbox[1][2]);
	    glTexCoord2f(0.f, 1.f);
	    glVertex3f(m_bbox[1][0], y +dy*jj/lsz, m_bbox[0][2]);
	    glEnd();
	    GLERROR_CHECK;
	  } // end of (jj)
	}

	y += dy;
      } // end of (j)
    }
    break;
  case -2: // -J : draw KI plane from Jmax to Jmin
    {
      lsz += s_intermediate[1];
      register float y = m_bbox[1][1];
      register float dt = 1.f / (float)m_reducedDims.size[1];
      register float dy = (m_bbox[1][1] - m_bbox[0][1]) * dt;
      for ( register int j = m_reducedDims.size[1] -1; j >= 0; j-- ) {
	// Base plane, just using texture-unit #0
        if ( s_useCombine2D )
	  glActiveTexture(GL_TEXTURE0);

	if ( g_useTexObject ) {
	  glBindTexture(s_renderMode, GetTexId2D(axis, j));
	  GLERROR_CHECK;
	  SetupTexUnits2D(GL_TEXTURE0);
	}
	else {
	  glTexImage2D(s_renderMode, 0,          // Level
		       g_useTexColorTable ?      // Internal
		       GL_LUMINANCE : GL_RGBA,
		       slSize[0], slSize[1],     // Size
		       0,                        // Border
		       m_texFormat,              // Format
		       GL_UNSIGNED_BYTE,         // Type
		       &slData[plSz * j]);       // Image
	  GLERROR_CHECK;
	}

	glBegin(GL_QUADS);
	glTexCoord2f(0.f, 0.f);
	glVertex3f(m_bbox[0][0], y, m_bbox[0][2]);
	glTexCoord2f(0.f, 1.f);
	glVertex3f(m_bbox[1][0], y, m_bbox[0][2]);
	glTexCoord2f(1.f, 1.f);
	glVertex3f(m_bbox[1][0], y, m_bbox[1][2]);
	glTexCoord2f(1.f, 0.f);
	glVertex3f(m_bbox[0][0], y, m_bbox[1][2]);
	glEnd();
	GLERROR_CHECK;

	// Intermediate slices, using combined texture
        if ( ! s_useCombine2D ) {y -= dy; continue;}
	if ( j != 0 ) {
	  glActiveTexture(GL_TEXTURE1);

	  if ( g_useTexObject ) {
	    glBindTexture(s_renderMode, GetTexId2D(axis, j-1));
	    GLERROR_CHECK;
	    SetupTexUnits2D(GL_TEXTURE1);
	  }
	  else {
	    glTexImage2D(s_renderMode, 0,          // Level
			 g_useTexColorTable ?      // Internal
			 GL_LUMINANCE : GL_RGBA,
			 slSize[0], slSize[1],     // Size
			 0,                        // Border
			 m_texFormat,              // Format
			 GL_UNSIGNED_BYTE,         // Type
			 &slData[plSz * (j-1)]);   // Image
	    GLERROR_CHECK;
	  }
	  glEnable(T2D);

	  for ( register size_t jj = 1; jj < lsz; jj++ ) {
	    Cc[0] = Cc[1] = Cc[2] = Cc[3] = dt*jj/lsz;
	    glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, Cc);
	    GLERROR_CHECK;

	    glBegin(GL_QUADS);
	    glTexCoord2f(0.f, 0.f);
	    glVertex3f(m_bbox[0][0], y -dy*jj/lsz, m_bbox[0][2]);
	    glTexCoord2f(0.f, 1.f);
	    glVertex3f(m_bbox[1][0], y -dy*jj/lsz, m_bbox[0][2]);
	    glTexCoord2f(1.f, 1.f);
	    glVertex3f(m_bbox[1][0], y -dy*jj/lsz, m_bbox[1][2]);
	    glTexCoord2f(1.f, 0.f);
	    glVertex3f(m_bbox[0][0], y -dy*jj/lsz, m_bbox[1][2]);
	    glEnd();
	    GLERROR_CHECK;
	  } // end of (jj)
	  glDisable(T2D);
	}
	else {
	  for ( register size_t jj = 1; jj < lsz; jj++ ) {
	    glBegin(GL_QUADS);
	    glTexCoord2f(0.f, 0.f);
	    glVertex3f(m_bbox[0][0], y -dy*jj/lsz, m_bbox[0][2]);
	    glTexCoord2f(0.f, 1.f);
	    glVertex3f(m_bbox[1][0], y -dy*jj/lsz, m_bbox[0][2]);
	    glTexCoord2f(1.f, 1.f);
	    glVertex3f(m_bbox[1][0], y -dy*jj/lsz, m_bbox[1][2]);
	    glTexCoord2f(1.f, 0.f);
	    glVertex3f(m_bbox[0][0], y -dy*jj/lsz, m_bbox[1][2]);
	    glEnd();
	    GLERROR_CHECK;
	  } // end of (jj)
	}

	y -= dy;
      } // end of (j)
    }
    break;
  case  3: // +K : draw IJ plane from Kmin to Kmax
    {
      lsz += s_intermediate[2];
      register float z = m_bbox[0][2];
      register float dt = 1.f / (float)m_reducedDims.size[2];
      register float dz = (m_bbox[1][2] - m_bbox[0][2]) * dt;
      for ( register size_t k = 0; k < m_reducedDims.size[2]; k++ ) {
	// Base plane, just using texture-unit #0
        if ( s_useCombine2D )
	  glActiveTexture(GL_TEXTURE0);

	if ( g_useTexObject ) {
	  glBindTexture(s_renderMode, GetTexId2D(axis, k));
	  GLERROR_CHECK;
	  SetupTexUnits2D(GL_TEXTURE0);
	}
	else {
	  glTexImage2D(s_renderMode, 0,          // Level
		       g_useTexColorTable ?      // Internal
		       GL_LUMINANCE : GL_RGBA,
		       slSize[0], slSize[1],     // Size
		       0,                        // Border
		       m_texFormat,              // Format
		       GL_UNSIGNED_BYTE,         // Type
		       &slData[plSz * k]);       // Image
	  GLERROR_CHECK;
	}

	glBegin(GL_QUADS);
	glTexCoord2f(0.f, 0.f);
	glVertex3f(m_bbox[0][0], m_bbox[0][1], z);
	glTexCoord2f(1.f, 0.f);
	glVertex3f(m_bbox[1][0], m_bbox[0][1], z);
	glTexCoord2f(1.f, 1.f);
	glVertex3f(m_bbox[1][0], m_bbox[1][1], z);
	glTexCoord2f(0.f, 1.f);
	glVertex3f(m_bbox[0][0], m_bbox[1][1], z);
	glEnd();
	GLERROR_CHECK;

	// Intermediate slices, using combined texture
        if ( ! s_useCombine2D ) {z += dz; continue;}
	if ( k != m_reducedDims.size[2]-1 ) {
	  glActiveTexture(GL_TEXTURE1);

	  if ( g_useTexObject ) {
	    glBindTexture(s_renderMode, GetTexId2D(axis, k+1));
	    GLERROR_CHECK;
	    SetupTexUnits2D(GL_TEXTURE1);
	  }
	  else {
	    glTexImage2D(s_renderMode, 0,          // Level
			 g_useTexColorTable ?      // Internal
			 GL_LUMINANCE : GL_RGBA,
			 slSize[0], slSize[1],     // Size
			 0,                        // Border
			 m_texFormat,              // Format
			 GL_UNSIGNED_BYTE,         // Type
			 &slData[plSz * (k+1)]);   // Image
	    GLERROR_CHECK;
	  }
	  glEnable(T2D);

	  for ( register size_t kk = 1; kk < lsz; kk++ ) {
	    Cc[0] = Cc[1] = Cc[2] = Cc[3] = dt*kk/lsz;
	    glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, Cc);
	    GLERROR_CHECK;

	    glBegin(GL_QUADS);
	    glTexCoord2f(0.f, 0.f);
	    glVertex3f(m_bbox[0][0], m_bbox[0][1], z +dz*kk/lsz);
	    glTexCoord2f(1.f, 0.f);
	    glVertex3f(m_bbox[1][0], m_bbox[0][1], z +dz*kk/lsz);
	    glTexCoord2f(1.f, 1.f);
	    glVertex3f(m_bbox[1][0], m_bbox[1][1], z +dz*kk/lsz);
	    glTexCoord2f(0.f, 1.f);
	    glVertex3f(m_bbox[0][0], m_bbox[1][1], z +dz*kk/lsz);
	    glEnd();
	    GLERROR_CHECK;
	  } // end of (kk)
	  glDisable(T2D);
	}
	else {
	  for ( register size_t kk = 1; kk < lsz; kk++ ) {
	    glBegin(GL_QUADS);
	    glTexCoord2f(0.f, 0.f);
	    glVertex3f(m_bbox[0][0], m_bbox[0][1], z +dz*kk/lsz);
	    glTexCoord2f(1.f, 0.f);
	    glVertex3f(m_bbox[1][0], m_bbox[0][1], z +dz*kk/lsz);
	    glTexCoord2f(1.f, 1.f);
	    glVertex3f(m_bbox[1][0], m_bbox[1][1], z +dz*kk/lsz);
	    glTexCoord2f(0.f, 1.f);
	    glVertex3f(m_bbox[0][0], m_bbox[1][1], z +dz*kk/lsz);
	    glEnd();
	    GLERROR_CHECK;
	  } // end of (kk)
	}

	z += dz;
      } // end of (k)
    }
    break;
  case -3: // -K : draw IJ plane from Kmax to Kmin
    {
      lsz += s_intermediate[2];
      register float z = m_bbox[1][2];
      register float dt = 1.f / (float)m_reducedDims.size[2];
      register float dz = (m_bbox[1][2] - m_bbox[0][2]) * dt;
      for ( register int k = m_reducedDims.size[2] -1; k >= 0; k-- ) {
	// Base plane, just using texture-unit #0
        if ( s_useCombine2D )
	  glActiveTexture(GL_TEXTURE0);

	if ( g_useTexObject ) {
	  glBindTexture(s_renderMode, GetTexId2D(axis, k));
	  GLERROR_CHECK;
	  SetupTexUnits2D(GL_TEXTURE0);
	}
	else {
	  glTexImage2D(s_renderMode, 0,          // Level
		       g_useTexColorTable ?      // Internal
		       GL_LUMINANCE : GL_RGBA,
		       slSize[0], slSize[1],     // Size
		       0,                        // Border
		       m_texFormat,              // Format
		       GL_UNSIGNED_BYTE,         // Type
		       &slData[plSz * k]);       // Image
	  GLERROR_CHECK;
	}

	glBegin(GL_QUADS);
	glTexCoord2f(0.f, 0.f);
	glVertex3f(m_bbox[0][0], m_bbox[0][1], z);
	glTexCoord2f(0.f, 1.f);
	glVertex3f(m_bbox[0][0], m_bbox[1][1], z);
	glTexCoord2f(1.f, 1.f);
	glVertex3f(m_bbox[1][0], m_bbox[1][1], z);
	glTexCoord2f(1.f, 0.f);
	glVertex3f(m_bbox[1][0], m_bbox[0][1], z);
	glEnd();
	GLERROR_CHECK;

	// Intermediate slices, using combined texture
        if ( ! s_useCombine2D ) {z -= dz; continue;}
	if ( k != 0 ) {
	  glActiveTexture(GL_TEXTURE1);

	  if ( g_useTexObject ) {
	    glBindTexture(s_renderMode, GetTexId2D(axis, k-1));
	    GLERROR_CHECK;
	    SetupTexUnits2D(GL_TEXTURE1);
	  }
	  else {
	    glTexImage2D(s_renderMode, 0,          // Level
			 g_useTexColorTable ?      // Internal
			 GL_LUMINANCE : GL_RGBA,
			 slSize[0], slSize[1],     // Size
			 0,                        // Border
			 m_texFormat,              // Format
			 GL_UNSIGNED_BYTE,         // Type
			 &slData[plSz * (k-1)]);   // Image
	    GLERROR_CHECK;
	  }
	  glEnable(T2D);

	  for ( register size_t kk = 1; kk < lsz; kk++ ) {
	    Cc[0] = Cc[1] = Cc[2] = Cc[3] = dt*kk/lsz;
	    glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, Cc);
	    GLERROR_CHECK;

	    glBegin(GL_QUADS);
	    glTexCoord2f(0.f, 0.f);
	    glVertex3f(m_bbox[0][0], m_bbox[0][1], z -dz*kk/lsz);
	    glTexCoord2f(0.f, 1.f);
	    glVertex3f(m_bbox[0][0], m_bbox[1][1], z -dz*kk/lsz);
	    glTexCoord2f(1.f, 1.f);
	    glVertex3f(m_bbox[1][0], m_bbox[1][1], z -dz*kk/lsz);
	    glTexCoord2f(1.f, 0.f);
	    glVertex3f(m_bbox[1][0], m_bbox[0][1], z -dz*kk/lsz);
	    glEnd();
	    GLERROR_CHECK;
	  } // end of (kk)
	  glDisable(T2D);
	}
	else {
	  for ( register size_t kk = 1; kk < lsz; kk++ ) {
	    glBegin(GL_QUADS);
	    glTexCoord2f(0.f, 0.f);
	    glVertex3f(m_bbox[0][0], m_bbox[0][1], z -dz*kk/lsz);
	    glTexCoord2f(0.f, 1.f);
	    glVertex3f(m_bbox[0][0], m_bbox[1][1], z -dz*kk/lsz);
	    glTexCoord2f(1.f, 1.f);
	    glVertex3f(m_bbox[1][0], m_bbox[1][1], z -dz*kk/lsz);
	    glTexCoord2f(1.f, 0.f);
	    glVertex3f(m_bbox[1][0], m_bbox[0][1], z -dz*kk/lsz);
	    glEnd();
	    GLERROR_CHECK;
	  } // end of (kk)
	}

	z -= dz;
      } // end of (k)
    }
    break;
  default:
    break;
  }

  if ( s_useCombine2D ) {
    glActiveTexture(GL_TEXTURE0); GLERROR_CHECK;
  }
  return;
}


//-----------------------------------------------------------
//  DrawSlices3D : draw slices with 3D texture
//-----------------------------------------------------------
void nvrOrthoSliceBrick::DrawSlices3D(const int axis) const
{
  if ( ! m_data ) return;
  if ( s_renderMode != T3D ) return;

  // base intermediate slice num
  register unsigned int lsz = 0x1<<m_reduceLevel;

  // load texture
  if ( g_useTexObject ) {
    glBindTexture(s_renderMode, GetTexId3D());
    GLERROR_CHECK;
  }
  else {
    glTexImage3D(s_renderMode, 0,        // Level
		 g_useTexColorTable ?    // Internal
		 GL_LUMINANCE : GL_RGBA,
		 m_reducedDims.size[0],  // Size(width)
		 m_reducedDims.size[1],  // Size(height)
		 m_reducedDims.size[2],  // Size(depth)
		 0,                      // Border
		 m_texFormat,            // Format
		 GL_UNSIGNED_BYTE,       // Type
		 m_data);                // Data
    GLERROR_CHECK;

    // set texture parameters
    glTexParameteri(s_renderMode, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(s_renderMode, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    GLERROR_CHECK;
    glTexParameteri(s_renderMode, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(s_renderMode, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(s_renderMode, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    GLERROR_CHECK;
  }

  // texture base color mode : Modulate
  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
  //glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
  GLERROR_CHECK;


  // DRAW SLICES
  glColor4d(1., 1., 1., 1.);

  int exAxis = (g_drawDirection == BTF) ? axis : -axis;
  switch ( exAxis ) {
  case  1: // +I : draw JK plane from Imin to Imax
    {
      lsz += s_intermediate[0];
      register float t = 0.f;
      register float x = m_bbox[0][0];
      register float dt = 1.f / (float)m_reducedDims.size[0];
      register float dx = (m_bbox[1][0] - m_bbox[0][0]) * dt;
      for ( register size_t i = 0; i < m_reducedDims.size[0]; i++ ) {
	glBegin(GL_QUADS);
	glTexCoord3f(t, 0.f, 0.f);
	glVertex3f(x, m_bbox[0][1], m_bbox[0][2]);
	glTexCoord3f(t, 1.f, 0.f);
	glVertex3f(x, m_bbox[1][1], m_bbox[0][2]);
	glTexCoord3f(t, 1.f, 1.f);
	glVertex3f(x, m_bbox[1][1], m_bbox[1][2]);
	glTexCoord3f(t, 0.f, 1.f);
	glVertex3f(x, m_bbox[0][1], m_bbox[1][2]);

	for ( register size_t ii = 1; ii < lsz; ii++ ) {
	  register float tic = t +dt*ii/lsz;
	  register float xic = x +dx*ii/lsz;
	  glTexCoord3f(tic, 0.f, 0.f);
	  glVertex3f(xic, m_bbox[0][1], m_bbox[0][2]);
	  glTexCoord3f(tic, 1.f, 0.f);
	  glVertex3f(xic, m_bbox[1][1], m_bbox[0][2]);
	  glTexCoord3f(tic, 1.f, 1.f);
	  glVertex3f(xic, m_bbox[1][1], m_bbox[1][2]);
	  glTexCoord3f(tic, 0.f, 1.f);
	  glVertex3f(xic, m_bbox[0][1], m_bbox[1][2]);
	} // end of (ii)

	glEnd();
	GLERROR_CHECK;
	t += dt;
	x += dx;
      } // end of (i)
    }
    break;
  case -1: // -I : draw JK plane from Imax to Imin
    {
      lsz += s_intermediate[0];
      register float t = 1.f;
      register float x = m_bbox[1][0];
      register float dt = 1.f / (float)m_reducedDims.size[0];
      register float dx = (m_bbox[1][0] - m_bbox[0][0]) * dt;
      for ( register int i = m_reducedDims.size[0] -1; i >= 0; i-- ) {
	glBegin(GL_QUADS);
	glTexCoord3f(t, 0.f, 0.f);
	glVertex3f(x, m_bbox[0][1], m_bbox[0][2]);
	glTexCoord3f(t, 0.f, 1.f);
	glVertex3f(x, m_bbox[0][1], m_bbox[1][2]);
	glTexCoord3f(t, 1.f, 1.f);
	glVertex3f(x, m_bbox[1][1], m_bbox[1][2]);
	glTexCoord3f(t, 1.f, 0.f);
	glVertex3f(x, m_bbox[1][1], m_bbox[0][2]);

	for ( register size_t ii = 1; ii < lsz; ii++ ) {
	  register float tic = t -dt*ii/lsz;
	  register float xic = x -dx*ii/lsz;
	  glTexCoord3f(tic, 0.f, 0.f);
	  glVertex3f(xic, m_bbox[0][1], m_bbox[0][2]);
	  glTexCoord3f(tic, 1.f, 0.f);
	  glVertex3f(xic, m_bbox[1][1], m_bbox[0][2]);
	  glTexCoord3f(tic, 1.f, 1.f);
	  glVertex3f(xic, m_bbox[1][1], m_bbox[1][2]);
	  glTexCoord3f(tic, 0.f, 1.f);
	  glVertex3f(xic, m_bbox[0][1], m_bbox[1][2]);
	} // end of (ii)

	glEnd();
	GLERROR_CHECK;
	t -= dt;
	x -= dx;
      } // end of (i)
    }
    break;
  case  2: // +J : draw KI plane from Jmin to Jmax
    {
      lsz += s_intermediate[1];
      register float t = 0.f;
      register float y = m_bbox[0][1];
      register float dt = 1.f / (float)m_reducedDims.size[1];
      register float dy = (m_bbox[1][1] - m_bbox[0][1]) * dt;
      for ( register size_t j = 0; j < m_reducedDims.size[1]; j++ ) {
	glBegin(GL_QUADS);
	glTexCoord3f(0.f, t, 0.f);
	glVertex3f(m_bbox[0][0], y, m_bbox[0][2]);
	glTexCoord3f(0.f, t, 1.f);
	glVertex3f(m_bbox[0][0], y, m_bbox[1][2]);
	glTexCoord3f(1.f, t, 1.f);
	glVertex3f(m_bbox[1][0], y, m_bbox[1][2]);
	glTexCoord3f(1.f, t, 0.f);
	glVertex3f(m_bbox[1][0], y, m_bbox[0][2]);

	for ( register size_t jj = 1; jj < lsz; jj++ ) {
	  register float tic = t +dt*jj/lsz;
	  register float yic = y +dy*jj/lsz;
	  glTexCoord3f(0.f, tic, 0.f);
	  glVertex3f(m_bbox[0][0], yic, m_bbox[0][2]);
	  glTexCoord3f(0.f, tic, 1.f);
	  glVertex3f(m_bbox[0][0], yic, m_bbox[1][2]);
	  glTexCoord3f(1.f, tic, 1.f);
	  glVertex3f(m_bbox[1][0], yic, m_bbox[1][2]);
	  glTexCoord3f(1.f, tic, 0.f);
	  glVertex3f(m_bbox[1][0], yic, m_bbox[0][2]);
	} // end of (jj)

	glEnd();
	GLERROR_CHECK;
	t += dt;
	y += dy;
      } // end of (j)
    }
    break;
  case -2: // -J : draw KI plane from Jmax to Jmin
    {
      lsz += s_intermediate[1];
      register float t = 1.f;
      register float y = m_bbox[1][1];
      register float dt = 1.f / (float)m_reducedDims.size[1];
      register float dy = (m_bbox[1][1] - m_bbox[0][1]) * dt;
      for ( register int j = m_reducedDims.size[1] -1; j >= 0; j-- ) {
	glBegin(GL_QUADS);
	glTexCoord3f(0.f, t, 0.f);
	glVertex3f(m_bbox[0][0], y, m_bbox[0][2]);
	glTexCoord3f(1.f, t, 0.f);
	glVertex3f(m_bbox[1][0], y, m_bbox[0][2]);
	glTexCoord3f(1.f, t, 1.f);
	glVertex3f(m_bbox[1][0], y, m_bbox[1][2]);
	glTexCoord3f(0.f, t, 1.f);
	glVertex3f(m_bbox[0][0], y, m_bbox[1][2]);

	for ( register size_t jj = 1; jj < lsz; jj++ ) {
	  register float tic = t -dt*jj/lsz;
	  register float yic = y -dy*jj/lsz;
	  glTexCoord3f(0.f, tic, 0.f);
	  glVertex3f(m_bbox[0][0], yic, m_bbox[0][2]);
	  glTexCoord3f(0.f, tic, 1.f);
	  glVertex3f(m_bbox[0][0], yic, m_bbox[1][2]);
	  glTexCoord3f(1.f, tic, 1.f);
	  glVertex3f(m_bbox[1][0], yic, m_bbox[1][2]);
	  glTexCoord3f(1.f, tic, 0.f);
	  glVertex3f(m_bbox[1][0], yic, m_bbox[0][2]);
	} // end of (jj)

	glEnd();
	GLERROR_CHECK;
	t -= dt;
	y -= dy;
      } // end of (j)
    }
    break;
  case  3: // +K : draw IJ plane from Kmin to Kmax
    {
      lsz += s_intermediate[2];
      register float t = 0.f;
      register float z = m_bbox[0][2];
      register float dt = 1.f / (float)m_reducedDims.size[2];
      register float dz = (m_bbox[1][2] - m_bbox[0][2]) * dt;
      for ( register size_t k = 0; k < m_reducedDims.size[2]; k++ ) {
	glBegin(GL_QUADS);
	glTexCoord3f(0.f, 0.f, t);
	glVertex3f(m_bbox[0][0], m_bbox[0][1], z);
	glTexCoord3f(1.f, 0.f, t);
	glVertex3f(m_bbox[1][0], m_bbox[0][1], z);
	glTexCoord3f(1.f, 1.f, t);
	glVertex3f(m_bbox[1][0], m_bbox[1][1], z);
	glTexCoord3f(0.f, 1.f, t);
	glVertex3f(m_bbox[0][0], m_bbox[1][1], z);

	for ( register size_t kk = 1; kk < lsz; kk++ ) {
	  register float tic = t +dt*kk/lsz;
	  register float zic = z +dz*kk/lsz;
	  glTexCoord3f(0.f, 0.f, tic);
	  glVertex3f(m_bbox[0][0], m_bbox[0][1], zic);
	  glTexCoord3f(1.f, 0.f, tic);
	  glVertex3f(m_bbox[1][0], m_bbox[0][1], zic);
	  glTexCoord3f(1.f, 1.f, tic);
	  glVertex3f(m_bbox[1][0], m_bbox[1][1], zic);
	  glTexCoord3f(0.f, 1.f, tic);
	  glVertex3f(m_bbox[0][0], m_bbox[1][1], zic);
	} // end of (kk)

	glEnd();
	GLERROR_CHECK;
	t += dt;
	z += dz;
      } // end of (k)
    }
    break;
  case -3: // -K : draw IJ plane from Kmax to Kmin
    {
      lsz += s_intermediate[2];
      register float t = 1.f;
      register float z = m_bbox[1][2];
      register float dt = 1.f / (float)m_reducedDims.size[2];
      register float dz = (m_bbox[1][2] - m_bbox[0][2]) * dt;
      for ( register int k = m_reducedDims.size[2] -1; k >= 0; k-- ) {
	glBegin(GL_QUADS);
	glTexCoord3f(0.f, 0.f, t);
	glVertex3f(m_bbox[0][0], m_bbox[0][1], z);
	glTexCoord3f(0.f, 1.f, t);
	glVertex3f(m_bbox[0][0], m_bbox[1][1], z);
	glTexCoord3f(1.f, 1.f, t);
	glVertex3f(m_bbox[1][0], m_bbox[1][1], z);
	glTexCoord3f(1.f, 0.f, t);
	glVertex3f(m_bbox[1][0], m_bbox[0][1], z);

	for ( register size_t kk = 1; kk < lsz; kk++ ) {
	  register float tic = t -dt*kk/lsz;
	  register float zic = z -dz*kk/lsz;
	  glTexCoord3f(0.f, 0.f, tic);
	  glVertex3f(m_bbox[0][0], m_bbox[0][1], zic);
	  glTexCoord3f(1.f, 0.f, tic);
	  glVertex3f(m_bbox[1][0], m_bbox[0][1], zic);
	  glTexCoord3f(1.f, 1.f, tic);
	  glVertex3f(m_bbox[1][0], m_bbox[1][1], zic);
	  glTexCoord3f(0.f, 1.f, tic);
	  glVertex3f(m_bbox[0][0], m_bbox[1][1], zic);
	} // end of (kk)

	glEnd();
	GLERROR_CHECK;
	t -= dt;
	z -= dz;
      } // end of (k)
    }
    break;
  default:
    break;
  }
  return;
}

//-----------------------------------------------------------
//  GenTexIds : Generate OpenGL texture-object ids
//-----------------------------------------------------------
bool nvrOrthoSliceBrick::GenTexIds() const
{
  if ( m_tids && m_numTids > 0 ) {
    if ( m_numTids >= GetNumNeedTexIds() )
      return true;

    glDeleteTextures(m_numTids, m_tids);
    m_numTids = 0;
  }
  m_loaded = false;
  m_numTids = GetNumNeedTexIds();

  m_tids = (GLuint*)ReAllocate(m_tids, m_numTids*sizeof(GLuint));
  if ( ! m_tids ) return false;

  glGenTextures(m_numTids, m_tids);
  GLERROR_CHECK;

  return true;
}

//-----------------------------------------------------------
//  BuildTexObj2D : Build OpenGL texture-object for T2D
//-----------------------------------------------------------
bool nvrOrthoSliceBrick::BuildTexObj2D() const
{
  if ( m_loaded ) return true;

  if ( ! m_data ) return false;
  if ( ! m_tids || m_numTids < GetNumNeedTexIds() ) return false;

  unsigned char* slData = (unsigned char*)GetDataPtr();
  size_t slSize[3][2] = {
    {m_reducedDims.size[0], m_reducedDims.size[1]},
    {m_reducedDims.size[1], m_reducedDims.size[2]},
    {m_reducedDims.size[2], m_reducedDims.size[0]}};
  register size_t k, plSz;
  
  register size_t idx = 0;
  plSz = slSize[0][0] * slSize[0][1];
  if ( m_texFormat == NVR::LUMINANCE )
    plSz *= sizeof(unsigned char);
  else if ( m_texFormat == NVR::RGBA )
    plSz *= sizeof(unsigned int);
  for ( k = 0; k < m_reducedDims.size[2]; k++ ) {
    glBindTexture(s_renderMode, m_tids[idx++]);
    glTexImage2D(s_renderMode, 0,           // Level
                 g_useTexColorTable ?       // Internal
		 GL_LUMINANCE : GL_RGBA,
		 slSize[0][0], slSize[0][1],// Size
		 0,                         // Border
		 m_texFormat,               // Format
		 GL_UNSIGNED_BYTE,          // Type
		 &slData[plSz * k]);        // Image
    glTexParameteri(s_renderMode, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(s_renderMode, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(s_renderMode, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(s_renderMode, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  }
  GLERROR_CHECK;

  slData = (unsigned char*)GetDataPtr(1);
  plSz = slSize[1][0] * slSize[1][1];
  if ( m_texFormat == NVR::LUMINANCE )
    plSz *= sizeof(unsigned char);
  else if ( m_texFormat == NVR::RGBA )
    plSz *= sizeof(unsigned int);

  for ( k = 0; k < m_reducedDims.size[0]; k++ ) {
    glBindTexture(s_renderMode, m_tids[idx++]);
    glTexImage2D(s_renderMode, 0,           // Level
                 g_useTexColorTable ?       // Internal
		 GL_LUMINANCE : GL_RGBA,
		 slSize[1][0], slSize[1][1],// Size
		 0,                         // Border
		 m_texFormat,               // Format
		 GL_UNSIGNED_BYTE,          // Type
		 &slData[plSz * k]);        // Image
    glTexParameteri(s_renderMode, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(s_renderMode, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(s_renderMode, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(s_renderMode, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  }
  GLERROR_CHECK;

  slData = (unsigned char*)GetDataPtr(2);
  plSz = slSize[2][0] * slSize[2][1];
  if ( m_texFormat == NVR::LUMINANCE )
    plSz *= sizeof(unsigned char);
  else if ( m_texFormat == NVR::RGBA )
    plSz *= sizeof(unsigned int);

  for ( k = 0; k < m_reducedDims.size[1]; k++ ) {
    glBindTexture(s_renderMode, m_tids[idx++]);
    glTexImage2D(s_renderMode, 0,           // Level
                 g_useTexColorTable ?       // Internal
		 GL_LUMINANCE : GL_RGBA,
		 slSize[2][0], slSize[2][1],// Size
		 0,                         // Border
		 m_texFormat,               // Format
		 GL_UNSIGNED_BYTE,          // Type
		 &slData[plSz * k]);        // Image
    glTexParameteri(s_renderMode, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(s_renderMode, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(s_renderMode, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(s_renderMode, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  }
  GLERROR_CHECK;

  m_loaded = true;
  return true;
}

//-----------------------------------------------------------
//  BuildTexObj3D : Build OpenGL texture-object for T3D
//-----------------------------------------------------------
bool nvrOrthoSliceBrick::BuildTexObj3D() const
{
  if ( m_loaded ) return true;

  if ( ! m_data ) return false;
  if ( ! m_tids || m_numTids < 1 ) return false;

  glBindTexture(s_renderMode, m_tids[0]);
  glTexImage3D(s_renderMode, 0,        // Level
               g_useTexColorTable ?    // Internal
	       GL_LUMINANCE : GL_RGBA,
	       m_reducedDims.size[0],  // Size(width)
	       m_reducedDims.size[1],  // Size(height)
	       m_reducedDims.size[2],  // Size(depth)
	       0,                      // Border
	       m_texFormat,            // Format
	       GL_UNSIGNED_BYTE,       // Type
	       m_data);                // Data
  GLERROR_CHECK;

  // set texture parameters
  glTexParameteri(s_renderMode, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(s_renderMode, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  GLERROR_CHECK;
  glTexParameteri(s_renderMode, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(s_renderMode, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(s_renderMode, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
  GLERROR_CHECK;

  m_loaded = true;
  return true;
}

void nvrOrthoSliceBrick::SetupTexUnits2D(const GLenum target,
					 const bool activate) const
{
  if ( ! s_useCombine2D ) {
    // REPLACE
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    GLERROR_CHECK;
    return;
  }

  TexEnvCombiner *pComb0, *pComb1;

  switch ( target ) {
  case GL_TEXTURE0:
    //----------------- TEXTURE UNIT #0 -----------------
    if ( activate ) {
      glActiveTexture(GL_TEXTURE0); GLERROR_CHECK;
    }

#if 1
    pComb0 = nvrOrthoSliceBrickFactory::GetTexEnvCombiner(0);
    if ( pComb0 ) {
      pComb0->Apply(TexEnvRGB); GLERROR_CHECK;
      pComb0->Apply(TexEnvALPHA); GLERROR_CHECK;
    }
#else
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
#endif
    break;

  case GL_TEXTURE1:
    //----------------- TEXTURE UNIT #1 -----------------
    if ( activate ) {
      glActiveTexture(GL_TEXTURE1); GLERROR_CHECK;
    }

    pComb1 = nvrOrthoSliceBrickFactory::GetTexEnvCombiner(1);
    if ( pComb1 ) {
      pComb1->Apply(TexEnvRGB); GLERROR_CHECK;
      pComb1->Apply(TexEnvALPHA); GLERROR_CHECK;
    }
    break;
  }

#if 0
  if ( s_useCombine2D ) {
    glActiveTexture(GL_TEXTURE0); GLERROR_CHECK;
  }
#endif
}

//-----------------------------------------------------------
//  InvalidateTexObjs : delete texture object id(s)
//-----------------------------------------------------------
void nvrOrthoSliceBrick::InvalidateTexObjs()
{
  if ( m_tids && m_numTids > 0 ) {
    glDeleteTextures(m_numTids, m_tids);
    m_numTids = 0;
  }
  m_loaded = false;  
}

//-----------------------------------------------------------
//  CheckReqOglExt : check reuired OpenGL extensions
//-----------------------------------------------------------
bool nvrOrthoSliceBrick::CheckReqOglExt() const
{
  // check in base class
  if ( ! nvrBrick::CheckReqOglExt() ) return false;

  // check local extensions
#ifndef __APPLE__
  if ( g_useTexObject ) {
    if ( ! NVR::nvrQueryGlExt("GL_EXT_texture_object") )
      return false;
  }
#endif // __APPLE__

  if ( g_useTexColorTable ) {
    if ( ! NVR::nvrQueryGlExt("GL_ARB_shading_language_100") )
      return false;
  }

  if ( s_renderMode == T2D && s_useCombine2D ) {
    if ( ! NVR::nvrQueryGlExt("GL_ARB_multitexture") ||
	 ! NVR::nvrQueryGlExt("GL_EXT_texture_env_combine") )
      return false;
  }

  return true;
}


//-----------------------------------------------------------
//  GetTexEnvCombiner : (STATIC)
//-----------------------------------------------------------
TexEnvCombiner* nvrOrthoSliceBrickFactory::GetTexEnvCombiner(const int unit)
{
  static TexEnvCombiner* p_comb0(NULL);
  static TexEnvCombiner* p_comb1(NULL);

  switch ( unit ) {
  case 0:
    //----------- COMBINER FOR TEXTURE UNIT #0 : REPLACE -----------
    if ( ! p_comb0 ) {
      p_comb0 = new TexEnvCombiner();
      if ( ! p_comb0 ) return NULL;
      p_comb0->rgb.combine = GL_REPLACE;
      p_comb0->alp.combine = GL_REPLACE;
    }
    return p_comb0;
  case 1:
    //----------- COMBINER FOR TEXTURE UNIT #1 : INTERPOLATE -----------
    if ( ! p_comb1 ) {
      p_comb1 = new TexEnvCombiner();
      if ( ! p_comb1 ) return NULL;
      p_comb1->rgb.combine = GL_INTERPOLATE_EXT;
      p_comb1->alp.combine = GL_INTERPOLATE_EXT;
    }
    return p_comb1;
  } // end of switch(unit)

  return NULL;
}
