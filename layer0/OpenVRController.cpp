#include <vector>

#include "os_std.h"
#include "os_gl.h"

#include "Matrix.h"

#include "OpenVRController.h"

static std::vector<CGLRenderModel *> s_vecRenderModels;


void OpenVRController::InitAxes(PyMOLGlobals * G) {
 
  std::vector<float> vertdataarray;
  m_uiControllerVertcount = 0;

  float center[4] = {0.0, 0.0, 0.0, 1.0};

  for ( int i = 0; i < 3; ++i )
  {
      float color[3] = {0, 0, 0};
      float point[4] = {0, 0, 0, 1.f};
      point[i] += 0.05f;  // offset in X, Y, Z
      color[i] = 1.0;  // R, G, B

      vertdataarray.push_back( center[0] );
      vertdataarray.push_back( center[1] );
      vertdataarray.push_back( center[2] );
      vertdataarray.push_back( center[3] );

      vertdataarray.push_back( color[0] );
      vertdataarray.push_back( color[1] );
      vertdataarray.push_back( color[2] );

      vertdataarray.push_back( point[0] );
      vertdataarray.push_back( point[1] );
      vertdataarray.push_back( point[2] );
      vertdataarray.push_back( point[3] );

      vertdataarray.push_back( color[0] );
      vertdataarray.push_back( color[1] );
      vertdataarray.push_back( color[2] );

      m_uiControllerVertcount += 2;
  }

  float start[4] = {0.f, 0.f, -0.02f, 1.f};
  float end[4] = {0.f, 0.f, -39.f, 1.f};
  float color[3] = {.92f, .92f, .71f};

  vertdataarray.push_back( start[0] ); vertdataarray.push_back( start[1] );vertdataarray.push_back( start[2] ); vertdataarray.push_back( start[3] );
  vertdataarray.push_back( color[0] ); vertdataarray.push_back( color[1] );vertdataarray.push_back( color[2] );

  vertdataarray.push_back( end[0] ); vertdataarray.push_back( end[1] );vertdataarray.push_back( end[2] ); vertdataarray.push_back( end[3] );
  vertdataarray.push_back( color[0] );vertdataarray.push_back( color[1] );vertdataarray.push_back( color[2] );
  m_uiControllerVertcount += 2;

  // Setup the VAO the first time through.
  if ( m_unControllerVAO == 0 )
  {
      glGenVertexArrays( 1, &m_unControllerVAO );
      glBindVertexArray( m_unControllerVAO );

      glGenBuffers( 1, &m_glControllerVertBuffer );
      glBindBuffer( GL_ARRAY_BUFFER, m_glControllerVertBuffer );

      GLuint stride = (4 + 3) * sizeof( float );
      uintptr_t offset = 0;

      glEnableVertexAttribArray( 0 );
      glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, stride, (const void *)offset);

      offset += 4 * sizeof(float);
      glEnableVertexAttribArray(1);
      glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (const void *)offset);

      glBindVertexArray(0);
  }

  glBindBuffer( GL_ARRAY_BUFFER, m_glControllerVertBuffer );

  // set vertex data if we have some
  if( vertdataarray.size() > 0 )
  {
      //$ TODO: Use glBufferSubData for this...
      glBufferData( GL_ARRAY_BUFFER, sizeof(float) * vertdataarray.size(), &vertdataarray[0], GL_STREAM_DRAW );
  }

  glBindBuffer( GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
}

void OpenVRController::InitAxesShader(PyMOLGlobals * G) {
  // vertex shader
  const char *vs = 
    "attribute vec4 position;\n"
    "attribute vec3 color_in;\n"
    "varying vec3 color;\n"
    "void main() {\n"
      "color = color_in;\n"
      "gl_Position = gl_ModelViewProjectionMatrix * position;\n"
    "}\n";

  // fragment shader
  const char *ps = 
    "varying vec3 color;\n"
    "void main() {\n"
      "gl_FragColor = vec4(color, 1.0);\n"
    "}\n";
  
  m_pAxesShader = CShaderPrg_New(G, "Controller", vs, ps);
}

void OpenVRController::DestroyAxes() {
  if( m_unControllerVAO != 0 ) {
    glDeleteVertexArrays( 1, &m_unControllerVAO );
    m_unControllerVAO = 0;
  }
  if (m_glControllerVertBuffer != 0) {
    glDeleteBuffers(1, &m_glControllerVertBuffer);
    m_glControllerVertBuffer = 0;
    m_uiControllerVertcount = 0;
  }
  if ( m_unControllerTransformProgramID ) {
    glDeleteProgram( m_unControllerTransformProgramID );
    m_unControllerTransformProgramID = 0;
  }
}

