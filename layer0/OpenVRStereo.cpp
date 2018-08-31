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

#include "os_std.h"
#include "os_gl.h"
#include "os_python.h"

#include <string>
#include <vector>

#include "openvr.h"

#include "OpenVRStereo.h"
#include "OpenVRStub.h"
#include "OpenVRController.h"
#include "OpenVRMenu.h"
#include "PyMOLOptions.h"
#include "Feedback.h"
#include "Matrix.h"

struct CEye {
  vr::EVREye Eye;

  GLfloat HeadToEyeMatrix[16];
  GLfloat ProjectionMatrix[16];

  GLuint FrameBufferID;
  GLuint DepthBufferID;
  GLuint ColorBufferID;

  GLuint ResolveBufferID;
  GLuint ResolveTextureID;

  vr::Texture_t Texture;
};

enum EHand
{
  HLeft = 0,
  HRight = 1,
  COUNT,
};

struct COpenVR {
  vr::EVRInitError InitError;
  vr::IVRSystem* System;
  vr::IVRCompositor* Compositor;
  vr::IVRInput* Input;
  vr::TrackedDevicePose_t Poses[vr::k_unMaxTrackedDeviceCount]; // todo remove from globals?

  GLfloat HeadPose[16];
  GLfloat WorldToHeadMatrix[16];

  unsigned Width;
  unsigned Height;

  CEye* Eye;
  CEye Left;
  CEye Right;

  OpenVRController Hands[COUNT];

  bool ForcedFront;
 
  OpenVRMenu Menu;

  // Such structures used to be calloc-ed, this replicates that
  void *operator new(size_t size) {
    void *mem = ::operator new(size);
    memset(mem, 0, size);
    return mem;
  }

  vr::VRActionHandle_t m_actionToggleMenu;
  vr::VRActionSetHandle_t m_actionset; 
};

static const float OPEN_VR_FRONT = 0.1f;

static char const* deviceClassNames[] = {
  "Invalid",
  "Head-Mounted Display",
  "Controller",
  "Generic Tracker",
  "Reference Point",
  "Accessory",
};
static const int deviceClassNamesCount = sizeof(deviceClassNames) / sizeof(*deviceClassNames);

static void FastInverseAffineSteamVRMatrix(float const *srcMat34, float *dstMat44);
static void ConvertSteamVRMatrixToGLMat(float const* srcMat34, float *dstMat44);
void UpdateDevicePoses(PyMOLGlobals * G);

bool OpenVRAvailable(PyMOLGlobals *)
{
  return vr::stub::VR_IsHmdPresent();
}

bool OpenVRReady(PyMOLGlobals * G) 
{
  COpenVR *I = G->OpenVR;
  return I && I->InitError == vr::VRInitError_None && I->System != NULL;
}

static bool EyeInit(CEye * I, vr::EVREye eye, int scene_width, int scene_height)
{
  I->Eye = eye;

  // framebuffer
  glGenFramebuffersEXT(1, &I->FrameBufferID);
  glBindFramebufferEXT(GL_FRAMEBUFFER, I->FrameBufferID);

  // - depth
  glGenRenderbuffersEXT(1, &I->DepthBufferID);
  glBindRenderbufferEXT(GL_RENDERBUFFER, I->DepthBufferID);
  glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER, 4, GL_DEPTH_COMPONENT, scene_width, scene_height);
  glFramebufferRenderbufferEXT(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, I->DepthBufferID);

  // - color
  glGenTextures(1, &I->ColorBufferID);
  glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, I->ColorBufferID);
  glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGBA8, scene_width, scene_height, GL_TRUE);
  glFramebufferTexture2DEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, I->ColorBufferID, 0);
  glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
  
  // resolve buffer
  glGenFramebuffersEXT(1, &I->ResolveBufferID);
  glBindFramebufferEXT(GL_FRAMEBUFFER, I->ResolveBufferID);
  
  // - color
  glGenTextures(1, &I->ResolveTextureID);
  glBindTexture(GL_TEXTURE_2D, I->ResolveTextureID);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, scene_width, scene_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  glFramebufferTexture2DEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, I->ResolveTextureID, 0);
  glBindTexture(GL_TEXTURE_2D, 0);

  // VR texture
  I->Texture.handle = (void*)I->ResolveTextureID;
  I->Texture.eType = vr::TextureType_OpenGL;
  I->Texture.eColorSpace = vr::ColorSpace_Gamma;

  // check FBO status
  GLenum status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER);
  glBindFramebufferEXT(GL_FRAMEBUFFER, 0);
  return (status == GL_FRAMEBUFFER_COMPLETE);
}

