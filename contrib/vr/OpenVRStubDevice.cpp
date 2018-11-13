#include "OpenVRStubDevice.h"

#include <string.h>

namespace vr {
namespace stub {

const Device_t DeviceList_t::devices[] = {{
  TrackedDeviceClass_HMD, {
    DeviceProperty_t(Prop_ModelNumber_String, "Stub HMD"),
    DeviceProperty_t(Prop_SerialNumber_String, "0DD-F00D0000"),
  },
}};

unsigned DeviceProperty_t::Get(char* buffer, unsigned bufferSize, ETrackedPropertyError* error) const {
  // validate prop type
  if (m_type != k_unStringPropertyTag) {
    if (error)
      *error = TrackedProp_WrongDataType;
    return 0;
  }

  // validate buffer size
  unsigned size = strlen(m_asString) + 1;
  if (!buffer || bufferSize < size) {
    if (error)
      *error = TrackedProp_BufferTooSmall;
    return size;
  }

  strcpy(buffer, m_asString);
  if (error)
    *error = TrackedProp_Success;
  return size;
}

DeviceProperty_t const* Device_t::FindProperty(ETrackedDeviceProperty prop) const
{
  for (unsigned i = 0; i < MAX_DEVICE_PROPERTIES && props[i].IsValid(); ++i) {
    if (props[i].Is(prop)) {
      return &props[i];
    }
  }
  return nullptr;
}

Device_t const* DeviceList_t::GetDevice(unsigned index)
{
  return index >= MAX_DEVICES || devices[index].deviceClass == TrackedDeviceClass_Invalid ? nullptr : &devices[index];
}

} // stub
} // vr 

