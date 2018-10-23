// this header
#include "OpenVRController.h"

// other headers
#include "os_gl.h"

OpenVRController::OpenVRController()
  : m_init(false)
  , m_deviceIndex(~0U)
  , m_bShowController(true)
{
}

void OpenVRController::Init()
{
  m_laser.Init();
  m_init = true;
}

void OpenVRController::Free()
{
  m_laser.Free();
}

bool OpenVRController::IsInitialized()
{
  return m_init;
}

void OpenVRController::Show(bool isVisible)
{
  m_bShowController = isVisible;
}

bool OpenVRController::IsVisible() const
{
  return m_bShowController;
}

void OpenVRController::LaserShow(bool isVisible)
{
  if (isVisible)
    m_laser.Show();
  else
    m_laser.Hide();
}

bool OpenVRController::IsLaserVisible() const
{
  return m_laser.IsVisible();
}

void OpenVRController::Draw()
{
  GL_DEBUG_FUN();

  glPushMatrix();
  glMultMatrixf(m_pose);

  if (m_pRenderModel) {
    m_pRenderModel->Draw();
  }

  m_laser.Draw();

  glPopMatrix();
}

bool OpenVRController::GetLaserRay(float* origin, float* dir) const
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

unsigned OpenVRController::GetLaserDeviceIndex() const
{
  return m_deviceIndex;
}

void OpenVRController::SetLaserLength(float length)
{
  m_laser.SetLength(length);
}

void OpenVRController::SetLaserColor(float r, float g, float b, float a)
{
  m_laser.SetColor(r, g, b, a);
}

void OpenVRController::SetLaserColor(float const color[])
{
  m_laser.SetColor(color[0], color[1], color[2], color[3]);
}
