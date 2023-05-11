/***************************************************************************
 *   Copyright (c) 2011 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
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

#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/Origin.h>
#include <Base/Console.h>
#include <Base/ExceptionSafeCall.h>
#include <Base/Tools.h>
#include <Gui/Application.h>
#include <Gui/CommandT.h>
#include <Gui/Selection.h>
#include <Gui/ViewProvider.h>
#include <Gui/ViewProviderOrigin.h>
#include <Mod/PartDesign/App/FeatureRevolution.h>
#include <Mod/PartDesign/App/FeatureGroove.h>
#include <Mod/PartDesign/App/Body.h>

#include "ui_TaskRevolutionParameters.h"
#include "TaskRevolutionParameters.h"
#include "ReferenceSelection.h"
#include "Utils.h"

using namespace PartDesignGui;
using namespace Gui;

/* TRANSLATOR PartDesignGui::TaskRevolutionParameters */

TaskRevolutionParameters::TaskRevolutionParameters(PartDesignGui::ViewProvider* RevolutionView, QWidget *parent)
    : TaskSketchBasedParameters(RevolutionView, parent, "PartDesign_Revolution", tr("Revolution parameters")),
      ui(new Ui_TaskRevolutionParameters),
      proxy(new QWidget(this))
{
    // we need a separate container widget to add all controls to
    ui->setupUi(proxy);

    ui->axis->setMouseTracking(true);
    ui->axis->installEventFilter(this);

    this->initUI(proxy);
    this->groupLayout()->addWidget(proxy);

    PartDesign::ProfileBased* pcFeat = static_cast<PartDesign::ProfileBased*>(vp->getObject());
    if (pcFeat->isDerivedFrom(PartDesign::Revolution::getClassTypeId())) {
        ui->revolveAngle->bind(static_cast<PartDesign::Revolution *>(pcFeat)->Angle);
    } else if (pcFeat->isDerivedFrom(PartDesign::Groove::getClassTypeId())) {
        ui->revolveAngle->bind(static_cast<PartDesign::Groove *> (pcFeat)->Angle);
    }

    refresh();
    setFocus ();

    //show the parts coordinate system axis for selection
    PartDesign::Body * body = PartDesign::Body::findBodyOf ( vp->getObject () );
    if(body) {
        try {
            App::Origin *origin = body->getOrigin();
            ViewProviderOrigin* vpOrigin;
            vpOrigin = static_cast<ViewProviderOrigin*>(Gui::Application::Instance->getViewProvider(origin));
            vpOrigin->setTemporaryVisibility(true, false);
        } catch (const Base::Exception &ex) {
            ex.ReportException();
        }
    }

    onAxisButton(true);

    connectSignals();
}

void TaskRevolutionParameters::onAxisButton(bool checked)
{
    if (checked) {
        AllowSelectionFlags conf;
        conf.setFlag(AllowSelection::EDGE);
        conf.setFlag(AllowSelection::FACE, false);
        conf.setFlag(AllowSelection::PLANAR);
        conf.setFlag(AllowSelection::CIRCLE);
        TaskSketchBasedParameters::onSelectReference(ui->buttonAxis, conf);
    } else
        exitSelectionMode();
}

void TaskRevolutionParameters::onSelectionModeChanged(SelectionMode)
{
    if (getSelectionMode() == SelectionMode::refAdd) {
        ui->buttonAxis->setChecked(true);
    } else {
        ui->buttonAxis->setChecked(false);
    }
}

