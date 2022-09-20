/*
 * 4DVis - 4D Visualization system of RIKEN -
 * Copyright (c) 2000-2006, RIKEN, Japan, All right reserved.
 */

#include "nvrRender.h"
#include "nvrBrickSorter.h"

#if defined WINDOWS
extern bool InitExtensions();
#endif

using namespace std;
using namespace NVR;
using namespace CES;

// global variable of NVR namespace
bool NVR::g_useTexColorTable = true;
bool NVR::g_useAlphaWeight = false;
bool NVR::g_useTexObject = true;
NVR::DrawDirectionType NVR::g_drawDirection = BTF;


//-----------------------------------------------------------
//  class nvrRender : Volume Renderer
//-----------------------------------------------------------
nvrRender::nvrRender() :
  m_N(0), m_dataType(UNSIGNED_BYTE), m_pBspTree(NULL)
{
  m_bbox[0] = Vec3<float>(0,0,0);
  m_bbox[1] = Vec3<float>(0,0,0);
}

nvrRender::~nvrRender() {
  if ( m_pBspTree ) delete m_pBspTree;
  ClearBricks();
}

void nvrRender::ClearBricks()
{
  // clear pointer-list
  m_bl.clear();
  m_blErr.clear();
  m_blDrw.clear();
}

bool nvrRender::CreateBspTree(const nvrBrickFactory* pf) {
  if ( m_dims.Size() < 8 ) return false;
  if ( ! pf ) return false;

  nvrVolArea wholeArea;
  wholeArea.SetStart(Dim3(0, 0, 0));
  wholeArea.SetDims(m_dims);
  wholeArea.SetWholeDims(m_dims);
  if ( m_pBspTree ) delete m_pBspTree;
  deque<nvrVolArea*> vaList;
  m_pBspTree = nvrBspTree::CreateTree(m_N, wholeArea, *pf, &vaList);
  if ( ! m_pBspTree ) return false;

  register size_t nBlk = vaList.size();
  ClearBricks();
  m_bl.resize(nBlk);
  m_blErr.resize(nBlk);
  m_blDrw.resize(nBlk);
  register size_t i;
  for ( i = 0; i < nBlk; i++ ) {
    m_bl[i] = dynamic_cast<nvrBrick*>(vaList[i]);
    m_blErr[i] = m_bl[i];
    m_blDrw[i] = m_bl[i];
  } // end of for(i)

  return true;
}

bool nvrRender::Initialize(const nvrBrickFactory* pBrickFactory,
			   const NVR::DataFormat& fmt,
			   const NVR::Dim3& dims, const size_t n)
{
#ifdef WINDOWS
  if ( ! InitExtensions() ) return false;
#endif

  if ( g_useTexColorTable ) {
    const char* glslExtStr = "GL_ARB_shading_language_100";
    if ( ! nvrQueryGlExt(glslExtStr) ) {
      fprintf(stderr, "NVR: OpenGL %s extension not supported.\n", glslExtStr);
      g_useTexColorTable = false;
    }
  }

  if ( g_useTexColorTable ) {
    if ( ! SetupGLSLProg() ) return false;
  }

  if ( ! pBrickFactory ) return false;
  if ( dims.Size() < 1 ) return false;

  Dim3 dtm = CalcDivTimes(dims, n);
  if ( ! IsPow2(dims[0] + (0x1<<dtm[0]) -1) ||
       ! IsPow2(dims[1] + (0x1<<dtm[1]) -1) ||
       ! IsPow2(dims[2] + (0x1<<dtm[2]) -1) ) return false;
  m_N = dtm[0] + dtm[1] + dtm[2];
  m_dims = dims;
  m_validDims = dims;
  m_dataType = fmt;

  return CreateBspTree(pBrickFactory);
}

