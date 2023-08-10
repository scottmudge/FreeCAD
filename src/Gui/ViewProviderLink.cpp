/****************************************************************************
 *   Copyright (c) 2017 Zheng Lei (realthunder) <realthunder.dev@gmail.com> *
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
#include "ViewProviderDocumentObject.h"

#ifndef _PreComp_
# include <atomic>
# include <cctype>
# include <boost/algorithm/string/predicate.hpp>
# include <Inventor/SoPickedPoint.h>
# include <Inventor/actions/SoGetBoundingBoxAction.h>
# include <Inventor/details/SoDetail.h>
# include <Inventor/draggers/SoCenterballDragger.h>
# include <Inventor/misc/SoChildList.h>
# include <Inventor/nodes/SoAnnotation.h>
# include <Inventor/nodes/SoCube.h>
# include <Inventor/nodes/SoDrawStyle.h>
# include <Inventor/nodes/SoMatrixTransform.h>
# include <Inventor/nodes/SoPickStyle.h>
# include <Inventor/nodes/SoSeparator.h>
# include <Inventor/nodes/SoShapeHints.h>
# include <Inventor/nodes/SoSurroundScale.h>
# include <Inventor/nodes/SoSwitch.h>
# include <Inventor/nodes/SoTransform.h>
# include <Inventor/sensors/SoNodeSensor.h>
# include <Inventor/SoPickedPoint.h>
# include <QApplication>
# include <QMenu>
# include <QCheckBox>
# include <QMouseEvent>
# include <QPointer>
#endif

#include <boost/range.hpp>

#include <App/ComplexGeoData.h>
#include <App/Document.h>
#include <App/GeoFeature.h>
#include <App/GeoFeatureGroupExtension.h>
#include <Base/BoundBoxPy.h>
#include <Base/Console.h>
#include <Base/MatrixPy.h>
#include <Base/PlacementPy.h>
#include <Base/Tools.h>
#include "Application.h"
#include "ActionFunction.h"
#include "BitmapFactory.h"
#include "Command.h"
#include "Control.h"
#include "DlgObjectSelection.h"
#include "Document.h"
#include "LinkViewPy.h"
#include "MainWindow.h"
#include "Selection.h"
#include "SoFCCSysDragger.h"
#include "SoFCUnifiedSelection.h"
#include "TaskCSysDragger.h"
#include "TaskElementColors.h"
#include "Tree.h"
#include "ViewParams.h"
#include "View3DInventor.h"
#include "ViewProviderLink.h"
#include "ViewProviderLinkPy.h"
#include "ViewProviderGeometryObject.h"

FC_LOG_LEVEL_INIT("App::Link", true, true)

using namespace Gui;
using namespace Base;

using CharRange = boost::iterator_range<const char*>;

static std::unordered_map<SoNode*, std::pair<App::Document*, App::DocumentObject*> > _LinkNodeMap;

static inline void _registerLinkNode(SoNode *node, const App::DocumentObject *obj=nullptr)
{
    if (!obj)
        _LinkNodeMap.erase(node);
    else {
        if (node->isOfType(SoFCSelectionRoot::getClassTypeId()))
            static_cast<SoFCSelectionRoot*>(node)->cacheHint.setValue(2);
        _LinkNodeMap[node] = std::make_pair(obj->getDocument(), const_cast<App::DocumentObject*>(obj));
    }
}

static inline void _registerLinkNode(SoNode *node, const ViewProviderDocumentObject *vp)
{
    _registerLinkNode(node, vp?vp->getObject():nullptr);
}

////////////////////////////////////////////////////////////////////////////

static inline bool appendPathSafe(SoPath *path, SoNode *node) {
    if(path->getLength()) {
        SoNode * tail = path->getTail();
        if(tail->isOfType(SoGroup::getClassTypeId())
                && static_cast<SoGroup*>(tail)->findChild(node)<0)
            return false;
    }
    path->append(node);
    return true;
}

#define appendPath(_path,_node)  _appendPath(__LINE__, _path, _node)
static inline void _appendPath(int line, SoPath *path, SoNode *node) {
#ifdef FC_DEBUG
    if(!appendPathSafe(path,node))
        _FC_ERR(__FILE__, line, "LinkView: coin path error");
#else
    path->append(node);
#endif
}

////////////////////////////////////////////////////////////////////////////
class Gui::LinkInfo {

public:
    std::atomic<int> ref;

    using Connection = boost::signals2::scoped_connection;
    Connection connChangeIcon;

    ViewProviderDocumentObject *pcLinked;
    std::unordered_set<Gui::LinkOwner*> links;

    using Pointer = LinkInfoPtr;

    SoNodeSensor sensor;
    SoNodeSensor switchSensor;
    SoNodeSensor childSensor;
    SoNodeSensor transformSensor;

    std::array<CoinPtr<SoSeparator>,LinkView::SnapshotMax> pcSnapshots;
    std::array<CoinPtr<SoFCSwitch>,LinkView::SnapshotMax> pcSwitches;
    std::array<CoinPtr<SoTransform>,LinkView::SnapshotMax> pcTransforms;
    CoinPtr<SoSwitch> pcLinkedSwitch;

    // for group type view providers
    CoinPtr<SoGroup> pcChildGroup;
    using NodeMap = std::unordered_map<SoNode *, Pointer>;
    NodeMap nodeMap;

    std::map<qint64, QIcon> iconMap;

    static ViewProviderDocumentObject *getView(App::DocumentObject *obj) {
        if(obj && obj->getNameInDocument()) {
            return Base::freecad_dynamic_cast<ViewProviderDocumentObject>(
                    Application::Instance->getViewProvider(obj));
        }
        return nullptr;
    }

    static Pointer get(App::DocumentObject *obj, Gui::LinkOwner *owner) {
        return get(getView(obj),owner);
    }

    static Pointer get(ViewProviderDocumentObject *vp, LinkOwner *owner) {
        if(!vp)
            return Pointer();

        auto ext = vp->getExtensionByType<ViewProviderLinkObserver>(true);
        if(!ext) {
            ext = new ViewProviderLinkObserver();
            ext->initExtension(vp);
        }
        if(!ext->linkInfo) {
            // extension can be created automatically when restored from document,
            // with an empty linkInfo. So we need to check here.
            ext->linkInfo = Pointer(new LinkInfo(vp));
            ext->linkInfo->update();
        } else if (!ext->linkInfo->isLinked()) {
            ext->linkInfo->pcLinked = vp;
            ext->linkInfo->update();
        }
        if(owner)
            ext->linkInfo->links.insert(owner);
        return ext->linkInfo;
    }

    static void sensorCB(void *data, SoSensor *) {
        static_cast<LinkInfo*>(data)->update();
    }

    static void switchSensorCB(void *data, SoSensor *) {
        static_cast<LinkInfo*>(data)->updateSwitch();
    }

    static void childSensorCB(void *data, SoSensor *) {
        static_cast<LinkInfo*>(data)->updateChildren();
    }

    static void transformSensorCB(void *data, SoSensor *) {
        auto self = static_cast<LinkInfo*>(data);
        for(size_t i=0;i<self->pcSnapshots.size();++i)  {
            if(self->pcSnapshots[i] && i!=LinkView::SnapshotTransform)
                self->getSnapshot(i,true);
        }
    }

    LinkInfo(ViewProviderDocumentObject *vp)
        :ref(0),pcLinked(vp)
    {
        FC_TRACE("new link to " << pcLinked->getObject()->getFullName());
        connChangeIcon = vp->signalChangeIcon.connect(
                boost::bind(&LinkInfo::slotChangeIcon,this));
        vp->forceUpdate(true);
        sensor.setFunction(sensorCB);
        sensor.setData(this);
        switchSensor.setFunction(switchSensorCB);
        switchSensor.setData(this);
        childSensor.setFunction(childSensorCB);
        childSensor.setData(this);
        transformSensor.setFunction(transformSensorCB);
        transformSensor.setData(this);
    }

    ~LinkInfo() {
    }

    bool checkName(const char *name) const {
        return isLinked() && strcmp(name,getLinkedName())==0;
    }

    void remove(LinkOwner *owner) {
        links.erase(owner);
    }

    bool isLinked() const {
        return pcLinked && pcLinked->getObject() &&
           pcLinked->getObject()->getNameInDocument();
    }

    const char *getLinkedName() const {
        return pcLinked->getObject()->getNameInDocument();
    }

    const char *getLinkedLabel() const {
        return pcLinked->getObject()->Label.getValue();
    }

    const char *getLinkedNameSafe() const {
        if(isLinked())
            return getLinkedName();
        return "<nil>";
    }

    const char *getDocName() const {
        return pcLinked->getDocument()->getDocument()->getName();
    }

    void detach(bool unlink) {
        FC_LOG("link detach " << getLinkedNameSafe());
        auto me = LinkInfoPtr(this);
        if(unlink) {
            while(!links.empty()) {
                auto link = *links.begin();
                links.erase(links.begin());
                link->unlink(me);
            }
        }
        sensor.detach();
        switchSensor.detach();
        childSensor.detach();
        transformSensor.detach();
        for(const auto &node : pcSnapshots) {
            if(node) {
                coinRemoveAllChildren(node);
                _registerLinkNode(node);
                // Do not reset snapshoot root so that redo/undo can work with
                // calling update()
                //
                // node.reset();
            }
        }
        for(auto &node : pcSwitches) {
            if(node) {
                coinRemoveAllChildren(node);
                node.reset();
            }
        }
        pcLinkedSwitch.reset();
        if(pcChildGroup) {
            coinRemoveAllChildren(pcChildGroup);
            pcChildGroup.reset();
        }
        nodeMap.clear();
        pcLinked = nullptr;
        connChangeIcon.disconnect();
    }

    void updateSwitch(SoSwitch *node=nullptr) {
        if(!isLinked() || !pcLinkedSwitch)
            return;
        SoFCSwitch *linkedSwitch = nullptr;
        if(pcLinkedSwitch->isOfType(SoFCSwitch::getClassTypeId()))
            linkedSwitch = static_cast<SoFCSwitch*>(pcLinkedSwitch.get());

        for(size_t i=0;i<pcSwitches.size();++i) {
            if(!pcSwitches[i] || (node && node!=pcSwitches[i]))
                continue;

            if(linkedSwitch) {
                if(pcSwitches[i]->defaultChild != linkedSwitch->defaultChild)
                    pcSwitches[i]->defaultChild = linkedSwitch->defaultChild;
                if(pcSwitches[i]->overrideSwitch != linkedSwitch->overrideSwitch)
                    pcSwitches[i]->overrideSwitch = linkedSwitch->overrideSwitch;
                if(pcSwitches[i]->childNames != linkedSwitch->childNames)
                    pcSwitches[i]->childNames = linkedSwitch->childNames;
                if(pcSwitches[i]->allowNamedOverride != linkedSwitch->allowNamedOverride)
                    pcSwitches[i]->allowNamedOverride = linkedSwitch->allowNamedOverride;
            }

            int count = pcSwitches[i]->getNumChildren();
            if((!pcLinked->Visibility.getValue() && i==LinkView::SnapshotChild) || !count)
                pcSwitches[i]->whichChild = -1;
            else if(count>pcLinked->getDefaultMode() && pcLinked->getDefaultMode()>0)
                pcSwitches[i]->whichChild = pcLinked->getDefaultMode();
            else
                pcSwitches[i]->whichChild = 0;
        }
    }

    inline void addref() {
        ++ref;
    }

    inline void release(){
        int r = --ref;
        assert(r>=0);
        if(r==0)
            delete this;
        else if(r==1) {
            if(pcLinked) {
                FC_LOG("link release " << getLinkedNameSafe());
                auto ext = pcLinked->getExtensionByType<ViewProviderLinkObserver>(true);
                if(ext && ext->linkInfo == this) {
                    pcLinked->forceUpdate(false);
                    detach(true);
                    ext->linkInfo.reset();
                }
            }
        }
    }

    // VC2013 has trouble with template argument dependent lookup in
    // namespace. Have to put the below functions in global namespace.
    //
    // However, gcc seems to behave the oppsite, hence the conditional
    // compilation  here.
    //
#ifdef _MSC_VER
    friend void ::intrusive_ptr_add_ref(LinkInfo *px);
    friend void ::intrusive_ptr_release(LinkInfo *px);
#else
    friend inline void intrusive_ptr_add_ref(LinkInfo *px) { px->addref(); }
    friend inline void intrusive_ptr_release(LinkInfo *px) { px->release(); }
#endif

    bool isVisible() const {
        if(!isLinked())
            return true;
        int indices[] = {LinkView::SnapshotTransform, LinkView::SnapshotVisible};
        for(int idx : indices) {
            if(!pcSwitches[idx])
                continue;
            if(pcSwitches[idx]->whichChild.getValue()==-1)
                return false;
        }
        return true;
    }

    void setVisible(bool visible) {
        if(!isLinked())
            return;
        int indices[] = {LinkView::SnapshotTransform, LinkView::SnapshotVisible};
        for(int idx : indices) {
            if(!pcSwitches[idx])
                continue;
            if(!visible)
                pcSwitches[idx]->whichChild = -1;
            else if(pcSwitches[idx]->getNumChildren()>pcLinked->getDefaultMode())
                pcSwitches[idx]->whichChild = pcLinked->getDefaultMode();
        }
    }

    void addChild(SoGroup *parent, SoNode *child, int &count) {
        int num = parent->getNumChildren();
        if (num == count)
            parent->addChild(child);
        else {
            assert(num > count);
            if (parent->getChild(count) != child) {
                for (int i=count+1; i<num; ++i) {
                    if (parent->getChild(i) == child) {
                        parent->removeChild(i);
                        break;
                    }
                }
                parent->insertChild(child, count);
            }
        }
        ++count;
    }

    SoSeparator *getSnapshot(int type, bool update=false) {
        if(type<0 || type>=LinkView::SnapshotMax)
            return nullptr;

        SoSeparator *root;
        if(!isLinked() || !(root=pcLinked->getRoot()))
            return nullptr;

        if(sensor.getAttachedNode()!=root) {
            sensor.detach();
            sensor.attach(root);
        }

        auto &pcSnapshot = pcSnapshots[type];
        auto &pcModeSwitch = pcSwitches[type];
        auto &pcTransform = pcTransforms[type];
        if(pcSnapshot && pcModeSwitch) {
            if(!update)
                return pcSnapshot;
        }else{
            if (!pcSnapshot) {
                if(ViewParams::getUseSelectionRoot())
                    pcSnapshot = new SoFCSelectionRoot(true);
                else {
                    pcSnapshot = new SoSeparator;
                    // pcSnapshot->boundingBoxCaching = SoSeparator::OFF;
                    pcSnapshot->renderCaching = SoSeparator::OFF;
                }
            }
            _registerLinkNode(pcSnapshot, pcLinked);
            pcModeSwitch = new SoFCSwitch;
        }

        if(pcSnapshot->isOfType(SoFCSelectionRoot::getClassTypeId())
                && root->isOfType(SoFCSelectionRoot::getClassTypeId()))
        {
            static_cast<SoFCSelectionRoot*>(pcSnapshot.get())->selectionStyle =
                static_cast<SoFCSelectionRoot*>(root)->selectionStyle.getValue();
        }

        pcLinkedSwitch.reset();

        pcModeSwitch->whichChild = -1;

        int childCount = 0;
        int modeCount = 0;
        // opt to incremental update instead of remove all and re-add to reduce
        // impact on existing SoPath.
        //
        // coinRemoveAllChildren(pcSnapshot);
        // coinRemoveAllChildren(pcModeSwitch);

        SoSwitch *pcUpdateSwitch = pcModeSwitch;

        auto childRoot = pcLinked->getChildRoot();
        if(!childRoot)
            childRoot = pcLinked->getChildrenGroup();

        for(int i=0,count=root->getNumChildren();i<count;++i) {
            SoNode *node = root->getChild(i);
            if(node==pcLinked->getTransformNode()) {
                if(type!=LinkView::SnapshotTransform)
                    addChild(pcSnapshot, node, childCount);
                else {
                    auto transform = pcLinked->getTransformNode();
                    const auto &scale = transform->scaleFactor.getValue();
                    if(scale[0]!=1.0 || scale[1]!=1.0 || scale[2]!=1.0) {
                        if (!pcTransform)
                            pcTransform = new SoTransform;
                        addChild(pcSnapshot, pcTransform, childCount);
                        pcTransform->scaleFactor.setValue(scale);
                        pcTransform->scaleOrientation = transform->scaleOrientation;
                        if(transformSensor.getAttachedNode()!=transform) {
                            transformSensor.detach();
                            transformSensor.attach(transform);
                        }
                    }
                }
                continue;
            } else if(node!=pcLinked->getModeSwitch()) {
                addChild(pcSnapshot, node, childCount);
                continue;
            }

            pcLinkedSwitch = static_cast<SoSwitch*>(node);
            if(switchSensor.getAttachedNode() != pcLinkedSwitch) {
                switchSensor.detach();
                switchSensor.attach(pcLinkedSwitch);
                pcUpdateSwitch = nullptr;
            }

            addChild(pcSnapshot, pcModeSwitch, childCount);
            for(int i=0,count=pcLinkedSwitch->getNumChildren();i<count;++i) {
                auto child = pcLinkedSwitch->getChild(i);
                if(pcChildGroup && child==childRoot)
                    addChild(pcModeSwitch, pcChildGroup, modeCount);
                else
                    addChild(pcModeSwitch, child, modeCount);
            }
            if(pcChildGroup && !childRoot) {
                addChild(pcModeSwitch, pcChildGroup, modeCount);
#if 0
                if(type==LinkView::SnapshotChild
                        && App::GeoFeatureGroupExtension::isNonGeoGroup(pcLinked->getObject()))
                {
                    pcModeSwitch->tailChild = pcModeSwitch->getNumChildren()-1;
                } else if(pcModeSwitch->tailChild.getValue()>=0)
                   pcModeSwitch->tailChild = -1;
#endif
            }
        }

        while(childCount < pcSnapshot->getNumChildren())
            pcSnapshot->removeChild(pcSnapshot->getNumChildren()-1);
        while(modeCount < pcModeSwitch->getNumChildren())
            pcModeSwitch->removeChild(pcModeSwitch->getNumChildren()-1);

        updateSwitch(pcUpdateSwitch);

        return pcSnapshot;
    }

    void updateData(const App::Property *prop) {
        LinkInfoPtr me(this);
        for(auto link : links)
            link->onLinkedUpdateData(me,prop);
        update();
    }

    void update() {
        if(!isLinked() || pcLinked->isRestoring())
            return;

        updateChildren();

        for(size_t i=0;i<pcSnapshots.size();++i)
            if(pcSnapshots[i])
                getSnapshot(i,true);
    }

    void updateChildren() {
        if(isLinked() && !pcLinked->isDerivedFrom(ViewProviderLink::getClassTypeId())) {
            if(!pcLinked->getChildRoot()) {
                if(!App::GeoFeatureGroupExtension::isNonGeoGroup(pcLinked->getObject())) {
                    auto linked = pcLinked->getObject()->getLinkedObject(true);
                    ViewProvider *vp = 0;
                    if(linked && linked!=pcLinked->getObject())
                        vp = Application::Instance->getViewProvider(linked);
                    if(!vp || !vp->getChildRoot()) {
                        childSensor.detach();
                        _updateChildren(pcLinked->claimChildren());
                        return;
                    }
                }
            } else if (!ViewParams::getLinkChildrenDirect()) {
                if(childSensor.getAttachedNode() != pcLinked->getChildRoot()) {
                    childSensor.detach();
                    childSensor.attach(pcLinked->getChildRoot());
                }
                _updateChildren(pcLinked->claimChildren3D());
                return;
            }
        }
        coinRemoveAllChildren(pcChildGroup);
        childSensor.detach();
        nodeMap.clear();
        pcChildGroup.reset();
    }

    void _updateChildren(const std::vector<App::DocumentObject *> &children) {

        int childCount = 0;
        if(!pcChildGroup)
            pcChildGroup = new SoGroup;

        NodeMap nodeMap;
        for(auto child : children) {
            Pointer info = get(child,nullptr);
            if(!info) continue;
            SoNode *node = info->getSnapshot(LinkView::SnapshotChild);
            if(!node) continue;
            nodeMap[node] = info;
            addChild(pcChildGroup, node, childCount);
        }

        while(childCount < pcChildGroup->getNumChildren())
            pcChildGroup->removeChild(pcChildGroup->getNumChildren()-1);

        // Use swap instead of clear() here to avoid potential link
        // destruction
        this->nodeMap.swap(nodeMap);
    }

    bool getElementPicked(bool addname, int type,
            const SoPickedPoint *pp, std::ostream &str) const
    {
        if(!pp || !isLinked() || (!pcLinked->isSelectable() && !ViewParams::getOverrideSelectability()))
            return false;

        if(addname)
            str << getLinkedName() <<'.';

        SoPath *path = pp->getPath();
        auto pcSwitch = type!=LinkView::SnapshotMax?pcSwitches[type]:0;
        if(pcSwitch && pcChildGroup) {
            int index = path->findNode(pcChildGroup);
            if(index>=0) {
                auto it = nodeMap.find(path->getNode(index+1));
                if(it==nodeMap.end())
                    return false;
                return it->second->getElementPicked(true,LinkView::SnapshotChild,pp,str);
            }
        }
        std::string subname;
        if(!pcLinked->getElementPicked(pp,subname))
            return false;
        str<<subname;
        return true;
    }

    static const char *checkSubname(App::DocumentObject *obj, const char *subname) {
#define CHECK_NAME(_name,_end) do{\
            if(!_name) return 0;\
            const char *_n = _name;\
            for(;*subname && *_n; ++subname,++_n)\
                if(*subname != *_n) break;\
            if(*_n || (*subname!=0 && *subname!=_end))\
                    return 0;\
            if(*subname == _end) ++subname;\
        }while(0)

        // if(subname[0] == '*') {
        //     ++subname;
        //     CHECK_NAME(obj->getDocument()->getName(),'*');
        // }
        CHECK_NAME(obj->getNameInDocument(),'.');
        return subname;
    }

    bool getDetail(bool checkname, int type, const char* subname,
            SoDetail *&det, SoFullPath *path) const
    {
        if(!isLinked())
            return false;

        if(checkname) {
            subname = checkSubname(pcLinked->getObject(),subname);
            if(!subname)
                return false;
        }

        if(type == LinkView::SnapshotMax)
            return pcLinked->getDetailPath(subname,path,true,det);

        if(!pcSnapshots[type] || pcSnapshots[type]->findChild(pcSwitches[type]) < 0) {
            if(path) {
                if(!appendPathSafe(path,pcSnapshots[type]))
                    return false;
            }
            // this is possible in case of editing, where the switch node
            // of the linked view object is temparaly removed from its root
            return true;
        }
        int len = 0;
        if(path) {
            len = path->getLength();
            if(!appendPathSafe(path,pcSnapshots[type]))
                return false;
            appendPath(path,pcSwitches[type]);
        }

        auto pcSwitch = pcSwitches[type];
        if(*subname == 0 || !pcChildGroup || !pcSwitch || Data::ComplexGeoData::isElementName(subname))
            return pcLinked->getDetailPath(subname,path,false,det);

        if(path){
            appendPath(path,pcChildGroup);
            type = LinkView::SnapshotChild;
        }

        // Special handling of nodes with childRoot, especially geo feature
        // group. It's object hierarchy in the tree view (i.e. in subname) is
        // different from its coin hierarchy. All objects under a geo feature
        // group is visually grouped directly under the group's childRoot,
        // even though some object has secondary hierarchy in subname. E.g.
        //
        // Body
        //   |--Pad
        //       |--Sketch
        //
        //  Both Sketch and Pad's coin nodes are grouped directly under Body as,
        //
        // Body
        //   |--Pad
        //   |--Sketch

        const char *dot = strchr(subname,'.');
        const char *nextsub = subname;
        if(!dot)
            return false;
        auto obj = pcLinked->getObject();
        auto sobj = obj;
        while(1) {
            std::string objname = std::string(nextsub,dot-nextsub+1);
            if(!obj->getSubObject(objname.c_str())) {
                // sub object is not found, abort.
                break;
            }
            auto ssobj = sobj->getSubObject(objname.c_str());
            if(!ssobj) {
                FC_ERR("invalid sub name " << nextsub << " of object " << sobj->getFullName());
                return false;
            }
            // Sub object found, remember this subname
            subname = nextsub;
            sobj = ssobj;

            if(ViewParams::getMapChildrenPlacement()
                    || sobj->hasExtension(App::LinkBaseExtension::getExtensionClassTypeId())) {
                // Link will contain all its children
                break;
            }
            auto vp = Application::Instance->getViewProvider(sobj);
            if(!vp) {
                FC_LOG("cannot find view provider of " << sobj->getFullName());
                return false;
            }
            if(vp->getChildRoot()) {
                // In case the children is also a geo group, it will visually
                // hold all of its own children, so stop going further down.
                break;
            }
            // new style mapped sub-element
            if(Data::ComplexGeoData::isMappedElement(dot+1))
                break;
            auto next = strchr(dot+1,'.');
            if(!next) {
                // no dot any more, the following must be a sub-element
                break;
            }
            nextsub = dot+1;
            dot = next;
        }

        for(auto v : nodeMap) {
            if(v.second->isLinked() && v.second->pcLinked->getObject() == sobj) {
                if(v.second->getDetail(true,type,subname,det,path))
                    return true;
                break;
            }
        }
        if(path)
            path->truncate(len);
        return false;
    }

    void slotChangeIcon() {
        iconMap.clear();
        if(!isLinked())
            return;
        LinkInfoPtr me(this);
        for(auto link : links)
            link->onLinkedIconChange(me);
    }

    QIcon getIcon(const QPixmap &px) {
        if(!isLinked())
            return QIcon();

        if(px.isNull())
            return pcLinked->getIcon();
        QIcon &iconLink = iconMap[px.cacheKey()];
        if(iconLink.isNull()) {
            QIcon icon = pcLinked->getIcon();
            iconLink = QIcon();
            iconLink.addPixmap(BitmapFactory().merge(icon.pixmap(64, QIcon::Normal, QIcon::Off),
                px,BitmapFactoryInst::BottomLeft), QIcon::Normal, QIcon::Off);
            iconLink.addPixmap(BitmapFactory().merge(icon.pixmap(64, QIcon::Normal, QIcon::On ),
                px,BitmapFactoryInst::BottomLeft), QIcon::Normal, QIcon::On);
        }
        return iconLink;
    }
};

#ifdef _MSC_VER
void intrusive_ptr_add_ref(Gui::LinkInfo *px){
    px->addref();
}

void intrusive_ptr_release(Gui::LinkInfo *px){
    px->release();
}
#endif

////////////////////////////////////////////////////////////////////////////////////

EXTENSION_TYPESYSTEM_SOURCE(Gui::ViewProviderLinkObserver,Gui::ViewProviderExtension)

ViewProviderLinkObserver::ViewProviderLinkObserver() {
    // TODO: any better way to get deleted automatically?
    m_isPythonExtension = true;
    initExtensionType(ViewProviderLinkObserver::getExtensionClassTypeId());
}

ViewProviderLinkObserver::~ViewProviderLinkObserver() {
    if(linkInfo) {
        linkInfo->detach(true);
        linkInfo.reset();
    }
}

bool ViewProviderLinkObserver::isLinkVisible() const {
    if(linkInfo)
        return linkInfo->isVisible();
    return true;
}

void ViewProviderLinkObserver::setLinkVisible(bool visible) {
    if(linkInfo)
        linkInfo->setVisible(visible);
}

void ViewProviderLinkObserver::extensionBeforeDelete() {
    if(linkInfo)
        linkInfo->detach(false);
}

void ViewProviderLinkObserver::extensionReattach(App::DocumentObject *) {
    if(linkInfo) {
        linkInfo->pcLinked =
            Base::freecad_dynamic_cast<ViewProviderDocumentObject>(getExtendedContainer());
        linkInfo->update();
    }
}

void ViewProviderLinkObserver::extensionOnChanged(const App::Property *prop) {
    (void)prop;
}

void ViewProviderLinkObserver::extensionModeSwitchChange() {
    auto owner = freecad_dynamic_cast<ViewProviderDocumentObject>(getExtendedContainer());
    if(owner && linkInfo)
        linkInfo->updateSwitch();
}

void ViewProviderLinkObserver::extensionUpdateData(const App::Property *prop) {
    if(linkInfo && linkInfo->pcLinked && linkInfo->pcLinked->getObject() &&
       prop != &linkInfo->pcLinked->getObject()->Visibility)
        linkInfo->updateData(prop);
}

void ViewProviderLinkObserver::extensionFinishRestoring() {
    if(linkInfo) {
        FC_TRACE("linked finish restoing");
        linkInfo->update();
    }
}

class LinkView::SubInfo : public LinkOwner {
public:
    LinkInfoPtr linkInfo;
    LinkView &handle;
    CoinPtr<SoSeparator> pcNode;
    CoinPtr<SoMatrixTransform> pcTransform;
    std::set<std::string> subElements;

    friend LinkView;

    SubInfo(LinkView &handle):handle(handle) {
        pcNode = new SoFCSelectionRoot(true);
        pcTransform = new SoMatrixTransform;
        pcNode->addChild(pcTransform);
    }

    ~SubInfo() override {
        unlink();
        auto root = handle.getLinkRoot();
        if(root) {
            int idx = root->findChild(pcNode);
            if(idx>=0)
                root->removeChild(idx);
        }
    }

    void onLinkedIconChange(LinkInfoPtr) override {
        if(handle.autoSubLink && handle.subInfo.size()==1)
            handle.onLinkedIconChange(handle.linkInfo);
    }

    void unlink(LinkInfoPtr info=LinkInfoPtr()) override {
        (void)info;
        if(linkInfo) {
            linkInfo->remove(this);
            linkInfo.reset();
        }
        coinRemoveAllChildren(pcNode);
        pcNode->addChild(pcTransform);
    }

    void link(App::DocumentObject *obj) {
        if(isLinked() && linkInfo->pcLinked->getObject()==obj)
            return;
        unlink();
        linkInfo = LinkInfo::get(obj,this);
        if(linkInfo)
            pcNode->addChild(linkInfo->getSnapshot(LinkView::SnapshotTransform));
    }

    bool isLinked() const{
        return linkInfo && linkInfo->isLinked();
    }
};

//////////////////////////////////////////////////////////////////////////////////

class LinkView::Element : public LinkOwner {
public:
    LinkInfoPtr linkInfo;
    LinkView &handle;
    CoinPtr<SoSwitch> pcSwitch;
    CoinPtr<SoSeparator> pcRoot;
    CoinPtr<SoMatrixTransform> pcTransform;
    int nodeType;
    int groupIndex = -1;
    int isGroup = 0;
    bool registered = false;

    friend LinkView;

    Element(LinkView &handle):handle(handle) {
        nodeType = handle.childType;
    }

    ~Element() override {
        unlink();
        auto root = handle.getLinkRoot();
        if(pcRoot && root) {
            int idx = root->findChild(pcRoot);
            if(idx>=0)
                root->removeChild(idx);
        }
        unlink();
    }

    SoNode *getTopNode() {
        if(pcSwitch)
            return pcSwitch;
        assert(pcRoot);
        return pcRoot;
    }

    void appendToPath(SoPath *path) {
        if(pcSwitch)
            appendPath(path,pcSwitch);
        if (pcRoot && isGroup>0)
            appendPath(path,pcRoot);
    }

    void unlink(LinkInfoPtr info=LinkInfoPtr()) override{
        (void)info;
        if (registered) {
            _registerLinkNode(pcRoot);
            registered =false;
        }
        if(linkInfo) {
            linkInfo->remove(this);
            linkInfo.reset();
        }
        if(pcSwitch && isGroup>0) {
            coinRemoveAllChildren(pcRoot);
        } else
            pcRoot.reset();
        isGroup = 0;
    }

    void link(App::DocumentObject *obj) {
        if(isLinked() && linkInfo->pcLinked->getObject()==obj)
            return;
        unlink();
        linkInfo = LinkInfo::get(obj,this);
        if(!isLinked())
            return;

        isGroup = App::GeoFeatureGroupExtension::isNonGeoGroup(obj)?1:0;

        nodeType = handle.childType;
        if(nodeType == LinkView::SnapshotMax) {
            pcSwitch.reset();
            pcRoot = linkInfo->pcLinked->getRoot();
            if(isGroup)
                isGroup = -1;
            return;
        }

        if(!pcSwitch)
            pcSwitch = new SoFCSwitch;
        else
            coinRemoveAllChildren(pcSwitch);

        if(isGroup<=0)
            pcRoot = linkInfo->getSnapshot(nodeType);
        else {
            if(!pcRoot)
                pcRoot = new SoFCSelectionRoot(true);
            else
                coinRemoveAllChildren(pcRoot);
            registered = true;
            _registerLinkNode(pcRoot, obj);
        }
        pcSwitch->addChild(pcRoot);
        pcSwitch->whichChild = 0;
    }

    SoNode *initGroup() {
        if(isGroup<=0)
            return 0;
        coinRemoveAllChildren(pcRoot);

        // In case the plain group has its own display mode, we add it as the
        // first child.
        if(isLinked() && linkInfo->pcLinked->getDefaultMode()>=0) {
            nodeType = SnapshotVisible;
            auto node = linkInfo->getSnapshot(nodeType);
            pcRoot->addChild(node);
            return node;
        }
        return 0;
    }

    bool isLinked() const{
        return linkInfo && linkInfo->isLinked();
    }
};

///////////////////////////////////////////////////////////////////////////////////

TYPESYSTEM_SOURCE(Gui::LinkView,Base::BaseClass)

LinkView::LinkView()
    :nodeType(SnapshotTransform)
    ,childType((SnapshotType)-1),autoSubLink(true)
{
    pcLinkRoot = new SoFCSelectionRoot;
}

LinkView::~LinkView() {
    unlink(linkInfo);
    unlink(linkOwner);
}

PyObject *LinkView::getPyObject()
{
    if (PythonObject.is(Py::_None()))
        PythonObject = Py::asObject(new LinkViewPy(this));
    return Py::new_reference_to(PythonObject);
}

void LinkView::setInvalid() {
    if (!PythonObject.is(Py::_None())){
        auto obj = static_cast<Base::PyObjectBase*>(PythonObject.ptr());
        obj->setInvalid();
        obj->DecRef();
    }else
        delete this;
}

Base::BoundBox3d LinkView::getBoundBox(ViewProviderDocumentObject *vpd) const {
    if(!vpd) {
        if(!linkOwner || !linkOwner->isLinked())
            LINK_THROW(Base::ValueError,"no ViewProvider");
        vpd = linkOwner->pcLinked;
    }
    return vpd->getBoundingBox();
}

ViewProviderDocumentObject *LinkView::getOwner() const {
    if(linkOwner && linkOwner->isLinked())
        return linkOwner->pcLinked;
    return nullptr;
}

void LinkView::setOwner(ViewProviderDocumentObject *vpd) {
    unlink(linkOwner);
    linkOwner = LinkInfo::get(vpd,this);
}

bool LinkView::isLinked() const{
    return linkInfo && linkInfo->isLinked();
}

void LinkView::setDrawStyle(int style, double lineWidth, double pointSize) {
    if(!pcDrawStyle) {
        if(!style)
            return;
        pcDrawStyle = new SoDrawStyle;
        pcDrawStyle->style = SoDrawStyle::FILLED;
        pcLinkRoot->insertChild(pcDrawStyle,0);
    }
    if(!style) {
        pcDrawStyle->setOverride(false);
        return;
    }
    pcDrawStyle->lineWidth = lineWidth;
    pcDrawStyle->pointSize = pointSize;
    switch(style) {
    case 2:
        pcDrawStyle->linePattern = 0xf00f;
        break;
    case 3:
        pcDrawStyle->linePattern = 0x0f0f;
        break;
    case 4:
        pcDrawStyle->linePattern = 0xff88;
        break;
    default:
        pcDrawStyle->linePattern = 0xffff;
    }
    pcDrawStyle->setOverride(true);
}

void LinkView::renderDoubleSide(bool enable) {
    if(enable) {
        if(!pcShapeHints) {
            pcShapeHints = new SoShapeHints;
            pcShapeHints->vertexOrdering = SoShapeHints::CLOCKWISE;
            pcShapeHints->shapeType = SoShapeHints::UNKNOWN_SHAPE_TYPE;
            pcLinkRoot->insertChild(pcShapeHints,0);
        }
        pcShapeHints->setOverride(true);
    }else if(pcShapeHints)
        pcShapeHints->setOverride(false);
}

void LinkView::setMaterial(int index, const App::Material *material) {
    if(index < 0) {
        if(!material) {
            pcLinkRoot->removeColorOverride();
            return;
        }
        App::Color c = material->diffuseColor;
        c.a = material->transparency;
        pcLinkRoot->setColorOverride(c);
        for(int i=0;i<getSize();++i)
            setMaterial(i,nullptr);
    }else if(index >= (int)nodeArray.size())
        LINK_THROW(Base::ValueError,"LinkView: material index out of range");
    else {
        auto &info = *nodeArray[index];
        if(!material) {
            if(info.pcRoot && info.pcRoot->isOfType(SoFCSelectionRoot::getClassTypeId()))
                static_cast<SoFCSelectionRoot*>(info.pcRoot.get())->removeColorOverride();
            return;
        }
        App::Color c = material->diffuseColor;
        c.a = material->transparency;
        if(info.pcRoot && info.pcRoot->isOfType(SoFCSelectionRoot::getClassTypeId()))
            static_cast<SoFCSelectionRoot*>(info.pcRoot.get())->setColorOverride(c);
    }
}

void LinkView::setLink(App::DocumentObject *obj, const std::vector<std::string> &subs) {
    setLinkViewObject(Base::freecad_dynamic_cast<ViewProviderDocumentObject>(
            Application::Instance->getViewProvider(obj)),subs);

}

void LinkView::setLinkViewObject(ViewProviderDocumentObject *vpd,
        const std::vector<std::string> &subs)
{
    if(!isLinked() || linkInfo->pcLinked != vpd) {
        unlink(linkInfo);
        linkInfo = LinkInfo::get(vpd,this);
        if(!linkInfo)
            return;
    }
    subInfo.clear();
    for(const auto &sub : subs) {
        if(sub.empty()) continue;
        std::string subname = Data::ComplexGeoData::noElementName(sub.c_str());
        std::string element = Data::ComplexGeoData::oldElementName(
                Data::ComplexGeoData::findElementName(sub.c_str()));
        auto it = subInfo.find(subname);
        if(it == subInfo.end()) {
            it = subInfo.insert(std::make_pair(subname,std::unique_ptr<SubInfo>())).first;
            it->second.reset(new SubInfo(*this));
        }
        if(!element.empty())
            it->second->subElements.insert(std::move(element));
    }
    updateLink();
}

void LinkView::setTransform(SoMatrixTransform *pcTransform, const Base::Matrix4D &mat) {
    if(!pcTransform)
        return;
    double dMtrx[16];
    mat.getGLMatrix(dMtrx);
    pcTransform->matrix = SbMatrix(dMtrx[0], dMtrx[1], dMtrx[2],  dMtrx[3],
                                   dMtrx[4], dMtrx[5], dMtrx[6],  dMtrx[7],
                                   dMtrx[8], dMtrx[9], dMtrx[10], dMtrx[11],
                                   dMtrx[12],dMtrx[13],dMtrx[14], dMtrx[15]);
}

void LinkView::setSize(int _size) {
    size_t size = _size<0?0:(size_t)_size;
    if(childType<0 && size==nodeArray.size())
        return;
    resetRoot();
    if(!size || childType>=0) {
        nodeArray.clear();
        nodeMap.clear();
        if(!size && childType<0) {
            if(pcLinkedRoot)
                pcLinkRoot->addChild(pcLinkedRoot);
            return;
        }
        childType = SnapshotContainer;
    }
    if(size<nodeArray.size()) {
        for(size_t i=size;i<nodeArray.size();++i)
            nodeMap.erase(nodeArray[i]->pcSwitch);
        nodeArray.resize(size);
    }
    for(const auto &info : nodeArray)
        pcLinkRoot->addChild(info->pcSwitch);

    while(nodeArray.size()<size) {
        nodeArray.push_back(std::unique_ptr<Element>(new Element(*this)));
        auto &info = *nodeArray.back();
        info.pcTransform = new SoMatrixTransform;
        info.pcSwitch = new SoFCSwitch;
        info.pcRoot = new SoFCSelectionRoot;
        info.pcRoot->addChild(info.pcTransform);
        info.pcSwitch->addChild(info.pcRoot);
        info.pcSwitch->whichChild = 0;
        if(pcLinkedRoot)
            info.pcRoot->addChild(pcLinkedRoot);
        pcLinkRoot->addChild(info.pcSwitch);
        nodeMap.emplace(info.pcSwitch,(int)nodeArray.size()-1);
    }
}

void LinkView::resetRoot() {
    nameMap.clear();
    coinRemoveAllChildren(pcLinkRoot);
    if(pcTransform)
        pcLinkRoot->addChild(pcTransform);
    if(pcShapeHints)
        pcLinkRoot->addChild(pcShapeHints);
    if(pcDrawStyle)
        pcLinkRoot->addChild(pcDrawStyle);
}

bool LinkView::isLikeGroup() const {
    return getSize() || (!hasSubs() && linkInfo && linkInfo->pcChildGroup);
}

void LinkView::setChildren(const std::vector<App::DocumentObject*> &children,
        const boost::dynamic_bitset<> &vis, SnapshotType type)
{
    if(children.empty()) {
        if(!nodeArray.empty()) {
            nameMap.clear();
            nodeArray.clear();
            nodeMap.clear();
            childType = SnapshotContainer;
            resetRoot();
            if(pcLinkedRoot)
                pcLinkRoot->addChild(pcLinkedRoot);
        }
        return;
    }

    if(type<0 || type>SnapshotMax)
        LINK_THROW(Base::ValueError,"invalid children type");

    if(childType<0 || childType!=type) {
        nodeArray.clear();
    } else {
        // checking for shortcut of adding or removing exactly one element This
        // has potential of huge slow down in operations batch calling of
        // add/removeObject()

        int idx = -1;
        if (children.size() + 1 == nodeArray.size()) {
            // removing an object
            int i = -1;
            for (const auto obj : children) {
                auto &info = *nodeArray[++i];
                if(!info.isLinked()) {
                    idx = -1; // not linked, so no pcRoot and thus must reset
                    break;
                }
                else if (info.linkInfo->pcLinked->getObject() != obj) {
                    if (idx >= 0) {
                        idx = -1;
                        break;
                    }
                    if (!nodeArray[i+1]->isLinked()
                            || nodeArray[i+1]->linkInfo->pcLinked->getObject() != obj)
                        break;
                    idx = i++;
                }
            }
            if (idx >= 0) {
                auto &info = *nodeArray[idx];
                auto node = info.getTopNode();
                nameMap.clear();
                nodeMap.erase(node);
                int nidx = pcLinkRoot->findChild(node);
                if (nidx >= 0)
                    pcLinkRoot->removeChild(nidx);
                nodeArray.erase(nodeArray.begin() + nidx);
            }
        }
        else if (children.size() == nodeArray.size() + 1) {
            // Adding an object
            int i = -1;
            idx = (int)children.size()-1;
            for (const auto &pinfo : nodeArray) {
                auto &info = *pinfo;
                if (!info.isLinked()) {
                    idx = -1;
                    break;
                }
                auto obj = children[++i];
                if (info.linkInfo->pcLinked->getObject() != obj) {
                    if (idx >= 0) {
                        idx = -1;
                        break;
                    }
                    idx = i++;
                    if (info.linkInfo->pcLinked->getObject() != children[i]) {
                        idx = -1;
                        break;
                    }
                }
            }
            if (idx >= 0) {
                auto obj = children[idx];
                if (App::GeoFeatureGroupExtension::isNonGeoGroup(obj)
                        || App::GroupExtension::getGroupOfObject(obj)) {
                    idx = -1;
                } else {
                    int pos = -1;
                    if (idx < static_cast<int>(nodeArray.size())) {
                        pos = pcLinkRoot->findChild((*nodeArray.begin())->getTopNode());
                        if (pos < 0 || pos + idx >= pcLinkRoot->getNumChildren())
                            idx = -1;
                        else
                            pos += idx;
                    }
                    if (idx >= 0) {
                        nodeArray.emplace(nodeArray.begin()+idx, new Element(*this));
                        auto &info = *nodeArray[idx];
                        info.groupIndex = -1;
                        info.link(obj);
                        auto node = info.getTopNode();
                        for (auto it = nodeArray.begin()+idx; it != nodeArray.end(); ++it) {
                            auto n = (*it)->getTopNode();
                            nodeMap[n] = it - nodeArray.begin();
                        }
                        if (pos < 0)
                            pcLinkRoot->addChild(node);
                        else
                            pcLinkRoot->insertChild(node, pos);
                    }
                }
            }
        }
        if (idx >= 0) {
            int i = -1;
            for (const auto &pinfo : nodeArray) {
                ++i;
                auto &info = *pinfo;
                if(info.pcSwitch && childType!=SnapshotChild) {
                    int which = ((int)vis.size()<=i||vis[i])?0:-1;
                    if (info.pcSwitch->whichChild.getValue() != which)
                        info.pcSwitch->whichChild = which;
                }
                if (i >= idx)
                    nodeMap[info.getTopNode()] = i;
            }
            return;
        }
    }

    resetRoot();

    nameMap.clear();
    nodeMap.clear();
    childType = type;

    if(nodeArray.size() > children.size())
        nodeArray.resize(children.size());
    else
        nodeArray.reserve(children.size());

    std::map<App::DocumentObject*, size_t> groups;
    for(size_t i=0;i<children.size();++i) {
        auto obj = children[i];
        if(nodeArray.size()<=i)
            nodeArray.push_back(std::unique_ptr<Element>(new Element(*this)));
        auto &info = *nodeArray[i];
        info.groupIndex = -1;
        info.link(obj);
        if(info.pcSwitch && childType!=SnapshotChild) {
            int which = (vis.size()<=i||vis[i])?0:-1;
            if (info.pcSwitch->whichChild.getValue() != which)
                info.pcSwitch->whichChild = which;
        }
        if(info.isGroup>0) {
            auto node = info.initGroup();
            if(node)
                nodeMap[node] = i;
            groups.emplace(obj,i);
        }
    }
    for(size_t i=0;i<nodeArray.size();++i) {
        auto &info = *nodeArray[i];
        if(!info.isLinked())
            continue;
        nodeMap.emplace(info.getTopNode(),i);
        if(!groups.empty()) {
            auto iter = groups.find(App::GroupExtension::getGroupOfObject(
                            info.linkInfo->pcLinked->getObject()));
            if(iter != groups.end()) {
                info.groupIndex = iter->second;
                auto &groupInfo = *nodeArray[iter->second];
                groupInfo.pcRoot->addChild(info.getTopNode());
                continue;
            }
        }
        pcLinkRoot->addChild(info.getTopNode());
    }
}

std::vector<ViewProviderDocumentObject*> LinkView::getChildren() const {
    std::vector<ViewProviderDocumentObject*> ret;
    for(const auto &info : nodeArray) {
        if(info->isLinked())
            ret.push_back(info->linkInfo->pcLinked);
    }
    return ret;
}

void LinkView::setTransform(int index, const Base::Matrix4D &mat) {
    if(index<0) {
        if(!pcTransform) {
            pcTransform = new SoMatrixTransform;
            pcLinkRoot->insertChild(pcTransform,0);
        }
        setTransform(pcTransform,mat);
        return;
    }
    if(index<0 || index>=(int)nodeArray.size())
        LINK_THROW(Base::ValueError,"LinkView: index out of range");
    setTransform(nodeArray[index]->pcTransform,mat);
}

void LinkView::setElementVisible(int idx, bool visible) {
    if(idx>=0 && idx<(int)nodeArray.size()) {
        if(nodeArray[idx]->pcSwitch && nodeArray[idx]->nodeType!=SnapshotChild)
            nodeArray[idx]->pcSwitch->whichChild = visible?0:-1;
    }
}

bool LinkView::isElementVisible(int idx) const {
    if(idx>=0 && idx<(int)nodeArray.size()) {
        if(!nodeArray[idx]->pcSwitch)
            return true;
        return nodeArray[idx]->pcSwitch->whichChild.getValue()>=0;
    }
    return false;
}

ViewProviderDocumentObject *LinkView::getLinkedView() const {
    auto link = linkInfo;
    if(autoSubLink && subInfo.size()==1)
        link = subInfo.begin()->second->linkInfo;
    return link?link->pcLinked:nullptr;
}

std::vector<std::string> LinkView::getSubNames() const {
    std::vector<std::string> ret;
    for(const auto &v : subInfo) {
        if(v.second->subElements.empty()) {
            ret.push_back(v.first);
            continue;
        }
        for(const auto &s : v.second->subElements)
            ret.push_back(v.first+s);
    }
    return ret;
}

void LinkView::setNodeType(SnapshotType type, bool sublink) {
    autoSubLink = sublink;
    if(nodeType==type)
        return;
    if(type>=SnapshotMax ||
       (type<0 && type!=SnapshotContainer && type!=SnapshotContainerTransform))
        LINK_THROW(Base::ValueError,"LinkView: invalid node type");

    if(nodeType>=0 && type<0) {
        if(pcLinkedRoot) {
            SoSelectionElementAction action(SoSelectionElementAction::None,true);
            action.apply(pcLinkedRoot);
        }
        replaceLinkedRoot(CoinPtr<SoSeparator>(new SoFCSelectionRoot));
    }else if(nodeType<0 && type>=0) {
        if(isLinked())
            replaceLinkedRoot(linkInfo->getSnapshot(type));
        else
            replaceLinkedRoot(nullptr);
    }
    nodeType = type;
    updateLink();
}

void LinkView::replaceLinkedRoot(SoSeparator *root) {
    if(root==pcLinkedRoot)
        return;
    if(nodeArray.empty()) {
        if(pcLinkedRoot && root)
            pcLinkRoot->replaceChild(pcLinkedRoot,root);
        else if(root)
            pcLinkRoot->addChild(root);
        else
            resetRoot();
    }else if(childType<0) {
        if(pcLinkedRoot && root) {
            for(const auto &info : nodeArray)
                info->pcRoot->replaceChild(pcLinkedRoot,root);
        }else if(root) {
            for(const auto &info : nodeArray)
                info->pcRoot->addChild(root);
        }else{
            for(const auto &info : nodeArray)
                info->pcRoot->removeChild(pcLinkedRoot);
        }
    }
    pcLinkedRoot = root;
}

void LinkView::onLinkedIconChange(LinkInfoPtr info) {
    if(info==linkInfo && info!=linkOwner && linkOwner && linkOwner->isLinked())
        linkOwner->pcLinked->signalChangeIcon();
}

void LinkView::onLinkedUpdateData(LinkInfoPtr info, const App::Property *prop) {
    if(info!=linkInfo || !linkOwner || !linkOwner->isLinked() || info==linkOwner)
        return;
    auto ext = linkOwner->pcLinked->getObject()->getExtensionByType<App::LinkBaseExtension>(true);
    if (ext && !(prop->getType() & App::Prop_Output) &&
            !prop->testStatus(App::Property::Output))
    {
        // propagate the signalChangedObject to potentially multiple levels
        // of links, to inform tree view of children change, and other
        // parent objects about the change. But we need to be careful to not
        // touch the object if the property of change is marked as output.
        ext->_LinkTouched.touch();
    }else{
        // In case the owner object does not have link extension, here is a
        // trick to link the signalChangedObject from linked object to the
        // owner
        linkOwner->pcLinked->getDocument()->signalChangedObject(
                *linkOwner->pcLinked,linkOwner->pcLinked->getObject()->Label);
    }
}

void LinkView::updateLink() {
    if(!isLinked())
        return;

    if(linkOwner && linkOwner->isLinked() && linkOwner->pcLinked->isRestoring()) {
        FC_TRACE("restoring '" << linkOwner->pcLinked->getObject()->getFullName() << "'");
        return;
    }

    // TODO: is it a good idea to clear any selection here?
    pcLinkRoot->resetContext();

    if(nodeType >= 0) {
        replaceLinkedRoot(linkInfo->getSnapshot(nodeType));
        return;
    }

    // rebuild link sub objects tree
    CoinPtr<SoSeparator> linkedRoot = pcLinkedRoot;
    if(!linkedRoot)
        linkedRoot = new SoFCSelectionRoot;
    else{
        SoSelectionElementAction action(SoSelectionElementAction::None,true);
        action.apply(linkedRoot);
        coinRemoveAllChildren(linkedRoot);
    }

    SoTempPath path(10);
    path.ref();
    appendPath(&path,linkedRoot);
    auto obj = linkInfo->pcLinked->getObject();
    for(const auto &v : subInfo) {
        auto &sub = *v.second;
        Base::Matrix4D mat;
        App::DocumentObject *sobj = obj->getSubObject(
                v.first.c_str(), nullptr, &mat, nodeType==SnapshotContainer);
        if(!sobj) {
            sub.unlink();
            continue;
        }
        sub.link(sobj);
        linkedRoot->addChild(sub.pcNode);
        setTransform(sub.pcTransform,mat);

        if(!sub.subElements.empty()) {
            path.truncate(1);
            appendPath(&path,sub.pcNode);
            SoSelectionElementAction action(SoSelectionElementAction::Append,true);
            for(const auto &subelement : sub.subElements) {
                path.truncate(2);
                SoDetail *det = nullptr;
                if(!sub.linkInfo->getDetail(false,SnapshotTransform,subelement.c_str(),det,&path))
                    continue;
                action.setElement(det);
                action.apply(&path);
                delete det;
            }
        }
    }
    path.unrefNoDelete();
    replaceLinkedRoot(linkedRoot);
}

bool LinkView::linkGetElementPicked(const SoPickedPoint *pp, std::string &subname) const
{
    std::ostringstream ss;
    CoinPtr<SoPath> path = pp->getPath();
    if(!nodeArray.empty()) {
        auto idx = path->findNode(pcLinkRoot);
        if(idx<0 || idx+2>=path->getLength())
            return false;
        auto node = path->getNode(idx+1);
        auto it = nodeMap.find(node);
        if(it == nodeMap.end())
            return false;
        int nodeIdx = it->second;
        ++idx;
        while(nodeArray[nodeIdx]->isGroup>0) {
            auto &info = *nodeArray[nodeIdx];
            if(!info.isLinked())
                return false;
            ss << info.linkInfo->getLinkedName() << '.';
            idx += info.pcSwitch?2:1;
            if(idx>=path->getLength())
                return false;
            auto iter = nodeMap.find(path->getNode(idx));
            if(iter == nodeMap.end())
                return false;
            if(nodeIdx == iter->second) {
                // In case the plain group has its own mode, we'll insert a
                // snapshot as the first child. So it is possible for the
                // nodeMap points to the group itself.
                break;
            }
            nodeIdx = iter->second;
        }
        auto &info = *nodeArray[nodeIdx];
        if(childType!=SnapshotMax && nodeIdx==it->second)
            ss << it->second << '.';
        else
            ss << info.linkInfo->getLinkedName() << '.';
        if(info.isLinked()) {
            if(!info.linkInfo->getElementPicked(false,info.nodeType,pp,ss))
                return false;
            subname = ss.str();
            return true;
        }
    }

    if(!isLinked())
        return false;

    if(nodeType >= 0) {
        if(linkInfo->getElementPicked(false,nodeType,pp,ss)) {
            subname = ss.str();
            return true;
        }
        return false;
    }
    auto idx = path->findNode(pcLinkedRoot);
    if(idx<0 || idx+1>=path->getLength())
        return false;
    auto node = path->getNode(idx+1);
    for(const auto &v : subInfo) {
        auto &sub = *v.second;
        if(node != sub.pcNode) continue;
        std::ostringstream ss2;
        if(!sub.linkInfo->getElementPicked(false,SnapshotTransform,pp,ss2))
            return false;
        const std::string &element = ss2.str();
        if(!sub.subElements.empty()) {
            auto pos = element.find('.');
            if (pos == std::string::npos)
                pos = 0;
            else
                pos += 1;
            if(sub.subElements.find(element.c_str()+pos)==sub.subElements.end())
                return false;
        }
        if(!autoSubLink || subInfo.size()>1)
            ss << v.first;
        ss << element;
        subname = ss.str();
        return true;
    }
    return false;
}

bool LinkView::getGroupHierarchy(int index, SoFullPath *path) const {
    if(index > (int)nodeArray.size())
        return false;
    auto &info = *nodeArray[index];
    if(info.groupIndex>=0 && !getGroupHierarchy(info.groupIndex,path))
        return false;
    info.appendToPath(path);
    return true;
}

bool LinkView::linkGetDetailPath(const char *subname, SoFullPath *path, SoDetail *&det) const
{
    if(!subname || *subname==0)
        return true;
    auto len = path->getLength();
    if(nodeArray.empty()) {
        if(!appendPathSafe(path,pcLinkRoot))
            return false;
    } else {
        int idx = -1;
        if (subname[0]>='0' && subname[0]<='9') {
            idx = App::LinkBaseExtension::getArrayIndex(subname,&subname);
        } else {
            while(1) {
                const char *dot = strchr(subname,'.');
                if(!dot)
                    break;
                int i = 0;
                if (subname[0] == '$') {
                    CharRange name(subname+1,dot);
                    for(const auto &info : nodeArray) {
                        if(info->isLinked() && boost::equals(name,info->linkInfo->getLinkedLabel())) {
                            idx = i;
                            break;
                        }
                        ++i;
                    }
                } else if (nodeArray.size() < 10) {
                    CharRange name(subname,dot);
                    for(auto &info : nodeArray) {
                        if(info->isLinked() && boost::equals(name,info->linkInfo->getLinkedName())) {
                            idx = i;
                            break;
                        }
                        ++i;
                    }
                } else {
                    if(nameMap.size()!=nodeArray.size()) {
                        for(auto &info : nodeArray) {
                            if(info->isLinked())
                                nameMap[info->linkInfo->getLinkedName()] = i;
                            ++i;
                        }
                    }
                    auto it = nameMap.find(std::string(subname,dot-subname));
                    if(it!=nameMap.end())
                        idx = it->second;
                }

                if(idx < 0 || idx >= (int)nodeArray.size())
                    return false;

                subname = dot+1;
                if(!subname[0] || nodeArray[idx]->isGroup==0)
                    break;
                if(ViewParams::getMapChildrenPlacement()
                        && nodeArray[idx]->isGroup<0)
                    break;
                idx = -1;
            }
        }

        if(idx<0 || idx>=(int)nodeArray.size())
            return false;
        auto &info = *nodeArray[idx];
        if(!appendPathSafe(path,pcLinkRoot))
            return false;
        if(info.groupIndex>=0 && !getGroupHierarchy(info.groupIndex,path))
            return false;

        info.appendToPath(path);

        if(info.isGroup!=0 && *subname == 0)
            return true;

        if(info.isLinked()) {
            // Here means we are in group mode
            if(info.pcRoot == info.linkInfo->pcLinked->getRoot())
                info.linkInfo->pcLinked->getDetailPath(subname,path,true,det);
            else
                info.linkInfo->getDetail(false,info.nodeType,subname,det,path);
            return true;
        }
        // if not info.isLinked(), then we are in array mode

        if (info.pcRoot)
            appendPathSafe(path, info.pcRoot);
    }
    if(isLinked()) {
        if(nodeType >= 0) {
            if(linkInfo->getDetail(false,nodeType,subname,det,path))
                return true;
        }else {
            appendPath(path,pcLinkedRoot);
            for(const auto &v : subInfo) {
                auto &sub = *v.second;
                if(!sub.isLinked())
                    continue;
                const char *nextsub;
                if(autoSubLink && subInfo.size()==1)
                    nextsub = subname;
                else{
                    if(!boost::algorithm::starts_with(subname,v.first))
                        continue;
                    nextsub = subname+v.first.size();
                    if(*nextsub != '.')
                        continue;
                    ++nextsub;
                }
                if(*nextsub && !sub.subElements.empty()) {
                    auto element = Data::ComplexGeoData::oldElementName(nextsub);
                    if (sub.subElements.find(element)==sub.subElements.end())
                       break;
                }
                appendPath(path,sub.pcNode);
                len = path->getLength();
                if(sub.linkInfo->getDetail(false,SnapshotTransform,nextsub,det,path))
                    return true;
                break;
            }
        }
    }
    path->truncate(len);
    return false;
}

void LinkView::unlink(LinkInfoPtr info) {
    if(!info)
        return;
    if(info == linkOwner) {
        linkOwner->remove(this);
        linkOwner.reset();
    }
    if(info != linkInfo)
        return;
    if(linkInfo) {
        linkInfo->remove(this);
        linkInfo.reset();
    }
    _registerLinkNode(pcLinkRoot);
    pcLinkRoot->resetContext();
    if(pcLinkedRoot) {
        if(nodeArray.empty())
            resetRoot();
        else {
            for(const auto &info : nodeArray) {
                int idx;
                if(info->isLinked() &&
                   (idx=info->pcRoot->findChild(pcLinkedRoot))>=0)
                    info->pcRoot->removeChild(idx);
            }
        }
        pcLinkedRoot.reset();
    }
    subInfo.clear();
    return;
}

QIcon LinkView::getLinkedIcon(QPixmap px) const {
    auto link = linkInfo;
    if(autoSubLink && subInfo.size()==1)
        link = subInfo.begin()->second->linkInfo;
    if(!link || !link->isLinked())
        return QIcon();
    return link->getIcon(px);
}

bool LinkView::hasSubs() const {
    return isLinked() && !subInfo.empty();
}

///////////////////////////////////////////////////////////////////////////////////

PROPERTY_SOURCE(Gui::ViewProviderLink, Gui::ViewProviderDocumentObject)

static const char *_LinkIcon = "Link";
// static const char *_LinkArrayIcon = "LinkArray";
static const char *_LinkGroupIcon = "LinkGroup";
static const char *_LinkElementIcon = "LinkElement";

ViewProviderLink::ViewProviderLink()
    :linkType(LinkTypeNone),hasSubName(false),hasSubElement(false)
    ,useCenterballDragger(false),childVp(nullptr),overlayCacheKey(0)
{
    sPixmap = _LinkIcon;

    ADD_PROPERTY_TYPE(OverrideMaterial, (false), " Link", App::Prop_None, "Override linked object's material");

    App::Material mat(App::Material::DEFAULT);
    mat.diffuseColor.setPackedValue(ViewParams::getDefaultLinkColor());
    ADD_PROPERTY_TYPE(ShapeMaterial, (mat), " Link", App::Prop_None, 0);
    ShapeMaterial.setStatus(App::Property::MaterialEdit, true);

    ADD_PROPERTY_TYPE(DrawStyle,((long int)0), " Link", App::Prop_None, "");
    static const char* DrawStyleEnums[]= {"None","Solid","Dashed","Dotted","Dashdot",nullptr};
    DrawStyle.setEnums(DrawStyleEnums);

    int lwidth = ViewParams::getDefaultShapeLineWidth();
    ADD_PROPERTY_TYPE(LineWidth,(lwidth), " Link", App::Prop_None, "");

    static App::PropertyFloatConstraint::Constraints sizeRange = {1.0,64.0,1.0};
    LineWidth.setConstraints(&sizeRange);

    ADD_PROPERTY_TYPE(PointSize,(lwidth), " Link", App::Prop_None, "");
    PointSize.setConstraints(&sizeRange);

    ADD_PROPERTY(MaterialList,());
    MaterialList.setStatus(App::Property::NoMaterialListEdit, true);

    ADD_PROPERTY(OverrideMaterialList,());
    ADD_PROPERTY(OverrideColorList,());

    ADD_PROPERTY(ChildViewProvider, (""));
    ChildViewProvider.setStatus(App::Property::Hidden,true);

    DisplayMode.setStatus(App::Property::Status::Hidden, true);

    linkView = new LinkView;
}

ViewProviderLink::~ViewProviderLink()
{
    linkView->setInvalid();
}

App::DocumentObject *ViewProviderLink::linkedObjectByNode(SoNode *node)
{
    auto it = _LinkNodeMap.find(node);
    if (it == _LinkNodeMap.end()) {
        if (auto doc = Application::Instance->activeDocument()) {
            auto vp = doc->getViewProvider(node);
            if (vp)
                return vp->getObject();
        }
        return nullptr;
    }
    Document *gdoc = Application::Instance->getDocument(it->second.first);
    // make sure the object still exists
    if (gdoc && gdoc->getViewProvider(it->second.second))
        return it->second.second;
    return nullptr;
}

bool ViewProviderLink::isSelectable() const {
    return !pcDragger && Selectable.getValue();
}

void ViewProviderLink::attach(App::DocumentObject *pcObj) {
    SoNode *node = linkView->getLinkRoot();

    // _registerLinkNode(node, pcObj);

    addDisplayMaskMode(node,"Link");
    if(childVp) {
        childVpLink = LinkInfo::get(childVp,nullptr);
        node = childVpLink->getSnapshot(LinkView::SnapshotTransform);
    }
    addDisplayMaskMode(node,"ChildView");

    if(this->pcModeSwitch->isOfType(SoFCSwitch::getClassTypeId())) {
        auto group = new SoGroup();
        group->addChild(linkView->getLinkRoot());
        if(childVp)
            group->addChild(node);
        addDisplayMaskMode(group,"ComboView");
        static_cast<SoFCSwitch*>(this->pcModeSwitch)->defaultChild = 2;
    }

    setDisplayMaskMode("Link");
    inherited::attach(pcObj);
    checkIcon();
    if(pcObj->isDerivedFrom(App::LinkElement::getClassTypeId()))
        hide();
    linkView->setOwner(this);
}

void ViewProviderLink::setDisplayMode(const char* ModeName)
{
    if (boost::equals(ModeName, "Link")
            || boost::equals(ModeName, "ChildView")
            || boost::equals(ModeName, "ComboView"))
        setDisplayMaskMode(ModeName);
}

void ViewProviderLink::reattach(App::DocumentObject *obj) {
    linkView->setOwner(this);
    inherited::reattach(obj);
    if(childVp) 
        childVp->reattach(obj);
}

std::vector<std::string> ViewProviderLink::getDisplayModes() const
{
    std::vector<std::string> StrList = inherited::getDisplayModes();
    StrList.emplace_back("Link");
    StrList.emplace_back("ChildView");
    StrList.emplace_back("ComboView");
    return StrList;
}

QIcon ViewProviderLink::getIcon() const {
    auto ext = getLinkExtension();
    if(ext) {
        auto link = ext->getLinkedObjectValue();
        if(link && link!=getObject()) {
            QPixmap overlay = getOverlayPixmap();
            overlayCacheKey = overlay.cacheKey();
            QIcon icon = linkView->getLinkedIcon(overlay);
            if(!icon.isNull())
                return icon;
        }
    }
    overlayCacheKey = 0;
    return ViewProviderDocumentObject::getIcon();
}

QPixmap ViewProviderLink::getOverlayPixmap() const {
    auto ext = getLinkExtension();
    QSizeF size(64, 64);
    if(ext && ext->getLinkedObjectProperty() && ext->_getElementCountValue())
        return BitmapFactory().pixmapFromSvg("LinkArrayOverlay", size);
    else if(hasSubElement)
        return BitmapFactory().pixmapFromSvg("LinkSubElement", size);
    else if(hasSubName)
        return BitmapFactory().pixmapFromSvg("LinkSubOverlay", size);
    else
        return BitmapFactory().pixmapFromSvg("LinkOverlay", size);
}

void ViewProviderLink::onChanged(const App::Property* prop) {
    Gui::ColorUpdater colorUpdater;

    if(prop==&ChildViewProvider) {
        childVp = freecad_dynamic_cast<ViewProviderDocumentObject>(ChildViewProvider.getObject().get());
        if(childVp && getObject()) {
            if(strcmp(childVp->getTypeId().getName(),getObject()->getViewProviderName())!=0
                    && !childVp->allowOverride(*getObject()))
            {
                FC_ERR("Child view provider type '" << childVp->getTypeId().getName()
                        << "' does not support " << getObject()->getFullName());
            } else {
                childVp->setPropertyPrefix("ChildViewProvider.");
                childVp->Visibility.setValue(getObject()->Visibility.getValue());
                childVp->attach(getObject());
                if (!isRestoring())
                    childVp->updateView();
                else
                    childVp->setStatus(ViewStatus::isRestoring, true);
                childVp->setActiveMode();
                if(pcModeSwitch->getNumChildren()>1){
                    childVpLink = LinkInfo::get(childVp,0);
                    auto node = childVpLink->getSnapshot(LinkView::SnapshotTransform);
                    if (node) {
                        pcModeSwitch->replaceChild(1,node);
                        if(pcModeSwitch->getNumChildren() > 2
                                && pcModeSwitch->getChild(2)->isOfType(SoGroup::getClassTypeId()))
                        {
                            auto group = static_cast<SoGroup*>(pcModeSwitch->getChild(2));
                            if(group->getNumChildren() > 1)
                                group->replaceChild(1,node);
                            else
                                group->addChild(node);
                        }
                    }
                }
            }
        }
    }else if(!isRestoring()) {
        if (prop == &OverrideMaterial || prop == &ShapeMaterial ||
            prop == &MaterialList || prop == &OverrideMaterialList)
        {
            applyMaterial();
            Gui::ColorUpdater::addObject(getObject());
        }else if(prop == &OverrideColorList) {
            applyColors();
            Gui::ColorUpdater::addObject(getObject());
        }else if(prop==&DrawStyle || prop==&PointSize || prop==&LineWidth) {
            if(!DrawStyle.getValue())
                linkView->setDrawStyle(0);
            else
                linkView->setDrawStyle(DrawStyle.getValue(),LineWidth.getValue(),PointSize.getValue());
        }
    }

    inherited::onChanged(prop);
}

bool ViewProviderLink::setLinkType(App::LinkBaseExtension *ext) {
    auto propLink = ext->getLinkedObjectProperty();
    if(!propLink)
        return false;
    LinkType type;
    if(hasSubName)
        type = LinkTypeSubs;
    else
        type = LinkTypeNormal;
    if(linkType != type)
        linkType = type;
    switch(type) {
    case LinkTypeSubs:
        linkView->setNodeType(ext->linkTransform()?LinkView::SnapshotContainer:
                LinkView::SnapshotContainerTransform);
        break;
    case LinkTypeNormal:
        linkView->setNodeType(ext->linkTransform()?LinkView::SnapshotVisible:
                LinkView::SnapshotTransform);
        break;
    default:
        break;
    }
    return true;
}

App::LinkBaseExtension *ViewProviderLink::getLinkExtension() {
    if(!pcObject || !pcObject->getNameInDocument())
        return 0;

    auto ext = pcObject->getExtensionByType<App::LinkBaseExtension>(true);
    if (ext && !connNewElement.connected()) {
        connNewElement = ext->signalNewLinkElements.connect(
            [this, ext](App::DocumentObject &parent,
                        int i, int end, std::vector<App::DocumentObject*> *elements)
            {
                if (!ext->getAutoPlacementValue())
                    return;
                auto placementProp = ext->getPlacementListProperty();
                if(placementProp && placementProp->getSize() > i && elements) {
                    for (; i < end; ++i) {
                        auto element = Base::freecad_dynamic_cast<App::LinkElement>((*elements)[i]);
                        if (element)
                            element->Placement.setValue(placementProp->getValues()[i]);
                    }
                    return;
                }
                auto parentVp = Application::Instance->getViewProvider(&parent);
                if (!parentVp)
                    return;
                auto bboxParent = parentVp->getBoundingBox();
                bool bboxParentValid = bboxParent.IsValid();
                auto bboxChild = parentVp->getBoundingBox("0.");
                bool bboxChildValid = bboxChild.IsValid();
                std::vector<Base::Placement> placements;
                if (!elements) {
                    placements = ext->getPlacementListValue();
                    placements.resize(end);
                }
                for (; i<end; ++i) {
                    if (i == 0) continue;
                    Base::Placement pla(Base::Vector3d(i%10,(i/10)%10,i/100),Base::Rotation());
                    Base::Vector3d pos; 
                    if (bboxParentValid) {
                        pos.x = bboxParent.MaxX;
                        pos.y = bboxParent.MinY;
                        pos.z = bboxParent.MinZ;
                    }
                    if (bboxChildValid) {
                        int offset = i - (int)ext->getElementListValue().size();
                        pos.x += 1.5 * bboxChild.LengthX() * ((offset % 10) + 0.5);
                        pos.y += 1.5 * bboxChild.LengthY() * (offset/10 % 10);
                        pos.z += 1.5 * bboxChild.LengthZ() * (offset/100);
                        pla.setPosition(pos);
                    }
                    if (elements) {
                        auto element = Base::freecad_dynamic_cast<App::LinkElement>((*elements)[i]);
                        if (element)
                            element->Placement.setValue(pla);
                    } else
                        placements[i] = pla;
                }
                if (placementProp)
                    placementProp->setValues(std::move(placements));
            });
    }
    return ext;
}

const App::LinkBaseExtension *ViewProviderLink::getLinkExtension() const{
    if(!pcObject || !pcObject->getNameInDocument())
        return nullptr;
    return const_cast<App::DocumentObject*>(pcObject)->getExtensionByType<App::LinkBaseExtension>(true);
}

void ViewProviderLink::updateData(const App::Property *prop) {
    if(childVp)
        childVp->updateData(prop);
    if(!isRestoring() && !pcObject->isRestoring()) {
        auto ext = getLinkExtension();
        if(ext) {
            updateDataPrivate(getLinkExtension(),prop);
            if (prop == ext->getLinkCopyOnChangeProperty()
                    || prop == ext->getLinkCopyOnChangeTouchedProperty()
                    || prop == ext->getLinkCopyOnChangeSourceProperty())
                signalChangeIcon();
        }
    }
    return inherited::updateData(prop);
}

static inline bool canScale(const Base::Vector3d &v) {
    return fabs(v.x)>1e-7 && fabs(v.y)>1e-7 && fabs(v.z)>1e-7;
}

void ViewProviderLink::updateDataPrivate(App::LinkBaseExtension *ext, const App::Property *prop) {
    if(!prop)
        return;
    if(prop == &ext->_ChildCache) {
        updateElementList(ext);
    } else if(prop == &ext->_LinkTouched) {
        if(linkView->hasSubs())
            linkView->updateLink();
        applyColors();
        checkIcon(ext);
    }else if(prop==ext->getColoredElementsProperty()) {
        if(!prop->testStatus(App::Property::User3))
            applyColors();
    }else if(prop==ext->getScaleProperty() || prop==ext->getScaleVectorProperty()) {
        if(!prop->testStatus(App::Property::User3)) {
            const auto &v = ext->getScaleVector();
            if(canScale(v))
                pcTransform->scaleFactor.setValue(v.x,v.y,v.z);
            SbMatrix matrix = convert(ext->getTransform(false));
            linkView->renderDoubleSide(matrix.det3() < 1e-7);
        }
    }else if(prop == ext->getMatrixProperty()) {
        if(!prop->testStatus(App::Property::User3)) {
            if (!pcMatrixTransform) {
                pcMatrixTransform = new SoMatrixTransform;
                int idx = pcRoot->findChild(pcTransform);
                pcRoot->insertChild(pcMatrixTransform, idx+1);
            }
            pcMatrixTransform->matrix = convert(ext->getMatrixValue());
            SbMatrix matrix = convert(ext->getTransform(false));
            linkView->renderDoubleSide(matrix.det3() < 1e-7);
        }
    }else if(prop == ext->getPlacementProperty() || prop == ext->getLinkPlacementProperty()) {
        auto propLinkPlacement = ext->getLinkPlacementProperty();
        if(!propLinkPlacement || propLinkPlacement == prop) {
            const auto &pla = static_cast<const App::PropertyPlacement*>(prop)->getValue();
            ViewProviderGeometryObject::updateTransform(pla, pcTransform);
            const auto &v = ext->getScaleVector();
            if(canScale(v))
                pcTransform->scaleFactor.setValue(v.x,v.y,v.z);
            SbMatrix matrix = convert(ext->getTransform(false));
            linkView->renderDoubleSide(matrix.det3() < 1e-7);
        }
    }else if(prop == ext->getLinkCopyOnChangeGroupProperty()) {
        if (auto group = ext->getLinkCopyOnChangeGroupValue()) {
            auto vp = Base::freecad_dynamic_cast<ViewProviderDocumentObject>(
                    Application::Instance->getViewProvider(group));
            if (vp) {
                vp->hide();
                vp->ShowInTree.setValue(false);
            }
        }
    }else if(prop == ext->getLinkedObjectProperty()) {
        if(!prop->testStatus(App::Property::User3)) {
            std::vector<std::string> subs;
            const char *subname = ext->getSubName();
            std::string sub;
            if(subname)
                sub = subname;
            hasSubElement = false;
            for(const auto &s : ext->getSubElements()) {
                if(s.empty()) continue;
                hasSubElement = true;
                subs.push_back(sub+s);
            }

            if(subs.empty() && !sub.empty())
                subs.push_back(sub);

            hasSubName = !subs.empty();
            setLinkType(ext);

            auto obj = ext->getLinkedObjectValue();
            linkView->setLink(obj,subs);

            if(ext->_getShowElementValue())
                updateElementList(ext);
            else
                updateDataPrivate(ext,ext->_getElementCountProperty());

            // applyColors();
            signalChangeIcon();
        }
    }else if(prop == ext->getLinkTransformProperty()) {
        setLinkType(ext);
        applyColors();
    }else if(prop==ext->_getElementCountProperty()) {
        if(!ext->_getShowElementValue()) {
            linkView->setSize(ext->_getElementCountValue());
            updateDataPrivate(ext,ext->getVisibilityListProperty());
            updateDataPrivate(ext,ext->getPlacementListProperty());
            applyMaterial();
            applyColors();
        } else if (!ext->_getElementCountValue() && ext->_getElementListValue().empty())
            linkView->setSize(0);
    }else if(prop == ext->_getShowElementProperty()) {
        if(!ext->_getShowElementValue()) {

            auto linked = freecad_dynamic_cast<ViewProviderDocumentObject>(getLinkedView(true,ext));
            if(linked && linked->getDocument()==getDocument())
                linked->hide();

            const auto &elements = ext->_getElementListValue();
            // elements is about to be collapsed, preserve the materials
            if(!elements.empty()) {
                std::vector<App::Material> materials;
                boost::dynamic_bitset<> overrideMaterials;
                overrideMaterials.resize(elements.size(),false);
                bool overrideMaterial = false;
                bool hasMaterial = false;
                materials.reserve(elements.size());
                for(size_t i=0;i<elements.size();++i) {
                    auto element = freecad_dynamic_cast<App::LinkElement>(elements[i]);
                    if(!element) continue;
                    auto vp = freecad_dynamic_cast<ViewProviderLink>(
                            Application::Instance->getViewProvider(element));
                    if(!vp) continue;
                    overrideMaterial = overrideMaterial || vp->OverrideMaterial.getValue();
                    hasMaterial = overrideMaterial || hasMaterial
                        || vp->ShapeMaterial.getValue()!=ShapeMaterial.getValue();
                    materials.push_back(vp->ShapeMaterial.getValue());
                    overrideMaterials[i] = vp->OverrideMaterial.getValue();
                }
                if(!overrideMaterial)
                    overrideMaterials.clear();
                OverrideMaterialList.setStatus(App::Property::User3,true);
                OverrideMaterialList.setValue(overrideMaterials);
                OverrideMaterialList.setStatus(App::Property::User3,false);
                if(!hasMaterial)
                    materials.clear();
                MaterialList.setStatus(App::Property::User3,true);
                MaterialList.setValue(materials);
                MaterialList.setStatus(App::Property::User3,false);

                linkView->setSize(ext->_getElementCountValue());
                updateDataPrivate(ext,ext->getVisibilityListProperty());
                applyMaterial();
                applyColors();
            }
        }
    }else if(prop==ext->getScaleListProperty()
            || prop==ext->getPlacementListProperty()
            || prop==ext->getMatrixListProperty()) {

        if(!prop->testStatus(App::Property::User3)
                && linkView->getSize()
                && !ext->_getShowElementValue())
        {
            auto propPlacements = ext->getPlacementListProperty();
            auto propScales = ext->getScaleListProperty();
            auto propMatrices = ext->getMatrixListProperty();
            if(linkView->getSize()) {
                const auto &touched =
                    prop==propScales ? propScales->getTouchList()
                                     : (prop==propPlacements ? propPlacements->getTouchList()
                                                             : propMatrices->getTouchList());
                if(touched.empty()) {
                    for(int i=0;i<linkView->getSize();++i) {
                        Base::Matrix4D mat;
                        if(propPlacements && propPlacements->getSize()>i)
                            mat = (*propPlacements)[i].toMatrix();
                        if(propScales && propScales->getSize()>i && canScale((*propScales)[i])) {
                            Base::Matrix4D s;
                            s.scale((*propScales)[i]);
                            mat *= s;
                        }
                        if(propMatrices && propMatrices->getSize()>i)
                            mat *= (*propMatrices)[i];
                        linkView->setTransform(i,mat);
                    }
                }else{
                    for(int i : touched) {
                        if(i<0 || i>=linkView->getSize())
                            continue;
                        Base::Matrix4D mat;
                        if(propPlacements && propPlacements->getSize()>i)
                            mat = (*propPlacements)[i].toMatrix();
                        if(propScales && propScales->getSize()>i && canScale((*propScales)[i])) {
                            Base::Matrix4D s;
                            s.scale((*propScales)[i]);
                            mat *= s;
                        }
                        if(propMatrices && propMatrices->getSize()>i)
                            mat *= (*propMatrices)[i];
                        linkView->setTransform(i,mat);
                    }
                }
            }
        }
    }else if(prop == ext->getVisibilityListProperty()) {
        const auto &vis = ext->getVisibilityListValue();
        for(size_t i=0;i<(size_t)linkView->getSize();++i) {
            if(vis.size()>i)
                linkView->setElementVisible(i,vis[i]);
            else
                linkView->setElementVisible(i,true);
        }
    }else if(prop == ext->_getElementListProperty()) {
        updateElementList(ext);
    }else if(prop == ext->getSyncGroupVisibilityProperty()) {
        updateElementList(ext);
    }
}

void ViewProviderLink::updateElementList(App::LinkBaseExtension *ext) {
    const auto &elements = ext->_getElementListValue();
    if(OverrideMaterialList.getSize() || MaterialList.getSize()) {
        int i=-1;
        for(auto obj : elements) {
            ++i;
            auto vp = freecad_dynamic_cast<ViewProviderLink>(
                    Application::Instance->getViewProvider(obj));
            if(!vp) continue;
            if(OverrideMaterialList.getSize()>i)
                vp->OverrideMaterial.setValue(OverrideMaterialList[i]);
            if(MaterialList.getSize()>i)
                vp->ShapeMaterial.setValue(MaterialList[i]);
        }
        OverrideMaterialList.setSize(0);
        MaterialList.setSize(0);
    }
    if(ext->getSyncGroupVisibilityValue() && ext->linkedPlainGroup())
        linkView->setChildren(elements);
    else if (!ext->_getElementCountValue() || ext->_getShowElementValue())
        linkView->setChildren(elements, ext->getVisibilityListValue(), LinkView::SnapshotVisible);
    applyColors();
}

void ViewProviderLink::checkIcon(const App::LinkBaseExtension *ext) {
    if(!ext) {
        ext = getLinkExtension();
        if(!ext)
            return;
    }
    const char *icon;
    auto element = freecad_dynamic_cast<App::LinkElement>(getObject());
    if(element)
        icon = _LinkElementIcon;
    else if(!ext->getLinkedObjectProperty() && ext->getElementListProperty())
        icon = _LinkGroupIcon;
    // else if(ext->_getElementCountValue())
    //     icon = _LinkArrayIcon;
    else
        icon = _LinkIcon;
    qint64 cacheKey = 0;
    if(getObject()->getLinkedObject(false)!=getObject())
        cacheKey = getOverlayPixmap().cacheKey();
    if(icon!=sPixmap || cacheKey!=overlayCacheKey) {
        sPixmap = icon;
        signalChangeIcon();
    }
}

void ViewProviderLink::applyMaterial() {
    if(OverrideMaterial.getValue())
        linkView->setMaterial(-1,&ShapeMaterial.getValue());
    else {
        for(int i=0;i<linkView->getSize();++i) {
            if(MaterialList.getSize()>i &&
               OverrideMaterialList.getSize()>i && OverrideMaterialList[i])
                linkView->setMaterial(i,&MaterialList[i]);
            else
                linkView->setMaterial(i,nullptr);
        }
        linkView->setMaterial(-1,nullptr);
    }
}

void ViewProviderLink::finishRestoring() {
    FC_TRACE("finish restoring");

    auto ext = getLinkExtension();
    if(!ext)
        return;
    linkView->setDrawStyle(DrawStyle.getValue(),LineWidth.getValue(),PointSize.getValue());
    updateDataPrivate(ext,ext->getLinkedObjectProperty());
    if(ext->getLinkPlacementProperty())
        updateDataPrivate(ext,ext->getLinkPlacementProperty());
    else
        updateDataPrivate(ext,ext->getPlacementProperty());
    updateDataPrivate(ext,ext->_getElementCountProperty());
    if(ext->getPlacementListProperty())
        updateDataPrivate(ext,ext->getPlacementListProperty());
    else
        updateDataPrivate(ext,ext->getScaleListProperty());
    if (ext->getMatrixProperty()) {
        static Base::Matrix4D identity;
        if (ext->getMatrixValue() != identity)
            updateDataPrivate(ext,ext->getMatrixProperty());
    }
    updateDataPrivate(ext,ext->_getElementListProperty());
    applyMaterial();
    applyColors();

    if(childVp) {
        childVp->setStatus(ViewStatus::isRestoring, false);
        childVp->finishRestoring();
    }

    inherited::finishRestoring();
}

bool ViewProviderLink::hasElements(const App::LinkBaseExtension *ext) const {
    if(!ext) {
        ext = getLinkExtension();
        if(!ext)
            return false;
    }
    const auto &elements = ext->getElementListValue();
    return !elements.empty() && (int)elements.size()==ext->_getElementCountValue();
}

bool ViewProviderLink::isGroup(const App::LinkBaseExtension *ext, bool plainGroup) const {
    if(!ext) {
        ext = getLinkExtension();
        if(!ext)
            return false;
    }
    return (plainGroup && ext->linkedPlainGroup())
        || (ext->getElementListProperty() && !ext->getLinkedObjectProperty());
}

ViewProvider *ViewProviderLink::getLinkedView(
        bool real,const App::LinkBaseExtension *ext) const
{
    if(!ext)
        ext = getLinkExtension();
    auto obj = ext&&real?ext->getTrueLinkedObject(true):
        getObject()->getLinkedObject(true);
    if(obj && obj!=getObject())
        return Application::Instance->getViewProvider(obj);
    return nullptr;
}

std::vector<App::DocumentObject*> ViewProviderLink::claimChildren() const {
    auto ext = getLinkExtension();
    std::vector<App::DocumentObject*> ret;

    if(ext && !ext->_getShowElementValue() && ext->_getElementCountValue()) {
        // in array mode without element objects, we'd better not show the
        // linked object's children to avoid inconsistent behavior on selection.
        // We claim the linked object instead
        if(ext) {
            auto obj = ext->getLinkedObjectValue();
            if(obj) ret.push_back(obj);
        }
    } else if(hasElements(ext) || isGroup(ext)) {
        ret = ext->getElementListValue();
        if (ext->_getElementCountValue()
                && ext->getLinkClaimChildValue()
                && ext->getLinkedObjectValue())
            ret.insert(ret.begin(), ext->getLinkedObjectValue());
    } else if(!hasSubName) {
        auto linked = getLinkedView(true);
        if(linked) {
            ret = linked->claimChildren();
            if (ext->getLinkClaimChildValue() && ext->getLinkedObjectValue())
                ret.insert(ret.begin(), ext->getLinkedObjectValue());
        }
    }
    if (ext && ext->getLinkCopyOnChangeGroupValue())
        ret.insert(ret.begin(), ext->getLinkCopyOnChangeGroupValue());
    return ret;
}

bool ViewProviderLink::canDragObject(App::DocumentObject* obj) const {
    auto ext = getLinkExtension();
    if(isGroup(ext))
        return true;
    if(hasElements(ext))
        return false;
    auto linked = getLinkedView(false,ext);
    if(linked)
        return linked->canDragObject(obj);
    return false;
}

bool ViewProviderLink::canDragObjects() const {
    auto ext = getLinkExtension();
    if(isGroup(ext))
        return true;
    if(hasElements(ext))
        return false;
    auto linked = getLinkedView(false,ext);
    if(linked)
        return linked->canDragObjects();
    return false;
}

void ViewProviderLink::dragObject(App::DocumentObject* obj) {
    auto ext = getLinkExtension();
    if(isGroup(ext)) {
        const auto &objs = ext->getElementListValue();
        for(size_t i=0;i<objs.size();++i) {
            if(obj==objs[i]) {
                ext->setLink(i,nullptr);
                break;
            }
        }
        return;
    }
    if(hasElements(ext))
        return;
    auto linked = getLinkedView(false);
    if(linked)
        linked->dragObject(obj);
}

bool ViewProviderLink::canDropObjects() const {
    auto ext = getLinkExtension();
    if(isGroup(ext))
        return true;
    if(hasElements(ext))
        return false;
    if(hasSubElement)
        return true;
    else if(hasSubName)
        return false;
    auto linked = getLinkedView(false,ext);
    if(linked)
        return linked->canDropObjects();
    return true;
}

bool ViewProviderLink::canDropObjectEx(App::DocumentObject *obj,
        App::DocumentObject *owner, const char *subname,
        const std::vector<std::string> &subElements) const
{
    if(pcObject == obj || pcObject == owner)
        return false;
    auto ext = getLinkExtension();
    if(isGroup(ext))
        return true;
    if(!ext || !ext->getLinkedObjectProperty() || hasElements(ext))
        return false;
    if(!hasSubName && linkView->isLinked()) {
        auto linked = getLinkedView(false,ext);
        if(linked) {
            auto linkedVdp = freecad_dynamic_cast<ViewProviderDocumentObject>(linked);
            if(linkedVdp) {
                if(linkedVdp->getObject()==obj || linkedVdp->getObject()==owner)
                    return false;
            }
            return linked->canDropObjectEx(obj,owner,subname,subElements);
        }
    }
    if(obj->getDocument() != getObject()->getDocument() &&
       !freecad_dynamic_cast<App::PropertyXLink>(ext->getLinkedObjectProperty()))
        return false;

    return true;
}

std::string ViewProviderLink::dropObjectEx(App::DocumentObject* obj,
    App::DocumentObject *owner, const char *subname,
    const std::vector<std::string> &subElements)
{
    auto ext = getLinkExtension();
    if (!ext)
        return std::string();

    if(isGroup(ext)) {
        size_t size = ext->getElementListValue().size();
        ext->setLink(size,obj);
        return std::to_string(size)+".";
    }

    if(!ext->getLinkedObjectProperty() || hasElements(ext))
        return std::string();

    if(!hasSubName) {
        auto linked = getLinkedView(false,ext);
        if(linked)
            return linked->dropObjectEx(obj,owner,subname,subElements);
    }
    if(owner) {
        if(!ext->getSubElements().empty())
            ext->setLink(-1,owner,subname,subElements);
        else
            ext->setLink(-1,owner,subname);
    } else if(!ext->getSubElements().empty())
        ext->setLink(-1,obj,nullptr,subElements);
    else
        ext->setLink(-1,obj,nullptr);
    return std::string(".");
}

bool ViewProviderLink::canDragAndDropObject(App::DocumentObject* obj) const {
    auto ext = getLinkExtension();
    if(!ext)
        return true;
    if(isGroup(ext)) {
        return ext->getLinkModeValue()<App::LinkBaseExtension::LinkModeAutoLink &&
               obj->getDocument()==getObject()->getDocument();
    }
    if(!ext->getLinkedObjectProperty() || hasElements(ext))
        return false;
    if(!hasSubName) {
        auto linked = getLinkedView(false,ext);
        if(linked)
            return linked->canDragAndDropObject(obj);
    }
    return false;
}

bool ViewProviderLink::getElementPicked(const SoPickedPoint *pp, std::string &subname) const {
    auto ext = getLinkExtension();
    if(!ext)
        return false;
    if(childVpLink && childVp) {
        auto path = pp->getPath();
        int idx = path->findNode(childVpLink->getSnapshot(LinkView::SnapshotTransform));
        if(idx>=0)
            return childVp->getElementPicked(pp,subname);
    }
    bool ret = linkView->linkGetElementPicked(pp,subname);
    if(!ret)
        return ret;
    if(isGroup(ext,true)) {
        const char *sub = nullptr;
        int idx = App::LinkBaseExtension::getArrayIndex(subname.c_str(),&sub);
        if(idx>=0 ) {
            --sub;
            assert(*sub == '.');
            const auto &elements = ext->_getElementListValue();
            subname.replace(0,sub-subname.c_str(),elements[idx]->getNameInDocument());
        }
    }
    return ret;
}

bool ViewProviderLink::getDetailPath(
        const char *subname, SoFullPath *pPath, bool append, SoDetail *&det) const
{
    auto ext = getLinkExtension();
    if(!ext)
        return false;

    auto len = pPath->getLength();
    if(append) {
        appendPath(pPath,pcRoot);
        appendPath(pPath,pcModeSwitch);
    }
    if(childVpLink
            && (Data::ComplexGeoData::isElementName(subname)
                || !subname || !subname[0])) {
        if(childVpLink->getDetail(false,LinkView::SnapshotTransform,subname,det,pPath))
            return true;
        pPath->truncate(len);
        return false;
    }

    std::string _subname;
    if(subname && subname[0]) {
        if (auto linked = ext->getLinkedObjectValue()) {
            if (const char *dot = strchr(subname,'.')) {
                if(subname[0]=='$') {
                    CharRange sub(subname+1, dot);
                    if (!boost::equals(sub, linked->Label.getValue()))
                        dot = nullptr;
                } else {
                    CharRange sub(subname, dot);
                    if (!boost::equals(sub, linked->getNameInDocument()))
                        dot = nullptr;
                }
                if (dot && linked->getSubObject(dot+1))
                    subname = dot+1;
            }
        }

        if (isGroup(ext,true) || hasElements(ext) || ext->getElementCountValue()) {
            int index = ext->getElementIndex(subname,&subname);
            if(index>=0) {
                _subname = std::to_string(index)+'.'+subname;
                subname = _subname.c_str();
            }
        }
    }
    if(linkView->linkGetDetailPath(subname,pPath,det))
        return true;
    pPath->truncate(len);
    return false;
}

bool ViewProviderLink::onDelete(const std::vector<std::string> &subElements) {
    auto element = freecad_dynamic_cast<App::LinkElement>(getObject());
    if (element && !element->canDelete())
        return false;
    auto ext = getLinkExtension();
    if (ext->isLinkMutated()) {
        auto linked = ext->getLinkedObjectValue();
        auto doc = ext->getContainer()->getDocument();
        if (linked->getDocument() == doc) {
            std::deque<std::string> objs;
            for (auto obj : ext->getOnChangeCopyObjects(nullptr, linked)) {
                if (obj->getDocument() == doc) {
                    // getOnChangeCopyObjects() returns object in depending
                    // order. So we delete it in reverse to avoid error
                    // reported by some parent object failing to find child
                    objs.emplace_front(obj->getNameInDocument());
                }
            }
            for (const auto &name : objs)
                doc->removeObject(name.c_str());
        }
        return true;
    }
    if (auto linkedView = getLinkedViewProvider()) {
        return linkedView->onDelete(subElements);
    }
    return true;
}

bool ViewProviderLink::canDelete(App::DocumentObject *obj) const {
    auto ext = getLinkExtension();
    if(isGroup(ext) || hasElements(ext) || hasSubElement)
        return true;
    auto linked = getLinkedView(false,ext);
    if(linked)
        return linked->canDelete(obj);
    return false;
}

bool ViewProviderLink::linkEdit(const App::LinkBaseExtension *ext) const {
    if(!ext)
        ext = getLinkExtension();
    if(!ext ||
       (!ext->_getShowElementValue() && ext->_getElementCountValue()) ||
       hasElements(ext) ||
       isGroup(ext) ||
       hasSubName)
    {
        return false;
    }
    return linkView->isLinked();
}

bool ViewProviderLink::doubleClicked() {
    if(linkEdit())
        return linkView->getLinkedView()->doubleClicked();
    return getDocument()->setEdit(this,ViewProvider::Transform);
}

void ViewProviderLink::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    auto ext = getLinkExtension();
    if (!ext)
        return;

    _setupContextMenu(ext, menu, receiver, member);
    if (linkingMenu)
        return;

    Gui::ActionFunction* func = nullptr;
    if (ext->isLinkedToConfigurableObject()) {
        QAction *act = menu->addAction(
                QObject::tr("Setup configurable object"));
        act->setToolTip(QObject::tr(
                    "Select which object to copy or exclude when configuration changes."
                    "All external linked object are excluded by default."));
        act->setData(-1);
        if (!func) func = new Gui::ActionFunction(menu);
        func->trigger(act, [ext](){
            try {
                std::vector<App::DocumentObject*> excludes;
                auto src = ext->getLinkCopyOnChangeSourceValue();
                if (!src)
                    src = ext->getLinkedObjectValue();
                auto objs = ext->getOnChangeCopyObjects(&excludes, src);
                if (objs.empty())
                    return;
                DlgObjectSelection dlg({src}, excludes, getMainWindow());
                dlg.setMessage(QObject::tr(
                            "Please select which objects to copy when the configuration is changed"));
                QCheckBox *box = new QCheckBox(QObject::tr("Apply to all"), &dlg);
                box->setToolTip(QObject::tr("Apply the setting to all links. Or, uncheck this\n"
                                            "option to apply only to this link."));
                box->setChecked(App::LinkParams::getCopyOnChangeApplyToAll());
                dlg.addCheckBox(box);
                if(dlg.exec()!=QDialog::Accepted)
                    return;
                bool applyAll = box->isChecked();
                App::LinkParams::setCopyOnChangeApplyToAll(applyAll);

                App::Link::OnChangeCopyOptions options;
                if (applyAll)
                    options |= App::Link::OnChangeCopyOptions::ApplyAll;

                App::AutoTransaction guard("Setup configurable object");
                auto sels = dlg.getSelections(DlgObjectSelection::SelectionOptions::InvertSort);
                for (const auto & exclude : excludes) {
                    auto iter = std::lower_bound(sels.begin(), sels.end(), exclude);
                    if (iter == sels.end() || *iter != exclude) {
                        ext->setOnChangeCopyObject(exclude, options);
                    } else
                        sels.erase(iter);
                }

                options |= App::Link::OnChangeCopyOptions::Exclude;
                for (auto obj : sels)
                    ext->setOnChangeCopyObject(obj, options);
                if (!applyAll)
                    ext->monitorOnChangeCopyObjects(ext->getOnChangeCopyObjects());
                else {
                    std::set<App::LinkBaseExtension*> exts;
                    for (auto o : App::Document::getDependencyList(objs)) {
                        if (auto ext = o->getExtensionByType<App::LinkBaseExtension>(true))
                            exts.insert(ext);
                    }
                    for (auto ext : exts)
                        ext->monitorOnChangeCopyObjects(ext->getOnChangeCopyObjects());
                }
                Command::updateActive();
            } catch (Base::Exception &e) {
                e.ReportException();
            }
        });

        if (ext->getLinkCopyOnChangeValue() == 0) {
            auto submenu = menu->addMenu(QObject::tr("Copy on change"));
            auto act = submenu->addAction(QObject::tr("Enable"));
            act->setToolTip(QObject::tr(
                        "Enable auto copy of linked object when its configuration is changed"));
            act->setData(-1);
            if (!func) func = new Gui::ActionFunction(menu);
            func->trigger(act, [ext](){
                try {
                    App::AutoTransaction guard("Enable Link copy on change");
                    ext->getLinkCopyOnChangeProperty()->setValue(1);
                    Command::updateActive();
                } catch (Base::Exception &e) {
                    e.ReportException();
                }
            });
            act = submenu->addAction(QObject::tr("Tracking"));
            act->setToolTip(QObject::tr(
                        "Copy the linked object when its configuration is changed.\n"
                        "Also auto redo the copy if the original linked object is changed.\n"));
            act->setData(-1);
            func->trigger(act, [ext](){
                try {
                    App::AutoTransaction guard("Enable Link tracking");
                    ext->getLinkCopyOnChangeProperty()->setValue(3);
                    Command::updateActive();
                } catch (Base::Exception &e) {
                    e.ReportException();
                }
            });
        }
    }

    if (ext->getLinkCopyOnChangeValue() != 2
            && ext->getLinkCopyOnChangeValue() != 0) {
        QAction *act = menu->addAction(
                QObject::tr("Disable copy on change"));
        act->setData(-1);
        if (!func) func = new Gui::ActionFunction(menu);
        func->trigger(act, [ext](){
            try {
                App::AutoTransaction guard("Disable copy on change");
                ext->getLinkCopyOnChangeProperty()->setValue((long)0);
                Command::updateActive();
            } catch (Base::Exception &e) {
                e.ReportException();
            }
        });
    }

    if (ext->isLinkMutated()) {
        QAction* act = menu->addAction(QObject::tr("Refresh configurable object"));
        act->setToolTip(QObject::tr(
                    "Synchronize the original configurable source object by\n"
                    "creating a new deep copy. Note that any changes made to\n"
                    "the current copy will be lost.\n"));
        act->setData(-1);
        if (!func) func = new Gui::ActionFunction(menu);
        func->trigger(act, [ext](){
            try {
                App::AutoTransaction guard("Link refresh");
                ext->syncCopyOnChange();
                Command::updateActive();
            } catch (Base::Exception &e) {
                e.ReportException();
            }
        });
    }
}

void ViewProviderLink::_setupContextMenu(
        App::LinkBaseExtension *ext, QMenu* menu, QObject* receiver, const char* member)
{
    if(linkEdit(ext)) {
        if (auto linkvp = Base::freecad_dynamic_cast<ViewProviderLink>(linkView->getLinkedView())) {
            Base::StateLocker guard(linkvp->linkingMenu);
            linkvp->setupContextMenu(menu, receiver, member);
        } else
            linkView->getLinkedView()->setupContextMenu(menu,receiver,member);
    }

    if (linkingMenu)
        return;

    if(ext->getLinkedObjectProperty()
            && ext->_getShowElementProperty()
            && ext->_getElementCountValue() > 1)
    {
        auto action = menu->addAction(QObject::tr("Toggle array elements"), [ext] {
            try {
                App::AutoTransaction guard(QT_TRANSLATE_NOOP("Command", "Toggle array elements"));
                ext->getShowElementProperty()->setValue(!ext->getShowElementValue());
                Command::updateActive();
            } catch (Base::Exception &e) {
                e.ReportException();
            }
        });
        action->setToolTip(QObject::tr(
                    "Change whether show each link array element as individual objects"));
    }

    if((ext->getPlacementProperty() && !ext->getPlacementProperty()->isReadOnly())
            || (ext->getLinkPlacementProperty() && !ext->getLinkPlacementProperty()->isReadOnly()))
    {
        bool found = false;
        const auto actions = menu->actions();
        for(auto action : actions) {
            if(action->data().toInt() == ViewProvider::Transform) {
                found = true;
                break;
            }
        }
        if (!found) {
            QAction* act = menu->addAction(QObject::tr("Transform"), receiver, member);
            act->setIcon(Gui::BitmapFactory().pixmap("Std_TransformManip.svg"));
            act->setToolTip(QObject::tr("Transform at the origin of the placement"));
            act->setData(QVariant((int)ViewProvider::Transform));
        }
        found = false;
        for(auto action : menu->actions()) {
            if(action->data().toInt() == ViewProvider::TransformAt) {
                found = true;
                break;
            }
        }
        if (!found) {
            auto act = menu->addAction(QObject::tr("Transform at"), receiver, member);
            act->setToolTip(QObject::tr("Transform at the center of the shape"));
            act->setData(QVariant((int)ViewProvider::TransformAt));
        }
    }

    if(ext->getColoredElementsProperty()) {
        bool found = false;
        const auto actions = menu->actions();
        for(auto action : actions) {
            if(action->data().toInt() == ViewProvider::Color) {
                action->setText(QObject::tr("Override colors..."));
                found = true;
                break;
            }
        }
        if(!found) {
            QAction* act = menu->addAction(QObject::tr("Override colors..."), receiver, member);
            act->setData(QVariant((int)ViewProvider::Color));
        }
    }
}

bool ViewProviderLink::initDraggingPlacement(int mode) {
    Base::PyGILStateLocker lock;
    try {
        auto* proxy = getPropertyByName("Proxy");
        if (proxy && proxy->getTypeId() == App::PropertyPythonObject::getClassTypeId()) {
            Py::Object feature = static_cast<App::PropertyPythonObject*>(proxy)->getValue();
            const char *fname = "initDraggingPlacement";
            if (feature.hasAttr(fname)) {
                Py::Callable method(feature.getAttr(fname));
                Py::Tuple arg;
                Py::Object ret(method.apply(arg));
                if(ret.isTuple()) {
                    PyObject *pymat,*pypla,*pybbox;
                    if(!PyArg_ParseTuple(ret.ptr(),"O!O!O!",&Base::MatrixPy::Type, &pymat,
                                &Base::PlacementPy::Type, &pypla,
                                &Base::BoundBoxPy::Type, &pybbox)) {
                        FC_ERR("initDraggingPlacement() expects return of type tuple(matrix,placement,boundbox)");
                        return false;
                    }
                    dragCtx.reset(new DraggerContext);
                    dragCtx->initialPlacement = *static_cast<Base::PlacementPy*>(pypla)->getPlacementPtr();
                    dragCtx->preTransform = *static_cast<Base::MatrixPy*>(pymat)->getMatrixPtr();
                    dragCtx->bbox = *static_cast<Base::BoundBoxPy*>(pybbox)->getBoundBoxPtr();
                    return true;
                }else if(!ret.isTrue())
                    return false;
            }
        }
    } catch (Py::Exception&) {
        Base::PyException e;
        e.ReportException();
        return false;
    }

    auto ext = getLinkExtension();
    if(!ext) {
        FC_ERR("no link extension");
        return false;
    }
    if(!ext->hasPlacement()) {
        FC_ERR("no placement");
        return false;
    }
    auto doc = Application::Instance->editDocument();
    if(!doc) {
        FC_ERR("no editing document");
        return false;
    }

    dragCtx.reset(new DraggerContext);

    dragCtx->preTransform = doc->getEditingTransform();
    doc->setEditingTransform(dragCtx->preTransform);

    const auto &pla = ext->getPlacementProperty()?
        ext->getPlacementValue():ext->getLinkPlacementValue();

    // Cancel out our own transformation from the editing transform, because
    // the dragger is meant to change our transformation.
    dragCtx->preTransform *= pla.inverse().toMatrix();

    auto selctx = Gui::Selection().getExtendedContext(getObject());
    auto objs = selctx.getSubObjectList();
    auto it = std::find(objs.begin(), objs.end(), getObject());
    if (it != objs.end()) {
        objs.erase(objs.begin(), it+1);
        std::string element = selctx.getElementName();
        selctx = App::SubObjectT(objs);
        selctx.setSubName(selctx.getSubName() + element);
    }
    dragCtx->bbox = getBoundingBox(selctx.getSubName().c_str(),nullptr,false);
    // The returned bounding box is before our own transform, but we still need
    // to scale it to get the correct center.
    auto scale = ext->getScaleVector();
    dragCtx->bbox.ScaleX(scale.x);
    dragCtx->bbox.ScaleY(scale.y);
    dragCtx->bbox.ScaleZ(scale.z);

    auto modifier = QApplication::queryKeyboardModifiers();
    // Determine the dragger base position
    // if CTRL key is down, force to use bound box center,
    // if SHIFT key is down, force to use origine,
    // if not a sub link, use origine,
    // else (e.g. group, array, sub link), use bound box center
    if(mode != TransformAt
            && modifier != Qt::ShiftModifier
            && ((ext->getLinkedObjectValue() && !linkView->hasSubs())
                || modifier == Qt::ControlModifier))
    {
        App::PropertyPlacement *propPla = nullptr;
        if(ext->getLinkTransformValue() && ext->getLinkedObjectValue()) {
            propPla = Base::freecad_dynamic_cast<App::PropertyPlacement>(
                    ext->getLinkedObjectValue()->getPropertyByName("Placement"));
        }
        if(propPla) {
            dragCtx->initialPlacement = pla * propPla->getValue();
            dragCtx->mat *= propPla->getValue().inverse().toMatrix();
        } else
            dragCtx->initialPlacement = pla;

    } else {
        auto offset = ViewProviderDragger::getDragOffset(this);

        // This determines the initial placement of the dragger. We place it at
        // the center and orientation of the focused sub-shape element
        // (obtained using ViewProviderDragger::getDragOffset()).
        dragCtx->initialPlacement = pla.toMatrix() * offset;

        if (dragPlacementIndex >= 0) {
            if (auto plaList = ext->getPlacementListProperty()) {
                if (dragPlacementIndex < plaList->getSize()) {
                    auto mat = plaList->getValue()[dragPlacementIndex].toMatrix();
                    auto scaleMat = ext->getTransform(false);
                    mat = scaleMat * mat;
                    mat.inverseGauss();
                    offset = mat * offset;
                    dragCtx->plaInverse = ext->getTransform(true);
                    dragCtx->plaInverse.inverseGauss();
                }
            }
        }

        // dragCtx->mat is to transform the dragger placement to our own placement.
        // So inverse the transform
        dragCtx->mat = offset;
        dragCtx->mat.inverseGauss();
    }

    return true;
}

bool ViewProviderLink::startDragArrayElement(int mode, int idx)
{
    if (mode != ViewProvider::Transform
            && mode != ViewProvider::TransformAt
            && mode != ViewProvider::Default)
        return false;
    if (mode != ViewProvider::TransformAt)
        dragPlacementIndex = idx;
    else
        dragPlacementIndex = -1;
    if (!startEditing(ViewProvider::TransformAt)) {
        dragPlacementIndex = -1;
        return false;
    }
    return true;
}

void ViewProviderLink::endDragArrayElement()
{
    dragPlacementIndex = -1;
}

ViewProvider *ViewProviderLink::startEditing(int mode) {
    if (mode < 0)
        return nullptr;

    if(mode==ViewProvider::Color) {
        auto ext = getLinkExtension();
        if(!ext || !ext->getColoredElementsProperty()) {
            if(linkEdit(ext))
                return linkView->getLinkedView()->startEditing(mode);
        }
        return inherited::startEditing(mode);
    }

    FC_STATIC bool _pendingTransform;
    FC_STATIC Base::Matrix4D  _editingTransform;

    auto doc = Application::Instance->editDocument();

    if(_pendingTransform || mode == ViewProvider::Transform || mode == ViewProvider::TransformAt) {
        if(_pendingTransform && doc)
            doc->setEditingTransform(_editingTransform);

        if(!initDraggingPlacement(mode))
            return nullptr;
        if(useCenterballDragger)
            pcDragger = CoinPtr<SoCenterballDragger>(new SoCenterballDragger);
        else
            pcDragger = CoinPtr<SoFCCSysDragger>(new SoFCCSysDragger);
        updateDraggingPlacement(dragCtx->initialPlacement,true);
        pcDragger->addStartCallback(dragStartCallback, this);
        pcDragger->addFinishCallback(dragFinishCallback, this);
        pcDragger->addMotionCallback(dragMotionCallback, this);
        return inherited::startEditing(mode);
    }

    if(!linkEdit()) {
        FC_ERR("unsupported edit mode " << mode);
        return nullptr;
    }

    // TODO: the 0x8000 mask here is for caller to disambiguate the intention
    // here, whether they want to, say transform the link itself or the linked
    // object. Use of a mask here will allow forwarding those editing modes that
    // are supported by both the link and the linked object, such as transform
    // and set color. We need to find a better place to declare this constant.
    mode &= ~0x8000;

    if(!doc) {
        FC_ERR("no editing document");
        return nullptr;
    }

    // We are forwarding the editing request to linked object. We need to
    // adjust the editing transformation.
    Base::Matrix4D mat;
    auto linked = getObject()->getLinkedObject(true,&mat,false);
    if(!linked || linked==getObject()) {
        FC_ERR("no linked object");
        return nullptr;
    }
    auto vpd = freecad_dynamic_cast<ViewProviderDocumentObject>(
                Application::Instance->getViewProvider(linked));
    if(!vpd) {
        FC_ERR("no linked viewprovider");
        return nullptr;
    }
    // Amend the editing transformation with the link transformation.
    // But save it first in case the linked object reroute the editing request
    // back to us.
    _editingTransform = doc->getEditingTransform();
    doc->setEditingTransform(doc->getEditingTransform()*mat);
    Base::StateLocker guard(_pendingTransform);
    return vpd->startEditing(mode);
}

bool ViewProviderLink::setEdit(int ModNum)
{
    if (ModNum == ViewProvider::Color) {
        auto ext = getLinkExtension();
        if(!ext || !ext->getColoredElementsProperty())
            return false;
        TaskView::TaskDialog *dlg = Control().activeDialog();
        if (dlg) {
            Control().showDialog(dlg);
            return false;
        }
        return true;
    }
    return inherited::setEdit(ModNum);
}

static QPointer<TaskCSysDragger> _TaskDragger;

void ViewProviderLink::setEditViewer(Gui::View3DInventorViewer* viewer, int ModNum)
{
    if (ModNum == ViewProvider::Color) {
        auto ext = getLinkExtension();
        if(!ext)
            return;
        Gui::Control().showDialog(new TaskElementColors(this, !linkView->isLikeGroup()));
        return;
    }

    if (pcDragger && viewer)
    {
        auto rootPickStyle = new SoPickStyle();
        rootPickStyle->style = SoPickStyle::UNPICKABLE;
        static_cast<SoFCUnifiedSelection*>(
                viewer->getSceneGraph())->insertChild(rootPickStyle, 0);

        if(useCenterballDragger) {
            auto dragger = static_cast<SoCenterballDragger*>(pcDragger.get());
            auto group = new SoAnnotation;
            auto pickStyle = new SoPickStyle;
            pickStyle->setOverride(true);
            group->addChild(pickStyle);
            group->addChild(pcDragger);

            // Because the dragger is not grouped with the actual geometry,
            // we use an invisible cube sized by the bounding box obtained from
            // initDraggingPlacement() to scale the centerball dragger properly

            auto * ss = static_cast<SoSurroundScale*>(dragger->getPart("surroundScale", TRUE));
            ss->numNodesUpToContainer = 3;
            ss->numNodesUpToReset = 2;

            auto *geoGroup = new SoGroup;
            group->addChild(geoGroup);
            auto *style = new SoDrawStyle;
            style->style.setValue(SoDrawStyle::INVISIBLE);
            style->setOverride(TRUE);
            geoGroup->addChild(style);
            auto *cube = new SoCube;
            geoGroup->addChild(cube);
            auto length = std::max(std::max(dragCtx->bbox.LengthX(),
                        dragCtx->bbox.LengthY()), dragCtx->bbox.LengthZ());
            cube->width = length;
            cube->height = length;
            cube->depth = length;

            viewer->setupEditingRoot(group,&dragCtx->preTransform);
        }else{
            auto dragger = static_cast<SoFCCSysDragger*>(pcDragger.get());
            dragger->draggerSize.setValue(0.05f);
            dragger->setUpAutoScale(viewer->getSoRenderManager()->getCamera());
            viewer->setupEditingRoot(pcDragger,&dragCtx->preTransform);

            _TaskDragger = new TaskCSysDragger(this, dragger);
            Gui::Control().showDialog(_TaskDragger);
        }
    }
}

void ViewProviderLink::unsetEditViewer(Gui::View3DInventorViewer* viewer)
{
    dragPlacementIndex = -1;
    SoNode *child = static_cast<SoFCUnifiedSelection*>(viewer->getSceneGraph())->getChild(0);
    if (child && child->isOfType(SoPickStyle::getClassTypeId()))
        static_cast<SoFCUnifiedSelection*>(viewer->getSceneGraph())->removeChild(child);
    pcDragger.reset();
    dragCtx.reset();
    Gui::Control().closeDialog();
}

Base::Placement ViewProviderLink::currentDraggingPlacement() const
{
    // if there isn't an active dragger return a default placement
    if (!pcDragger)
        return Base::Placement();

    SbVec3f v;
    SbRotation r;
    if (useCenterballDragger) {
        auto dragger = static_cast<SoCenterballDragger*>(pcDragger.get());
        v = dragger->center.getValue();
        r = dragger->rotation.getValue();
    }
    else {
        auto dragger = static_cast<SoFCCSysDragger*>(pcDragger.get());
        v = dragger->translation.getValue();
        r = dragger->rotation.getValue();
    }

    float q1,q2,q3,q4;
    r.getValue(q1,q2,q3,q4);
    return Base::Placement(Base::Vector3d(v[0],v[1],v[2]),Base::Rotation(q1,q2,q3,q4));
}

void ViewProviderLink::enableCenterballDragger(bool enable) {
    if(enable == useCenterballDragger)
        return;
    if(pcDragger)
        LINK_THROW(Base::RuntimeError,"Cannot change dragger during dragging");
    useCenterballDragger = enable;
}

void ViewProviderLink::updateDraggingPlacement(const Base::Placement &pla, bool force) {
    if(pcDragger && (force || currentDraggingPlacement()!=pla)) {
        const auto &pos = pla.getPosition();
        const auto &rot = pla.getRotation();
        FC_LOG("updating dragger placement (" << pos.x << ", " << pos.y << ", " << pos.z << ')');
        if(useCenterballDragger) {
            auto dragger = static_cast<SoCenterballDragger*>(pcDragger.get());
            SbBool wasenabled = dragger->enableValueChangedCallbacks(FALSE);
            SbMatrix matrix;
            matrix = convert(pla.toMatrix());
            dragger->center.setValue(SbVec3f(0,0,0));
            dragger->setMotionMatrix(matrix);
            if (wasenabled) {
                dragger->enableValueChangedCallbacks(TRUE);
                dragger->valueChanged();
            }
        }else{
            auto dragger = static_cast<SoFCCSysDragger*>(pcDragger.get());
            dragger->translation.setValue(SbVec3f(pos.x,pos.y,pos.z));
            dragger->rotation.setValue(rot[0],rot[1],rot[2],rot[3]);
        }
    }
}

bool ViewProviderLink::callDraggerProxy(const char *fname, bool update) {
    if(!pcDragger)
        return false;
    Base::PyGILStateLocker lock;
    try {
        auto* proxy = getPropertyByName("Proxy");
        if (proxy && proxy->getTypeId() == App::PropertyPythonObject::getClassTypeId()) {
            Py::Object feature = static_cast<App::PropertyPythonObject*>(proxy)->getValue();
            if (feature.hasAttr(fname)) {
                Py::Callable method(feature.getAttr(fname));
                Py::Tuple args;
                if(method.apply(args).isTrue())
                    return true;
            }
        }
    } catch (Py::Exception&) {
        Base::PyException e;
        e.ReportException();
        return true;
    }

    if(update) {
        auto ext = getLinkExtension();
        if(ext) {
            const auto &pla = currentDraggingPlacement();
            if (dragPlacementIndex >= 0) {
                if (auto plaList = ext->getPlacementListProperty()) {
                    if (dragPlacementIndex < plaList->getSize()) {
                        auto plaNew = dragCtx->plaInverse * pla.toMatrix() * dragCtx->mat;
                        plaList->set1Value(dragPlacementIndex, plaNew);
                    }
                }
            } else {
                auto prop = ext->getLinkPlacementProperty();
                if(!prop)
                    prop = ext->getPlacementProperty();
                if(prop) {
                    auto plaNew = pla * Base::Placement(dragCtx->mat);
                    prop->setValue(plaNew);
                }
            }
            updateDraggingPlacement(pla);
        }
    }
    return false;
}

void ViewProviderLink::dragStartCallback(void *data, SoDragger *) {
    auto me = static_cast<ViewProviderLink*>(data);
    me->dragCtx->initialPlacement = me->currentDraggingPlacement();
    if(!me->callDraggerProxy("onDragStart",false)) {
        me->dragCtx->cmdPending = true;
        me->getDocument()->openCommand(QT_TRANSLATE_NOOP("Command", "Link Transform"));
    }else
        me->dragCtx->cmdPending = false;
}

void ViewProviderLink::dragFinishCallback(void *data, SoDragger *) {
    auto me = static_cast<ViewProviderLink*>(data);
    me->callDraggerProxy("onDragEnd",true);
    if(me->dragCtx->cmdPending) {
        if(me->currentDraggingPlacement() == me->dragCtx->initialPlacement)
            me->getDocument()->abortCommand();
        else {
            if (_TaskDragger)
                _TaskDragger->onEndMove();
            me->getDocument()->commitCommand();
        }
    }
}

void ViewProviderLink::dragMotionCallback(void *data, SoDragger *) {
    auto me = static_cast<ViewProviderLink*>(data);
    me->callDraggerProxy("onDragMotion",true);
}

void ViewProviderLink::updateLinks(ViewProvider *vp) {
    try {
        auto ext = vp->getExtensionByType<ViewProviderLinkObserver>(true);
        if (ext && ext->linkInfo)
            ext->linkInfo->update();
    }
    catch (const Base::TypeError &e) {
        e.ReportException();
    }
    catch (const Base::ValueError &e) {
        e.ReportException();
    }
}

PyObject *ViewProviderLink::getPyObject() {
    if (!pyViewObject)
        pyViewObject = new ViewProviderLinkPy(this);
    pyViewObject->IncRef();
    return pyViewObject;
}

PyObject *ViewProviderLink::getPyLinkView() {
    return linkView->getPyObject();
}

std::map<std::string, App::Color> ViewProviderLink::getElementColors(const char *subname) const {
    auto ext = getLinkExtension();
    if(ext && ext->getColoredElementsProperty()) {
        const auto &mat = ShapeMaterial.getValue();
        auto colors =  getElementColorsFrom(*this,subname,*ext->getColoredElementsProperty(),
                OverrideColorList, OverrideMaterial.getValue(), &mat, ext->getElementCountValue());
        if (!colors.empty())
            return colors;
    }
    if (childVp)
        return childVp->getElementColors(subname);
    return {};
}

std::map<std::string, App::Color> ViewProviderLink::getElementColorsFrom(
            const ViewProviderDocumentObject &vp,
            const char *subname,
            const App::PropertyLinkSub &coloredElements,
            const App::PropertyColorList &colorList,
            bool overrideMaterial,
            const App::Material *shapeMaterial,
            int element_count)
{
    bool isPrefix = true;
    if(!subname)
        subname = "";
    else {
        auto len = strlen(subname);
        isPrefix = !len || subname[len-1]=='.';
    }

    if(!shapeMaterial)
        overrideMaterial = false;

    const auto &subs = coloredElements.getShadowSubs();
    int size = colorList.getSize();

    std::map<std::string, App::Color> colors;

    std::string _subname;
    std::string wildcard(subname);
    if(wildcard == "Face" || wildcard == "Face*" || wildcard.empty()) {
        if(wildcard.size()==4 || overrideMaterial) {
            App::Color c = shapeMaterial->diffuseColor;
            c.a = shapeMaterial->transparency;
            colors["Face"] = c;
            if(wildcard.size()==4)
                return colors;
        }
        if(!wildcard.empty())
            wildcard.resize(4);
    }else if(wildcard == "Edge*")
        wildcard.resize(4);
    else if(wildcard == "Vertex*")
        wildcard.resize(5);
    else if(wildcard == ViewProvider::hiddenMarker()+"*")
        wildcard.resize(ViewProvider::hiddenMarker().size());
    else if(wildcard.back() == '*') {
        _subname = std::move(wildcard);
        _subname.resize(_subname.size()-1);
        subname = _subname.c_str();
        isPrefix = true;
        wildcard.clear();
    } else
        wildcard.clear();

    int i=-1;
    if(!wildcard.empty()) {
        for(const auto &sub : subs) {
            if(++i >= size)
                break;
            auto pos = sub.second.rfind('.');
            if(pos == std::string::npos)
                pos = 0;
            else
                ++pos;
            const char *element = sub.second.c_str()+pos;
            if(boost::starts_with(element,wildcard))
                colors[sub.second] = colorList[i];
            else if(!element[0] && wildcard=="Face")
                colors[sub.second.substr(0,element-sub.second.c_str())+wildcard] = colorList[i];
        }

        bool overridden = false;
        if(wildcard!=ViewProvider::hiddenMarker() && overrideMaterial) {
            auto color = shapeMaterial->diffuseColor;
            color.a = shapeMaterial->transparency;
            colors.emplace(wildcard,color);
            overridden = true;
        }

        // In case of multi-level linking, we recursively call into each level,
        // and merge the colors
        auto vpd = &vp;
        while(1) {
            if(!vpd->getObject())
                break;
            auto link = vpd->getObject()->getLinkedObject(false);
            if(!link || link==vpd->getObject())
                break;
            auto next = freecad_dynamic_cast<ViewProviderLink>(
                    Application::Instance->getViewProvider(link));
            if(!next)
                break;
            if(!overridden && wildcard!=ViewProvider::hiddenMarker() && next->OverrideMaterial.getValue()) {
                auto color = next->ShapeMaterial.getValue().diffuseColor;
                color.a = next->ShapeMaterial.getValue().transparency;
                colors.emplace(wildcard,color);
                overridden = true;
            }
            for(const auto &v : next->getElementColors(subname))
                colors.insert(v);
            vpd = next;
        }

        if(wildcard!=ViewProvider::hiddenMarker()) {
            // Get collapsed array color override.
            const App::LinkBaseExtension *ext=0;
            auto vpLink = freecad_dynamic_cast<ViewProviderLink>(&vp);
            if(vpLink)
                ext = vpLink->getLinkExtension();
            if(ext && ext->_getElementCountValue() && !ext->_getShowElementValue()) {
                const auto &overrides = vpLink->OverrideMaterialList.getValues();
                int i=-1;
                for(const auto &mat : vpLink->MaterialList.getValues()) {
                    if(++i>=(int)overrides.size())
                        break;
                    if(!overrides[i])
                        continue;
                    auto color = mat.diffuseColor;
                    color.a = mat.transparency;
                    colors.emplace(std::to_string(i)+"."+wildcard,color);
                }
            }
        }
        return colors;
    }

    for(const auto &sub : subs) {
        if(++i >= size)
            break;

        int offset = 0;

        if(!sub.second.empty() && element_count>0 && !std::isdigit(sub.second[0])) {
            // For checking and expanding color override of array base
            if(!subname[0]) {
                std::ostringstream ss;
                ss << "0." << sub.second;
                if(vp.getObject()->getSubObject(ss.str().c_str())) {
                    for(int j=0;j<element_count;++j) {
                        ss.str("");
                        ss << j << '.' << sub.second;
                        colors.emplace(ss.str(),colorList[i]);
                    }
                    continue;
                }
            } else if (std::isdigit(subname[0])) {
                const char *dot = strchr(subname,'.');
                if(dot)
                    offset = dot-subname+1;
            }
        }

        if(isPrefix) {
            if(!boost::starts_with(sub.first,subname+offset)
                    && !boost::starts_with(sub.second,subname+offset))
                continue;
        }else if(sub.first!=subname+offset && sub.second!=subname+offset)
            continue;

        if(offset)
            colors.emplace(std::string(subname,offset)+sub.second, colorList[i]);
        else
            colors[sub.second] = colorList[i];
    }

    if(!subname[0])
        return colors;

    bool found = true;
    if(colors.empty()) {
        found = false;
        colors.emplace(subname,App::Color());
    }
    std::map<std::string, App::Color> ret;
    for(const auto &v : colors) {
        const char *pos = 0;
        auto sobj = vp.getObject()->resolve(v.first.c_str(),nullptr,nullptr,&pos);
        if(!sobj || !pos)
            continue;
        auto link = sobj->getLinkedObject(true);
        if(!link || link==vp.getObject())
            continue;
        auto vp = Application::Instance->getViewProvider(sobj->getLinkedObject(true));
        if(!vp)
            continue;
        // In case the topo name is gone, query the shape owner so it can
        // return some suggested elements
        for(const auto &v2 : vp->getElementColors(!pos[0]?"Face":pos)) {
            std::string name;
            if(pos[0])
                name = v.first.substr(0,pos-v.first.c_str())+v2.first;
            else
                name = v.first;
            ret[name] = found?v.second:v2.second;
        }
    }
    return ret;
}

void ViewProviderLink::setElementColors(const std::map<std::string, App::Color> &colorMap) {
    auto ext = getLinkExtension();
    if(!ext || ! ext->getColoredElementsProperty())
        return;
    setElementColorsTo(*this,colorMap,*ext->getColoredElementsProperty(),
            OverrideColorList, &OverrideMaterial, &ShapeMaterial, ext->getElementCountValue());
}

void ViewProviderLink::setElementColorsTo(
        ViewProviderDocumentObject &vp,
        const std::map<std::string, App::Color> &colorMap,
        App::PropertyLinkSub &coloredElements,
        App::PropertyColorList &colorList,
        App::PropertyBool *overrideMaterial,
        App::PropertyMaterial *shapeMaterial,
        int element_count)
{
    if(!vp.getObject())
        return;

    // For checking and collapsing array element color
    std::map<std::string,std::map<int,App::Color> > subMap;

    std::vector<std::string> subs;
    std::vector<App::Color> colors;
    App::Color faceColor;
    bool hasFaceColor = false;
    for(const auto &v : colorMap) {
        if(!hasFaceColor && v.first == "Face") {
            hasFaceColor = true;
            faceColor = v.second;
            continue;
        }

        if(element_count>0 && !v.first.empty() && std::isdigit(v.first[0])) {
            // In case of array, check if there are override of the same
            // sub-element for every array element. And collapse those overrides
            // into one without the index.
            const char *dot = strchr(v.first.c_str(),'.');
            if(dot) {
                subMap[dot+1][std::atoi(v.first.c_str())] = v.second;
                continue;
            }
        }
        subs.push_back(v.first);
        colors.push_back(v.second);
    }
    for(auto &v : subMap) {
        if(element_count == (int)v.second.size()) {
            App::Color firstColor = v.second.begin()->second;
            subs.push_back(v.first);
            colors.push_back(firstColor);
            for(auto it=v.second.begin();it!=v.second.end();) {
                if(it->second==firstColor)
                    it = v.second.erase(it);
                else
                    ++it;
            }
        }
        std::ostringstream ss;
        for(const auto &colorInfo : v.second) {
            ss.str("");
            ss << colorInfo.first << '.' << v.first;
            subs.push_back(ss.str());
            colors.push_back(colorInfo.second);
        }
    }
    if(subs!=coloredElements.getSubValues() || colors!=colorList.getValues()) {
        coloredElements.setStatus(App::Property::User3,true);
        coloredElements.setValue(vp.getObject(),subs);
        coloredElements.setStatus(App::Property::User3,false);
        colorList.setValues(colors);
    }
    if(hasFaceColor && shapeMaterial) {
        auto mat = shapeMaterial->getValue();
        mat.diffuseColor = faceColor;
        mat.transparency = faceColor.a;
        shapeMaterial->setStatus(App::Property::User3,true);
        shapeMaterial->setValue(mat);
        shapeMaterial->setStatus(App::Property::User3,false);
    }
    if(overrideMaterial)
        overrideMaterial->setValue(hasFaceColor);
}

void ViewProviderLink::applyColors() {
    auto ext = getLinkExtension();
    if(!ext || ! ext->getColoredElementsProperty())
        return;
    prevColorOverride = applyColorsTo(*this, prevColorOverride);
}

bool ViewProviderLink::applyColorsTo(ViewProviderDocumentObject &vp, bool prevOverride) {
    auto obj = vp.getObject();
    if (!obj || obj->isRestoring() || vp.isRestoring())
        return prevOverride;
    auto node = vp.getModeSwitch();
    if(!obj || !node)
        return prevOverride;

    SoSelectionElementAction action(SoSelectionElementAction::Color,true);
    // reset color and visibility first
    SoFCSwitch::switchOverride(&action, SoFCSwitch::OverrideVisible);
    if (prevOverride) {
        prevOverride = false;
        action.apply(node);
    }
    // SoFCSwitch::switchOverride(&action, SoFCSwitch::OverrideDefault);

    std::map<std::string, std::map<std::string,App::Color> > colorMap;
    std::set<std::string> hideList;
    auto colors = vp.getElementColors();
    colors.erase("Face");
    for(const auto &v : colors) {
        const char *subname = v.first.c_str();
        const char *element = nullptr;
        auto sobj = obj->resolve(subname,nullptr,nullptr,&element);
        if(!sobj || !element)
            continue;
        if(ViewProvider::hiddenMarker() == element)
            hideList.emplace(subname,element-subname);
        else
            colorMap[std::string(subname,element-subname)][element] = v.second;
    }

    SoTempPath path(10);
    path.ref();
    for(auto &v : colorMap) {
        action.swapColors(v.second);
        if(v.first.empty()) {
            prevOverride = true;
            action.apply(node);
            continue;
        }
        SoDetail *det=nullptr;
        path.truncate(0);
        if(vp.getDetailPath(v.first.c_str(), &path, false, det)) {
            prevOverride = true;
            action.apply(&path);
        }
        delete det;
    }

    action.setType(SoSelectionElementAction::Hide);
    for(const auto &sub : hideList) {
        SoDetail *det=nullptr;
        path.truncate(0);
        if(!sub.empty() && vp.getDetailPath(sub.c_str(), &path, false, det)) {
            prevOverride = true;
            action.apply(&path);
        }
        delete det;
    }
    path.unrefNoDelete();
    return prevOverride;
}

void ViewProviderLink::setOverrideMode(const std::string &mode) {
    auto ext = getLinkExtension();
    if(!ext)
        return;
    auto obj = ext->getTrueLinkedObject(false);
    if(obj && obj!=getObject()) {
        auto vp = Application::Instance->getViewProvider(obj);
        vp->setOverrideMode(mode);
    }
    if(childVp)
        childVp->setOverrideMode(mode);
}

void ViewProviderLink::onBeforeChange(const App::Property *prop) {
    if(prop == &ChildViewProvider) {
        if(childVp) {
            childVp->beforeDelete();
            pcModeSwitch->replaceChild(1,linkView->getLinkRoot());
            childVpLink.reset();
            childVp = nullptr;
            if(pcModeSwitch->getNumChildren() > 1
                    && pcModeSwitch->getChild(2)->isOfType(SoGroup::getClassTypeId()))
            {
                auto group = static_cast<SoGroup*>(pcModeSwitch->getChild(2));
                if(group->getNumChildren() > 1)
                    group->removeChild(1);
            }
        }
    }
    inherited::onBeforeChange(prop);
}

void ViewProviderLink::beforeDelete()
{
    if (childVp)
        childVp->beforeDelete();
    inherited::beforeDelete();
}

static bool isExcludedProperties(const char *name) {
#define CHECK_EXCLUDE_PROP(_name) if(strcmp(name,#_name)==0) return true;
    CHECK_EXCLUDE_PROP(Proxy);
    return false;
}

App::Property *ViewProviderLink::getPropertyByName(const char *name) const {
    auto prop = inherited::getPropertyByName(name);
    if(prop || isExcludedProperties(name))
        return prop;
    if(childVp) {
        prop = childVp->getPropertyByName(name);
        if(prop && !prop->testStatus(App::Property::Hidden))
            return prop;
        prop = nullptr;
    }
    if(pcObject && pcObject->canLinkProperties()) {
        auto linked = getLinkedViewProvider(nullptr,true);
        if(linked && linked!=this)
            prop = linked->getPropertyByName(name);
    }
    return prop;
}

void ViewProviderLink::getPropertyMap(std::map<std::string,App::Property*> &Map) const {
    inherited::getPropertyMap(Map);
    if(!childVp)
        return;
    std::map<std::string,App::Property*> childMap;
    childVp->getPropertyMap(childMap);
    for(const auto &v : childMap) {
        auto ret = Map.insert(v);
        if(!ret.second) {
            auto myProp = ret.first->second;
            if(myProp->testStatus(App::Property::Hidden))
                ret.first->second = v.second;
        }
    }
}

void ViewProviderLink::getPropertyList(std::vector<App::Property*> &List) const {
    std::map<std::string,App::Property*> Map;
    getPropertyMap(Map);
    List.reserve(List.size()+Map.size());
    for(const auto &v:Map)
        List.push_back(v.second);
}

ViewProviderDocumentObject *ViewProviderLink::getLinkedViewProvider(
        std::string *subname, bool recursive) const
{
    auto self = const_cast<ViewProviderLink*>(this);
    auto ext = getLinkExtension();
    if(!ext)
        return self;
    App::DocumentObject *linked = nullptr;
    if(!recursive) {
        linked = ext->getLink();
        const char *s = ext->getSubName();
        if(subname && s)
            *subname = s;
    } else
        linked = ext->getTrueLinkedObject(recursive);
    if(!linked)
        return self;
    auto res = Base::freecad_dynamic_cast<ViewProviderDocumentObject>(
            Application::Instance->getViewProvider(linked));
    if(res)
        return res;
    return self;
}

Base::BoundBox3d ViewProviderLink::_getBoundingBox(
        const char *subname, const Base::Matrix4D *mat, bool transform,
        const View3DInventorViewer *viewer, int depth) const
{
    Base::BoundBox3d bbox;
    auto obj = getObject();
    if(!obj)
        return bbox;

    auto ext = getLinkExtension();
    if(!ext || isGroup(ext,true) || obj->getLinkedObject(false)==obj || (subname && subname[0]))
        return inherited::_getBoundingBox(subname,mat,transform,viewer,depth);

    Base::Matrix4D smat;
    if(mat)
        smat = *mat;

    ViewProvider *vp = nullptr;
    subname = ext->getSubName();
    if(subname && subname[0]) {
        auto sobj = obj->getSubObject(subname,0,&smat,transform,depth);
        if(!sobj || sobj == obj)
            return bbox;
        vp = Application::Instance->getViewProvider(sobj);
    } else {
        auto linked = obj->getLinkedObject(false,&smat,transform,depth);
        if(!linked || linked==obj)
            return bbox;
        vp = Application::Instance->getViewProvider(linked);
    }
    if(!vp || vp == this)
        return Base::BoundBox3d();

    const auto &subs = ext->getSubElements();
    if(subs.empty())
        return vp->getBoundingBox(0,&smat,false,viewer,depth+1);

    for(const auto &s : subs)
        bbox.Add(vp->getBoundingBox(s.c_str(),&smat,false,viewer,depth+1));
    return bbox;
}

void ViewProviderLink::setTransformation(const Base::Matrix4D &rcMatrix)
{
    inherited::setTransformation(rcMatrix);
    auto ext = getLinkExtension();
    if(ext) {
        if (ext->getScaleVectorProperty())
            updateDataPrivate(getLinkExtension(),ext->getScaleVectorProperty());
        else
            updateDataPrivate(getLinkExtension(),ext->getScaleProperty());
    }
}

void ViewProviderLink::setTransformation(const SbMatrix &rcMatrix)
{
    inherited::setTransformation(rcMatrix);
    auto ext = getLinkExtension();
    if(ext) {
        if (ext->getScaleVectorProperty())
            updateDataPrivate(getLinkExtension(),ext->getScaleVectorProperty());
        else
            updateDataPrivate(getLinkExtension(),ext->getScaleProperty());
    }
}

bool ViewProviderLink::canReplaceObject(App::DocumentObject* oldValue,
                                       App::DocumentObject* newValue)
{
    if (!oldValue || !newValue)
        return false;
    auto ext = getLinkExtension();
    if (!ext)
        return false;
    if (!ext->getLinkedObjectProperty())
        return false;
    if (hasElements(ext))
        return false;
    if(!hasSubName && linkView->isLinked()) {
        auto linked = getLinkedView(false,ext);
        if(linked)
            return linked->canReplaceObject(oldValue, newValue);
    }
    return false;
}

int ViewProviderLink::replaceObject(App::DocumentObject* oldValue,
                                    App::DocumentObject* newValue)
{
    auto ext = getLinkExtension();
    if(!ext || !oldValue || !newValue)
        return 0;
    if (ext->getLinkedObjectProperty()) {
        if(hasElements(ext))
            return -1;
        if(!hasSubName && linkView->isLinked()) {
            auto linked = getLinkedView(false,ext);
            if(linked)
                return linked->replaceObject(oldValue, newValue);
        }
        return -1;
    }
    auto prop = ext->getElementListProperty();
    if (!prop)
        return -1;
    int idx=-1, idx2=-1;
    prop->find(oldValue->getNameInDocument(), &idx);
    if (idx < 0)
        return 0;
    prop->find(newValue->getNameInDocument(), &idx2);
    if (idx2 < 0) {
        if (newValue->getDocument() != getDocument()->getDocument()) {
            auto link = static_cast<App::Link*>(
                    getDocument()->getDocument()->addObject("App::Link", "Link"));
            link->LinkedObject.setValue(newValue);
            newValue = link;
        }
        return ViewProviderDocumentObject::replaceObject(oldValue, newValue);
    }
    auto children = prop->getValues();
    children.erase(children.begin()+idx2);
    children.insert(children.begin()+idx, newValue);
    prop->setValues(children);
    return 1;
}

bool ViewProviderLink::canReorderObject(App::DocumentObject* obj,
                                       App::DocumentObject* before)
{
    auto ext = getLinkExtension();
    if (!ext)
        return false;
    if (!ext->getLinkedObjectProperty())
        return canReorderObjectInProperty(ext->getElementListProperty(), obj, before);
    if(hasElements(ext))
        return false;
    if(!hasSubName && linkView->isLinked()) {
        auto linked = getLinkedView(false,ext);
        if(linked)
            return linked->canReorderObject(obj, before);
    }
    return false;
}

bool ViewProviderLink::reorderObjects(const std::vector<App::DocumentObject*> &objs,
                                    App::DocumentObject* before)
{
    auto ext = getLinkExtension();
    if(!ext)
        return 0;
    if (ext->getLinkedObjectProperty()) {
        if(hasElements(ext))
            return false;
        if(!hasSubName && linkView->isLinked()) {
            auto linked = getLinkedView(false,ext);
            if(linked)
                return linked->reorderObjects(objs, before);
        }
    }
        
    return reorderObjectsInProperty(ext->getElementListProperty(), objs, before);
}

static const QByteArray &_refreshIconTag()
{
    static QByteArray _tag("link:refresh");
    return _tag;
}

static const QByteArray &_mutateIconTag()
{
    static QByteArray _tag("link:mutate");
    return _tag;
}

void ViewProviderLink::getExtraIcons(
        std::vector<std::pair<QByteArray, QPixmap> > &icons) const
{
    auto ext = getLinkExtension();
    if (ext && ext->isLinkMutated()) {
        if (ext->getLinkCopyOnChangeTouchedValue())
            icons.emplace_back(_refreshIconTag(), Gui::BitmapFactory().pixmap("LinkRefresh"));
        else
            icons.emplace_back(_mutateIconTag(), Gui::BitmapFactory().pixmap("LinkMutate"));
    }

    return inherited::getExtraIcons(icons);
}

QString ViewProviderLink::getToolTip(const QByteArray &tag) const
{
    if (tag == _refreshIconTag())
        return QObject::tr(
                "The configurable source object has be changed.\n"
                "ALT + click this icon to refresh.");
    else if (tag == _mutateIconTag())
        return QObject::tr(
                "This link points to a mutated instance of a configurable object.\n"
                "ALT + click this icon to select the source object");
    return inherited::getToolTip(tag);
}

bool ViewProviderLink::iconMouseEvent(QMouseEvent *ev, const QByteArray &tag)
{
    if (ev->type() == QEvent::MouseButtonPress) {
        if (tag == _refreshIconTag()) {
            if (auto ext = getLinkExtension()) {
                try {
                    App::AutoTransaction guard("Link refresh");
                    ext->syncCopyOnChange();
                    Command::updateActive();
                } catch (Base::Exception &e) {
                    e.ReportException();
                }
            }
            return true;
        } else if (tag == _mutateIconTag()) {
            if (auto ext = getLinkExtension()) {
                if (auto src = ext->getLinkCopyOnChangeSourceValue()) {
                    if (!(QApplication::queryKeyboardModifiers() & Qt::ControlModifier)) {
                        Selection().selStackPush();
                        Selection().clearCompleteSelection();
                    }
                    Selection().addSelection(App::SubObjectT(src, ""));
                    TreeWidget::scrollItemToTop();
                }
            }
            return true;
        }
    }
    return inherited::iconMouseEvent(ev, tag);
}

////////////////////////////////////////////////////////////////////////////////////////

namespace Gui {
PROPERTY_SOURCE_TEMPLATE(Gui::ViewProviderLinkPython, Gui::ViewProviderLink)
template class GuiExport ViewProviderPythonFeatureT<ViewProviderLink>;
}
