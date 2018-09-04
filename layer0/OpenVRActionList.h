#ifndef _H_OpenVRActionList
#define _H_OpenVRActionList

#include <vector>
#include "OpenVRAction.h"

struct OpenVRActionList {
  typedef std::vector<OpenVRAction*> ListType;
  ListType all;

  vr::VRActionSetHandle_t DefaultSet;

  OpenVRAction* LeftHand;
  OpenVRAction* RightHand;
  OpenVRAction* ToggleMenu;
  OpenVRAction* LClick;
  OpenVRAction* PadCenter;
  OpenVRAction* PadEast;
  OpenVRAction* PadWest;
  OpenVRAction* PadNorth;
  OpenVRAction* PadSouth;

  explicit OpenVRActionList(vr::IVRInput* Input) {
    Input->GetActionSetHandle("/actions/pymol", &DefaultSet);

#define OPENVR_ADD_ACTION(set, Action, ...) all.push_back(Action = new OpenVRAction(Input, "/actions/" set "/in/" #Action, ##__VA_ARGS__));
    OPENVR_ADD_ACTION("pymol", LeftHand, OpenVRAction::TYPE_POSE);
    OPENVR_ADD_ACTION("pymol", RightHand, OpenVRAction::TYPE_POSE);
    OPENVR_ADD_ACTION("pymol", ToggleMenu);
    OPENVR_ADD_ACTION("pymol", LClick);
    OPENVR_ADD_ACTION("pymol", PadCenter);
    OPENVR_ADD_ACTION("pymol", PadEast);
    OPENVR_ADD_ACTION("pymol", PadWest);
    OPENVR_ADD_ACTION("pymol", PadNorth);
    OPENVR_ADD_ACTION("pymol", PadSouth);
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
