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
#ifndef _H_OpenVRControllerModel
#define _H_OpenVRControllerModel

#include "openvr.h"

#include "OpenVRQuad.h"

#include "PyMOLGlobals.h"
#include "ShaderMgr.h"

class OpenVRControllerModel
{
public:
  OpenVRControllerModel(const std::string & sRenderModelName);
  ~OpenVRControllerModel();

  bool Init(PyMOLGlobals * G, const vr::RenderModel_t & vrModel, const vr::RenderModel_TextureMap_t & vrDiffuseTexture);
  void Free();

  void SetHintsTexture(GLuint hintsTexture, unsigned spriteCount);

  void Draw();

  const std::string & GetName() const { return m_sModelName; }

private:
  void InitGeometry(const vr::RenderModel_t &vrModel);
  void FreeGeometry();

  void InitTexture(const vr::RenderModel_TextureMap_t &vrDiffuseTexture);
  void FreeTexture();

  bool InitShaders(PyMOLGlobals * G);
  void FreeShaders();

private:
  GLuint m_glVertBuffer;
  GLuint m_glIndexBuffer;
  GLuint m_glVertArray;
  GLuint m_glTexture;
  GLsizei m_unVertexCount;
  std::string m_sModelName;
  CShaderPrg *m_pShader;
  OpenVRQuad* m_hintsQuad;
};

void ShutdownRenderModels();
OpenVRControllerModel *FindOrLoadRenderModel(PyMOLGlobals *G, const char *pchRenderModelName);

#endif /* _H_OpenVRController */
