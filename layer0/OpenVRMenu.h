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
#ifndef _H_OpenVRMenu
#define _H_OpenVRMenu

#define nullptr 0
#include "openvr.h"

#include "os_gl.h"
#include "PyMOLGlobals.h"
#include "ShaderMgr.h"

class OpenVRMenu {
public:
  OpenVRMenu() {}

  void Init();
  void Free();

  void Start(unsigned width, unsigned height);
  void Finish();

private:
  void InitBuffers(unsigned width, unsigned height);
  void FreeBuffers();

private:
  unsigned m_width;
  unsigned m_height;

  GLuint m_frameBufferID;
  GLuint m_textureID;
};

#endif /* _H_OpenVRMenu */
