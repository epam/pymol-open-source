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

#include "PyMOLGlobals.h"
#include "ShaderMgr.h"
#include "OpenVRControllerModel.h"

class OpenVRController {
public:
  OpenVRController():  m_uiControllerVertcount(0), m_glControllerVertBuffer(0),
    m_unControllerVAO(0), m_unControllerTransformProgramID(0), m_pAxesShader(NULL),
    m_deviceIndex(~0U), m_bShowController(true), m_bShowLaser(false)
    {}
 
  bool IsInitialized() {return m_uiControllerVertcount && m_glControllerVertBuffer && m_unControllerVAO &&
    m_pAxesShader;}
 
  void Init(PyMOLGlobals * G) { InitAxes(G); InitAxesShader(G); }
  void Free() { DestroyAxes(); }
  void Draw(PyMOLGlobals * G);
  float *GetPose() {return m_pose;} // it's not safe =)

  void Show(bool isVisible) {m_bShowController = isVisible;}
  bool IsVisible() const { return m_bShowController; }

  void ShowLaser(bool isVisible) {m_bShowLaser = isVisible;}
  bool IsLaserVisible() const { return m_bShowLaser; }
  bool GetLaser(float* origin, float* dir);

public:
// FIXME make good initialization
  vr::TrackedDeviceIndex_t m_deviceIndex;
  OpenVRControllerModel *m_pRenderModel;
  std::string m_sRenderModelName;

private:
  unsigned int m_uiControllerVertcount;
  GLuint m_glControllerVertBuffer;
  GLuint m_unControllerVAO;
  GLuint m_unControllerTransformProgramID;
  CShaderPrg *m_pAxesShader;
  GLfloat m_pose[16]; // model2world matrix 
  bool m_bShowController;
  bool m_bShowLaser;
  
  void InitAxes(PyMOLGlobals * G);
  void InitAxesShader(PyMOLGlobals * G);
  void DestroyAxes(); 	
};

#endif /* _H_OpenVRController */