static void EyeFree(CEye * I)
{
  glDeleteTextures(1, &I->ResolveTextureID);
  glDeleteFramebuffers(1, &I->ResolveBufferID);
  glDeleteTextures(1, &I->ColorBufferID);
  glDeleteRenderbuffers(1, &I->DepthBufferID);
  glDeleteFramebuffers(1, &I->FrameBufferID);
}

int OpenVRInit(PyMOLGlobals * G)
{
  if(G->OpenVR)
    return 1; // already initialized
  
  vr::stub::VR_StubEnable(G->Option->openvr_stub);
  if (!OpenVRAvailable(G))
    return 0; // don't bother initializing the whole system

  COpenVR *I = G->OpenVR = new COpenVR();
  if(!I) 
    return 0;

  I->InitError = vr::VRInitError_None;
  I->System = vr::stub::VR_Init(&I->InitError, vr::VRApplication_Scene);
  if (I->InitError != vr::VRInitError_None) {
    I->System = NULL;
    return 0;
  }

  I->Compositor = vr::stub::VRCompositor();
  I->ForcedFront = true;

  I->Input = vr::stub::VRInput(); 
  if (I->Input) {
    // init manifest
    std::string manifestPath = std::string(getenv("PYMOL_PATH")) + "\\data\\openvr\\actions.json";
    I->Input->SetActionManifestPath(manifestPath.c_str());

    I->Input->GetActionSetHandle("/actions/pymol", &I->m_actionset); 

    I->Input->GetInputSourceHandle("/user/hand/left", &I->Hands[HLeft].m_source);
    I->Input->GetActionHandle("/actions/pymol/in/Hand_Left", &I->Hands[HLeft].m_actionPose);

    I->Input->GetInputSourceHandle("/user/hand/right", &I->Hands[HRight].m_source);
    I->Input->GetActionHandle("/actions/pymol/in/Hand_Right", &I->Hands[HRight].m_actionPose);

    I->Input->GetActionHandle("/actions/pymol/in/ToggleMenu", &I->m_actionToggleMenu);
  }
  
  return 1;
}

void OpenVRFree(PyMOLGlobals * G)
{
  ShutdownRenderModels();

  if(!G->OpenVR)
    return;

  COpenVR *I = G->OpenVR;
  if(I->System) {
    vr::stub::VR_Shutdown();

    I->Menu.Free();

    I->Hands[HLeft].Free();
    I->Hands[HRight].Free();

    EyeFree(&I->Right);
    EyeFree(&I->Left);

    I->System = NULL;
  }

  delete G->OpenVR;
  G->OpenVR = NULL;
}

static void OpenVRInitPostponed(PyMOLGlobals * G)
{
  COpenVR *I = G->OpenVR;
  if(!OpenVRReady(G))
    return;

  if (!I->Width || !I->Height) {
    I->System->GetRecommendedRenderTargetSize(&I->Width, &I->Height);
    EyeInit(&I->Left, vr::Eye_Left, I->Width, I->Height);
    EyeInit(&I->Right, vr::Eye_Right, I->Width, I->Height);

    I->Menu.Init();
  }

  for (int i = HLeft; i <= HRight; ++i) {
    OpenVRController &hand = I->Hands[i];
    if (!hand.IsInitialized()) {
      hand.Init(G);
    }  
  }  
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

  return std::string("<ERROR>");
}

