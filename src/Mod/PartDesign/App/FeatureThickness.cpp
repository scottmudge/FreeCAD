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
# include <Precision.hxx>
# include <TopoDS.hxx>
#endif

#include <Base/Exception.h>
#include <Base/Parameter.h>
#include <Base/Console.h>
#include <App/Application.h>
#include <App/Document.h>
#include "FeatureThickness.h"

FC_LOG_LEVEL_INIT("PartDesign",true,true)

using namespace PartDesign;

const char *PartDesign::Thickness::ModeEnums[] = {"Skin", "Pipe", "RectoVerso", nullptr};
const char *PartDesign::Thickness::JoinEnums[] = {"Arc", "Intersection", nullptr};

PROPERTY_SOURCE(PartDesign::Thickness, PartDesign::DressUp)

Thickness::Thickness()
{
    ADD_PROPERTY_TYPE(Value, (1.0), "Thickness", App::Prop_None, "Thickness value");
    ADD_PROPERTY_TYPE(Mode, (long(0)), "Thickness", App::Prop_None, "Mode");
    Mode.setEnums(ModeEnums);
    ADD_PROPERTY_TYPE(Join, (long(0)), "Thickness", App::Prop_None, "Join type");
    Join.setEnums(JoinEnums);
    ADD_PROPERTY_TYPE(Reversed,(false),"Thickness",App::Prop_None,"Apply the thickness towards the solids interior");
    ADD_PROPERTY_TYPE(Intersection,(false),"Thickness",App::Prop_None,"Enable intersection-handling");
    ADD_PROPERTY_TYPE(MakeOffset,(false),"Thickness",App::Prop_None,"Make a thicken or shrunken solid instead of a thin shell");

    AddSubType.setStatus(App::Property::ReadOnly, false);
}

short Thickness::mustExecute() const
{
    if (Placement.isTouched() ||
        Value.isTouched() ||
        Mode.isTouched() ||
        Join.isTouched())
        return 1;
    return DressUp::mustExecute();
}

App::DocumentObjectExecReturn *Thickness::execute()
{
    // Base shape
    Part::TopoShape baseShape;
    try {
        baseShape = getBaseShape();
    } catch (Base::Exception &e) {
        return new App::DocumentObjectExecReturn(e.what());
    }

    std::map<int,std::vector<TopoShape> > closeFaces;
    const std::vector<std::string>& subStrings = Base.getSubValues(true);
    for (std::vector<std::string>::const_iterator it = subStrings.begin(); it != subStrings.end(); ++it) {
        TopoDS_Shape face;
        try {
            face = baseShape.getSubShape(it->c_str());
        }catch(...){}
        if(face.IsNull())
            return new App::DocumentObjectExecReturn("Invalid face reference");
        int index = baseShape.findAncestor(face,TopAbs_SOLID);
        if(!index) {
            FC_WARN(getFullName() << ": Ignore non-solid face  " << *it);
            continue;
        }
        closeFaces[index].emplace_back(face);
    }

    bool reversed = Reversed.getValue();
    bool intersection = Intersection.getValue();
    double thickness =  (reversed ? -1. : 1. )*Value.getValue();
    double tol = Precision::Confusion();
    short mode = (short)Mode.getValue();
    short join = (short)Join.getValue();

    std::vector<TopoShape> shapes;
    int count = baseShape.countSubShapes(TopAbs_SOLID);
    if(!count)
        return new App::DocumentObjectExecReturn("No solid");

    if (fabs(thickness) > 2*tol) {
        auto it = closeFaces.begin();
        for(int i=1;i<=count;++i) {
            std::vector<TopoShape> dummy;
            const auto *faces = &dummy;
            TopoShape solid = baseShape;
            if(it!=closeFaces.end() && i>=it->first) {
                faces = &it->second;
                solid = baseShape.getSubTopoShape(TopAbs_SOLID,it->first);
            }
            TopoShape res(0,getDocument()->getStringHasher());
            try {
                res = solid.makEThickSolid(*faces, thickness, tol, intersection, false, mode,
                                           static_cast<Part::TopoShape::JoinType>(join));
                this->fixShape(res);

                // When no face to remove, OCC makeThickSolid actually behave
                // the same as makeOffset. We slightly change that behavior to
                // always create thick shell unless `MakeOffset` is active. So if
                // face is empty, and MakeOffset is not active, we'll do a cut to
                // make the shell.
                if (faces->empty() && !MakeOffset.getValue()) {
                    if (thickness < 0.)
                        res = solid.makECut(res);
                    else
                        res = res.makECut(solid);
                }
                else if (!faces->empty() && MakeOffset.getValue()) {
                    if (thickness < 0.)
                        res = solid.makECut(res);
                    else
                        res = solid.makEFuse(res);
                }
                this->fixShape(res);
                shapes.push_back(res);
            }catch(Standard_Failure &e) {
                FC_ERR("Exception on making thick solid: " << e.GetMessageString());
                return new App::DocumentObjectExecReturn("Failed to make thick solid");
            }
            if (it !=closeFaces.end()) {
                ++it;
            }
        }
    }

    TopoShape result(0,getDocument()->getStringHasher());
    if(shapes.size()>1) {
        result.makEFuse(shapes);
    }else if (shapes.empty())
        result = baseShape;
    else
        result = shapes.front();
    result = refineShapeIfActive(result);
    this->Shape.setValue(getSolid(result));
    return App::DocumentObject::StdReturn;
}