void TaskRevolutionParameters::refresh()
{
    if (!vp || !vp->getObject())
        return;

    // Temporarily prevent unnecessary feature recomputes
    for (QWidget* child : proxy->findChildren<QWidget*>())
        child->blockSignals(true);

    //bind property mirrors
    PartDesign::ProfileBased* pcFeat = static_cast<PartDesign::ProfileBased*>(vp->getObject());
    if (pcFeat->isDerivedFrom(PartDesign::Revolution::getClassTypeId())) {
        PartDesign::Revolution* rev = static_cast<PartDesign::Revolution*>(vp->getObject());
        this->propAngle = &(rev->Angle);
        this->propMidPlane = &(rev->Midplane);
        this->propReferenceAxis = &(rev->ReferenceAxis);
        this->propReversed = &(rev->Reversed);
    } else {
        assert(pcFeat->isDerivedFrom(PartDesign::Groove::getClassTypeId()));
        PartDesign::Groove* rev = static_cast<PartDesign::Groove*>(vp->getObject());
        this->propAngle = &(rev->Angle);
        this->propMidPlane = &(rev->Midplane);
        this->propReferenceAxis = &(rev->ReferenceAxis);
        this->propReversed = &(rev->Reversed);
    }

    ui->checkBoxMidplane->setChecked(propMidPlane->getValue());
    ui->checkBoxReversed->setChecked(propReversed->getValue());

    ui->revolveAngle->setValue(propAngle->getValue());
    ui->revolveAngle->setMaximum(propAngle->getMaximum());
    ui->revolveAngle->setMinimum(propAngle->getMinimum());

    blockUpdate = false;
    updateUI();

    TaskSketchBasedParameters::refresh();

    for (QWidget* child : proxy->findChildren<QWidget*>())
        child->blockSignals(false);
}


void TaskRevolutionParameters::fillAxisCombo(bool forceRefill)
{
    Base::StateLocker lock(blockUpdate, true);

    if (axesInList.empty())
        forceRefill = true;//not filled yet, full refill

    if (forceRefill) {
        ui->axis->clear();
        axesInList.clear();

        //add sketch axes
        auto *pcFeat = dynamic_cast<PartDesign::ProfileBased*>(vp->getObject());
        if (!pcFeat)
            throw Base::TypeError("The object is not ProfileBased.");
        auto *pcSketch = static_cast<Part::Part2DObject*>(pcFeat->Profile.getValue());
        if (pcSketch){
            addAxisToCombo(pcSketch, "V_Axis", QObject::tr("Vertical sketch axis"));
            addAxisToCombo(pcSketch, "H_Axis", QObject::tr("Horizontal sketch axis"));
            for (int i=0; i < pcSketch->getAxisCount(); i++) {
                QString itemText = QObject::tr("Construction line %1").arg(i+1);
                std::stringstream sub;
                sub << "Axis" << i;
                addAxisToCombo(pcSketch,sub.str(),itemText);
            }
        }

        //add part axes
        PartDesign::Body * body = PartDesign::Body::findBodyOf ( pcFeat );
        if (body) {
            try {
                App::Origin* orig = body->getOrigin();
                addAxisToCombo(orig->getX(), std::string(), tr("Base X axis"));
                addAxisToCombo(orig->getY(), std::string(), tr("Base Y axis"));
                addAxisToCombo(orig->getZ(), std::string(), tr("Base Z axis"));
            } catch (const Base::Exception &ex) {
                ex.ReportException();
            }
        }

        //add "Select reference"
        addAxisToCombo(nullptr, std::string(), tr("Select reference..."));
    }//endif forceRefill

    //add current link, if not in list
    //first, figure out the item number for current axis
    int indexOfCurrent = -1;
    App::DocumentObject* ax = propReferenceAxis->getValue();
    const std::vector<std::string> &subList = propReferenceAxis->getSubValues();
    for (size_t i = 0; i < axesInList.size(); i++) {
        if (ax == axesInList[i]->getValue() && subList == axesInList[i]->getSubValues())
            indexOfCurrent = i;
    }
    if (indexOfCurrent == -1  &&  ax) {
        assert(subList.size() <= 1);
        std::string sub;
        if (!subList.empty())
            sub = subList[0];
        addAxisToCombo(ax, sub, getRefStr(ax, subList));
        indexOfCurrent = axesInList.size()-1;
    }

    //highlight current.
    if (indexOfCurrent != -1)
        ui->axis->setCurrentIndex(indexOfCurrent);
}

