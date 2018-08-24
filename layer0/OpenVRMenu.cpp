#include "OpenVRMenu.h"

void OpenVRMenu::Init()
{
}

void OpenVRMenu::Free()
{
  FreeBuffers();
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

void OpenVRMenu::Start(unsigned width, unsigned height)
{
  if (m_width != width || m_height != height)
    InitBuffers(width, height);
  glBindFramebufferEXT(GL_FRAMEBUFFER, m_frameBufferID);
}

void OpenVRMenu::Finish()
{
  glBindFramebufferEXT(GL_FRAMEBUFFER, 0);
}