bool nvrRender::SetValidDims(const NVR::Dim3& dims) {
  if ( m_dims[0] != m_validDims[0] ||
       m_dims[1] != m_validDims[1] || m_dims[2] != m_validDims[2] )
    return false; // already set

  m_validDims = dims;
  if ( m_validDims[0] > m_dims[0] ) m_validDims[0] = m_dims[0];
  if ( m_validDims[1] > m_dims[1] ) m_validDims[1] = m_dims[1];
  if ( m_validDims[2] > m_dims[2] ) m_validDims[2] = m_dims[2];

  Dim3 start;
  deque<nvrBrick*>::iterator it;
  for ( it = m_bl.begin(); it != m_bl.end(); it++ ) {
    if ( ! *it ) continue;
    start = (*it)->GetStart();
    if ( start[0] > m_validDims[0] &&
	 start[1] > m_validDims[1] && start[2] > m_validDims[2] )
      (*it) = NULL;
  } // end of for(it)
  m_blErr = m_bl;
  m_blDrw = m_bl;

  if ( m_pBspTree ) m_pBspTree->AdjustValidDims(m_validDims);
  return true;
}

bool nvrRender::UpdateData(void* data, const Vec3<float>* bbox)
{
  if ( ! data ) return false;
  if ( ! m_pBspTree ) return false;

  Vec3<float> xbb[] = {
    Vec3<float>(0,0,0),
    Vec3<float>(m_dims.size[0]-1, m_dims.size[1]-1, m_dims.size[2]-1)};
  const Vec3<float>* wbb = bbox ? bbox : xbb;

  if ( ! m_pBspTree->AdjustBbox(wbb) )
    return false;
  m_bbox[0] = wbb[0]; m_bbox[1] = wbb[1];

  deque<nvrBrick*>::iterator it;
  for ( it = m_bl.begin(); it != m_bl.end(); it++ ) {
    if ( ! *it ) continue;
    (*it)->SetRefData(data, m_dataType);
  } // end of for(it)

  return true;
}

bool nvrRender::UpdateLUT(const nvrLUT& lut)
{
  m_lut = lut;
  deque<nvrBrick*>::iterator it;
  for ( it = m_bl.begin(); it != m_bl.end(); it++ ) {
    if ( ! *it ) continue;
    (*it)->SetRefLUT(&m_lut);
  }
  return true;
}

bool nvrRender::Reduce(const ReducePolicy policy, const float eRatio)
{
  deque<nvrBrick*>::iterator it;

  if ( policy == MEMORY_SIZE ) {
    register size_t texMemSize = MaxTextureSize();
    if ( texMemSize < 1 ) return false;
    register size_t totalSize = GetTotalProxySize();
    while ( totalSize > texMemSize ) {
      /* sort by residual error of "current_level + 1" */
      nvrBrickSorter::SortByError(m_blErr);

      /* test reduce the block of the smallest error */
      if ( ! m_blErr[0] ) return false;
      m_blErr[0]->SetTestLevel(m_blErr[0]->GetTestLevel() + 1);

      /* calculate total size */
      totalSize = GetTotalProxySize();
    }
  }
  else if ( policy == ERROR_RATIO ) {
    if ( eRatio < 0.f || eRatio >= 1.f ) return false;
    register float err = 0.f;
    while ( err < eRatio ) {
      /* sort by residual error of "current_level + 1" */
      nvrBrickSorter::SortByError(m_blErr);

      /* test reduce the block of the smallest error */
      if ( ! m_blErr[0] ) return false;
      m_blErr[0]->SetTestLevel(m_blErr[0]->GetTestLevel() + 1);

      /* calculate total error */
      err = 0.f;
      for ( it = m_blErr.begin(); it != m_blErr.end(); it++ )
	err += (*it)->GetResidualErr((*it)->GetTestLevel());
      if ( m_blErr.size() > 0 )
	err /= (float)m_blErr.size();
    }
  }
  else { // policy == SIMPLE
    unsigned int rl = 0;
    if ( eRatio > 0.5f ) rl = (unsigned int)eRatio;
    for ( it = m_blErr.begin(); it != m_blErr.end(); it++ ) {
      if ( ! *it ) continue;
      (*it)->SetTestLevel(rl);
    }
  }

  /* now, do reduce */
  for ( it = m_blErr.begin(); it != m_blErr.end(); it++ ) {
    if ( ! *it ) continue;
    if ( ! (*it)->Reduce((*it)->GetTestLevel()) )
      return false;
  }
  return true;
}

