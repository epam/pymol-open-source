#include "OpenVRMenu.h"

OpenVRMenu::OpenVRMenu()
  : m_width(640)
  , m_height(480)
  , m_sceneColor(0.2f)
  , m_sceneAlpha(0.75f)
  , m_distance(2.0f)
  , m_fovTangent(1.0f)
  , m_valid(false)
  , m_visible(false)
  , m_frameBufferID(0)
  , m_textureID(0)
  , m_vertexArrayID(0)
  , m_vertexBufferID(0)
  , m_vertexCount(0)
  , m_programID(0)
{
  memset(m_matrix, 0, sizeof(m_matrix));
  m_matrix[0] = m_matrix[5] = m_matrix[10] = m_matrix[15] = 1.0f;
}

void OpenVRMenu::Init()
{
  InitGeometry();
  m_valid = InitShaders();
}

void OpenVRMenu::Free()
{
  FreeBuffers();
  FreeShaders();
  FreeGeometry();
}

void OpenVRMenu::InitGeometry()
{
  static struct Vertex_t {
    Vector3f position;
    Vector3f texcoord;
  } vertices[] = {
    {{-1.0f, +1.0f}, {0.0f, 1.0f}},
    {{+1.0f, +1.0f}, {1.0f, 1.0f}},
    {{-1.0f, -1.0f}, {0.0f, 0.0f}},
    {{+1.0f, -1.0f}, {1.0f, 0.0f}},
  };

  m_vertexCount = sizeof(vertices) / sizeof(vertices[0]);

  // Create and bind a VAO to hold state for this model
  glGenVertexArrays(1, &m_vertexArrayID);
  glBindVertexArray(m_vertexArrayID);

  // Populate a vertex buffer
  glGenBuffers(1, &m_vertexBufferID);
  glBindBuffer(GL_ARRAY_BUFFER, m_vertexBufferID);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  // Identify the components in the vertex buffer
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex_t), (void *)offsetof(Vertex_t, position));
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex_t), (void *)offsetof(Vertex_t, texcoord));

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
}

void OpenVRMenu::FreeGeometry()
{
  if (m_vertexBufferID) {
    glDeleteVertexArrays(1, &m_vertexArrayID);
    glDeleteBuffers(1, &m_vertexBufferID);
    m_vertexArrayID = 0;
    m_vertexBufferID = 0;
  }
}

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

static GLuint CompileProgram(char const* vertexShader,  char const* fragmentShader)
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


bool OpenVRMenu::InitShaders()
{
  const char* vs =
    "attribute vec2 position;\n"
    "attribute vec2 texcoords_in;\n\n"
    "varying vec2 texcoords;\n\n"
    "void main() {\n"
      "texcoords = texcoords_in;\n"
      "gl_Position = gl_ModelViewProjectionMatrix * vec4(position, 0.0, 1.0);\n"
    "}\n";
  const char* fs =
    "uniform sampler2D texture;\n"
    "varying vec2 texcoords;\n"
    "void main() {\n"
      "gl_FragColor = texture2D(texture, texcoords);\n"
    "}\n";

  m_programID = CompileProgram(vs, fs);
  return m_programID;
}

void OpenVRMenu::FreeShaders()
{
  glDeleteProgram(m_programID);
}

void OpenVRMenu::InitBuffers(unsigned width, unsigned height)
{
  if (m_frameBufferID)
    FreeBuffers();

  m_width = width;
  m_height = height;

  // framebuffer
  glGenFramebuffersEXT(1, &m_frameBufferID);
  glBindFramebufferEXT(GL_FRAMEBUFFER, m_frameBufferID);

  // - color
  glGenTextures(1, &m_textureID);
  glBindTexture(GL_TEXTURE_2D, m_textureID);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  glFramebufferTexture2DEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_textureID, 0);

  glBindTexture(GL_TEXTURE_2D, 0);
  glBindFramebufferEXT(GL_FRAMEBUFFER, 0);
}

void OpenVRMenu::FreeBuffers()
{
  glDeleteTextures(1, &m_textureID);
  glDeleteFramebuffers(1, &m_frameBufferID);
}

void OpenVRMenu::Start(unsigned width, unsigned height, bool clear)
{
  if (m_width != width || m_height != height)
    InitBuffers(width, height);

  glBindFramebufferEXT(GL_FRAMEBUFFER, m_frameBufferID);

  if (clear) {
    glClearColor(m_sceneColor, m_sceneColor, m_sceneColor, m_sceneAlpha);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  }
}

void OpenVRMenu::Finish()
{
  glBindFramebufferEXT(GL_FRAMEBUFFER, 0);
}

void OpenVRMenu::Show(GLfloat const* headMatrix)
{
  memcpy(m_matrix, headMatrix, sizeof(m_matrix));
  m_visible = true;
}

void OpenVRMenu::Hide()
{
  m_visible = false;
}

bool OpenVRMenu::IsVisible() const
{
  return m_visible;
}

void OpenVRMenu::Draw()
{
  if (!m_valid || !m_visible)
    return;

  // calculate extents of the panel
  float worldWidth = m_distance * m_fovTangent;
  float worldHeight = worldWidth * m_height / m_width;

  glPushMatrix();
  glMultMatrixf(m_matrix);
  glTranslatef(0.0f, 0.0f, -m_distance);
  glScalef(worldWidth, worldHeight, 1.0f);

  glDisable(GL_DEPTH_TEST);
  glUseProgram(m_programID);
  glBindVertexArray(m_vertexArrayID);
  glBindTexture(GL_TEXTURE_2D, m_textureID);

  glDrawArrays(GL_TRIANGLE_STRIP, 0, m_vertexCount);

  glBindTexture(GL_TEXTURE_2D, 0);
  glBindVertexArray(0);
  glUseProgram(0);
  glEnable(GL_DEPTH_TEST);

  glPopMatrix();
}