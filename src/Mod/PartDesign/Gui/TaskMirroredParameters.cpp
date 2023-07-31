/******************************************************************************
 *   Copyright (c) 2012 Jan Rheinländer <jrheinlaender@users.sourceforge.net> *
 *                                                                            *
 *   This file is part of the FreeCAD CAx development system.                 *
 *                                                                            *
 *   This library is free software; you can redistribute it and/or            *
 *   modify it under the terms of the GNU Library General Public              *
 *   License as published by the Free Software Foundation; either             *
 *   version 2 of the License, or (at your option) any later version.         *
 *                                                                            *
 *   This library  is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *   GNU Library General Public License for more details.                     *
 *                                                                            *
 *   You should have received a copy of the GNU Library General Public        *
 *   License along with this library; see the file COPYING.LIB. If not,       *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,            *
 *   Suite 330, Boston, MA  02111-1307, USA                                   *
 *                                                                            *
 ******************************************************************************/


#include "PreCompiled.h"

#ifndef _PreComp_
# include <QAction>
# include <QMessageBox>
#endif

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/Origin.h>
#include <Base/Console.h>
#include <Base/ExceptionSafeCall.h>
#include <Base/Tools.h>
#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/Selection.h>
#include <Gui/ViewProviderOrigin.h>
#include <Mod/PartDesign/App/Body.h>
#include <Mod/PartDesign/App/FeatureMirrored.h>

#include "ui_TaskMirroredParameters.h"
#include "TaskMirroredParameters.h"
#include "ReferenceSelection.h"
#include "TaskMultiTransformParameters.h"

using namespace PartDesignGui;
using namespace Gui;

/* TRANSLATOR PartDesignGui::TaskMirroredParameters */

TaskMirroredParameters::TaskMirroredParameters(ViewProviderTransformed *TransformedView, QWidget *parent)
    : TaskTransformedParameters(TransformedView, parent)
    , ui(new Ui_TaskMirroredParameters)
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui->setupUi(proxy);

    this->groupLayout()->addWidget(proxy);

    ui->buttonOK->hide();
    ui->checkBoxUpdateView->setEnabled(true);

    selectionMode = none;

    blockUpdate = false; // Hack, sometimes it is NOT false although set to false in Transformed::Transformed()!!
    setupUI();
}

TaskMirroredParameters::TaskMirroredParameters(TaskMultiTransformParameters *parentTask, QLayout *layout)
        : TaskTransformedParameters(parentTask), ui(new Ui_TaskMirroredParameters)
{
    proxy = new QWidget(parentTask);
    ui->setupUi(proxy);
    connect(ui->buttonOK, &QToolButton::pressed,
            parentTask, &TaskMirroredParameters::onSubTaskButtonOK);

    layout->addWidget(proxy);

    ui->buttonOK->setEnabled(true);
    ui->checkBoxUpdateView->hide();

    selectionMode = none;

    blockUpdate = false; // Hack, sometimes it is NOT false although set to false in Transformed::Transformed()!!
    setupUI();
}

void TaskMirroredParameters::setupUI()
{
    setupBaseUI();

    this->planeLinks.setCombo(*(ui->comboPlane));
    ui->comboPlane->setEnabled(true);

    App::DocumentObject* sketch = getSketchObject();
    if (sketch && sketch->isDerivedFrom(Part::Part2DObject::getClassTypeId())) {
        this->fillPlanesCombo(planeLinks,static_cast<Part::Part2DObject*>(sketch));
    }
    else {
        this->fillPlanesCombo(planeLinks, nullptr);
    }

    //show the parts coordinate system planes for selection
    PartDesign::Body * body = PartDesign::Body::findBodyOf ( getObject() );
    if(body) {
        try {
            App::Origin *origin = body->getOrigin();
            ViewProviderOrigin* vpOrigin;
            vpOrigin = static_cast<ViewProviderOrigin*>(Gui::Application::Instance->getViewProvider(origin));
            vpOrigin->setTemporaryVisibility(false, true);
        } catch (const Base::Exception &ex) {
            Base::Console().Error ("%s\n", ex.what () );
        }
    }

    updateUI();

    Base::connect(ui->comboPlane, qOverload<int>(&QComboBox::activated),
            this, &TaskMirroredParameters::onPlaneChanged);
    Base::connect(ui->checkBoxUpdateView, &QCheckBox::toggled,
            this, &TaskMirroredParameters::onUpdateView);
}

