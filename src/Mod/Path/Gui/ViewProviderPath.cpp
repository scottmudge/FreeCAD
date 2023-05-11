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
# include <boost/algorithm/string/replace.hpp>

# include <Inventor/SbVec3f.h>
# include <Inventor/details/SoLineDetail.h>
# include <Inventor/nodes/SoBaseColor.h>
# include <Inventor/nodes/SoCoordinate3.h>
# include <Inventor/nodes/SoDrawStyle.h>
# include <Inventor/nodes/SoMaterial.h>
# include <Inventor/nodes/SoMaterialBinding.h>
# include <Inventor/nodes/SoPointSet.h>
# include <Inventor/nodes/SoPickStyle.h>
# include <Inventor/nodes/SoAnnotation.h>
# include <Inventor/nodes/SoSeparator.h>
# include <Inventor/nodes/SoSwitch.h>
# include <Inventor/nodes/SoTransform.h>
# include <QFile>
# include <QMenu>
# include <QAction>
# include <QApplication>
#endif

#include <Inventor/SbXfBox3d.h>
#include <boost/algorithm/string/replace.hpp>

#include <App/Application.h>
#include <App/DocumentObject.h>
#include <Base/Parameter.h>
#include <Base/Stream.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/SoAxisCrossKit.h>
#include <Gui/SoFCBoundingBox.h>
#include <Gui/SoFCUnifiedSelection.h>
#include <Mod/Path/App/FeaturePath.h>
#include <Mod/Path/App/Path.h>
#include <Mod/Path/App/PathSegmentWalker.h>

#include "ViewProviderPath.h"

using namespace Gui;
using namespace PathGui;
using namespace Path;
using namespace PartGui;

namespace PathGui {

class PathSelectionObserver: public Gui::SelectionObserver {
public:
    static void init() {
        static PathSelectionObserver *instance;
        if(!instance)
            instance = new PathSelectionObserver();
    }

    void setArrow(SoSwitch *pcSwitch=nullptr) {
        if(pcSwitch==pcLastArrowSwitch)
            return;
        if(pcLastArrowSwitch) {
            pcLastArrowSwitch->whichChild = -1;
            pcLastArrowSwitch->unref();
            pcLastArrowSwitch = nullptr;
        }
        if(pcSwitch) {
            pcSwitch->ref();
            pcSwitch->whichChild = 0;
            pcLastArrowSwitch = pcSwitch;
        }
    }

    void onSelectionChanged(const Gui::SelectionChanges& msg) override {
        if(msg.Type == Gui::SelectionChanges::RmvPreselect) {
            setArrow();
            return;
        }
        if(msg.Type!=Gui::SelectionChanges::SetPreselect
                    && msg.Type!=Gui::SelectionChanges::MovePreselect)
            return;
        auto obj = msg.Object.getObject();
        if(!obj)
            return;
        Base::Matrix4D mat;
        auto sobj = obj->getSubObject(msg.pSubName,nullptr,&mat);
        if(!sobj)
            return;
        Base::Matrix4D linkMat;
        auto linked = sobj->getLinkedObject(true,&linkMat,false);
        auto vp = Base::freecad_dynamic_cast<ViewProviderPath>(
                        Application::Instance->getViewProvider(linked));
        if(!vp) {
            setArrow();
            return;
        }

        if(vp->pt0Index >= 0) {
            mat *= linkMat;
            mat.inverse();
            Base::Vector3d pt = mat*Base::Vector3d(msg.x,msg.y,msg.z);
            if(vp->pcLineCoords->point.getNum() > 0){
                auto ptTo = vp->pcLineCoords->point.getValues(vp->pt0Index);
                SbVec3f ptFrom(pt.x,pt.y,pt.z);
                if(ptTo && ptFrom != *ptTo) {
                    vp->pcArrowTransform->pointAt(ptFrom,*ptTo);
                    setArrow(vp->pcArrowSwitch);
                    return;
                }
            }
        }
        setArrow();
    }

    SoSwitch *pcLastArrowSwitch = nullptr;
};
}

//////////////////////////////////////////////////////////////////////////////

PROPERTY_SOURCE(PathGui::ViewProviderPath, Gui::ViewProviderGeometryObject)

