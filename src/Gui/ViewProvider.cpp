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
# include <QApplication>
# include <QPixmap>
# include <QTimer>
# include <Inventor/SoPickedPoint.h>
# include <Inventor/nodes/SoSeparator.h>
# include <Inventor/nodes/SoSwitch.h>
# include <Inventor/details/SoDetail.h>
# include <Inventor/nodes/SoTransform.h>
# include <Inventor/nodes/SoCamera.h>
# include <Inventor/events/SoMouseButtonEvent.h>
# include <Inventor/events/SoLocation2Event.h>
# include <Inventor/actions/SoGetMatrixAction.h>
# include <Inventor/actions/SoSearchAction.h>
# include <Inventor/actions/SoGetBoundingBoxAction.h>
# include <boost_bind_bind.hpp>
#endif

#include <unordered_map>

/// Here the FreeCAD includes sorted by Base,App,Gui......
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Tools.h>
#include <Base/BoundBox.h>
#include <Base/Matrix.h>
#include <App/PropertyGeo.h>
#include <App/Document.h>

#include "ViewProvider.h"
#include "Application.h"
#include "ActionFunction.h"
#include "Document.h"
#include "ViewProviderPy.h"
#include "BitmapFactory.h"
#include "View3DInventor.h"
#include "View3DInventorViewer.h"
#include "SoFCDB.h"
#include "ViewProviderExtension.h"
#include "SoFCUnifiedSelection.h"
#include "ViewProviderLink.h"
#include "ViewParams.h"


FC_LOG_LEVEL_INIT("ViewProvider",true,true)

using namespace std;
using namespace Gui;


namespace Gui {

void coinRemoveAllChildren(SoGroup *group) {
    if(!group)
        return;
    int count = group->getNumChildren();
    if(!count)
        return;
    FC_TRACE("coin remove all children " << count);
    SbBool autonotify = group->enableNotify(FALSE);
    for(;count>0;--count)
        group->removeChild(count-1);
    group->enableNotify(autonotify);
    group->touch();
}

} // namespace Gui

//**************************************************************************
//**************************************************************************
// ViewProvider
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

PROPERTY_SOURCE_ABSTRACT(Gui::ViewProvider, App::TransactionalObject)

ViewProvider::ViewProvider()
    : pcAnnotation(0)
    , pyViewObject(0)
    , overrideMode("As Is")
    , _iActualMode(-1)
    , _iEditMode(-1)
    , viewOverrideMode(-1)
{
    setStatus(UpdateData, true);


    // SoFCSeparater and SoFCSelectionRoot can both track render cache setting.
    // We change to SoFCSelectionRoot so that we can dynamically change full
    // selection mode (full highlight vs. boundbox). Note that comparing to
    // SoFCSeparater, there are some small overhead with SoFCSelectionRoot for
    // selection context tracking.
    //
    // pcRoot = new SoFCSeparator(true);
    pcRoot = new SoFCSelectionRoot(true,this);
    static_cast<SoFCSelectionRoot*>(pcRoot)->noHandleEvent = true;
    pcRoot->ref();
    pcModeSwitch = new SoFCSwitch();
    pcModeSwitch->ref();
    pcTransform  = new SoTransform();
    pcTransform->ref();
    pcRoot->addChild(pcTransform);
    pcRoot->addChild(pcModeSwitch);
    sPixmap = "px";
    pcModeSwitch->whichChild = _iActualMode;

    setRenderCacheMode(ViewParams::instance()->getRenderCache());
}

ViewProvider::~ViewProvider()
{
    if (pyViewObject) {
        Base::PyGILStateLocker lock;
        pyViewObject->setInvalid();
        pyViewObject->DecRef();
    }

    if(pcRoot && pcRoot->isOfType(SoFCSelectionRoot::getClassTypeId()))
        static_cast<SoFCSelectionRoot*>(pcRoot)->setViewProvider(0);

    pcRoot->unref();
    pcTransform->unref();
    pcModeSwitch->unref();
    if (pcAnnotation)
        pcAnnotation->unref();
}

ViewProvider *ViewProvider::startEditing(int ModNum)
{
    if(setEdit(ModNum)) {
        _iEditMode = ModNum;
        return this;
    }
    return 0;
}

int ViewProvider::getEditingMode() const
{
    return _iEditMode;
}

bool ViewProvider::isEditing() const
{
    return getEditingMode() > -1;
}

void ViewProvider::finishEditing()
{
    unsetEdit(_iEditMode);
    _iEditMode = -1;
}

bool ViewProvider::setEdit(int ModNum)
{
    Q_UNUSED(ModNum);
    return true;
}

void ViewProvider::unsetEdit(int ModNum)
{
    Q_UNUSED(ModNum);
}

void ViewProvider::setEditViewer(View3DInventorViewer*, int ModNum)
{
    Q_UNUSED(ModNum);
}

