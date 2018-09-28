#include "OpenVRScenePicker.h"

void OpenVRScenePicker::Activate(unsigned ownerID)
{
  m_ownerID = ownerID;
  m_active = true;
}

void OpenVRScenePicker::Deactivate()
{
  m_ownerID = ~0u;
  m_active = false;
}

bool OpenVRScenePicker::IsActive() const
{
  return m_active;
}

bool OpenVRScenePicker::LaserShoot(float const* origin, float const* dir, float const* color, float* distance /* = 0 */)
{
  return false;
}

void OpenVRScenePicker::LaserClick(bool down)
{
}

bool OpenVRScenePicker::IsLaserAllowed(unsigned deviceIndex) const
{
  return m_ownerID == deviceIndex || m_ownerID == ~0u;
}

float const (*OpenVRScenePicker::GetLaserColors())[4]
{
  static const float colors[2][4] = {
    {1.0f, 1.0f, 0.0f, 0.5f},
    {0.0f, 1.0f, 0.0f, 1.0f},
  };
  return colors;
}
