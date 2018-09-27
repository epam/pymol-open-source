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
#ifndef _H_OpenVRController
#define _H_OpenVRController

#include "openvr.h"

#include "OpenVRControllerModel.h"
#include "OpenVRLaser.h"

class OpenVRController {
public:
  OpenVRController();
 
  void Init();
  void Free();
  bool IsInitialized();

  void Draw();

  float *GetPose() {return m_pose;} // it's not safe =)

  void Show(bool isVisible);
  bool IsVisible() const;

  void ShowLaser(bool isVisible);
  bool IsLaserVisible() const;
  bool GetLaser(float* origin, float* dir, float* color = 0);
  void SetLaserLength(float length);
  void SetLaserColor(float r, float g, float b, float a);

public:
// FIXME make good initialization
  vr::TrackedDeviceIndex_t m_deviceIndex;
  OpenVRControllerModel *m_pRenderModel;
  std::string m_sRenderModelName;

private:
  bool m_init;
  bool m_bShowController;
  GLfloat m_pose[16]; // model2world matrix 
  OpenVRLaser m_laser;
};

#endif /* _H_OpenVRController */
