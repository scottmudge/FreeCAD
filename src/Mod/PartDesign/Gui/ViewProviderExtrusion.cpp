/***************************************************************************
 *   Copyright (c) 2010 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
 *   Copyright (c) 2020 Zheng Lei (realthunder)<realthunder.dev@gmail.com> *
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
# include <QAction>
# include <QMenu>
#endif

#include "TaskPadParameters.h"
#include <Mod/PartDesign/App/FeatureExtrusion.h>
#include <Mod/Part/Gui/PartParams.h>
#include "ViewProviderExtrusion.h"
#include "ViewProviderDatum.h"

using namespace PartDesignGui;

PROPERTY_SOURCE(PartDesignGui::ViewProviderExtrusion,PartDesignGui::ViewProviderPad)

ViewProviderExtrusion::ViewProviderExtrusion()
{
    sPixmap = "PartDesign_Extrusion.svg";

    // Default to datum color
    //
    unsigned long shcol = PartGui::PartParams::getDefaultDatumColor();
    App::Color col ( (uint32_t) shcol );
    
    MapFaceColor.setValue(false);
    MapLineColor.setValue(false);
    MapPointColor.setValue(false);
    MapTransparency.setValue(false);

    ShapeColor.setValue(col);
    LineColor.setValue(col);
    PointColor.setValue(col);
    Transparency.setValue(60);
    LineWidth.setValue(1);
}

ViewProviderExtrusion::~ViewProviderExtrusion()
{
}

void ViewProviderExtrusion::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    // Note: This methode couldn't be unified with others because menu entry string
    //       should present united in sources for proper translation and shouldn't be 
    //       constructed on runtime.
    QAction* act;
    act = menu->addAction(QObject::tr("Edit Extrusion"), receiver, member);
    act->setData(QVariant((int)ViewProvider::Default));
    PartDesignGui::ViewProviderSketchBased::setupContextMenu(menu, receiver, member);
}

TaskDlgFeatureParameters *ViewProviderExtrusion::getEditDialog()
{
    return new TaskDlgPadParameters(this, false);
}

std::vector<App::DocumentObject*>
ViewProviderExtrusion::_claimChildren(void) const {
    auto feature = Base::freecad_dynamic_cast<PartDesign::ProfileBased>(getObject());

    if (feature && feature->ClaimChildren.getValue())
        return ViewProviderPad::_claimChildren();

    return {};
}

void ViewProviderExtrusion::updateData(const App::Property* p) {
    auto feat = Base::freecad_dynamic_cast<PartDesign::Extrusion>(getObject());
    if (feat) {
        if (p == &feat->NewSolid) {
            if (IconColor.getValue().getPackedValue() && !feat->NewSolid.getValue())
                signalChangeIcon();
        }
    }
    ViewProviderPad::updateData(p);
}