void OpenVRFeedback(PyMOLGlobals * G)
{
  COpenVR *I = G->OpenVR;
  if(vr::stub::VR_IsStubEnabled()) {
    FeedbackAdd(G, " OpenVR stub is enabled.\n");
  }
  if(!OpenVRAvailable(G)) {
    FeedbackAdd(G, " OpenVR system is not available.\n");
  } else if(!OpenVRReady(G)) {
    PRINTF
      " OpenVR system is not ready: %s.\n",
      I ? vr::stub::VR_GetVRInitErrorAsEnglishDescription(I->InitError) : "Failed to initialize properly"
    ENDF(G);
  } else {
    FeedbackAdd(G, " Detected OpenVR system. Devices being currently tracked:\n");

    bool found = false;
    for(uint32_t i = 0; i < vr::k_unMaxTrackedDeviceCount; ++i) {
      vr::ETrackedDeviceClass deviceClass = I->System->GetTrackedDeviceClass(i);
      if(deviceClass != vr::TrackedDeviceClass_Invalid) {
        found = true;

        char const* className = (0 <= deviceClass && deviceClass < deviceClassNamesCount) ? deviceClassNames[deviceClass] : "<ERROR>";
        std::string model = GetStringTrackedDeviceProperty(I->System, i, vr::Prop_ModelNumber_String);
        std::string serial = GetStringTrackedDeviceProperty(I->System, i, vr::Prop_SerialNumber_String);

        PRINTF "  %02u: %s (%s %s)\n", i, className, model.c_str(), serial.c_str() ENDF(G);
      }
    }
    if(!found) {
      FeedbackAdd(G, "  No valid devices found.\n");
    }
  }
  FeedbackAdd(G, "\n");
}

void OpenVRFrameStart(PyMOLGlobals * G)
{
  COpenVR *I = G->OpenVR;
  if(!OpenVRReady(G))
    return;

  // create OpenGL assets on the first use
  OpenVRInitPostponed(G);

  // get matrices from tracked devices
  if (I->Compositor->WaitGetPoses(I->Poses, vr::k_unMaxTrackedDeviceCount, NULL, 0) != vr::VRCompositorError_None) {
    FeedbackAdd(G, "  Cannot update device poses\n");
  } 
  UpdateDevicePoses(G);
}

void OpenVREyeStart(PyMOLGlobals * G, int eye)
{
  COpenVR *I = G->OpenVR;
  if(!OpenVRReady(G))
    return;

  GL_DEBUG_FUN();

  CEye *E = I->Eye = eye ? &I->Right : &I->Left;

  glBindFramebufferEXT(GL_FRAMEBUFFER, E->FrameBufferID);
  glViewport(0, 0, I->Width, I->Height);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void OpenVREyeFinish(PyMOLGlobals * G)
{
  COpenVR *I = G->OpenVR;
  if(!OpenVRReady(G))
    return;

  GL_DEBUG_FUN();

  CEye *E = I->Eye;
  if(!E)
    return;

  if(G->Option->multisample)
    glDisable(0x809D);       /* GL_MULTISAMPLE_ARB */

  glBindFramebufferEXT(GL_READ_FRAMEBUFFER, E->FrameBufferID);
  glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER, E->ResolveBufferID);
  glBlitFramebufferEXT(0, 0, I->Width, I->Height, 0, 0, I->Width, I->Height, GL_COLOR_BUFFER_BIT, GL_LINEAR);
  glBindFramebufferEXT(GL_READ_FRAMEBUFFER, 0);
  glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER, 0);
  glBindFramebufferEXT(GL_FRAMEBUFFER, 0);

  if(G->Option->multisample)
    glEnable(0x809D);       /* GL_MULTISAMPLE_ARB */
  
  I->Eye = NULL;
}