void ViewProvider::unsetEditViewer(View3DInventorViewer*)
{
}

bool ViewProvider::isUpdatesEnabled () const
{
    return testStatus(UpdateData);
}

void ViewProvider::setUpdatesEnabled (bool enable)
{
    setStatus(UpdateData, enable);
}

void highlight(const HighlightMode& high)
{
    Q_UNUSED(high);
}

void ViewProvider::eventCallback(void * ud, SoEventCallback * node)
{
    const SoEvent * ev = node->getEvent();
    Gui::View3DInventorViewer* viewer = reinterpret_cast<Gui::View3DInventorViewer*>(node->getUserData());
    ViewProvider *self = reinterpret_cast<ViewProvider*>(ud);
    assert(self);

    try {
        // Keyboard events
        if (ev->getTypeId().isDerivedFrom(SoKeyboardEvent::getClassTypeId())) {
            SoKeyboardEvent * ke = (SoKeyboardEvent *)ev;
            const SbBool press = ke->getState() == SoButtonEvent::DOWN ? true : false;
            switch (ke->getKey()) {
            case SoKeyboardEvent::ESCAPE:
                if (self->keyPressed (press, ke->getKey())) {
                    node->setHandled();
                }
                else if(QApplication::mouseButtons()==Qt::NoButton) {
                    // Because of a Coin bug (https://bitbucket.org/Coin3D/coin/pull-requests/119),
                    // FC may crash if user hits ESC to cancel while still
                    // holding the mouse button while using some SoDragger.
                    // Therefore, we shall ignore ESC while any mouse button is
                    // pressed, until this Coin bug is fixed.
                    if (!press) {
                        // react only on key release 
                        // Let first selection mode terminate
                        Gui::Document* doc = Gui::Application::Instance->activeDocument();
                        Gui::View3DInventor* view = static_cast<Gui::View3DInventor*>(doc->getActiveView());
                        if (view)
                        {
                            Gui::View3DInventorViewer* viewer = view->getViewer();
                            if (viewer->isSelecting())
                            {
                                return;
                            }
                        }
                        
                        Gui::TimerFunction* func = new Gui::TimerFunction();
                        func->setAutoDelete(true);
                        func->setFunction(boost::bind(&Document::resetEdit, doc));
                        QTimer::singleShot(0, func, SLOT(timeout()));
                    }
                }
                else if (press) {
                    FC_WARN("Please release all mouse buttons before exiting editing");
                }
                break;
            default:
                // call the virtual method
                if (self->keyPressed (press, ke->getKey()))
                    node->setHandled();
                break;
            }
        }
        // switching the mouse buttons
        else if (ev->getTypeId().isDerivedFrom(SoMouseButtonEvent::getClassTypeId())) {

            const SoMouseButtonEvent * const event = (const SoMouseButtonEvent *) ev;
            const int button = event->getButton();
            const SbBool press = event->getState() == SoButtonEvent::DOWN ? true : false;

            // call the virtual method
            if (self->mouseButtonPressed(button,press,ev->getPosition(),viewer))
                node->setHandled();
        }
        // Mouse Movement handling
        else if (ev->getTypeId().isDerivedFrom(SoLocation2Event::getClassTypeId())) {
            if (self->mouseMove(ev->getPosition(),viewer))
                node->setHandled();
        }
    }
    catch (const Base::Exception& e) {
        Base::Console().Error("Unhandled exception in ViewProvider::eventCallback: %s\n"
                              "(Event type: %s, object type: %s)\n"
                              , e.what(), ev->getTypeId().getName().getString()
                              , self->getTypeId().getName());
    }
    catch (const std::exception& e) {
        Base::Console().Error("Unhandled std exception in ViewProvider::eventCallback: %s\n"
                              "(Event type: %s, object type: %s)\n"
                              , e.what(), ev->getTypeId().getName().getString()
                              , self->getTypeId().getName());
    }
    catch (...) {
        Base::Console().Error("Unhandled unknown C++ exception in ViewProvider::eventCallback"
                              " (Event type: %s, object type: %s)\n"
                              , ev->getTypeId().getName().getString()
                              , self->getTypeId().getName());
    }
}

SoSeparator* ViewProvider::getAnnotation(void)
{
    if (!pcAnnotation) {
        pcAnnotation = new SoSeparator();
        pcAnnotation->ref();
        pcRoot->addChild(pcAnnotation);
    }
    return pcAnnotation;
}

void ViewProvider::update(const App::Property* prop)
{
    // Hide the object temporarily to speed up the update
    if (!isUpdatesEnabled())
        return;
    bool vis = ViewProvider::isShow();
    if (vis) ViewProvider::hide();
    updateData(prop);
    if (vis) ViewProvider::show();
}

QIcon ViewProvider::getIcon(void) const
{
    return mergeOverlayIcons (Gui::BitmapFactory().pixmap(sPixmap));
}

