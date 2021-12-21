/***************************************************************************
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>              *
 *   Copyright (c) 2014 Luke Parry <l.parry@warwick.ac.uk>                 *
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
# include <sstream>
# include <QCoreApplication>
# include <QDir>
# include <QFile>
# include <QFileInfo>
# include <QMessageBox>
# include <QRegExp>
#endif

#include <QStringBuilder>

#include <QGraphicsView>
#include <QPainter>
#include <QSvgRenderer>
#include <QSvgGenerator>

#include <vector>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObjectGroup.h>
#include <App/DocumentObject.h>
#include <App/FeaturePython.h>
#include <App/PropertyGeo.h>
#include <App/GeoFeature.h>
#include <App/Link.h>
#include <App/PropertyGeo.h>
#include <App/PropertyLinks.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Parameter.h>
#include <Base/PyObjectBase.h>
#include <Base/Tools.h>
#include <Gui/Action.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/CommandT.h>
#include <Gui/Control.h>
#include <Gui/Document.h>
#include <Gui/FileDialog.h>
#include <Gui/MainWindow.h>
#include <Gui/Selection.h>
#include <Gui/ViewProvider.h>
#include <Gui/WaitCursor.h>

#include <Mod/Part/App/PartFeature.h>
#include <Mod/Part/App/Part2DObject.h>
#include <Mod/Spreadsheet/App/Sheet.h>

#include <Mod/TechDraw/App/DrawPage.h>
#include <Mod/TechDraw/App/DrawViewPart.h>
#include <Mod/TechDraw/App/DrawProjGroupItem.h>
#include <Mod/TechDraw/App/DrawProjGroup.h>
#include <Mod/TechDraw/App/DrawViewDimension.h>
#include <Mod/TechDraw/App/DrawViewBalloon.h>
#include <Mod/TechDraw/App/DrawViewClip.h>
#include <Mod/TechDraw/App/DrawViewSymbol.h>
#include <Mod/TechDraw/App/DrawViewDraft.h>
#include <Mod/TechDraw/App/DrawViewMulti.h>
#include <Mod/TechDraw/App/DrawViewDetail.h>
#include <Mod/TechDraw/App/DrawViewArch.h>
#include <Mod/TechDraw/App/DrawUtil.h>
#include <Mod/TechDraw/Gui/QGVPage.h>

#include "DrawGuiUtil.h"
#include "PreferencesGui.h"
#include "MDIViewPage.h"
#include "PreferencesGui.h"
#include "QGIViewPart.h"
#include "Rez.h"
#include "TaskProjGroup.h"
#include "TaskSectionView.h"
#include "TaskActiveView.h"
#include "TaskDetail.h"
#include "ViewProviderPage.h"
#include "ViewProviderViewPart.h"

class Vertex;
using namespace TechDrawGui;
using namespace TechDraw;
using namespace std;

//===========================================================================
// TechDraw_PageDefault
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawPageDefault)

CmdTechDrawPageDefault::CmdTechDrawPageDefault()
  : Command("TechDraw_PageDefault")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Insert Default Page");
    sToolTipText    = sMenuText;
    sWhatsThis      = "TechDraw_PageDefault";
    sStatusTip      = sToolTipText;
    sPixmap         = "actions/TechDraw_PageDefault";
}

void CmdTechDrawPageDefault::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    QString templateFileName = Preferences::defaultTemplate();

    std::string PageName = getUniqueObjectName("Page");
    std::string TemplateName = getUniqueObjectName("Template");

    QFileInfo tfi(templateFileName);
    if (tfi.isReadable()) {
        Gui::WaitCursor wc;
        openCommand(QT_TRANSLATE_NOOP("Command", "Drawing create page"));
        doCommand(Doc,"App.activeDocument().addObject('TechDraw::DrawPage','%s')",PageName.c_str());
        doCommand(Doc,"App.activeDocument().addObject('TechDraw::DrawSVGTemplate','%s')",TemplateName.c_str());

        doCommand(Doc,"App.activeDocument().%s.Template = '%s'",TemplateName.c_str(), templateFileName.toStdString().c_str());
        doCommand(Doc,"App.activeDocument().%s.Template = App.activeDocument().%s",PageName.c_str(),TemplateName.c_str());

        commitCommand();
        TechDraw::DrawPage* fp = dynamic_cast<TechDraw::DrawPage*>(getDocument()->getObject(PageName.c_str()));
        if (!fp) {
            throw Base::TypeError("CmdTechDrawPageDefault fp not found\n");
        }

        Gui::ViewProvider* vp = Gui::Application::Instance->getDocument(getDocument())->getViewProvider(fp);
        TechDrawGui::ViewProviderPage* dvp = dynamic_cast<TechDrawGui::ViewProviderPage*>(vp);
        if (dvp) {
            dvp->show();
        }
        else {
            Base::Console().Log("INFO - Template: %s for Page: %s NOT Found\n", PageName.c_str(),TemplateName.c_str());
        }
    } else {
        QMessageBox::critical(Gui::getMainWindow(),
            QLatin1String("No template"),
            QLatin1String("No default template found"));
    }
}

bool CmdTechDrawPageDefault::isActive(void)
{
    return hasActiveDocument();
}

//===========================================================================
// TechDraw_PageTemplate
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawPageTemplate)

CmdTechDrawPageTemplate::CmdTechDrawPageTemplate()
  : Command("TechDraw_PageTemplate")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Insert Page using Template");
    sToolTipText    = sMenuText;
    sWhatsThis      = "TechDraw_PageTemplate";
    sStatusTip      = sToolTipText;
    sPixmap         = "actions/TechDraw_PageTemplate";
}

void CmdTechDrawPageTemplate::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    QString work_dir = Gui::FileDialog::getWorkingDirectory();
    QString templateDir = Preferences::defaultTemplateDir();
    QString templateFileName = Gui::FileDialog::getOpenFileName(Gui::getMainWindow(),
                                                   QString::fromUtf8(QT_TR_NOOP("Select a Template File")),
                                                   templateDir,
                                                   QString::fromUtf8(QT_TR_NOOP("Template (*.svg *.dxf)")));
    Gui::FileDialog::setWorkingDirectory(work_dir);  // Don't overwrite WD with templateDir

    if (templateFileName.isEmpty()) {
        return;
    }

    std::string PageName = getUniqueObjectName("Page");
    std::string TemplateName = getUniqueObjectName("Template");

    QFileInfo tfi(templateFileName);
    if (tfi.isReadable()) {
        Gui::WaitCursor wc;
        openCommand(QT_TRANSLATE_NOOP("Command", "Drawing create page"));
        doCommand(Doc,"App.activeDocument().addObject('TechDraw::DrawPage','%s')",PageName.c_str());

        // Create the Template Object to attach to the page
        doCommand(Doc,"App.activeDocument().addObject('TechDraw::DrawSVGTemplate','%s')",TemplateName.c_str());

        //why is "Template" property set twice? -wf
        // once to set DrawSVGTemplate.Template to OS template file name
        templateFileName = Base::Tools::escapeEncodeFilename(templateFileName);
        doCommand(Doc,"App.activeDocument().%s.Template = \"%s\"",TemplateName.c_str(), templateFileName.toUtf8().constData());
        // once to set Page.Template to DrawSVGTemplate.Name
        doCommand(Doc,"App.activeDocument().%s.Template = App.activeDocument().%s",PageName.c_str(),TemplateName.c_str());
        // consider renaming DrawSVGTemplate.Template property?

        commitCommand();
        TechDraw::DrawPage* fp = dynamic_cast<TechDraw::DrawPage*>(getDocument()->getObject(PageName.c_str()));
        if (!fp) {
            throw Base::TypeError("CmdTechDrawNewPagePick fp not found\n");
        }
        Gui::ViewProvider* vp = Gui::Application::Instance->getDocument(getDocument())->getViewProvider(fp);
        TechDrawGui::ViewProviderPage* dvp = dynamic_cast<TechDrawGui::ViewProviderPage*>(vp);
        if (dvp) {
            dvp->show();
        }
        else {
            Base::Console().Log("INFO - Template: %s for Page: %s NOT Found\n", PageName.c_str(),TemplateName.c_str());
        }
    }
    else {
        QMessageBox::critical(Gui::getMainWindow(),
            QLatin1String("No template"),
            QLatin1String("Template file is invalid"));
    }
}

bool CmdTechDrawPageTemplate::isActive(void)
{
    return hasActiveDocument();
}

//===========================================================================
// TechDraw_RedrawPage
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawRedrawPage)

CmdTechDrawRedrawPage::CmdTechDrawRedrawPage()
  : Command("TechDraw_RedrawPage")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Redraw Page");
    sToolTipText    = sMenuText;
    sWhatsThis      = "TechDraw_RedrawPage";
    sStatusTip      = sToolTipText;
    sPixmap         = "actions/TechDraw_RedrawPage";
}

void CmdTechDrawRedrawPage::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    TechDraw::DrawPage* page = DrawGuiUtil::findPage(this);
    if (!page) {
        return;
    }
    Gui::WaitCursor wc;

    page->redrawCommand();
}

bool CmdTechDrawRedrawPage::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this,false);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_View
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawView)

CmdTechDrawView::CmdTechDrawView()
  : Command("TechDraw_View")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Insert View");
    sToolTipText    = QT_TR_NOOP("Insert a View");
    sWhatsThis      = "TechDraw_View";
    sStatusTip      = sToolTipText;
    sPixmap         = "actions/techdraw-View";
}

void CmdTechDrawView::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    TechDraw::DrawPage* page = DrawGuiUtil::findPage(this);
    if (!page) {
        return;
    }
    std::string PageName = page->getNameInDocument();

    //set projection direction from selected Face
    //use first object with a face selected
    std::vector<App::DocumentObject*> shapes;
    std::vector<App::DocumentObject*> xShapes;
    App::DocumentObject* partObj = nullptr;
    std::string faceName;
    int resolve = 1;                                //mystery
    bool single = false;                            //mystery
    auto selection = getSelection().getSelectionEx(0,
                                                   App::DocumentObject::getClassTypeId(),
                                                   resolve,
                                                   single);
    for (auto& sel: selection) {
        auto obj = sel.getObject();
        if (obj->isDerivedFrom(TechDraw::DrawPage::getClassTypeId()) ) {
            continue;
        }
        if ( obj->getDocument() != page->getDocument() ) {
            xShapes.push_back(obj);
            continue;
        }
        //not an external object and not null.  assume to be drawable.  Undrawables will be
        // skipped later.
        shapes.push_back(obj);
        if(partObj != nullptr) {
            continue;
        }
        //don't know if this works for an XLink
        for(auto& sub : sel.getSubNames()) {
            if (TechDraw::DrawUtil::getGeomTypeFromName(sub) == "Face") {
                faceName = sub;
                //
                partObj = obj;
                break;
            }
        }
    }

    if ( shapes.empty() &&
         xShapes.empty() ) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("No Shapes, Groups or Links in this selection"));
        return;
    }

    Base::Vector3d projDir;

    Gui::WaitCursor wc;
    openCommand(QT_TRANSLATE_NOOP("Command", "Create view"));
    std::string FeatName = getUniqueObjectName("View",page);
    Gui::cmdAppDocument(page,std::ostringstream() << "addObject('TechDraw::DrawViewPart','" << FeatName << "')");
    App::DocumentObject *docObj = getDocument()->getObject(FeatName.c_str());
    Gui::cmdAppObject(page, std::ostringstream() << "addView(" << getObjectCmd(docObj) << ")");

    TechDraw::DrawViewPart* dvp = dynamic_cast<TechDraw::DrawViewPart *>(docObj);
    if (!dvp) {
        throw Base::TypeError("CmdTechDrawView DVP not found\n");
    }
    dvp->Source.setValues(shapes);
    dvp->XSource.setValues(xShapes);
    auto dirs = faceName.size() ? DrawGuiUtil::getProjDirFromFace(partObj,faceName)
                                : DrawGuiUtil::get3DDirAndRot();
    projDir = dirs.first;
    {
        Base::ObjectStatusLocker<App::Document::Status, App::Document> lock(
                App::Document::Status::SkipRecompute, docObj->getDocument());
        Gui::cmdAppObjectArgs(docObj, "Direction = FreeCAD.Vector(%.3f,%.3f,%.3f)",
                    projDir.x,projDir.y,projDir.z);
        Gui::cmdAppObjectArgs(docObj, "XDirection = FreeCAD.Vector(%.3f,%.3f,%.3f)",
                    dirs.second.x,dirs.second.y,dirs.second.z);
    }
    updateActive();
    commitCommand();
}

bool CmdTechDrawView::isActive(void)
{
    return DrawGuiUtil::needPage(this);
}

//===========================================================================
// TechDraw_ActiveView
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawActiveView)

CmdTechDrawActiveView::CmdTechDrawActiveView()
  : Command("TechDraw_ActiveView")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Insert Active View (3D View)");
    sToolTipText    = sMenuText;
    sWhatsThis      = "TechDraw_ActiveView";
    sStatusTip      = sToolTipText;
    sPixmap         = "actions/TechDraw_ActiveView";
}

void CmdTechDrawActiveView::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    TechDraw::DrawPage* page = DrawGuiUtil::findPage(this);
    if (!page) {
        return;
    }
    std::string PageName = page->getNameInDocument();
    Gui::Control().showDialog(new TaskDlgActiveView(page));
}

bool CmdTechDrawActiveView::isActive(void)
{
    return DrawGuiUtil::needPage(this);
}

//===========================================================================
// TechDraw_SectionView
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawSectionView)

CmdTechDrawSectionView::CmdTechDrawSectionView()
  : Command("TechDraw_SectionView")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Insert Section View");
    sToolTipText    = sMenuText;
    sWhatsThis      = "TechDraw_SectionView";
    sStatusTip      = sToolTipText;
    sPixmap         = "actions/TechDraw_SectionView";
}

void CmdTechDrawSectionView::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    TechDraw::DrawPage* page = DrawGuiUtil::findPage(this);
    if (!page) {
        return;
    }

    std::vector<App::DocumentObject*> baseObj = getSelection().getObjectsOfType(TechDraw::DrawViewPart::getClassTypeId());
    if (baseObj.empty()) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select at least 1 DrawViewPart object as Base."));
        return;
    }
    TechDraw::DrawViewPart* dvp = static_cast<TechDraw::DrawViewPart*>(*baseObj.begin());
    Gui::Control().showDialog(new TaskDlgSectionView(dvp));

    updateActive();             //ok here since dialog doesn't call doc.recompute()
    commitCommand();
}

bool CmdTechDrawSectionView::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    bool taskInProgress = false;
    if (havePage) {
        taskInProgress = Gui::Control().activeDialog();
    }
    return (havePage && haveView && !taskInProgress);
}

//===========================================================================
// TechDraw_DetailView
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawDetailView)

CmdTechDrawDetailView::CmdTechDrawDetailView()
  : Command("TechDraw_DetailView")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Insert Detail View");
    sToolTipText    = sMenuText;
    sWhatsThis      = "TechDraw_DetailView";
    sStatusTip      = sToolTipText;
    sPixmap         = "actions/TechDraw_DetailView";
}

void CmdTechDrawDetailView::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    TechDraw::DrawPage* page = DrawGuiUtil::findPage(this);
    if (!page) {
        return;
    }

    std::vector<App::DocumentObject*> baseObj =  getSelection().
                            getObjectsOfType(TechDraw::DrawViewPart::getClassTypeId());
    if (baseObj.empty()) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select at least 1 DrawViewPart object as Base."));
        return;
    }
    TechDraw::DrawViewPart* dvp = static_cast<TechDraw::DrawViewPart*>(*(baseObj.begin()));

    Gui::Control().showDialog(new TaskDlgDetail(dvp));
}

bool CmdTechDrawDetailView::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    bool taskInProgress = false;
    if (havePage) {
        taskInProgress = Gui::Control().activeDialog();
    }
    return (havePage && haveView && !taskInProgress);
}

//===========================================================================
// TechDraw_ProjectionGroup
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawProjectionGroup)

CmdTechDrawProjectionGroup::CmdTechDrawProjectionGroup()
  : Command("TechDraw_ProjectionGroup")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Insert Projection Group");
    sToolTipText    = QT_TR_NOOP("Insert multiple linked views of drawable object(s)");
    sWhatsThis      = "TechDraw_ProjectionGroup";
    sStatusTip      = sToolTipText;
    sPixmap         = "actions/TechDraw_ProjectionGroup";
}

void CmdTechDrawProjectionGroup::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    TechDraw::DrawPage* page = DrawGuiUtil::findPage(this);
    if (!page) {
        return;
    }
    std::string PageName = page->getNameInDocument();
//    auto inlist = page->getInListEx(true);
//    inlist.insert(page);

    //set projection direction from selected Face
    //use first object with a face selected
    std::vector<App::DocumentObject*> shapes;
    std::vector<App::DocumentObject*> xShapes;
    App::DocumentObject* partObj = nullptr;
    std::string faceName;
    int resolve = 1;                                //mystery
    bool single = false;                            //mystery
    auto selection = getSelection().getSelectionEx(0,
                                                   App::DocumentObject::getClassTypeId(),
                                                   resolve,
                                                   single);
    for (auto& sel: selection) {
        auto obj = sel.getObject();
        if (obj->isDerivedFrom(TechDraw::DrawPage::getClassTypeId()) ) {
            continue;
        }
        if ( obj->getDocument() != page->getDocument() ) {
            xShapes.push_back(obj);
            continue;
        }
        //not an external object and not null.  assume to be drawable.  Undrawables will be
        // skipped later.
        shapes.push_back(obj);
        if(partObj != nullptr) {
            continue;
        }
        for(auto& sub : sel.getSubNames()) {
            if (TechDraw::DrawUtil::getGeomTypeFromName(sub) == "Face") {
                faceName = sub;
                partObj = obj;
                break;
            }
        }
    }
    if ( shapes.empty() &&
         xShapes.empty() ) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("No Shapes, Groups or Links in this selection"));
        return;
    }

    Base::Vector3d projDir;
    Gui::WaitCursor wc;

    openCommand(QT_TRANSLATE_NOOP("Command", "Create Projection Group"));
    std::string multiViewName = getUniqueObjectName("ProjGroup",page);
    Gui::cmdAppDocument(page, std::ostringstream() << "addObject('TechDraw::DrawProjGroup','" << multiViewName << "')");

    App::DocumentObject *docObj = getDocument()->getObject(multiViewName.c_str());
    auto multiView( static_cast<TechDraw::DrawProjGroup *>(docObj) );

    Gui::cmdAppObject(page, std::ostringstream() << "addView(" << getObjectCmd(multiView) << ")");

    multiView->Source.setValues(shapes);
    multiView->XSource.setValues(xShapes);
    Gui::cmdAppObject(multiView, "addProjection('Front')");

    auto dirs = faceName.size() ? DrawGuiUtil::getProjDirFromFace(partObj,faceName)
                                : DrawGuiUtil::get3DDirAndRot();
    {
        Base::ObjectStatusLocker<App::Document::Status, App::Document> lock(
                App::Document::Status::SkipRecompute, docObj->getDocument());
        Gui::cmdAppObjectArgs(docObj, "Anchor.Direction = FreeCAD.Vector(%.3f,%.3f,%.3f)",
                    dirs.first.x,dirs.first.y,dirs.first.z);
        Gui::cmdAppObjectArgs(docObj, "Anchor.XDirection = FreeCAD.Vector(%.3f,%.3f,%.3f)",
                    dirs.second.x,dirs.second.y,dirs.second.z);
        Gui::cmdAppObjectArgs(docObj, "Anchor.RotationVector = FreeCAD.Vector(%.3f,%.3f,%.3f)",
                    dirs.second.x,dirs.second.y,dirs.second.z);
    }
 
    updateActive();
    commitCommand();

    // create the rest of the desired views
    Gui::Control().showDialog(new TaskDlgProjGroup(multiView,true));
}

bool CmdTechDrawProjectionGroup::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool taskInProgress = false;
    if (havePage) {
        taskInProgress = Gui::Control().activeDialog();
    }
    return (havePage  && !taskInProgress);
}

//===========================================================================
// TechDraw_NewMulti  **deprecated**
//===========================================================================

//DEF_STD_CMD_A(CmdTechDrawNewMulti);

//CmdTechDrawNewMulti::CmdTechDrawNewMulti()
//  : Command("TechDraw_NewMulti")
//{
//    sAppModule      = "TechDraw";
//    sGroup          = QT_TR_NOOP("TechDraw");
//    sMenuText       = QT_TR_NOOP("Insert multi-part view in drawing");
//    sToolTipText    = QT_TR_NOOP("Insert a new View of a multiple Parts in the active drawing");
//    sWhatsThis      = "TechDraw_NewMulti";
//    sStatusTip      = sToolTipText;
//    sPixmap         = "actions/techdraw-multiview";
//}

//void CmdTechDrawNewMulti::activated(int iMsg)
//{
//    Q_UNUSED(iMsg);
//    TechDraw::DrawPage* page = DrawGuiUtil::findPage(this);
//    if (!page) {
//        return;
//    }

//    std::vector<App::DocumentObject*> shapes = getSelection().getObjectsOfType(App::DocumentObject::getClassTypeId());
//    if (shapes.empty()) {
//        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
//            QObject::tr("Can not  MultiView from this selection."));
//        return;
//    }

//    std::string PageName = page->getNameInDocument();

//    Gui::WaitCursor wc;

//    openCommand(QT_TRANSLATE_NOOP("Command", "Create view"));
//    std::string FeatName = getUniqueObjectName("MultiView");
//    doCommand(Doc,"App.activeDocument().addObject('TechDraw::DrawViewMulti','%s')",FeatName.c_str());
//    App::DocumentObject *docObj = getDocument()->getObject(FeatName.c_str());
//    auto multiView( static_cast<TechDraw::DrawViewMulti *>(docObj) );
//    multiView->Sources.setValues(shapes);
//    doCommand(Doc,"App.activeDocument().%s.addView(App.activeDocument().%s)",PageName.c_str(),FeatName.c_str());
//    updateActive();
//    commitCommand();
//}

//bool CmdTechDrawNewMulti::isActive(void)
//{
//    return DrawGuiUtil::needPage(this);
//}

//===========================================================================
// TechDraw_Balloon
//===========================================================================

//! common checks of Selection for Dimension commands
//non-empty selection, no more than maxObjs selected and at least 1 DrawingPage exists
bool _checkSelectionBalloon(Gui::Command* cmd, unsigned maxObjs) {
    std::vector<Gui::SelectionObject> selection = cmd->getSelection().getSelectionEx();
    if (selection.size() == 0) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Incorrect selection"),
                             QObject::tr("Select an object first"));
        return false;
    }

    const std::vector<std::string> SubNames = selection[0].getSubNames();
    if (SubNames.size() > maxObjs){
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Incorrect selection"),
            QObject::tr("Too many objects selected"));
        return false;
    }

    std::vector<App::DocumentObject*> pages = cmd->getDocument()->getObjectsOfType(TechDraw::DrawPage::getClassTypeId());
    if (pages.empty()){
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Incorrect selection"),
            QObject::tr("Create a page first."));
        return false;
    }
    return true;
}

bool _checkDrawViewPartBalloon(Gui::Command* cmd) {
    std::vector<Gui::SelectionObject> selection = cmd->getSelection().getSelectionEx();
    auto objFeat( dynamic_cast<TechDraw::DrawViewPart *>(selection[0].getObject()) );
    if( !objFeat ) {
        QMessageBox::warning( Gui::getMainWindow(),
                              QObject::tr("Incorrect selection"),
                              QObject::tr("No View of a Part in selection.") );
        return false;
    }
    return true;
}

bool _checkDirectPlacement(const QGIViewPart *viewPart, const std::vector<std::string> &subNames, QPointF &placement)
{
    // Let's see, if we can help speed up the placement of the balloon:
    // As of now we support:
    //     Single selected vertex: place the ballon tip end here
    //     Single selected edge:   place the ballon tip at its midpoint (suggested placement for e.g. chamfer dimensions)
    //
    // Single selected faces are currently not supported, but maybe we could in this case use the center of mass?

    if (subNames.size() != 1) {
        // If nothing or more than one subjects are selected, let the user decide, where to place the balloon
        return false;
    }

    std::string geoType = TechDraw::DrawUtil::getGeomTypeFromName(subNames[0]);
    if (geoType == "Vertex") {
        int index = TechDraw::DrawUtil::getIndexFromName(subNames[0]);
        TechDraw::Vertex *vertex = static_cast<DrawViewPart *>(viewPart->getViewObject())->getProjVertexByIndex(index);
        if (vertex) {
            placement = viewPart->mapToScene(Rez::guiX(vertex->x()), Rez::guiX(vertex->y()));
            return true;
        }
    }
    else if (geoType == "Edge") {
        int index = TechDraw::DrawUtil::getIndexFromName(subNames[0]);
        TechDraw::BaseGeom *geo = static_cast<DrawViewPart *>(viewPart->getViewObject())->getGeomByIndex(index);
        if (geo) {
            Base::Vector3d midPoint(Rez::guiX(geo->getMidPoint()));
            placement = viewPart->mapToScene(midPoint.x, midPoint.y);
            return true;
        }
    }

    return false;
}

DEF_STD_CMD_A(CmdTechDrawBalloon)

CmdTechDrawBalloon::CmdTechDrawBalloon()
  : Command("TechDraw_Balloon")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Insert Balloon Annotation");
    sToolTipText    = sMenuText;
    sWhatsThis      = "TechDraw_Balloon";
    sStatusTip      = sToolTipText;
    sPixmap         = "TechDraw_Balloon";
}

void CmdTechDrawBalloon::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    bool result = _checkSelectionBalloon(this,1);
    if (!result)
        return;
    result = _checkDrawViewPartBalloon(this);
    if (!result)
        return;

    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();

    auto objFeat( dynamic_cast<TechDraw::DrawViewPart *>(selection[0].getObject()) );
    if( objFeat == nullptr ) {
        return;
    }

    TechDraw::DrawPage* page = objFeat->findParentPage();
    
    page->balloonParent = objFeat;

    Gui::Document *guiDoc = Gui::Application::Instance->getDocument(page->getDocument());
    ViewProviderPage *pageVP = dynamic_cast<ViewProviderPage *>(guiDoc->getViewProvider(page));
    ViewProviderViewPart *partVP = dynamic_cast<ViewProviderViewPart *>(guiDoc->getViewProvider(objFeat));

    if (pageVP && partVP) {
        QGVPage *viewPage = pageVP->getGraphicsView();
        if (viewPage) {
            viewPage->startBalloonPlacing();

            QGIViewPart *viewPart = dynamic_cast<QGIViewPart *>(partVP->getQView());
            QPointF placement;
            if (viewPart && _checkDirectPlacement(viewPart, selection[0].getSubNames(), placement)) {
                viewPage->createBalloon(placement, objFeat);
            }
        }
    }
}

bool CmdTechDrawBalloon::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_ClipGroup
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawClipGroup)

CmdTechDrawClipGroup::CmdTechDrawClipGroup()
  : Command("TechDraw_ClipGroup")
{
    // setting the
    sGroup        = QT_TR_NOOP("TechDraw");
    sMenuText     = QT_TR_NOOP("Insert Clip Group");
    sToolTipText  = sMenuText;
    sWhatsThis    = "TechDraw_ClipGroup";
    sStatusTip    = sToolTipText;
    sPixmap       = "actions/TechDraw_ClipGroup";
}

void CmdTechDrawClipGroup::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    TechDraw::DrawPage* page = DrawGuiUtil::findPage(this);
    if (!page) {
        return;
    }
    std::string PageName = page->getNameInDocument();

    std::string FeatName = getUniqueObjectName("Clip",page);
    openCommand(QT_TRANSLATE_NOOP("Command", "Create Clip"));
    Gui::cmdAppDocument(page, std::ostringstream() << "addObject('TechDraw::DrawViewClip','" << FeatName << "')");
    Gui::cmdAppObjectArgs(page, "addView(%s))", getObjectCmd(page));
    updateActive();
    commitCommand();
}

bool CmdTechDrawClipGroup::isActive(void)
{
    return DrawGuiUtil::needPage(this);
}

//===========================================================================
// TechDraw_ClipGroupAdd
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawClipGroupAdd)

CmdTechDrawClipGroupAdd::CmdTechDrawClipGroupAdd()
  : Command("TechDraw_ClipGroupAdd")
{
    sGroup        = QT_TR_NOOP("TechDraw");
    sMenuText     = QT_TR_NOOP("Add View to Clip Group");
    sToolTipText  = sMenuText;
    sWhatsThis    = "TechDraw_ClipGroupAdd";
    sStatusTip    = sToolTipText;
    sPixmap       = "actions/TechDraw_ClipGroupAdd";
}

void CmdTechDrawClipGroupAdd::activated(int iMsg)
{
    Q_UNUSED(iMsg);
   std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();
   if (selection.size() != 2) {
       QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                            QObject::tr("Select one Clip group and one View."));
       return;
   }

    TechDraw::DrawViewClip* clip = 0;
    TechDraw::DrawView* view = 0;
    std::vector<Gui::SelectionObject>::iterator itSel = selection.begin();
    for (; itSel != selection.end(); itSel++)  {
        if ((*itSel).getObject()->isDerivedFrom(TechDraw::DrawViewClip::getClassTypeId())) {
            clip = static_cast<TechDraw::DrawViewClip*>((*itSel).getObject());
        } else if ((*itSel).getObject()->isDerivedFrom(TechDraw::DrawView::getClassTypeId())) {
            view = static_cast<TechDraw::DrawView*>((*itSel).getObject());
        }
    }
    if (!view) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select exactly one View to add to group."));
        return;
    }
    if (!clip) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select exactly one Clip group."));
        return;
    }

    TechDraw::DrawPage* pageClip = clip->findParentPage();
    TechDraw::DrawPage* pageView = view->findParentPage();

    if (pageClip != pageView) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Clip and View must be from same Page."));
        return;
    }

    openCommand(QT_TRANSLATE_NOOP("Command", "ClipGroupAdd"));
    Gui::cmdAppObjectHide(view);
    Gui::cmdAppObject(pageClip, std::ostringstream() << "addView(" << getObjectCmd(view) << ")");
    Gui::cmdAppObjectShow(view);
    updateActive();
    commitCommand();
}

bool CmdTechDrawClipGroupAdd::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveClip = false;
    if (havePage) {
        auto drawClipType( TechDraw::DrawViewClip::getClassTypeId() );
        auto selClips = getDocument()->getObjectsOfType(drawClipType);
        if (!selClips.empty()) {
            haveClip = true;
        }
    }
    return (havePage && haveClip);
}

//===========================================================================
// TechDraw_ClipGroupRemove
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawClipGroupRemove)

CmdTechDrawClipGroupRemove::CmdTechDrawClipGroupRemove()
  : Command("TechDraw_ClipGroupRemove")
{
    sGroup        = QT_TR_NOOP("TechDraw");
    sMenuText     = QT_TR_NOOP("Remove View from Clip Group");
    sToolTipText  = sMenuText;
    sWhatsThis    = "TechDraw_ClipGroupRemove";
    sStatusTip    = sToolTipText;
    sPixmap       = "actions/TechDraw_ClipGroupRemove";
}

void CmdTechDrawClipGroupRemove::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    auto dObj( getSelection().getObjectsOfType(TechDraw::DrawView::getClassTypeId()) );
    if (dObj.empty()) {
        QMessageBox::warning( Gui::getMainWindow(),
                              QObject::tr("Wrong selection"),
                              QObject::tr("Select exactly one View to remove from Group.") );
        return;
    }

    auto view( static_cast<TechDraw::DrawView*>(dObj.front()) );

    TechDraw::DrawPage* page = view->findParentPage();
    const std::vector<App::DocumentObject*> pViews = page->Views.getValues();
    TechDraw::DrawViewClip *clip(nullptr);
    for (auto &v : pViews) {
        clip = dynamic_cast<TechDraw::DrawViewClip*>(v);
        if (clip && clip->isViewInClip(view)) {
            break;
        }
        clip = nullptr;
    }

    if (!clip) {
        QMessageBox::warning( Gui::getMainWindow(),
                              QObject::tr("Wrong selection"),
                              QObject::tr("View does not belong to a Clip") );
        return;
    }

    openCommand("ClipGroupRemove");
    Gui::cmdAppObjectHide(view);
    Gui::cmdAppObject(clip, std::ostringstream() << "removeView(" << getObjectCmd(view) << ")");
    Gui::cmdAppObjectShow(view);
    updateActive();
    commitCommand();
}

bool CmdTechDrawClipGroupRemove::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveClip = false;
    if (havePage) {
        auto drawClipType( TechDraw::DrawViewClip::getClassTypeId() );
        auto selClips = getDocument()->getObjectsOfType(drawClipType);
        if (!selClips.empty()) {
            haveClip = true;
        }
    }
    return (havePage && haveClip);
}


//===========================================================================
// TechDraw_Symbol
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawSymbol)

CmdTechDrawSymbol::CmdTechDrawSymbol()
  : Command("TechDraw_Symbol")
{
    // setting the Gui eye-candy
    sGroup        = QT_TR_NOOP("TechDraw");
    sMenuText     = QT_TR_NOOP("Insert SVG Symbol");
    sToolTipText  = QT_TR_NOOP("Insert symbol from a SVG file");
    sWhatsThis    = "TechDraw_Symbol";
    sStatusTip    = sToolTipText;
    sPixmap       = "actions/TechDraw_Symbol";
}

void CmdTechDrawSymbol::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    TechDraw::DrawPage* page = DrawGuiUtil::findPage(this);
    if (!page) {
        return;
    }
    std::string PageName = page->getNameInDocument();

    // Reading an image
    QString filename = Gui::FileDialog::getOpenFileName(Gui::getMainWindow(),
        QObject::tr("Choose an SVG file to open"), QString::null,
        QString::fromLatin1("%1 (*.svg *.svgz);;%2 (*.*)").
        arg(QObject::tr("Scalable Vector Graphic")).
        arg(QObject::tr("All Files")));

    if (!filename.isEmpty())
    {
        std::string FeatName = getUniqueObjectName("Symbol",page);
        filename = Base::Tools::escapeEncodeFilename(filename);
        openCommand(QT_TRANSLATE_NOOP("Command", "Create Symbol"));
        doCommand(Doc,"f = open(\"%s\",'r')",(const char*)filename.toUtf8());
        doCommand(Doc,"svg = f.read()");
        doCommand(Doc,"f.close()");

        Gui::cmdAppDocument(page, std::ostringstream() << "addObject('TechDraw::DrawViewSymbol','" << FeatName << "')");
        auto feat = page->getDocument()->getObject(FeatName.c_str());
        Gui::cmdAppObject(feat,"Symbol = svg");
        Gui::cmdAppObject(page, std::ostringstream() << "addView(" << getObjectCmd(feat) << ")");
        updateActive();
        commitCommand();
    }
}

bool CmdTechDrawSymbol::isActive(void)
{
    return DrawGuiUtil::needPage(this);
}

//===========================================================================
// TechDraw_DraftView
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawDraftView)

CmdTechDrawDraftView::CmdTechDrawDraftView()
  : Command("TechDraw_DraftView")
{
    // setting the Gui eye-candy
    sGroup        = QT_TR_NOOP("TechDraw");
    sMenuText     = QT_TR_NOOP("Insert Draft Workbench Object");
    sToolTipText  = QT_TR_NOOP("Insert a View of a Draft Workbench object");
    sWhatsThis    = "TechDraw_NewDraft";
    sStatusTip    = sToolTipText;
    sPixmap       = "actions/techdraw-DraftView";
}

void CmdTechDrawDraftView::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    TechDraw::DrawPage* page = DrawGuiUtil::findPage(this);
    if (!page) {
        return;
    }
    std::vector<App::DocumentObject*> objects = getSelection().
                                            getObjectsOfType(App::DocumentObject::getClassTypeId());

    if (objects.empty()) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select at least one object."));
        return;
    }

    openCommand(QT_TRANSLATE_NOOP("Command", "Create DraftView"));

    std::pair<Base::Vector3d,Base::Vector3d> dirs = DrawGuiUtil::get3DDirAndRot();

    int draftItemsFound = 0;
    for (std::vector<App::DocumentObject*>::iterator it = objects.begin(); it != objects.end(); ++it) {
        if (DrawGuiUtil::isDraftObject((*it)))  {
            draftItemsFound++;
            std::string FeatName = getUniqueObjectName("DraftView",page);
            Gui::cmdAppDocument(page, std::ostringstream() << "addObject('TechDraw::DrawViewDraft','" << FeatName << "')");
            auto feat = page->getDocument()->getObject(FeatName.c_str());
            Gui::cmdAppObject(feat, std::ostringstream() << "Source = " << getObjectCmd(*it));
            Gui::cmdAppObject(page, std::ostringstream() << "addView(" << getObjectCmd(feat) << ")");
            Gui::cmdAppObjectArgs(feat, "Direction = FreeCAD.Vector(%.3f,%.3f,%.3f)",
                    dirs.first.x, dirs.first.y, dirs.first.z);
        }
    }

    updateActive();
    commitCommand();

    if (draftItemsFound == 0) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("There were no DraftWB objects in the selection."));
    }
}

bool CmdTechDrawDraftView::isActive(void)
{
    return DrawGuiUtil::needPage(this);
}

//===========================================================================
// TechDraw_ArchView
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawArchView)

CmdTechDrawArchView::CmdTechDrawArchView()
  : Command("TechDraw_ArchView")
{
    // setting the Gui eye-candy
    sGroup        = QT_TR_NOOP("TechDraw");
    sMenuText     = QT_TR_NOOP("Insert Arch Workbench Object");
    sToolTipText  = QT_TR_NOOP("Insert a View of a Section Plane from Arch Workbench");
    sWhatsThis    = "TechDraw_NewArch";
    sStatusTip    = sToolTipText;
    sPixmap       = "actions/techdraw-ArchView";
}

void CmdTechDrawArchView::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    TechDraw::DrawPage* page = DrawGuiUtil::findPage(this);
    if (!page) {
        return;
    }

    const std::vector<App::DocumentObject*> objects = getSelection().getObjectsOfType(App::DocumentObject::getClassTypeId());
    App::DocumentObject* archObject = nullptr;
    int archCount = 0;
    for (auto& obj : objects) {
        if (DrawGuiUtil::isArchSection(obj) ) {
            archCount++;
            archObject = obj;
        }
    }
    if ( archCount > 1 ) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Please select only 1 Arch Section."));
        return;
    }

    if (archObject == nullptr) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("No Arch Sections in selection."));
        return;
    }

    std::string FeatName = getUniqueObjectName("ArchView");
    openCommand(QT_TRANSLATE_NOOP("Command", "Create ArchView"));
    Gui::cmdAppDocument(page, std::ostringstream() << "addObject('TechDraw::DrawViewArch','" << FeatName << "')");
    auto feat = page->getDocument()->getObject(FeatName.c_str());
    Gui::cmdAppObject(feat, std::ostringstream() << "Source = " << getObjectCmd(archObject));
    Gui::cmdAppObject(page, std::ostringstream() << "addView(" << getObjectCmd(feat) << ")");
    updateActive();
    commitCommand();
}

bool CmdTechDrawArchView::isActive(void)
{
    return DrawGuiUtil::needPage(this);
}

//===========================================================================
// TechDraw_SpreadsheetView
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawSpreadsheetView)

CmdTechDrawSpreadsheetView::CmdTechDrawSpreadsheetView()
  : Command("TechDraw_SpreadsheetView")
{
    // setting the
    sGroup        = QT_TR_NOOP("TechDraw");
    sMenuText     = QT_TR_NOOP("Insert Spreadsheet View");
    sToolTipText  = QT_TR_NOOP("Insert View to a spreadsheet");
    sWhatsThis    = "TechDraw_SpreadsheetView";
    sStatusTip    = sToolTipText;
    sPixmap       = "actions/TechDraw_SpreadsheetView";
}

void CmdTechDrawSpreadsheetView::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    const std::vector<App::DocumentObject*> spreads = getSelection().getObjectsOfType(Spreadsheet::Sheet::getClassTypeId());
    if (spreads.size() != 1) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select exactly one Spreadsheet object."));
        return;
    }

    TechDraw::DrawPage* page = DrawGuiUtil::findPage(this);
    if (!page) {
        return;
    }

    openCommand(QT_TRANSLATE_NOOP("Command", "Create spreadsheet view"));
    std::string FeatName = getUniqueObjectName("Sheet",page);
    Gui::cmdAppDocument(page, std::ostringstream() << "addObject('TechDraw::DrawViewSpreadsheet','" << FeatName << "')");
    auto feat = page->getDocument()->getObject(FeatName.c_str());
    Gui::cmdAppObject(feat, std::ostringstream() << "Source = " << getObjectCmd(spreads.front()));
    Gui::cmdAppObject(page, std::ostringstream() << "addView(" << getObjectCmd(feat) << ")");
    updateActive();
    commitCommand();
}

bool CmdTechDrawSpreadsheetView::isActive(void)
{
    //need a Page and a SpreadSheet::Sheet
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveSheet = false;
    if (havePage) {
        auto spreadSheetType( Spreadsheet::Sheet::getClassTypeId() );
        auto selSheets = getDocument()->getObjectsOfType(spreadSheetType);
        if (!selSheets.empty()) {
            haveSheet = true;
        }
    }
    return (havePage && haveSheet);
}


//===========================================================================
// TechDraw_ExportPageSVG
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawExportPageSVG)

CmdTechDrawExportPageSVG::CmdTechDrawExportPageSVG()
  : Command("TechDraw_ExportPageSVG")
{
    sGroup        = QT_TR_NOOP("File");
    sMenuText     = QT_TR_NOOP("Export Page as SVG");
    sToolTipText  = sMenuText;
    sWhatsThis    = "TechDraw_ExportPageSVG";
    sStatusTip    = sToolTipText;
    sPixmap       = "actions/TechDraw_ExportPageSVG";
}

void CmdTechDrawExportPageSVG::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    TechDraw::DrawPage* page = DrawGuiUtil::findPage(this);
    if (!page) {
        return;
    }
    Gui::Document* activeGui = Gui::Application::Instance->getDocument(page->getDocument());
    Gui::ViewProvider* vp = activeGui->getViewProvider(page);
    ViewProviderPage* dvp = dynamic_cast<ViewProviderPage*>(vp);

    if (dvp  && dvp->getMDIViewPage()) {
        dvp->getMDIViewPage()->saveSVG();
    } else {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("No Drawing View"),
            QObject::tr("Open Drawing View before attempting export to SVG."));
        return;
    }
}

bool CmdTechDrawExportPageSVG::isActive(void)
{
    return DrawGuiUtil::needPage(this);
}

//===========================================================================
// TechDraw_ExportPageDXF
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawExportPageDXF)

CmdTechDrawExportPageDXF::CmdTechDrawExportPageDXF()
  : Command("TechDraw_ExportPageDXF")
{
    sGroup        = QT_TR_NOOP("File");
    sMenuText     = QT_TR_NOOP("Export Page as DXF");
    sToolTipText  = sMenuText;
    sWhatsThis    = "TechDraw_ExportPageDXF";
    sStatusTip    = sToolTipText;
    sPixmap       = "actions/TechDraw_ExportPageDXF";
}

static inline QString _getDefaultName(const App::DocumentObject *obj) {
    if(!obj)
        return QString();
    if(obj->getDocument()
           && obj->getDocument()->FileName.getStrValue().size())
    {
        QDir dir = QFileInfo(QString::fromUtf8(
                    obj->getDocument()->FileName.getValue())).dir();
        return QFileInfo(dir,QString::fromUtf8(obj->Label.getValue())).filePath();
    } else
        return QString::fromUtf8(obj->Label.getValue());
}

void CmdTechDrawExportPageDXF::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    TechDraw::DrawPage* page = DrawGuiUtil::findPage(this);
    if (!page) {
        return;
    }

    std::vector<App::DocumentObject*> views = page->Views.getValues();
    for (auto& v: views) {
        if (v->isDerivedFrom(TechDraw::DrawViewArch::getClassTypeId())) {
            QMessageBox::StandardButton rc =
                QMessageBox::question(Gui::getMainWindow(), QObject::tr("Can not export selection"),
                            QObject::tr("Page contains DrawViewArch which will not be exported. Continue?"),
                            QMessageBox::StandardButtons(QMessageBox::Yes| QMessageBox::No));
            if (rc == QMessageBox::No) {
                return;
            } else {
                break;
            }
        }
    }

//WF? allow more than one TD Page per Dxf file??  1 TD page = 1 DXF file = 1 drawing?
    QString fileName = Gui::FileDialog::getSaveFileName(Gui::getMainWindow(),
                                                   QString::fromUtf8(QT_TR_NOOP("Save Dxf File ")),
                                                   _getDefaultName(page),
                                                   QString::fromUtf8(QT_TR_NOOP("Dxf (*.dxf)")));

    if (fileName.isEmpty()) {
        return;
    }

    openCommand(QT_TRANSLATE_NOOP("Command", "Save page to dxf"));
    doCommand(Doc,"import TechDraw");
    fileName = Base::Tools::escapeEncodeFilename(fileName);
    doCommand(Doc,"TechDraw.writeDXFPage(%s,u'%s')",
            getObjectCmd(page).c_str(), fileName.toUtf8().constData());
    commitCommand();
}


bool CmdTechDrawExportPageDXF::isActive(void)
{
    return DrawGuiUtil::needPage(this);
}

void CreateTechDrawCommands(void)
{
    Gui::CommandManager &rcCmdMgr = Gui::Application::Instance->commandManager();

    rcCmdMgr.addCommand(new CmdTechDrawPageDefault());
    rcCmdMgr.addCommand(new CmdTechDrawPageTemplate());
    rcCmdMgr.addCommand(new CmdTechDrawRedrawPage());
    rcCmdMgr.addCommand(new CmdTechDrawView());
    rcCmdMgr.addCommand(new CmdTechDrawActiveView());
    rcCmdMgr.addCommand(new CmdTechDrawSectionView());
    rcCmdMgr.addCommand(new CmdTechDrawDetailView());
    rcCmdMgr.addCommand(new CmdTechDrawProjectionGroup());
    rcCmdMgr.addCommand(new CmdTechDrawClipGroup());
    rcCmdMgr.addCommand(new CmdTechDrawClipGroupAdd());
    rcCmdMgr.addCommand(new CmdTechDrawClipGroupRemove());
    rcCmdMgr.addCommand(new CmdTechDrawSymbol());
    rcCmdMgr.addCommand(new CmdTechDrawExportPageSVG());
    rcCmdMgr.addCommand(new CmdTechDrawExportPageDXF());
    rcCmdMgr.addCommand(new CmdTechDrawDraftView());
    rcCmdMgr.addCommand(new CmdTechDrawArchView());
    rcCmdMgr.addCommand(new CmdTechDrawSpreadsheetView());
    rcCmdMgr.addCommand(new CmdTechDrawBalloon());
}