void OpenVRController::Draw(PyMOLGlobals * G) {
  GL_DEBUG_FUN();
  
  glPushMatrix();
  glMultMatrixf(m_pose);

  CShaderPrg_Enable(m_pAxesShader);
  glBindVertexArray( m_unControllerVAO );

  glDrawArrays( GL_LINES, 0, m_uiControllerVertcount );

  glBindVertexArray( 0 );

  if (m_pRenderModel) {
    m_pRenderModel->Draw();
  }
  CShaderPrg_Disable(m_pAxesShader);  

  glPopMatrix();
}

//-----------------------------------------------------------------------------
// Purpose: Create/destroy GL Render Models
//-----------------------------------------------------------------------------
CGLRenderModel::CGLRenderModel(const std::string & sRenderModelName) : m_sModelName( sRenderModelName ) {
  m_glIndexBuffer = 0;
  m_glVertArray = 0;
  m_glVertBuffer = 0;
  m_glTexture = 0;
}

CGLRenderModel::~CGLRenderModel() {
  Cleanup();
}

//-----------------------------------------------------------------------------
// Purpose: Allocates and populates the GL resources for a render model
//-----------------------------------------------------------------------------
bool CGLRenderModel::BInit(PyMOLGlobals * G, const vr::RenderModel_t & vrModel, const vr::RenderModel_TextureMap_t & vrDiffuseTexture) {
  // create and bind a VAO to hold state for this model
  glGenVertexArrays( 1, &m_glVertArray );
  glBindVertexArray( m_glVertArray );

  // Populate a vertex buffer
  glGenBuffers( 1, &m_glVertBuffer );
  glBindBuffer( GL_ARRAY_BUFFER, m_glVertBuffer );
  glBufferData( GL_ARRAY_BUFFER, sizeof( vr::RenderModel_Vertex_t ) * vrModel.unVertexCount, vrModel.rVertexData, GL_STATIC_DRAW );

  // Identify the components in the vertex buffer
  glEnableVertexAttribArray( 0 );
  glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, sizeof( vr::RenderModel_Vertex_t ), (void *)offsetof( vr::RenderModel_Vertex_t, vPosition ) );
  glEnableVertexAttribArray( 1 );
  glVertexAttribPointer( 1, 3, GL_FLOAT, GL_FALSE, sizeof( vr::RenderModel_Vertex_t ), (void *)offsetof( vr::RenderModel_Vertex_t, vNormal ) );
  glEnableVertexAttribArray( 2 );
  glVertexAttribPointer( 2, 2, GL_FLOAT, GL_FALSE, sizeof( vr::RenderModel_Vertex_t ), (void *)offsetof( vr::RenderModel_Vertex_t, rfTextureCoord ) );

  // Create and populate the index buffer
  glGenBuffers( 1, &m_glIndexBuffer );
  glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, m_glIndexBuffer );
  glBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof( uint16_t ) * vrModel.unTriangleCount * 3, vrModel.rIndexData, GL_STATIC_DRAW );

  glBindVertexArray( 0 );

  // create and populate the texture
  glGenTextures(1, &m_glTexture );
  glBindTexture( GL_TEXTURE_2D, m_glTexture );

  glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, vrDiffuseTexture.unWidth, vrDiffuseTexture.unHeight,
    0, GL_RGBA, GL_UNSIGNED_BYTE, vrDiffuseTexture.rubTextureMapData );

  // If this renders black ask McJohn what's wrong.
  glGenerateMipmap(GL_TEXTURE_2D);

  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );

  GLfloat fLargest;
  glGetFloatv( GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &fLargest );
  glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, fLargest );

  glBindTexture( GL_TEXTURE_2D, 0 ); 

  m_unVertexCount = vrModel.unTriangleCount * 3;

  // vertex shader
  const char *vs = 
    "attribute vec3 position;\n"
    "attribute vec3 v3NormalIn;\n" // FIXME remove, we don't need it 
    "attribute vec2 v2TexCoordsIn;\n"
    "varying vec2 v2TexCoord;\n"
    "void main()\n"
    "{\n"
    "	v2TexCoord = v2TexCoordsIn;\n"
    "	gl_Position = gl_ModelViewProjectionMatrix * vec4(position, 1);\n"
    "}\n";

  // fragment shader
  const char *ps = 
    "uniform sampler2D diffuse;\n"
    "varying vec2 v2TexCoord;\n"
    "void main()\n"
    "{\n"
    "  gl_FragColor = texture2D( diffuse, v2TexCoord);\n"
    "}\n";    
    
  m_pShader = CShaderPrg_New(G, "CRenderModel", vs, ps);
  glBindAttribLocation(m_pShader->id, 0, "position");
  glBindAttribLocation(m_pShader->id, 1, "v3NormalIn");
  glBindAttribLocation(m_pShader->id, 2, "v2TexCoordsIn");
  glLinkProgram(m_pShader->id);

  return true;
}

