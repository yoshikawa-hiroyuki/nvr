/*
 * 4DVis - 4D Visualization system of RIKEN -
 * Copyright (c) 2000-2005, RIKEN, Japan, All right reserved.
 */
#ifndef _NVR_BSP_TREE_H_
#define _NVR_BSP_TREE_H_

#include <deque>
#include "utilMath.h"
#include "utilMemory.h"
#include "nvrDefs.h"

namespace NVR {
  template<class T> inline T Min3(const T a, const T b, const T c) {
    if ( a < b ) return (a < c) ? a : c;
    else return (b < c) ? b : c;
  }
  template<class T> inline T Max3(const T a, const T b, const T c) {
    if ( a > b ) return (a > c) ? a : c;
    else return (b > c) ? b : c;
  }

  static inline size_t WrapPow2(const size_t x, const size_t max =4096) {
    register int s;
    for ( s = max; s > 1; s/= 2 )
      if ( s < x ) break;
    return (s * 2);
  }
  static inline bool IsPow2(unsigned int x) {
    return ((x > 0) && !(x & (x - 1)));
  }

  static inline NVR::Dim3 CalcDivTimes(Dim3 dims, const size_t n =128) {
    Dim3 dtm;
    register size_t i;
    for ( i = 0; i < 3; i++ ) {
      register size_t x, j = 0;
      x = WrapPow2(dims[i] + (0x1<<j) -1);
      while ( x / (0x1<<j) > n ) {
	x = WrapPow2(dims[i] + (0x1<<(++j)) -1);
      } // end of while(x)
      dtm[i] = j;
    } // end of for(i)
    return dtm;
  }

  static inline NVR::Dim3 GetWrapDims(const Dim3& dims, const Dim3& dtm) {
    Dim3 xdims;
    xdims[0] = WrapPow2(dims[0] + (0x1<<dtm[0]) -1) - ((0x1<<dtm[0]) -1);
    xdims[1] = WrapPow2(dims[1] + (0x1<<dtm[1]) -1) - ((0x1<<dtm[1]) -1);
    xdims[2] = WrapPow2(dims[2] + (0x1<<dtm[2]) -1) - ((0x1<<dtm[2]) -1);
    return xdims;
  }
};


//-----------------------------------------------------------
//  class nvrVolArea
//-----------------------------------------------------------
class nvrVolArea {
public:
  nvrVolArea(const int id =-1) : m_id(id), m_prior(0), p_refBsp(NULL) {}
  nvrVolArea(const nvrVolArea& org, const int id =-1)
    : m_id(id) {*this = org;}
  virtual ~nvrVolArea() {}

  void operator=(const nvrVolArea& org) {
    m_start = org.m_start; m_dims = org.m_dims;
    m_wholeDims = org.m_wholeDims;
    m_bbox[0] = org.m_bbox[0]; m_bbox[1] = org.m_bbox[1];
  }

  //! 開始インデックスの設定
  void SetStart(const NVR::Dim3& st) {m_start = st;}
  //! サイズの設定
  void SetDims(const NVR::Dim3& dims) {m_dims = dims;}
  //! 全体サイズの設定
  void SetWholeDims(const NVR::Dim3& whole) {m_wholeDims = whole;}
  //! バウンディングボックスの設定
  void SetBbox(const CES::Vec3<float>* pbbox) {
    if ( pbbox ) {m_bbox[0] = pbbox[0]; m_bbox[1] = pbbox[1];}
  }

  //! 開始インデックスを返す
  NVR::Dim3 GetStart() const {return m_start;}
  //! サイズを返す
  NVR::Dim3 GetDims() const {return m_dims;}
  //! 全体サイズを返す
  NVR::Dim3 GetWholeDims() const {return m_wholeDims;}
  //! バウンディングボックスを返す
  const CES::Vec3<float>* GetBbox() const {return m_bbox;}

  //! リファレンスデータポインタを返す
  const void* GetRefBsp() const {return p_refBsp;}
  //! リファレンスデータの設定
  void SetRefBsp(void* ref) {p_refBsp = ref;}

  //! データの有効性の評価
  bool IsValid() const {
    if ( m_wholeDims[0] < m_dims[0] || m_wholeDims[1] < m_dims[1] ||
	 m_wholeDims[2] < m_dims[2] ) return false;
    CES::Vec3<float> bbsz = m_bbox[1] - m_bbox[0];
    if ( bbsz[0] < 0.f || bbsz[1] < 0.f || bbsz[2] < 0.f ) return false;
    return true;
  }

  //! 優先度を設定する
  void SetPrior(const size_t p) {m_prior = p;}
  //! 優先度を返す
  size_t GetPrior() const {return m_prior;}

  //! 識別番号を返す
  int GetId() const {return m_id;}

  CXX_ALLOCATOR_DEFINITION;

protected:
  NVR::Dim3 m_start;
  NVR::Dim3 m_dims;
  NVR::Dim3 m_wholeDims;
  CES::Vec3<float> m_bbox[2];
  int m_id;

  void* p_refBsp;
  size_t m_prior;
};


//-----------------------------------------------------------
//  class nvrVolAreaFactory : Abstract factory for nvrVolArea
//-----------------------------------------------------------
class nvrVolAreaFactory {
public:
  //! nvrVolAreaの生成
  virtual nvrVolArea* Create() const {return new nvrVolArea();}
};


