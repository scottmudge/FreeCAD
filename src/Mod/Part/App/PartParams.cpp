/****************************************************************************
 *   Copyright (c) 2022 Zheng Lei (realthunder) <realthunder.dev@gmail.com> *
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

#include "PreCompiled.h"

/*[[[cog
import PartParams
PartParams.define()
]]]*/

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:197)
#include <unordered_map>
#include <App/Application.h>
#include <App/DynamicProperty.h>
#include "PartParams.h"
using namespace Part;

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:208)
namespace {
class PartParamsP: public ParameterGrp::ObserverType {
public:
    ParameterGrp::handle handle;
    std::unordered_map<const char *,void(*)(PartParamsP*),App::CStringHasher,App::CStringHasher> funcs;
    bool ShapePropertyCopy;
    bool DisableShapeCache;
    long CommandOverride;
    long EnableWrapFeature;
    bool CopySubShape;
    bool UseBrepToolsOuterWire;
    bool UseBaseObjectName;
    bool AutoGroupSolids;
    bool SingleSolid;
    bool UsePipeForExtrusionDraft;
    bool LinearizeExtrusionDraft;
    bool AutoCorrectLink;
    bool RefineModel;
    bool AuxGroupUniqueLabel;
    bool AutoAuxGrouping;
    bool EnforcePrecision;
    long OpsPrecisionLevel;
    bool AutoHideOrigins;
    bool SplitEllipsoid;
    long ParallelRunThreshold;
    bool AutoValidateShape;
    bool FixShape;
    unsigned long LoftMaxDegree;
    double MinimumDeviation;
    double MeshDeviation;
    double MeshAngularDeflection;
    double MinimumAngularDeflection;

    // Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:252)
    PartParamsP() {
        handle = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Part");
        handle->Attach(this);

        ShapePropertyCopy = this->handle->GetBool("ShapePropertyCopy", false);
        funcs["ShapePropertyCopy"] = &PartParamsP::updateShapePropertyCopy;
        DisableShapeCache = this->handle->GetBool("DisableShapeCache", false);
        funcs["DisableShapeCache"] = &PartParamsP::updateDisableShapeCache;
        CommandOverride = this->handle->GetInt("CommandOverride", 2);
        funcs["CommandOverride"] = &PartParamsP::updateCommandOverride;
        EnableWrapFeature = this->handle->GetInt("EnableWrapFeature", 2);
        funcs["EnableWrapFeature"] = &PartParamsP::updateEnableWrapFeature;
        CopySubShape = this->handle->GetBool("CopySubShape", false);
        funcs["CopySubShape"] = &PartParamsP::updateCopySubShape;
        UseBrepToolsOuterWire = this->handle->GetBool("UseBrepToolsOuterWire", true);
        funcs["UseBrepToolsOuterWire"] = &PartParamsP::updateUseBrepToolsOuterWire;
        UseBaseObjectName = this->handle->GetBool("UseBaseObjectName", false);
        funcs["UseBaseObjectName"] = &PartParamsP::updateUseBaseObjectName;
        AutoGroupSolids = this->handle->GetBool("AutoGroupSolids", false);
        funcs["AutoGroupSolids"] = &PartParamsP::updateAutoGroupSolids;
        SingleSolid = this->handle->GetBool("SingleSolid", false);
        funcs["SingleSolid"] = &PartParamsP::updateSingleSolid;
        UsePipeForExtrusionDraft = this->handle->GetBool("UsePipeForExtrusionDraft", false);
        funcs["UsePipeForExtrusionDraft"] = &PartParamsP::updateUsePipeForExtrusionDraft;
        LinearizeExtrusionDraft = this->handle->GetBool("LinearizeExtrusionDraft", true);
        funcs["LinearizeExtrusionDraft"] = &PartParamsP::updateLinearizeExtrusionDraft;
        AutoCorrectLink = this->handle->GetBool("AutoCorrectLink", false);
        funcs["AutoCorrectLink"] = &PartParamsP::updateAutoCorrectLink;
        RefineModel = this->handle->GetBool("RefineModel", false);
        funcs["RefineModel"] = &PartParamsP::updateRefineModel;
        AuxGroupUniqueLabel = this->handle->GetBool("AuxGroupUniqueLabel", false);
        funcs["AuxGroupUniqueLabel"] = &PartParamsP::updateAuxGroupUniqueLabel;
        AutoAuxGrouping = this->handle->GetBool("AutoAuxGrouping", true);
        funcs["AutoAuxGrouping"] = &PartParamsP::updateAutoAuxGrouping;
        EnforcePrecision = this->handle->GetBool("EnforcePrecision", true);
        funcs["EnforcePrecision"] = &PartParamsP::updateEnforcePrecision;
        OpsPrecisionLevel = this->handle->GetInt("OpsPrecisionLevel", 9);
        funcs["OpsPrecisionLevel"] = &PartParamsP::updateOpsPrecisionLevel;
        AutoHideOrigins = this->handle->GetBool("AutoHideOrigins", true);
        funcs["AutoHideOrigins"] = &PartParamsP::updateAutoHideOrigins;
        SplitEllipsoid = this->handle->GetBool("SplitEllipsoid", true);
        funcs["SplitEllipsoid"] = &PartParamsP::updateSplitEllipsoid;
        ParallelRunThreshold = this->handle->GetInt("ParallelRunThreshold", 100);
        funcs["ParallelRunThreshold"] = &PartParamsP::updateParallelRunThreshold;
        AutoValidateShape = this->handle->GetBool("AutoValidateShape", false);
        funcs["AutoValidateShape"] = &PartParamsP::updateAutoValidateShape;
        FixShape = this->handle->GetBool("FixShape", false);
        funcs["FixShape"] = &PartParamsP::updateFixShape;
        LoftMaxDegree = this->handle->GetUnsigned("LoftMaxDegree", 5);
        funcs["LoftMaxDegree"] = &PartParamsP::updateLoftMaxDegree;
        MinimumDeviation = this->handle->GetFloat("MinimumDeviation", 0.05);
        funcs["MinimumDeviation"] = &PartParamsP::updateMinimumDeviation;
        MeshDeviation = this->handle->GetFloat("MeshDeviation", 0.2);
        funcs["MeshDeviation"] = &PartParamsP::updateMeshDeviation;
        MeshAngularDeflection = this->handle->GetFloat("MeshAngularDeflection", 28.65);
        funcs["MeshAngularDeflection"] = &PartParamsP::updateMeshAngularDeflection;
        MinimumAngularDeflection = this->handle->GetFloat("MinimumAngularDeflection", 5.0);
        funcs["MinimumAngularDeflection"] = &PartParamsP::updateMinimumAngularDeflection;
    }

    // Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:282)
    ~PartParamsP() {
    }

    // Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:289)
    void OnChange(Base::Subject<const char*> &, const char* sReason) {
        if(!sReason)
            return;
        auto it = funcs.find(sReason);
        if(it == funcs.end())
            return;
        it->second(this);
        
    }


    // Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:307)
    static void updateShapePropertyCopy(PartParamsP *self) {
        self->ShapePropertyCopy = self->handle->GetBool("ShapePropertyCopy", false);
    }
    // Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:307)
    static void updateDisableShapeCache(PartParamsP *self) {
        self->DisableShapeCache = self->handle->GetBool("DisableShapeCache", false);
    }
    // Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:307)
    static void updateCommandOverride(PartParamsP *self) {
        self->CommandOverride = self->handle->GetInt("CommandOverride", 2);
    }
    // Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:307)
    static void updateEnableWrapFeature(PartParamsP *self) {
        self->EnableWrapFeature = self->handle->GetInt("EnableWrapFeature", 2);
    }
    // Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:307)
    static void updateCopySubShape(PartParamsP *self) {
        self->CopySubShape = self->handle->GetBool("CopySubShape", false);
    }
    // Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:307)
    static void updateUseBrepToolsOuterWire(PartParamsP *self) {
        self->UseBrepToolsOuterWire = self->handle->GetBool("UseBrepToolsOuterWire", true);
    }
    // Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:307)
    static void updateUseBaseObjectName(PartParamsP *self) {
        self->UseBaseObjectName = self->handle->GetBool("UseBaseObjectName", false);
    }
    // Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:307)
    static void updateAutoGroupSolids(PartParamsP *self) {
        self->AutoGroupSolids = self->handle->GetBool("AutoGroupSolids", false);
    }
    // Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:307)
    static void updateSingleSolid(PartParamsP *self) {
        self->SingleSolid = self->handle->GetBool("SingleSolid", false);
    }
    // Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:307)
    static void updateUsePipeForExtrusionDraft(PartParamsP *self) {
        self->UsePipeForExtrusionDraft = self->handle->GetBool("UsePipeForExtrusionDraft", false);
    }
    // Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:307)
    static void updateLinearizeExtrusionDraft(PartParamsP *self) {
        self->LinearizeExtrusionDraft = self->handle->GetBool("LinearizeExtrusionDraft", true);
    }
    // Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:307)
    static void updateAutoCorrectLink(PartParamsP *self) {
        self->AutoCorrectLink = self->handle->GetBool("AutoCorrectLink", false);
    }
    // Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:307)
    static void updateRefineModel(PartParamsP *self) {
        self->RefineModel = self->handle->GetBool("RefineModel", false);
    }
    // Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:307)
    static void updateAuxGroupUniqueLabel(PartParamsP *self) {
        self->AuxGroupUniqueLabel = self->handle->GetBool("AuxGroupUniqueLabel", false);
    }
    // Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:307)
    static void updateAutoAuxGrouping(PartParamsP *self) {
        self->AutoAuxGrouping = self->handle->GetBool("AutoAuxGrouping", true);
    }
    // Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:307)
    static void updateEnforcePrecision(PartParamsP *self) {
        self->EnforcePrecision = self->handle->GetBool("EnforcePrecision", true);
    }
    // Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:307)
    static void updateOpsPrecisionLevel(PartParamsP *self) {
        self->OpsPrecisionLevel = self->handle->GetInt("OpsPrecisionLevel", 9);
    }
    // Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:307)
    static void updateAutoHideOrigins(PartParamsP *self) {
        self->AutoHideOrigins = self->handle->GetBool("AutoHideOrigins", true);
    }
    // Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:307)
    static void updateSplitEllipsoid(PartParamsP *self) {
        self->SplitEllipsoid = self->handle->GetBool("SplitEllipsoid", true);
    }
    // Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:307)
    static void updateParallelRunThreshold(PartParamsP *self) {
        self->ParallelRunThreshold = self->handle->GetInt("ParallelRunThreshold", 100);
    }
    // Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:307)
    static void updateAutoValidateShape(PartParamsP *self) {
        self->AutoValidateShape = self->handle->GetBool("AutoValidateShape", false);
    }
    // Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:307)
    static void updateFixShape(PartParamsP *self) {
        self->FixShape = self->handle->GetBool("FixShape", false);
    }
    // Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:307)
    static void updateLoftMaxDegree(PartParamsP *self) {
        self->LoftMaxDegree = self->handle->GetUnsigned("LoftMaxDegree", 5);
    }
    // Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:307)
    static void updateMinimumDeviation(PartParamsP *self) {
        self->MinimumDeviation = self->handle->GetFloat("MinimumDeviation", 0.05);
    }
    // Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:307)
    static void updateMeshDeviation(PartParamsP *self) {
        self->MeshDeviation = self->handle->GetFloat("MeshDeviation", 0.2);
    }
    // Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:307)
    static void updateMeshAngularDeflection(PartParamsP *self) {
        self->MeshAngularDeflection = self->handle->GetFloat("MeshAngularDeflection", 28.65);
    }
    // Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:307)
    static void updateMinimumAngularDeflection(PartParamsP *self) {
        self->MinimumAngularDeflection = self->handle->GetFloat("MinimumAngularDeflection", 5.0);
    }
};

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:329)
PartParamsP *instance() {
    static PartParamsP *inst = new PartParamsP;
    return inst;
}

} // Anonymous namespace

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:340)
ParameterGrp::handle PartParams::getHandle() {
    return instance()->handle;
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:369)
const char *PartParams::docShapePropertyCopy() {
    return "";
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:377)
const bool & PartParams::getShapePropertyCopy() {
    return instance()->ShapePropertyCopy;
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:385)
const bool & PartParams::defaultShapePropertyCopy() {
    const static bool def = false;
    return def;
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:394)
void PartParams::setShapePropertyCopy(const bool &v) {
    instance()->handle->SetBool("ShapePropertyCopy",v);
    instance()->ShapePropertyCopy = v;
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:403)
void PartParams::removeShapePropertyCopy() {
    instance()->handle->RemoveBool("ShapePropertyCopy");
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:369)
const char *PartParams::docDisableShapeCache() {
    return "";
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:377)
const bool & PartParams::getDisableShapeCache() {
    return instance()->DisableShapeCache;
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:385)
const bool & PartParams::defaultDisableShapeCache() {
    const static bool def = false;
    return def;
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:394)
void PartParams::setDisableShapeCache(const bool &v) {
    instance()->handle->SetBool("DisableShapeCache",v);
    instance()->DisableShapeCache = v;
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:403)
void PartParams::removeDisableShapeCache() {
    instance()->handle->RemoveBool("DisableShapeCache");
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:369)
const char *PartParams::docCommandOverride() {
    return "";
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:377)
const long & PartParams::getCommandOverride() {
    return instance()->CommandOverride;
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:385)
const long & PartParams::defaultCommandOverride() {
    const static long def = 2;
    return def;
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:394)
void PartParams::setCommandOverride(const long &v) {
    instance()->handle->SetInt("CommandOverride",v);
    instance()->CommandOverride = v;
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:403)
void PartParams::removeCommandOverride() {
    instance()->handle->RemoveInt("CommandOverride");
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:369)
const char *PartParams::docEnableWrapFeature() {
    return "";
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:377)
const long & PartParams::getEnableWrapFeature() {
    return instance()->EnableWrapFeature;
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:385)
const long & PartParams::defaultEnableWrapFeature() {
    const static long def = 2;
    return def;
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:394)
void PartParams::setEnableWrapFeature(const long &v) {
    instance()->handle->SetInt("EnableWrapFeature",v);
    instance()->EnableWrapFeature = v;
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:403)
void PartParams::removeEnableWrapFeature() {
    instance()->handle->RemoveInt("EnableWrapFeature");
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:369)
const char *PartParams::docCopySubShape() {
    return "";
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:377)
const bool & PartParams::getCopySubShape() {
    return instance()->CopySubShape;
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:385)
const bool & PartParams::defaultCopySubShape() {
    const static bool def = false;
    return def;
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:394)
void PartParams::setCopySubShape(const bool &v) {
    instance()->handle->SetBool("CopySubShape",v);
    instance()->CopySubShape = v;
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:403)
void PartParams::removeCopySubShape() {
    instance()->handle->RemoveBool("CopySubShape");
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:369)
const char *PartParams::docUseBrepToolsOuterWire() {
    return "";
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:377)
const bool & PartParams::getUseBrepToolsOuterWire() {
    return instance()->UseBrepToolsOuterWire;
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:385)
const bool & PartParams::defaultUseBrepToolsOuterWire() {
    const static bool def = true;
    return def;
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:394)
void PartParams::setUseBrepToolsOuterWire(const bool &v) {
    instance()->handle->SetBool("UseBrepToolsOuterWire",v);
    instance()->UseBrepToolsOuterWire = v;
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:403)
void PartParams::removeUseBrepToolsOuterWire() {
    instance()->handle->RemoveBool("UseBrepToolsOuterWire");
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:369)
const char *PartParams::docUseBaseObjectName() {
    return "";
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:377)
const bool & PartParams::getUseBaseObjectName() {
    return instance()->UseBaseObjectName;
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:385)
const bool & PartParams::defaultUseBaseObjectName() {
    const static bool def = false;
    return def;
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:394)
void PartParams::setUseBaseObjectName(const bool &v) {
    instance()->handle->SetBool("UseBaseObjectName",v);
    instance()->UseBaseObjectName = v;
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:403)
void PartParams::removeUseBaseObjectName() {
    instance()->handle->RemoveBool("UseBaseObjectName");
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:369)
const char *PartParams::docAutoGroupSolids() {
    return "";
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:377)
const bool & PartParams::getAutoGroupSolids() {
    return instance()->AutoGroupSolids;
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:385)
const bool & PartParams::defaultAutoGroupSolids() {
    const static bool def = false;
    return def;
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:394)
void PartParams::setAutoGroupSolids(const bool &v) {
    instance()->handle->SetBool("AutoGroupSolids",v);
    instance()->AutoGroupSolids = v;
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:403)
void PartParams::removeAutoGroupSolids() {
    instance()->handle->RemoveBool("AutoGroupSolids");
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:369)
const char *PartParams::docSingleSolid() {
    return "";
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:377)
const bool & PartParams::getSingleSolid() {
    return instance()->SingleSolid;
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:385)
const bool & PartParams::defaultSingleSolid() {
    const static bool def = false;
    return def;
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:394)
void PartParams::setSingleSolid(const bool &v) {
    instance()->handle->SetBool("SingleSolid",v);
    instance()->SingleSolid = v;
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:403)
void PartParams::removeSingleSolid() {
    instance()->handle->RemoveBool("SingleSolid");
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:369)
const char *PartParams::docUsePipeForExtrusionDraft() {
    return "";
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:377)
const bool & PartParams::getUsePipeForExtrusionDraft() {
    return instance()->UsePipeForExtrusionDraft;
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:385)
const bool & PartParams::defaultUsePipeForExtrusionDraft() {
    const static bool def = false;
    return def;
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:394)
void PartParams::setUsePipeForExtrusionDraft(const bool &v) {
    instance()->handle->SetBool("UsePipeForExtrusionDraft",v);
    instance()->UsePipeForExtrusionDraft = v;
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:403)
void PartParams::removeUsePipeForExtrusionDraft() {
    instance()->handle->RemoveBool("UsePipeForExtrusionDraft");
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:369)
const char *PartParams::docLinearizeExtrusionDraft() {
    return "";
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:377)
const bool & PartParams::getLinearizeExtrusionDraft() {
    return instance()->LinearizeExtrusionDraft;
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:385)
const bool & PartParams::defaultLinearizeExtrusionDraft() {
    const static bool def = true;
    return def;
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:394)
void PartParams::setLinearizeExtrusionDraft(const bool &v) {
    instance()->handle->SetBool("LinearizeExtrusionDraft",v);
    instance()->LinearizeExtrusionDraft = v;
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:403)
void PartParams::removeLinearizeExtrusionDraft() {
    instance()->handle->RemoveBool("LinearizeExtrusionDraft");
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:369)
const char *PartParams::docAutoCorrectLink() {
    return "";
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:377)
const bool & PartParams::getAutoCorrectLink() {
    return instance()->AutoCorrectLink;
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:385)
const bool & PartParams::defaultAutoCorrectLink() {
    const static bool def = false;
    return def;
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:394)
void PartParams::setAutoCorrectLink(const bool &v) {
    instance()->handle->SetBool("AutoCorrectLink",v);
    instance()->AutoCorrectLink = v;
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:403)
void PartParams::removeAutoCorrectLink() {
    instance()->handle->RemoveBool("AutoCorrectLink");
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:369)
const char *PartParams::docRefineModel() {
    return "";
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:377)
const bool & PartParams::getRefineModel() {
    return instance()->RefineModel;
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:385)
const bool & PartParams::defaultRefineModel() {
    const static bool def = false;
    return def;
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:394)
void PartParams::setRefineModel(const bool &v) {
    instance()->handle->SetBool("RefineModel",v);
    instance()->RefineModel = v;
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:403)
void PartParams::removeRefineModel() {
    instance()->handle->RemoveBool("RefineModel");
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:369)
const char *PartParams::docAuxGroupUniqueLabel() {
    return "";
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:377)
const bool & PartParams::getAuxGroupUniqueLabel() {
    return instance()->AuxGroupUniqueLabel;
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:385)
const bool & PartParams::defaultAuxGroupUniqueLabel() {
    const static bool def = false;
    return def;
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:394)
void PartParams::setAuxGroupUniqueLabel(const bool &v) {
    instance()->handle->SetBool("AuxGroupUniqueLabel",v);
    instance()->AuxGroupUniqueLabel = v;
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:403)
void PartParams::removeAuxGroupUniqueLabel() {
    instance()->handle->RemoveBool("AuxGroupUniqueLabel");
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:369)
const char *PartParams::docAutoAuxGrouping() {
    return "";
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:377)
const bool & PartParams::getAutoAuxGrouping() {
    return instance()->AutoAuxGrouping;
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:385)
const bool & PartParams::defaultAutoAuxGrouping() {
    const static bool def = true;
    return def;
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:394)
void PartParams::setAutoAuxGrouping(const bool &v) {
    instance()->handle->SetBool("AutoAuxGrouping",v);
    instance()->AutoAuxGrouping = v;
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:403)
void PartParams::removeAutoAuxGrouping() {
    instance()->handle->RemoveBool("AutoAuxGrouping");
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:369)
const char *PartParams::docEnforcePrecision() {
    return "";
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:377)
const bool & PartParams::getEnforcePrecision() {
    return instance()->EnforcePrecision;
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:385)
const bool & PartParams::defaultEnforcePrecision() {
    const static bool def = true;
    return def;
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:394)
void PartParams::setEnforcePrecision(const bool &v) {
    instance()->handle->SetBool("EnforcePrecision",v);
    instance()->EnforcePrecision = v;
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:403)
void PartParams::removeEnforcePrecision() {
    instance()->handle->RemoveBool("EnforcePrecision");
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:369)
const char *PartParams::docOpsPrecisionLevel() {
    return "";
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:377)
const long & PartParams::getOpsPrecisionLevel() {
    return instance()->OpsPrecisionLevel;
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:385)
const long & PartParams::defaultOpsPrecisionLevel() {
    const static long def = 9;
    return def;
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:394)
void PartParams::setOpsPrecisionLevel(const long &v) {
    instance()->handle->SetInt("OpsPrecisionLevel",v);
    instance()->OpsPrecisionLevel = v;
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:403)
void PartParams::removeOpsPrecisionLevel() {
    instance()->handle->RemoveInt("OpsPrecisionLevel");
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:369)
const char *PartParams::docAutoHideOrigins() {
    return "";
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:377)
const bool & PartParams::getAutoHideOrigins() {
    return instance()->AutoHideOrigins;
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:385)
const bool & PartParams::defaultAutoHideOrigins() {
    const static bool def = true;
    return def;
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:394)
void PartParams::setAutoHideOrigins(const bool &v) {
    instance()->handle->SetBool("AutoHideOrigins",v);
    instance()->AutoHideOrigins = v;
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:403)
void PartParams::removeAutoHideOrigins() {
    instance()->handle->RemoveBool("AutoHideOrigins");
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:369)
const char *PartParams::docSplitEllipsoid() {
    return "";
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:377)
const bool & PartParams::getSplitEllipsoid() {
    return instance()->SplitEllipsoid;
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:385)
const bool & PartParams::defaultSplitEllipsoid() {
    const static bool def = true;
    return def;
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:394)
void PartParams::setSplitEllipsoid(const bool &v) {
    instance()->handle->SetBool("SplitEllipsoid",v);
    instance()->SplitEllipsoid = v;
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:403)
void PartParams::removeSplitEllipsoid() {
    instance()->handle->RemoveBool("SplitEllipsoid");
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:369)
const char *PartParams::docParallelRunThreshold() {
    return "";
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:377)
const long & PartParams::getParallelRunThreshold() {
    return instance()->ParallelRunThreshold;
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:385)
const long & PartParams::defaultParallelRunThreshold() {
    const static long def = 100;
    return def;
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:394)
void PartParams::setParallelRunThreshold(const long &v) {
    instance()->handle->SetInt("ParallelRunThreshold",v);
    instance()->ParallelRunThreshold = v;
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:403)
void PartParams::removeParallelRunThreshold() {
    instance()->handle->RemoveInt("ParallelRunThreshold");
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:369)
const char *PartParams::docAutoValidateShape() {
    return "";
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:377)
const bool & PartParams::getAutoValidateShape() {
    return instance()->AutoValidateShape;
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:385)
const bool & PartParams::defaultAutoValidateShape() {
    const static bool def = false;
    return def;
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:394)
void PartParams::setAutoValidateShape(const bool &v) {
    instance()->handle->SetBool("AutoValidateShape",v);
    instance()->AutoValidateShape = v;
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:403)
void PartParams::removeAutoValidateShape() {
    instance()->handle->RemoveBool("AutoValidateShape");
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:369)
const char *PartParams::docFixShape() {
    return "";
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:377)
const bool & PartParams::getFixShape() {
    return instance()->FixShape;
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:385)
const bool & PartParams::defaultFixShape() {
    const static bool def = false;
    return def;
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:394)
void PartParams::setFixShape(const bool &v) {
    instance()->handle->SetBool("FixShape",v);
    instance()->FixShape = v;
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:403)
void PartParams::removeFixShape() {
    instance()->handle->RemoveBool("FixShape");
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:369)
const char *PartParams::docLoftMaxDegree() {
    return "";
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:377)
const unsigned long & PartParams::getLoftMaxDegree() {
    return instance()->LoftMaxDegree;
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:385)
const unsigned long & PartParams::defaultLoftMaxDegree() {
    const static unsigned long def = 5;
    return def;
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:394)
void PartParams::setLoftMaxDegree(const unsigned long &v) {
    instance()->handle->SetUnsigned("LoftMaxDegree",v);
    instance()->LoftMaxDegree = v;
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:403)
void PartParams::removeLoftMaxDegree() {
    instance()->handle->RemoveUnsigned("LoftMaxDegree");
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:369)
const char *PartParams::docMinimumDeviation() {
    return "";
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:377)
const double & PartParams::getMinimumDeviation() {
    return instance()->MinimumDeviation;
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:385)
const double & PartParams::defaultMinimumDeviation() {
    const static double def = 0.05;
    return def;
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:394)
void PartParams::setMinimumDeviation(const double &v) {
    instance()->handle->SetFloat("MinimumDeviation",v);
    instance()->MinimumDeviation = v;
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:403)
void PartParams::removeMinimumDeviation() {
    instance()->handle->RemoveFloat("MinimumDeviation");
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:369)
const char *PartParams::docMeshDeviation() {
    return "";
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:377)
const double & PartParams::getMeshDeviation() {
    return instance()->MeshDeviation;
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:385)
const double & PartParams::defaultMeshDeviation() {
    const static double def = 0.2;
    return def;
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:394)
void PartParams::setMeshDeviation(const double &v) {
    instance()->handle->SetFloat("MeshDeviation",v);
    instance()->MeshDeviation = v;
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:403)
void PartParams::removeMeshDeviation() {
    instance()->handle->RemoveFloat("MeshDeviation");
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:369)
const char *PartParams::docMeshAngularDeflection() {
    return "";
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:377)
const double & PartParams::getMeshAngularDeflection() {
    return instance()->MeshAngularDeflection;
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:385)
const double & PartParams::defaultMeshAngularDeflection() {
    const static double def = 28.65;
    return def;
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:394)
void PartParams::setMeshAngularDeflection(const double &v) {
    instance()->handle->SetFloat("MeshAngularDeflection",v);
    instance()->MeshAngularDeflection = v;
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:403)
void PartParams::removeMeshAngularDeflection() {
    instance()->handle->RemoveFloat("MeshAngularDeflection");
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:369)
const char *PartParams::docMinimumAngularDeflection() {
    return "";
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:377)
const double & PartParams::getMinimumAngularDeflection() {
    return instance()->MinimumAngularDeflection;
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:385)
const double & PartParams::defaultMinimumAngularDeflection() {
    const static double def = 5.0;
    return def;
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:394)
void PartParams::setMinimumAngularDeflection(const double &v) {
    instance()->handle->SetFloat("MinimumAngularDeflection",v);
    instance()->MinimumAngularDeflection = v;
}

// Auto generated code (C:\Development\3D\FreeCAD\repo\src\Tools\params_utils.py:403)
void PartParams::removeMinimumAngularDeflection() {
    instance()->handle->RemoveFloat("MinimumAngularDeflection");
}
//[[[end]]]
