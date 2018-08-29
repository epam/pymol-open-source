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

#include "openvr.h"

#include "os_gl.h"
#include "PyMOLGlobals.h"
#include "ShaderMgr.h"

class OpenVRMenu {
public:
  OpenVRMenu();

  void Init();
  void Free();

  void Start(unsigned width, unsigned height, bool clear);
  void Finish();

  void Show(GLfloat const* headMatrix);
  void Hide();
  bool IsVisible() const;

  void Draw();

private:
  void InitGeometry();
  void FreeGeometry();

  bool InitShaders();
  void FreeShaders();

  void InitBuffers(unsigned width, unsigned height);
  void FreeBuffers();

private:
  unsigned m_width;
  unsigned m_height;
  float m_sceneColor;
  float m_sceneAlpha;
  float m_distance;
  float m_fovTangent;
  bool m_valid;
  bool m_visible;

  GLfloat m_matrix[16];

  // offscreen framebuffer
  GLuint m_frameBufferID;
  GLuint m_textureID;

  // geometry
  GLuint m_vertexArrayID;
  GLuint m_vertexBufferID;
  GLuint m_vertexCount;

  // shader
  GLuint m_programID;
};

#endif /* _H_OpenVRMenu */
