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
