// TEST_NVR
#include <stdio.h>
#include <vector>

#include "nvrOglExt.h"

#include "vfrDrawAreaSDL.h"
#include "vfrImageReaderSDL.h"
#include "vfrScreen.h"
#include "vfrCamera3D.h"
#include "vfrScene.h"
#include "vfrGroup.h"
#include "vfrCube.h"
#include "vfrDefaultActions.h"

#include "nvrRender.h"
#include "nvrOrthoSliceBrick.h"

using namespace std;
using namespace CES;
using namespace NVR;


vfrScreen* screen;
vfrCamera3D* camera;
vfrScene* scene;
vfrGroup* root;
nvrRender s_render;

Bool READ_DATA(const size_t n, const char* fname)
{
  FILE* fp;
  unsigned char xdim[3];
  unsigned char* wkdp;
  float* datap;

  fp = fopen(fname, "rb");
  if ( ! fp ) return FALSE;
  fread(xdim, 1, 3, fp);
  Dim3 dims(xdim[0], xdim[1], xdim[2]);
  if ( dims.Size() < 1 ) {
    fclose(fp); return FALSE;
  }
  dims = GetWrapDims(dims, CalcDivTimes(dims, n));
  printf("dims = (%d, %d, %d)\n", dims[0], dims[1], dims[2]);
  datap = new float[dims.Size()];
  wkdp = new unsigned char[xdim[0]];
  if ( ! datap || ! wkdp ){
    if ( datap ) delete [] datap;
    if ( wkdp ) delete [] wkdp;
    fclose(fp); return FALSE;
  }
  memset(datap, 0, sizeof(float)*dims.Size());

  register size_t i, j, k, idx;
  for ( k = 0; k < xdim[2]; k++ )
    for ( j = 0; j < xdim[1]; j++ ) {
      fread(wkdp, 1, xdim[0], fp);
      for ( i = 0; i < xdim[0]; i++ ) {
	idx = dims[0] * dims[1] * k + dims[0] * j + i;
	datap[idx] = (float)wkdp[i];
      }
    }
  fclose(fp);

  Vec3<float> bb[] = {Vec3<float>(-1.f, -1.f, -1.f),
		      Vec3<float>(1.f, 1.f, 1.f)};
  nvrOrthoSliceBrickFactory brickFactory;
  if ( s_render.Initialize(&brickFactory, NVR::FLOAT, dims, n) ) {
    //s_render.GenTexNames();
    if ( s_render.UpdateData(datap, bb) ) {
      nvrLUT lut;
      s_render.UpdateLUT(lut);
    } else {
      return FALSE;
    }
  } else
    return FALSE;

  return TRUE;
}

class VRender : public vfrNode {
public:
  VRender() : p_render(NULL), firstTime(TRUE) {
    setAlpha(TRUE);
  }
  virtual ~VRender() {}

  virtual void renderSolid() {
    if ( ! p_render ) return;
    if (firstTime ) {
      //p_render->Reduce(NVR::ERROR_RATIO, 0.05f);
      p_render->Reduce(NVR::MEMORY_SIZE);
      //p_render->Reduce(NVR::SIMPLE, 0);

      nvrOrthoSliceBrickFactory brickFactory;
      if ( p_render->CheckReqExtensions(&brickFactory) )
        printf("Required OpenGL Extensions supported.\n");
      else
        printf("Required OpenGL Extensions NOT supported.\n");

      GLboolean isRGBA; GLint alphaBits;
      glGetBooleanv(GL_RGBA_MODE, &isRGBA);
      glGetIntegerv(GL_ALPHA_BITS, &alphaBits);
      printf("RGBA=%s , ALPHA=%d\n", (isRGBA ? "YES" : "NO"), alphaBits);

      firstTime = FALSE;
    }
    p_render->DrawVolume();
  }
  virtual void renderWire() {
    if ( ! p_render ) return;
    glColor3f(1.f, 1.f, 1.f);
    p_render->DrawBbox();
  }

  nvrRender* p_render;
  Bool firstTime;
};

VRender* render;

int main(int argc, char**argv) {
  //const size_t n = 32;
  const size_t n = 1;
  nvrBrick::s_renderMode = NVR::T3D;
  //nvrBrick::s_renderMode = NVR::T2D;
  //nvrOrthoSliceBrick::s_intermediate = NVR::Dim3(1,1,1);
  //nvrOrthoSliceBrick::s_useCombine2D = false;
  NVR::g_useTexObject = false;
  //NVR::g_useTexColorTable = false;
  //NVR::g_useAlphaWeight = true;
  //NVR::g_drawDirection = NVR::FTB;


  vfrDrawAreaSDL* da = vfrDrawAreaSDL::GetInstance();
  da->initialize("NVR VIEW", 640, 480);

  screen = new vfrScreen();
  da->addScreen(screen);
  camera = new vfrCamera3D();
  camera->setBgColor(0.6, 0.4, 0.2);
  screen->setCamera(camera);
  scene = new vfrScene("Scene", TRUE);
  camera->setScene(scene);
  root = new vfrGroup("ROOT", TRUE);
  scene->addChild(root);

  vfrMaterial* rMate = root->alcMaterial();
  rMate->setRenderMode(RT_NOLIGHT | RT_WIRE);

  render = new VRender();
  render->p_render = &s_render;
  root->addChild(render);
  vfrDefaultActions::SetSelectedNode(root);

  vfrDefaultActions::SetDefaultAction(*da);
  vfrDispatch& dpr = vfrDispatch::instance(*da);
  vfrEvSDrag::instance(dpr).regist(&vfrTransNodeAction::instance());
  vfrEvCDrag::instance(dpr).regist(&vfrRotNodeAction::instance());
  vfrEvSCDrag::instance(dpr).regist(&vfrScaleNodeAction::instance());
  vfrEvClick::instance(dpr).regist(NULL);


  if ( argc < 2 ) return 0;
  if ( ! READ_DATA(n, argv[1]) )
    return 0;


  while( 1 ) {
    if ( ! da->ProcessEvent() ) break;
    da->redraw();
  }

  return 0;
}
