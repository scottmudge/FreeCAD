/***************************************************************************
 *   Copyright (c) 2014 Yorik van Havre <yorik@uncreated.net>              *
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
#endif

#include <App/DocumentObjectGroup.h>
#include <Gui/Control.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Control.h>

#include "TaskDlgPathCompound.h"


using namespace Gui;
using namespace PathGui;

PROPERTY_SOURCE(PathGui::ViewProviderPathCompound, PathGui::ViewProviderPath)

ViewProviderPathCompound::ViewProviderPathCompound()
{
    sPixmap = "Path_Compound";
}

bool ViewProviderPathCompound::setEdit(int ModNum)
{
    Q_UNUSED(ModNum);
    Gui::TaskView::TaskDialog* dlg = new TaskDlgPathCompound(this);
    Gui::Control().showDialog(dlg);
    return true;
}

void ViewProviderPathCompound::unsetEdit(int ModNum)
{
    Q_UNUSED(ModNum);
    // when pressing ESC make sure to close the dialog
    Gui::Control().closeDialog();
}

static void getObjectsInGroup(App::DocumentObject *obj, std::set<App::DocumentObject*> &objs)
{
    if (auto grp = Base::freecad_dynamic_cast<App::DocumentObjectGroup>(obj)) {
        for (auto obj : grp->Group.getValues()) {
            if (objs.insert(obj).second)
                getObjectsInGroup(obj, objs);
        }
    }
}

std::vector<App::DocumentObject*> ViewProviderPathCompound::claimChildren()const
{
    auto feat = static_cast<Path::FeatureCompound *>(getObject());
    auto res = feat->Group.getValues();
    std::set<App::DocumentObject *> objs;
    const auto &groups = feat->Groups.getValues();
    for (auto obj : groups)
        getObjectsInGroup(obj, objs);
    for (auto it = res.begin(); it != res.end();) {
        if (objs.count(*it))
            it = res.erase(it);
        else
            ++it;
    }
    res.insert(res.end(), groups.begin(), groups.end());
    return res;
}

bool ViewProviderPathCompound::canDragObjects() const
{
    return true;
}

void ViewProviderPathCompound::dragObject(App::DocumentObject* obj)
{
    static_cast<Path::FeatureCompound *>(getObject())->removeObject(obj);
}

bool ViewProviderPathCompound::canDropObjects() const
{
    return true;
}

bool ViewProviderPathCompound::canDropObject(App::DocumentObject* obj) const
{
    auto feat = static_cast<Path::FeatureCompound *>(getObject());
    return obj && obj->getNameInDocument()
        && (obj->isDerivedFrom(App::DocumentObjectGroup::getClassTypeId())
            || feat->Group.find(obj->getNameInDocument()));
}

void ViewProviderPathCompound::dropObject(App::DocumentObject* obj)
{
    auto feat = static_cast<Path::FeatureCompound *>(getObject());
    if (obj->isDerivedFrom(App::DocumentObjectGroup::getClassTypeId())
            && !feat->Groups.find(obj->getNameInDocument()))
    {
        auto groups = feat->Groups.getValues();
        groups.push_back(obj);
        feat->Groups.setValues(std::move(groups));
    }
    else if (auto grp = App::GroupExtension::getGroupOfObject(obj)) {
        if (feat->Group.find(grp->getNameInDocument())) {
            grp->getExtensionByType<App::GroupExtension>()->removeObject(obj);
            feat->Group.touch();
        }
    }
}

bool ViewProviderPathCompound::canReorderObject(App::DocumentObject* obj,
                                                App::DocumentObject* before)
{
    auto feat = static_cast<Path::FeatureCompound *>(getObject());
    return canReorderObjectInProperty(&feat->Group, obj, before)
        || canReorderObjectInProperty(&feat->Groups, obj, before);
}

bool ViewProviderPathCompound::reorderObjects(const std::vector<App::DocumentObject*> &objs,
                                              App::DocumentObject* before)
{
    auto feat = static_cast<Path::FeatureCompound *>(getObject());
    return reorderObjectsInProperty(&feat->Group, objs, before)
        || reorderObjectsInProperty(&feat->Groups, objs, before);
}

// Python object -----------------------------------------------------------------------

namespace Gui {
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(PathGui::ViewProviderPathCompoundPython, PathGui::ViewProviderPathCompound)
/// @endcond

// explicit template instantiation
template class PathGuiExport ViewProviderPythonFeatureT<PathGui::ViewProviderPathCompound>;
}
