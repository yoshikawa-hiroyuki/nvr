/*
 * 4DVis - 4D Visualization system of RIKEN -
 *
 * Copyright (c) The Institute of Physical and Chemical Research, 2000-2006
 *           All right reserved.
 */

#ifndef _NVR_LUT_H_
#define _NVR_LUT_H_

#include "utilMemory.h"
#include "utilEndian.h"
#include "nvrDefs.h"

//-----------------------------------------------------------
//  struct nvrLUT : Look-Up Table
//-----------------------------------------------------------

//! 色成分の格納数
#define NVR_LUT_LENGTH 256

//! ルックアップテーブル構造体
/*! ボリュームレンダリング用のテクスチャカラールックアップテーブルの
    データ構造体です。データ値とカラー(R, G, B, A)の対応関係を表します。
*/
struct nvrLUT {
  //! カラーデータテーブル
  /*! カラーデータテーブルが格納される配列です。
      ABGR各1byteの値がunsigned intにパックされ、格納されます。
  */
  unsigned int m_abgr[NVR_LUT_LENGTH];
  unsigned int m_abgrAW[NVR_LUT_LENGTH];

  //! デフォルトコンストラクタ
  nvrLUT() {
    register int x;
    for ( register size_t i = 0; i < NVR_LUT_LENGTH; i++ ) {
      register unsigned char r, g, b, a;
      r = g = b = (unsigned char)i;
      a = (unsigned char)i;
      m_abgr[i] = (a << 24) | (b << 16) | (g << 8) | r;      
      x = (int)((float)(i * i) / 255.f);
      r = (unsigned char)(x);
      g = (unsigned char)(x);
      b = (unsigned char)(x);
      m_abgrAW[i] = (a << 24) | (b << 16) | (g << 8) | r;      
    }
    if ( CES::BigEndianSys() ) {
      BSWAPVEC(m_abgr, NVR_LUT_LENGTH);
      BSWAPVEC(m_abgrAW, NVR_LUT_LENGTH);
    }
  }
  //! デストラクタ
  ~nvrLUT() {}

  //! 代入オペレータ
  void operator=(const nvrLUT& org) {
    memcpy(m_abgr, org.m_abgr, sizeof(unsigned int)*NVR_LUT_LENGTH);
    memcpy(m_abgrAW, org.m_abgrAW, sizeof(unsigned int)*NVR_LUT_LENGTH);
  }
  //! カラー値取得
  unsigned int getVal(const int idx) const {
    if ( idx < 0 ) return m_abgr[0];
    else if ( idx >= NVR_LUT_LENGTH-1 ) return m_abgr[NVR_LUT_LENGTH-1];
    else return m_abgr[idx];
  }
  //! カラー値取得(アルファウエイト指定)
  unsigned int getValAlpha(const int idx) const {
    if ( idx < 0 ) return m_abgrAW[0];
    else if ( idx >= NVR_LUT_LENGTH-1 ) return m_abgrAW[NVR_LUT_LENGTH-1];
    else return m_abgrAW[idx];
  }
  //! カラーテーブルからの変換
  bool SetTable(const float* rgba);
  bool SetTable(const unsigned int* abgr);

  //! OpenGLへのカラーテーブルロード
  void LoadTable(const GLenum target =GL_TEXTURE1) const {
    GLint current; glGetIntegerv(GL_ACTIVE_TEXTURE, &current);
    if ( current != target ) {glActiveTexture(target); GLERROR_CHECK;}
    glEnable(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, NVR_LUT_LENGTH, 1, 0,
		 GL_RGBA, GL_UNSIGNED_BYTE, m_abgr);
    GLERROR_CHECK;
    if ( current != target ) {glActiveTexture(current); GLERROR_CHECK;}
  }
  void LoadTableAlpha(const GLenum target =GL_TEXTURE1) const {
    GLint current; glGetIntegerv(GL_ACTIVE_TEXTURE, &current);
    if ( current != target ) {glActiveTexture(target); GLERROR_CHECK;}
    glEnable(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, NVR_LUT_LENGTH, 1, 0,
		 GL_RGBA, GL_UNSIGNED_BYTE, m_abgrAW);
    GLERROR_CHECK;
    if ( current != target ) {glActiveTexture(current); GLERROR_CHECK;}
  }
  void UnLoadTable(const GLenum target =GL_TEXTURE1) const {
    GLint current; glGetIntegerv(GL_ACTIVE_TEXTURE, &current);
    if ( current != target ) {glActiveTexture(target); GLERROR_CHECK;}
    glDisable(GL_TEXTURE_2D);
    if ( current != target ) {glActiveTexture(current); GLERROR_CHECK;}
  }

  CXX_ALLOCATOR_DEFINITION;
};


inline bool nvrLUT::SetTable(const float* rgba)
{
  if ( ! rgba )  return false;
  register unsigned char r, g, b, a;
  for ( register size_t i = 0; i < NVR_LUT_LENGTH; i++ ) {
    r = (unsigned char)(rgba[i*4   ] * 255.f);
    g = (unsigned char)(rgba[i*4 +1] * 255.f);
    b = (unsigned char)(rgba[i*4 +2] * 255.f);
    a = (unsigned char)(rgba[i*4 +3] * 255.f);
    m_abgr[i] = (a << 24) | (b << 16) | (g << 8) | r;
    r = (unsigned char)(rgba[i*4   ] * rgba[i*4 +3] * 255.f);
    g = (unsigned char)(rgba[i*4 +1] * rgba[i*4 +3] * 255.f);
    b = (unsigned char)(rgba[i*4 +2] * rgba[i*4 +3] * 255.f);
    m_abgrAW[i] = (a << 24) | (b << 16) | (g << 8) | r;
  }
  if ( CES::BigEndianSys() ) {
    BSWAPVEC(m_abgr, NVR_LUT_LENGTH);
    BSWAPVEC(m_abgrAW, NVR_LUT_LENGTH);
  }
  return true;
}

inline bool nvrLUT::SetTable(const unsigned int* abgr)
{
  if ( ! abgr )  return false;
  memcpy(m_abgr, abgr, sizeof(m_abgr));

  union {unsigned int iv; unsigned char rgba[4];} x;
  register unsigned char r, g, b; register unsigned int w;
  for ( register size_t i = 0; i < NVR_LUT_LENGTH; i++ ) {
    x.iv = m_abgr[i];
    w = (int)((float)x.rgba[0] * (float)x.rgba[3] / 255.f);
    r = (unsigned char)w;
    w = (int)((float)x.rgba[1] * (float)x.rgba[3] / 255.f);
    g = (unsigned char)w;
    w = (int)((float)x.rgba[2] * (float)x.rgba[3] / 255.f);
    b = (unsigned char)w;
    m_abgrAW[i] = (x.rgba[3] << 24) | (b << 16) | (g << 8) | r;
  }
  if ( CES::BigEndianSys() ) {
    BSWAPVEC(m_abgr, NVR_LUT_LENGTH);
    BSWAPVEC(m_abgrAW, NVR_LUT_LENGTH);
  }
  return true;
}

#endif // _NVR_LUT_H_
