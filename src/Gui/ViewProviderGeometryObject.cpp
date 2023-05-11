/***************************************************************************
 *   Copyright (c) 2006 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
#include "ViewProviderDocumentObject.h"

#ifndef _PreComp_
# include <Inventor/SoPickedPoint.h>
# include <Inventor/actions/SoRayPickAction.h>
# include <Inventor/actions/SoSearchAction.h>
# include <Inventor/nodes/SoBaseColor.h>
# include <Inventor/nodes/SoCamera.h>
# include <Inventor/nodes/SoDrawStyle.h>
# include <Inventor/nodes/SoFont.h>
# include <Inventor/nodes/SoMaterial.h>
# include <Inventor/nodes/SoSeparator.h>
# include <Inventor/nodes/SoSwitch.h>
# include <Inventor/nodes/SoDirectionalLight.h>
# include <Inventor/sensors/SoNodeSensor.h>
#endif

#include <Inventor/nodes/SoResetTransform.h>

#include <App/GeoFeature.h>
#include <App/PropertyGeo.h>

#include "ViewProviderGeometryObject.h"
#include "Application.h"
#include "Document.h"
#include "ViewParams.h"

#include "SoFCBoundingBox.h"
#include "SoFCSelection.h"
#include "SoFCUnifiedSelection.h"
#include "View3DInventorViewer.h"


using namespace Gui;

PROPERTY_SOURCE(Gui::ViewProviderGeometryObject, Gui::ViewProviderDragger)

const App::PropertyIntegerConstraint::Constraints intPercent = {0, 100, 5};

ViewProviderGeometryObject::ViewProviderGeometryObject()
    : pcBoundingBox(nullptr)
    , pcBoundSwitch(nullptr)
    , pcBoundColor(nullptr)
    , pcSwitchSensor(nullptr)
{
    float r,g,b;

    if (ViewParams::getRandomColor()){
        // compute a random color in the HSV space
        SbColor color;
        color.setHSVValue(((float)rand()/RAND_MAX), // [0-1]
                          ((float)rand()/RAND_MAX) * 0.3 + 0.2, // [0.2-0.5]
                          ((float)rand()/RAND_MAX) * 0.35 + 0.55); // [0.55-0.9]
        r = color[0];
        g = color[1];
        b = color[2];
    }
    else {
        unsigned long shcol = ViewParams::getDefaultShapeColor();
        r = ((shcol >> 24) & 0xff) / 255.0;
        g = ((shcol >> 16) & 0xff) / 255.0;
        b = ((shcol >> 8) & 0xff) / 255.0;
    }

    int initialTransparency = ViewParams::getDefaultShapeTransparency(); 

    static const char *dogroup = "Display Options";
    static const char *osgroup = "Object Style";

    ADD_PROPERTY_TYPE(ShapeColor, (r, g, b), osgroup, App::Prop_None, "Set shape color");
    ADD_PROPERTY_TYPE(Transparency, (initialTransparency), osgroup, App::Prop_None, "Set object transparency");
    Transparency.setConstraints(&intPercent);
    App::Material mat(App::Material::DEFAULT);
    ADD_PROPERTY_TYPE(ShapeMaterial,(mat), osgroup, App::Prop_None, "Shape material");
    ADD_PROPERTY_TYPE(BoundingBox, (false), dogroup, App::Prop_None, "Display object bounding box");

    pcShapeMaterial = new SoMaterial;
    pcShapeMaterial->diffuseColor.setValue(r, g, b);
    pcShapeMaterial->transparency = float(initialTransparency);
    pcShapeMaterial->ref();

    sPixmap = "Feature";
}

ViewProviderGeometryObject::~ViewProviderGeometryObject()
{
    pcShapeMaterial->unref();
    if(pcBoundingBox)
        pcBoundingBox->unref();
    if(pcBoundSwitch)
        pcBoundSwitch->unref();
    if(pcBoundColor)
        pcBoundColor->unref();
    delete pcSwitchSensor;
}

void ViewProviderGeometryObject::onChanged(const App::Property* prop)
{
    Gui::ColorUpdater colorUpdater;

    // Actually, the properties 'ShapeColor' and 'Transparency' are part of the property 'ShapeMaterial'.
    // Both redundant properties are kept due to more convenience for the user. But we must keep the values
    // consistent of all these properties.
    if (prop == &ShapeColor) {
        const App::Color &c = ShapeColor.getValue();
        pcShapeMaterial->diffuseColor.setValue(c.r, c.g, c.b);
        if (c != ShapeMaterial.getValue().diffuseColor)
            ShapeMaterial.setDiffuseColor(c);
    }
    else if (prop == &Transparency) {
        const App::Material &Mat = ShapeMaterial.getValue();
        long value = (long)(100 * Mat.transparency);
        if (value != Transparency.getValue()) {
            float trans = Transparency.getValue() / 100.0f;
            pcShapeMaterial->transparency = trans;
            ShapeMaterial.setTransparency(trans);
        }
    }
    else if (prop == &ShapeMaterial) {
        if (getObject() && getObject()->testStatus(App::ObjectStatus::TouchOnColorChange))
            getObject()->touch(true);
        const App::Material &Mat = ShapeMaterial.getValue();
        long value = (long)(100 * Mat.transparency);
        if (value != Transparency.getValue())
            Transparency.setValue(value);
        const App::Color& color = Mat.diffuseColor;
        pcShapeMaterial->ambientColor.setValue(Mat.ambientColor.r,Mat.ambientColor.g,Mat.ambientColor.b);
        pcShapeMaterial->diffuseColor.setValue(Mat.diffuseColor.r,Mat.diffuseColor.g,Mat.diffuseColor.b);
        pcShapeMaterial->specularColor.setValue(Mat.specularColor.r,Mat.specularColor.g,Mat.specularColor.b);
        pcShapeMaterial->emissiveColor.setValue(Mat.emissiveColor.r,Mat.emissiveColor.g,Mat.emissiveColor.b);
        pcShapeMaterial->shininess.setValue(Mat.shininess);
        pcShapeMaterial->transparency.setValue(Mat.transparency);
        if (color != ShapeColor.getValue())
            ShapeColor.setValue(Mat.diffuseColor);
        Gui::ColorUpdater::addObject(getObject());
    }
    else if (prop == &BoundingBox) {
        showBoundingBox(BoundingBox.getValue());
    }

    ViewProviderDragger::onChanged(prop);
}

void ViewProviderGeometryObject::attach(App::DocumentObject *pcObj)
{
    ViewProviderDragger::attach(pcObj);
}

void ViewProviderGeometryObject::updateData(const App::Property* prop)
{
    if(prop->isDerivedFrom(App::PropertyComplexGeoData::getClassTypeId()))
        updateBoundingBox();

    ViewProviderDragger::updateData(prop);
}

void ViewProviderGeometryObject::updateBoundingBox() {
    if(pcBoundingBox) {
        Base::BoundBox3d box = this->getBoundingBox(0,0,false);
        if(!box.IsValid())
            return;
        pcBoundingBox->minBounds.setValue(box.MinX, box.MinY, box.MinZ);
        pcBoundingBox->maxBounds.setValue(box.MaxX, box.MaxY, box.MaxZ);
    }
}

SoPickedPointList ViewProviderGeometryObject::getPickedPoints(const SbVec2s& pos, const View3DInventorViewer& viewer,bool pickAll) const
{
    auto root = new SoSeparator;
    root->ref();
    root->addChild(viewer.getHeadlight());
    root->addChild(viewer.getSoRenderManager()->getCamera());
    root->addChild(getRoot());

    SoRayPickAction rp(viewer.getSoRenderManager()->getViewportRegion());
    rp.setPickAll(pickAll);
    rp.setRadius(viewer.getPickRadius());
    rp.setPoint(pos);
    rp.apply(root);
    root->unref();

    // returns a copy of the list
    return rp.getPickedPointList();
}

SoPickedPoint* ViewProviderGeometryObject::getPickedPoint(const SbVec2s& pos, const View3DInventorViewer& viewer) const
{
    auto root = new SoSeparator;
    root->ref();
    root->addChild(viewer.getHeadlight());
    root->addChild(viewer.getSoRenderManager()->getCamera());
    root->addChild(getRoot());

    SoRayPickAction rp(viewer.getSoRenderManager()->getViewportRegion());
    rp.setPoint(pos);
    rp.setRadius(viewer.getPickRadius());
    rp.apply(root);
    root->unref();

    // returns a copy of the point
    SoPickedPoint* pick = rp.getPickedPoint();
    //return (pick ? pick->copy() : 0); // needs the same instance of CRT under MS Windows
    return (pick ? new SoPickedPoint(*pick) : nullptr);
}

unsigned long ViewProviderGeometryObject::getBoundColor() const
{
    return ViewParams::getBoundingBoxColor();
}

void ViewProviderGeometryObject::addBoundSwitch() {
    if(!pcBoundSwitch)
        return;

    if(pcModeSwitch->isOfType(SoFCSwitch::getClassTypeId())) {
        if(pcModeSwitch->findChild(pcBoundSwitch)>=0)
            return;
        pcModeSwitch->addChild(pcBoundSwitch);
        // SoFCSwitch::tailChild is shown together with whichChild as long as
        // whichChild is not -1. Put the bound box there is better then putting
        // as the last node in all mode group node.  For example,
        // Arch.BuildingPart puts a SoTransform inside its mode group, which
        // messes up the bound box display.
        //
        // It is also better than putting the bound switch outside of mode
        // switch like before, because we can easily hide the bound box together
        // with the object, and also good for Link as it won't show the bound
        // box
        static_cast<SoFCSwitch*>(pcModeSwitch)->tailChild = pcModeSwitch->getNumChildren()-1;
        return;
    }

    for(int i=0;i<pcModeSwitch->getNumChildren();++i) {
        auto node = pcModeSwitch->getChild(i);
        if(!node->isOfType(SoGroup::getClassTypeId()))
            continue;
        auto group = static_cast<SoGroup*>(node);
        int idx = group->findChild(pcBoundSwitch);
        if(idx >= 0) {
            // make sure we are added last
            if(idx == group->getNumChildren()-1)
                continue;
            group->removeChild(idx);
        }
        group->addChild(pcBoundSwitch);
    }
}

namespace {
float getBoundBoxFontSize()
{
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/View");
    return hGrp->GetFloat("BoundingBoxFontSize", 10.0);
}
}

void ViewProviderGeometryObject::showBoundingBox(bool show)
{
    if (!pcBoundSwitch && show) {
        unsigned long bbcol = getBoundColor();
        float r,g,b;
        r = ((bbcol >> 24) & 0xff) / 255.0; g = ((bbcol >> 16) & 0xff) / 255.0; b = ((bbcol >> 8) & 0xff) / 255.0;

        pcBoundSwitch = new SoSwitch();
        pcBoundSwitch->ref();
        auto pBoundingSep = new SoSeparator();
        auto lineStyle = new SoDrawStyle;
        lineStyle->lineWidth = 2.0f;
        pBoundingSep->addChild(lineStyle);

        if(!pcBoundColor) {
            pcBoundColor = new SoBaseColor;
            pcBoundColor->ref();
        }
        pcBoundColor->rgb.setValue(r, g, b);
        pBoundingSep->addChild(pcBoundColor);
        auto font = new SoFont();
        font->size.setValue(getBoundBoxFontSize());
        pBoundingSep->addChild(font);

        if(!pcBoundingBox) {
            pcBoundingBox = new SoFCBoundingBox;
            pcBoundingBox->ref();
        }
        pBoundingSep->addChild(pcBoundingBox);
        pcBoundingBox->coordsOn.setValue(false);
        pcBoundingBox->dimensionsOn.setValue(true);

        // add to the highlight node
        pcBoundSwitch->addChild(pBoundingSep);

        updateBoundingBox();

        addBoundSwitch();
        pcSwitchSensor = new SoNodeSensor;
        pcSwitchSensor->setData(this);
        pcSwitchSensor->attach(pcModeSwitch);
        pcSwitchSensor->setFunction([](void *data, SoSensor*) {
            reinterpret_cast<ViewProviderGeometryObject*>(data)->addBoundSwitch();
        });
    }

    if (pcBoundSwitch) {
        pcBoundSwitch->whichChild = (show ? 0 : -1);
    }
}
