/***************************************************************************
 *   Copyright (c) 2010 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
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
# include <BRepAlgoAPI_Fuse.hxx>
# include <BRepPrimAPI_MakeRevol.hxx>
# include <gp_Lin.hxx>
# include <Precision.hxx>
# include <TopExp_Explorer.hxx>
# include <TopoDS.hxx>
#endif

#include <Base/Axis.h>
#include <Base/Exception.h>
#include <Base/Placement.h>
#include <Base/Tools.h>
#include <App/Document.h>
#include <Mod/Part/App/TopoShape.h>
#include <Mod/Part/App/TopoShapeOpCode.h>
#include "FeatureRevolution.h"

using namespace PartDesign;

namespace PartDesign {


PROPERTY_SOURCE(PartDesign::Revolution, PartDesign::ProfileBased)

const App::PropertyAngle::Constraints Revolution::floatAngle = { Base::toDegrees<double>(Precision::Angular()), 360.0, 1.0 };

Revolution::Revolution()
{
    ADD_PROPERTY_TYPE(Base,(Base::Vector3d(0.0,0.0,0.0)),"Revolution", App::Prop_ReadOnly, "Base");
    ADD_PROPERTY_TYPE(Axis,(Base::Vector3d(0.0,1.0,0.0)),"Revolution", App::Prop_ReadOnly, "Axis");
    ADD_PROPERTY_TYPE(Angle,(360.0),"Revolution", App::Prop_None, "Angle");
    Angle.setConstraints(&floatAngle);
    ADD_PROPERTY_TYPE(ReferenceAxis,(nullptr),"Revolution",(App::Prop_None),"Reference axis of revolution");
}

short Revolution::mustExecute() const
{
    if (Placement.isTouched() ||
        ReferenceAxis.isTouched() ||
        Axis.isTouched() ||
        Base.isTouched() ||
        Angle.isTouched())
        return 1;
    return ProfileBased::mustExecute();
}

App::DocumentObjectExecReturn *Revolution::execute()
{
    // Validate parameters
    double angle = Angle.getValue();
    if (angle > 360.0)
        return new App::DocumentObjectExecReturn("Angle of revolution too large");

    angle = Base::toRadians<double>(angle);
    if (angle < Precision::Angular())
        return new App::DocumentObjectExecReturn("Angle of revolution too small");

    // Reverse angle if selected
    if (Reversed.getValue() && !Midplane.getValue())
        angle *= (-1.0);

    TopoShape sketchshape;
    try {
        sketchshape = getVerifiedFace();
    } catch (const Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }

    // if the Base property has a valid shape, fuse the AddShape into it
    TopoShape base;
    try {
        base = getBaseShape();
    } catch (const Base::Exception&) {
        // fall back to support (for legacy features)
    }

    // update Axis from ReferenceAxis
    try {
        updateAxis();
    } catch (const Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }

    // get revolve axis
    Base::Vector3d b = Base.getValue();
    gp_Pnt pnt(b.x,b.y,b.z);
    Base::Vector3d v = Axis.getValue();
    gp_Dir dir(v.x,v.y,v.z);

    try {
        if (sketchshape.isNull())
            return new App::DocumentObjectExecReturn("Creating a face from sketch failed");

        // Rotate the face by half the angle to get Revolution symmetric to sketch plane
        if (Midplane.getValue()) {
            gp_Trsf mov;
            mov.SetRotation(gp_Ax1(pnt, dir), Base::toRadians<double>(Angle.getValue()) * (-1.0) / 2.0);
            TopLoc_Location loc(mov);
            sketchshape.move(loc);
        }

        auto invObjLoc = this->positionByPrevious();
        pnt.Transform(invObjLoc.Transformation());
        dir.Transform(invObjLoc.Transformation());
        base.move(invObjLoc);
        sketchshape.move(invObjLoc);

        // Check distance between sketchshape and axis - to avoid failures and crashes
        TopExp_Explorer xp;
        xp.Init(sketchshape.getShape(), TopAbs_FACE);
        for (;xp.More(); xp.Next()) {
            if (checkLineCrossesFace(gp_Lin(pnt, dir), TopoDS::Face(xp.Current())))
                return new App::DocumentObjectExecReturn("Revolve axis intersects the sketch");
        }

        // revolve the face to a solid
        TopoShape result(0,getDocument()->getStringHasher());
        try {
            result.makERevolve(sketchshape,gp_Ax1(pnt, dir), angle);
            result = refineShapeIfActive(result);
        }catch(Standard_Failure &) {
            return new App::DocumentObjectExecReturn("Could not revolve the sketch!");
        }

        // set the additive shape property for later usage in e.g. pattern
        this->AddSubShape.setValue(result);            
        if (isRecomputePaused())
            return App::DocumentObject::StdReturn;

        if(base.isNull()) {
            result = refineShapeIfActive(result);
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
            return new App::DocumentObjectExecReturn("Unknown operation type");
        }
        try {
            this->fixShape(result);
            boolOp.makEBoolean(maker, {base,result});
        }catch(Standard_Failure &e) {
            return new App::DocumentObjectExecReturn("Failed to perform boolean operation");
        }
        boolOp = this->getSolid(boolOp);
        // lets check if the result is a solid
        if (boolOp.isNull())
            return new App::DocumentObjectExecReturn("Resulting shape is not a solid");

        boolOp = refineShapeIfActive(boolOp);
        Shape.setValue(getSolid(boolOp));
        return App::DocumentObject::StdReturn;
    }
    catch (Standard_Failure& e) {

        if (std::string(e.GetMessageString()) == "TopoDS::Face")
            return new App::DocumentObjectExecReturn("Could not create face from sketch.\n"
                "Intersecting sketch entities in a sketch are not allowed.");
        else
            return new App::DocumentObjectExecReturn(e.GetMessageString());
    }
    catch (Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }
}

bool Revolution::suggestReversed()
{
    try {
        updateAxis();
    } catch (const Base::Exception&) {
        return false;
    }

    return ProfileBased::getReversedAngle(Base.getValue(), Axis.getValue()) < 0.0;
}

void Revolution::updateAxis()
{
    App::DocumentObject *pcReferenceAxis = ReferenceAxis.getValue();
    const std::vector<std::string> &subReferenceAxis = ReferenceAxis.getSubValues();
    Base::Vector3d base;
    Base::Vector3d dir;
    getAxis(pcReferenceAxis, subReferenceAxis, base, dir, ForbiddenAxis::NotParallelWithNormal);

    Base.setValue(base.x,base.y,base.z);
    Axis.setValue(dir.x,dir.y,dir.z);
}

}
