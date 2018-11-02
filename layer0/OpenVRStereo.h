/* 
A* -------------------------------------------------------------------
B* This file contains source code for the PyMOL computer program
C* Copyright (c) EPAM Systems, Inc.
D* -------------------------------------------------------------------
E* It is unlawful to modify or remove this copyright notice.
F* -------------------------------------------------------------------
G* Please see the accompanying LICENSE file for further information. 
H* -------------------------------------------------------------------
I* Additional authors of this source file include:
-* 
-* 
-*
Z* -------------------------------------------------------------------
*/
#ifndef _H_OpenVRStereo
#define _H_OpenVRStereo

#include "PyMOLGlobals.h"

bool OpenVRAvailable(PyMOLGlobals * G);
bool OpenVRReady(PyMOLGlobals * G);

int OpenVRInit(PyMOLGlobals * G);
void OpenVRFree(PyMOLGlobals * G);

// PyMOL-style enum
enum {
  cAction_scene_next,
  cAction_scene_prev,

  cAction_movie_toggle,
  cAction_movie_next,
  cAction_movie_prev,
};

class OpenVRInputHandlers {
public:
  virtual void KeyboardFunc(unsigned char k, int x, int y, int mod) {}
  virtual void SpecialFunc(int k, int x, int y, int mod) {}
  virtual int MouseFunc(int button, int state, int x, int y, int mod) { return 0; }
  virtual int MotionFunc(int x, int y, int mod) { return 0; }
  virtual void ActionFunc(int a) {}
};

void OpenVRSetInputHandlers(PyMOLGlobals * G, OpenVRInputHandlers* handlers);

void OpenVRFeedback(PyMOLGlobals * G);

void OpenVRFrameStart(PyMOLGlobals * G);
void OpenVREyeStart(PyMOLGlobals * G, int eye);
void OpenVREyeFinish(PyMOLGlobals * G);
void OpenVRSceneFinish(PyMOLGlobals * G, unsigned sceneX, unsigned sceneY, unsigned sceneWidth, unsigned sceneHeight);
void OpenVRFrameFinish(PyMOLGlobals * G);

void OpenVRGetWidthHeight(PyMOLGlobals * G, int* width, int* height);

void OpenVRMenuBufferStart(PyMOLGlobals * G, unsigned width, unsigned height, bool clear = true);
void OpenVRMenuBufferFinish(PyMOLGlobals * G);
void OpenVRMenuToggle(PyMOLGlobals * G, unsigned deviceIndex = ~0U);
void OpenVRMenuCrop(PyMOLGlobals * G, unsigned x, unsigned y, unsigned width, unsigned height);
void OpenVRMenuSettingsChanged(PyMOLGlobals * G);

float* OpenVRGetWorldToHead(PyMOLGlobals * G);
float* OpenVRGetHeadToEye(PyMOLGlobals * G);
float* OpenVRGetControllerPose(PyMOLGlobals * G);
float* OpenVRGetProjection(PyMOLGlobals * G, float near_plane, float far_plane);
float const* OpenVRGetPickingMatrix(PyMOLGlobals * G);
float const *OpenVRGetMolecule2WorldMatrix(PyMOLGlobals * G, float *scaler);

void OpenVRLoadProjectionMatrix(PyMOLGlobals * G, float near_plane, float far_plane);
void OpenVRLoadWorld2EyeMatrix(PyMOLGlobals * G);

bool OpenVRIsMoleculeCaptured(PyMOLGlobals * G);

void OpenVRHandleInput(PyMOLGlobals * G, int SceneX, int SceneY, int SceneWidth, int SceneHeight, float *model2World);

void OpenVRDraw(PyMOLGlobals * G);

#endif /* _H_OpenVRStereo */
