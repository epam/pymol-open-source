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

class OpenVRScenePicker : public OpenVRLaserTarget {
  unsigned m_ownerID;
  bool m_active;

public:
  OpenVRScenePicker() : m_ownerID(~0u), m_active(false) {}

  void Activate(unsigned ownerID);
  void Deactivate();
  bool IsActive() const;

  bool LaserShoot(float const* origin, float const* dir, float const* color, float* distance = 0);
  void LaserClick(bool down);
  bool IsLaserAllowed(unsigned deviceIndex) const;

  static float const (*GetLaserColors())[4];
};

#endif // _H_OpenVRScenePicker
