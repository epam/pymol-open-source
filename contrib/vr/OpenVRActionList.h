#ifndef _H_OpenVRActionList
#define _H_OpenVRActionList

// system headers
#include <vector>

// local headers
#include "OpenVRAction.h"

struct OpenVRActionList {
  typedef std::vector<OpenVRAction*> ListType;
  ListType all;

  vr::VRActionSetHandle_t DefaultSet;

  OpenVRAction* LeftHand;
  OpenVRAction* RightHand;
  OpenVRAction* LGrip;
  OpenVRAction* RGrip;
  OpenVRAction* ToggleMenu;
  OpenVRAction* Laser;
  OpenVRAction* ActionSetNext;
  OpenVRAction* ActionSetPrev;
  OpenVRAction* Action1;
  OpenVRAction* Action2;
  OpenVRAction* Action3;

  explicit OpenVRActionList(vr::IVRInput* Input) {
    Input->GetActionSetHandle("/actions/pymol", &DefaultSet);

#define OPENVR_ADD_ACTION(set, Action, ...) all.push_back(Action = new OpenVRAction(Input, "/actions/" set "/in/" #Action, ##__VA_ARGS__));
    OPENVR_ADD_ACTION("pymol", LeftHand, OpenVRAction::TYPE_POSE);
    OPENVR_ADD_ACTION("pymol", RightHand, OpenVRAction::TYPE_POSE);
    OPENVR_ADD_ACTION("pymol", LGrip);
    OPENVR_ADD_ACTION("pymol", RGrip);
    OPENVR_ADD_ACTION("pymol", ToggleMenu);
    OPENVR_ADD_ACTION("pymol", Laser);
    OPENVR_ADD_ACTION("pymol", ActionSetNext);
    OPENVR_ADD_ACTION("pymol", ActionSetPrev);
    OPENVR_ADD_ACTION("pymol", Action1);
    OPENVR_ADD_ACTION("pymol", Action2);
    OPENVR_ADD_ACTION("pymol", Action3);
#undef OPENVR_ADD_ACTION
  }

  void Update(vr::IVRInput* Input) {
    vr::VRActiveActionSet_t activeSet = {DefaultSet};
    Input->UpdateActionState(&activeSet, sizeof(activeSet), 1);

    for (ListType::iterator i = all.begin(), iend = all.end(); i != iend; ++i) {
      (*i)->Update(Input);
    }
  }
};

#endif // _H_OpenVRActionList
