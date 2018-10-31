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
#ifndef _H_OpenVRLaserTarget
#define _H_OpenVRLaserTarget

class OpenVRLaserTarget {
public:
  virtual bool LaserShoot(float const* origin, float const* dir, float const* color, float* distance = 0) = 0;
  virtual void LaserClick(int glutButton, int glutState) = 0;
  virtual bool IsLaserAllowed(unsigned deviceIndex) const = 0;
};

#endif // _H_OpenVRLaserTarget
