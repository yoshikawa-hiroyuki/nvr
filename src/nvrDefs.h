/*
 * 4DVis - 4D Visualization system of RIKEN -
 *
 * Copyright (c) The Institute of Physical and Chemical Research, 2000-2006
 *           All right reserved.
 */

#ifndef _NVR_DEFS_H_
#define _NVR_DEFS_H_

#ifdef WINDOWS
#include "nvrOglExt.h"
#else // WINDOWS
#define GL_GLEXT_PROTOTYPES 1
#ifdef __APPLE__
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else // __APPLE__
#include <GL/gl.h>
#include <GL/glu.h>
#endif // __APPLE__
#endif // WINDOWS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


//---------------  Check OpenGL errors ---------------
#ifdef WINDOWS
#define DO_GL_DEBUG 0
#else
# ifdef _GL_DEBUG
#define DO_GL_DEBUG 1
# else
#define DO_GL_DEBUG 0
# endif // _GL_DEBUG
#endif // WINDOWS

#if DO_GL_DEBUG
#define GLERROR_CHECK do {\
  GLenum error; int i = 0; \
  while((error = glGetError()) != GL_NO_ERROR) \
    fprintf(stderr, "GL error: %s:%d (%d) : %s\n", \
      __FILE__, __LINE__, i++, gluErrorString(error)); \
} while(0)
#else
#define GLERROR_CHECK do {;} while(0)
#endif // DO_GL_DEBUG

//! 絶対値
#ifndef iabs
#define iabs(x)   ((x)<0?-(x):(x))
#endif


//--------------- NVR namespace ---------------
//! NVR名前空間
/*! NVRライブラリが使用するデータタイプが定義される名前空間です。
 */
namespace NVR {
  static const char Version[]
    = "version 2.1";

  //! テクスチャタイプ
  enum nvrRenderMode {
    T2D = GL_TEXTURE_2D,
    T3D = GL_TEXTURE_3D
  };
  //! ボリュームデータフォーマットタイプ
  enum DataFormat {
    UNSIGNED_BYTE   = GL_UNSIGNED_BYTE,
    UNSIGNED_SHORT  = GL_UNSIGNED_SHORT,
    UNSIGNED_INT    = GL_UNSIGNED_INT,
    FLOAT           = GL_FLOAT
  };
  //! テクスチャデータフォーマットタイプ
  enum TexFormat {
    COLOR_INDEX     = GL_COLOR_INDEX,
    RGBA            = GL_RGBA,
    LUMINANCE_ALPHA = GL_LUMINANCE_ALPHA,
    LUMINANCE       = GL_LUMINANCE
  };
  //! 解像度縮小ポリシータイプ
  enum ReducePolicy {
    MEMORY_SIZE,
    ERROR_RATIO,
    SIMPLE
  };
  //! 誤差レベル登録タイプ
  enum ErrLevelType {
    TEST_LEVEL,
    REDUCED
  };
  //! ブレンディングモードタイプ
  enum BlendingType {
    OVER_BLENDING,
    MAXIMUM_INTENSITY
  };
  //! ボリュームレンダリング方向
  enum DrawDirectionType {
    BTF,
    FTB
  };

  //! ボリュームサイズ定義構造体
  struct Dim3 {
    size_t size[3];
    //! デフォルトコンストラクタ
    Dim3(const size_t a =0, const size_t b =0, const size_t c =0) {
      size[0] = a; size[1] = b; size[2] = c;
    }
    //! コピーコンストラクタ
    Dim3(const Dim3& org) {*this = org;}
    //! 代入オペレータ
    void operator=(const Dim3& org) {
      size[0]=org.size[0]; size[1]=org.size[1]; size[2]=org.size[2];
    }
    //! トータルサイズの計算
    size_t Size() const {return size[0]*size[1]*size[2];}
    //! []オペレータ
    size_t& operator[](const size_t i) {return size[i%3];}
    size_t operator[](const size_t i) const {return size[i%3];}
  };

  //! テクスチャカラーテーブル使用フラグ
  extern bool g_useTexColorTable;

  //! アルファウエイト参照フラグ
  extern bool g_useAlphaWeight;

  //! テクスチャオブジェクト使用フラグ
  extern bool g_useTexObject;

  //! ボリュームレンダリング方向(BTF|FTB)
  extern DrawDirectionType g_drawDirection;


  //! OpenGL拡張機能のテスト
  static bool nvrQueryGlExt(const char* extName);
};


// static inline functions

static inline bool NVR::nvrQueryGlExt(const char* extName) {
#if 0
  const GLubyte* p = glGetString(GL_EXTENSIONS);
  if ( ! p ) return false;
  return gluCheckExtension((GLubyte*)extName, p);
#else
  char* p = (char*)glGetString(GL_EXTENSIONS);
  if ( ! p ) return false;
  char* end = p + strlen(p);
  while ( p < end ) {
    size_t n = strcspn(p, " ");
    if ( (strlen(extName) == n) && (strncmp(extName, p, n) == 0) )
      return true;
    p += (n + 1);
  }
  return false;
#endif
}

#endif // _NVR_DEFS_H_
