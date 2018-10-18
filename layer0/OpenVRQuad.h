#ifndef _H_OpenVRQuad
#define _H_OpenVRQuad

#include "os_gl.h"

class OpenVRQuad {

public:
  OpenVRQuad();
  ~OpenVRQuad();

  void SetTexture(GLuint textureID, unsigned spriteCount = 1);
  void SetSprite(unsigned index);
  void SetSize(float width, float height);
  void SetAlpha(float alpha);
  void SetMirror(bool mirror);

  void Draw();

private:
  void InitGeometry();
  void FreeGeometry();

  bool InitShaders();
  void FreeShaders();

private:
  float m_width;
  float m_height;
  unsigned m_spriteIndex;
  unsigned m_spriteCount;
  float m_alpha;
  bool m_mirror;

  // geometry
  GLuint m_vertexArrayID;
  GLuint m_vertexBufferID;
  GLuint m_vertexCount;

  // texture
  GLuint m_textureID;

  // shader
  GLuint m_programID;
  GLint m_positionMulAddUniform;
  GLint m_texcoordMulAddUniform;
  GLint m_colorMulAddUniform;
  GLint m_textureUniform;
};

#endif /* _H_OpenVRQuad */