void ViewProvider::getExtraIcons(std::vector<std::pair<QByteArray,QPixmap> > &icons) const
{
    callExtension(&ViewProviderExtension::extensionGetExtraIcons,icons);
}

QIcon ViewProvider::mergeOverlayIcons (const QIcon & orig) const
{
    QIcon overlayedIcon = orig;
    foreachExtension<ViewProviderExtension>([&overlayedIcon](ViewProviderExtension *ext) {
        if (!ext->ignoreOverlayIcon())
            ext->extensionMergeOverlayIcons(overlayedIcon);
        return true;
    });

    return overlayedIcon;
}

void ViewProvider::setTransformation(const Base::Matrix4D &rcMatrix)
{
    double dMtrx[16];
    rcMatrix.getGLMatrix(dMtrx);

    pcTransform->setMatrix(SbMatrix(dMtrx[0], dMtrx[1], dMtrx[2],  dMtrx[3],
                                    dMtrx[4], dMtrx[5], dMtrx[6],  dMtrx[7],
                                    dMtrx[8], dMtrx[9], dMtrx[10], dMtrx[11],
                                    dMtrx[12],dMtrx[13],dMtrx[14], dMtrx[15]));
}

void ViewProvider::setTransformation(const SbMatrix &rcMatrix)
{
    pcTransform->setMatrix(rcMatrix);
}

SbMatrix ViewProvider::convert(const Base::Matrix4D &rcMatrix)
{
    double dMtrx[16];
    rcMatrix.getGLMatrix(dMtrx);
    return SbMatrix(dMtrx[0], dMtrx[1], dMtrx[2],  dMtrx[3],
                    dMtrx[4], dMtrx[5], dMtrx[6],  dMtrx[7],
                    dMtrx[8], dMtrx[9], dMtrx[10], dMtrx[11],
                    dMtrx[12],dMtrx[13],dMtrx[14], dMtrx[15]);
}

Base::Matrix4D ViewProvider::convert(const SbMatrix &smat)
{
    Base::Matrix4D mat;
    for(int i=0;i<4;++i) {
        for(int j=0;j<4;++j)
            mat[i][j] = smat[j][i];
    }
    return mat;
}

void ViewProvider::addDisplayMaskMode(SoNode *node, const char* type)
{
    if(pcChildGroup) {
        int idx = pcModeSwitch->findChild(pcChildGroup);
        if(idx>=0)
            pcModeSwitch->removeChild(idx);
    }
    _sDisplayMaskModes[type] = pcModeSwitch->getNumChildren();
    if(pcModeSwitch->isOfType(SoFCSwitch::getClassTypeId()))
        static_cast<SoFCSwitch*>(pcModeSwitch)->childNames.set1Value(
                pcModeSwitch->getNumChildren(), type);
    pcModeSwitch->addChild(node);
    if(pcChildGroup)
        pcModeSwitch->addChild(pcChildGroup);
}

void ViewProvider::setDisplayMaskMode(const char* type)
{
    std::map<std::string, int>::const_iterator it = _sDisplayMaskModes.find(type);
    if (it != _sDisplayMaskModes.end())
        _iActualMode = it->second;
    else
        _iActualMode = -1;
    setModeSwitch();
}

SoNode* ViewProvider::getDisplayMaskMode(const char* type) const
{
    std::map<std::string, int>::const_iterator it = _sDisplayMaskModes.find( type );
    if (it != _sDisplayMaskModes.end()) {
        return pcModeSwitch->getChild(it->second);
    }

    return 0;
}

std::vector<std::string> ViewProvider::getDisplayMaskModes() const
{
    std::vector<std::string> types;
    for (std::map<std::string, int>::const_iterator it = _sDisplayMaskModes.begin();
         it != _sDisplayMaskModes.end(); ++it)
        types.push_back( it->first );
    return types;
}

/**
 * If you add new viewing modes in @ref getDisplayModes() then you need to reimplement
 * also seDisplaytMode() to handle these new modes by setting the appropriate display
 * mode.
 */
void ViewProvider::setDisplayMode(const char* ModeName)
{
    _sCurrentMode = ModeName;

    //infom the exteensions
    callExtension(&ViewProviderExtension::extensionSetDisplayMode,ModeName);
}

const char* ViewProvider::getDefaultDisplayMode() const {

    return 0;
}

vector<std::string> ViewProvider::getDisplayModes(void) const {

    std::vector< std::string > modes;
    callExtension(&ViewProviderExtension::extensionGetDisplayModes, modes);
    return modes;
}

std::string ViewProvider::getActiveDisplayMode(void) const
{
    return _sCurrentMode;
}

void ViewProvider::hide(void)
{
    int which = pcModeSwitch->whichChild.getValue();
    if(which >= 0)
        pcModeSwitch->whichChild = -1;

    foreachExtension<ViewProviderExtension>([&](ViewProviderExtension *ext) {
        if(which >= 0)
            ext->extensionModeSwitchChange();
        ext->extensionHide();
        return false;
    });
}

