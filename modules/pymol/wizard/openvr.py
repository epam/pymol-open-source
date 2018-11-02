from pymol.wizard import Wizard
from pymol import cmd
import pymol

class Openvr(Wizard):

    def __init__(self,_self=cmd):
        Wizard.__init__(self,_self)
        self.menu['scene'] = [
            [2, 'Scene Menu', ''],
            [1, 'Next', 'scene action=next'],
            [1, 'Previous', 'scene action=previous'],
        ]
        self.menu['wizard'] = [
            [2, 'Wizard Menu', ''],
            [1, 'Measurement', 'wizard measurement'],
            [1, 'Mutagenesis', 'wizard mutagenesis'],
            [1, 'Density', 'wizard density'],
        ]
        self.menu['gui'] = [
            [2, 'VR GUI Presets', ''],
            [1, 'Old Defaults',
                'set openvr_gui_use_alpha,0;' +
                'set openvr_gui_scene_color,0;' +
                'set openvr_gui_scene_alpha,0.75;' +
                'set openvr_gui_use_backdrop,0;' +
                'set openvr_gui_overlay,0'],
            [1, 'Spatial Opaque',
                'set openvr_gui_use_alpha,0;' +
                'set openvr_gui_scene_color,0;' +
                'set openvr_gui_scene_alpha,1;' +
                'set openvr_gui_use_backdrop,0;' +
                'set openvr_gui_overlay,0'],
            [1, 'Spatial Semi-Transparent',
                'set openvr_gui_use_alpha,1;' +
                'set openvr_gui_alpha,0.85;' +
                'set openvr_gui_scene_color,0;' +
                'set openvr_gui_scene_alpha,1;' +
                'set openvr_gui_use_backdrop,0;' +
                'set openvr_gui_overlay,0'],
            [1, 'Spatial Transparent',
                'set openvr_gui_use_alpha,1;' +
                'set openvr_gui_alpha,0.5;' +
                'set openvr_gui_scene_color,0;' +
                'set openvr_gui_scene_alpha,0;' +
                'set openvr_gui_use_backdrop,0;' +
                'set openvr_gui_overlay,0'],
            [1, 'Overlay',
                'set openvr_gui_use_alpha,0;' +
                'set openvr_gui_scene_color,0;' +
                'set openvr_gui_scene_alpha,1;' +
                'set openvr_gui_back_color,0.5;' +
                'set openvr_gui_back_alpha,0.85;' +
                'set openvr_gui_use_backdrop,1;' +
                'set openvr_gui_overlay,1'],
            [1, 'Responsive Overlay',
                'set openvr_gui_use_alpha,2;' +
                'set openvr_gui_alpha,0.5;' +
                'set openvr_gui_scene_color,0;' +
                'set openvr_gui_scene_alpha,0.85;' +
                'set openvr_gui_back_color,0.125;' +
                'set openvr_gui_back_alpha,0.85;' +
                'set openvr_gui_use_backdrop,2;' +
                'set openvr_gui_overlay,2'],
            [1, 'Responsive Spatial',
                'set openvr_gui_use_alpha,2;' +
                'set openvr_gui_alpha,0.5;' +
                'set openvr_gui_scene_color,0;' +
                'set openvr_gui_scene_alpha,0.85;' +
                'set openvr_gui_back_color,0.125;' +
                'set openvr_gui_back_alpha,0.85;' +
                'set openvr_gui_use_backdrop,2;' +
                'set openvr_gui_overlay,0'],
        ]
        self.panel = [
            [1, 'OpenVR Menu', ''],
            [3, 'Scene', 'scene'],
            [3, 'Wizard', 'wizard'],
            [3, 'VR GUI', 'gui'],
        ]
