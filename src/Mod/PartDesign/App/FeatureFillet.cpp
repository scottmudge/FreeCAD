/***************************************************************************
 *   Copyright (c) 2008 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <BRepAlgo.hxx>
# include <BRepFilletAPI_MakeFillet.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Edge.hxx>
# include <TopTools_ListOfShape.hxx>
# include <ShapeFix_Shape.hxx>
# include <ShapeFix_ShapeTolerance.hxx>
#endif

#include <Base/Exception.h>
#include <Base/Reader.h>
#include <App/Application.h>
#include <App/Document.h>
#include <Mod/Part/App/TopoShape.h>

#include "FeatureFillet.h"


using namespace PartDesign;


PROPERTY_SOURCE(PartDesign::Fillet, PartDesign::DressUp)

const App::PropertyQuantityConstraint::Constraints floatRadius = {0.0,FLT_MAX,0.1};

inline bool getEnforcePrecision() { 
    const auto partParamsGroup = App::GetApplication().GetUserParameter().GetGroup("BaseApp/Preferences/Mod/Part"); 
    return partParamsGroup ? partParamsGroup->GetBool("EnforcePrecision", true) : true; 
} 
 
inline long getOperationalPrecisionLevel() { 
    const auto partParamsGroup = App::GetApplication().GetUserParameter().GetGroup("BaseApp/Preferences/Mod/Part"); 
    return partParamsGroup ? partParamsGroup->GetInt("OpsPrecisionLevel", 9) : 9; 
} 

inline double getMinimumPrecisionIncrement() {
    return 1.0f / std::pow(10.0, getOperationalPrecisionLevel());
}

Fillet::Fillet()
{
    ADD_PROPERTY_TYPE(Radius, (1.0), "Fillet", App::Prop_None, "Fillet radius.");
    ADD_PROPERTY(Segments,());
    Radius.setUnit(Base::Unit::Length);
    Radius.setConstraints(&floatRadius);

    Segments.connectLinkProperty(Base);

    ADD_PROPERTY_TYPE(UseAllEdges, (false), "Fillet", App::Prop_None,
      "Fillet all edges if true, else use only those edges in Base property.\n"
      "If true, then this overrides any edge changes made to the Base property or in the dialog.\n");
}

short Fillet::mustExecute() const
{
    if (Placement.isTouched()
            || Radius.isTouched()
            || Segments.isTouched())
        return 1;
    return DressUp::mustExecute();
}

App::DocumentObjectExecReturn *Fillet::execute()
{
    Part::TopoShape baseShape;
    try {
        baseShape = getBaseShape();
    }
    catch (Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }
    baseShape.setTransform(Base::Matrix4D());

    auto edges = UseAllEdges.getValue() ? baseShape.getSubTopoShapes(TopAbs_EDGE)
                                        : getContinuousEdges(baseShape);
    if (edges.empty())
        return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Fillet not possible on selected shapes"));

    double radius = Radius.getValue();

    if(radius <= 0)
        return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Fillet radius must be greater than zero"));

    this->positionByBaseFeature();

    auto tryFillet = [&](const double dec = 0.0){
        try {
            TopoShape shape(0,getDocument()->getStringHasher());

            std::vector<TopoShape::FilletSegments> segmentList;
            std::string sub;
            for (const auto &e : edges) {
                int index = baseShape.findShape(e.getShape());
                segmentList.emplace_back();
                auto &conf = segmentList.back();
                if (index == 0)
                    continue;
                sub = "Edge";
                sub += std::to_string(index);
                const auto &segments = Segments.getValue(sub);
                for (const auto &segment : segments)
                    conf.emplace_back(segment.param, segment.radius - dec, segment.length);
            }

            shape.makEFillet(baseShape,edges,segmentList,Radius.getValue() - dec);
            if (shape.isNull())
                return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Resulting shape is null"));

            TopTools_ListOfShape aLarg;
            aLarg.Append(baseShape.getShape());
            bool failed = false;
            if (!BRepAlgo::IsValid(aLarg, shape.getShape(), Standard_False, Standard_False)) {
                ShapeFix_ShapeTolerance aSFT;
                aSFT.LimitTolerance(shape.getShape(), Precision::Confusion(), Precision::Confusion(), TopAbs_SHAPE);
                // For backward compatibility
                if (FixShape.getValue() == 0) {
                    shape.fix();
                    if (!BRepAlgo::IsValid(aLarg, shape.getShape(), Standard_False, Standard_False))
                        failed = true;
                }
            }

            if (!failed) {
                shape = refineShapeIfActive(shape);
                shape = getSolid(shape);
            }
            this->Shape.setValue(shape);

            if (failed)
                return new App::DocumentObjectExecReturn("Resulting shape is invalid");
            return App::DocumentObject::StdReturn;
        }
        catch (Standard_Failure& e) {
            return new App::DocumentObjectExecReturn(e.GetMessageString());
        }  
    };

    if (!getEnforcePrecision()) return tryFillet();
    if (tryFillet() != App::DocumentObject::StdReturn) return tryFillet(getMinimumPrecisionIncrement());
    return App::DocumentObject::StdReturn;   
}

void Fillet::handleChangedPropertyType(Base::XMLReader &reader, const char * TypeName, App::Property * prop)
{
    if (prop && strcmp(TypeName,"App::PropertyFloatConstraint") == 0 &&
        strcmp(prop->getTypeId().getName(), "App::PropertyQuantityConstraint") == 0) {
        App::PropertyFloatConstraint p;
        p.Restore(reader);
        static_cast<App::PropertyQuantityConstraint*>(prop)->setValue(p.getValue());
    }
    else {
        DressUp::handleChangedPropertyType(reader, TypeName, prop);
    }
}