ViewProviderPath::ViewProviderPath()
    :pt0Index(-1),blockPropertyChange(false),edgeStart(-1),coordStart(-1),coordEnd(-1),bboxCached(false)
{
    sPixmap = "Path_Toolpath";

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Path");
    unsigned long lcol = hGrp->GetUnsigned("DefaultNormalPathColor",11141375UL); // dark green (0,170,0)
    float lr,lg,lb;
    lr = ((lcol >> 24) & 0xff) / 255.0; lg = ((lcol >> 16) & 0xff) / 255.0; lb = ((lcol >> 8) & 0xff) / 255.0;
    unsigned long mcol = hGrp->GetUnsigned("DefaultPathMarkerColor",1442775295UL); // lime green (85,255,0)
    float mr,mg,mb;
    mr = ((mcol >> 24) & 0xff) / 255.0; mg = ((mcol >> 16) & 0xff) / 255.0; mb = ((mcol >> 8) & 0xff) / 255.0;
    int lwidth = hGrp->GetInt("DefaultPathLineWidth",1);
    ADD_PROPERTY_TYPE(NormalColor,(lr,lg,lb),"Path",App::Prop_None,"The color of the feed rate moves");
    ADD_PROPERTY_TYPE(MarkerColor,(mr,mg,mb),"Path",App::Prop_None,"The color of the markers");
    ADD_PROPERTY_TYPE(LineWidth,(lwidth),"Path",App::Prop_None,"The line width of this path");
    ADD_PROPERTY_TYPE(ShowNodes,(false),"Path",App::Prop_None,"Turns the display of nodes on/off");


    ShowCountConstraints.LowerBound=0;
    ShowCountConstraints.UpperBound=INT_MAX;
    ShowCountConstraints.StepSize=1;
    ShowCount.setConstraints(&ShowCountConstraints);
    StartIndexConstraints.LowerBound=0;
    StartIndexConstraints.UpperBound=INT_MAX;
    StartIndexConstraints.StepSize=1;
    StartIndex.setConstraints(&StartIndexConstraints);
    ADD_PROPERTY_TYPE(StartPosition,(Base::Vector3d()),"Show",App::Prop_None,"Tool initial position");
    ADD_PROPERTY_TYPE(StartIndex,(0),"Show",App::Prop_None,"The index of first GCode to show");
    ADD_PROPERTY_TYPE(ShowCount,(0),"Show",App::Prop_None,"Number of movement GCode to show, 0 means all");

    pcLineCoords = new SoCoordinate3();
    pcLineCoords->ref();

    pcMarkerSwitch = new SoSwitch();
    pcMarkerSwitch->ref();
    pcMarkerSwitch->whichChild = -1;

    pcMarkerCoords = new SoCoordinate3();
    pcMarkerCoords->ref();

    pcMarkerStyle = new SoDrawStyle();
    pcMarkerStyle->ref();
    pcMarkerStyle->style = SoDrawStyle::POINTS;
    pcMarkerStyle->pointSize = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/View")->GetInt("MarkerSize", 4);

    pcDrawStyle = new SoDrawStyle();
    pcDrawStyle->ref();
    pcDrawStyle->style = SoDrawStyle::LINES;
    pcDrawStyle->lineWidth = LineWidth.getValue();

    pcLines = new PartGui::SoBrepEdgeSet();
    pcLines->ref();
    pcLines->coordIndex.setNum(0);

    pcLineColor = new SoMaterial;
    pcLineColor->ref();

    pcMatBind = new SoMaterialBinding;
    pcMatBind->ref();
    pcMatBind->value = SoMaterialBinding::OVERALL;

    pcMarkerColor = new SoBaseColor;
    pcMarkerColor->ref();

    pcArrowSwitch = new SoSwitch();
    pcArrowSwitch->ref();

    auto pArrowGroup = new SoFCPathAnnotation;
    pArrowGroup->priority = 2;

    auto pickStyle = new SoPickStyle;
    pickStyle->style = SoPickStyle::UNPICKABLE;
    pickStyle->setOverride(true);
    pArrowGroup->addChild(pickStyle);

    pcArrowTransform = new SoTransform();
    pArrowGroup->addChild(pcArrowTransform);

    static Gui::CoinPtr<SoShapeScale> pArrowScale;
    static Gui::CoinPtr<SoAxisCrossKit> pArrow;
    if (!pArrow) {
        pArrowScale = new SoShapeScale();
        pArrowScale->scaleFactor = 1.0f;
        pArrow = new SoAxisCrossKit();
        pArrow->set("xAxis.appearance.drawStyle", "style INVISIBLE");
        pArrow->set("xHead.appearance.drawStyle", "style INVISIBLE");
        pArrow->set("yAxis.appearance.drawStyle", "style INVISIBLE");
        pArrow->set("yHead.appearance.drawStyle", "style INVISIBLE");
        pArrow->set("zAxis.appearance.drawStyle", "style INVISIBLE");
        pArrow->set("zHead.transform", "translation 0 0 0");
        pArrowScale->setPart("shape", pArrow);
    }

    pArrowGroup->addChild(pArrowScale);

    pcArrowSwitch->addChild(pArrowGroup);
    pcArrowSwitch->whichChild = -1;

    NormalColor.touch();
    MarkerColor.touch();

    DisplayMode.setStatus(App::Property::Status::Hidden, true);
    unsigned long sstyle = hGrp->GetInt("DefaultSelectionStyle",0);
    if(sstyle==0 || sstyle==1)
        SelectionStyle.setValue(sstyle);

    PathSelectionObserver::init();
}