void ViewProvider::show(void)
{
    if (isRestoring())
        return;

    setModeSwitch();

    //tell extensions that we show
    callExtension(&ViewProviderExtension::extensionShow);
}

bool ViewProvider::isShow(void) const
{
    return pcModeSwitch->whichChild.getValue() != -1;
}

void ViewProvider::setVisible(bool s)
{
    s ? show() : hide();
}

bool ViewProvider::isVisible() const
{
    return isShow();
}

void ViewProvider::setOverrideMode(const std::string &mode)
{
    if (mode == "As Is") {
        viewOverrideMode = -1;
        overrideMode = mode;
    }
    else {
        std::map<std::string, int>::const_iterator it = _sDisplayMaskModes.find(mode);
        if (it == _sDisplayMaskModes.end())
            return; //view style not supported
        viewOverrideMode = (*it).second;
        overrideMode = mode;
    }
    if (pcModeSwitch->whichChild.getValue() != -1)
        setModeSwitch();
    else
        callExtension(&ViewProviderExtension::extensionModeSwitchChange);
}

const string ViewProvider::getOverrideMode() {
    return overrideMode;
}


void ViewProvider::setModeSwitch()
{
    if (isRestoring())
        return;

    int mode;
    if (viewOverrideMode == -1)
        mode = _iActualMode;
    else if (viewOverrideMode < pcModeSwitch->getNumChildren())
        mode = viewOverrideMode;
    else
        return;
    if(mode != pcModeSwitch->whichChild.getValue()) {
        pcModeSwitch->whichChild = mode;
        callExtension(&ViewProviderExtension::extensionModeSwitchChange);
    }
}

void ViewProvider::setDefaultMode(int val)
{
    _iActualMode = val;
    setModeSwitch();
}

int ViewProvider::getDefaultMode(bool noOverride) const {
    return (!noOverride && viewOverrideMode>=0)?viewOverrideMode:_iActualMode;
}

void ViewProvider::onBeforeChange(const App::Property* prop)
{
    Application::Instance->signalBeforeChangeObject(*this, *prop);

    App::TransactionalObject::onBeforeChange(prop);
}

void ViewProvider::onChanged(const App::Property* prop)
{
    Application::Instance->signalChangedObject(*this, *prop);
    Application::Instance->updateActions();

    App::TransactionalObject::onChanged(prop);
}

std::string ViewProvider::toString() const
{
    return SoFCDB::writeNodesToString(pcRoot);
}

PyObject* ViewProvider::getPyObject()
{
    if (!pyViewObject)
        pyViewObject = new ViewProviderPy(this);
    pyViewObject->IncRef();
    return pyViewObject;
}

#include <boost/graph/topological_sort.hpp>

namespace Gui {
typedef boost::adjacency_list <
        boost::vecS,           // class OutEdgeListS  : a Sequence or an AssociativeContainer
        boost::vecS,           // class VertexListS   : a Sequence or a RandomAccessContainer
        boost::directedS,      // class DirectedS     : This is a directed graph
        boost::no_property,    // class VertexProperty:
        boost::no_property,    // class EdgeProperty:
        boost::no_property,    // class GraphProperty:
        boost::listS           // class EdgeListS:
> Graph;
typedef boost::graph_traits<Graph>::vertex_descriptor Vertex;
typedef boost::graph_traits<Graph>::edge_descriptor Edge;

void addNodes(Graph& graph, std::map<SoNode*, Vertex>& vertexNodeMap, SoNode* node)
{
    if (node->getTypeId().isDerivedFrom(SoGroup::getClassTypeId())) {
        SoGroup* group = static_cast<SoGroup*>(node);
        Vertex groupV = vertexNodeMap[group];

        for (int i=0; i<group->getNumChildren(); i++) {
            SoNode* child = group->getChild(i);
            auto it = vertexNodeMap.find(child);

            // the child node is not yet added to the map
            if (it == vertexNodeMap.end()) {
                Vertex childV = add_vertex(graph);
                vertexNodeMap[child] = childV;
                add_edge(groupV, childV, graph);
                addNodes(graph, vertexNodeMap, child);
            }
            // the child is already there, only add the edge then
            else {
                add_edge(groupV, it->second, graph);
            }
        }
    }
}
}

bool ViewProvider::checkRecursion(SoNode* node)
{
    if (node->getTypeId().isDerivedFrom(SoGroup::getClassTypeId())) {
        std::list<Vertex> make_order;
        Graph graph;
        std::map<SoNode*, Vertex> vertexNodeMap;
        Vertex groupV = add_vertex(graph);
        vertexNodeMap[node] = groupV;
        addNodes(graph, vertexNodeMap, node);

        try {
            boost::topological_sort(graph, std::front_inserter(make_order));
        }
        catch (const std::exception&) {
            return false;
        }
    }

    return true;
}

