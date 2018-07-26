/* 
A* -------------------------------------------------------------------
B* This file contains source code for the PyMOL computer program
C* Copyright (c) EPAM Systems, Inc.
D* -------------------------------------------------------------------
E* It is unlawful to modify or remove this copyright notice.
F* -------------------------------------------------------------------
G* Please see the accompanying LICENSE file for further information. 
H* -------------------------------------------------------------------
I* Additional authors of this source file include:
-* 
-* 
-*
Z* -------------------------------------------------------------------
*/

#include <string>

#include "os_python.h"

#define nullptr 0
#include "openvr.h"

#include "OpenVRStereo.h"
#include "Feedback.h"

struct COpenVR {
  vr::EVRInitError InitError;
  vr::IVRSystem* System;

  // Such structures used to be calloc-ed, this replicates that
  void *operator new(size_t size) {
    void *mem = ::operator new(size);
    memset(mem, 0, size);
    return mem;
  }
};

static char const* deviceClassNames[] = {
  "Invalid",
  "Head-Mounted Display",
  "Controller",
  "Generic Tracker",
  "Reference Point",
  "Accessory",
};
static const int deviceClassNamesCount = sizeof(deviceClassNames) / sizeof(*deviceClassNames);

bool OpenVRAvailable(PyMOLGlobals *)
{
  return vr::VR_IsHmdPresent();
}

bool OpenVRReady(PyMOLGlobals * G)
{
  COpenVR *I = G->OpenVR;
  return I && I->InitError == vr::VRInitError_None && I->System != NULL;
}

int OpenVRInit(PyMOLGlobals * G)
{
  if(G->OpenVR)
    return 1; // already initialized
  else if (!OpenVRAvailable(G))
    return 0; // don't bother initializing the whole system

  COpenVR *I = G->OpenVR = new COpenVR();
  if(I) {
    I->InitError = vr::VRInitError_None;
    I->System = vr::VR_Init(&I->InitError, vr::VRApplication_Scene);
    if (I->InitError != vr::VRInitError_None) {
      I->System = NULL;
      return 0;
    }
    return 1;
  } else
    return 0;
}

void OpenVRFree(PyMOLGlobals * G)
{
  if(!G->OpenVR)
    return; // nothing to free

  COpenVR *I = G->OpenVR;
  if(I->System) {
    vr::VR_Shutdown();
    I->System = NULL;
  }

  delete G->OpenVR;
  G->OpenVR = NULL;
}

static std::string GetStringTrackedDeviceProperty(vr::IVRSystem *System, vr::TrackedDeviceIndex_t index, vr::TrackedDeviceProperty prop)
{
  uint32_t length = System->GetStringTrackedDeviceProperty(index, prop, NULL, 0);
  if(length != 0) {
    std::string buffer(length, 0);
    if (System->GetStringTrackedDeviceProperty(index, prop, &buffer[0], length) != 0) {
      return buffer;
    }
  }

  return std::string("ERROR");
}

void OpenVRFeedback(PyMOLGlobals * G)
{
  COpenVR *I = G->OpenVR;
  if(!OpenVRAvailable(G)) {
    FeedbackAdd(G, " OpenVR system is not available.\n");
  } else if(!OpenVRReady(G)) {
    PRINTF
      " OpenVR system is not ready: %s.\n",
      I ? vr::VR_GetVRInitErrorAsEnglishDescription(I->InitError) : "Failed to initialize properly"
    ENDF(G);
  } else {
    FeedbackAdd(G, " Detected OpenVR system. Devices being currently tracked:\n");

    bool found = false;
    for(uint32_t i = 0; i < vr::k_unMaxTrackedDeviceCount; ++i) {
      vr::ETrackedDeviceClass deviceClass = I->System->GetTrackedDeviceClass(i);
      if(deviceClass != vr::TrackedDeviceClass_Invalid) {
        found = true;

        std::string model = GetStringTrackedDeviceProperty(I->System, i, vr::Prop_ModelNumber_String);
        std::string serial = GetStringTrackedDeviceProperty(I->System, i, vr::Prop_SerialNumber_String);

        PRINTF "  %02u: %s (%s %s)\n", i,
          deviceClass < deviceClassNamesCount ? deviceClassNames[deviceClass] : "Unknown",
          model.c_str(),
          serial.c_str()
        ENDF(G);
      }
    }
    if(!found) {
      FeedbackAdd(G, "  No valid devices found.\n");
    }
  }
  FeedbackAdd(G, "\n");
}
