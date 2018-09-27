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
#include "OpenVRStereo.h"

class OpenVRMenu {
  struct Hotspot_t {
    int x, y, radius;
    float color[4];

    Hotspot_t() : x(0), y(0), radius(3) {
      color[0] = 1.0f;
      color[1] = 1.0f;
      color[2] = 1.0f;
      color[3] = 0.25f;
    }
  };

public:
  OpenVRMenu();

  void Init(OpenVRInputHandlers* inputHandlers);
  void Free();

  void Start(unsigned width, unsigned height, bool clear);
  void Finish();

  void Show(GLfloat const* headMatrix, unsigned ownerID);
  void Hide();
  bool IsVisible() const;
  unsigned GetOwnerID() const;

  void ShowtHotspot(int x, int y, float const* color = 0);
  void HideHotspot();

  void Draw(GLuint sceneTextureID = 0);

  bool LaserShoot(float const* origin, float const* dir, float const* color, float* distance = 0);
  void LaserClick(bool down);

private:
  void InitGeometry();
  void FreeGeometry();

  bool InitShaders();
  void FreeShaders();

  void InitBuffers(unsigned width, unsigned height);
  void FreeBuffers();

  bool IntersectRay(GLfloat const* origin, GLfloat const* dir, int* x, int* y, float* distance = 0);

private:
  OpenVRInputHandlers* m_inputHandlers;
  unsigned m_width;
  unsigned m_height;
  float m_sceneColor;
  float m_sceneAlpha;
  float m_distance;
  float m_fovTangent;
  bool m_valid;
  bool m_visible;
  unsigned m_ownerID;

  GLfloat m_matrix[16];
  GLfloat m_worldHalfWidth;
  GLfloat m_worldHalfHeight;

  Hotspot_t m_hotspot;

  // offscreen framebuffer
  GLuint m_frameBufferID;
  GLuint m_guiTextureID;

  // geometry
  GLuint m_vertexArrayID;
  GLuint m_vertexBufferID;
  GLuint m_vertexCount;

  // shader
  GLuint m_programID;
  GLint m_hotspotUniform;
  GLint m_hotspotColorUniform;
  GLint m_guiTextureUniform;
  GLint m_sceneTextureUniform;
};

inline bool OpenVRMenu::IsVisible() const {
  return m_visible;
}

inline unsigned OpenVRMenu::GetOwnerID() const {
  return m_ownerID;
}

#endif /* _H_OpenVRMenu */
