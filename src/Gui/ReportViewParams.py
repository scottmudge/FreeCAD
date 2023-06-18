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
'''Auto code generator for parameters in Preferences/OutputWindow
'''
import sys
from os import sys, path

# import Tools/params_utils.py
sys.path.append(path.join(path.dirname(path.dirname(path.abspath(__file__))), 'Tools'))
import params_utils

from params_utils import ParamBool, ParamInt, ParamString, ParamQString, ParamUInt, ParamFloat

NameSpace = 'Gui'
ClassName = 'ReportViewParams'
ParamPath = 'User parameter:BaseApp/Preferences/OutputWindow'
ClassDoc = 'Convenient class to obtain ReportView related parameters'
Signal = True

Params = [
    ParamBool('checkShowReportViewOnWarning', True),
    ParamBool('checkShowReportViewOnError', True),
    ParamBool('checkShowReportViewOnNormalMessage', False),
    ParamBool('checkShowReportViewOnLogMessage', False),
    ParamBool('checkShowReportViewOnCritical', False),
    ParamBool("checkShowReportTimecode", True),

    ParamInt("LogMessageSize", 0),
    ParamQString('CommandRedirect', '',
        doc='Prefix for marking python command in message to be redirected to Python console\n'
            'This is used as a debug help for output command from external libraries'),
]

def declare():
    params_utils.declare_begin(sys.modules[__name__])
    params_utils.declare_end(sys.modules[__name__])

def define():
    params_utils.define(sys.modules[__name__])
