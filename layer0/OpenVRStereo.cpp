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

#define nullptr 0
#include "openvr.h"

#include "OpenVRStereo.h"
#include "OpenVRStub.h"
#include "PyMOLOptions.h"
#include "Feedback.h"
#include "OpenVRController.h"

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
  vr::TrackedDevicePose_t Poses[vr::k_unMaxTrackedDeviceCount]; // todo remove from globals?
  GLfloat HmdPose[16];

  unsigned Width;
  unsigned Height;

  CEye* Eye;
  CEye Left;
  CEye Right;

  OpenVRController Hands[COUNT];
  GLfloat HandsPose[COUNT][16];
 
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

void static ConvertOpenVRMatrixToMatrix4( const vr::HmdMatrix34_t &mat, GLfloat *fMat);
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
  if(status != GL_FRAMEBUFFER_COMPLETE)
    return false;

  glBindFramebufferEXT(GL_FRAMEBUFFER, 0);
  return true;
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
  if(I) {
    I->InitError = vr::VRInitError_None;
    I->System = vr::stub::VR_Init(&I->InitError, vr::VRApplication_Scene);
    if (I->InitError != vr::VRInitError_None) {
      I->System = NULL;
      return 0;
    }

    I->Compositor = vr::stub::VRCompositor();
    return 1;
  } else
    return 0;
}

