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
#include "OpenVRUtils.h"
#include "OpenVRStub.h"
#include "OpenVRController.h"
#include "OpenVRMenu.h"
#include "OpenVRActionList.h"
#include "OpenVRScenePicker.h"
#include "OpenVRLaserTarget.h"
#include "PyMOLOptions.h"
#include "Setting.h"
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
  // Such structures used to be calloc-ed, this replicates that
  void *operator new(size_t size) {
    void *mem = ::operator new(size);
    memset(mem, 0, size);
    return mem;
  }

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
  GLuint ControllerHintsTexture;

  bool ForcedFront;
  bool ForcedBack;
 
  OpenVRMenu Menu;
  OpenVRScenePicker Picker;

  OpenVRInputHandlers* Handlers;

  OpenVRActionList* Actions;

  // mouse cursor imitation information
  int startX, startY;
  int deltaX, deltaY;

  float moleculeToWorldMatrix[16];
  float moleculeToCapturingController[16];
  float controllersCenterToWorld[16];
  int capturingHandIdx;
  float controllersDistance;
};

static const float OPEN_VR_FRONT = 0.1f;
static const float OPEN_VR_BACK = 10.0f;

static char const* deviceClassNames[] = {
  "Invalid",
  "Head-Mounted Display",
  "Controller",
  "Generic Tracker",
  "Reference Point",
  "Accessory",
};
static const int deviceClassNamesCount = sizeof(deviceClassNames) / sizeof(*deviceClassNames);

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

  OpenVRSetInputHandlers(G, new OpenVRInputHandlers());

  I->Input = vr::stub::VRInput(); 
  if (I->Input) {
    // init manifest
    std::string manifestPath = std::string(getenv("PYMOL_PATH")) + "\\data\\openvr\\actions.json";
    I->Input->SetActionManifestPath(manifestPath.c_str());

    I->Actions = new OpenVRActionList(I->Input);
  }

  I->capturingHandIdx = -1;
  
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

    I->Picker.Free();
    I->Menu.Free();
    delete I->Handlers;

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

    I->ControllerHintsTexture = OpenVRUtils::LoadTexture("hints_vive_controller.png");

    I->Menu.Init(I->Handlers);
    I->Picker.Init(I->Handlers);
  }

  for (int i = HLeft; i <= HRight; ++i) {
    OpenVRController &hand = I->Hands[i];
    if (!hand.IsInitialized()) {
      hand.Init();
    }
  }
}

