/*
 * 4DVis - 4D Visualization system of RIKEN -
 * Copyright (c) 2000-2006, RIKEN, Japan, All right reserved.
 */

#include "nvrBrick.h"
using namespace NVR;
using namespace CES;

// STATIC members
nvrRenderMode nvrBrick::s_renderMode = T2D;


//-----------------------------------------------------------
//  SetRefData : set reference data and parameters
//-----------------------------------------------------------
bool nvrBrick::SetRefData(void* p, const DataFormat format,
			  const RefDataType rdt)
{
  m_errCache.m_level = 0;
  m_testLevel = 0;

  if ( ! p ) {
    p_refData = NULL;
    return true;
  }

  p_refData = p;
  m_dataFormat = format;
  m_refDataType = rdt;
  RefUpdated();
  return true;
}


//-----------------------------------------------------------
//  GetResidualErr : returns residual error from the original
//-----------------------------------------------------------
float nvrBrick::GetResidualErr(const unsigned int level) const
{
  if ( level == 0 ) return 0.f;
  if ( ! p_refData ) return 0.f;

  // check cache
  if ( m_errCache.m_level == level )
    return m_errCache.m_error;
  m_errCache.m_level = level;

  register float err = 0.f;
  register float base = 0.f;
  register size_t i, j, k, cnt = 0;
  register size_t lsz = 0x1 << level;
  Dim3 ndim(m_dims.size[0] >> level,
	    m_dims.size[1] >> level, m_dims.size[2] >> level);
  if ( ndim.size[0] < 4 || ndim.size[1] < 4 || ndim.size[2] < 4 ) {
    m_errCache.m_error = 1.f;
    return m_errCache.m_error;
  }

  Dim3 dimWhole, dimStart;
  if ( m_refDataType == RD_Whole ) {
    dimWhole = m_wholeDims;
    dimStart = m_start;
  } else { // m_refDataType == RD_Brick
    dimWhole = m_dims;
    //dimStart = Dim3(0,0,0); // not required
  }

  switch ( m_dataFormat ) {
  case UNSIGNED_BYTE: {
    unsigned char* p0 = (unsigned char*)p_refData;
    for ( k = 0; k < ndim.size[2]; k++ )
      for ( j = 0; j < ndim.size[1]; j++ ) 
	for ( i = 0; i < ndim.size[0]; i++ ) {
	  register size_t idx =
	    dimWhole.size[0]*dimWhole.size[1]*(dimStart.size[2]+k*lsz)
	    + dimWhole.size[0]*(dimStart.size[1]+j*lsz)
	    + (dimStart.size[0]+i*lsz);

	  for ( register size_t kk = 0; kk < lsz; kk++ )
	    for ( register size_t jj = 0; jj < lsz; jj++ ) {
	      register size_t idx0 =
		dimWhole.size[0]*dimWhole.size[1]
		*(dimStart.size[2]+k*lsz+kk)
		+dimWhole.size[0]*(dimStart.size[1]+j*lsz+jj)
		+(dimStart.size[0]+i*lsz);
	      for ( register size_t ii = 0; ii < lsz; ii++ ) {
		err += iabs((float)p0[idx0] - (float)p0[idx]);
		base += (float)p0[idx0];
		idx0 ++; cnt ++;
	      } // end of for(ii)
	    } // end of for(jj)

	} // end of for(i)
    break;
  }
  case UNSIGNED_SHORT: {
    unsigned short* p0 = (unsigned short*)p_refData;
    for ( k = 0; k < ndim.size[2]; k++ )
      for ( j = 0; j < ndim.size[1]; j++ ) 
	for ( i = 0; i < ndim.size[0]; i++ ) {
	  register size_t idx =
	    dimWhole.size[0]*dimWhole.size[1]*(dimStart.size[2]+k*lsz)
	    + dimWhole.size[0]*(dimStart.size[1]+j*lsz)
	    + (dimStart.size[0]+i*lsz);

	  for ( register size_t kk = 0; kk < lsz; kk++ )
	    for ( register size_t jj = 0; jj < lsz; jj++ ) {
	      register size_t idx0 =
		dimWhole.size[0]*dimWhole.size[1]
		*(dimStart.size[2]+k*lsz+kk)
		+dimWhole.size[0]*(dimStart.size[1]+j*lsz+jj)
		+(dimStart.size[0]+i*lsz);
	      for ( register size_t ii = 0; ii < lsz; ii++ ) {
		err += iabs((float)p0[idx0] - (float)p0[idx]);
		base += (float)p0[idx0];
		idx0 ++; cnt ++;
	      } // end of for(ii)
	    } // end of for(jj)

	} // end of for(i)
    break;
  }
  case UNSIGNED_INT: {
    unsigned int* p0 = (unsigned int*)p_refData;
    for ( k = 0; k < ndim.size[2]; k++ )
      for ( j = 0; j < ndim.size[1]; j++ ) 
	for ( i = 0; i < ndim.size[0]; i++ ) {
	  register size_t idx =
	    dimWhole.size[0]*dimWhole.size[1]*(dimStart.size[2]+k*lsz)
	    + dimWhole.size[0]*(dimStart.size[1]+j*lsz)
	    + (dimStart.size[0]+i*lsz);

	  for ( register size_t kk = 0; kk < lsz; kk++ )
	    for ( register size_t jj = 0; jj < lsz; jj++ ) {
	      register size_t idx0 =
		dimWhole.size[0]*dimWhole.size[1]
		*(dimStart.size[2]+k*lsz+kk)
		+dimWhole.size[0]*(dimStart.size[1]+j*lsz+jj)
		+(dimStart.size[0]+i*lsz);
	      for ( register size_t ii = 0; ii < lsz; ii++ ) {
		err += iabs((float)p0[idx0] - (float)p0[idx]);
		base += (float)p0[idx0];
		idx0 ++; cnt ++;
	      } // end of for(ii)
	    } // end of for(jj)

	} // end of for(i)
    break;
  }
  case NVR::FLOAT: {
    float* p0 = (float*)p_refData;
    for ( k = 0; k < ndim.size[2]; k++ )
      for ( j = 0; j < ndim.size[1]; j++ ) 
	for ( i = 0; i < ndim.size[0]; i++ ) {
	  register size_t idx =
	    dimWhole.size[0]*dimWhole.size[1]*(dimStart.size[2]+k*lsz)
	    + dimWhole.size[0]*(dimStart.size[1]+j*lsz)
	    + (dimStart.size[0]+i*lsz);

	  for ( register size_t kk = 0; kk < lsz; kk++ )
	    for ( register size_t jj = 0; jj < lsz; jj++ ) {
	      register size_t idx0 =
		dimWhole.size[0]*dimWhole.size[1]
		*(dimStart.size[2]+k*lsz+kk)
		+dimWhole.size[0]*(dimStart.size[1]+j*lsz+jj)
		+(dimStart.size[0]+i*lsz);
	      for ( register size_t ii = 0; ii < lsz; ii++ ) {
		err += iabs(p0[idx0] - p0[idx]);
		base += iabs(p0[idx0]);
		idx0 ++; cnt ++;
	      } // end of for(ii)
	    } // end of for(jj)

	} // end of for(i)
    break;
  }
  default:
    m_errCache.m_error = 1.f;
    return m_errCache.m_error;
  }

  // prepare cache
  if ( base < 1e-8 ) base = cnt * 0.01f;
  m_errCache.m_error = err / base;
  return m_errCache.m_error;
}

