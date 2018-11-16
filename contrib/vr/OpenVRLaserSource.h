#ifndef _H_OpenVRLaserSource
#define _H_OpenVRLaserSource

class OpenVRLaserSource {
public:
  virtual void LaserShow(bool isVisible) = 0;
  virtual bool IsLaserVisible() const = 0;

  virtual bool GetLaserRay(float* origin, float* dir) const = 0;
  virtual unsigned GetLaserDeviceIndex() const = 0;

  virtual void SetLaserLength(float length) = 0;
  virtual void SetLaserColor(float r, float g, float b, float a) = 0;
  virtual void SetLaserColor(float const color[]) = 0;
};

#endif // _H_OpenVRLaserSource
