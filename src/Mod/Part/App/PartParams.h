/****************************************************************************
 *   Copyright (c) 2020 Zheng Lei (realthunder) <realthunder.dev@gmail.com> *
 *                                                                          *
 *   This file is part of the FreeCAD CAx development system.               *
 *                                                                          *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Library General Public            *
 *   License as published by the Free Software Foundation; either           *
 *   version 2 of the License, or (at your option) any later version.       *
 *                                                                          *
 *   This library  is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Library General Public License for more details.                   *
 *                                                                          *
 *   You should have received a copy of the GNU Library General Public      *
 *   License along with this library; see the file COPYING.LIB. If not,     *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,          *
 *   Suite 330, Boston, MA  02111-1307, USA                                 *
 *                                                                          *
 ****************************************************************************/

#ifndef PART_APP_PARAMS_H
#define PART_APP_PARAMS_H

#include <Base/Parameter.h>
#include <App/DynamicProperty.h>

namespace Part {

/** Convenient class to obtain Mod/Part related parameters 
 *
 * The parameters are under group "User parameter:BaseApp/Preferences/Mod/Part"
 *
 * To add a new parameter, add a new line under FC_APP_PART_PARAMS using macro
 *
 * @code
 *      FC_APP_PART_PARAM(parameter_name, c_type, parameter_type, default_value)
 * @endcode
 *
 * If there is special handling on parameter change, use FC_APP_PART_PARAM2()
 * instead, and add a function with the following signature in PartParams.cpp,
 *
 * @code
 *      void PartParams:on<ParamName>Changed()
 * @endcode
 */
class PartExport PartParams: public ParameterGrp::ObserverType
{
public:
    PartParams();
    virtual ~PartParams();
    void OnChange(Base::Subject<const char*> &, const char* sReason);
    static PartParams *instance();

    ParameterGrp::handle getHandle() {
        return handle;
    }

#define FC_APP_PART_PARAMS \
    FC_APP_PART_PARAM(ShapePropertyCopy, bool, Bool, false) \
    FC_APP_PART_PARAM(DisableShapeCache, bool, Bool, false) \
    FC_APP_PART_PARAM(CommandOverride, int, Int, 2) \
    FC_APP_PART_PARAM(EnableWrapFeature, int, Int, 2) \
    FC_APP_PART_PARAM(CopySubShape, bool, Bool, false) \
    FC_APP_PART_PARAM(UseBrepToolsOuterWire, bool, Bool, true) \
    FC_APP_PART_PARAM(UseBaseObjectName,bool,Bool,false) \
    FC_APP_PART_PARAM(AutoGroupSolids,bool,Bool,false) \
    FC_APP_PART_PARAM(AutoAuxiliaryGrouping,bool,Bool,false) \
    FC_APP_PART_PARAM(SingleSolid,bool,Bool,false) \
    FC_APP_PART_PARAM(UsePipeForExtrusionDraft,bool,Bool,false) \
    FC_APP_PART_PARAM(LinearizeExtrusionDraft,bool,Bool,true) \
    FC_APP_PART_PARAM(AutoCorrectLink,bool,Bool,false) \
    FC_APP_PART_PARAM(RefineModel,bool,Bool,false) \
    FC_APP_PART_PARAM(AuxGroupUniqueLabel,bool,Bool,false) \

#undef FC_APP_PART_PARAM
#define FC_APP_PART_PARAM(_name,_ctype,_type,_def) \
    static const _ctype & _name() { return instance()->_##_name; }\
    static void set_##_name(_ctype _v) { instance()->handle->Set##_type(#_name,_v); instance()->_##_name=_v; }\
    static void update##_name(PartParams *self) { self->_##_name = self->handle->Get##_type(#_name,_def); }\

#undef FC_APP_PART_PARAM2
#define FC_APP_PART_PARAM2(_name,_ctype,_type,_def) \
    static const _ctype & _name() { return instance()->_##_name; }\
    static void set_##_name(_ctype _v) { instance()->handle->Set##_type(#_name,_v); instance()->_##_name=_v; }\
    void on##_name##Changed();\
    static void update##_name(PartParams *self) { \
        auto _v = self->handle->Get##_type(#_name,_def); \
        if (self->_##_name != _v) {\
            self->_##_name = _v;\
            self->on##_name##Changed();\
        }\
    }\

    FC_APP_PART_PARAMS

private:
#undef FC_APP_PART_PARAM
#define FC_APP_PART_PARAM(_name,_ctype,_type,_def) \
    _ctype _##_name;

#undef FC_APP_PART_PARAM2
#define FC_APP_PART_PARAM2 FC_APP_PART_PARAM

    FC_APP_PART_PARAMS
    ParameterGrp::handle handle;
    std::unordered_map<const char *,void(*)(PartParams*),App::CStringHasher,App::CStringHasher> funcs;
};

#undef FC_APP_PART_PARAM
#undef FC_APP_PART_PARAM2

} // namespace Part

#endif // PART_APP_PARAMS_H
