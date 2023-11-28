# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2022 Zheng Lei (realthunder) <realthunder.dev@gmail.com>*
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************
'''Auto code generator for Mesh related parameters
'''
import sys
import cog
from os import sys, path

# import Tools/params_utils.py
sys.path.append(path.join(path.dirname(path.dirname(path.dirname(path.dirname(path.abspath(__file__))))), 'Tools'))
import params_utils

from params_utils import ParamBool, ParamInt, ParamString, ParamUInt,\
                         ParamFloat, ParamSpinBox, ParamColor, ParamHex,\
                         auto_comment, quote

NameSpace = 'Mesh'
ClassName = 'MeshParams'
ParamPath = 'User parameter:BaseApp/Preferences/Mod/Mesh'
ClassDoc = 'Convenient class to obtain Mesh related parameters'

Params = [
    ParamString('AsymptoteWidth', '500', on_change=True, subpath='Asymptote', param_name='Width'),
    ParamString('AsymptoteHeight', '500', on_change=True, subpath='Asymptote', param_name='Height'),
    ParamInt('DefaultShapeType', 0),
    ParamUInt('MeshColor', 0),
    ParamUInt('LineColor', 0),
    ParamInt('MeshTransparency', 0),
    ParamInt('LineTransparency', 0),
    ParamBool('TwoSideRendering', False),
    ParamBool('VertexPerNormals', False),
    ParamFloat('CreaseAngle', 0.0),
    ParamString('DisplayAliasFormatString', '%V = %A'),
    ParamBool('ShowBoundingBox', False),
    ParamFloat('MaxDeviationExport', 0.1),
    ParamInt('RenderTriangleLimit', -1),
    ParamBool("CheckNonManifoldPoints", False, subpath='Evaluation'),
    ParamBool("EnableFoldsCheck", False, subpath='Evaluation'),
    ParamBool("StrictlyDegenerated", True, subpath='Evaluation'),
    ParamBool("SubElementSelection", False),
]

def declare():
    params_utils.declare_begin(sys.modules[__name__])
    params_utils.declare_end(sys.modules[__name__])

def define():
    params_utils.define(sys.modules[__name__])
