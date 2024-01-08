/***************************************************************************
 *   Copyright (c) 2015 Stefan Tröger <stefantroeger@gmx.net>              *
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
# include <TopoDS_Wire.hxx>
# include <TopExp_Explorer.hxx>
# include <BRepAlgoAPI_Cut.hxx>
# include <BRepAlgoAPI_Fuse.hxx>
# include <BRepBuilderAPI_MakeSolid.hxx>
# include <BRepBuilderAPI_Sewing.hxx>
# include <BRepClass3d_SolidClassifier.hxx>
# include <BRepOffsetAPI_ThruSections.hxx>
# include <TopoDS.hxx>
# include <Precision.hxx>
#endif

#include <boost/core/ignore_unused.hpp>

#include <App/Document.h>
#include <App/DocumentObserver.h>
#include <Base/Exception.h>
#include <Base/Reader.h>
#include <Mod/Part/App/TopoShapeOpCode.h>

#include "FeatureLoft.h"


using namespace PartDesign;

PROPERTY_SOURCE(PartDesign::Loft, PartDesign::ProfileBased)

Loft::Loft()
{
    ADD_PROPERTY_TYPE(Sections,(nullptr),"Loft",App::Prop_None,"List of sections");
    Sections.setValue(nullptr);
    ADD_PROPERTY_TYPE(Ruled,(false),"Loft",App::Prop_None,"Create ruled surface");
    ADD_PROPERTY_TYPE(Closed,(false),"Loft",App::Prop_None,"Close Last to First Profile");
    ADD_PROPERTY_TYPE(SplitProfile,(false),"Loft",App::Prop_None,
            "In case of profile with multiple faces, split profile\n"
            "and build each shell independently before fusing.");
}

short Loft::mustExecute() const
{
    if (Sections.isTouched())
        return 1;
    if (Ruled.isTouched())
        return 1;
    if (Closed.isTouched())
        return 1;

    return ProfileBased::mustExecute();
}

std::vector<Part::TopoShape>
Loft::getSectionShape(const char *name,
                      App::DocumentObject *obj,
                      const std::vector<std::string> &subs,
                      size_t expected_size)
{
    std::vector<TopoShape> shapes;
    if (subs.empty() || std::find(subs.begin(), subs.end(), std::string()) != subs.end()) {
        shapes.push_back(Part::Feature::getTopoShape(obj));
        if (shapes.back().isNull())
            FC_THROWM(Part::NullShapeException, "Failed to get shape of "
                    << name << " " << App::SubObjectT(obj, "").getSubObjectFullName(obj->getDocument()->getName()));
    } else {
        for (const auto &sub : subs) {
            shapes.push_back(Part::Feature::getTopoShape(obj, sub.c_str(), /*needSubElement*/true));
            if (shapes.back().isNull())
                FC_THROWM(Part::NullShapeException, "Failed to get shape of " << name << " "
                        << App::SubObjectT(obj, sub.c_str()).getSubObjectFullName(obj->getDocument()->getName()));
        }
    }
    auto compound = TopoShape().makECompound(shapes, "", false);
    auto wires = compound.getSubTopoShapes(TopAbs_WIRE);
    auto edges = compound.getSubTopoShapes(TopAbs_EDGE, TopAbs_WIRE); // get free edges and make wires from it
    if (edges.size()) {
        auto extra = TopoShape().makEWires(edges).getSubTopoShapes(TopAbs_WIRE);
        wires.insert(wires.end(), extra.begin(), extra.end());
    }
    const char *msg = "Sections need to have the same amount of wires or vertices as the base section";
    if (!wires.empty()) {
        if (expected_size && expected_size != wires.size())
            FC_THROWM(Base::CADKernelError, msg);
        return wires;
    }
    auto vertices = compound.getSubTopoShapes(TopAbs_VERTEX);
    if (vertices.empty())
        FC_THROWM(Base::CADKernelError, "Invalid " << name << " shape, expecting either wires or vertices");
    if (expected_size && expected_size != vertices.size())
        FC_THROWM(Base::CADKernelError, msg);
    return vertices;
}

