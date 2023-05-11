/***************************************************************************
 *   Copyright (c) 2011 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/


#include "PreCompiled.h"
#ifndef _PreComp_
# include <Standard_Failure.hxx>
#endif

#include <App/Application.h>
#include <App/FeaturePythonPyImp.h>
#include <Base/Parameter.h>
#include <Mod/Part/App/modelRefine.h>

#include "FeatureAddSub.h"
#include "FeaturePy.h"


using namespace PartDesign;


PROPERTY_SOURCE(PartDesign::FeatureAddSub, PartDesign::Feature)

FeatureAddSub::FeatureAddSub()
{
    ADD_PROPERTY(AddSubShape,(TopoDS_Shape()));
    ADD_PROPERTY_TYPE(Refine,(0),"Part Design",(App::PropertyType)(App::Prop_None),"Refine shape (clean up redundant edges) after adding/subtracting");
    //init Refine property
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/PartDesign");
    this->Refine.setValue(hGrp->GetBool("RefineModel", false));

    static const char* TypeEnums[]= {"Additive","Subtractive","Intersecting",NULL};
    ADD_PROPERTY_TYPE(AddSubType,((long)0),"Part Design",
            (App::PropertyType)(App::Prop_None), "Operation type");
    AddSubType.setEnums(TypeEnums);
}

FeatureAddSub::Type FeatureAddSub::getAddSubType()
{
    return addSubType;
}

void FeatureAddSub::initAddSubType(Type t)
{
    switch(t) {
    case Subtractive:
        addSubType = Subtractive;
        AddSubType.setValue((long)1);
        break;
    case Intersecting:
        addSubType = Intersecting;
        AddSubType.setValue((long)2);
        break;
    default:
        addSubType = Additive;
        AddSubType.setValue((long)0);
        break;
    }
}

void FeatureAddSub::onChanged(const App::Property *prop)
{
    if (prop == &AddSubType) {
        switch(AddSubType.getValue()) {
        case 0:
            addSubType = Additive;
            break;
        case 1:
            addSubType = Subtractive;
            break;
        case 2:
            addSubType = Intersecting;
            break;
        }
    }
    PartDesign::Feature::onChanged(prop);
}

short FeatureAddSub::mustExecute() const
{
    if (Refine.isTouched())
        return 1;
    return PartDesign::Feature::mustExecute();
}

TopoShape FeatureAddSub::refineShapeIfActive(const TopoShape& oldShape) const
{
    if (this->Refine.getValue()) {
        TopoShape shape(oldShape);
        this->fixShape(shape);
        return shape.makERefine();
    }
    return oldShape;
}

void FeatureAddSub::getAddSubShape(std::vector<std::pair<Part::TopoShape, Type> > &shapes)
{
    if (Suppress.getValue())
        return;
    shapes.emplace_back(AddSubShape.getShape(), addSubType);
}

const std::string &FeatureAddSub::addsubElementPrefix()
{
    static std::string res(Data::ComplexGeoData::elementMapPrefix()
                           + "AddSub"
                           + Data::ComplexGeoData::elementMapPrefix());
    return res;
}

void FeatureAddSub::setPauseRecompute(bool enable)
{
    if (enable == pauseRecompute)
        return;
    pauseRecompute = enable;
    if (enable)
        pausedRevision = this->getRevision();
    else if (pausedRevision != this->getRevision())
        touch();
}

bool FeatureAddSub::isRecomputePaused() const
{
    return pauseRecompute;
}

namespace App {
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(PartDesign::FeatureAddSubPython, PartDesign::FeatureAddSub)
template<> const char* PartDesign::FeatureAddSubPython::getViewProviderName(void) const {
    return "PartDesignGui::ViewProviderPython";
}
template<> PyObject* PartDesign::FeatureAddSubPython::getPyObject(void) {
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new FeaturePythonPyT<PartDesign::FeaturePy>(this),true);
    }
    return Py::new_reference_to(PythonObject);
}
/// @endcond

// explicit template instantiation
template class PartDesignExport FeaturePythonT<PartDesign::FeatureAddSub>;
}


namespace PartDesign {

PROPERTY_SOURCE(PartDesign::FeatureAdditivePython, PartDesign::FeatureAddSubPython)

FeatureAdditivePython::FeatureAdditivePython()
{
}

FeatureAdditivePython::~FeatureAdditivePython()
{
}


PROPERTY_SOURCE(PartDesign::FeatureSubtractivePython, PartDesign::FeatureAddSubPython)

FeatureSubtractivePython::FeatureSubtractivePython()
{
    initAddSubType(Subtractive);
}

FeatureSubtractivePython::~FeatureSubtractivePython()
{
}

PROPERTY_SOURCE(PartDesign::FeatureIntersectingPython, PartDesign::FeatureAddSubPython)

FeatureIntersectingPython::FeatureIntersectingPython()
{
    initAddSubType(Intersecting);
}

FeatureIntersectingPython::~FeatureIntersectingPython()
{
}

}