ViewProviderPath::~ViewProviderPath()
{
    pcLineCoords->unref();
    pcMarkerCoords->unref();
    pcMarkerSwitch->unref();
    pcDrawStyle->unref();
    pcMarkerStyle->unref();
    pcLines->unref();
    pcLineColor->unref();
    pcMatBind->unref();
    pcMarkerColor->unref();
    pcArrowSwitch->unref();
}

void ViewProviderPath::attach(App::DocumentObject *pcObj)
{
    inherited::attach(pcObj);

    // Draw trajectory lines
    SoSeparator* linesep = new SoSeparator;
    linesep->addChild(pcLineColor);
    linesep->addChild(pcMatBind);
    linesep->addChild(pcDrawStyle);
    linesep->addChild(pcLineCoords);
    linesep->addChild(pcLines);

    // Draw markers
    SoSeparator* markersep = new SoSeparator;
    SoPointSet* marker = new SoPointSet;
    markersep->addChild(pcMarkerColor);
    markersep->addChild(pcMarkerCoords);
    markersep->addChild(pcMarkerStyle);
    markersep->addChild(marker);
    pcMarkerSwitch->addChild(markersep);

    SoSeparator* pcPathRoot = new SoSeparator();
    pcPathRoot->addChild(pcMarkerSwitch);
    pcPathRoot->addChild(linesep);
    pcPathRoot->addChild(pcArrowSwitch);

    addDisplayMaskMode(pcPathRoot, "Waypoints");
}

bool ViewProviderPath::useNewSelectionModel() const
{
    return true;
}

void ViewProviderPath::setDisplayMode(const char* ModeName)
{
    if ( strcmp("Waypoints",ModeName)==0 )
        setDisplayMaskMode("Waypoints");
    inherited::setDisplayMode( ModeName );
}

std::vector<std::string> ViewProviderPath::getDisplayModes() const
{
    std::vector<std::string> StrList;
    StrList.emplace_back("Waypoints");
    return StrList;
}

std::string ViewProviderPath::getElement(const SoDetail* detail) const
{
    if(edgeStart>=0 && detail && detail->getTypeId() == SoLineDetail::getClassTypeId()) {
        const SoLineDetail* line_detail = static_cast<const SoLineDetail*>(detail);
        int index = line_detail->getLineIndex()+edgeStart;
        if(index>=0 && index<(int)edge2Command.size()) {
            index = edge2Command[index];
            Path::Feature* pcPathObj = static_cast<Path::Feature*>(pcObject);
            const Toolpath &tp = pcPathObj->Path.getValue();
            if(index<(int)tp.getSize()) {
                std::stringstream str;
                str << index+1 << " " << tp.getCommand(index).toGCode(6,false);
                pt0Index = line_detail->getPoint0()->getCoordinateIndex();
                if(pt0Index<0 || pt0Index>=pcLineCoords->point.getNum())
                    pt0Index = -1;
                return boost::replace_all_copy(str.str(),".",",");
            }
        }
    }
    pt0Index = -1;
    pcArrowSwitch->whichChild = -1;
    return std::string();
}

SoDetail* ViewProviderPath::getDetail(const char* subelement) const
{
    int index = std::atoi(subelement);
    SoDetail* detail = nullptr;
    if (index>0 && index<=(int)command2Edge.size()) {
        index = command2Edge[index-1];
        if(index>=0 && edgeStart>=0 && edgeStart<=index) {
            detail = new SoLineDetail();
            static_cast<SoLineDetail*>(detail)->setLineIndex(index-edgeStart);
        }
    }
    return detail;
}