App::DocumentObjectExecReturn *Loft::execute(void)
{
    std::vector<TopoShape> wires;
    try {
        wires = getSectionShape("Profile", Profile.getValue(), Profile.getSubValues());
    } catch (const Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }

    // if the Base property has a valid shape, fuse the pipe into it
    TopoShape base;
    try {
        base = getBaseShape();
    } catch (const Base::Exception&) {
    }

    auto hasher = getDocument()->getStringHasher();
 
    try {
        //setup the location
        auto invObjLoc = this->positionByPrevious();
        if(!base.isNull())
            base.move(invObjLoc);
             
        //build up multisections
        auto multisections = Sections.getSubListValues();
        if(multisections.empty())
            return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Loft: At least one section is needed"));
        
        std::vector<std::vector<TopoShape>> wiresections;
        wiresections.reserve(wires.size());
        for(auto& wire : wires)
            wiresections.emplace_back(1, wire);
                
        for (const auto &subSet : multisections) {
            int i=0;
            for (const auto &s : getSectionShape("Section", subSet.first, subSet.second, wiresections.size()))
                wiresections[i++].push_back(s);
        }

        TopoShape result(0,hasher);
        std::vector<TopoShape> shapes;

        if (SplitProfile.getValue()) {
            for (auto &wires : wiresections) {
                for(auto& wire : wires)
                    wire.move(invObjLoc);
                shapes.push_back(TopoShape(0, hasher).makELoft(
                            wires, true, Ruled.getValue(), Closed.getValue()));
            }
        } else {
            //build all shells
            std::vector<TopoShape> shells;
            for (auto &wires : wiresections) {
                for(auto& wire : wires)
                    wire.move(invObjLoc);
                shells.push_back(TopoShape(0, hasher).makELoft(
                            wires, false, Ruled.getValue(), Closed.getValue()));
            }

            //build the top and bottom face, sew the shell and build the final solid
            TopoShape front;
            if (wiresections[0].front().shapeType() != TopAbs_VERTEX) {
                front = getVerifiedFace();
                if (front.isNull())
                    return new App::DocumentObjectExecReturn(
                            QT_TRANSLATE_NOOP("Exception", "Loft: Creating a face from sketch failed"));
                front.move(invObjLoc);
            }

            TopoShape back;
            if (wiresections[0].back().shapeType() != TopAbs_VERTEX) {
                std::vector<TopoShape> backwires;
                for(auto& wires : wiresections)
                    backwires.push_back(wires.back());
                back = TopoShape(0,hasher).makEFace(backwires);
            }
            
            if (!front.isNull() || !back.isNull()) {
                BRepBuilderAPI_Sewing sewer;
                sewer.SetTolerance(Precision::Confusion());
                if (!front.isNull())
                    sewer.Add(front.getShape());
                if (!back.isNull())
                    sewer.Add(back.getShape());
                for(auto& s : shells)
                    sewer.Add(s.getShape());      
                
                sewer.Perform();

                if (!front.isNull())
                    shells.push_back(front);
                if (!back.isNull())
                    shells.push_back(back);
                result = result.makEShape(sewer,shells);
            }

            if(!result.countSubShapes(TopAbs_SHELL))
                return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Loft: Failed to create shell"));
            shapes = result.getSubTopoShapes(TopAbs_SHELL);
        }

        for (auto &s : shapes) {
            //build the solid
            s = s.makESolid();
            BRepClass3d_SolidClassifier SC(s.getShape());
            SC.PerformInfinitePoint(Precision::Confusion());
            if ( SC.State() == TopAbs_IN)
                s.setShape(s.getShape().Reversed(),false);
            if (Linearize.getValue())
                s.linearize(true, false);
            this->fixShape(s);
        }

        AddSubShape.setValue(result.makECompound(shapes, nullptr, false));
        if (isRecomputePaused())
            return App::DocumentObject::StdReturn;
        
        if (shapes.size() > 1)
            result.makEFuse(shapes);
        else
            result = shapes.front();
            
        if(base.isNull()) {
            Shape.setValue(getSolid(result));
            return App::DocumentObject::StdReturn;
        }

        result.Tag = -getID();
        TopoShape boolOp(0,getDocument()->getStringHasher());

        const char *maker;
        switch(getAddSubType()) {
        case Additive:
            maker = Part::OpCodes::Fuse;
            break;
        case Subtractive:
            maker = Part::OpCodes::Cut;
            break;
        case Intersecting:
            maker = Part::OpCodes::Common;
            break;
        default:
            return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Unknown operation type"));
        }
        try {
            boolOp.makEBoolean(maker, {base,result});
        }
        catch(Standard_Failure &e) {
            return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Failed to perform boolean operation"));
        }
        boolOp = this->getSolid(boolOp);
        // lets check if the result is a solid
        if (boolOp.isNull())
            return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Resulting shape is not a solid"));

        boolOp = refineShapeIfActive(boolOp);
        Shape.setValue(getSolid(boolOp));
        return App::DocumentObject::StdReturn;
    }
    catch (Standard_Failure& e) {
        return new App::DocumentObjectExecReturn(e.GetMessageString());
    }
    catch (const Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }
    catch (...) {
        return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Loft: A fatal error occurred when making the loft"));
    }
}


PROPERTY_SOURCE(PartDesign::AdditiveLoft, PartDesign::Loft)
AdditiveLoft::AdditiveLoft() {
}

PROPERTY_SOURCE(PartDesign::SubtractiveLoft, PartDesign::Loft)
SubtractiveLoft::SubtractiveLoft() {
    initAddSubType(Subtractive);
}

void Loft::handleChangedPropertyType(Base::XMLReader& reader, const char* TypeName, App::Property* prop)
{
    // property Sections had the App::PropertyLinkList and was changed to App::PropertyXLinkSubList
    if (prop == &Sections && strcmp(TypeName, "App::PropertyLinkList") == 0) {
        Sections.upgrade(reader, TypeName);
    }
    else {
        ProfileBased::handleChangedPropertyType(reader, TypeName, prop);
    }
}
