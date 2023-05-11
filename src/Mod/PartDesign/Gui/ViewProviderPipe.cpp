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
# include <QMenu>
#endif

#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Mod/PartDesign/App/FeaturePipe.h>
#include <Mod/Part/Gui/ReferenceHighlighter.h>

#include "ViewProviderPipe.h"
#include "TaskPipeParameters.h"

using namespace PartDesignGui;

PROPERTY_SOURCE(PartDesignGui::ViewProviderPipe,PartDesignGui::ViewProviderAddSub)

ViewProviderPipe::ViewProviderPipe()
{
    sPixmap = "PartDesign_AdditivePipe";
}

ViewProviderPipe::~ViewProviderPipe()
{
}

std::vector<App::DocumentObject*> ViewProviderPipe::_claimChildren() const
{
    std::vector<App::DocumentObject*> temp;

    PartDesign::Pipe* pcPipe = static_cast<PartDesign::Pipe*>(getObject());

    App::DocumentObject* sketch = pcPipe->Profile.getValue();
    if (sketch && !sketch->isDerivedFrom(PartDesign::Feature::getClassTypeId()))
        temp.push_back(sketch);

    for(App::DocumentObject* obj : pcPipe->Sections.getValues()) {
        if (obj && !obj->isDerivedFrom(PartDesign::Feature::getClassTypeId())) {
            if (std::find(temp.begin(), temp.end(), obj) == temp.end())
                temp.push_back(obj);
        }
    }

    App::DocumentObject* spine = pcPipe->Spine.getValue();
    if (spine && !spine->isDerivedFrom(PartDesign::Feature::getClassTypeId())) {
        if (std::find(temp.begin(), temp.end(), spine) == temp.end())
            temp.push_back(spine);
    }

    App::DocumentObject* auxspine = pcPipe->AuxillerySpine.getValue();
    if (auxspine && !auxspine->isDerivedFrom(PartDesign::Feature::getClassTypeId())) {
        if (std::find(temp.begin(), temp.end(), auxspine) == temp.end())
            temp.push_back(auxspine);
    }

    return temp;
}

void ViewProviderPipe::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    addDefaultAction(menu, QObject::tr("Edit pipe"));
    PartDesignGui::ViewProvider::setupContextMenu(menu, receiver, member);
}

TaskDlgFeatureParameters* ViewProviderPipe::getEditDialog() {
    return new TaskDlgPipeParameters(this, false);
}