void ViewProviderPath::onChanged(const App::Property* prop)
{
    if(blockPropertyChange)
        return;

    if (prop == &LineWidth) {
        pcDrawStyle->lineWidth = LineWidth.getValue();
    } else if (prop == &NormalColor) {
        if (!colorindex.empty() && coordStart>=0 && coordStart<(int)colorindex.size()) {
            const App::Color& c = NormalColor.getValue();
            ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Path");
            unsigned long rcol = hGrp->GetUnsigned("DefaultRapidPathColor",2852126975UL); // dark red (170,0,0)
            float rr,rg,rb;
            rr = ((rcol >> 24) & 0xff) / 255.0; rg = ((rcol >> 16) & 0xff) / 255.0; rb = ((rcol >> 8) & 0xff) / 255.0;

            unsigned long pcol = hGrp->GetUnsigned("DefaultProbePathColor",4293591295UL); // yellow (255,255,5)
            float pr,pg,pb;
            pr = ((pcol >> 24) & 0xff) / 255.0; pg = ((pcol >> 16) & 0xff) / 255.0; pb = ((pcol >> 8) & 0xff) / 255.0;

            pcMatBind->value = SoMaterialBinding::PER_PART;
            // resizing and writing the color vector:

            int count = coordEnd-coordStart;
            if(count > (int)colorindex.size()-coordStart) count = colorindex.size()-coordStart;
            pcLineColor->diffuseColor.setNum(count);
            SbColor* colors = pcLineColor->diffuseColor.startEditing();
            for(int i=0;i<count;i++) {
                switch(colorindex[i+coordStart]){
                case 0:
                    colors[i] = SbColor(rr,rg,rb);
                    break;
                case 1:
                    colors[i] = SbColor(c.r,c.g,c.b);
                    break;
                default:
                    colors[i] = SbColor(pr,pg,pb);
                }
            }
            pcLineColor->diffuseColor.finishEditing();
        }
    } else if (prop == &MarkerColor) {
        const App::Color& c = MarkerColor.getValue();
        pcMarkerColor->rgb.setValue(c.r,c.g,c.b);
    } else if(prop == &ShowNodes) {
        pcMarkerSwitch->whichChild = ShowNodes.getValue()?0:-1;
    } else if (prop == &ShowCount || prop==&StartIndex) {
        bool vis = isShow();
        if (vis) hide();
        updateVisual();
        if (vis) show();
    } else if (prop == &StartPosition) {
        if(pcLineCoords->point.getNum()){
            const Base::Vector3d &pt = StartPosition.getValue();
            pcLineCoords->point.set1Value(0,pt.x,pt.y,pt.z);
            pcMarkerCoords->point.set1Value(0,pt.x,pt.y,pt.z);
        }
    } else 
        inherited::onChanged(prop);
}

void ViewProviderPath::showBoundingBox(bool show) {
    if(show) {
        if(!pcLineCoords->point.getNum())
            return;
    }
    inherited::showBoundingBox(show);
}

unsigned long ViewProviderPath::getBoundColor() const {
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Path");
    if(SelectionStyle.getValue() == 0 || !Selectable.getValue())
        return hGrp->GetUnsigned("DefaultBBoxNormalColor",4294967295UL); // white (255,255,255)
    else
        return hGrp->GetUnsigned("DefaultBBoxSelectionColor",0xc8ffff00UL); // rgb(0,85,255)
}

void ViewProviderPath::updateShowConstraints() {
    Path::Feature* pcPathObj = static_cast<Path::Feature*>(pcObject);
    const Toolpath &tp = pcPathObj->Path.getValue();

    StartIndexConstraints.UpperBound = tp.getSize();

    if (StartIndex.getValue() >= (long)tp.getSize()) {
        int start = ((int)tp.getSize())-ShowCount.getValue();
        if(start>=(int)tp.getSize())
            start=tp.getSize()-1;
        if(start<0) start = 0;
        blockPropertyChange = true;
        StartIndex.setValue(start);
        blockPropertyChange = false;
        StartIndex.purgeTouched();
    }
    StartIndexConstraints.StepSize = ShowCount.getValue()>2?ShowCount.getValue()-2:1;
}