void OpenVRFrameFinish(PyMOLGlobals * G, unsigned scene_width, unsigned scene_height)
{
  COpenVR *I = G->OpenVR;
  if(!OpenVRReady(G))
    return;

  GL_DEBUG_FUN();

  // send rendered pictures into the headset
  I->Compositor->Submit(vr::Eye_Left, &I->Left.Texture);
  I->Compositor->Submit(vr::Eye_Right, &I->Right.Texture);

  // find a proper rectangle with the scene aspect ratio
  unsigned width = I->Height * scene_width / scene_height;
  unsigned height = I->Width * scene_height / scene_width;
  unsigned dx = 0, dy = 0;
  if (width < I->Width) {
    dx = (I->Width - width) / 2;
    height = I->Height;
  } else {
    dy = (I->Height - height) / 2;
    width = I->Width;
  }

  // display a copy of the VR framebuffer in the main PyMOL window
  glDrawBuffer(GL_BACK);
  glBindFramebufferEXT(GL_READ_FRAMEBUFFER, I->Left.ResolveBufferID);
  glBlitFramebufferEXT(dx, dy, dx + width, dy + height, 0, 0, scene_width, scene_height, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);
  glBindFramebufferEXT(GL_READ_FRAMEBUFFER, 0);
}

void OpenVRMenuBufferStart(PyMOLGlobals * G, unsigned width, unsigned height, bool clear /* = false */)
{
  COpenVR *I = G->OpenVR;
  if(!OpenVRReady(G))
    return;

  I->Menu.Start(width, height, clear);
}

void OpenVRMenuBufferFinish(PyMOLGlobals * G)
{
  COpenVR *I = G->OpenVR;
  if(!OpenVRReady(G))
    return;

  I->Menu.Finish();
}

void OpenVRMenuToggle(PyMOLGlobals * G)
{
  COpenVR *I = G->OpenVR;
  if(!OpenVRReady(G))
    return;

  if (!I->Menu.IsVisible())
    I->Menu.Show(I->HeadPose);
  else
    I->Menu.Hide();
}

float* OpenVRGetHeadToEye(PyMOLGlobals * G)
{
  COpenVR *I = G->OpenVR;
  if(!OpenVRReady(G) || !I->Eye)
    return NULL;

  CEye *E = I->Eye;
  vr::HmdMatrix34_t EyeToHeadTransform = I->System->GetEyeToHeadTransform(E->Eye);
  FastInverseAffineSteamVRMatrix((const float *)EyeToHeadTransform.m, E->HeadToEyeMatrix);

  return E->HeadToEyeMatrix;
}

float* OpenVRGetWorldToHead(PyMOLGlobals * G) {
  COpenVR *I = G->OpenVR;
  if(!OpenVRReady(G))
    return NULL;

  return I->WorldToHeadMatrix;
}

float* OpenVRGetControllerPose(PyMOLGlobals * G, EHand handIdx) {
  COpenVR *I = G->OpenVR;
  if(!OpenVRReady(G))
    return NULL;

  return I->Hands[handIdx].GetPose();
}