void TaskRevolutionParameters::addAxisToCombo(App::DocumentObject* linkObj,
                                              std::string linkSubname,
                                              QString itemText)
{
    this->ui->axis->addItem(itemText);
    this->axesInList.emplace_back(new App::PropertyLinkSub());
    App::PropertyLinkSub &lnk = *(axesInList[axesInList.size()-1]);
    lnk.setValue(linkObj,std::vector<std::string>(1,linkSubname));
}

void TaskRevolutionParameters::connectSignals()
{
    QMetaObject::connectSlotsByName(this);
    Base::connect(ui->revolveAngle, QOverload<double>::of(&Gui::QuantitySpinBox::valueChanged),
                  this, &TaskRevolutionParameters::onAngleChanged);
    Base::connect(ui->axis, QOverload<int>::of(&QComboBox::currentIndexChanged),
                  this, &TaskRevolutionParameters::onAxisChanged);
    Base::connect(ui->checkBoxMidplane, &QCheckBox::toggled, this, &TaskRevolutionParameters::onMidplane);
    Base::connect(ui->checkBoxReversed, &QCheckBox::toggled, this, &TaskRevolutionParameters::onReversed);
    Base::connect(ui->buttonAxis, &QPushButton::clicked, this, &TaskRevolutionParameters::onAxisButton);
}

void TaskRevolutionParameters::updateUI()
{
    if (blockUpdate)
        return;
    Base::StateLocker lock(blockUpdate, true);
    fillAxisCombo();
}

void TaskRevolutionParameters::_onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if (msg.Type == Gui::SelectionChanges::AddSelection) {
        if (getSelectionMode() == SelectionMode::refAdd) {
            std::vector<std::string> axis;
            App::DocumentObject* selObj;
            if (getReferencedSelection(vp->getObject(), msg, selObj, axis) && selObj) {
                setupTransaction();
                propReferenceAxis->setValue(selObj, axis);
                recomputeFeature();
                updateUI();
            }
        }
    }
}


void TaskRevolutionParameters::onAngleChanged(double len)
{
    setupTransaction();
    propAngle->setValue(len);
    recomputeFeature();
}

bool TaskRevolutionParameters::eventFilter(QObject *o, QEvent *ev)
{
    switch(ev->type()) {
    case QEvent::Leave:
        if (o == ui->axis)
            Gui::Selection().rmvPreselect();
        break;
    case QEvent::Enter:
        if (vp && ui->axis) {
            if (auto obj = propReferenceAxis->getValue()) {
                const auto &subs = propReferenceAxis->getSubValues();
                PartDesignGui::highlightObjectOnTop(App::SubObjectT(obj, subs.size()?subs.front().c_str():""));
            }
        }
        break;
    default:
        break;
    }
    return false;
}

void TaskRevolutionParameters::onAxisChanged(int num)
{
    if (blockUpdate)
        return;
    PartDesign::ProfileBased* pcRevolution = static_cast<PartDesign::ProfileBased*>(vp->getObject());

    if (axesInList.empty())
        return;

    App::DocumentObject *oldRefAxis = propReferenceAxis->getValue();
    std::vector<std::string> oldSubRefAxis = propReferenceAxis->getSubValues();
    std::string oldRefName;
    if (!oldSubRefAxis.empty())
        oldRefName = oldSubRefAxis.front();

    App::PropertyLinkSub &lnk = *(axesInList[num]);
    if (lnk.getValue()) {
        if (!pcRevolution->getDocument()->isIn(lnk.getValue())){
            Base::Console().Error("Object was deleted\n");
            return;
        }
        setupTransaction();
        propReferenceAxis->Paste(lnk);
    }

    try {
        App::DocumentObject *newRefAxis = propReferenceAxis->getValue();
        const std::vector<std::string> &newSubRefAxis = propReferenceAxis->getSubValues();
        std::string newRefName;
        if (!newSubRefAxis.empty())
            newRefName = newSubRefAxis.front();

        if (oldRefAxis != newRefAxis ||
            oldSubRefAxis.size() != newSubRefAxis.size() ||
            oldRefName != newRefName) {
            bool reversed = propReversed->getValue();
            if (pcRevolution->isDerivedFrom(PartDesign::Revolution::getClassTypeId()))
                reversed = static_cast<PartDesign::Revolution*>(pcRevolution)->suggestReversed();
            if (pcRevolution->isDerivedFrom(PartDesign::Groove::getClassTypeId()))
                reversed = static_cast<PartDesign::Groove*>(pcRevolution)->suggestReversed();

            if (reversed != propReversed->getValue()) {
                propReversed->setValue(reversed);
                ui->checkBoxReversed->blockSignals(true);
                ui->checkBoxReversed->setChecked(reversed);
                ui->checkBoxReversed->blockSignals(false);
            }
        }

        recomputeFeature();
    }
    catch (const Base::Exception& e) {
        e.ReportException();
    }
}

