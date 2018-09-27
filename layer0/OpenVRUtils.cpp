#include "OpenVRUtils.h"
#include <stdio.h>

namespace OpenVRUtils {

static GLuint CompileShader(GLenum shaderType, char const* shader)
{
  GLuint shaderID = glCreateShader(shaderType);
  glShaderSource(shaderID, 1, &shader, NULL);
  glCompileShader(shaderID);

  GLint success = GL_FALSE;
  glGetShaderiv(shaderID, GL_COMPILE_STATUS, &success);
  if (!success) {
    int infoLogLength = 0;
    glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &infoLogLength);
    if (infoLogLength > 0) {
      char *infoLog = new char[infoLogLength];
      glGetShaderInfoLog(shaderID, infoLogLength, 0, infoLog);
      printf("GLSL shader compilation failed, log follows:\n"
        "=============================================================================\n"
        "%s"
        "=============================================================================\n", infoLog);
      delete[] infoLog;
    }
    glDeleteShader(shaderID);
    shaderID = 0;
  }
  return shaderID;
}

GLuint CompileProgram(char const* vertexShader,  char const* fragmentShader)
{
  GLuint vertexShaderID = CompileShader(GL_VERTEX_SHADER, vertexShader);
  GLuint fragmentShaderID = CompileShader(GL_FRAGMENT_SHADER, fragmentShader);

  GLuint programID = 0;
  if (vertexShaderID && fragmentShaderID) {
    programID = glCreateProgram();
    glAttachShader(programID, vertexShaderID);
    glAttachShader(programID, fragmentShaderID);
    glLinkProgram(programID);

    GLint success = GL_FALSE;
    glGetProgramiv(programID, GL_LINK_STATUS, &success);
    if (!success) {
      int infoLogLength = 0;
      glGetProgramiv(programID, GL_INFO_LOG_LENGTH, &infoLogLength);
      if (infoLogLength > 0) {
        char *infoLog = new char[infoLogLength];
        glGetProgramInfoLog(programID, infoLogLength, 0, infoLog);
        printf("GLSL program linking failed, log follows:\n"
          "=============================================================================\n"
          "%s"
          "=============================================================================\n", infoLog);
        delete[] infoLog;
      }

      glDeleteProgram(programID);
      programID = 0;
    }
  }

  if (vertexShaderID) {
    glDeleteShader(vertexShaderID);
  }

  if (fragmentShaderID) {
    glDeleteShader(fragmentShaderID);
  }

  if (programID) {
    glUseProgram(programID);
    glUseProgram(0);
  }

  return programID;
}

} // namespace OpenVRUtils