//-----------------------------------------------------------
//  AllocData : allocate memory (for reduced data)
//-----------------------------------------------------------
bool nvrBrick::AllocData(const Dim3& dims)
{
  if ( dims.Size() < 1 ) return false;

  size_t elmSz = 0;
  if ( m_texFormat == COLOR_INDEX )
    elmSz = sizeof(unsigned char);
  else if ( m_texFormat == RGBA )
    elmSz = sizeof(unsigned int);
  else if ( m_texFormat == LUMINANCE_ALPHA )
    elmSz = sizeof(unsigned short);
  else if ( m_texFormat == LUMINANCE )
    elmSz = sizeof(unsigned char);
  if ( s_renderMode == T2D ) elmSz *= 3;

  m_data = ReAllocate(m_data, elmSz * dims.Size());
  if ( ! m_data ) {
    Dim3 zeroD;
    m_reducedDims = zeroD;
    return false;
  }

  m_reducedDims = dims;
  return true;
}

#define VOXVAL_IDX(x) (unsigned char)(x)
#define VOXVAL_RGB(x) (g_useAlphaWeight ? \
                       p_refLUT->getValAlpha((unsigned int)(x)) : \
                       p_refLUT->getVal((unsigned int)(x)))

//-----------------------------------------------------------
//  Reduce : reduce the data according to level
//-----------------------------------------------------------
bool nvrBrick::Reduce(const unsigned int level)
{
  if ( ! p_refData ) return true;
  if ( ! p_refLUT ) return false;
  if ( m_texFormat != RGBA && m_texFormat != LUMINANCE )
    return false; // OTHER NOT IMPLEMENTED

  Dim3 ndim(m_dims);
  ndim.size[0] >>= level; ndim.size[1] >>= level; ndim.size[2] >>= level;
  if ( ndim.Size() < 1 ) return false;

  if ( ! AllocData(ndim) ) return false;

  register size_t lsz = 0x1 << level;
  register size_t i, j, k;
  register unsigned char* pIDX = (unsigned char*)m_data;
  register unsigned char* pIDX0 = pIDX;
  register unsigned int* pRGB = (unsigned int*)m_data;
  register unsigned int* pRGB0 = pRGB;
  register size_t lsz3 = lsz * lsz * lsz;

  Dim3 dimWhole, dimStart;
  if ( m_refDataType == RD_Whole ) {
    dimWhole = m_wholeDims;
    dimStart = m_start;
  } else { // m_refDataType == RD_Brick
    dimWhole = m_dims;
    //dimStart = Dim3(0,0,0); // not required
  }

  switch ( m_dataFormat ) {
  case UNSIGNED_BYTE: {
    unsigned char* p0 = (unsigned char*)p_refData;
    register unsigned char val;
    for ( k = 0; k < ndim.size[2]; k++ )
      for ( j = 0; j < ndim.size[1]; j++ ) 
	for ( i = 0; i < ndim.size[0]; i++ ) {
	  register float fval = 0;
	  for ( register size_t kk = 0; kk < lsz; kk++ )
	    for ( register size_t jj = 0; jj < lsz; jj++ ) {
	      register size_t idx0 =
		dimWhole.size[0]*dimWhole.size[1]
		*(dimStart.size[2]+k*lsz+kk)
		+dimWhole.size[0]*(dimStart.size[1]+j*lsz+jj)
		+(dimStart.size[0]+i*lsz);
	      for ( register size_t ii = 0; ii < lsz; ii++ )
		fval += p0[idx0 ++];
	    } // end of for(jj)
	  val = (unsigned char)(fval / lsz3);

          if ( g_useTexColorTable ) {
	    (*pIDX) = VOXVAL_IDX(val);
	    pIDX++;
          } else {
	    (*pRGB) = VOXVAL_RGB(val);
	    pRGB++;
          }
	} // end of (i)
    if ( s_renderMode == T2D ) {
      for ( i = 0; i < ndim.size[0]; i++ )
	for ( k = 0; k < ndim.size[2]; k++ )
	  for ( j = 0; j < ndim.size[1]; j++ ) {
	    //val = VOL_VAL(p0, i, j, k);
            if ( g_useTexColorTable ) {
	      (*pIDX) = pIDX0[ndim.size[0]*ndim.size[1]*k +ndim.size[0]*j +i];
	      pIDX++;
            } else {
	      (*pRGB) = pRGB0[ndim.size[0]*ndim.size[1]*k +ndim.size[0]*j +i];
	      pRGB++;
            }
	  } // end of (j)
      for ( j = 0; j < ndim.size[1]; j++ )
	for ( i = 0; i < ndim.size[0]; i++ )
	  for ( k = 0; k < ndim.size[2]; k++ ) {
	    //val = VOL_VAL(p0, i, j, k);
            if ( g_useTexColorTable ) {
	      (*pIDX) = pIDX0[ndim.size[0]*ndim.size[1]*k +ndim.size[0]*j +i];
	      pIDX++;
            } else {
	      (*pRGB) = pRGB0[ndim.size[0]*ndim.size[1]*k +ndim.size[0]*j +i];
	      pRGB++;
            }
	  } // end of (k)
    }
    break;
  }
  case UNSIGNED_SHORT: {
    unsigned short* p0 = (unsigned short*)p_refData;
    register unsigned short val;
    for ( k = 0; k < ndim.size[2]; k++ )
      for ( j = 0; j < ndim.size[1]; j++ ) 
	for ( i = 0; i < ndim.size[0]; i++ ) {
	  register float fval = 0;
	  for ( register size_t kk = 0; kk < lsz; kk++ )
	    for ( register size_t jj = 0; jj < lsz; jj++ ) {
	      register size_t idx0 =
		dimWhole.size[0]*dimWhole.size[1]
		*(dimStart.size[2]+k*lsz+kk)
		+dimWhole.size[0]*(dimStart.size[1]+j*lsz+jj)
		+(dimStart.size[0]+i*lsz);
	      for ( register size_t ii = 0; ii < lsz; ii++ )
		fval += p0[idx0 ++];
	    } // end of for(jj)
	  val = (unsigned short)(fval / lsz3);

          if ( g_useTexColorTable ) {
	    (*pIDX) = VOXVAL_IDX(val);
	    pIDX++;
          } else {
	    (*pRGB) = VOXVAL_RGB(val);
	    pRGB++;
          }
	} // end of (i)
    if ( s_renderMode == T2D ) {
      for ( i = 0; i < ndim.size[0]; i++ )
	for ( k = 0; k < ndim.size[2]; k++ )
	  for ( j = 0; j < ndim.size[1]; j++ ) {
	    //val = VOL_VAL(p0, i, j, k);
            if ( g_useTexColorTable ) {
	      (*pIDX) = pIDX0[ndim.size[0]*ndim.size[1]*k +ndim.size[0]*j +i];
	      pIDX++;
            } else {
	      (*pRGB) = pRGB0[ndim.size[0]*ndim.size[1]*k +ndim.size[0]*j +i];
	      pRGB++;
            }
	  } // end of (j)
      for ( j = 0; j < ndim.size[1]; j++ )
	for ( i = 0; i < ndim.size[0]; i++ )
	  for ( k = 0; k < ndim.size[2]; k++ ) {
	    //val = VOL_VAL(p0, i, j, k);
            if ( g_useTexColorTable ) {
	      (*pIDX) = pIDX0[ndim.size[0]*ndim.size[1]*k +ndim.size[0]*j +i];
	      pIDX++;
            } else {
	      (*pRGB) = pRGB0[ndim.size[0]*ndim.size[1]*k +ndim.size[0]*j +i];
	      pRGB++;
            }
	  } // end of (k)
    }
    break;
  }
  case UNSIGNED_INT: {
    unsigned int* p0 = (unsigned int*)p_refData;
    unsigned int val;
    for ( k = 0; k < ndim.size[2]; k++ )
      for ( j = 0; j < ndim.size[1]; j++ ) 
	for ( i = 0; i < ndim.size[0]; i++ ) {
	  register float fval = 0;
	  for ( register size_t kk = 0; kk < lsz; kk++ )
	    for ( register size_t jj = 0; jj < lsz; jj++ ) {
	      register size_t idx0 =
		dimWhole.size[0]*dimWhole.size[1]
		*(dimStart.size[2]+k*lsz+kk)
		+dimWhole.size[0]*(dimStart.size[1]+j*lsz+jj)
		+(dimStart.size[0]+i*lsz);
	      for ( register size_t ii = 0; ii < lsz; ii++ )
		fval += p0[idx0 ++];
	    } // end of for(jj)
	  val = (unsigned int)(fval / lsz3);

          if ( g_useTexColorTable ) {
	    (*pIDX) = VOXVAL_IDX(val);
	    pIDX++;
          } else {
	    (*pRGB) = VOXVAL_RGB(val);
	    pRGB++;
          }
	} // end of (i)
    if ( s_renderMode == T2D ) {
      for ( i = 0; i < ndim.size[0]; i++ )
	for ( k = 0; k < ndim.size[2]; k++ )
	  for ( j = 0; j < ndim.size[1]; j++ ) {
	    //val = VOL_VAL(p0, i, j, k);
            if ( g_useTexColorTable ) {
	      (*pIDX) = pIDX0[ndim.size[0]*ndim.size[1]*k +ndim.size[0]*j +i];
	      pIDX++;
            } else {
	      (*pRGB) = pRGB0[ndim.size[0]*ndim.size[1]*k +ndim.size[0]*j +i];
	      pRGB++;
            }
	  } // end of (j)
      for ( j = 0; j < ndim.size[1]; j++ )
	for ( i = 0; i < ndim.size[0]; i++ )
	  for ( k = 0; k < ndim.size[2]; k++ ) {
	    //val = VOL_VAL(p0, i, j, k);
            if ( g_useTexColorTable ) {
	      (*pIDX) = pIDX0[ndim.size[0]*ndim.size[1]*k +ndim.size[0]*j +i];
	      pIDX++;
            } else {
	      (*pRGB) = pRGB0[ndim.size[0]*ndim.size[1]*k +ndim.size[0]*j +i];
	      pRGB++;
            }
	  } // end of (k)
    }
    break;
  }
  case NVR::FLOAT: {
    float* p0 = (float*)p_refData;
    register float val;
    for ( k = 0; k < ndim.size[2]; k++ )
      for ( j = 0; j < ndim.size[1]; j++ ) 
	for ( i = 0; i < ndim.size[0]; i++ ) {
	  val = 0.f;
	  for ( register size_t kk = 0; kk < lsz; kk++ )
	    for ( register size_t jj = 0; jj < lsz; jj++ ) {
	      register size_t idx0 =
		dimWhole.size[0]*dimWhole.size[1]
		*(dimStart.size[2]+k*lsz+kk)
		+dimWhole.size[0]*(dimStart.size[1]+j*lsz+jj)
		+(dimStart.size[0]+i*lsz);
	      for ( register size_t ii = 0; ii < lsz; ii++ )
		val += p0[idx0 ++];
	    } // end of for(jj)
	  val = val / lsz3;

          if ( g_useTexColorTable ) {
	    (*pIDX) = VOXVAL_IDX(val);
	    pIDX++;
          } else {
	    (*pRGB) = VOXVAL_RGB(val);
	    pRGB++;
          }
	} // end of (i)
    if ( s_renderMode == T2D ) {
      for ( i = 0; i < ndim.size[0]; i++ )
	for ( k = 0; k < ndim.size[2]; k++ )
	  for ( j = 0; j < ndim.size[1]; j++ ) {
	    //val = VOL_VAL(p0, i, j, k);
            if ( g_useTexColorTable ) {
	      (*pIDX) = pIDX0[ndim.size[0]*ndim.size[1]*k +ndim.size[0]*j +i];
	      pIDX++;
            } else {
	      (*pRGB) = pRGB0[ndim.size[0]*ndim.size[1]*k +ndim.size[0]*j +i];
	      pRGB++;
            }
	  } // end of (j)
      for ( j = 0; j < ndim.size[1]; j++ )
	for ( i = 0; i < ndim.size[0]; i++ )
	  for ( k = 0; k < ndim.size[2]; k++ ) {
	    //val = VOL_VAL(p0, i, j, k);
            if ( g_useTexColorTable ) {
	      (*pIDX) = pIDX0[ndim.size[0]*ndim.size[1]*k +ndim.size[0]*j +i];
	      pIDX++;
            } else {
	      (*pRGB) = pRGB0[ndim.size[0]*ndim.size[1]*k +ndim.size[0]*j +i];
	      pRGB++;
            }
	  } // end of (k)
    }
    break;
  }
  default:
    return false;
  }
  m_reduceLevel = level;
  PostReduced();
  return true;
}
#undef VOXVAL_IDX
#undef VOXVAL_RGB
#undef VO_VAL