void OpenVRFree(PyMOLGlobals * G)
{
  if(!G->OpenVR)
    return;

  COpenVR *I = G->OpenVR;
  if(I->System) {
    vr::stub::VR_Shutdown();

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

  CEye *E = I->Eye = eye ? &I->Right : &I->Left;

  glBindFramebuffer(GL_FRAMEBUFFER, E->FrameBufferID);
  glViewport(0, 0, I->Width, I->Height);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void OpenVREyeFinish(PyMOLGlobals * G)
{
  COpenVR *I = G->OpenVR;
  if(!OpenVRReady(G))
    return;

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

  if(G->Option->multisample)
    glEnable(0x809D);       /* GL_MULTISAMPLE_ARB */
  
  I->Eye = NULL;
}

void OpenVRFrameFinish(PyMOLGlobals * G, unsigned scene_width, unsigned scene_height)
{
  COpenVR *I = G->OpenVR;
  if(!OpenVRReady(G))
    return;

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

// FIXME make static
// Fast affine inverse matrix, row major to column major, whew...
void FastInverseAffineMatrix(float const *src, float *dst) {
    float const (*src44)[4] = (float const (*)[4])src;
    float (*dst44)[4] = (float (*)[4])dst;

    // transpose rotation
    dst44[0][0] = src44[0][0];
    dst44[0][1] = src44[0][1];
    dst44[0][2] = src44[0][2];
    dst44[0][3] = 0.0f;
    dst44[1][0] = src44[1][0];
    dst44[1][1] = src44[1][1];
    dst44[1][2] = src44[1][2];
    dst44[1][3] = 0.0f;
    dst44[2][0] = src44[2][0];
    dst44[2][1] = src44[2][1];
    dst44[2][2] = src44[2][2];
    dst44[2][3] = 0.0f;
    
    // trnspose-rotated negative translation
    dst44[3][0] = -(src44[0][0] * src44[0][3] + src44[1][0] * src44[1][3] + src44[2][0] * src44[2][3]);
    dst44[3][1] = -(src44[0][1] * src44[0][3] + src44[1][1] * src44[1][3] + src44[2][1] * src44[2][3]);
    dst44[3][2] = -(src44[0][2] * src44[0][3] + src44[1][2] * src44[1][3] + src44[2][2] * src44[2][3]);
    dst44[3][3] = 1.0f;
}

float* OpenVRGetHeadToEye(PyMOLGlobals * G)
{
  COpenVR *I = G->OpenVR;
  if(!OpenVRReady(G) || !I->Eye)
    return NULL;

  CEye *E = I->Eye;
  vr::HmdMatrix34_t EyeToHeadTransform = I->System->GetEyeToHeadTransform(E->Eye);
  FastInverseAffineMatrix((const float *)EyeToHeadTransform.m, E->HeadToEyeMatrix);

  return E->HeadToEyeMatrix;
}

float* OpenVRGetHDMPose(PyMOLGlobals * G) {
  COpenVR *I = G->OpenVR;
  if(!OpenVRReady(G))
    return NULL;

  return I->HmdPose;
}

float* OpenVRGetControllerPose(PyMOLGlobals * G, EHand handIdx) {
  COpenVR *I = G->OpenVR;
  if(!OpenVRReady(G))
    return NULL;

  //FIXME
  return I->HandsPose[handIdx];
}

float* OpenVRGetProjection(PyMOLGlobals * G, float near_plane, float far_plane)
{
  COpenVR *I = G->OpenVR;
  if(!OpenVRReady(G) || !I->Eye)
    return NULL;

  CEye *E = I->Eye;

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

void OpenVRHandleInput(PyMOLGlobals * G)
{
  COpenVR *I = G->OpenVR;
  if(!OpenVRReady(G))
    return;

  vr::VREvent_t event;
  while (I->System->PollNextEvent(&event, sizeof(event)))
    /* pass */;
}

void UpdateDevicePoses(PyMOLGlobals * G) {
  COpenVR *I = G->OpenVR;

  for (uint32_t nDevice = 0; nDevice < vr::k_unMaxTrackedDeviceCount; nDevice++) {
    vr::TrackedDevicePose_t &pose = I->Poses[nDevice];
    if (pose.bPoseIsValid) {
      vr::ETrackedDeviceClass device = I->System->GetTrackedDeviceClass(nDevice);
      switch (device) {
        case vr::TrackedDeviceClass_HMD:
          FastInverseAffineMatrix((const float *)pose.mDeviceToAbsoluteTracking.m, I->HmdPose);
          break;
        case vr::TrackedDeviceClass_Controller:
         {
            vr::ETrackedControllerRole role = I->System->GetControllerRoleForTrackedDeviceIndex(nDevice);
            if (role == vr::TrackedControllerRole_LeftHand)
              ConvertOpenVRMatrixToMatrix4(pose.mDeviceToAbsoluteTracking, I->HandsPose[HLeft]);
            else if (role == vr::TrackedControllerRole_RightHand)
              ConvertOpenVRMatrixToMatrix4(pose.mDeviceToAbsoluteTracking, I->HandsPose[HRight]);
          }
          break;
        default:
          break;
      }
    }
  }
}

// FIXME change params
static void ConvertOpenVRMatrixToMatrix4(const vr::HmdMatrix34_t &mat, GLfloat *fMat)
{
  float (*dst)[4] = (float(*)[4])fMat;
  dst[0][0] = mat.m[0][0]; dst[0][1] = mat.m[1][0]; dst[0][2] = mat.m[2][0]; dst[0][3] = 0.0f;
  dst[1][0] = mat.m[0][1]; dst[1][1] = mat.m[1][1]; dst[1][2] = mat.m[2][1]; dst[1][3] = 0.0f;
  dst[2][0] = mat.m[0][2]; dst[2][1] = mat.m[1][2]; dst[2][2] = mat.m[2][2]; dst[2][3] = 0.0f;
  dst[3][0] = mat.m[0][3]; dst[3][1] = mat.m[1][3]; dst[3][2] = mat.m[3][2]; dst[3][3] = 1.0f;
}

void OpenVRDrawControllers(PyMOLGlobals * G, float Front, float Back)
{
  COpenVR *I = G->OpenVR;
  if(!OpenVRReady(G))
    return;

  for (int i = HLeft; i <= HRight; ++i) {
    // FIXME calc matrix here???
    I->Hands[i].Draw(G, OpenVRGetProjection(G, 0.1/*Front*/, Back), OpenVRGetHeadToEye(G), OpenVRGetHDMPose(G), I->HandsPose[i]);  
  }
}
