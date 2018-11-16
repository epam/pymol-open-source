#ifndef _H_OpenVRController
#define _H_OpenVRController

// system headers
#include "openvr.h"

// local headers
#include "OpenVRLaserSource.h"
#include "OpenVRControllerModel.h"
#include "OpenVRLaser.h"

class OpenVRController : public OpenVRLaserSource {
public:
  OpenVRController();
 
  void Init();
  void Free();
  bool IsInitialized();

  void SetHintsTexture(GLuint hintsTexture, unsigned spriteCount);
  void SetHintsIndex(unsigned index);

  void Draw();

  float *GetPose() {return m_pose;} // it's not safe =)
  float *GetWorldToControllerMatrix() {return m_worldToController;} // it's not safe =)

  void Show(bool isVisible);
  bool IsVisible() const;

  void LaserShow(bool isVisible);
  bool IsLaserVisible() const;
  bool GetLaserRay(float* origin, float* dir) const;
  unsigned GetLaserDeviceIndex() const;
  void SetLaserLength(float length);
  void SetLaserColor(float r, float g, float b, float a);
  void SetLaserColor(float const color[]);

  bool isGripPressed() const { return m_gripIsPressed; }
  void pressGrip(bool press) { m_gripIsPressed = press; }

public:
// FIXME make good initialization
  vr::TrackedDeviceIndex_t m_deviceIndex;
  OpenVRControllerModel *m_pRenderModel;
  std::string m_sRenderModelName;

private:
  bool m_init;
  bool m_bShowController;
  GLfloat m_pose[16]; // model2world matrix 
  OpenVRLaser m_laser;
  GLfloat m_worldToController[16];
  bool m_gripIsPressed;
  OpenVRQuad* m_hintsQuad;
};

#endif /* _H_OpenVRController */
