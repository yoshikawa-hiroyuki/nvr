/*
 * 4DVis - 4D Visualization system of RIKEN -
 * Copyright (c) 2000-2006, RIKEN, Japan, All right reserved.
 */

#ifndef _NVR_BRICK_H_
#define _NVR_BRICK_H_

#include <stdlib.h>
#include <string.h>
#include "nvrBspTree.h"
#include "nvrLUT.h"


//-----------------------------------------------------------
//  class nvrBrick : Reducible volume sub-brick
//-----------------------------------------------------------

//! ボリュームサブブロッククラス
/*! ボリュームデータの部分領域(サブブロック)を表現するクラスです。
    サブブロックの情報は、元のボリュームデータにおける開始インデックスと
    部分領域のサイズによって表現されます。
*/
class nvrBrick : public nvrVolArea {
public:
  enum RefDataType {RD_Whole, RD_Brick};

  //! デフォルトコンストラクタ
  nvrBrick() :
    nvrVolArea(), m_dataFormat(NVR::UNSIGNED_BYTE),
    m_reduceLevel(0), m_reducedDims(), m_testLevel(0), m_refDataType(RD_Whole),
    m_data(NULL), p_refData(NULL), p_refLUT(NULL), p_localPtr(NULL) {
    m_bbox[0] = CES::Vec3<float>(0,0,0);
    m_bbox[1] = CES::Vec3<float>(0,0,0);
    if ( NVR::g_useTexColorTable )
      m_texFormat = NVR::LUMINANCE;
    else
      m_texFormat = NVR::RGBA;
  };
  //! デストラクタ
  virtual ~nvrBrick() {
    if (m_data) CES::DeAllocate(m_data);
  }

  //! 点pからブロック中心までの距離を返す
  /*! 点pからブロック中心までの距離を返します。
      引数mがNULLでなければ、ブロック中心座標に幾何変換行列mを適用した結果の
      座標値とpとの距離を返します。
  */
  float GetDistance(const CES::Vec3<float>& p,
		    const CES::Mat4<float>* m =NULL) const {
    CES::Vec3<float> c = (m_bbox[0] + m_bbox[1]) * 0.5f;
    if ( m ) c = (*m) * c;
    return (c - p).Length();
  }

  //! オリジナルとの誤差を返す
  /*! オリジナルの解像度のデータに対し、縮小度レベルをlevelに設定した場合の
      誤差を返します。
  */
  float GetResidualErr(const unsigned int level) const;

  //! 現在のテストレベルを返す
  /*! 現在設定されている縮小度テストレベルを返します。
  */
  unsigned int GetTestLevel() const {return m_testLevel;}
  //! テストレベルを設定する
  /*! 縮小度テストレベルを設定します。
      縮小度テストレベルは、実際に解像度縮小を行う前に、ある縮小度レベルに
      解像度を落した場合の、モデル全体の誤差を評価するために設定します。
  */
  void SetTestLevel(const unsigned int tl) const {m_testLevel = tl;}

  //! サブブロックサイズを返す
  /*! ボリュームサブブロックの格子サイズを返します。 */
  const NVR::Dim3& GetDims() const {return m_dims;}

  //! サブブロックの開始インデックスを返す
  /*! ボリュームデータ全体における、サブブロックの開始インデックス
      (オフセットインデックス)を返します。
  */
  const NVR::Dim3& GetStart() const {return m_start;}

  //! 解像度縮小後のサイズを返す
  /*! 解像度縮小を行った後のボリュームサブブロックの格子サイズを返します。
      解像度縮小が行われていない場合は、(0,0,0)を返します。
  */
  const NVR::Dim3& GetReducedDims() const {return m_reducedDims;}

  //! 解像度縮小レベルを返す
  /*! 解像度縮小が行われている場合の縮小度レベルを返します。 */
  unsigned int GetReduceLevel() const {return m_reduceLevel;}

  //! データフォーマットを返す
  /*! ボリュームデータのデータフォーマットを返します。 */
  NVR::DataFormat GetDataFormat() const {return m_dataFormat;}
  //! 内部テクスチャフォーマットを返す
  /*! ボリュームレンダリング時に使用するOpenGLのテクスチャフォーマットを
      返します。テクスチャカラーテーブルがサポートされていないOpenGL環境では、
      NVR::RGBAが使用されます。
  */
  NVR::TexFormat GetTexFormat() const {return m_texFormat;}

  //! サブブロックのバウンディングボックスを返す
  /*! ボリュームサブブロックのバウンディングボックス(座標範囲)を返します。 */
  const CES::Vec3<float>* GetBbox() const {return m_bbox;}

