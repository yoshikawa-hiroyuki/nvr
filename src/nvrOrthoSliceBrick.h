/*
 * 4DVis - 4D Visualization system of RIKEN -
 * Copyright (c) 2000-2006, RIKEN, Japan, All right reserved.
 */

#ifndef _NVR_ORTHO_SLICE_BRICK_H_
#define _NVR_ORTHO_SLICE_BRICK_H_

#ifdef WINDOWS
#include "nvrOglExt.h"
#endif
#include "utilOglTexEnv.h"
#include "nvrBrick.h"


//-----------------------------------------------------------
//  class nvrOrthoSliceBrick : 
//    Volume sub-brick with orthogonal slice based volume rendering
//-----------------------------------------------------------

//! 直交スライスによるボリュームレンダリング可能なボリュームサブブロッククラス
/*! 直交スライス(各軸方向に垂直な断面スライス)を使用したテクスチャマッッピング
    ベースのボリュームレンダリング機能を実装したボリュームサブブロックの
    クラスです。
*/
class nvrOrthoSliceBrick : public nvrBrick {
public:
  //! デフォルトコンストラクタ
  nvrOrthoSliceBrick() : nvrBrick(),
    m_numTids(0), m_tids(NULL), m_loaded(false) {
  }
  //! デストラクタ
  virtual ~nvrOrthoSliceBrick() {
    if ( m_tids ) {
      if ( m_numTids > 0 )
        glDeleteTextures(m_numTids, m_tids);
      CES::DeAllocate(m_tids);
    }
  }

  //! 描画
  /*! 指定された視線軸方向における、ボリュームサブブロックの描画を行います。
      視線軸方向に垂直な断面スライスにテクスチャマッピングを行い、
      Back To Frontにブレンディングしながら描画することで、ボリューム
      レンダリングを行います。
  */
  virtual void Draw(const int axis) const;

  //! テクスチャオブジェクト使用モード
  virtual void InvalidateTexObjs();

  //! OpenGLエクステンションのチェック
  virtual bool CheckReqOglExt() const;


  //! オプショナル中間スライス生成数
  /*! 描画時に、元のボリュームデータの解像度に対して中間スライスを
      生成する枚数を各軸方向毎に設定します。
  */
  static NVR::Dim3 s_intermediate;

  //! TEXTURE_COMBINER使用フラグ
  /*! 中間スライス生成時に、TEXTURE_COMBINER拡張を使用するかどうかを
      設定します。
  */
  static bool s_useCombine2D;

protected:
  mutable GLsizei m_numTids;
  mutable GLuint* m_tids;
  mutable bool m_loaded;

  // Post-Update routine
  virtual void RefUpdated() {m_loaded = false;}
  virtual void LutUpdated() {
    if ( NVR::g_useTexColorTable )
      m_loaded = false;
    else
      Reduce(m_testLevel);
  }
  virtual void PostReduced() {m_loaded = false;}

  // Texture object ids
  bool GenTexIds() const;
  bool BuildTexObj2D() const;
  bool BuildTexObj3D() const;
  size_t GetNumNeedTexIds() const {
    if ( s_renderMode == NVR::T3D )
      return 1;
    else
      return m_reducedDims.size[0]+m_reducedDims.size[1]+m_reducedDims.size[2];
  }
  virtual size_t GetTexId2D(const int axis, const size_t sl) const {
    if ( ! m_tids ) return 0;
    switch ( axis ) {
    case -1: case 1:
      return m_tids[m_reducedDims.size[2] + sl];
    case -2: case 2:
      return m_tids[m_reducedDims.size[2] + m_reducedDims.size[0] + sl];
    case -3: case 3:
      return m_tids[sl];
    }
    return 0;
  }
  virtual size_t GetTexId3D() const {
    if ( ! m_tids ) return 0;
    return m_tids[0];
  }

  void SetupTexUnits2D(const GLenum, const bool activate =false) const;

  // Volume renderer by using TEXTURE_2D
  void DrawSlices2D(const int axis) const;

  // Volume renderer by using TEXTURE_3D
  void DrawSlices3D(const int axis) const;
};


//-----------------------------------------------------------
//  class nvrOrthoSliceBrickFactory : 
//    Abstract factory for nvrOrthoSliceBrick
//-----------------------------------------------------------

//! nvrOrthoSliceBrick用ファクトリークラス
class nvrOrthoSliceBrickFactory : public nvrBrickFactory {
public:
  //! nvrOrthoSliceBrickの生成
  /*! nvrOrthoSliceBrickのインスタンスを生成します。*/
  virtual nvrVolArea* Create() const {
    return new nvrOrthoSliceBrick();
  }

  //! OpenGLテクスチャコンバイナー
  static CES::TexEnvCombiner* GetTexEnvCombiner(const int unit);
};

#endif // _NVR_ORTHO_SLICE_BRICK_H_