SoPickedPoint* ViewProvider::getPointOnRay(const SbVec2s& pos, const View3DInventorViewer* viewer) const
{
    return viewer->getPointOnRay(pos,const_cast<ViewProvider*>(this));
}

SoPickedPoint* ViewProvider::getPointOnRay(const SbVec3f& pos,const SbVec3f& dir, const View3DInventorViewer* viewer) const
{
    return viewer->getPointOnRay(pos,dir,const_cast<ViewProvider*>(this));
}


std::vector<Base::Vector3d> ViewProvider::getModelPoints(const SoPickedPoint* pp) const
{
    // the default implementation just returns the picked point from the visual representation
    std::vector<Base::Vector3d> pts;
    const SbVec3f& vec = pp->getPoint();
    pts.emplace_back(vec[0],vec[1],vec[2]);
    return pts;
}

bool ViewProvider::keyPressed(bool pressed, int key)
{
    (void)pressed;
    (void)key;
    return false;
}

bool ViewProvider::mouseMove(const SbVec2s &cursorPos,
                             View3DInventorViewer* viewer)
{
    (void)cursorPos;
    (void)viewer;
    return false;
}

bool ViewProvider::mouseButtonPressed(int button, bool pressed,
                                      const SbVec2s &cursorPos,
                                      const View3DInventorViewer* viewer)
{
    (void)button;
    (void)pressed;
    (void)cursorPos;
    (void)viewer;
    return false;
}

void ViewProvider::setupContextMenu(QMenu* menu, QObject* receiver, const char* method)
{
    auto vector = getExtensionsDerivedFromType<Gui::ViewProviderExtension>();
    for (Gui::ViewProviderExtension* ext : vector)
        ext->extensionSetupContextMenu(menu, receiver, method);
}

bool ViewProvider::onDelete(const vector< string >& subNames)
{
    bool del = true;
    foreachExtension<ViewProviderExtension>([&](ViewProviderExtension *ext) {
        del &= ext->extensionOnDelete(subNames);
        return false;
    });
    return del;
}

bool ViewProvider::canDelete(App::DocumentObject*) const
{
    return false;
}

bool ViewProvider::canDragObject(App::DocumentObject* obj) const
{
    return queryExtension(&ViewProviderExtension::extensionCanDragObject,obj);
}

bool ViewProvider::canDragObjects() const
{
    return queryExtension(&ViewProviderExtension::extensionCanDragObjects);
}

void ViewProvider::dragObject(App::DocumentObject* obj)
{
    int res = false;
    foreachExtension<ViewProviderExtension>([&res,obj](ViewProviderExtension *ext) {
        if (ext->extensionCanDragObject(obj)) {
            ext->extensionDragObject(obj);
            res = true;
            return true;
        }
        return false;
    });
    if(!res)
        throw Base::RuntimeError("Cannot drag object.");
}

bool ViewProvider::canDropObject(App::DocumentObject* obj) const
{
    return queryExtension(&ViewProviderExtension::extensionCanDropObject,obj);
}

bool ViewProvider::canDropObjects() const {
    return queryExtension(&ViewProviderExtension::extensionCanDropObjects);
}

bool ViewProvider::canDragAndDropObject(App::DocumentObject* obj) const {
    int res = true;
    foreachExtension<ViewProviderExtension>([&res,obj](ViewProviderExtension *ext) {
        if (!ext->extensionCanDragAndDropObject(obj)) {
            res = false;
            return true;
        }
        return false;
    });
    return res;
}

void ViewProvider::dropObject(App::DocumentObject* obj) {
    int res = false;
    foreachExtension<ViewProviderExtension>([&res,obj](ViewProviderExtension *ext) {
        if (ext->extensionCanDropObject(obj)) {
            ext->extensionDropObject(obj);
            res = true;
            return true;
        }
        return false;
    });

    if(!res)
        throw Base::RuntimeError("Cannot drop object.");
}

bool ViewProvider::canDropObjectEx(App::DocumentObject* obj, App::DocumentObject *owner,
        const char *subname, const std::vector<std::string> &elements) const
{
    if(queryExtension(&ViewProviderExtension::extensionCanDropObjectEx,obj,owner,subname,elements))
        return true;
    return canDropObject(obj);
}

std::string ViewProvider::dropObjectEx(App::DocumentObject* obj, App::DocumentObject *owner,
        const char *subname, const std::vector<std::string> &elements)
{
    std::string name;
    bool res = false;
    foreachExtension<ViewProviderExtension>([&](ViewProviderExtension *ext) {
        if(ext->extensionCanDropObjectEx(obj, owner, subname, elements)) {
            res = true;
            name = ext->extensionDropObjectEx(obj, owner, subname, elements);
            return true;
        }
        return false;
    });

    if(!res)
        dropObject(obj);
    return name;
}