void nvrRender::DrawVolume(const BlendingType blendingType)
{
  /* enable the depth buffer for reading only */
  glDepthMask(false);

  /* load the classification lookup tables */
  if ( g_useTexColorTable ) {
    if ( g_useAlphaWeight )
      m_lut.LoadTableAlpha(GL_TEXTURE2);
    else
      m_lut.LoadTable(GL_TEXTURE2);

    m_glslProg.Apply();
    GLERROR_CHECK;
  }

  /* setup the compositing function */
#ifndef WINDOWS
  if ( blendingType == OVER_BLENDING ) {
    // ---- over blending ----
    if ( g_drawDirection == BTF ) {
      if ( g_useAlphaWeight )
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
      else
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    } else {
      //glBlendFunc(GL_SRC_ALPHA_SATURATE, GL_ONE);
      if ( g_useAlphaWeight )
        glBlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_ONE);
      else
        glBlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_DST_ALPHA);
    }
    glBlendEquation(GL_FUNC_ADD);
    GLERROR_CHECK;
  } else if ( blendingType == MAXIMUM_INTENSITY ) {
    // ---- maximum intensity blending ----
    glBlendEquation(GL_MAX);
  }
#else
  if ( g_drawDirection == BTF ) {
    if ( g_useAlphaWeight )
      glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    else
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  } else {
    if ( g_useAlphaWeight )
      glBlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_ONE);
    else
      glBlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_DST_ALPHA);
  }
  GLERROR_CHECK;
#endif // WINDOWS

  /* view axis and matrices */
  Mat4<float> M; glGetFloatv(GL_MODELVIEW_MATRIX, M.m_v);
  float bbl = (m_bbox[1] - m_bbox[0]).Length();
  int axis = FindViewAxis(M.m_v, 10.f * bbl);

  /* calc viewing priorities */
  Vec3<float> E(0.f, 0.f, 0.f);
  m_pBspTree->CalcPriorities(0, E, &M);
  nvrBrickSorter::SortByPriority(m_blDrw);

  /* enable texturing and blending */
  glEnable(nvrBrick::s_renderMode);
  glEnable(GL_BLEND);
  GLERROR_CHECK;

  /* render the bricks from far to near */
  register size_t i, nBlk = m_blDrw.size();
  for ( i = 0; i < nBlk; i++ ) {
    if ( ! m_blDrw[i] ) continue;
    //int axis = FindViewAxis(m_blDrw[i], E, &M);
    m_blDrw[i]->Draw(axis);
  } // end of for(i)

  /* post rendering process */
  if ( g_useTexColorTable ) {
    m_glslProg.UnApply();
    m_lut.UnLoadTable(GL_TEXTURE2);
  }

  glDisable(GL_BLEND);
  glDisable(nvrBrick::s_renderMode);
}

