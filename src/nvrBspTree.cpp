/*
 * 4DVis - 4D Visualization system of RIKEN -
 * Copyright (c) 2000-2005, RIKEN, Japan, All right reserved.
 */
#include "nvrBspTree.h"

using namespace std;
using namespace CES;
using namespace NVR;


nvrBspTree::AreaType
nvrBspTree::GetAreaOfEye(const Vec3<float>& eye,
			 const Mat4<float>* pMVM) const {
  if ( m_part == LeafBit )
    return InvalidArea;

  float dx, dy, dz;
  Vec3<float> nx(1,0,0), ny(0,1,0), nz(0,0,1), cp(m_cp);
  if ( pMVM ) {
    nx = nx * 1e3f; ny = ny * 1e3f; nz = nz * 1e3f;
    Mat4<float> mvm = *pMVM;
    nx = nx + cp; ny = ny + cp; nz = nz + cp;
    nx = mvm * nx; ny = mvm * ny; nz = mvm * nz; cp = mvm * cp;
    nx = nx - cp; ny = ny - cp; nz = nz - cp;
    nx.UnitVec(); ny.UnitVec(); nz.UnitVec();
  }
  dx = -(nx | cp); dy = -(ny | cp); dz = -(nz | cp);

  AreaType result = InvalidArea;
  switch ( m_part ) {
  case XPartBit:
    result = ( (nx | eye) + dx < 0 ) ? NegArea : PosArea;
    break;
  case YPartBit:
    result = ( (ny | eye) + dy < 0 ) ? NegArea : PosArea;
    break;
  case ZPartBit:
    result = ( (nz | eye) + dz < 0 ) ? NegArea : PosArea;
    break;
  }
  return result;
}


const nvrVolArea* const
nvrBspTree::GetPartner() const {
  if ( m_dirty ) return NULL;
  if ( ! p_parent ) {
    // this is root node
    return NULL;
  }

  nvrBspTree* node = NULL;

  // get brother
  if ( this == p_parent->m_front )
    node = p_parent->m_back;
  else if ( this == p_parent->m_back )
    node = p_parent->m_front;
  if ( ! node ) return NULL;

  // get the partner(the highest prior node)
  while ( node->m_part != LeafBit ) {
    if ( ! node->m_front || ! node->m_back )
      return NULL;
    if ( node->m_front->GetLastResult() == PosArea )
      node = node->m_front;
    else
      node = node->m_back;
  }

  return node->m_area;
}


size_t
nvrBspTree::CalcPriorities(const size_t curPri,
			   const CES::Vec3<float>& eye,
			   const CES::Mat4<float>* pMVM,
			   const AreaType res) {
  m_lastResult = InvalidArea;

  if ( m_dirty ) return curPri;
  m_dirty = true;
  volatile size_t newPri = curPri;

  if ( m_part == LeafBit ) {
    if ( m_area ) {
      m_area->SetPrior(curPri);
      newPri ++;
    }
  }
  else {
    AreaType eyeArea = GetAreaOfEye(eye, pMVM);
    if ( eyeArea == PosArea ) {
      if ( m_front )
        newPri = m_front->CalcPriorities(newPri, eye, pMVM, PosArea);
      if ( m_back )
        newPri = m_back ->CalcPriorities(newPri, eye, pMVM, NegArea);
    } else if (eyeArea == NegArea ) {
      if ( m_back )
        newPri = m_back ->CalcPriorities(newPri, eye, pMVM, PosArea);
      if ( m_front )
        newPri = m_front->CalcPriorities(newPri, eye, pMVM, NegArea);
    } else
      newPri = 0;
  }

  m_lastResult = res;
  m_dirty = false;
  return newPri;
}

bool
nvrBspTree::AdjustBbox(const Vec3<float>* bb) {
  if ( ! bb ) return false;
  if ( (bb[1] - bb[0]).Length() < 1e-8 ) return false;

  m_cp = (bb[0] + bb[1]) * 0.5f;

  if ( m_part == LeafBit ) {
    if ( m_area )
      m_area->SetBbox(bb);
    return true;
  }

  Vec3<float> nBbF[2], nBbB[2];
  nBbF[0] = bb[0]; nBbF[1] = bb[1];
  nBbB[0] = bb[0]; nBbB[1] = bb[1];
  switch ( m_part ) {
  case XPartBit:
    nBbB[1][0] = nBbF[0][0] = m_cp[0];
    break;
  case YPartBit:
    nBbB[1][1] = nBbF[0][1] = m_cp[1];
    break;
  case ZPartBit:
    nBbB[1][2] = nBbF[0][2] = m_cp[2];
    break;
  }

  if ( ! m_front || ! m_front->AdjustBbox(nBbF) )
    return false;
  if ( ! m_back || ! m_back->AdjustBbox(nBbB) )
    return false;

  return true;
}

