/****************************************************************************
 *   Copyright (c) 2021 Zheng Lei (realthunder) <realthunder.dev@gmail.com> *
 *                                                                          *
 *   This file is part of the FreeCAD CAx development system.               *
 *                                                                          *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Library General Public            *
 *   License as published by the Free Software Foundation; either           *
 *   version 2 of the License, or (at your option) any later version.       *
 *                                                                          *
 *   This library  is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Library General Public License for more details.                   *
 *                                                                          *
 *   You should have received a copy of the GNU Library General Public      *
 *   License along with this library; see the file COPYING.LIB. If not,     *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,          *
 *   Suite 330, Boston, MA  02111-1307, USA                                 *
 *                                                                          *
 ****************************************************************************/

#include "PreCompiled.h"
#ifndef _PreComp_
# include <QMouseEvent>
# include <QByteArray>
# include <QMenu>
#endif

#include <boost/algorithm/string/predicate.hpp>
#include <App/SavedView.h>
#include <App/AutoTransaction.h>
#include <App/GroupExtension.h>
#include "View3DInventor.h"
#include "View3DInventorViewer.h"
#include "Clipping.h"
#include "BitmapFactory.h"
#include "Application.h"
#include "Document.h"
#include "Command.h"
#include "ViewParams.h"
#include "ViewProviderSavedView.h"
#include "ViewProviderSavedViewPy.h"

using namespace Gui;
using namespace Gui::Dialog;

PROPERTY_SOURCE(Gui::ViewProviderSavedView, Gui::ViewProviderDocumentObject)

ViewProviderSavedView::ViewProviderSavedView()
{
    sPixmap = "SavedView";
}

ViewProviderSavedView::~ViewProviderSavedView()
{
}

void ViewProviderSavedView::setupContextMenu(QMenu *menu, QObject*, const char*)
{
    prepareMenu(menu);
}

void ViewProviderSavedView::prepareMenu(QMenu *menu)
{
    auto obj = Base::freecad_dynamic_cast<App::SavedView>(getObject());
    if (!obj)
        return;
    menu->addAction(QObject::tr("Capture"), [this]() {capture();});
    menu->addAction(QObject::tr("Capture clip planes"), [this]() {capture(CaptureOption::Clippings);});
    menu->addAction(QObject::tr("Capture camera"), [this]() {capture(CaptureOption::Camera);});
    menu->addAction(QObject::tr("Capture visibilities"), [this]() {capture(CaptureOption::Visibilities);});
    menu->addAction(QObject::tr("Capture shadow settings"), [this]() {capture(CaptureOption::Shadow);});
    menu->addAction(QObject::tr("Capture all"), [this]() {capture(CaptureOption::All);});
    menu->addSeparator();
    menu->addAction(QObject::tr("Restore"), [this]() {apply();});
    menu->addAction(QObject::tr("Restore clip planes"), [this]() {apply(CaptureOption::Clippings);});
    menu->addAction(QObject::tr("Restore camera"), [this]() {apply(CaptureOption::Camera);});
    menu->addAction(QObject::tr("Restore visibilities"), [this]() {apply(CaptureOption::Visibilities);});
    menu->addAction(QObject::tr("Restore shadow settings"), [this]() {apply(CaptureOption::Shadow);});
    menu->addAction(QObject::tr("Restore all"), [this]() {apply(CaptureOption::All);});
}

bool ViewProviderSavedView::doubleClicked()
{
    apply();
    return true;
}

static const QByteArray &_captureIconTag()
{
    static QByteArray _tag("SavedView:capture");
    return _tag;
}

void ViewProviderSavedView::getExtraIcons(std::vector<std::pair<QByteArray, QPixmap> > &icons) const
{
    icons.emplace_back(_captureIconTag(), Gui::BitmapFactory().pixmap("SaveView"));
    inherited::getExtraIcons(icons);
}

bool ViewProviderSavedView::iconMouseEvent(QMouseEvent *ev, const QByteArray &tag)
{
    switch (ev->type()) {
    case QEvent::MouseButtonPress:
        if (ev->button() == Qt::LeftButton) {
            if (tag == _captureIconTag()) {
                capture();
                return true;
            }
        }
        break;
    case QEvent::MouseButtonRelease:
        if (ev->button() == Qt::RightButton) {
            QMenu menu;
            prepareMenu(&menu);
            menu.exec(QCursor::pos());
            return true;
        }
        break;
    default:
        break;
    }
    return inherited::iconMouseEvent(ev, tag);
}

