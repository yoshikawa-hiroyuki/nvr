//
// nvrOglExt
//

#ifdef WINDOWS
#include "nvrOglExt.h"

// the actual function pointer instances
PFNGLACTIVETEXTUREPROC glActiveTexture;
PFNGLCLIENTACTIVETEXTUREPROC glClientActiveTexture;
PFNGLMULTITEXCOORD2FPROC glMultiTexCoord2f;
PFNGLMULTITEXCOORD3FPROC glMultiTexCoord3f;
PFNGLMULTITEXCOORD4FPROC glMultiTexCoord4f;
PFNGLTEXIMAGE3DPROC glTexImage3D;


bool InitExtensions() {
  glActiveTexture
    = (PFNGLACTIVETEXTUREPROC)wglGetProcAddress("glActiveTexture");
  if ( glActiveTexture == 0 ) return false;

  glClientActiveTexture
    = (PFNGLCLIENTACTIVETEXTUREPROC)wglGetProcAddress("glClientActiveTexture");
  if ( glClientActiveTexture == 0 ) return false;

  glMultiTexCoord2f
    = (PFNGLMULTITEXCOORD2FPROC)wglGetProcAddress("glMultiTexCoord2f");
  if ( glMultiTexCoord2f == 0 ) return false;

  glMultiTexCoord3f
    = (PFNGLMULTITEXCOORD3FPROC)wglGetProcAddress("glMultiTexCoord3f");
  if ( glMultiTexCoord3f == 0 ) return false;

  glMultiTexCoord4f
    = (PFNGLMULTITEXCOORD4FPROC)wglGetProcAddress("glMultiTexCoord4f");
  if ( glMultiTexCoord4f == 0 ) return false;


  glTexImage3D = (PFNGLTEXIMAGE3DPROC)wglGetProcAddress("glTexImage3D");
  if ( glTexImage3D == 0 ) return false;

  return true;
}

#endif // WINDOWS

