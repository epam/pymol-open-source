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
        self.panel = [
            [1, 'OpenVR Menu', ''],
            [3, 'Scene', 'scene'],
            [3, 'Wizard', 'wizard'],
        ]
