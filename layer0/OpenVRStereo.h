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

void OpenVRMenuBufferStart(PyMOLGlobals * G, unsigned width, unsigned height);
void OpenVRMenuBufferFinish(PyMOLGlobals * G);

float* OpenVRGetHDMPose(PyMOLGlobals * G);
float* OpenVRGetControllerPose(PyMOLGlobals * G);
float* OpenVRGetHeadToEye(PyMOLGlobals * G);
float* OpenVRGetProjection(PyMOLGlobals * G, float near_plane, float far_plane);

void OpenVRHandleInput(PyMOLGlobals * G);

void OpenVRDrawControllers(PyMOLGlobals * G, float Front, float Back);

#endif /* _H_OpenVRStereo */