QString ViewProviderSavedView::getToolTip(const QByteArray &tag) const
{
    if (tag == _captureIconTag())
        return QObject::tr("Double click to restore the captured view.\n"
                           "ALT + click this icon to capture current view.\n"
                           "ALT + right click for more actions.");
                           
    return inherited::getToolTip(tag);
}

bool ViewProviderSavedView::setEdit(int ModNum)
{
    if (ModNum == ViewProvider::Default) {
        apply();
        return false;
    }
    return inherited::setEdit(ModNum);
}

void ViewProviderSavedView::apply(CaptureOptions options)
{
    try {
        App::AutoTransaction guard("Apply view", false, true);

        auto obj = Base::freecad_dynamic_cast<App::SavedView>(getObject());
        if (!obj)
            throw Base::RuntimeError("Not object attached");
        auto view = Base::freecad_dynamic_cast<View3DInventor>(getActiveView());
        if (!view)
            throw Base::RuntimeError("No 3D view");

        if (options & CaptureOption::Default) {
            if (obj->SaveClippings.getValue())
                options |= CaptureOption::Clippings;
            if (obj->SaveCamera.getValue())
                options |= CaptureOption::Camera;
            if (obj->SaveVisibilities.getValue())
                options |= CaptureOption::Visibilities;
            if (obj->SaveShadowSettings.getValue())
                options |= CaptureOption::Shadow;
        }

        if (options & CaptureOption::Clippings) {
            if (auto prop = obj->getClippingProperty<App::PropertyBool>("ClipFill"))
                ViewParams::setSectionFill(prop->getValue());
            if (auto prop = obj->getClippingProperty<App::PropertyBool>("ClipConcave"))
                ViewParams::setSectionConcave(prop->getValue());
            if (auto prop = obj->getClippingProperty<App::PropertyBool>("ClipHatch"))
                ViewParams::setSectionHatchTextureEnable(prop->getValue());
            if (auto prop = obj->getClippingProperty<App::PropertyFloat>("ClipHatchScale"))
                ViewParams::setSectionHatchTextureScale(prop->getValue());
            if (auto prop = obj->getClippingProperty<App::PropertyString>("ClipHatchTexture"))
                ViewParams::setSectionHatchTexture(prop->getValue());
            if (auto prop = obj->getClippingProperty<App::PropertyBool>("ClipGroup"))
                ViewParams::setSectionFillGroup(prop->getValue());
            if (auto prop = obj->getClippingProperty<App::PropertyBool>("ClipShowPlane"))
                ViewParams::setShowClipPlane(prop->getValue());
            if (auto prop = obj->getClippingProperty<App::PropertyFloat>("ClipPlaneSize"))
                ViewParams::setClipPlaneSize(prop->getValue());
            if (auto prop = obj->getClippingProperty<App::PropertyBool>("BackFaceLight"))
                ViewParams::setEnableBacklight(prop->getValue());
            if (auto prop = obj->getClippingProperty<App::PropertyColor>("BackFaceLightColor"))
                ViewParams::setBacklightColor(prop->getValue().getPackedValue());
            if (auto prop = obj->getClippingProperty<App::PropertyFloat>("BackFaceLightIntensity"))
                ViewParams::setBacklightIntensity(prop->getValue());

            Base::Vector3d posX, posY, posZ, pos;
            Base::Rotation rot;
            bool enableX = false, enableY = false, enableZ = false, enable = false;
            if (auto prop = obj->getClippingProperty<App::PropertyBool>("ClipEnableX"))
                enableX = prop->getValue();
            if (auto prop = obj->getClippingProperty<App::PropertyVector>("ClipPositionX"))
                posX = prop->getValue();
            if (auto prop = obj->getClippingProperty<App::PropertyBool>("ClipEnableY"))
                enableY = prop->getValue();
            if (auto prop = obj->getClippingProperty<App::PropertyVector>("ClipPositionY"))
                posY = prop->getValue();
            if (auto prop = obj->getClippingProperty<App::PropertyBool>("ClipEnableZ"))
                enableZ = prop->getValue();
            if (auto prop = obj->getClippingProperty<App::PropertyVector>("ClipPositionZ"))
                posZ = prop->getValue();
            if (auto prop = obj->getClippingProperty<App::PropertyBool>("ClipEnableCustom"))
                enable = prop->getValue();
            if (auto prop = obj->getClippingProperty<App::PropertyVector>("ClipPositionCustom"))
                pos = prop->getValue();
            if (auto prop = obj->getClippingProperty<App::PropertyRotation>("ClipRotationCustom"))
                rot = prop->getValue();
            Clipping::restoreClipPlanes(view, posX, enableX, posY, enableY, posZ, enableZ, Base::Placement(pos, rot), enable);
        }

        if (options & CaptureOption::Camera) {
            if (auto prop = obj->getCameraProperty<App::PropertyString>("CameraSettings")) {
                try {
                    if (boost::starts_with(prop->getStrValue(), "SetCamera "))
                        view->onMsg(prop->getValue(), nullptr);
                } catch (Base::Exception &e) {
                    e.ReportException();
                }
            }
        }

        if (options & CaptureOption::Shadow) {
            App::Document *doc = obj->getDocument();
            int count = 0;
            for (const auto &name : obj->getDynamicPropertyNames()) {
                if (!boost::starts_with(name, "Shadow_"))
                    continue;
                if (auto prop = obj->getPropertyByName(name.c_str())) {
                    auto p = doc->getPropertyByName(name.c_str());
                    if (!p)
                        p = doc->addDynamicProperty(prop->getTypeId().getName(),
                                                    prop->getName(),
                                                    prop->getGroup(),
                                                    prop->getDocumentation());
                    if (!p || p->getTypeId() != prop->getTypeId())
                        continue;
                    p->Paste(*prop);
                    ++count;
                }
            }
            if (!(options & CaptureOption::Camera))
                view->getViewer()->setOverrideMode("Shadow");
        }

        if (options & CaptureOption::Visibilities) {
            if (auto prop = obj->getVisibilityProperty<App::PropertyStringList>("Visibilities")) {
                App::DocumentObject *lastObject = nullptr;
                App::Document *doc = obj->getDocument();
                for (const auto &s : prop->getValues()) {
                    const char *value = s.c_str();
                    bool visible = !boost::starts_with(value, "*");
                    if (!visible)
                        ++value;
                    if (boost::starts_with(value, ".")) {
                        if (lastObject)
                            lastObject->setElementVisible(value+1, visible);
                        continue;
                    }
                    const char *dot = strchr(value, '.');
                    if (dot) {
                        auto parent = doc->getObject(std::string(value, dot-value).c_str());
                        if (parent)
                            parent->setElementVisible(dot+1, visible);
                        continue;
                    }
                    lastObject = doc->getObject(value);
                    if (lastObject)
                        lastObject->Visibility.setValue(visible);
                }
            }
        }

        Command::updateActive();
    } catch (Base::Exception &e) {
        e.ReportException();
    }
}

