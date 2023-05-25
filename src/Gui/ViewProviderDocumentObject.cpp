/***************************************************************************
 *   Copyright (c) 2004 Jürgen Riegel <juergen.riegel@web.de>              *
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
# include <QByteArray>
# include <QMenu>
# include <QMouseEvent>
# include <QPixmap>
# include <Inventor/SoFullPath.h>
# include <Inventor/SoPickedPoint.h>
# include <Inventor/actions/SoSearchAction.h>
# include <Inventor/details/SoDetail.h>
# include <Inventor/misc/SoChildList.h>
# include <Inventor/nodes/SoSeparator.h>
#endif

#include <Inventor/annex/FXViz/nodes/SoShadowStyle.h>

#include <App/AutoTransaction.h>
#include <App/Document.h>
#include <App/DocumentObserver.h>
#include <App/GeoFeatureGroupExtension.h>
#include <App/Material.h>
#include <App/Origin.h>
#include <Base/Console.h>
#include <Base/Tools.h>

#include "ViewProviderDocumentObjectPy.h"
#include "ActionFunction.h"
#include "Application.h"
#include "Command.h"
#include "Document.h"
#include "SoFCUnifiedSelection.h"
#include "SoFCSelection.h"
#include "MDIView.h"
#include "View3DInventor.h"
#include "View3DInventorViewer.h"
#include "ViewProviderGeoFeatureGroupExtension.h"
#include "ViewParams.h"
#include "ViewProviderLink.h"
#include "BitmapFactory.h"
#include <Gui/ViewProviderDocumentObjectPy.h>
#include "ViewProviderDocumentObject.h"
#include "ViewProviderExtension.h"
#include "TaskView/TaskAppearance.h"

FC_LOG_LEVEL_INIT("Gui", true, true)

using namespace Gui;

///////////////////////////////////////////////////////////////////////////
//
static int _ColorUpdateCounter;
static std::set<App::DocumentObjectT> _ColorChangedObjects;

ColorUpdater::ColorUpdater()
{
    if (_ColorUpdateCounter >= 0)
        ++_ColorUpdateCounter;
}

ColorUpdater::~ColorUpdater()
{
    if (_ColorUpdateCounter <= 0
            || --_ColorUpdateCounter > 0
            || _ColorChangedObjects.empty())
        return;
    // To prevent infinite recursive update
    _ColorUpdateCounter = -1;
    try {
        std::set<App::DocumentObject*> inset;
        for (const auto &objT : _ColorChangedObjects) {
            if (auto obj = objT.getObject()) {
                if (!obj->isRecomputing() && !obj->isTouched())
                    obj->getInListEx(inset, true);
            }
        }
        std::vector<App::DocumentObject*> objs(inset.begin(), inset.end());
        for (auto obj : App::Document::getDependencyList(objs, App::Document::DepSort)) {
            if (obj->isRecomputing() || obj->isTouched() || !inset.count(obj))
                continue;
            if (auto vp = Base::freecad_dynamic_cast<ViewProviderDocumentObject>(
                        Gui::Application::Instance->getViewProvider(obj))) {
                vp->checkColorUpdate();
            }
        }
    } catch (Base::Exception &e) {
        e.ReportException();
    } catch (...)
    {}
    _ColorChangedObjects.clear();
    _ColorUpdateCounter = 0;
}

void ColorUpdater::addObject(App::DocumentObject *obj)
{
    if (_ColorUpdateCounter && obj && !obj->isRecomputing() && !obj->isTouched())
        _ColorChangedObjects.emplace(obj);
}


///////////////////////////////////////////////////////////////////////////////////

PROPERTY_SOURCE(Gui::ViewProviderDocumentObject, Gui::ViewProvider)

ViewProviderDocumentObject::ViewProviderDocumentObject()
  : pcObject(nullptr)
  , pcDocument(nullptr)
{
    static const char *dogroup = "Display Options";
    static const char *sgroup = "Selection";

    ADD_PROPERTY_TYPE(DisplayMode, ((long)0), dogroup, App::Prop_None, "Set the display mode");
    ADD_PROPERTY_TYPE(Visibility, (true), dogroup, App::Prop_None, "Show the object in the 3d view");
    ADD_PROPERTY_TYPE(ShowInTree, (true), dogroup, App::Prop_None, "Show the object in the tree view");

    ADD_PROPERTY_TYPE(SelectionStyle, ((long)0), sgroup, App::Prop_None, "Set the object selection style");
    static const char *SelectionStyleEnum[] = {"Shape","BoundBox",nullptr};
    SelectionStyle.setEnums(SelectionStyleEnum);

    static const char *ShadowStyleEnum[] = {
        "Cast shadow and shadowed","Cast shadow", "Shadowed", "No shadowing", nullptr};
    ShadowStyle.setEnums(ShadowStyleEnum);
    ADD_PROPERTY_TYPE(ShadowStyle,(0L), dogroup, App::Prop_None,
            "Cast shadow and shadowed: Cast shadow and receive shadows.\n"
            "Cast shadow: Only cast shadow, but not receive any shadow.\n"
            "Shadowed: Only receive shadow, but not cast any shadow.\n"
            "No shadowing: Neither cast nor receive any shadow.");

    ADD_PROPERTY_TYPE(ResetClipPlane, (false), dogroup, App::Prop_None,
            "Clear current clipping plans");

    ADD_PROPERTY_TYPE(Selectable, (true), sgroup, App::Prop_None, "Set if the object is selectable in the 3d view");
    Selectable.setValue(ViewParams::getEnableSelection());

    static const char* OnTopEnum[]= {"Disabled","Enabled","Object","Element",nullptr};
    ADD_PROPERTY_TYPE(OnTopWhenSelected,((long int)0), sgroup, App::Prop_None,
            "Enabled: Display the object on top of any other object when selected\n"
            "Object: On top only if the whole object is selected\n"
            "Element: On top only if some sub-element of the object is selected");
    OnTopWhenSelected.setEnums(OnTopEnum);

    sPixmap = "Feature";
}

ViewProviderDocumentObject::~ViewProviderDocumentObject()
{
}

void ViewProviderDocumentObject::getTaskViewContent(std::vector<Gui::TaskView::TaskContent*>& vec) const
{
    vec.push_back(new Gui::TaskView::TaskAppearance());
}

void ViewProviderDocumentObject::startRestoring()
{
    _VisibilityRestored = false;
    hide();
    callExtension(&ViewProviderExtension::extensionStartRestoring);
}

void ViewProviderDocumentObject::finishRestoring()
{
    if (!_VisibilityRestored)
        Visibility.setValue(getObject()->Visibility.getValue());
    callExtension(&ViewProviderExtension::extensionFinishRestoring);
}

bool ViewProviderDocumentObject::isAttachedToDocument() const
{
    return (!testStatus(Detach));
}

const char* ViewProviderDocumentObject::detachFromDocument()
{
    // here we can return an empty string since the object
    // name comes from the document object
    setStatus(Detach, true);
    return "";
}

bool ViewProviderDocumentObject::removeDynamicProperty(const char* name)
{
    App::Property* prop = getDynamicPropertyByName(name);
    if(!prop || prop->testStatus(App::Property::LockDynamic))
        return false;

    // transactions of view providers are also managed in App::Document.
    App::DocumentObject* docobject = getObject();
    App::Document* document = docobject ? docobject->getDocument() : nullptr;
    if (document)
        document->addOrRemovePropertyOfObject(this, prop, false);

    return ViewProvider::removeDynamicProperty(name);
}

App::Property* ViewProviderDocumentObject::addDynamicProperty(
    const char* type, const char* name, const char* group, const char* doc,
    short attr, bool ro, bool hidden)
{
    auto prop = ViewProvider::addDynamicProperty(type,name,group,doc,attr,ro,hidden);
    if(prop) {
        // transactions of view providers are also managed in App::Document.
        App::DocumentObject* docobject = getObject();
        App::Document* document = docobject ? docobject->getDocument() : nullptr;
        if (document)
            document->addOrRemovePropertyOfObject(this, prop, true);
    }
    return prop;
}

void ViewProviderDocumentObject::onBeforeChange(const App::Property* prop)
{
    if (isAttachedToDocument() && !testStatus(SecondaryView)) {
        App::DocumentObject* obj = getObject();
        App::Document* doc = obj ? obj->getDocument() : nullptr;
        if (doc) {
            onBeforeChangeProperty(doc, prop);
        }
    }

    ViewProvider::onBeforeChange(prop);
}

void ViewProviderDocumentObject::onChanged(const App::Property* prop)
{
    if (prop == &DisplayMode) {
        setActiveMode();
    }
    else if (prop == &ShadowStyle) {
        if(!pcShadowStyle && ShadowStyle.getValue()!=0) {
            pcShadowStyle = new SoShadowStyle();
            pcRoot->insertChild(pcShadowStyle,0);
        }
        switch(ShadowStyle.getValue()) {
        case 0:
            if(pcShadowStyle)
                pcShadowStyle->style = SoShadowStyle::CASTS_SHADOW_AND_SHADOWED;
            break;
        case 1:
            pcShadowStyle->style = SoShadowStyle::CASTS_SHADOW;
            break;
        case 2:
            pcShadowStyle->style = SoShadowStyle::SHADOWED;
            break;
        case 3:
            pcShadowStyle->style = SoShadowStyle::NO_SHADOWING;
            break;
        }
    }
    else if (prop == &Visibility) {
        if (isRestoring())
            _VisibilityRestored = true;
        // use this bit to check whether show() or hide() must be called
        if (!Visibility.testStatus(App::Property::User2)) {
            Base::ObjectStatusLocker<App::Property::Status,App::Property>
                guard(App::Property::User2, &Visibility);
            Visibility.getValue() ? show() : hide();
        }
        if (!Visibility.testStatus(App::Property::User1)
                && !testStatus(SecondaryView)
                && getObject()
                && getObject()->Visibility.getValue()!=Visibility.getValue())
        {
            // Changing the visibility of a document object will automatically set
            // the document modified but if the 'TouchDocument' flag is not set then
            // this is undesired behaviour. So, if this change marks the document as
            // modified then it must be be reversed.
            if (!testStatus(Gui::ViewStatus::TouchDocument)) {
                // Note: reverting document modified status like that is not
                // appropriate because we can't tell if there is any other
                // property being changed due to the change of Visibility here.
                // Temporary setting the Visibility property as 'NoModify' is
                // the proper way.
                Base::ObjectStatusLocker<App::Property::Status,App::Property> guard(
                        App::Property::NoModify, &Visibility);
                // bool mod = false;
                // if (pcDocument)
                //     mod = pcDocument->isModified();
                getObject()->Visibility.setValue(Visibility.getValue());
                // if (pcDocument)
                //     pcDocument->setModified(mod);
            }
            else {
                getObject()->Visibility.setValue(Visibility.getValue());
            }
        }
    }
    else if (prop == &ResetClipPlane) {
        if(getRoot()->isOfType(SoFCSelectionRoot::getClassTypeId())) {
            auto root = static_cast<SoFCSelectionRoot*>(getRoot());
            root->resetClipPlane.setValue(ResetClipPlane.getValue());
        }
    }
    else if (prop == &SelectionStyle || prop == &Selectable) {
        if(getRoot()->isOfType(SoFCSelectionRoot::getClassTypeId())) {
            auto root = static_cast<SoFCSelectionRoot*>(getRoot());
            if(Selectable.getValue()) {
                root->selectionStyle = SelectionStyle.getValue()
                    ? SoFCSelectionRoot::Box : SoFCSelectionRoot::Full;
            } else
                root->selectionStyle = SoFCSelectionRoot::Unpickable;
        }
        if(prop == &Selectable)
            setSelectable(Selectable.getValue());
    }

    if (prop && !prop->testStatus(App::Property::NoModify)
             && pcDocument
             && !pcDocument->isModified()
             && testStatus(Gui::ViewStatus::TouchDocument)) {
        if (prop)
            FC_LOG(prop->getFullName() << " changed");
        pcDocument->setModified(true);
    }

    ViewProvider::onChanged(prop);
    if (pcObject)
        pcObject->ViewObject.touch();
}

void ViewProviderDocumentObject::hide()
{
    auto obj = getObject();
    if(obj && obj->getDocument() && obj->getNameInDocument()
           && !SelectionNoTopParentCheck::enabled())
    {
        Gui::Selection().updateSelection(
                false, obj->getDocument()->getName(), obj->getNameInDocument(),0);
    }

    ViewProvider::hide();

    // use this bit to check whether 'Visibility' must be adjusted
    if (Visibility.getValue() && !Visibility.testStatus(App::Property::User2)) {
        Base::ObjectStatusLocker<App::Property::Status,App::Property>
            guard(App::Property::User2, &Visibility);
        Visibility.setValue(false);
    }
}

void ViewProviderDocumentObject::startDefaultEditMode()
{
    QString text = QObject::tr("Edit %1").arg(QString::fromUtf8(getObject()->Label.getValue()));
    Gui::Command::openCommand(text.toUtf8());

    Gui::Document* document = this->getDocument();
    if (document) {
        document->setEdit(this, ViewProvider::Default);
    }
}

void ViewProviderDocumentObject::addDefaultAction(QMenu* menu, const QString& text)
{
    QAction* act = menu->addAction(text);
    act->setData(QVariant((int)ViewProvider::Default));
    auto func = new Gui::ActionFunction(menu);
    func->trigger(act, std::bind(&ViewProviderDocumentObject::startDefaultEditMode, this));
}

void ViewProviderDocumentObject::setModeSwitch() {
    if(isShowable())
        ViewProvider::setModeSwitch();
    else
        callExtension(&ViewProviderExtension::extensionModeSwitchChange);
}

void ViewProviderDocumentObject::show()
{
    ViewProvider::show();

    if(ViewParams::getUpdateSelectionVisual()
           && !SelectionNoTopParentCheck::enabled())
    {
        auto obj = getObject();
        if(obj && obj->getDocument() && obj->getNameInDocument())
            Gui::Selection().updateSelection(
                    true, obj->getDocument()->getName(), obj->getNameInDocument(),0);
    }

    // use this bit to check whether 'Visibility' must be adjusted
    if (!Visibility.getValue() && !Visibility.testStatus(App::Property::User2)) {
        Base::ObjectStatusLocker<App::Property::Status,App::Property>
            guard(App::Property::User2, &Visibility);
        Visibility.setValue(true);
    }
}

const char* ViewProviderDocumentObject::getTransactionText() const
{
    return QT_TRANSLATE_NOOP("Command", "Edit");
}

void ViewProviderDocumentObject::updateView()
{
    if(!pcObject || testStatus(ViewStatus::UpdatingView))
        return;

    Base::ObjectStatusLocker<ViewStatus,ViewProviderDocumentObject> lock(ViewStatus::UpdatingView,this);

    // Disable object visibility syncing
    Base::ObjectStatusLocker<App::Property::Status,App::Property> lock2(App::Property::User1, &Visibility);

    std::map<std::string, App::Property*> Map;
    pcObject->getPropertyMap(Map);

    // Hide the object temporarily to speed up the update
    bool vis = ViewProvider::isShow();
    if (vis) ViewProvider::hide();
    for (const auto & it : Map) {
        updateData(it.second);
    }
    if (vis && Visibility.getValue()) ViewProvider::show();
}

void ViewProviderDocumentObject::attachDocumentObject(App::DocumentObject *pcObj)
{
    pcObject = pcObj;

    if(pcObj && pcObj->getNameInDocument()
             && !testStatus(SecondaryView))
    {
        Base::PyGILStateLocker lock;
        pcObj->ViewObject.setValue(Py::asObject(this->getPyObject()));
    }

    attach(pcObj);
}

void ViewProviderDocumentObject::attach(App::DocumentObject *pcObj)
{
    // save Object pointer
    pcObject = pcObj;

    if(pcObj && pcObj->getNameInDocument()
             && !testStatus(SecondaryView))
    {
        pcObj->setStatus(App::ObjectStatus::ViewProviderAttached,true);
        if (!pcObj->getDocument()->testStatus(App::Document::Restoring))
            pcObj->Visibility.setValue(Visibility.getValue());
    }

    DisplayMode.setEnumVector(this->getDisplayModes());
    DisplayMode.setPersistEnums(false);

    if(!isRestoring()) {
        // set the active mode
        const char* defmode = this->getDefaultDisplayMode();
        if (defmode)
            DisplayMode.setValue(defmode);
    }

    //attach the extensions
    callExtension(&ViewProviderExtension::extensionAttach,pcObj);

    updateChildren();
}

void ViewProviderDocumentObject::reattach(App::DocumentObject *pcObj) {
    setStatus(Detach, false);
    callExtension(&ViewProviderExtension::extensionReattach,pcObj);
    updateChildren();
}

void ViewProviderDocumentObject::update(const App::Property* prop)
{
    // DocumentObject.ViewObject has been changed to a real property instead of
    // a Python attribute. It is used as a bridge for expression binding of all
    // view properties. Any view provider property change will touch ViewObject
    // to trigger ExpressionEngine update. This may cause problem for some
    // python extended view object initialization. For better backward
    // compatibility, it is disabled here.
    if (prop == &getObject()->ViewObject)
        return;

    // bypass view provider update to always allow changing visibility from
    // document object
    if(prop == &getObject()->Visibility) {
        if(!isRestoring() && Visibility.getValue()!=getObject()->Visibility.getValue())
            Visibility.setValue(!Visibility.getValue());
    } else if (isUpdatesEnabled()) {
        // Disable object visibility syncing
        Base::ObjectStatusLocker<App::Property::Status,App::Property>
            guard(App::Property::User1, &Visibility);

        bool vis = ViewProvider::isShow();
        bool visValue = Visibility.getValue();
        if (vis)
            ViewProvider::hide();

        updateData(prop);

        // Some view provider may switch on even if Visibility is false. Make
        // sure to restore visibility in this case.
        if (vis && (Visibility.getValue() || visValue==Visibility.getValue()))
            ViewProvider::show();

        if(!getObject()->getDocument()->testStatus(App::Document::Restoring)
                && !getObject()->testStatus(App::PendingRecompute))
        {
            updateChildren(prop->testStatus(App::Property::Output)
                            || prop->testStatus(App::Property::PropOutput));
        }
    }
}

Gui::Document* ViewProviderDocumentObject::getDocument() const
{
    if(!pcObject)
        throw Base::RuntimeError("View provider detached");
    if (pcDocument) {
        return pcDocument;
    }
    else {
        App::Document* pAppDoc = pcObject->getDocument();
        return Gui::Application::Instance->getDocument(pAppDoc);
    }
}

Gui::MDIView* ViewProviderDocumentObject::getActiveView() const
{
    if(!pcObject)
        throw Base::RuntimeError("View provider detached");
    App::Document* pAppDoc = pcObject->getDocument();
    Gui::Document* pGuiDoc = Gui::Application::Instance->getDocument(pAppDoc);
    return pGuiDoc->getActiveView();
}

Gui::MDIView* ViewProviderDocumentObject::getEditingView() const
{
    if(!pcObject)
        throw Base::RuntimeError("View provider detached");
    App::Document* pAppDoc = pcObject->getDocument();
    Gui::Document* pGuiDoc = Gui::Application::Instance->getDocument(pAppDoc);
    return pGuiDoc->getEditingViewOfViewProvider(const_cast<ViewProviderDocumentObject*>(this));
}

Gui::MDIView* ViewProviderDocumentObject::getInventorView() const
{
    if(!pcObject)
        throw Base::RuntimeError("View provider detached");
    App::Document* pAppDoc = pcObject->getDocument();
    Gui::Document* pGuiDoc = Gui::Application::Instance->getDocument(pAppDoc);

    Gui::MDIView* mdi = pGuiDoc->getEditingViewOfViewProvider(const_cast<ViewProviderDocumentObject*>(this));
    if (!mdi) {
        mdi = pGuiDoc->getViewOfViewProvider(const_cast<ViewProviderDocumentObject*>(this));
    }

    return mdi;
}

Gui::MDIView* ViewProviderDocumentObject::getViewOfNode(SoNode* node) const
{
    if(!pcObject)
        throw Base::RuntimeError("View provider detached");
    App::Document* pAppDoc = pcObject->getDocument();
    Gui::Document* pGuiDoc = Gui::Application::Instance->getDocument(pAppDoc);
    return pGuiDoc->getViewOfNode(node);
}

SoNode* ViewProviderDocumentObject::findFrontRootOfType(const SoType& type) const
{
    if(!pcObject)
        return nullptr;
    // first get the document this object is part of and get its GUI counterpart
    App::Document* pAppDoc = pcObject->getDocument();
    Gui::Document* pGuiDoc = Gui::Application::Instance->getDocument(pAppDoc);

    SoSearchAction searchAction;
    searchAction.setType(type);
    searchAction.setInterest(SoSearchAction::FIRST);

    // search in all view providers for the node type
    std::vector<App::DocumentObject*> obj = pAppDoc->getObjects();
    for (auto & it : obj) {
        const ViewProvider* vp = pGuiDoc->getViewProvider(it);
        // Ignore 'this' view provider. It could also happen that vp is 0, e.g. when
        // several objects have been added to the App::Document before notifying the
        // Gui::Document
        if (!vp || vp == this)
            continue;
        SoSeparator* front = vp->getFrontRoot();
        //if (front && front->getTypeId() == type)
        //    return front;
        if (front) {
            searchAction.apply(front);
            SoPath* path = searchAction.getPath();
            if (path)
                return path->getTail();
        }
    }

    return nullptr;
}

void ViewProviderDocumentObject::setActiveMode()
{
    if (DisplayMode.isValid()) {
        const char* mode = DisplayMode.getValueAsString();
        if (mode)
            setDisplayMode(mode);
    }
    if (!Visibility.getValue())
        ViewProvider::hide();
}

void ViewProviderDocumentObject::setDisplayMode(const char* ModeName)
{
    if (!DisplayMode.testStatus(App::Property::User1)) {
        Base::ObjectStatusLocker<App::Property::Status,App::Property> guard(
                App::Property::User1, &DisplayMode);
        DisplayMode.setValue(ModeName);
    }
    ViewProvider::setDisplayMode(ModeName);
}

bool ViewProviderDocumentObject::canDelete(App::DocumentObject* obj) const
{
    Q_UNUSED(obj)
    if (getObject()->hasExtension(App::GroupExtension::getExtensionClassTypeId()))
        return true;
    if (getObject()->isDerivedFrom(App::Origin::getClassTypeId()))
        return true;
    return false;
}

PyObject* ViewProviderDocumentObject::getPyObject()
{
    if (!pyViewObject)
        pyViewObject = new ViewProviderDocumentObjectPy(this);
    pyViewObject->IncRef();
    return pyViewObject;
}

bool ViewProviderDocumentObject::canDropObjectEx(App::DocumentObject* obj, App::DocumentObject *owner,
        const char *subname, const std::vector<std::string> &elements) const
{
    if(queryExtension(&ViewProviderExtension::extensionCanDropObjectEx,obj,owner,subname,elements))
        return true;
    if(obj && obj->getDocument()!=getObject()->getDocument())
        return false;
    return canDropObject(obj);
}

int ViewProviderDocumentObject::replaceObject(
        App::DocumentObject *oldObj, App::DocumentObject *newObj)
{
    if(!oldObj || !oldObj->getNameInDocument()
            || !newObj || !newObj->getNameInDocument())
    {
        FC_THROWM(Base::RuntimeError,"Invalid object");
    }

    auto obj = getObject();
    if(!obj || !obj->getNameInDocument())
        FC_THROWM(Base::RuntimeError,"View provider not attached");

    int res = ViewProvider::replaceObject(oldObj,newObj);
    if(res>=0)
        return res;

    std::vector<std::pair<App::DocumentObjectT, std::unique_ptr<App::Property> > > propChanges;
    std::vector<App::Property*> props;
    obj->getPropertyList(props);
    for(auto prop : props) {
        auto linkProp = Base::freecad_dynamic_cast<App::PropertyLinkBase>(prop);
        if(!linkProp)
            continue;
        std::unique_ptr<App::Property> copy(linkProp->CopyOnLinkReplace(obj, oldObj,newObj));
        if(!copy)
            continue;
        propChanges.emplace_back(prop,std::move(copy));
    }

    if(propChanges.empty())
        return 0;

    // Global search for affected links
    for(auto doc : App::GetApplication().getDocuments()) {
        for(auto o : doc->getObjects()) {
            if(o == obj)
                continue;
            props.clear();
            o->getPropertyList(props);
            for(auto prop : props) {
                auto linkProp = Base::freecad_dynamic_cast<App::PropertyLinkBase>(prop);
                if(!linkProp)
                    continue;
                std::unique_ptr<App::Property> copy(linkProp->CopyOnLinkReplace(obj,oldObj,newObj));
                if(!copy)
                    continue;
                propChanges.emplace_back(App::DocumentObjectT(prop),std::move(copy));
            }
        }
    }

    for(auto &v : propChanges) {
        auto prop = v.first.getProperty();
        if(prop)
            prop->Paste(*v.second.get());
    }
    return 1;
}

bool ViewProviderDocumentObject::showInTree() const {
    return ShowInTree.getValue();
}

bool ViewProviderDocumentObject::getElementPicked(const SoPickedPoint *pp, std::string &subname) const
{
    if(queryExtension(&ViewProviderExtension::extensionGetElementPicked,pp,subname))
        return true;

    SoPath* path = pp->getPath();
    int idx = -1;
    auto childRoot = getChildRoot();
    if(!childRoot && ViewParams::getMapChildrenPlacement())
        childRoot = getChildrenGroup();
    if(childRoot)
        idx = path->findNode(childRoot);
    if(idx < 0) {
        subname = getElement(pp?pp->getDetail():0);
        return true;
    }

    if(idx+1<path->getLength()) {
        auto vp = getDocument()->getViewProvider(path->getNode(idx+1));
        if(!vp) return false;
        auto obj = vp->getObject();
        if(!obj || !obj->getNameInDocument())
            return false;
        std::ostringstream str;
        str << obj->getNameInDocument() << '.';
        if(vp->getElementPicked(pp,subname))
            str << subname;
        subname = str.str();
    }
    return true;
}

bool ViewProviderDocumentObject::getDetailPath(
        const char *subname, SoFullPath *path, bool append, SoDetail *&det) const
{
    if(pcRoot->findChild(pcModeSwitch) < 0) {
        // this is possible in case of editing, where the switch node of the
        // linked view object is temporarily removed from its root. We must
        // still return true here, to prevent the selection action leaking to
        // parent and sibling nodes.
        if(append)
            path->append(pcRoot);
        return true;
    }

    auto len = path->getLength();
    if(!append && len>=2)
        len -= 2;

    if(append) {
        path->append(pcRoot);
        path->append(pcModeSwitch);
    }
    if(queryExtension(&ViewProviderExtension::extensionGetDetailPath,subname,path,det))
        return true;

    const char *dot = nullptr;
    if(Data::ComplexGeoData::isMappedElement(subname) || (dot=strchr(subname,'.')) == 0) {
        det = getDetail(subname);
        return true;
    }

    auto obj = getObject();
    if(!obj || !obj->getNameInDocument())
        return false;
    auto sobj = obj->getSubObject(std::string(subname,dot-subname+1).c_str());
    if(!sobj || !sobj->getNameInDocument())
        return false;
    auto vp = Application::Instance->getViewProvider(sobj);
    if(!vp)
        return false;

    auto childRoot = getChildRoot();
    if(!childRoot && ViewParams::getMapChildrenPlacement())
        childRoot = getChildrenGroup();

    for(;;) {
        if(!childRoot) {
            // If no child root, then this view provider does not stack children
            // view provider under its own root, so we pop till before the root
            // node of this view provider.
            path->truncate(len);
        } else {
            // Do not account for our own visibility, we maybe called by a Link
            // that has independent visibility. Just make sure the child root node
            // is indeed a child of mode switch.
            if(pcModeSwitch->findChild(childRoot)<0) {
                childRoot = 0;
                continue;
            }
            path->append(childRoot);
        }
        if(path->getLength()) {
            SoNode * tail = path->getTail();
            if(tail->isOfType(SoGroup::getClassTypeId())
                        && static_cast<SoGroup*>(tail)->findChild(vp->getRoot())>=0)
                return vp->getDetailPath(dot+1,path,true,det);
        }
        if(childRoot) {
            // Can't find under child root, try again without it
            childRoot = 0;
        } else
            return false;
    }
}

void ViewProviderDocumentObject::onPropertyStatusChanged(
        const App::Property &prop, unsigned long oldStatus)
{
    (void)oldStatus;
    if(!App::Document::isAnyRestoring() && pcObject && pcObject->getDocument())
        pcObject->getDocument()->signalChangePropertyEditor(*pcObject->getDocument(),prop);
}

ViewProviderDocumentObject *ViewProviderDocumentObject::getLinkedViewProvider(
        std::string *subname, bool recursive) const
{
    (void)subname;
    auto self = const_cast<ViewProviderDocumentObject*>(this);
    if(!pcObject || !pcObject->getNameInDocument())
        return self;
    auto linked = pcObject->getLinkedObject(recursive);
    if(!linked || linked == pcObject)
        return self;
    auto res = Base::freecad_dynamic_cast<ViewProviderDocumentObject>(
            Application::Instance->getViewProvider(linked));
    if(!res)
        res = self;
    return res;
}

std::string ViewProviderDocumentObject::getFullName(bool python) const {
    if (pcObject)
        return pcObject->getFullName(python) + ".ViewObject";
    return std::string(python?"None":"?");
}

App::Document *ViewProviderDocumentObject::getOwnerDocument() const {
    return pcObject?pcObject->getDocument():0;
}

void ViewProviderDocumentObject::setSelectable(bool selectable)
{
    SoSearchAction sa;
    sa.setInterest(SoSearchAction::ALL);
    sa.setSearchingAll(true);
    sa.setType(Gui::SoFCSelection::getClassTypeId());
    sa.apply(pcRoot);

    SoPathList & pathList = sa.getPaths();

    for (int i=0;i<pathList.getLength();i++) {
        SoFCSelection *selNode = dynamic_cast<SoFCSelection*>(pathList[i]->getTail());
        if (selectable) {
            if (selNode) {
                selNode->selectionMode = SoFCSelection::SEL_ON;
                selNode->highlightMode = SoFCSelection::AUTO;
            }
        }
        else {
            if (selNode) {
                selNode->selectionMode = SoFCSelection::SEL_OFF;
                selNode->highlightMode = SoFCSelection::OFF;
                selNode->selected = SoFCSelection::NOTSELECTED;
            }
        }
    }
}

Base::BoundBox3d ViewProviderDocumentObject::_getBoundingBox(
        const char *subname, const Base::Matrix4D *mat, bool transform,
        const View3DInventorViewer *viewer, int depth) const
{
    if(!viewer) {
        viewer = getActiveViewer();
        if(!viewer) {
            if (FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG))
                FC_ERR("no view");
            return Base::BoundBox3d();
        }
    }

    App::DocumentObject *obj = getObject();
    if(!subname || !subname[0] || !obj) {
        if(obj) {
            auto subs = obj->getSubObjects(App::DocumentObject::GS_SELECT);
            if(subs.size()) {
                Base::BoundBox3d box;
                for(std::string &sub : subs) {
                    Base::Matrix4D smat;
                    if(mat)
                        smat = *mat;
                    int vis = obj->isElementVisibleEx(sub.c_str(), App::DocumentObject::GS_SELECT);
                    if(!vis)
                        continue;
                    auto sobj = obj->getSubObject(sub.c_str(),0,&smat,transform,depth+1);
                    auto vp = Application::Instance->getViewProvider(sobj);
                    if(!vp)
                        continue;
                    if(vis < 0 && !sobj->Visibility.getValue())
                        continue;
                    auto sbox = vp->getBoundingBox(0,&smat,false,viewer,depth+1);
                    if(sbox.IsValid())
                        box.Add(sbox);
                }
                return box;
            }
        }

        return ViewProvider::_getBoundingBox(0,mat,transform,viewer,depth);
    }

    Base::Matrix4D smat;
    if(mat)
        smat = *mat;

    std::string _subname;
    const char *nextsub;
    const char *dot=0;
    if(Data::ComplexGeoData::isMappedElement(subname)
            || (dot=strchr(subname,'.'))==0)
    {
        return ViewProvider::_getBoundingBox(subname,&smat,false,viewer,depth+1);
    }

    if(ViewParams::getMapChildrenPlacement()) {
        _subname = std::string(subname,dot+1-subname);
        subname = _subname.c_str();
        nextsub = dot+1;
    } else
        nextsub = Data::ComplexGeoData::findElementName(dot+1);

    auto sobj = getObject()->getSubObject(subname,0,&smat,transform,depth);
    auto vp = Application::Instance->getViewProvider(sobj);
    if(!vp)
        return Base::BoundBox3d();
    return vp->getBoundingBox(nextsub, &smat,false,viewer,depth+1);
}

const std::vector<App::DocumentObject*> &ViewProviderDocumentObject::getCachedChildren() const {
    return claimedChildren;
}

void ViewProviderDocumentObject::beforeDelete() {
    ViewProvider::beforeDelete();

    for(auto child : childSet) {
        auto vpd = Base::freecad_dynamic_cast<ViewProviderDocumentObject>(
                Application::Instance->getViewProvider(child));
        if(vpd && vpd->parentSet.erase(getObject())) {
            if(getObject() && vpd->getObject()
                    && getObject()->getDocument() == vpd->getObject()->getDocument())
            {
                vpd->isShowable(true);
            }
        }
    }
    claimedChildren.clear();
    childSet.clear();
    parentSet.clear();
}

const std::set<App::DocumentObject *>& ViewProviderDocumentObject::claimedBy() const {
    return parentSet;
}

void ViewProviderDocumentObject::updateChildren(bool propagate) {
    auto obj = getObject();
    if(!obj || !obj->getNameInDocument())
        return;

    auto newChildren = claimChildren();
    if(claimedChildren == newChildren)
        return;

    std::set<App::DocumentObject *> newSet;
    for (auto child : newChildren) {
        auto vpd = Base::freecad_dynamic_cast<ViewProviderDocumentObject>(
                Application::Instance->getViewProvider(child));
        if(!vpd || !newSet.insert(child).second)
            continue;
        if(!childSet.erase(child)) {
            // this means new child detected
            if(vpd->parentSet.insert(obj).second
                    && child->getDocument() == obj->getDocument())
            {
                // Trigger visibility check
                vpd->isShowable(true);
            }
        }
    }

    for (auto child : childSet) {
        if(newSet.find(child) == newSet.end()) {
            // this means old child removed
            auto vpd = Base::freecad_dynamic_cast<ViewProviderDocumentObject>(
                    Application::Instance->getViewProvider(child));
            if(!vpd)
                continue;
            vpd->parentSet.erase(obj);
            if(child->getDocument() == obj->getDocument()) {
                // Trigger showability check
                vpd->isShowable(true);
            }
        }
    }

    childSet = std::move(newSet);
    claimedChildren = std::move(newChildren);

    if(propagate) {
        // This is true if the children update is triggered by a property that
        // is marked as output. It will not touch its object, and thus, its
        // property change will not be propagated through recomputation. So we
        // have to manually check for each links here.
        for(auto link : App::GetApplication().getLinksTo(obj,App::GetLinkRecursive)) {
            auto vpd = Base::freecad_dynamic_cast<ViewProviderDocumentObject>(
                    Application::Instance->getViewProvider(link));
            if(vpd)
                vpd->updateChildren(false);
        }
    }

    //if the item is in a GeoFeatureGroup we may need to update that too, as the claim children
    //of the geofeaturegroup depends on what the childs claim
    if (ViewProviderGeoFeatureGroupExtension::needUpdateChildren(obj)) {
        auto vpd = Base::freecad_dynamic_cast<ViewProviderDocumentObject>(
                Application::Instance->getViewProvider(obj));
        if(vpd)
            vpd->updateChildren(true);
    }

    signalChangedChildren(*this);
    Application::Instance->signalChangedChildren(*this);

    if(!getChildRoot() && !isDerivedFrom(ViewProviderLink::getClassTypeId())) {
        if(!pcChildGroup) {
            pcChildGroup = new SoGroup;
            pcModeSwitch->addChild(pcChildGroup);
        } else
            coinRemoveAllChildren(pcChildGroup);
        for(auto child : childSet) {
            auto vp = Application::Instance->getViewProvider(child);
            if(vp)
                pcChildGroup->addChild(vp->getRoot());
        }
    }
}

SoGroup *ViewProviderDocumentObject::getChildrenGroup() const {
    if(pcChildGroup)
        return pcChildGroup;
    return 0;
}

bool ViewProviderDocumentObject::isShowable(bool refresh) {
    if(!refresh || isRestoring())
        return _Showable;

    auto obj = getObject();
    if(!obj || _Busy)
        return _Showable;

    // guard against cyclic dependency
    Base::StateLocker locker(_Busy);
    bool showable = true;
    for(auto parent : parentSet) {
        // Calling getViewProvider() also servers as a safety measure, to make
        // sure the object exists.
        auto parentVp = Base::freecad_dynamic_cast<ViewProviderDocumentObject>(
                                    Application::Instance->getViewProvider(parent));
        if(!parentVp || parent->getDocument()!=obj->getDocument())
            continue;
        if(App::GeoFeatureGroupExtension::isNonGeoGroup(parent)) {
            if(parentVp->isShowable()) {
                showable = true;
                break;
            }
        } else if(!parent->hasChildElement()
                    && parent->getLinkedObject(false)==parent)
        {
            showable = true;
            break;
        }
        showable = false;
    }

    if(_Showable == showable)
        return showable;

    _Showable = showable;
    FC_TRACE((_Showable?"showable ":"not showable ") << obj->getNameInDocument());

    // showability changed

    int which = getModeSwitch()->whichChild.getValue();
    if(_Showable && which == -1 && Visibility.getValue()) {
        setModeSwitch();
    } else if (!_Showable) {
        if(which >= 0) {
            getModeSwitch()->whichChild = -1;
            callExtension(&ViewProviderExtension::extensionModeSwitchChange);
        }
    }

    // Plain group is special, as its view provider does not claim any of its
    // child nodes, and its mode switch node may not have any children, so its
    // whichChild may always be -1.  Therefore, we have to manually propagate
    // the showability changes to all children
    if(App::GeoFeatureGroupExtension::isNonGeoGroup(obj)) {
        for(auto &child : claimedChildren) {
            auto vpd = Base::freecad_dynamic_cast<ViewProviderDocumentObject>(
                    Application::Instance->getViewProvider(child));
            if(vpd)
                vpd->isShowable(true);
        }
    }
    return _Showable;
}

bool ViewProviderDocumentObject::setEdit(int ModNum)
{
    if (ModNum == ExportInGroup) {
        if (auto group = App::GroupExtension::getAnyGroupOfObject(getObject())) {
            auto ext = group->getExtensionByType<App::GroupExtension>(true);
            if (ext) {
                std::string name(QT_TRANSLATE_NOOP("Command", "Toggle export "));
                name += getObject()->Label.getValue();
                App::AutoTransaction committer(name.c_str());
                try {
                    ext->toggleChildExport(getObject(), true);
                } catch (Base::Exception &e) {
                    e.ReportException();
                }
            }
        }
        return false;
    }
    return ViewProvider::setEdit(ModNum);
}

static QByteArray _iconTagExport("group:Export");

void ViewProviderDocumentObject::getExtraIcons(
        std::vector<std::pair<QByteArray,QPixmap> > &icons) const
{
    {
        BitmapCacheContext ctx(getObject() ? getObject()->getTypeId().getName() : getTypeId().getName());
        ViewProvider::getExtraIcons(icons);
    }

    auto prop = App::GroupExtension::getChildExportProperty(getObject());
    if (prop && prop->getValue()) {
        if (auto group = App::GroupExtension::getAnyGroupOfObject(getObject())) {
            auto ext = group->getExtensionByType<App::GroupExtension>(true);
            if (ext && ext->ExportMode.getValue() == App::GroupExtension::ExportByChildQuery) {
                icons.emplace_back(_iconTagExport, 
                                    Gui::BitmapFactory().pixmap("Std_Export"));
            }
        }
    }
}

QString ViewProviderDocumentObject::getToolTip(const QByteArray &tag) const
{
    if (tag == _iconTagExport) {
        return QObject::tr("This object is marked for export for its owner group.\n"
                           "ALT + click this icon to disable. You can re-enable it\n"
                           "by right click and choose menu action 'Toggle export'.");
    }
    return ViewProvider::getToolTip(tag);
}

bool ViewProviderDocumentObject::iconMouseEvent(QMouseEvent *ev, const QByteArray &tag)
{
    if (ev->type() == QEvent::MouseButtonPress) {
        if (tag == _iconTagExport) {
            setEdit(ExportInGroup);
            return true;
        }
    }
    return ViewProvider::iconMouseEvent(ev, tag);
}

void ViewProviderDocumentObject::updateData(const App::Property *prop)
{
    if (getObject() && !getObject()->isRestoring()) {
        if (prop == App::GroupExtension::getChildExportProperty(getObject()))
            signalChangeIcon();
    }
    ViewProvider::updateData(prop);
}

void ViewProviderDocumentObject::setupContextMenu(QMenu* menu, QObject* receiver, const char* method)
{
    ViewProvider::setupContextMenu(menu, receiver, method);

    // Show export menu action if this object is part of a group that is
    // exporting its children, but not exporting by visibility.
    if (App::GroupExtension::getChildExportProperty(getObject())) {
        if (auto group = App::GroupExtension::getAnyGroupOfObject(getObject())) {
            auto ext = group->getExtensionByType<App::GroupExtension>(true);
            if (ext && ext->ExportMode.getValue() != App::GroupExtension::ExportByVisibility) {
                bool found = false;
                for (auto action : menu->actions()) {
                    if (action->data().toInt() == ExportInGroup) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    QAction *act = menu->addAction(QObject::tr("Toggle export"), receiver, method);
                    act->setData(QVariant((int)ExportInGroup));
                }
            }
        }
    }
}


QIcon ViewProviderDocumentObject::getIcon(void) const
{
    BitmapCacheContext ctx(getObject() ? getObject()->getTypeId().getName() : getTypeId().getName());
    return ViewProvider::getIcon();
}