//-----------------------------------------------------------
//  class nvrBspTree
//-----------------------------------------------------------
class nvrBspTree {
public:
  enum AreaType {NegArea =-1, PosArea =1, InvalidArea =0};
  enum PartType {LeafBit =0,
                 ZPartBit =0x1, YPartBit =(0x1<<1), XPartBit =(0x1<<2),
                 PartForRoot =ZPartBit};

  nvrBspTree(const nvrBspTree* parent =NULL,
	     const PartType part =LeafBit) :
    m_dirty(false), m_inFront(false), m_lastResult(InvalidArea),
    p_parent(NULL), m_front(NULL), m_back(NULL), m_area(NULL),
    m_part(part), m_level(0) {}
  virtual ~nvrBspTree() {
    if ( m_front ) {delete m_front; m_front = NULL;}
    if ( m_back  ) {delete m_back;  m_back  = NULL;}
    if ( m_area  ) {delete m_area;  m_area  = NULL;}
  }

  //! 親ノードへのポインタを返す
  const nvrBspTree* GetParent() const {return p_parent;}
  //! 分割平面に対する表領域のノードへのポインタを返す
  const nvrBspTree* GetFrontNode() const {return m_front;}
  //! 分割平面に対する裏領域のノードへのポインタを返す
  const nvrBspTree* GetBackNode() const {return m_back;}
  //! 末端ノードの場合の、ボリューム領域へのポインタを返す
  const nvrVolArea* GetVolArea() const {return m_area;}
  //! 分割平面のタイプを返す
  PartType GetPartType() const {return m_part;}
  //! ツリー全体における、ルートからの階層レベルを返す
  size_t GetLevel() const {return m_level;}
  //! 親ノードの分割平面に対し、表領域にあるかを返す
  bool IsInFront() const {return m_inFront;}

  //! 親ノードの分割平面に対する反対側のボリューム領域へのポインタを返す
  const nvrVolArea* const GetPartner() const;

  //! 表示優先度の計算
  /*! 配下のツリーの表示優先度を計算します。
      優先度は、ルートノードのCalcPrioritiesに渡したcurPriから数え、
      末端のm_areaへの到達順に加算した値がm_areaに登録されます。
      (小さい値ほど優先度が高い)
  */
  size_t CalcPriorities(const size_t curPri,
                        const CES::Vec3<float>& eye,
                        const CES::Mat4<float>* pMVM =NULL,
                        const AreaType res =InvalidArea);

  //! 前回の表示優先度の判定結果を返す
  /*! 最後にCalcPriorities()が呼び出された際に、引数resで渡された
      表示優先度の判定結果を保持し、それを返します。
  */
  AreaType GetLastResult() const {
    if ( m_dirty ) return InvalidArea;
    return m_lastResult;
  }

  //! ボリュームデータ領域のバウンディングボックスの再設定
  /*! 引数で渡されたボリュームデータ領域のバウンディングボックスデータから、
      分割平面を通る点m_cp=(bb[0]+bb[1])/2)を計算し、設定します。
      末端ノードの場合はボリューム領域のバウンディングボックスを設定し、
      そうでない場合は表および裏領域のノードに対して
      再帰的にAdjustBbox()を呼び出します。
   */
  bool AdjustBbox(const CES::Vec3<float>* bb);

  //! 有効範囲設定
  bool AdjustValidDims(const NVR::Dim3& dims);

  //---------- static methods ----------

  //! 分割レベルに対する分割数(2のlevel乗)を返す
  static inline size_t LevelToAreaNum(const size_t level) {
    return (0x1 << level); // returns 2^level
  }

  //! BSPツリーを構築し、ルートノードを返す
  /*! 引数vaで表されるボリュームデータ領域に対し、levelで示される
      分割レベルまでBSP分割を行ったBSPツリーデータを構築し、
      ルートノードのポインタを返します。
      引数pvaListがNULLでない場合は、末端ノードのボリューム領域のポインタを
      生成された順にpvaListに格納します。
   */
  static nvrBspTree* CreateTree(const size_t level, const nvrVolArea& va,
				const nvrVolAreaFactory& leafFac,
				std::deque<nvrVolArea*>* pvaList =NULL);

  CXX_ALLOCATOR_DEFINITION;

protected:
  nvrBspTree *p_parent;
  nvrBspTree *m_front, *m_back;
  nvrVolArea *m_area;
  CES::Vec3<float> m_cp;
  PartType m_part;
  bool m_inFront;
  size_t m_level;

  volatile mutable AreaType m_lastResult;
  volatile mutable bool m_dirty;

  //! 子のノードに対し、親ノードのポインタと表領域にあるかを設定
  void InFront(const bool f, nvrBspTree* p) {m_inFront = f; p_parent = p;}

  //! 表領域および裏領域のノード(子のノード)を再帰的に生成する
  bool CreateBranch(const size_t maxl, const nvrVolArea& va,
		    const nvrVolAreaFactory& leafFac,
                    std::deque<nvrVolArea*>& vaList);

  //! 分割平面に対し、視点が表領域にあるか裏領域にあるかを判定する
  /*! 分割平面に対して、引数eyeで示される視点が表領域にあるか
      裏領域にあるかを判定し、返します。
      引数pMVMがNULLでない場合は、分割平面にpMVMで表される幾何変換行列を
      適用してから判定を行います。
      もし自分が末端ノードの場合は、判定結果としてInvalidAreaを返します。
  */
  AreaType GetAreaOfEye(const CES::Vec3<float>& eye,
                        const CES::Mat4<float>* pMVM =NULL) const;

};

#endif // _NVR_BSP_TREE_H_
