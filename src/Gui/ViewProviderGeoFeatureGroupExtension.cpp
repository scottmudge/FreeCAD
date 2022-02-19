/***************************************************************************
 *   Copyright (c) 2011 Jürgen Riegel <FreeCAD@juergen-riegel.net>         *
 *   Copyright (c) 2015 Alexander Golubev (Fat-Zer) <fatzer2@gmail.com>    *
 *   Copyright (c) 2016 Stefan Tröger <stefantroeger@gmx.net>              *
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

#include "ViewProviderGeoFeatureGroupExtension.h"
#include "ViewProviderLink.h"
#include "ViewParams.h"
#include "View3DInventor.h"
#include "Command.h"
#include "Application.h"
#include "Document.h"
#include <App/Application.h>
#include <App/DocumentObserver.h>
#include <App/GeoFeatureGroupExtension.h>
#include "SoFCUnifiedSelection.h"

using namespace Gui;

struct ViewProviderGeoFeatureGroupExtension::Private {
    std::vector<boost::signals2::scoped_connection> conns;
};

EXTENSION_PROPERTY_SOURCE(Gui::ViewProviderGeoFeatureGroupExtension, Gui::ViewProviderGroupExtension)

ViewProviderGeoFeatureGroupExtension::ViewProviderGeoFeatureGroupExtension()
    :impl(new Private), linkView(0)
{
    initExtensionType(ViewProviderGeoFeatureGroupExtension::getExtensionClassTypeId());

    if(ViewParams::LinkChildrenDirect()) {
        linkView = new LinkView;
        pcGroupChildren = linkView->getLinkRoot();
    } else 
        pcGroupChildren = new SoFCSelectionRoot;

    pcGroupChildren->ref();
}

ViewProviderGeoFeatureGroupExtension::~ViewProviderGeoFeatureGroupExtension()
{
    if(linkView)
        linkView->setInvalid();

    impl.reset();

    pcGroupChildren->unref();
    pcGroupChildren = 0;
}

void ViewProviderGeoFeatureGroupExtension::extensionClaimChildren3D(
        std::vector<App::DocumentObject*> &children) const 
{

    //all object in the group must be claimed in 3D, as we are a coordinate system for all of them
    auto* ext = getExtendedViewProvider()->getObject()->getExtensionByType<App::GeoFeatureGroupExtension>();
    if (ext) {
        const auto &objs = ext->Group.getValues();
        children.insert(children.end(),objs.begin(),objs.end());
    }
}

void ViewProviderGeoFeatureGroupExtension::extensionClaimChildren(
        std::vector<App::DocumentObject *> &children) const 
{
    auto* group = getExtendedViewProvider()->getObject()->getExtensionByType<App::GeoFeatureGroupExtension>();
    auto objs = group->_ExportChildren.getValues();
    children.insert(children.end(), objs.begin(), objs.end());
}

void ViewProviderGeoFeatureGroupExtension::extensionFinishRestoring()
{
    auto* group = getExtendedViewProvider()->getObject()->getExtensionByType<App::GeoFeatureGroupExtension>();
    if (!group->_ExportChildren.getSize())
        buildExport();
    ViewProviderGroupExtension::extensionFinishRestoring();
}

void ViewProviderGeoFeatureGroupExtension::extensionAttach(App::DocumentObject* pcObject)
{
    ViewProviderGroupExtension::extensionAttach(pcObject);
    getExtendedViewProvider()->addDisplayMaskMode(pcGroupChildren, "Group");
}

bool ViewProviderGeoFeatureGroupExtension::extensionHandleChildren3D(
        const std::vector<App::DocumentObject*> &) 
{
    if(linkView) {
        buildChildren3D();
        return true;
    }
    return false;
}

void ViewProviderGeoFeatureGroupExtension::buildChildren3D() {
    if(!linkView)
        return;

    auto children = getExtendedViewProvider()->claimChildren3D();
#if 0
    for(auto it=children.begin();it!=children.end();) {
        if(App::GroupExtension::getGroupOfObject(*it))
            it = children.erase(it);
        else
            ++it;
    }
#endif
    linkView->setChildren(children);
} 

bool ViewProviderGeoFeatureGroupExtension::extensionGetElementPicked(
        const SoPickedPoint *pp, std::string &element) const 
{
    if(linkView) 
        return linkView->linkGetElementPicked(pp,element);
    return false;
}

bool ViewProviderGeoFeatureGroupExtension::extensionGetDetailPath(
        const char *subname, SoFullPath *path, SoDetail *&det) const 
{
    if(linkView)
        return linkView->linkGetDetailPath(subname,path,det);
    return false;
}

void ViewProviderGeoFeatureGroupExtension::extensionSetDisplayMode(const char* ModeName)
{
    if ( strcmp("Group",ModeName)==0 )
        getExtendedViewProvider()->setDisplayMaskMode("Group");

    ViewProviderGroupExtension::extensionSetDisplayMode( ModeName );
}

void ViewProviderGeoFeatureGroupExtension::extensionGetDisplayModes(std::vector<std::string> &StrList) const
{
    // get the modes of the father
    ViewProviderGroupExtension::extensionGetDisplayModes(StrList);

    // add your own modes
    StrList.push_back("Group");
}

void ViewProviderGeoFeatureGroupExtension::extensionUpdateData(const App::Property* prop)
{
    auto obj = getExtendedViewProvider()->getObject();
    auto group = obj->getExtensionByType<App::GeoFeatureGroupExtension>();
    if(group) {
        if (prop == &group->_GroupTouched) {
            if (!obj->getDocument()->testStatus(App::Document::Restoring))
                buildExport();
        } else if (prop == &group->Group) {

            if (!obj->getDocument()->testStatus(App::Document::Restoring))
                buildExport();

            impl->conns.clear();
            if(linkView) {
                for(auto obj : group->Group.getValues()) {
                    // check for plain group
                    if(!obj || !obj->getNameInDocument())
                        continue;
                    auto ext = App::GeoFeatureGroupExtension::getNonGeoGroup(obj);
                    if(!ext)
                        continue;
                    impl->conns.push_back(
                            ext->Group.signalChanged.connect([=](const App::Property &){
                                this->buildChildren3D();
                        }));
                }
            }
        } else if(prop == &group->placement()) 
            getExtendedViewProvider()->setTransformation ( group->placement().getValue().toMatrix() );
    }
    ViewProviderGroupExtension::extensionUpdateData ( prop );
}

static void filterLinksByScope(const App::DocumentObject *obj, std::vector<App::DocumentObject *> &children)
{
    if(!obj || !obj->getNameInDocument())
        return;

    std::vector<App::Property*> list;
    obj->getPropertyList(list);
    std::set<App::DocumentObject *> links;
    for(auto prop : list) {
        auto link = Base::freecad_dynamic_cast<App::PropertyLinkBase>(prop);
        if(link && link->getScope()!=App::LinkScope::Global)
            link->getLinkedObjects(std::inserter(links,links.end()),true);
    }
    for(auto it=children.begin();it!=children.end();) {
        if(!links.count(*it))
            it = children.erase(it);
        else
            ++it;
    }
}

void ViewProviderGeoFeatureGroupExtension::buildExport() const {
    App::DocumentObject * obj = getExtendedViewProvider()->getObject();
    auto* group = obj->getExtensionByType<App::GeoFeatureGroupExtension>();
    if(!group)
        return;

    if(obj->testStatus(App::PendingRecompute) && !obj->isRecomputing())
        return;

    auto model = group->Group.getValues ();
    std::set<App::DocumentObject*> outSet; //< set of objects not to claim (childrens of childrens)

    // search for objects handled (claimed) by the features
    for (auto obj: model) {
        if (!obj || !shouldCheckExport(obj)) { continue; }

        Gui::ViewProvider* vp = Gui::Application::Instance->getViewProvider ( obj );
        if (!vp || vp == getExtendedViewProvider()) { continue; }

        auto children = vp->claimChildren();
        filterLinksByScope(obj,children);
        outSet.insert(children.begin(),children.end());
    }

    // remove the otherwise handled objects, preserving their order so the order in the TreeWidget is correct
    for(auto it=model.begin();it!=model.end();) {
        auto obj = *it;
        if(!obj || !obj->getNameInDocument() || outSet.count(obj))
            it = model.erase(it);
        else
            ++it;
    }

    if(group->_ExportChildren.getValues()!=model)
        group->_ExportChildren.setValues(std::move(model));
}

bool ViewProviderGeoFeatureGroupExtension::shouldCheckExport(App::DocumentObject *obj) const
{
    //By default, do not check geofeaturegroup children for export exclusion,
    //because stuff in another geofeaturegroup is normally not in the model
    return !obj->hasExtension(App::GeoFeatureGroupExtension::getExtensionClassTypeId());
}

void ViewProviderGeoFeatureGroupExtension::extensionModeSwitchChange(void)
{
    auto vp = getExtendedViewProvider();
    if (auto obj = vp->getObject()) {
        auto ext = obj->getExtensionByType<App::GeoFeatureGroupExtension>();
        int mode = vp->getDefaultMode(true);
        ext->enableSelectionSubObjects(
                mode >= 0 && pcGroupChildren == vp->getModeSwitch()->getChild(mode));
    }
}

bool ViewProviderGeoFeatureGroupExtension::needUpdateChildren(App::DocumentObject *obj)
{
    bool found = false;
    for (auto o : obj->getInList()) {
        auto geogroup = o->getExtensionByType<App::GeoFeatureGroupExtension>(true);
        if (geogroup) {
            auto vp = Application::Instance->getViewProvider(o);
            if (vp) {
                auto ext = vp->getExtensionByType<ViewProviderGeoFeatureGroupExtension>(true);
                if (ext)
                    ext->buildExport();
            }
            return true;
        }
        if (!found && o->hasExtension(App::GroupExtension::getExtensionClassTypeId()))
            found = true;
    }
    return found;
}

namespace Gui {
EXTENSION_PROPERTY_SOURCE_TEMPLATE(Gui::ViewProviderGeoFeatureGroupExtensionPython, Gui::ViewProviderGeoFeatureGroupExtension)

// explicit template instantiation
template class GuiExport ViewProviderExtensionPythonT<ViewProviderGeoFeatureGroupExtension>;
}
