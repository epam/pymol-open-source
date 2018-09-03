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

enum OpenVRAction_t {
  OPENVR_ACTION_SCENE_NEXT,
  OPENVR_ACTION_SCENE_PREV,

  OPENVR_ACTION_MOVIE_TOGGLE,
  OPENVR_ACTION_MOVIE_NEXT,
  OPENVR_ACTION_MOVIE_PREV,
};

typedef void OpenVRKeyboardFunc_t(PyMOLGlobals * G, unsigned char k, int x, int y, int mod);
typedef void OpenVRSpecialFunc_t(PyMOLGlobals * G, int k, int x, int y, int mod);
typedef int OpenVRMouseFunc_t(PyMOLGlobals * G, int button, int state, int x, int y, int mod);
typedef int OpenVRMotionFunc_t(PyMOLGlobals * G, int x, int y, int mod);
typedef void OpenVRActionFunc_t(PyMOLGlobals * G, OpenVRAction_t a);

void OpenVRSetKeyboardFunc(PyMOLGlobals * G, OpenVRKeyboardFunc_t* handler);
void OpenVRSetSpecialFunc(PyMOLGlobals * G, OpenVRSpecialFunc_t* handler);
void OpenVRSetMouseFunc(PyMOLGlobals * G, OpenVRMouseFunc_t* handler);
void OpenVRSetMotionFunc(PyMOLGlobals * G, OpenVRMotionFunc_t* handler);
void OpenVRSetActionFunc(PyMOLGlobals * G, OpenVRActionFunc_t* handler);

void OpenVRFeedback(PyMOLGlobals * G);

void OpenVRFrameStart(PyMOLGlobals * G);
void OpenVREyeStart(PyMOLGlobals * G, int eye);
void OpenVREyeFinish(PyMOLGlobals * G);
void OpenVRFrameFinish(PyMOLGlobals * G, unsigned width, unsigned height);

void OpenVRMenuBufferStart(PyMOLGlobals * G, unsigned width, unsigned height, bool clear = false);
void OpenVRMenuBufferFinish(PyMOLGlobals * G);
void OpenVRMenuToggle(PyMOLGlobals * G);

float* OpenVRGetWorldToHead(PyMOLGlobals * G);
float* OpenVRGetHeadToEye(PyMOLGlobals * G);
float* OpenVRGetControllerPose(PyMOLGlobals * G);
float* OpenVRGetProjection(PyMOLGlobals * G, float near_plane, float far_plane);

void OpenVRLoadProjectionMatrix(PyMOLGlobals * G, float near_plane, float far_plane);
void OpenVRLoadWorld2EyeMatrix(PyMOLGlobals * G);

void OpenVRHandleInput(PyMOLGlobals * G);

void OpenVRDraw(PyMOLGlobals * G);

#endif /* _H_OpenVRStereo */