void ViewProviderSavedView::capture(CaptureOptions options) 
{
    try {
        App::AutoTransaction guard("Capture view");

        auto obj = Base::freecad_dynamic_cast<App::SavedView>(getObject());
        if (!obj)
            throw Base::RuntimeError("Not object attached");
        auto view = Base::freecad_dynamic_cast<View3DInventor>(getActiveView());
        if (!view)
            throw Base::RuntimeError("No 3D view");

        if (options & CaptureOption::Default) {
            if (obj->SaveClippings.getValue())
                options |= CaptureOption::Clippings;
            if (obj->SaveCamera.getValue())
                options |= CaptureOption::Camera;
            if (obj->SaveVisibilities.getValue())
                options |= CaptureOption::Visibilities;
            if (obj->SaveShadowSettings.getValue())
                options |= CaptureOption::Shadow;
        }

        if (options & CaptureOption::Clippings) {
            obj->SaveClippings.setValue(true);

            Base::Vector3d posX, posY, posZ;
            Base::Placement pla;
            Base::Rotation rotate;
            bool enableX = false, enableY = false, enableZ = false, enableClip = false;
            Clipping::getClipPlanes(view,
                                    posX, enableX,
                                    posY, enableY,
                                    posZ, enableZ,
                                    pla, enableClip);
            obj->getClippingProperty<App::PropertyBool>("ClipEnableX", true)->setValue(enableX);
            obj->getClippingProperty<App::PropertyBool>("ClipEnableY", true)->setValue(enableY);
            obj->getClippingProperty<App::PropertyBool>("ClipEnableZ", true)->setValue(enableZ);
            obj->getClippingProperty<App::PropertyBool>("ClipEnable", true)->setValue(enableClip);
            obj->getClippingProperty<App::PropertyVector>("ClipPositionX", true)->setValue(posX);
            obj->getClippingProperty<App::PropertyVector>("ClipPositionY", true)->setValue(posY);
            obj->getClippingProperty<App::PropertyVector>("ClipPositionZ", true)->setValue(posZ);
            obj->getClippingProperty<App::PropertyVector>("ClipPosition", true)->setValue(pla.getPosition());
            obj->getClippingProperty<App::PropertyRotation>("ClipRotation", true)->setValue(pla.getRotation());
            obj->getClippingProperty<App::PropertyBool>("ClipFill", true)->setValue(ViewParams::SectionFill());
            obj->getClippingProperty<App::PropertyBool>("ClipConcave", true)->setValue(ViewParams::SectionConcave());
            obj->getClippingProperty<App::PropertyBool>("ClipHatch", true)->setValue(ViewParams::SectionHatchTextureEnable());
            obj->getClippingProperty<App::PropertyFloat>("ClipHatchScale", true)->setValue(ViewParams::SectionHatchTextureScale());
            obj->getClippingProperty<App::PropertyString>("ClipHatchTexture", true)->setValue(ViewParams::SectionHatchTexture());
            obj->getClippingProperty<App::PropertyBool>("ClipGroup", true)->setValue(ViewParams::SectionFillGroup());
            obj->getClippingProperty<App::PropertyBool>("ClipShowPlane", true)->setValue(ViewParams::ShowClipPlane());
            obj->getClippingProperty<App::PropertyFloat>("ClipPlaneSize", true)->setValue(ViewParams::ClipPlaneSize());
            obj->getClippingProperty<App::PropertyBool>("BackLight", true)->setValue(ViewParams::EnableBacklight());
            obj->getClippingProperty<App::PropertyColor>("BackLightColor", true)->setValue(ViewParams::BacklightColor());
            obj->getClippingProperty<App::PropertyFloat>("BackLightIntensity", true)->setValue(ViewParams::BacklightIntensity());
        }

        if (options & CaptureOption::Camera) {
            obj->SaveCamera.setValue(true);
            const char *cameraSettings = nullptr;
            view->onMsg("GetCamera", &cameraSettings);
            if (cameraSettings) {
                std::string s;
                getDocument()->saveCameraSettings(cameraSettings, &s);
                obj->getCameraProperty<App::PropertyString>("CameraSettings", true)->setValue(s);
            }
        }

        if (options & CaptureOption::Shadow) {
            obj->SaveShadowSettings.setValue(true);
            auto doc = obj->getDocument();
            for (const auto &name : doc->getDynamicPropertyNames()) {
                if (!boost::starts_with(name, "Shadow_"))
                    continue;
                auto prop = doc->getPropertyByName(name.c_str());
                if (!prop)
                    continue;
                auto p = obj->getProperty(prop->getTypeId(), prop->getName(), prop->getGroup(), true);
                p->Paste(*prop);
            }
        }

        if (options & CaptureOption::Visibilities) {
            obj->SaveVisibilities.setValue(true);
            std::vector<std::string> values;
            std::ostringstream ss;
            std::string childName;
            for (auto o : obj->getDocument()->getObjects()) {
                ss.str("");
                if (!o->Visibility.getValue())
                    ss << "*";
                ss << o->getNameInDocument();
                values.push_back(ss.str());
                if (o->hasExtension(App::GroupExtension::getExtensionClassTypeId()))
                    continue;
                for (auto sub : o->getSubObjects(App::DocumentObject::GS_SELECT)) {
                    App::DocumentObject *parent = nullptr;
                    childName.clear();
                    auto sobj = obj->resolve(sub.c_str(),&parent,&childName);
                    int vis;
                    if(!sobj || !parent
                            || parent->getDocument() != obj->getDocument()
                            || (vis=parent->isElementVisible(childName.c_str()))<0)
                        continue;
                    ss.str("");
                    if (!vis)
                        ss << "*";
                    if (parent != o)
                        ss << parent->getNameInDocument();
                    ss << "." << childName;
                    values.push_back(ss.str());
                }
            }
            obj->getVisibilityProperty<App::PropertyStringList>("Visibilities", true)->setValues(std::move(values));
        }
    } catch (Base::Exception &e) {
        e.ReportException();
    }
}

PyObject *ViewProviderSavedView::getPyObject() {
    if (!pyViewObject)
        pyViewObject = new ViewProviderSavedViewPy(this);
    pyViewObject->IncRef();
    return pyViewObject;
}