float* OpenVRGetProjection(PyMOLGlobals * G, float near_plane, float far_plane)
{
  COpenVR *I = G->OpenVR;
  if(!OpenVRReady(G) || !I->Eye)
    return NULL;

  CEye *E = I->Eye;

  if (I->ForcedFront)
    near_plane = OPEN_VR_FRONT;

  float left, right, top, bottom;
  I->System->GetProjectionRaw(E->Eye, &left, &right, &top, &bottom);
  
  // fast affine inverse matrix, row major to column major, whew...
  {
    float (*dst)[4] = (float(*)[4])E->ProjectionMatrix;
    float dx = (right - left);
    float dy = (bottom - top);
    float dz = far_plane - near_plane;

    // transpose rotation
    dst[0][0] = 2.0f / dx;
    dst[0][1] = 0.0f;
    dst[0][2] = 0.0f;
    dst[0][3] = 0.0f;
    
    dst[1][0] = 0.0f;
    dst[1][1] = 2.0f / dy;
    dst[1][2] = 0.0f;
    dst[1][3] = 0.0f;
    
    dst[2][0] = (right + left) / dx;
    dst[2][1] = (top + bottom) / dy;
    dst[2][2] = -(far_plane + near_plane) / dz;
    dst[2][3] = -1.0f;
    
    dst[3][0] = 0.0f;
    dst[3][1] = 0.0f;
    dst[3][2] = -2.0f * far_plane * near_plane / dz;
    dst[3][3] = 0.0f;
  }

  return E->ProjectionMatrix;
}

void OpenVRLoadProjectionMatrix(PyMOLGlobals * G, float near_plane, float far_plane)
{
  glLoadMatrixf(OpenVRGetProjection(G, near_plane, far_plane));
}

void OpenVRLoadWorld2EyeMatrix(PyMOLGlobals * G)
{
  glLoadMatrixf(OpenVRGetHeadToEye(G));
  glMultMatrixf(OpenVRGetWorldToHead(G));
}

void GetActionDevice(COpenVR* I, vr::VRInputValueHandle_t actionOrigin, vr::TrackedDeviceIndex_t* deviceIndex = 0)
{
  if (deviceIndex) {
    vr::InputOriginInfo_t originInfo;
    if (I->Input->GetOriginTrackedDeviceInfo(actionOrigin, &originInfo, sizeof(originInfo)) == vr::VRInputError_None) {
      *deviceIndex = originInfo.trackedDeviceIndex;
    } else {
      *deviceIndex = vr::k_unTrackedDeviceIndexInvalid;
    }
  }
}

// generic function for acquiring button state, use inline specializations defined below
bool CheckButtonAction(COpenVR* I, vr::VRActionHandle_t action, bool pressOnly, bool changeOnly, vr::TrackedDeviceIndex_t* deviceIndex = 0)
{
  if (deviceIndex)
    *deviceIndex = vr::k_unTrackedDeviceIndexInvalid;

  if (!I || !I->Input)
    return false;

  vr::InputDigitalActionData_t actionData;
  if (I->Input->GetDigitalActionData(action, &actionData, sizeof(actionData), vr::k_ulInvalidInputValueHandle) == vr::VRInputError_None) {
    if (actionData.bActive && (!changeOnly || actionData.bChanged) && (!pressOnly || actionData.bState)) {
      GetActionDevice(I, actionData.activeOrigin, deviceIndex);
      return true;
    }
  }

  return false;
}

// returns true while the button is pressed
inline bool CheckButtonState(COpenVR* I, vr::VRActionHandle_t action, vr::TrackedDeviceIndex_t* deviceIndex = 0)
{
  return CheckButtonAction(I, action, true, false, deviceIndex);
}

// returns true when user has just pressed the button
inline bool CheckButtonClick(COpenVR* I, vr::VRActionHandle_t action, vr::TrackedDeviceIndex_t* deviceIndex = 0)
{
  return CheckButtonAction(I, action, true, true, deviceIndex);
}

// returns true when user has just pressed or released the button
inline bool CheckButtonToggle(COpenVR* I, vr::VRActionHandle_t action, vr::TrackedDeviceIndex_t* deviceIndex = 0)
{
  return CheckButtonAction(I, action, false, true, deviceIndex);
}

