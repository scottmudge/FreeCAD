/***************************************************************************
 *   Copyright (c) 2013 Jan Rheinländer                                    *
 *                                   <jrheinlaender@users.sourceforge.net> *
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
# include <QMenu>
# include <QMessageBox>
#endif

#include "ViewProviderBoolean.h"
#include "TaskBooleanParameters.h"
#include <Mod/PartDesign/App/FeatureBoolean.h>
#include <Gui/Application.h>
#include <Gui/Control.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/SoFCUnifiedSelection.h>


using namespace PartDesignGui;

PROPERTY_SOURCE_WITH_EXTENSIONS(PartDesignGui::ViewProviderBoolean,PartDesignGui::ViewProvider)

const char* PartDesignGui::ViewProviderBoolean::DisplayEnum[] = {"Result","Tools",nullptr};


ViewProviderBoolean::ViewProviderBoolean()
{
    sPixmap = "PartDesign_Boolean.svg";
    Gui::ViewProviderGeoFeatureGroupExtension::initExtension(this);

    ADD_PROPERTY(Display,((long)0));
    Display.setEnums(DisplayEnum);

    if(pcModeSwitch->isOfType(SoFCSwitch::getClassTypeId()))
        static_cast<SoFCSwitch*>(pcModeSwitch)->defaultChild = 1;
}

ViewProviderBoolean::~ViewProviderBoolean()
{
}


void ViewProviderBoolean::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    addDefaultAction(menu, QObject::tr("Edit boolean"));
    PartDesignGui::ViewProvider::setupContextMenu(menu, receiver, member);
}

TaskDlgFeatureParameters *ViewProviderBoolean::getEditDialog()
{
    return new TaskDlgBooleanParameters( this );
}

void ViewProviderBoolean::attach(App::DocumentObject* obj) {
    PartGui::ViewProviderPartExt::attach(obj);

    //set default display mode to override the "Group" display mode
    setDisplayMode("Flat Lines");
}

void ViewProviderBoolean::onChanged(const App::Property* prop) {

    PartDesignGui::ViewProvider::onChanged(prop);

    if(prop == &Display) {

        if(Display.getValue() == 0) {
            const char *mode = DisplayMode.getValueAsString();
            if (mode && strcmp(mode, "Group") == 0)
                setDisplayMode("Flat Lines");
        } else {
            setDisplayMode("Group");
        }
    } else if (prop == &DisplayMode) {
        const char *mode = DisplayMode.getValueAsString();
        if (isRestoring()) {
            if (Display.getValue() == 0) {
                // Because an old bug where DisplayMode is not synced by
                // setDisplayMode(), we shall respect Display value over
                // DisplayMode.
                if (mode && strcmp(mode, "Group") == 0)
                    setDisplayMode("Flat Lines");
            } else if (mode && strcmp(mode, "Group") != 0)
                setDisplayMode("Group");
        } else if (mode && strcmp(mode, "Group") == 0) {
            Display.setValue((long)1);
        } else
            Display.setValue((long)0);
    }
}

void ViewProviderBoolean::extensionModeSwitchChange()
{
    // Skip ViewProviderGeoFeatureExtension::extensionModeSwitchChange() so
    // that GroupExtension::extensionGetSubObjects(GS_SELECT) can work
    // TODO: find a better work around
}