void ViewProviderPath::updateData(const App::Property* prop)
{
    Path::Feature* pcPathObj = static_cast<Path::Feature*>(pcObject);
    if(prop == &pcPathObj->Path) {
        updateVisual(true);
        return;
    }
    inherited::updateData(prop);
}

void ViewProviderPath::hideSelection() {
    // Clear selection
    SoSelectionElementAction saction(Gui::SoSelectionElementAction::None);
    saction.apply(pcLines);

    // Clear highlighting
    SoHighlightElementAction haction;
    haction.apply(pcLines);

    // Hide arrow
    pcArrowSwitch->whichChild = -1;
}

class VisualPathSegmentVisitor
: public PathSegmentVisitor
{
public:
    VisualPathSegmentVisitor(
        const Toolpath &tp,
        SoCoordinate3 *pcLineCoords_,
        SoCoordinate3 *pcMarkerCoords_,
        std::vector<int> &command2Edge_,
        std::deque<int> &edge2Command_,
        std::deque<int> &edgeIndices_,
        std::vector<int> &colorindex_,
        std::deque<Base::Vector3d> &points_,
        std::deque<Base::Vector3d> &markers_)
    : pcLineCoords(pcLineCoords_)
    , pcMarkerCoords(pcMarkerCoords_)
    , command2Edge(command2Edge_)
    , edge2Command(edge2Command_)
    , edgeIndices(edgeIndices_)
    , colorindex(colorindex_)
    , points(points_)
    , markers(markers_)
    {
        pcLineCoords->point.deleteValues(0);
        pcMarkerCoords->point.deleteValues(0);

        command2Edge.clear();
        edge2Command.clear();
        edgeIndices.clear();

        colorindex.clear();

        command2Edge.resize(tp.getSize(),-1);
    }

    void setup(const Base::Vector3d &last) override
    {
        points.push_back(last);
        markers.push_back(last);
    }

    void g0(int id, const Base::Vector3d &last, const Base::Vector3d &next, const std::deque<Base::Vector3d> &pts) override
    {
        (void)last;
        gx(id, &next, pts, 0);
    }

    void g1(int id, const Base::Vector3d &last, const Base::Vector3d &next, const std::deque<Base::Vector3d> &pts) override
    {
        (void)last;
        gx(id, &next, pts, 1);
    }

    void g23(int id, const Base::Vector3d &last, const Base::Vector3d &next, const std::deque<Base::Vector3d> &pts, const Base::Vector3d &center) override
    {
        (void)last;
        gx(id, &next, pts, 1);
        markers.push_back(center);
    }

    void g8x(int id, const Base::Vector3d &last, const Base::Vector3d &next, const std::deque<Base::Vector3d> &pts,
                     const std::deque<Base::Vector3d> &p, const std::deque<Base::Vector3d> &q) override
    {
        (void)last;

        gx(id, nullptr, pts, 0);

        points.push_back(p[0]);
        markers.push_back(p[0]);
        colorindex.push_back(0);

        points.push_back(p[1]);
        markers.push_back(p[1]);
        colorindex.push_back(0);

        points.push_back(next);
        markers.push_back(next);
        colorindex.push_back(1);

        for (std::deque<Base::Vector3d>::const_iterator it=q.begin(); q.end() != it; ++it) {
            markers.push_back(*it);
        }

        points.push_back(p[2]);
        markers.push_back(p[2]);
        colorindex.push_back(0);

        pushCommand(id);
    }

    void g38(int id, const Base::Vector3d &last, const Base::Vector3d &next) override
    {
#if 0
      Base::Vector3d p1(next.x,next.y,last.z);
      points.push_back(p1);
      colorindex.push_back(0);

      points.push_back(next);
      colorindex.push_back(2);

      Base::Vector3d p3(next.x,next.y,last.z);
      points.push_back(p3);
      colorindex.push_back(0);

      pushCommand(id);
#else
      (void)last;
      const std::deque<Base::Vector3d> pts{};
      gx(id, &next, pts, 2);
#endif
    }

private:
    SoCoordinate3 *pcLineCoords;
    SoCoordinate3 *pcMarkerCoords;

    std::vector<int> &command2Edge;
    std::deque<int> &edge2Command;
    std::deque<int> &edgeIndices;

    std::vector<int> &colorindex;
    std::deque<Base::Vector3d> &points;
    std::deque<Base::Vector3d> &markers;

    virtual void gx(int id, const Base::Vector3d *next, const std::deque<Base::Vector3d> &pts, int color)
    {
        for (std::deque<Base::Vector3d>::const_iterator it=pts.begin(); pts.end() != it; ++it) {
          points.push_back(*it);
          colorindex.push_back(color);
        }

        if (next) {
            points.push_back(*next);
            markers.push_back(*next);
            colorindex.push_back(color);

            pushCommand(id);
        }
    }

    void pushCommand(int id) {
      command2Edge[id] = edgeIndices.size();
      edgeIndices.push_back(points.size());
      edge2Command.push_back(id);
    }
};