void nvrRender::DrawBbox()
{
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

size_t nvrRender::GetTotalProxySize(const ErrLevelType ltype)
{
  register size_t total = 0;
  deque<nvrBrick*>::iterator it;
  if ( ltype == TEST_LEVEL )
    for ( it = m_bl.begin(); it != m_bl.end(); it++ ) {
      if ( ! *it ) continue;
      total += (*it)->GetProxySize((*it)->GetTestLevel());
    }
  else
    for ( it = m_bl.begin(); it != m_bl.end(); it++ ) {
      if ( ! *it ) continue;
      total += (*it)->GetProxySize((*it)->GetReduceLevel());
    }
  if ( nvrBrick::s_renderMode == T2D )
    return total * 3;
  else
    return total;
}

// STATIC
size_t nvrRender::MaxTextureSize() {
  GLint x;
  glGetIntegerv(GL_MAX_TEXTURE_SIZE, &x);
  if ( x < 1 ) return 0;
  if ( g_useTexColorTable ) {
    // Texture format may be LUMINANCE, we guess...
    return (size_t)(x*x) * sizeof(unsigned char);
  } else {
    // Texture format may be RGBA, we guess...
    return (size_t)(x*x) * sizeof(unsigned int);
  }
}

bool nvrRender::CheckReqExtensions(const nvrBrickFactory* factory) const
{
  if ( ! factory ) return false;
  nvrBrick* pBlk = dynamic_cast<nvrBrick*>(factory->Create());
  if ( ! pBlk ) return false;
  return pBlk->CheckReqOglExt();
}

//-----------------------------------------------------------
//  FindViewAxis :
//-----------------------------------------------------------
int nvrRender::FindViewAxis(const GLfloat mvm[16], const float fd)
{
  Mat4<float> M(mvm);
  Vec3<float> c = (m_bbox[0] + m_bbox[1]) * 0.5f;

  Vec3<float> e1(1.f+c[0], c[1], c[2]);
  Vec3<float> e2(c[0], 1.f+c[1], c[2]);
  Vec3<float> e3(c[0], c[1], 1.f+c[2]);
  Vec3<float> c_v = M * c;
  Vec3<float> e1_v = (M * e1) - c_v;
  Vec3<float> e2_v = (M * e2) - c_v;
  Vec3<float> e3_v = (M * e3) - c_v;
  e1_v.UnitVec(); e2_v.UnitVec(); e3_v.UnitVec();

  Vec3<float> a(0.f-c_v[0], 0.f-c_v[1], fd - c_v[2]);
  if ( a.Length() < 0.001f )
    a = Vec3<float>(0, 0, 1);
  else
    a.UnitVec();

  int ma = -1;
  double tmp_c, ma_c = M_PI/2.0;
  tmp_c = acos((double)(e1_v | a));
  if ( sin(tmp_c) < sin(ma_c) ) {ma = 1; ma_c = tmp_c;}
  tmp_c = acos((double)(e2_v | a));
  if ( sin(tmp_c) < sin(ma_c) ) {ma = 2; ma_c = tmp_c;}
  tmp_c = acos((double)(e3_v | a));
  if ( sin(tmp_c) < sin(ma_c) ) {ma = 3; ma_c = tmp_c;}

  ma *= (ma_c > M_PI/2.0 ? -1 : 1);
  return ma;
}

int nvrRender::FindViewAxis(const GLdouble mvm[16], const GLdouble fd)
{
  Mat4<double> M(mvm);
  Vec3<double> c((m_bbox[0][0] +m_bbox[1][0])*.5,
		 (m_bbox[0][1] +m_bbox[1][1])*.5,
		 (m_bbox[0][2] +m_bbox[1][2])*.5);
  Vec3<double> e1(1.f+c[0], c[1], c[2]);
  Vec3<double> e2(c[0], 1.f+c[1], c[2]);
  Vec3<double> e3(c[0], c[1], 1.f+c[2]);
  Vec3<double> c_v = M * c;
  Vec3<double> e1_v = (M * e1) - c_v;
  Vec3<double> e2_v = (M * e2) - c_v;
  Vec3<double> e3_v = (M * e3) - c_v;
  e1_v.UnitVec(); e2_v.UnitVec(); e3_v.UnitVec();

  Vec3<double> a(0.0 -c_v[0], 0.0 -c_v[1], fd - c_v[2]);
  a.UnitVec();

  int ma = -1;
  double tmp_c, ma_c = M_PI/2.0;
  tmp_c = acos(e1_v | a);
  if ( sin(tmp_c) < sin(ma_c) ) {ma = 1; ma_c = tmp_c;}
  tmp_c = acos(e2_v | a);
  if ( sin(tmp_c) < sin(ma_c) ) {ma = 2; ma_c = tmp_c;}
  tmp_c = acos(e3_v | a);
  if ( sin(tmp_c) < sin(ma_c) ) {ma = 3; ma_c = tmp_c;}

  ma *= (ma_c > M_PI/2.0 ? -1 : 1);
  return ma;
}

int nvrRender::FindViewAxis(const nvrBrick* pb,
			    const CES::Vec3<float>& eye,
			    const CES::Mat4<float>* pMVM) const
{
  if ( ! pb ) return 0;
  const CES::Vec3<float>* pbb = pb->GetBbox();
  Vec3<float> c = (pbb[0] + pbb[1]) * 0.5f;
  Mat4<float> M; if ( pMVM ) M = *pMVM;

  Vec3<float> e1(1.f+c[0], c[1], c[2]);
  Vec3<float> e2(c[0], 1.f+c[1], c[2]);
  Vec3<float> e3(c[0], c[1], 1.f+c[2]);
  Vec3<float> c_v = M * c;
  Vec3<float> e1_v = (M * e1) - c_v;
  Vec3<float> e2_v = (M * e2) - c_v;
  Vec3<float> e3_v = (M * e3) - c_v;
  e1_v.UnitVec(); e2_v.UnitVec(); e3_v.UnitVec();

  Vec3<float> a = eye - c_v;
  if ( a.Length() < 0.001f )
    a = Vec3<float>(0, 0, 1);
  else
    a.UnitVec();

  int ma = -1;
  double tmp_c, ma_c = M_PI/2.0;
  tmp_c = acos((double)(e1_v | a));
  if ( sin(tmp_c) < sin(ma_c) ) {ma = 1; ma_c = tmp_c;}
  tmp_c = acos((double)(e2_v | a));
  if ( sin(tmp_c) < sin(ma_c) ) {ma = 2; ma_c = tmp_c;}
  tmp_c = acos((double)(e3_v | a));
  if ( sin(tmp_c) < sin(ma_c) ) {ma = 3; ma_c = tmp_c;}

  ma *= (ma_c > M_PI/2.0 ? -1 : 1);
  return ma;
}

//-----------------------------------------------------------
//  SetTexObjMode :
//-----------------------------------------------------------
void nvrRender::SetTexObjMode(const bool tom)
{
  if ( tom == g_useTexObject ) return;
  g_useTexObject = tom;
  if ( ! g_useTexObject ) {
    deque<nvrBrick*>::iterator it;
    for ( it = m_bl.begin(); it != m_bl.end(); it++ ) {
      if ( ! *it ) continue;
      (*it)->InvalidateTexObjs();
    }
  }
}

//-----------------------------------------------------------
//  SetupGLSLProg
//-----------------------------------------------------------
bool nvrRender::SetupGLSLProg() {
  const GLchar* vps_buff = NULL;
  const GLchar* fps_buff = NULL;
  const GLchar* fps2d_buff =
    "uniform sampler2D texture_vol;\n"
    "uniform sampler2D texture_stp;\n"
    "void main (void) {\n"
    "  float texCrd;\n"
    "  vec4 texVal = texture2D(texture_vol, gl_TexCoord[0].xy);\n"
    "  texCrd = texVal.r;\n"
    "  vec4 texCol = texture2D(texture_stp, vec2(texCrd,0));\n"
    "  gl_FragColor = texCol;\n"
    "}\n";
  const GLchar* fps3d_buff =
    "uniform sampler3D texture_vol;\n"
    "uniform sampler2D texture_stp;\n"
    "void main (void) {\n"
    "  float texCrd;\n"
    "  vec4 texVal = texture3D(texture_vol, gl_TexCoord[0].xyz);\n"
    "  texCrd = texVal.r;\n"
    "  vec4 texCol = texture2D(texture_stp, vec2(texCrd,0));\n"
    "  gl_FragColor = texCol;\n"
    "}\n";
  if ( nvrBrick::s_renderMode == NVR::T3D )
    fps_buff = fps3d_buff;
  else
    fps_buff = fps2d_buff;

  if ( ! m_glslProg.Init() ) return false;
  if ( ! m_glslProg.Create(vps_buff, fps_buff) ) return false;

  if ( ! m_glslProg.Apply() ) return false;
  m_glslProg.SetUnifLocI1("texture_vol", 0); // use GL_TEXTURE0
  m_glslProg.SetUnifLocI1("texture_stp", 2); // use GL_TEXTURE2
  m_glslProg.UnApply();

  return true;
}
