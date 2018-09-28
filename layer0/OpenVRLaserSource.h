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
#ifndef _H_OpenVRLaserSource
#define _H_OpenVRLaserSource

class OpenVRLaserSource {
public:
  virtual void LaserShow(bool isVisible) = 0;
  virtual bool IsLaserVisible() const = 0;

  virtual bool GetLaserRay(float* origin, float* dir) const = 0;
  virtual unsigned GetLaserDeviceIndex() const = 0;

  virtual void SetLaserLength(float length) = 0;
  virtual void SetLaserColor(float r, float g, float b, float a) = 0;
  virtual void SetLaserColor(float const color[]) = 0;
};

#endif // _H_OpenVRLaserSource