void TaskMirroredParameters::updateUI()
{
    Base::StateLocker lock(blockUpdate);

    PartDesign::Mirrored* pcMirrored = static_cast<PartDesign::Mirrored*>(getObject());

    if (planeLinks.setCurrentLink(pcMirrored->MirrorPlane) == -1){
        //failed to set current, because the link isn't in the list yet
        planeLinks.addLink(pcMirrored->MirrorPlane, getRefStr(pcMirrored->MirrorPlane.getValue(),pcMirrored->MirrorPlane.getSubValues()));
        planeLinks.setCurrentLink(pcMirrored->MirrorPlane);
    }
}

void TaskMirroredParameters::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if (selectionMode!=none && msg.Type == Gui::SelectionChanges::AddSelection) {

        std::vector<std::string> mirrorPlanes;
        App::DocumentObject* selObj;
        PartDesign::Mirrored* pcMirrored = static_cast<PartDesign::Mirrored*>(getObject());
        getReferencedSelection(pcMirrored, msg, selObj, mirrorPlanes);
        if (!selObj)
                return;
        
        if ( selectionMode == reference || selObj->isDerivedFrom ( App::Plane::getClassTypeId () ) ) {
            setupTransaction();
            pcMirrored->MirrorPlane.setValue(selObj, mirrorPlanes);
            recomputeFeature();
            updateUI();
        }
        exitSelectionMode();
        return;
    }

    TaskTransformedParameters::onSelectionChanged(msg);
}

void TaskMirroredParameters::onPlaneChanged(int /*num*/)
{
    if (blockUpdate)
        return;
    setupTransaction();
    PartDesign::Mirrored* pcMirrored = static_cast<PartDesign::Mirrored*>(getObject());
    try{
        if (!planeLinks.getCurrentLink().getValue()) {
            // enter reference selection mode
            selectionMode = reference;
            Gui::Selection().clearSelection();
            addReferenceSelectionGate(AllowSelection::FACE | AllowSelection::PLANAR);
        } else {
            exitSelectionMode();
            pcMirrored->MirrorPlane.Paste(planeLinks.getCurrentLink());
        }
    } catch (Base::Exception &e) {
        QMessageBox::warning(nullptr,tr("Error"),QApplication::translate("Exception", e.what()));
    }

    recomputeFeature();
}

void TaskMirroredParameters::onUpdateView(bool on)
{
    blockUpdate = !on;
    if (on) {
        setupTransaction();
        // Do the same like in TaskDlgMirroredParameters::accept() but without doCommand
        PartDesign::Mirrored* pcMirrored = static_cast<PartDesign::Mirrored*>(getObject());
        std::vector<std::string> mirrorPlanes;
        App::DocumentObject* obj;

        getMirrorPlane(obj, mirrorPlanes);
        pcMirrored->MirrorPlane.setValue(obj,mirrorPlanes);

        recomputeFeature();
    }
}

void TaskMirroredParameters::getMirrorPlane(App::DocumentObject*& obj, std::vector<std::string> &sub) const
{
    const App::PropertyLinkSub &lnk = planeLinks.getCurrentLink();
    obj = lnk.getValue();
    sub = lnk.getSubValues();
}

void TaskMirroredParameters::apply()
{
}

TaskMirroredParameters::~TaskMirroredParameters()
{
    //hide the parts coordinate system axis for selection
    try {
        PartDesign::Body * body = PartDesign::Body::findBodyOf ( getObject() );
        if ( body ) {
            App::Origin *origin = body->getOrigin();
            ViewProviderOrigin* vpOrigin;
            vpOrigin = static_cast<ViewProviderOrigin*>(Gui::Application::Instance->getViewProvider(origin));
            vpOrigin->resetTemporaryVisibility();
        }
    } catch (const Base::Exception &ex) {
        Base::Console().Error ("%s\n", ex.what () );
    }

    if (proxy)
        delete proxy;
}

void TaskMirroredParameters::changeEvent(QEvent *e)
{
    TaskBox::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(proxy);
    }
}

//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgMirroredParameters::TaskDlgMirroredParameters(ViewProviderMirrored *MirroredView)
    : TaskDlgTransformedParameters(MirroredView, new TaskMirroredParameters(MirroredView))
{
}
//==== calls from the TaskView ===============================================================

bool TaskDlgMirroredParameters::accept()
{
    TaskMirroredParameters* mirrorParameter = static_cast<TaskMirroredParameters*>(parameter);
    std::vector<std::string> mirrorPlanes;
    App::DocumentObject* obj;
    mirrorParameter->getMirrorPlane(obj, mirrorPlanes);
    std::string mirrorPlane = buildLinkSingleSubPythonStr(obj, mirrorPlanes);

    FCMD_OBJ_CMD(vp->getObject(),"MirrorPlane = " << mirrorPlane);

    return TaskDlgTransformedParameters::accept();
}

#include "moc_TaskMirroredParameters.cpp"