void ViewProviderPath::updateVisual(bool rebuild) {

    hideSelection();

    updateShowConstraints();

    pcLines->coordIndex.deleteValues(0);

    if(rebuild) {
        Path::Feature* pcPathObj = static_cast<Path::Feature*>(pcObject);
        const Toolpath &tp = pcPathObj->Path.getValue();

        std::deque<Base::Vector3d> points;
        std::deque<Base::Vector3d> markers;

        VisualPathSegmentVisitor collect(tp,
            pcLineCoords,
            pcMarkerCoords,
            command2Edge,
            edge2Command,
            edgeIndices,
            colorindex,
            points,
            markers);

        PathSegmentWalker segments(tp);
        segments.walk(collect, StartPosition.getValue());


        if (!edgeIndices.empty()) {
            pcLineCoords->point.setNum(points.size());
            SbVec3f* verts = pcLineCoords->point.startEditing();
            int i=0;
            for(const auto &pt : points)
                verts[i++].setValue(pt.x,pt.y,pt.z);
            pcLineCoords->point.finishEditing();

            pcMarkerCoords->point.setNum(markers.size());
            for(unsigned int i=0;i<markers.size();i++)
                pcMarkerCoords->point.set1Value(i,markers[i].x,markers[i].y,markers[i].z);

            bboxCached = false;
            updateBoundingBox();
        }
    }

    // count = index + separators
    edgeStart = -1;
    int i;
    for(i=StartIndex.getValue();i<(int)command2Edge.size();++i)
        if((edgeStart=command2Edge[i])>=0) break;

    if(edgeStart<0)
        return;

    if(i!=StartIndex.getValue() && StartIndex.getValue()!=0) {
        blockPropertyChange = true;
        StartIndex.setValue(i);
        blockPropertyChange = false;
        StartIndex.purgeTouched();
    }

    int edgeEnd = edgeStart+ShowCount.getValue();
    if(edgeEnd==edgeStart || edgeEnd>(int)edgeIndices.size())
        edgeEnd = edgeIndices.size();

    // coord index start
    coordStart = edgeStart==0?0:(edgeIndices[edgeStart-1]-1);
    coordEnd = edgeIndices[edgeEnd-1];

    // count = coord indices + index separators
    int count = coordEnd-coordStart+2*(edgeEnd-edgeStart-1)+1;

    pcLines->coordIndex.setNum(count);
    int32_t *idx = pcLines->coordIndex.startEditing();
    i=0;
    int start = coordStart;
    for(int e=edgeStart;e!=edgeEnd;++e) {
        for(int end=edgeIndices[e];start<end;++start)
            idx[i++] = start;
        idx[i++]=-1;
        --start;
    }
    pcLines->coordIndex.finishEditing();
    assert(i==count);

    NormalColor.touch();
}

Base::BoundBox3d ViewProviderPath::_getBoundingBox(
        const char *, const Base::Matrix4D *_mat, unsigned transform,
        const Gui::View3DInventorViewer *, int) const 
{
    // update the boundbox
    Base::Matrix4D mat;
    if(_mat)
        mat = *_mat;
    if(transform & Gui::ViewProvider::Transform) {
        Path::Feature* pcPathObj = static_cast<Path::Feature*>(pcObject);
        mat *= pcPathObj->Placement.getValue().toMatrix();
    }

    if(!bboxCached) {
        bboxCached = true;
        Base::BoundBox3d bbox;
        Base::Vector3d pt;
        for (int i=1;i<pcLineCoords->point.getNum();i++) {
            pt.x = pcLineCoords->point[i].getValue()[0];
            pt.y = pcLineCoords->point[i].getValue()[1];
            pt.z = pcLineCoords->point[i].getValue()[2];
            bbox.Add(pt);
        }
        bboxCache = bbox;
    }

    SbXfBox3d xbox;
    xbox.setBounds(bboxCache.MinX,bboxCache.MinY,bboxCache.MinZ,
                   bboxCache.MaxX,bboxCache.MaxY,bboxCache.MaxZ);
    xbox.setTransform(ViewProvider::convert(mat));

    Base::BoundBox3d bbox;
    xbox.project().getBounds(bbox.MinX,bbox.MinY,bbox.MinZ,
                             bbox.MaxX,bbox.MaxY,bbox.MaxZ);
    return bbox;
}

