#ifndef _H_OpenVRStubDevice
#define _H_OpenVRStubDevice

// system headers
#include "openvr.h"

namespace vr {
namespace stub {

class DeviceProperty_t {
  ETrackedDeviceProperty m_prop;
  PropertyTypeTag_t m_type;
  union {
    char const* m_asString;
  };

public:
  DeviceProperty_t() : m_prop(Prop_Invalid) {}
  DeviceProperty_t(ETrackedDeviceProperty prop, char const* value) : m_prop(prop), m_type(k_unStringPropertyTag), m_asString(value) {}

  bool Is(ETrackedDeviceProperty prop) const {
    return m_prop == prop;
  }

  bool IsValid() const {
    return m_prop != Prop_Invalid;
  }

  unsigned Get(char* buffer, unsigned bufferSize, ETrackedPropertyError* error) const;
};

struct Device_t {
  static const unsigned MAX_DEVICE_PROPERTIES = 2;

  // should be a plain old struct w/o constructor for easier initialization
  ETrackedDeviceClass deviceClass;
  DeviceProperty_t props[MAX_DEVICE_PROPERTIES];

  DeviceProperty_t const* FindProperty(ETrackedDeviceProperty prop) const;
};

struct DeviceList_t {
  static const unsigned MAX_DEVICES = 1;

  static const Device_t devices[MAX_DEVICES];

  static Device_t const* GetDevice(unsigned index);
};

} // stub
} // vr 

#endif /* _H_OpenVRStubDevice */
