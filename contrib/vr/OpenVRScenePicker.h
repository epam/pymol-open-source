#ifndef _H_OpenVRScenePicker
#define _H_OpenVRScenePicker

// local headers
#include "OpenVRLaserTarget.h"
#include "OpenVRMode.h"

class OpenVRScenePicker : public OpenVRLaserTarget {
  OpenVRInputHandlers* m_inputHandlers;
  unsigned m_ownerID;
  int m_clickX;
  int m_clickY;
  bool m_active;
  float m_matrix[16];

public:
  OpenVRScenePicker();

  void Init(OpenVRInputHandlers* inputHandlers);
  void Free();

  void Activate(unsigned ownerID, int x, int y);
  void Deactivate();
  bool IsActive() const;

  bool LaserShoot(float const* origin, float const* dir, float const* color, float* distance = 0);
  void LaserClick(int glutButton, int glutState);
  bool IsLaserAllowed(unsigned deviceIndex) const;
  float const* GetLaserColor() const;

  float const* GetMatrix() const;
};

#endif // _H_OpenVRScenePicker
