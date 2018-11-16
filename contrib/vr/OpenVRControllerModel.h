#ifndef _H_OpenVRControllerModel
#define _H_OpenVRControllerModel

// system headers
#include "openvr.h"

// pymol headers
#include "PyMOLGlobals.h"
#include "ShaderMgr.h"

// local headers
#include "OpenVRQuad.h"

class OpenVRControllerModel
{
public:
  OpenVRControllerModel(const std::string & sRenderModelName);
  ~OpenVRControllerModel();

  bool Init(PyMOLGlobals * G, const vr::RenderModel_t & vrModel, const vr::RenderModel_TextureMap_t & vrDiffuseTexture);
  void Free();

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
};

void ShutdownRenderModels();
OpenVRControllerModel *FindOrLoadRenderModel(PyMOLGlobals *G, const char *pchRenderModelName);

#endif /* _H_OpenVRController */
