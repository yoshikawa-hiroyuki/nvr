//
// nvrOglExt
//
#ifndef _NVR_OGL_EXT_H_
#define _NVR_OGL_EXT_H_
#ifdef WINDOWS

#include <windows.h>
#include <GL/glew.h>

#include <GL/gl.h>
#include <GL/glu.h>

#if 0
#if defined __cplusplus
extern "C" {
#endif
// The function pointers
extern PFNGLACTIVETEXTUREPROC glActiveTexture;
extern PFNGLCLIENTACTIVETEXTUREPROC glClientActiveTexture;
extern PFNGLMULTITEXCOORD2FPROC glMultiTexCoord2f;
extern PFNGLMULTITEXCOORD3FPROC glMultiTexCoord3f;
extern PFNGLMULTITEXCOORD4FPROC glMultiTexCoord4f;
extern PFNGLTEXIMAGE3DPROC glTexImage3D;
#if defined __cplusplus
}
#endif
#endif


// Prototype of initialization function
bool InitExtensions();

#endif // WINDOWS
#endif //_NVR_OGL_EXT_H_