int ViewProvider::replaceObject(App::DocumentObject* oldValue, App::DocumentObject* newValue)
{
    if(!canReplaceObject(oldValue, newValue))
        return -1;

    int res = -1;
    foreachExtension<ViewProviderExtension>([&](ViewProviderExtension *ext) {
        res = ext->extensionReplaceObject(oldValue, newValue);
        return res>=0;
    });
    return res;
}

bool ViewProvider::canReplaceObject(App::DocumentObject* oldValue, App::DocumentObject* newValue)
{
    int res = -1;
    foreachExtension<ViewProviderExtension>([&](ViewProviderExtension *ext) {
        res = ext->extensionCanReplaceObject(oldValue, newValue);
        return res>=0;
    });

    if(res < 0) {
        // no one implements this function, fallback to canDropObject
        return canDropObject(newValue);
    }
    return res>=0;
}

void ViewProvider::Restore(Base::XMLReader& reader) {
    // Because some PropertyLists type properties are stored in a separate file,
    // and is thus restored outside this function. So we rely on Gui::Document
    // to set the isRestoring flags for us.
    //
    // setStatus(Gui::isRestoring, true);

    TransactionalObject::Restore(reader);

    // setStatus(Gui::isRestoring, false);
}

void ViewProvider::updateData(const App::Property* prop)
{
    callExtension(&ViewProviderExtension::extensionUpdateData,prop);
}

SoSeparator* ViewProvider::getBackRoot(void) const
{
    SoSeparator *node = 0;
    foreachExtension<ViewProviderExtension>([&node](ViewProviderExtension *ext) {
        node = ext->extensionGetBackRoot();
        return node?true:false;
    });
    return node;
}

SoGroup* ViewProvider::getChildRoot(void) const
{
    SoGroup *node = 0;
    foreachExtension<ViewProviderExtension>([&node](ViewProviderExtension *ext) {
        node = ext->extensionGetChildRoot();
        return node?true:false;
    });
    return node;
}

SoSeparator* ViewProvider::getFrontRoot(void) const
{
    SoSeparator *node = 0;
    foreachExtension<ViewProviderExtension>([&node](ViewProviderExtension *ext) {
        node = ext->extensionGetFrontRoot();
        return node?true:false;
    });
    return node;
}

std::vector< App::DocumentObject* > ViewProvider::claimChildren(void) const
{
    std::vector< App::DocumentObject* > vec;
    callExtension(&ViewProviderExtension::extensionClaimChildren,vec);
    return vec;
}

std::vector< App::DocumentObject* > ViewProvider::claimChildren3D(void) const
{
    std::vector< App::DocumentObject* > vec;
    callExtension(&ViewProviderExtension::extensionClaimChildren3D,vec);
    return vec;
}

bool ViewProvider::handleChildren3D(const std::vector<App::DocumentObject*> &children) {
    return queryExtension(&ViewProviderExtension::extensionHandleChildren3D,children);
}

bool ViewProvider::getElementPicked(const SoPickedPoint *pp, std::string &subname) const {
    if(!isSelectable()) return false;
    if(!queryExtension(&ViewProviderExtension::extensionGetElementPicked,pp,subname))
        subname = getElement(pp?pp->getDetail():0);
    return true;
}

bool ViewProvider::getDetailPath(const char *subname, SoFullPath *pPath, bool append, SoDetail *&det) const {
    if(pcRoot->findChild(pcModeSwitch) < 0) {
        // this is possible in case of editing, where the switch node
        // of the linked view object is temporarily removed from its root
        // if(append)
        //     pPath->append(pcRoot);
        return false;
    }
    if(append) {
        pPath->append(pcRoot);
        pPath->append(pcModeSwitch);
    }
    if(!queryExtension(&ViewProviderExtension::extensionGetDetailPath,subname,pPath,det))
        det = getDetail(subname);
    return true;
}

const std::string &ViewProvider::hiddenMarker() {
    return App::DocumentObject::hiddenMarker();
}

const char *ViewProvider::hasHiddenMarker(const char *subname) {
    return App::DocumentObject::hasHiddenMarker(subname);
}

int ViewProvider::partialRender(const std::vector<std::string> &elements, bool clear) {
    if(elements.empty()) {
        auto node = pcModeSwitch->getChild(_iActualMode);
        if(node) {
            FC_LOG("partial render clear");
            SoSelectionElementAction action(SoSelectionElementAction::None,true);
            action.apply(node);
        }
    }
    int count = 0;
    SoFullPath *path = static_cast<SoFullPath*>(new SoPath);
    path->ref();
    SoSelectionElementAction action;
    action.setSecondary(true);
    for(auto element : elements) {
        bool hidden = hasHiddenMarker(element.c_str());
        if(hidden)
            element.resize(element.size()-hiddenMarker().size());
        path->truncate(0);
        SoDetail *det = 0;
        if(getDetailPath(element.c_str(),path,false,det)) {
            if(!hidden && !det) {
                FC_LOG("partial render element not found: " << element);
                continue;
            }
            FC_LOG("partial render (" << path->getLength() << "): " << element);
            if(!hidden)
                action.setType(clear?SoSelectionElementAction::Remove:SoSelectionElementAction::Append);
            else
                action.setType(clear?SoSelectionElementAction::Show:SoSelectionElementAction::Hide);
            action.setElement(det);
            action.apply(path);
            ++count;
        }
        delete det;
    }
    path->unref();
    return count;
}