std::string GetTrackedDeviceString(PyMOLGlobals * G, vr::TrackedDeviceIndex_t unDevice, vr::TrackedDeviceProperty prop, vr::TrackedPropertyError *peError = NULL )
{
  COpenVR *I = G->OpenVR;
  if (!I || !I->System) 
    return "";

  uint32_t unRequiredBufferLen = I->System->GetStringTrackedDeviceProperty( unDevice, prop, NULL, 0, peError );
  if( unRequiredBufferLen == 0 )
    return "";

  char *pchBuffer = new char[ unRequiredBufferLen ];
  unRequiredBufferLen = I->System->GetStringTrackedDeviceProperty( unDevice, prop, pchBuffer, unRequiredBufferLen, peError );
  std::string sResult = pchBuffer;
  delete [] pchBuffer;
  return sResult;
}

void OpenVRHandleInput(PyMOLGlobals * G)
{
  COpenVR *I = G->OpenVR;
  if(!OpenVRReady(G))
    return;

  vr::VREvent_t event;
  while (I->System->PollNextEvent(&event, sizeof(event)))
    /* pass */;

  if (!I->Input)
    return;
  
  // Process SteamVR action state
  // UpdateActionState is called each frame to update the state of the actions themselves. The application
  // controls which action sets are active with the provided array of VRActiveActionSet_t structs.
  vr::VRActiveActionSet_t actionSet = { 0 };
  actionSet.ulActionSet = I->m_actionset;
  I->Input->UpdateActionState( &actionSet, sizeof(actionSet), 1 );

  // process actions
  if (I->Input) {
    if (CheckButtonClick(I, I->m_actionToggleMenu)) {
      OpenVRMenuToggle(G); // TODO: call user action handler
    }
    // ...etc
  }

  // get position and source if needed
  I->Hands[HLeft].Show(true);
  I->Hands[HRight].Show(true);
  for (int i = HLeft; i <= HRight; i++) {
    vr::InputPoseActionData_t poseData;
    vr::EVRInputError result = I->Input->GetPoseActionData( I->Hands[i].m_actionPose, vr::TrackingUniverseSeated, 0, &poseData, sizeof(poseData),
      vr::k_ulInvalidInputValueHandle);
    if (result != vr::VRInputError_None || !poseData.bActive || !poseData.pose.bPoseIsValid) {
      I->Hands[i].Show(false);
    } else {
      //FIXME
    //	m_rHand[eHand].m_rmat4Pose = ConvertSteamVRMatrixToMatrix4( poseData.pose.mDeviceToAbsoluteTracking );
      vr::InputOriginInfo_t originInfo;
      if ( I->Input->GetOriginTrackedDeviceInfo(poseData.activeOrigin, &originInfo, sizeof(originInfo)) == vr::VRInputError_None 
        && originInfo.trackedDeviceIndex != vr::k_unTrackedDeviceIndexInvalid ) {
        std::string sRenderModelName = GetTrackedDeviceString(G, originInfo.trackedDeviceIndex, vr::Prop_RenderModelName_String);
        if ( sRenderModelName != I->Hands[i].m_sRenderModelName ) {
          I->Hands[i].m_pRenderModel = FindOrLoadRenderModel(G, sRenderModelName.c_str());
          I->Hands[i].m_sRenderModelName = sRenderModelName;
        }
      }
    }
  }

  // intersect the laser ray with the UI
  if (I->Menu.IsVisible()) {
    OpenVRController* hand = &I->Hands[HRight];
    if (!hand->IsVisible())
      hand = &I->Hands[HLeft];
    if (hand->IsVisible()) {
      float const* handPose = hand->GetPose();
      float const* origin = handPose + 12;
      float direction[3] = {-handPose[8], -handPose[9], -handPose[10]};

      int x, y;
      bool hit = I->Menu.IntersectRay(origin, direction, &x, &y);
      if (hit) {
        I->Menu.ShowtHotspot(x, y);
      } else {
        I->Menu.HideHotspot();
      }
    }
  }
}