void TaskRevolutionParameters::onMidplane(bool on)
{
    setupTransaction();
    propMidPlane->setValue(on);
    recomputeFeature();
}

void TaskRevolutionParameters::onReversed(bool on)
{
    setupTransaction();
    propReversed->setValue(on);
    recomputeFeature();
}

double TaskRevolutionParameters::getAngle() const
{
    return ui->revolveAngle->value().getValue();
}

void TaskRevolutionParameters::getReferenceAxis(App::DocumentObject*& obj, std::vector<std::string>& sub) const
{
    if (axesInList.empty())
        throw Base::RuntimeError("Not initialized!");

    int num = ui->axis->currentIndex();
    const App::PropertyLinkSub &lnk = *(axesInList[num]);
    if (!lnk.getValue()) {
        throw Base::RuntimeError("Still in reference selection mode; reference wasn't selected yet");
    } else {
        PartDesign::ProfileBased* pcRevolution = static_cast<PartDesign::ProfileBased*>(vp->getObject());
        if (!pcRevolution->getDocument()->isIn(lnk.getValue())){
            throw Base::RuntimeError("Object was deleted");
        }

        obj = lnk.getValue();
        sub = lnk.getSubValues();
    }
}

bool TaskRevolutionParameters::getMidplane() const
{
    return ui->checkBoxMidplane->isChecked();
}

bool TaskRevolutionParameters::getReversed() const
{
    return ui->checkBoxReversed->isChecked();
}

TaskRevolutionParameters::~TaskRevolutionParameters()
{
    try {
        //hide the parts coordinate system axis for selection
        PartDesign::Body * body = vp ? PartDesign::Body::findBodyOf(vp->getObject()) : nullptr;
        if (body) {
            App::Origin *origin = body->getOrigin();
            ViewProviderOrigin* vpOrigin;
            vpOrigin = static_cast<ViewProviderOrigin*>(Gui::Application::Instance->getViewProvider(origin));
            vpOrigin->resetTemporaryVisibility();
        }
    } catch (const Base::Exception &ex) {
        ex.ReportException();
    }

    axesInList.clear();
}

void TaskRevolutionParameters::changeEvent(QEvent *event)
{
    TaskBox::changeEvent(event);
    if (event->type() == QEvent::LanguageChange) {
        ui->retranslateUi(proxy);
    }
}

void TaskRevolutionParameters::apply()
{
    //Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Revolution changed"));
    ui->revolveAngle->apply();
    std::vector<std::string> sub;
    App::DocumentObject* obj;
    getReferenceAxis(obj, sub);
    std::string axis = buildLinkSingleSubPythonStr(obj, sub);
    auto tobj = vp->getObject();
    FCMD_OBJ_CMD(tobj,"ReferenceAxis = " << axis);
    FCMD_OBJ_CMD(tobj,"Midplane = " << (getMidplane() ? 1 : 0));
    FCMD_OBJ_CMD(tobj,"Reversed = " << (getReversed() ? 1 : 0));
}

//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
TaskDlgRevolutionParameters::TaskDlgRevolutionParameters(PartDesignGui::ViewProvider *RevolutionView)
    : TaskDlgSketchBasedParameters(RevolutionView)
{
    assert(RevolutionView);
    Content.push_back(new TaskRevolutionParameters(RevolutionView));
}


#include "moc_TaskRevolutionParameters.cpp"
