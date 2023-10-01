/***************************************************************************
 *   Copyright (c) 2016 WandererFan <wandererfan@gmail.com>                *
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

#ifndef DrawViewDetail_h_
#define DrawViewDetail_h_

#include <gp_Ax2.hxx>
#include <TopoDS_Shape.hxx>

#include <App/DocumentObject.h>
#include <App/FeaturePython.h>
#include <App/PropertyLinks.h>
#include <Mod/TechDraw/TechDrawGlobal.h>

#include "DrawViewPart.h"


class gp_Pln;
class gp_Ax2;
class TopoDS_Face;

namespace TechDraw
{
class Face;
}

namespace TechDraw
{


class TechDrawExport DrawViewDetail : public DrawViewPart
{
    PROPERTY_HEADER_WITH_OVERRIDE(Part::DrawViewDetail);

public:
    /// Constructor
    DrawViewDetail();
    ~DrawViewDetail() override;

    App::PropertyLink   BaseView;
    App::PropertyVector AnchorPoint;
    App::PropertyFloat  Radius;
    App::PropertyString Reference;

    short mustExecute() const override;
    App::DocumentObjectExecReturn *execute() override;
    void onChanged(const App::Property* prop) override;
    const char* getViewProviderName() const override {
        return "TechDrawGui::ViewProviderViewPart";
    }
    void unsetupObject() override;


    void detailExec(const TopoDS_Shape& s,
                    DrawViewPart* baseView,
                    DrawViewSection* sectionAlias);
    void postHlrTasks() override;
    void waitingForDetail(bool s) { m_waitingForDetail = s; }
    bool waitingForDetail(void) const { return m_waitingForDetail; }
    bool waitingForResult() const override;

    double getFudgeRadius(void);
    static TopoDS_Shape projectEdgesOntoFace(TopoDS_Shape& edgeShape,
                                             TopoDS_Face& projFace,
                                             gp_Dir& projDir);

    std::vector<DrawViewDetail*> getDetailRefs() const override;

protected:
    struct Output {
        TopoDS_Shape shape;
        Base::Vector3d centroid;
    };
    void onMakeDetailFinished(std::shared_ptr<Output> output);

    struct DetailParams {
        std::string featureName;
        std::shared_ptr<Base::SequencerLauncher> progress;
        std::shared_ptr<Output> output;
        TopoDS_Shape shape;
        gp_Ax2 viewAxis;
        Base::Vector3d dirDetail;
        Base::Vector3d shapeCenter;
        Base::Vector3d anchorPoint;
        double radius;
        double rotation;
        double scale;
        bool moveShape;
    };
    static void makeDetailShape(const DetailParams &params);

    void abortMakeDetail();

    void getParameters(void);
    double m_fudge;
    static bool debugDetail();

    TopoDS_Shape m_scaledShape;
    gp_Ax2 m_viewAxis;

    DrawViewPart* m_saveDvp;
    DrawViewSection* m_saveDvs;

private:
    std::unique_ptr<QFutureWatcher<void>> m_detailWatcher;
    std::shared_ptr<Base::SequencerLauncher> m_progress;
    bool m_waitingForDetail = false;
};

using DrawViewDetailPython = App::FeaturePythonT<DrawViewDetail>;

} //namespace TechDraw

#endif