//-----------------------------------------------------------
//  GetDataPtr : returns pointer to reduced data
//-----------------------------------------------------------
const void* nvrBrick::GetDataPtr(const int axis) const
{
  if ( ! m_data ) return NULL;
  if ( s_renderMode == T3D ) return m_data;
  switch ( iabs(axis) ) {
  case 1: // axis is I ... JK plane
    return &(((unsigned char*)m_data)[GetProxySize(m_reduceLevel)]);
  case 2: // axis is J ... KI plane
    return &(((unsigned char*)m_data)[GetProxySize(m_reduceLevel)*2]);
  case 3: // axis is K ... IJ plane
  default:
    return m_data;
  }
}

//-----------------------------------------------------------
//  Draw : draw sub-volume according to axis
//-----------------------------------------------------------
void nvrBrick::Draw(const int axis) const
{
  glColor4d(1.,1.,1.,1.);

  glBegin(GL_LINE_LOOP);
  glVertex3f(m_bbox[1][0], m_bbox[0][1], m_bbox[0][2]);
  glVertex3f(m_bbox[1][0], m_bbox[1][1], m_bbox[0][2]);
  glVertex3f(m_bbox[1][0], m_bbox[1][1], m_bbox[1][2]);
  glVertex3f(m_bbox[1][0], m_bbox[0][1], m_bbox[1][2]);
  glEnd();

  glBegin(GL_LINE_LOOP);
  glVertex3f(m_bbox[0][0], m_bbox[0][1], m_bbox[0][2]);
  glVertex3f(m_bbox[0][0], m_bbox[0][1], m_bbox[1][2]);
  glVertex3f(m_bbox[0][0], m_bbox[1][1], m_bbox[1][2]);
  glVertex3f(m_bbox[0][0], m_bbox[1][1], m_bbox[0][2]);
  glEnd();

  glBegin(GL_LINES);
  glVertex3f(m_bbox[1][0], m_bbox[0][1], m_bbox[0][2]);
  glVertex3f(m_bbox[0][0], m_bbox[0][1], m_bbox[0][2]);

  glVertex3f(m_bbox[1][0], m_bbox[1][1], m_bbox[0][2]);
  glVertex3f(m_bbox[0][0], m_bbox[1][1], m_bbox[0][2]);

  glVertex3f(m_bbox[0][0], m_bbox[1][1], m_bbox[1][2]);
  glVertex3f(m_bbox[1][0], m_bbox[1][1], m_bbox[1][2]);

  glVertex3f(m_bbox[0][0], m_bbox[0][1], m_bbox[1][2]);
  glVertex3f(m_bbox[1][0], m_bbox[0][1], m_bbox[1][2]);
  glEnd();

  GLERROR_CHECK;
}

//-----------------------------------------------------------
//  CheckReqOglExt : check reuired OpenGL extensions
//-----------------------------------------------------------
bool nvrBrick::CheckReqOglExt() const
{
  if ( ! NVR::nvrQueryGlExt("GL_EXT_abgr") )
    return false;
  return true;
}