  //! 解像度縮小後のデータへのポインタを返す
  /*! 解像度縮小を行った結果のボリュームデータへのポインタを返します。
      ボリュームレンダリングモードがNVR::T3Dの場合と、NVR::T2Dの場合で
      視線方向軸が+Zまたは-Zの場合に使用するデータへのポインタが返されます。
  */
  const void* GetDataPtr() const {return m_data;}
  //! 解像度縮小後のデータへのポインタを返す(視線軸方向指定)
  /*! 解像度縮小を行った結果のボリュームデータへのポインタを返します。
      ボリュームレンダリングモードがNVR::T2Dの場合の、視線方向軸がaxisの時に
      使用するデータへのポインタが返されます。
  */
  const void* GetDataPtr(const int axis) const;

  //! 関連データポインタの設定
  /*! サブブロックに関連づけられたデータ領域へのポインタを設定します。 */
  void SetLocalPtr(void* p) {p_localPtr = p;}
  //! 関連データポインタの取得
  /*! サブブロックに関連づけられたデータ領域へのポインタを返します。 */
  void* GetLocalPtr() {return p_localPtr;}

  //! リファレンスデータの設定
  bool SetRefData(void* p, const NVR::DataFormat format,
		  const RefDataType rdt =RD_Whole);

  //! LUTの設定
  /*! 参照するLUT(カラールックアップテーブル)へのポインタを設定します。
      LUTへのポインタそのものに変更が無い場合でも、参照先のLUTの内容が
      変更された場合にも、コールする必要があります。
  */
  void SetRefLUT(nvrLUT* lut) {p_refLUT = lut; LutUpdated();}

  //! データのリデュース
  /*! 指定された縮小度レベルへの、解像度縮小を行います。
      解像度縮小を行った結果のデータは、V[K][J][I]のデータ並びを保ったものが
      まずストアされ、更にボリュームレンダリングモードがNVR::T2Dの場合は、
      V[I][K][J], V[J][I][K]のデータ並びのものがストアされます。
  */
  bool Reduce(const unsigned int level);

  //! テクスチャプロキシーサイズを返す
  /*! 指定された縮小度レベルに解像度縮小した際に、必要となるテクスチャデータの
      サイズ(bytes)を返します。
  */
  size_t GetProxySize(const unsigned int level) const {
    if ( ! p_refData ) return 0;
    register size_t elmSz = NVR::g_useTexColorTable ? 1 : 4;
    if ( level == 0 ) return m_dims.Size() * elmSz;
    NVR::Dim3 ndim(m_dims);
    ndim.size[0] >>= level; ndim.size[1] >>= level; ndim.size[2] >>= level;
    return ndim.Size() * elmSz;
  }

  //! 描画
  /*! 指定された視線軸方向における、ボリュームサブブロックの描画を行います。
      nvrBrickではサブブロックのバウンディングボックスの描画が実装されており、
      ボリュームレンダリングは派生クラスで実装されます。
  */
  virtual void Draw(const int axis) const;

  //! テクスチャオブジェクト使用モード
  virtual void InvalidateTexObjs() {}

  //! OpenGLエクステンションのチェック
  virtual bool CheckReqOglExt() const;

  /* static members */

  //! ボリュームレンダリングモード(T2D | T3D)
  /*! ボリュームレンダリング時に使用するテクスチャの種別を設定します。*/
  static NVR::nvrRenderMode s_renderMode;

  // Vecoe data allocator
  CXX_ALLOCATOR_DEFINITION;

protected:
  NVR::Dim3 m_reducedDims;
  unsigned int m_reduceLevel;
  NVR::DataFormat m_dataFormat;
  NVR::TexFormat m_texFormat;
  RefDataType m_refDataType;
  void* m_data;
  void* p_refData;
  nvrLUT* p_refLUT;
  void* p_localPtr;

  // Post-Update routine
  virtual void RefUpdated() {}
  virtual void LutUpdated() {}
  virtual void PostReduced() {}

  // Data allocation for Reduce
  bool AllocData(const NVR::Dim3&);

  // structure for Residual error cache
  struct RSizeCache {
    unsigned int m_level;
    float m_error;
    RSizeCache() : m_level(0), m_error(1.f) {}
  };
  mutable RSizeCache m_errCache;
  mutable unsigned int m_testLevel;
};


//-----------------------------------------------------------
//  class nvrBrickFactory : Abstract factory for nvrBrick
//-----------------------------------------------------------

//! nvrBrick用ファクトリークラス
class nvrBrickFactory : public nvrVolAreaFactory {
public:
  //! nvrBrickの生成
  /*! nvrBrickのインスタンスを生成します。*/
  virtual nvrVolArea* Create() const {return new nvrBrick();}
};

#endif // _NVR_BRICK_H_
