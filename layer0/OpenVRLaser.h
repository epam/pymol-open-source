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
#ifndef _H_OpenVRLaser
#define _H_OpenVRLaser

#include "os_gl.h"

class OpenVRLaser {
public:
  static double MAX_LENGTH;

public:
  OpenVRLaser();

  void Init();
  void Free();

  void Show();
  void Hide();
  bool IsVisible() const;

  void SetColor(float r, float g, float b, float a = 1.0f);
  void GetColor(float* color) const;

  void SetLength(float length);

  void Draw();

private:
  void InitGeometry();
  void FreeGeometry();

  bool InitShaders();
  void FreeShaders();

private:
  bool m_valid;
  bool m_visible;
  float m_length;
  float m_color[4];

  // geometry
  GLuint m_vertexArrayID;
  GLuint m_vertexBufferID;
  GLuint m_vertexCount;

  // shader
  GLuint m_programID;
  GLint m_scaleUniform;
  GLint m_colorUniform;
};

inline void OpenVRLaser::Show()
{
  m_visible = true;
}

inline void OpenVRLaser::Hide()
{
  m_visible = false;
}

inline bool OpenVRLaser::IsVisible() const {
  return m_visible;
}

#endif /* _H_OpenVRLaser */
