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
#ifndef _H_OpenVRScenePicker
#define _H_OpenVRScenePicker

#include "OpenVRLaserTarget.h"
#include "OpenVRStereo.h"

class OpenVRScenePicker : public OpenVRLaserTarget {
  OpenVRInputHandlers* m_inputHandlers;
  unsigned m_ownerID;
  int m_clickX;
  int m_clickY;
  bool m_active;
  float m_matrix[16];

public:
  OpenVRScenePicker();

  void Init(OpenVRInputHandlers* inputHandlers);
  void Free();

  void Activate(unsigned ownerID, int x, int y);
  void Deactivate();
  bool IsActive() const;

  bool LaserShoot(float const* origin, float const* dir, float const* color, float* distance = 0);
  void LaserClick(bool down);
  bool IsLaserAllowed(unsigned deviceIndex) const;

  float const* GetMatrix() const;

  static float const (*GetLaserColors())[4];
};

#endif // _H_OpenVRScenePicker
