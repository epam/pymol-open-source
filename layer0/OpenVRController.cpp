#include <vector>

#include "os_std.h"
#include "os_gl.h"

#include "Matrix.h"

#include "OpenVRController.h"

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
  if (m_pAxesShader) {
    CShaderPrg_Delete(m_pAxesShader);
    m_pAxesShader = NULL;
  }
}

void OpenVRController::Draw(PyMOLGlobals * G) {
  GL_DEBUG_FUN();
  
  glPushMatrix();
  glMultMatrixf(m_pose);

  CShaderPrg_Enable(m_pAxesShader);

  if (m_bShowLaser) {
    glBindVertexArray( m_unControllerVAO );
    glDrawArrays( GL_LINES, 0, m_uiControllerVertcount );
    glBindVertexArray( 0 );
  }

  if (m_pRenderModel) {
    m_pRenderModel->Draw();
  }

  CShaderPrg_Disable(m_pAxesShader);  

  glPopMatrix();
}

bool OpenVRController::GetLaser(float* origin, float* dir)
{
  if (IsLaserVisible()) {
    origin[0] = m_pose[12];
    origin[1] = m_pose[13];
    origin[2] = m_pose[14];
    dir[0] = -m_pose[8];
    dir[1] = -m_pose[9];
    dir[2] = -m_pose[10];
    return true;
  }
  return false;
}