bool ViewProvider::useNewSelectionModel() const {
    return ViewParams::instance()->getUseNewSelection();
}

void ViewProvider::beforeDelete() {
    callExtension(&ViewProviderExtension::extensionBeforeDelete);
}

void ViewProvider::setRenderCacheMode(int mode) {
    pcRoot->renderCaching =
        mode==0?SoSeparator::AUTO:(mode==1?SoSeparator::ON:SoSeparator::OFF);
}

const View3DInventorViewer *ViewProvider::getActiveViewer() {
    auto view  = dynamic_cast<View3DInventor*>(Application::Instance->activeView());
    if(!view) {
        auto doc = Application::Instance->activeDocument();
        if(doc) {
            auto views = doc->getMDIViewsOfType(View3DInventor::getClassTypeId());
            if(views.size())
                view = dynamic_cast<View3DInventor*>(views.front());
        }
        if(!view)
            return 0;
    }
    return view->getViewer();
}

static int BBoxCacheId;
struct BBoxKey {
    std::string subname;
    Base::Matrix4D mat;
    bool transform;

    BBoxKey(const char *s, const Base::Matrix4D *m, bool t)
        :subname(s?s:""),transform(t)
    {
        if(m)
            mat = *m;
    }

    bool operator==(const BBoxKey &other) const {
        return subname==other.subname && mat==other.mat;
    }
};

// copied from boost::hash_combine, because boost changes hearder location in
// different version.
template <class T>
inline void hash_combine(std::size_t& seed, const T& v)
{
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
}

static std::hash<std::string> StringHasher;
static std::hash<bool> BoolHasher;
static std::hash<double> DoubleHasher;

struct BBoxKeyHasher {
    std::size_t operator()(const BBoxKey &key) const {
        std::size_t seed = StringHasher(key.subname);
        hash_combine(seed,BoolHasher(key.transform));
        hash_combine(seed,DoubleHasher(key.mat[0][0]));
        hash_combine(seed,DoubleHasher(key.mat[0][1]));
        hash_combine(seed,DoubleHasher(key.mat[0][2]));
        hash_combine(seed,DoubleHasher(key.mat[0][3]));
        hash_combine(seed,DoubleHasher(key.mat[1][0]));
        hash_combine(seed,DoubleHasher(key.mat[1][1]));
        hash_combine(seed,DoubleHasher(key.mat[1][2]));
        hash_combine(seed,DoubleHasher(key.mat[1][3]));
        hash_combine(seed,DoubleHasher(key.mat[2][0]));
        hash_combine(seed,DoubleHasher(key.mat[2][1]));
        hash_combine(seed,DoubleHasher(key.mat[2][2]));
        hash_combine(seed,DoubleHasher(key.mat[2][3]));
        hash_combine(seed,DoubleHasher(key.mat[3][0]));
        hash_combine(seed,DoubleHasher(key.mat[3][1]));
        hash_combine(seed,DoubleHasher(key.mat[3][2]));
        hash_combine(seed,DoubleHasher(key.mat[3][3]));
        return seed;
    }
};

struct ViewProvider::BoundingBoxCache {
    int cacheId = 0;
    bool busy = false;
    std::unordered_map<BBoxKey, Base::BoundBox3d, BBoxKeyHasher> cache;
};

void ViewProvider::clearBoundingBoxCache() {
    ++BBoxCacheId;
}

Base::BoundBox3d ViewProvider::getBoundingBox(
        const char *subname, const Base::Matrix4D *mat,
        bool transform, const View3DInventorViewer *viewer, int depth) const
{
    if (auto doc = getOwnerDocument()) {
        if (doc->testStatus(App::Document::Restoring)) {
            if (bboxCache) {
                auto it = bboxCache->cache.find(BBoxKey(subname,mat,transform));
                if (it != bboxCache->cache.end())
                    return it->second;
            }
            return Base::BoundBox3d();
        }
    }

    if(!bboxCache)
        bboxCache.reset(new BoundingBoxCache);

    if(bboxCache->busy)
        return ViewProvider::_getBoundingBox(subname,mat,transform,viewer,depth);

    if(!ViewParams::instance()->getUseBoundingBoxCache()) {
        Base::FlagToggler<> guard(bboxCache->busy);
        return _getBoundingBox(subname,mat,transform,viewer,depth);
    }

    if(bboxCache->cacheId != BBoxCacheId) {
        bboxCache->cache.clear();
        bboxCache->cacheId = BBoxCacheId;
    }

    auto &bbox = bboxCache->cache[BBoxKey(subname,mat,transform)];
    if(!bbox.IsValid()) {
        Base::FlagToggler<> guard(bboxCache->busy);
        bbox = _getBoundingBox(subname,mat,transform,viewer,depth);
    }
    return bbox;
}