template<class Func>
static void setupFilter(const char *msg,
                        Path::Feature *feat,
                        const std::vector<int> &indices,
                        Func f)
{
    App::AutoTransaction guard(msg);
    auto edges = feat->CommandFilter.getValues();
    std::set<int> filter(edges.begin(), edges.end());
    for (int idx : indices)
        f(filter, idx);
    edges.clear();
    edges.insert(edges.end(), filter.begin(), filter.end());
    feat->CommandFilter.setValues(std::move(edges));
    feat->BuildShape.setValue(true);
    Gui::Command::updateActive();
}

void ViewProviderPath::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    auto feat = Base::freecad_dynamic_cast<Path::Feature>(getObject());
    if (!feat)
        return;

    QAction* act = menu->addAction(QObject::tr("Inspect G-Code"), receiver, member);
    act->setData(QVariant((int)Gui::ViewProvider::Default));

    act = menu->addAction(QObject::tr("Toggle build shape"), [feat]() {
        App::AutoTransaction guard(QT_TRANSLATE_NOOP("ViewProviderPath", "Reset all edges"));
        feat->BuildShape.setValue(!feat->BuildShape.getValue());
        Gui::Command::updateActive();
    });
    act->setToolTip(QApplication::translate("ViewProviderPath", "Toggle building shape from G-Code"));

    std::vector<int> indices;
    for (const auto &sel : Gui::Selection().getSelection()) {
        if (sel.pObject != feat || !sel.SubName)
            continue;
        int idx = atoi(sel.SubName);
        if (idx > 0)
            indices.push_back(idx);
    }

    if (indices.size()) {
        act = menu->addAction(QObject::tr("Toggle selected edge(s)"), [feat, indices]() {
            setupFilter(QT_TRANSLATE_NOOP("ViewProviderPath", "Reset all edges"), feat, indices,
                [](std::set<int> &filter, int idx){
                    auto res = filter.insert(idx);
                    if (!res.second)
                        filter.erase(res.first);
                });
        });
        act->setToolTip(QApplication::translate("ViewProviderPath", "Toggle edge(s) of shape from G-Code"));

        act = menu->addAction(QObject::tr("Remove edge(s)"), [feat, indices]() {
            setupFilter(QT_TRANSLATE_NOOP("ViewProviderPath", "Remove selected edge(s)"), feat, indices,
                [](std::set<int> &filter, int idx) {
                    filter.insert(idx);
                });
        });
        act->setToolTip(QApplication::translate("ViewProviderPath", "Remove edge(s) of shape from G-Code"));
    }

    act = menu->addAction(QObject::tr("Reset all edges"), [feat]() {
        App::AutoTransaction guard(QT_TRANSLATE_NOOP("ViewProviderPath", "Reset all edges"));
        feat->CommandFilter.setValues();
        Gui::Command::updateActive();
    });
    act->setToolTip(QApplication::translate("ViewProviderPath", "Clear edge filter"));

    inherited::setupContextMenu(menu, receiver, member);
}

bool ViewProviderPath::doubleClicked(void)
{
    Gui::Application::Instance->activeDocument()->setEdit(this, (int)ViewProvider::Default);
    return true;
}

bool ViewProviderPath::setEdit(int ModNum)
{
    if (ModNum == ViewProvider::Default) {
        try {
            Gui::Command::addModule(Gui::Command::Doc, "PathScripts.PathInspect");
            Gui::Command::doCommand(Gui::Command::Doc, 
                    "PathScripts.PathInspect.show(%s)", getObject()->getFullName(true).c_str()); 
        } catch (Base::Exception &e) {
            e.ReportException();
        }
        return false;
    }
    return ViewProviderGeometryObject::setEdit(ModNum);
}

// Python object -----------------------------------------------------------------------

namespace Gui {
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(PathGui::ViewProviderPathPython, PathGui::ViewProviderPath)
/// @endcond

// explicit template instantiation
template class PathGuiExport ViewProviderPythonFeatureT<PathGui::ViewProviderPath>;
}
