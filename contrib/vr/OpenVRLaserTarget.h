#ifndef _H_OpenVRLaserTarget
#define _H_OpenVRLaserTarget

class OpenVRLaserTarget {
public:
  virtual bool LaserShoot(float const* origin, float const* dir, float const* color, float* distance = 0) = 0;
  virtual void LaserClick(int glutButton, int glutState) = 0;
  virtual bool IsLaserAllowed(unsigned deviceIndex) const = 0;
  virtual float const* GetLaserColor() const = 0;
};

#endif // _H_OpenVRLaserTarget