void OpenVRSetInputHandlers(PyMOLGlobals * G, OpenVRInputHandlers* handlers)
{
  COpenVR *I = G->OpenVR;
  if (I) {
    if (I->Handlers)
      delete I->Handlers;
    I->Handlers = handlers;
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

void OpenVRSceneFinish(PyMOLGlobals * G, unsigned sceneX, unsigned sceneY, unsigned sceneWidth, unsigned sceneHeight)
{
  COpenVR *I = G->OpenVR;
  if(!OpenVRReady(G))
    return;

  GL_DEBUG_FUN();

  // find a proper rectangle with the scene aspect ratio
  unsigned width = I->Height * sceneWidth / sceneHeight;
  unsigned height = I->Width * sceneHeight / sceneWidth;
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
  glBlitFramebufferEXT(dx, dy, dx + width, dy + height, sceneX, sceneY, sceneX + sceneWidth, sceneY + sceneHeight, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);
  glBindFramebufferEXT(GL_READ_FRAMEBUFFER, 0);
}

void OpenVRFrameFinish(PyMOLGlobals * G)
{
  COpenVR *I = G->OpenVR;
  if(!OpenVRReady(G))
    return;

  GL_DEBUG_FUN();

  // send rendered pictures into the headset
  I->Compositor->Submit(vr::Eye_Left, &I->Left.Texture);
  I->Compositor->Submit(vr::Eye_Right, &I->Right.Texture);
}

void OpenVRGetWidthHeight(PyMOLGlobals * G, int* width, int* height)
{
  COpenVR *I = G->OpenVR;
  if (I) {
    *width = I->Width;
    *height = I->Height;
  }
}

void OpenVRMenuBufferStart(PyMOLGlobals * G, unsigned width, unsigned height, bool clear /* = false */)
{
  COpenVR *I = G->OpenVR;
  if(!OpenVRReady(G))
    return;

  I->Menu.SetSceneAlpha(SettingGetGlobal_f(G, cSetting_openvr_gui_scene_alpha));
  I->Menu.Start(width, height, clear);
}

void OpenVRMenuBufferFinish(PyMOLGlobals * G)
{
  COpenVR *I = G->OpenVR;
  if(!OpenVRReady(G))
    return;

  I->Menu.Finish();
}

void OpenVRMenuToggle(PyMOLGlobals * G, unsigned deviceIndex /* = ~0U */)
{
  COpenVR *I = G->OpenVR;
  if(!OpenVRReady(G))
    return;

  if (!I->Menu.IsVisible()) {
    I->Menu.Show(I->HeadPose, deviceIndex);
    I->ForcedBack = true;
  } else {
    unsigned ownerIndex = I->Menu.GetOwnerID();
    if (deviceIndex == ownerIndex || deviceIndex == ~0U || ownerIndex == ~0U) {
      I->Menu.Hide();
      I->ForcedBack = false;
    }
  }
}

void OpenVRMenuCrop(PyMOLGlobals * G, unsigned x, unsigned y, unsigned width, unsigned height)
{
  COpenVR *I = G->OpenVR;
  if(!OpenVRReady(G))
    return;

  I->Menu.Crop(x, y, width, height);
}


float* OpenVRGetHeadToEye(PyMOLGlobals * G)
{
  COpenVR *I = G->OpenVR;
  if(!OpenVRReady(G) || !I->Eye)
    return NULL;

  CEye *E = I->Eye;
  vr::HmdMatrix34_t EyeToHeadTransform = I->System->GetEyeToHeadTransform(E->Eye);
  OpenVRUtils::MatrixFastInverseVRGL((const float *)EyeToHeadTransform.m, E->HeadToEyeMatrix);
  
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

  if (I->ForcedFront) {
    near_plane = OPEN_VR_FRONT;
  }
  if (I->ForcedBack) {
    far_plane = OPEN_VR_BACK;
  }
 
  far_plane = 100.0f;

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

float const* OpenVRGetPickingMatrix(PyMOLGlobals * G)
{
  COpenVR *I = G->OpenVR;
  if(!OpenVRReady(G))
    return NULL;

  return I->Picker.GetMatrix();
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

void ProcessButtonDragAsMouse(PyMOLGlobals * G, OpenVRAction *action, int glutButton, int screenCenterX, int screenCenterY) {
  COpenVR *I = G->OpenVR;
  if (!action || !I) return;

  OpenVRInputHandlers* Handlers = I->Handlers;
  if (!Handlers)
   return;

  // imitate mouse cursor position from controller camera position
  float (*mat)[4] = (float (*)[4])I->Hands[HRight].GetPose();
  int x = (int)(mat[3][0]* 500.0f), y = (int)(mat[3][1] * 500.0f); // magic factors

  bool nowPressed = action->IsPressed();
  if (action->WasPressedOrReleased()) {
    if (nowPressed) {
      I->startX = x;
      I->startY = y;
      Handlers->MouseFunc(glutButton, P_GLUT_DOWN, screenCenterX, screenCenterY, 0);
    } else {
      Handlers->MouseFunc(glutButton, P_GLUT_UP, I->deltaX + screenCenterX, I->deltaY + screenCenterY, 0);
    }
  }
  if (nowPressed) {
    I->deltaX = x - I->startX;
    I->deltaY = y - I->startY;
    Handlers->MotionFunc(I->deltaX + screenCenterX, I->deltaY + screenCenterY, 0);
  }
}

bool OpenVRIsMoleculeCaptured(PyMOLGlobals * G) {
  COpenVR *I = G->OpenVR;
  OpenVRActionList* Actions = I->Actions;
  return Actions->LGrip->IsPressed() || Actions->RGrip->IsPressed();
}

void CalculateScalingPivotToWorldMatrix(PyMOLGlobals * G, float *pivotToWorldMatrix) {
  COpenVR *I = G->OpenVR;
  if (!I || !pivotToWorldMatrix)
    return;

  identity44f(pivotToWorldMatrix);
  float *lPose = I->Hands[HLeft].GetPose();
  float *rPose = I->Hands[HRight].GetPose();
  // get center traslation matrix
  average3f(&lPose[12], &rPose[12], &pivotToWorldMatrix[12]); 
}

float const *OpenVRGetMolecule2WorldMatrix(PyMOLGlobals * G, float *scaler) {
  COpenVR *I = G->OpenVR;
  if (!I)
    return NULL;

  float temp[16];
  OpenVRActionList* Actions = I->Actions;
  if (Actions->LGrip->IsPressed() && Actions->RGrip->IsPressed()) {
    // translate after the scaling pivot
    float pivotToWorldMatrix[16];
    CalculateScalingPivotToWorldMatrix(G, pivotToWorldMatrix);
    memcpy(temp, pivotToWorldMatrix, sizeof(temp));
    // scale due to changing distance between controllers
    if (scaler) {
      float newDistance = inline_diff3f(&(I->Hands[HLeft].GetPose()[12]), &(I->Hands[HRight].GetPose()[12]));
      *scaler = newDistance / I->controllersDistance;
      I->controllersDistance = newDistance;
    }
  } else {
    memcpy(temp, I->Hands[I->capturingHandIdx].GetPose(), sizeof(temp));
    *scaler = 1.0f;
  }
  MatrixMultiplyC44f(I->moleculeToCapturingController, temp);
  memcpy(I->moleculeToWorldMatrix, temp, sizeof(I->moleculeToWorldMatrix));
  return I->moleculeToWorldMatrix;
}

void AttachMoleculeToController(PyMOLGlobals * G, int handIdx) {
  COpenVR *I = G->OpenVR;
  I->capturingHandIdx = handIdx;
  // catch up
  memcpy(I->moleculeToCapturingController, I->Hands[handIdx].GetWorldToControllerMatrix(), sizeof(I->moleculeToCapturingController));
  MatrixMultiplyC44f(I->moleculeToWorldMatrix, I->moleculeToCapturingController);
}

// FIXME generalize attach functions
void AttachMoleculeToСenter(PyMOLGlobals * G) {
  COpenVR *I = G->OpenVR;

  // get scaling pivot center = controllers mass center
  float worldToPivotMatrix[16];
  CalculateScalingPivotToWorldMatrix(G, worldToPivotMatrix);
  // inverse transform to be exactly WorldToPivot
  worldToPivotMatrix[12] *= -1.0;
  worldToPivotMatrix[13] *= -1.0;
  worldToPivotMatrix[14] *= -1.0;
  // attach molecule to pivot
  memcpy(I->moleculeToCapturingController, worldToPivotMatrix, sizeof(I->moleculeToCapturingController));
  MatrixMultiplyC44f(I->moleculeToWorldMatrix, I->moleculeToCapturingController);
  // save distance between controllers
  I->controllersDistance = inline_diff3f(&(I->Hands[HLeft].GetPose()[12]), &(I->Hands[HRight].GetPose()[12]));
}

void OpenVRHandleInput(PyMOLGlobals * G, int SceneX, int SceneY, int SceneWidth, int SceneHeight, float *model2World)
{
  COpenVR *I = G->OpenVR;
  if(!OpenVRReady(G))
    return;

  vr::VREvent_t event;
  while (I->System->PollNextEvent(&event, sizeof(event)))
    /* pass */;

  if (!I->Input)
    return;

  OpenVRActionList* Actions = I->Actions;
  Actions->Update(I->Input);

  OpenVRController& LeftHand = I->Hands[HLeft];
  OpenVRController& RightHand = I->Hands[HRight];
  int centerX = SceneX + SceneWidth / 2;
  int centerY = SceneY + SceneHeight / 2;

  // update VR GUI state
  if (Actions->ToggleMenu->WasPressed()) {
    OpenVRMenuToggle(G, Actions->ToggleMenu->DeviceIndex());
  }

  // update picking state
  if (Actions->LaserShoot->IsPressed()) {
    I->Picker.Activate(Actions->LaserShoot->DeviceIndex(), centerX, centerY);
  } else {
    I->Picker.Deactivate();
  }

  // update controllers visibility
  LeftHand.Show(Actions->LeftHand->PoseValid());
  RightHand.Show(Actions->RightHand->PoseValid());

  if (!I->Menu.IsVisible()) {

    // process in-app actions
    if (Actions->PadCenter->WasPressed())
      I->Handlers->ActionFunc(cAction_movie_toggle);
    if (Actions->PadEast->WasPressed())
      I->Handlers->ActionFunc(cAction_movie_next);
    if (Actions->PadWest->WasPressed())
      I->Handlers->ActionFunc(cAction_movie_prev);
    if (Actions->PadNorth->WasPressed())
      I->Handlers->ActionFunc(cAction_scene_prev);
    if (Actions->PadSouth->WasPressed())
      I->Handlers->ActionFunc(cAction_scene_next);

    ProcessButtonDragAsMouse(G, Actions->LMouse, P_GLUT_LEFT_BUTTON, centerX, centerY);
    ProcessButtonDragAsMouse(G, Actions->MMouse, P_GLUT_MIDDLE_BUTTON, centerX, centerY);
    ProcessButtonDragAsMouse(G, Actions->RMouse, P_GLUT_RIGHT_BUTTON, centerX, centerY);

    if (Actions->LGrip->IsPressed() || Actions->RGrip->IsPressed()) {
      memcpy(I->moleculeToWorldMatrix, model2World, sizeof(I->moleculeToWorldMatrix)); 
    }
    if (Actions->LGrip->WasPressed() && !Actions->RGrip->IsPressed()) {
      AttachMoleculeToController(G, HLeft);
    }
    if (Actions->RGrip->WasPressed() && !Actions->LGrip->IsPressed()) {
      AttachMoleculeToController(G, HRight);
    }
    // TODO make it being less ugly
    if ((Actions->RGrip->WasPressed() && Actions->LGrip->IsPressed()) ||
       (Actions->LGrip->WasPressed() && Actions->RGrip->IsPressed())) {
      AttachMoleculeToСenter(G);
    }
    if (Actions->LGrip->WasReleased() && Actions->RGrip->IsPressed()) {
      AttachMoleculeToController(G, HRight);
    }
    if (Actions->RGrip->WasReleased() && Actions->LGrip->IsPressed()) {
      AttachMoleculeToController(G, HLeft);
    }
    if ((Actions->LGrip->WasReleased() && !Actions->RGrip->IsPressed()) ||
        (Actions->RGrip->WasReleased() && !Actions->LGrip->IsPressed())) {
      I->capturingHandIdx = -1;
    }

  } else {

    // process GUI actions

  }

  // process laser

  OpenVRLaserTarget* laserTarget = 0;
  float const (*laserColors)[4] = 0;
  if (I->Menu.IsVisible()) {
    laserTarget = &I->Menu;
    laserColors = OpenVRMenu::GetLaserColors();
  } else if (I->Picker.IsActive()) {
    laserTarget = &I->Picker;
    laserColors = OpenVRScenePicker::GetLaserColors();
  }

  for (size_t laserIndex = 0; laserIndex < sizeof(I->Hands) / sizeof(I->Hands[0]); ++laserIndex) {
    OpenVRLaserSource* laserSource = &I->Hands[laserIndex];
    laserSource->LaserShow(laserTarget && laserTarget->IsLaserAllowed(laserSource->GetLaserDeviceIndex()));

    float origin[3], dir[3];
    if (!laserSource->GetLaserRay(origin, dir))
      continue;

    float distance = 0.0f;
    bool hit = laserTarget->LaserShoot(origin, dir, laserColors[1], &distance);
    laserSource->SetLaserLength(distance);
    laserSource->SetLaserColor(laserColors[int(hit)]);

    if (Actions->LClick->WasPressedOrReleased() && Actions->LClick->DeviceIndex() == laserSource->GetLaserDeviceIndex()) {
      laserTarget->LaserClick(Actions->LClick->IsPressed());
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
          OpenVRUtils::MatrixCopyVRGL((const float *)pose.mDeviceToAbsoluteTracking.m, I->HeadPose);
          OpenVRUtils::MatrixFastInverseVRGL((const float *)pose.mDeviceToAbsoluteTracking.m, I->WorldToHeadMatrix);
          break;
        case vr::TrackedDeviceClass_Controller:
          {
            vr::ETrackedControllerRole role = I->System->GetControllerRoleForTrackedDeviceIndex(nDevice);

            OpenVRController* hand = 0;
            if (role == vr::TrackedControllerRole_LeftHand) {
              hand = &I->Hands[HLeft];
            } else if (role == vr::TrackedControllerRole_RightHand) {
              hand = &I->Hands[HRight];
            }

            if (hand) {
              OpenVRUtils::MatrixCopyVRGL((const float *)pose.mDeviceToAbsoluteTracking.m, (float *)hand->GetPose());
              OpenVRUtils::MatrixFastInverseVRGL((const float *)pose.mDeviceToAbsoluteTracking.m, (float *)hand->GetWorldToControllerMatrix());
              hand->m_deviceIndex = nDevice;
              std::string sRenderModelName = GetTrackedDeviceString(G, nDevice, vr::Prop_RenderModelName_String);
              if (sRenderModelName != hand->m_sRenderModelName) {
                hand->m_pRenderModel = FindOrLoadRenderModel(G, sRenderModelName.c_str());
                hand->m_pRenderModel->SetHintsTexture(I->ControllerHintsTexture, 2);
                hand->m_sRenderModelName = sRenderModelName;
              }
            }
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
    I->Hands[i].Draw();
  }

  glPopMatrix();
}
