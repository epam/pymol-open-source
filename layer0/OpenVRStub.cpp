#include "OpenVRStub.h"
#include "OpenVRStubDevice.h"

static bool _stubEnabled = false;
static vr::IVRSystem* _stubSystem = nullptr;
static vr::IVRCompositor* _stubCompositor = nullptr;

namespace vr {
namespace stub {

void VR_StubEnable(bool on /* = true */)
{
  _stubEnabled = on;
}

bool VR_IsStubEnabled()
{
  return _stubEnabled;
}

IVRSystem* VRSystem()
{
  return _stubEnabled ? _stubSystem : vr::VRSystem();
}

IVRCompositor* VRCompositor()
{
  return _stubEnabled ? _stubCompositor : vr::VRCompositor();
}

IVRSystem *VR_Init(EVRInitError *peError, EVRApplicationType eApplicationType, const char *pStartupInfo /* = nullptr */)
{
  // pass-through
  if (!_stubEnabled) {
    return vr::VR_Init(peError, eApplicationType, pStartupInfo);
  }

  // create a stub
  _stubSystem = new VRSystemStub();
  _stubCompositor = new VRCompositorStub();
  if (peError)
    *peError = vr::VRInitError_None;

  return _stubSystem;
}

void VR_Shutdown()
{
  // pass-through
  if (!_stubEnabled) {
    vr::VR_Shutdown();
  }

  // destroy stubs
  if (_stubCompositor) {
    delete _stubCompositor;
    _stubCompositor = nullptr;
  }
  if (_stubSystem) {
    delete _stubSystem;
    _stubSystem = nullptr;
  }
}

bool VR_IsHmdPresent()
{
  // pass-through
  if (!_stubEnabled) {
    return vr::VR_IsHmdPresent();
  }
  return true;
}

const char *VR_GetVRInitErrorAsEnglishDescription(EVRInitError error)
{
  return vr::VR_GetVRInitErrorAsEnglishDescription(error);
}

void VRSystemStub::GetRecommendedRenderTargetSize(uint32_t *pnWidth, uint32_t *pnHeight)
{
  *pnWidth = 1512;
  *pnHeight = 1680;
}

void VRSystemStub::GetProjectionRaw(EVREye eEye, float *pfLeft, float *pfRight, float *pfTop, float *pfBottom)
{
  if (eEye == Eye_Left) {
    *pfLeft = -1.39666343f;
    *pfRight = 1.23994398f;
    *pfTop = -1.47110546f;
    *pfBottom = 1.45868433f;
  } else {
    *pfLeft = -1.24060011f;
    *pfRight = 1.39432383f;
    *pfTop = -1.46788049f;
    *pfBottom = 1.46032500f;
  }
}

HmdMatrix34_t VRSystemStub::GetEyeToHeadTransform(EVREye eEye)
{
  static HmdMatrix34_t matrix[] = {{{
    {1.000000000, 0.000000000, 0.000000000, -0.0321500003},
    {0.000000000, 1.000000000, 0.000000000,  0.000000000},
    {0.000000000, 0.000000000, 1.000000000,  0.0149999997},
  }}, {{
    {1.000000000, 0.000000000, 0.000000000, 0.0321500003},
    {0.000000000, 1.000000000, 0.000000000, 0.0000000000},
    {0.000000000, 0.000000000, 1.000000000, 0.0149999997},
  }}};
  return matrix[eEye];
}

ETrackedDeviceClass VRSystemStub::GetTrackedDeviceClass(vr::TrackedDeviceIndex_t unDeviceIndex)
{
  Device_t const* device = DeviceList_t::GetDevice(unDeviceIndex);
  return device ? device->deviceClass : TrackedDeviceClass_Invalid;
}

uint32_t VRSystemStub::GetStringTrackedDeviceProperty(vr::TrackedDeviceIndex_t unDeviceIndex, ETrackedDeviceProperty prop, char *pchValue, uint32_t unBufferSize, ETrackedPropertyError *pError /* = 0L */)
{
  if (pchValue != nullptr && unBufferSize != 0)
    *pchValue = 0;

  Device_t const* device = DeviceList_t::GetDevice(unDeviceIndex);
  if (!device) {
    if (pError)
      *pError = TrackedProp_InvalidDevice;
    return 0;
  }

  DeviceProperty_t const* property = device->FindProperty(prop);
  if (!property) {
    if (pError)
      *pError = TrackedProp_UnknownProperty;
    return 0;
  }

  return property->Get(pchValue, unBufferSize, pError);
}

bool VRSystemStub::PollNextEvent( VREvent_t *pEvent, uint32_t uncbVREvent )
{
  return false;
}

EVRCompositorError VRCompositorStub::WaitGetPoses(TrackedDevicePose_t* pRenderPoseArray, uint32_t unRenderPoseArrayCount, TrackedDevicePose_t* pGamePoseArray, uint32_t unGamePoseArrayCount)
{
  return VRCompositorError_RequestFailed;
}

EVRCompositorError VRCompositorStub::Submit(EVREye eEye, const Texture_t *pTexture, const VRTextureBounds_t* pBounds /* = 0 */, EVRSubmitFlags nSubmitFlags /* = Submit_Default */)
{
  return VRCompositorError_None;
}

} // stub
} // vr 
