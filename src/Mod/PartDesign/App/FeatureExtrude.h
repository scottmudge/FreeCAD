/***************************************************************************
 *   Copyright (c) 2020 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef PARTDESIGN_FEATURE_EXTRUDE_H
#define PARTDESIGN_FEATURE_EXTRUDE_H

#include <App/PropertyStandard.h>
#include <App/PropertyUnits.h>
#include <Base/Bitmask.h>
#include "FeatureSketchBased.h"

class gp_Dir;
class TopoDS_Face;
class TopoDS_Shape;

namespace PartDesign
{

class PartDesignExport FeatureExtrude : public ProfileBased
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartDesign::FeatureExtrude);

public:
    FeatureExtrude();

    App::PropertyEnumeration Type;
    App::PropertyLength      Length;
    App::PropertyLength      Length2;
    App::PropertyAngle       TaperAngle;
    App::PropertyAngle       TaperAngle2; // for upstream compatibility
    App::PropertyAngle       TaperAngleRev;
    App::PropertyBool        UseCustomVector;
    App::PropertyVector      Direction;
    App::PropertyBool        AlongSketchNormal;
    App::PropertyLength      Offset;
    App::PropertyLinkSub     ReferenceAxis;
    App::PropertyAngle       TaperInnerAngle;
    App::PropertyAngle       TaperInnerAngleRev;
    App::PropertyBool        AutoTaperInnerAngle;
    App::PropertyBool        UsePipeForDraft;
    App::PropertyBool        CheckUpToFaceLimits;

    static App::PropertyQuantityConstraint::Constraints signedLengthConstraint;
    static double maxAngle;
    static App::PropertyAngle::Constraints floatAngle;

    /** @name methods override feature */
    //@{
    short mustExecute() const override;
    void setupObject() override;
    //@}

protected:
    void initProperties(const char *group);

    void onChanged(const App::Property *) override;
    void handleChangedPropertyName(Base::XMLReader &reader, const char * TypeName, const char *Name) override;

    Base::Vector3d computeDirection(const Base::Vector3d& sketchVector, bool inverse);
    bool hasTaperedAngle() const;

    /// Options for buildExtrusion()
    enum class ExtrudeOption {
        MakeFace = 1,
        MakeFuse = 2,
        LegacyPocket = 4,
        InverseDirection = 8,
    };

    using ExtrudeOptions = Base::Flags<ExtrudeOption>;

    App::DocumentObjectExecReturn *buildExtrusion(ExtrudeOptions options);

    /**
      * Generates an extrusion of the input sketchshape and stores it in the given \a prism
      */
    void generatePrism(TopoShape& prism,
                       TopoShape sketchshape,
                       const std::string& method,
                       const gp_Dir& direction,
                       const double L,
                       const double L2,
                       const bool midplane,
                       const bool reversed);

    /**
      * Disables settings that are not valid for the current method
      */
    void updateProperties(const std::string &method);
};

} //namespace PartDesign

ENABLE_BITMASK_OPERATORS(PartDesign::FeatureExtrude::ExtrudeOption)

#endif // PARTDESIGN_FEATURE_EXTRUDE_H
