/*
 * 4DVis - 4D Visualization system of RIKEN -
 * Copyright (c) 2000-2006, RIKEN, Japan, All right reserved.
 */
#ifndef _NVR_RENDER_H_
#define _NVR_RENDER_H_

#include <deque>
#include <set>
#include "nvrBrick.h"

#include "utilOglSL.h"


//-----------------------------------------------------------
//  class nvrRender : Volume Renderer
//-----------------------------------------------------------

//! ボリュームレンダラークラス
/*! ボリュームデータに対して、テクスチャマッピングを使用したボリューム
    レンダリングを行うクラスです。
    ボリュームデータはボリュームサブブロックに分割され、
    各サブブロック毎に解像度縮小とボリュームレンダリングが行われます。
*/
class nvrRender {
public:
  //! デフォルトコンストラクタ
  nvrRender();
  //! デストラクタ
  virtual ~nvrRender();

  //! ボリュームレンダリング
  /*! テクスチャマッピングを使用したボリュームレンダリングを行います。
      事前にInitialize()とUpdateData()がコールされている必要があります。
  */
  void DrawVolume(const NVR::BlendingType blendingType =NVR::OVER_BLENDING);

  //! バウンディングボックスの描画
  /*! ボリュームデータ全体のバウンディングボックスをラインで表示します。*/
  void DrawBbox();

  //! 初期化
  /*! ボリュームレンダラーの初期化を行います。
      引数: ボリュームサブブロック生成用ファクトリーへのポインタ,
            ボリュームデータのフォーマット, ボリュームデータのサイズ, 
	    サブブロックの基準サイズ
      指定するボリュームデータのサイズは、2のべき乗でなければなりません。
  */
  bool Initialize(const nvrBrickFactory* pf,
		  const NVR::DataFormat& fmt, const NVR::Dim3& dims,
		  const size_t n =128);

  //! 実サイズ
  bool SetValidDims(const NVR::Dim3& dims);

  //! ボリュームデータの設定(更新)
  /*! ボリュームデータの設定(および更新)を行います。
      設定されるボリュームデータは、Initialize()で設定したフォーマット及び
      サイズのデータでなければなりません。
      バウンディングボックス(bbox)にNULLが指定された場合、
      (0,0,0), (dims[0]-1, dims[1]-1, dims[2]-1)が使用されます。
  */
  bool UpdateData(void* data, const CES::Vec3<float>* bbox =NULL);

  //! LUTの設定(更新)
  /*! ボリュームレンダリング時に使用するLUT(カラールックアップテーブル)の
      設定(および更新)を行います。
  */
  bool UpdateLUT(const nvrLUT& lut);

  //! 解像度縮小
  /*! ボリュームサブブロックの解像度縮小を行います。
      解像度縮小は、指定された解像度縮小ポリシーに基づいて行われます。
      縮小ポリシーがNVR::MEMORY_SIZEの場合、動作環境のOpenGL最大テクスチャ
      サイズを求め、各サブブロックのテクスチャプロキシーサイズの合計が
      この値以下になるまで、最もオリジナルとの誤差の小さいサブブロックの
      解像度を落す処理を繰り返します。
      縮小ポリシーがNVR::ERROR_RATIOの場合、各サブブロックの誤差の合計が
      指定された値に達するまで最も誤差の小さいサブブロックの解像度を落す
      処理を繰り返します。
  */
  bool Reduce(const NVR::ReducePolicy policy, const float ratio =1.f);

  //! 必要なOpenGL拡張のチェック
  /*! サブブロックの生成に必要なOpenGL拡張の実装の有無をチェックします。*/
  bool CheckReqExtensions(const nvrBrickFactory* pf) const;

  //! 視線軸方向の計算
  /*! 与えられたモデルビュー行列でボリュームデータを変換した状態で
      最も視線方向に近い軸方向を計算します。
      戻り値は、1=+I, -1=-I, 2=+J, -2=-J, 3=+K, -3=-K を意味します。
  */
  int FindViewAxis(const GLfloat mvm[16], const float fd =10.f);
  int FindViewAxis(const GLdouble mvm[16], const GLdouble fd =10.);

  int FindViewAxis(const nvrBrick* pb,
		   const CES::Vec3<float>& eye,
		   const CES::Mat4<float>* pMVM =NULL) const;

  //! ボリュームデータのサイズを返す
  const NVR::Dim3& GetDims() const {return m_dims;}
  //! ボリュームデータのフォーマットを返す
  const NVR::DataFormat GetFormat() const {return m_dataType;}
  //! 設定されているLUTへの参照を返す
  const nvrLUT& GetLUT() const {return m_lut;}
  //! ボリュームデータのバウンディングボックスへのポインタを返す
  const CES::Vec3<float>* GetBboxPtr() const {return m_bbox;}
  //! サブブロックリストへの参照を返す
  const std::deque<nvrBrick*>& GetBrickList() const {return m_bl;}
  std::deque<nvrBrick*>& GetBrickList() {return m_bl;}
  //! BspTreeの参照
  nvrBspTree* GetBspTree() {return m_pBspTree;}

  //! テクスチャオブジェクト使用モードの設定
  void SetTexObjMode(const bool tom);


  /*** static methods ***/

  //! 最大テクスチャデータサイズの取得
  static size_t MaxTextureSize();

  CXX_ALLOCATOR_DEFINITION;

protected:
  size_t m_N; // division times
  NVR::Dim3 m_dims;
  NVR::Dim3 m_validDims;
  CES::Vec3<float> m_bbox[2];
  NVR::DataFormat m_dataType;
  nvrLUT m_lut;

  std::deque<nvrBrick*> m_bl;
  std::deque<nvrBrick*> m_blErr;
  std::deque<nvrBrick*> m_blDrw;

  nvrBspTree* m_pBspTree;

  bool CreateBspTree(const nvrBrickFactory* pf);
  size_t GetTotalProxySize(const NVR::ErrLevelType =NVR::TEST_LEVEL);
  void ClearBricks();

  CES::GLSLProg m_glslProg;
  bool SetupGLSLProg();
};

#endif // _NVR_RENDER_H_