//-----------------------------------------------------------------------------
// Purpose: Frees the GL resources for a render model
//-----------------------------------------------------------------------------
void CGLRenderModel::Cleanup() {
  if( m_glVertBuffer ) {
    glDeleteBuffers(1, &m_glIndexBuffer);
    glDeleteVertexArrays( 1, &m_glVertArray );
    glDeleteBuffers(1, &m_glVertBuffer);
    m_glIndexBuffer = 0;
    m_glVertArray = 0;
    m_glVertBuffer = 0;
  }
}

//-----------------------------------------------------------------------------
// Purpose: Draws the render model
//-----------------------------------------------------------------------------
void CGLRenderModel::Draw() {
  GL_DEBUG_FUN();
  CShaderPrg_Enable(m_pShader);
  glBindVertexArray( m_glVertArray );

  glActiveTexture( GL_TEXTURE0 );
  glBindTexture( GL_TEXTURE_2D, m_glTexture );

  glDrawElements( GL_TRIANGLES, m_unVertexCount, GL_UNSIGNED_SHORT, 0 );

  glBindVertexArray( 0 );
  CShaderPrg_Disable(m_pShader);  
}

// Purpose:
//-----------------------------------------------------------------------------
void ThreadSleep( unsigned long nMilliseconds )
{
#if defined(_WIN32)
  ::Sleep( nMilliseconds );
#elif defined(POSIX)
  usleep( nMilliseconds * 1000 );
#endif
}

// Purpose: Finds a render model we've already loaded or loads a new one
//-----------------------------------------------------------------------------
CGLRenderModel *FindOrLoadRenderModel(PyMOLGlobals * G, const char *pchRenderModelName)
{
  CGLRenderModel *pRenderModel = NULL;
  for(std::vector< CGLRenderModel * >::iterator i = s_vecRenderModels.begin(); i != s_vecRenderModels.end(); i++) {
    if( !stricmp((*i)->GetName().c_str(), pchRenderModelName )){
      pRenderModel = *i;
      break;
    }
  }

  // load the model if we didn't find one
  if(!pRenderModel) {
    vr::RenderModel_t *pModel;
    vr::EVRRenderModelError error;
    while (true) {
      error = vr::VRRenderModels()->LoadRenderModel_Async( pchRenderModelName, &pModel );
      if ( error != vr::VRRenderModelError_Loading )
        break;

      ThreadSleep( 1 );
    }

    if (error != vr::VRRenderModelError_None) {
      //FXME dprintf( "Unable to load render model %s - %s\n", pchRenderModelName, vr::VRRenderModels()->GetRenderModelErrorNameFromEnum( error ) );
      return NULL; // move on to the next tracked device
    }

    vr::RenderModel_TextureMap_t *pTexture;
    while (true) {
      error = vr::VRRenderModels()->LoadTexture_Async( pModel->diffuseTextureId, &pTexture );
      if ( error != vr::VRRenderModelError_Loading )
        break;
      ThreadSleep( 1 );
    }

    if (error != vr::VRRenderModelError_None) {
      //FXME dprintf( "Unable to load render texture id:%d for render model %s\n", pModel->diffuseTextureId, pchRenderModelName );
      vr::VRRenderModels()->FreeRenderModel( pModel );
      return NULL; // move on to the next tracked device
    }

    pRenderModel = new CGLRenderModel( pchRenderModelName );
    if (!pRenderModel->BInit(G, *pModel, *pTexture)) {
      //FXME dprintf( "Unable to create GL model from render model %s\n", pchRenderModelName );
      delete pRenderModel;
      pRenderModel = NULL;
    } else {
      s_vecRenderModels.push_back(pRenderModel);
    }
    vr::VRRenderModels()->FreeRenderModel(pModel);
    vr::VRRenderModels()->FreeTexture(pTexture);
  }
  return pRenderModel;
}

void ShutdownRenderModels() {
  for(std::vector<CGLRenderModel *>::iterator i = s_vecRenderModels.begin(); i != s_vecRenderModels.end(); i++) {
    delete (*i);
  }
  s_vecRenderModels.clear();
}