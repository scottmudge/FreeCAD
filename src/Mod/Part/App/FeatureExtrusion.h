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


#ifndef PART_FEATUREEXTRUSION_H
#define PART_FEATUREEXTRUSION_H

#include <App/PropertyStandard.h>
#include <App/PropertyUnits.h>
#include <App/StringHasher.h>
#include "PartFeature.h"
#include "FaceMakerCheese.h"
#include <TopoDS_Face.hxx>

namespace Part
{

class PartExport Extrusion : public Part::Feature
{
    PROPERTY_HEADER_WITH_OVERRIDE(Part::Extrusion);

public:
    Extrusion();

    App::PropertyLink Base;
    App::PropertyVector Dir;
    App::PropertyEnumeration DirMode;
    App::PropertyLinkSub DirLink;
    App::PropertyDistance LengthFwd;
    App::PropertyDistance LengthRev;
    App::PropertyBool Solid;
    App::PropertyBool Reversed;
    App::PropertyBool Symmetric;
    App::PropertyAngle TaperAngle;
    App::PropertyAngle TaperAngleRev;
    App::PropertyAngle TaperInnerAngle;
    App::PropertyAngle TaperInnerAngleRev;
    App::PropertyBool AutoTaperInnerAngle;
    App::PropertyString FaceMakerClass;
    App::PropertyBool UsePipeForDraft;
    App::PropertyBool Linearize;

    /**
     * @brief The ExtrusionParameters struct is supposed to be filled with final
     * extrusion parameters, after resolving links, applying mode logic,
     * reversing, etc., and be passed to extrudeShape.
     */
    struct PartExport ExtrusionParameters {
        gp_Dir dir;
        double lengthFwd;
        double lengthRev;
        bool solid;
        bool innertaper;
        bool usepipe;
        bool linearize;
        double taperAngleFwd; //in radians
        double taperAngleRev;
        double innerTaperAngleFwd; //in radians
        double innerTaperAngleRev;
        std::string faceMakerClass;
        ExtrusionParameters();
    };

    /** @name methods override feature */
    //@{
    /// recalculate the feature
    App::DocumentObjectExecReturn *execute(void) override;
    short mustExecute() const override;
    /// returns the type name of the view provider
    const char* getViewProviderName(void) const override {
        return "PartGui::ViewProviderExtrusion";
    }
    void handleChangedPropertyName(Base::XMLReader &reader, const char * TypeName, const char *Name) override;
    void onChanged(const App::Property *) override;
    //@}

    /**
     * @brief extrudeShape powers the extrusion feature.
     * @param source: the shape to be extruded
     * @param params: extrusion parameters
     * @return result of extrusion
     */
    static void extrudeShape(TopoShape &result, const TopoShape &source, const ExtrusionParameters& params);

    /**
     * @brief fetchAxisLink: read AxisLink to obtain the direction and
     * length. Note: this routine is re-used in Extrude dialog, hence it
     * is static.
     * @param axisLink (input): the link
     * @param basepoint (output): starting point of edge. Not used by extrude as of now.
     * @param dir (output): direction of axis, with magnitude (length)
     * @return true if link was fetched. false if link was empty. Throws if the
     * link is wrong.
     */
    static bool fetchAxisLink(const App::PropertyLinkSub& axisLink,
                              Base::Vector3d &basepoint,
                              Base::Vector3d &dir);

    /**
     * @brief computeFinalParameters: applies mode logic and fetches links, to
     * compute the actual parameters of extrusion. Dir property is updated in
     * the process, hence the function is non-const.
     */
    ExtrusionParameters computeFinalParameters();

    static Base::Vector3d calculateShapeNormal(const App::PropertyLink &shapeLink);

public: //mode enumerations
    enum eDirMode{
        dmCustom,
        dmEdge,
        dmNormal
    };
    static const char* eDirModeStrings[];
    static void makeDraft(const ExtrusionParameters& params, const TopoShape&, 
            std::vector<TopoShape>&, App::StringHasherRef hasher);

protected:
    virtual void setupObject() override;
};

/**
 * @brief FaceMakerExtrusion provides legacy compounding-structure-ignorant behavior of facemaker of Part Extrude.
 * Strengths: makes faces with holes
 * Weaknesses: can't make islands in holes. Ignores compounding nesting. All faces must be on same plane.
 */
class FaceMakerExtrusion: public FaceMakerCheese
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public:
    virtual std::string getUserFriendlyName() const override;
    virtual std::string getBriefExplanation() const override;

    virtual void Build() override;
protected:
    virtual void Build_Essence() override {}
};

} //namespace Part


#endif // PART_FEATUREEXTRUSION_H