Base::BoundBox3d ViewProvider::_getBoundingBox(
        const char *subname, const Base::Matrix4D *mat,
        bool transform, const View3DInventorViewer *viewer, int) const
{
    if(!pcRoot || !pcModeSwitch || pcRoot->findChild(pcModeSwitch)<0)
        return Base::BoundBox3d();

    if(!viewer) {
        viewer = getActiveViewer();
        if(!viewer) {
            FC_ERR("no view");
            return Base::BoundBox3d();
        }
    }

    static FC_COIN_THREAD_LOCAL SoGetBoundingBoxAction *bboxAction;
    if (!bboxAction)
        bboxAction = new SoGetBoundingBoxAction(SbViewportRegion());
    bboxAction->setViewportRegion(viewer->getSoRenderManager()->getViewportRegion());

    static FC_COIN_THREAD_LOCAL SoTempPath path(20);
    path.ref();
    path.truncate(0);
    static FC_COIN_THREAD_LOCAL CoinPtr<SoGroup> fakeRoot;
    if (!fakeRoot) 
        fakeRoot = new SoGroup;
    coinRemoveAllChildren(fakeRoot);
    fakeRoot->addChild(viewer->getSoRenderManager()->getCamera());
    fakeRoot->addChild(pcRoot);
    path.append(fakeRoot);

    static FC_COIN_THREAD_LOCAL SoSelectionElementAction selAction(
            SoSelectionElementAction::Append,true,true);

    SoDetail *det=0;
    if(subname && subname[0]) {
        if(!getDetailPath(subname,&path,true,det)) {
            path.truncate(0);
            path.unrefNoDelete();
            coinRemoveAllChildren(fakeRoot);
            return Base::BoundBox3d();
        }
        if(det) {
            selAction.setType(SoSelectionElementAction::Append);
            selAction.setElement(det);
            SoFCSwitch::switchOverride(&selAction);
            selAction.apply(&path);
        }
    }
    static FC_COIN_THREAD_LOCAL SoTempPath resetPath(3);
    resetPath.ref();
    resetPath.truncate(0);
    if(!transform) {
        resetPath.append(pcRoot);
        resetPath.append(pcModeSwitch);
        bboxAction->setResetPath(&resetPath,true,SoGetBoundingBoxAction::TRANSFORM);
    }
    if(path.getLength() == 1) {
        path.append(pcRoot);
        path.append(pcModeSwitch);
    }
    SoFCSwitch::switchOverride(bboxAction);
    bboxAction->apply(&path);

    if(det) {
        delete det;
        selAction.setElement(0);
        selAction.setType(SoSelectionElementAction::None);
        SoFCSwitch::switchOverride(&selAction);
        selAction.apply(&path);
    }

    resetPath.truncate(0);
    resetPath.unrefNoDelete();
    path.truncate(0);
    path.unrefNoDelete();
    coinRemoveAllChildren(fakeRoot);

    auto xbbox = bboxAction->getXfBoundingBox();
    if(mat)
        xbbox.transform(convert(*mat));
    auto bbox = xbbox.project();
    float minX,minY,minZ,maxX,maxY,maxZ;
    bbox.getMax().getValue(maxX,maxY,maxZ);
    bbox.getMin().getValue(minX,minY,minZ);
    return Base::BoundBox3d(minX,minY,minZ,maxX,maxY,maxZ);
}

bool ViewProvider::isLinkVisible() const {
    auto ext = getExtensionByType<ViewProviderLinkObserver>(true);
    if(!ext)
        return true;
    return ext->isLinkVisible();
}

void ViewProvider::setLinkVisible(bool visible) {
    auto ext = getExtensionByType<ViewProviderLinkObserver>(true);
    if(ext)
        ext->setLinkVisible(visible);
}

QString ViewProvider::getToolTip(const QByteArray &tag) const
{
    QString tooltip;
    foreachExtension<ViewProviderExtension>([&](ViewProviderExtension *ext) {
        return ext->extensionGetToolTip(tag, tooltip);
    });
    return tooltip;
}

bool ViewProvider::iconMouseEvent(QMouseEvent *ev, const QByteArray &tag)
{
    bool res = false;
    foreachExtension<ViewProviderExtension>([&](ViewProviderExtension *ext) {
        return (res = ext->extensionIconMouseEvent(ev, tag));
    });
    return false;
}