bool
nvrBspTree::AdjustValidDims(const NVR::Dim3& dims) {
  if ( m_part == LeafBit ) {
    if ( ! m_area ) return false;
    Dim3 start = m_area->GetStart();
    if ( start[0] > dims[0] && start[1] > dims[1] && start[2] > dims[2] ) {
      delete m_area;
      m_area = NULL;
    }
    return true;
  }

  if ( ! m_front || ! m_front->AdjustValidDims(dims) )
    return false;
  if ( ! m_back || ! m_back->AdjustValidDims(dims) )
    return false;

  return true;
}

bool
nvrBspTree::CreateBranch(const size_t maxl, const nvrVolArea& va,
			 const nvrVolAreaFactory& leafFac,
			 deque<nvrVolArea*>& vaList) {
  if ( m_level > maxl ) return false;
  if ( ! va.IsValid() ) return false;
  const Vec3<float>* pbb = va.GetBbox();
  m_cp = (pbb[0] + pbb[1]) * 0.5f;

  if ( m_level == maxl ) {
    m_part = LeafBit;
    if ( m_front ) {delete m_front; m_front = NULL;}
    if ( m_back  ) {delete m_back;  m_back  = NULL;}
    if ( m_area ) delete m_area;
    m_area = leafFac.Create();
    if ( ! m_area ) return false;
    m_area->SetWholeDims(va.GetWholeDims());
    m_area->SetStart(va.GetStart());
    m_area->SetDims(va.GetDims());
    m_area->SetRefBsp((void*)this);
    vaList.push_back(m_area);
    return true;
  }

  m_front = new nvrBspTree(this);
  m_back = new nvrBspTree(this);
  if ( ! m_front || ! m_back ) return false;
  m_front->InFront(true, this); m_back->InFront(false, this);

  // level
  m_front->m_level = m_back->m_level = m_level +1;

  // volume-area
  Dim3 newStart(va.GetStart());
  Dim3 newDims(va.GetDims());
  Vec3<float> nBbF[2], nBbB[2];
  nBbF[0] = pbb[0]; nBbF[1] = pbb[1];
  nBbB[0] = pbb[0]; nBbB[1] = pbb[1];
  nvrVolArea vaF(va), vaB(va);

  // part type
  size_t maxDim = Max3(newDims[0], newDims[1], newDims[2]);
  if ( maxDim == newDims[2] ) m_part = ZPartBit;
  else if ( maxDim == newDims[1] ) m_part = YPartBit;
  else m_part = XPartBit;

  switch ( m_part ) {
  case XPartBit:
    newDims[0] = (newDims[0] - 1) / 2 + 1;
    newStart[0] += (newDims[0] - 1);
    nBbB[1][0] = nBbF[0][0] = m_cp[0];
    break;
  case YPartBit:
    newDims[1] = (newDims[1] - 1) / 2 + 1;
    newStart[1] += (newDims[1] - 1);
    nBbB[1][1] = nBbF[0][1] = m_cp[1];
    break;
  case ZPartBit:
    newDims[2] = (newDims[2] - 1) / 2 + 1;
    newStart[2] += (newDims[2] - 1);
    nBbB[1][2] = nBbF[0][2] = m_cp[2];
    break;
  }
  vaF.SetStart(newStart);
  vaF.SetDims(newDims); vaB.SetDims(newDims);
  vaF.SetBbox(nBbF);  vaB.SetBbox(nBbB);

  if ( ! m_front->CreateBranch(maxl, vaF, leafFac, vaList) ||
       ! m_back->CreateBranch(maxl, vaB, leafFac, vaList) ) return false;

  return true;
}


// STATIC
nvrBspTree*
nvrBspTree::CreateTree(const size_t level, const nvrVolArea& va,
		       const nvrVolAreaFactory& leafFac,
		       deque<nvrVolArea*>* pvaList) {
  nvrBspTree* root = new nvrBspTree();
  if ( ! root ) return NULL;

  bool ret = false;
  if ( pvaList )
    ret = root->CreateBranch(level, va, leafFac, *pvaList);
  else {
    deque<nvrVolArea*> vaList;
    ret = root->CreateBranch(level, va, leafFac, vaList);
  }

  if ( ret )
    return root;
  else {
    delete root;
    return NULL;
  }
}