void UpdateDevicePoses(PyMOLGlobals * G) {
  COpenVR *I = G->OpenVR;

  for (uint32_t nDevice = 0; nDevice < vr::k_unMaxTrackedDeviceCount; nDevice++) {
    vr::TrackedDevicePose_t &pose = I->Poses[nDevice];
    if (pose.bPoseIsValid) {
      vr::ETrackedDeviceClass device = I->System->GetTrackedDeviceClass(nDevice);
      switch (device) {
        case vr::TrackedDeviceClass_HMD:
          ConvertSteamVRMatrixToGLMat((const float *)pose.mDeviceToAbsoluteTracking.m, I->HeadPose);
          FastInverseAffineSteamVRMatrix((const float *)pose.mDeviceToAbsoluteTracking.m, I->WorldToHeadMatrix);
          break;
        case vr::TrackedDeviceClass_Controller:
          {
            vr::ETrackedControllerRole role = I->System->GetControllerRoleForTrackedDeviceIndex(nDevice);
            if (role == vr::TrackedControllerRole_LeftHand)
              ConvertSteamVRMatrixToGLMat((const float *)pose.mDeviceToAbsoluteTracking.m, (float *)I->Hands[HLeft].GetPose());
            else if (role == vr::TrackedControllerRole_RightHand)
              ConvertSteamVRMatrixToGLMat((const float *)pose.mDeviceToAbsoluteTracking.m, (float *)I->Hands[HRight].GetPose());
          }
          break;
        default:
          break;
      }
    }
  }
}

void OpenVRDraw(PyMOLGlobals * G)
{
  COpenVR *I = G->OpenVR;
  if(!OpenVRReady(G))
    return;

  GL_DEBUG_FUN();

  glPushMatrix();
  OpenVRLoadWorld2EyeMatrix(G);

  // render menu if present
  I->Menu.Draw();

  // render controllers
  for (int i = HLeft; i <= HRight; ++i) {
    I->Hands[i].Draw(G);  
  }

  glPopMatrix();
}

// Fast affine inverse matrix, row major to column major, whew...
static void FastInverseAffineSteamVRMatrix(float const *srcMat34, float *dstMat44) {
    float const (*src)[4] = (float const (*)[4])srcMat34;
    float (*dst)[4] = (float (*)[4])dstMat44;

    // transpose rotation
    dst[0][0] = src[0][0];
    dst[0][1] = src[0][1];
    dst[0][2] = src[0][2];
    dst[0][3] = 0.0f;
    dst[1][0] = src[1][0];
    dst[1][1] = src[1][1];
    dst[1][2] = src[1][2];
    dst[1][3] = 0.0f;
    dst[2][0] = src[2][0];
    dst[2][1] = src[2][1];
    dst[2][2] = src[2][2];
    dst[2][3] = 0.0f;
    
    // transpose-rotated negative translation
    dst[3][0] = -(src[0][0] * src[0][3] + src[1][0] * src[1][3] + src[2][0] * src[2][3]);
    dst[3][1] = -(src[0][1] * src[0][3] + src[1][1] * src[1][3] + src[2][1] * src[2][3]);
    dst[3][2] = -(src[0][2] * src[0][3] + src[1][2] * src[1][3] + src[2][2] * src[2][3]);
    dst[3][3] = 1.0f;
}

static void ConvertSteamVRMatrixToGLMat(float const* srcMat34, float *dstMat44)
{
  float (*dst)[4] = (float(*)[4])dstMat44;
  float (*src)[4] = (float(*)[4])srcMat34;
  dst[0][0] = src[0][0]; dst[0][1] = src[1][0]; dst[0][2] = src[2][0]; dst[0][3] = 0.0f;
  dst[1][0] = src[0][1]; dst[1][1] = src[1][1]; dst[1][2] = src[2][1]; dst[1][3] = 0.0f;
  dst[2][0] = src[0][2]; dst[2][1] = src[1][2]; dst[2][2] = src[2][2]; dst[2][3] = 0.0f;
  dst[3][0] = src[0][3]; dst[3][1] = src[1][3]; dst[3][2] = src[2][3]; dst[3][3] = 1.0f;
}
