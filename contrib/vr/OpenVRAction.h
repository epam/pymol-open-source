#ifndef _H_OpenVRAction
#define _H_OpenVRAction

#include "openvr.h"

struct OpenVRAction {
  char const* path;

  enum Type {
    TYPE_DIGITAL,
    TYPE_ANALOG,
    TYPE_POSE,
  } type;

  vr::VRActionHandle_t handle;
  vr::VRInputValueHandle_t restrictToDevice;

  union {
    struct {
      bool bActive;
      vr::VRInputValueHandle_t activeOrigin;
    };
    vr::InputDigitalActionData_t digital;
    vr::InputAnalogActionData_t analog;
    vr::InputPoseActionData_t pose;
  };

  vr::InputOriginInfo_t origin;

  explicit OpenVRAction(vr::IVRInput* Input, char const* path, Type type = TYPE_DIGITAL)
    : path(path)
    , type(type)
    , handle(vr::k_ulInvalidActionHandle)
    , restrictToDevice(vr::k_ulInvalidInputValueHandle)
  {
    bActive = false;
    Input->GetActionHandle(path, &handle);
  }

  bool IsPressed() const {
    return bActive && digital.bState;
  }

  bool WasPressed() const {
    return bActive && digital.bChanged && digital.bState;
  }

  bool WasReleased() const {
    return bActive && digital.bChanged && !digital.bState;
  }

  bool WasPressedOrReleased() const {
    return bActive && digital.bChanged;
  }

  bool PoseValid() const {
    return bActive && pose.pose.bPoseIsValid;
  }

  unsigned DeviceIndex() const {
    return origin.trackedDeviceIndex;
  }

  void Update(vr::IVRInput* Input) {
    vr::EVRInputError error = vr::VRInputError_WrongType;
    if (handle != vr::k_ulInvalidActionHandle) {
      switch (type) {
        case TYPE_DIGITAL:
          error = Input->GetDigitalActionData(handle, &digital, sizeof(digital), restrictToDevice);
          break;
        case TYPE_ANALOG:
          error = Input->GetAnalogActionData(handle, &analog, sizeof(analog), restrictToDevice);
          break;
        case TYPE_POSE:
          error = Input->GetPoseActionData(handle, vr::TrackingUniverseSeated, 0.0f, &pose, sizeof(pose), restrictToDevice);
          break;
        default:
          break;
      }
    }
    if (error != vr::VRInputError_None)
      bActive = false;
    if (bActive)
      error = Input->GetOriginTrackedDeviceInfo(activeOrigin, &origin, sizeof(origin));
    if (error != vr::VRInputError_None) {
      origin.devicePath = vr::k_ulInvalidInputValueHandle;
      origin.trackedDeviceIndex = vr::k_unTrackedDeviceIndexInvalid;
      origin.rchRenderModelComponentName[0] = '\0';
    }
  }
};

#endif // _H_OpenVRAction
