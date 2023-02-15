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

#ifndef _PreComp_
# include <sstream>
# include <QRegExp>
# include <QTextStream>
# include <Precision.hxx>
#endif

#include "ui_TaskPocketParameters.h"
#include "TaskPocketParameters.h"
#include <App/Application.h>
#include <App/Document.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/BitmapFactory.h>
#include <Gui/ViewProvider.h>
#include <Gui/WaitCursor.h>
#include <Base/Console.h>
#include <Base/Tools.h>
#include <Gui/Selection.h>
#include <Gui/Command.h>
#include <Mod/PartDesign/App/FeaturePocket.h>
#include <Mod/Sketcher/App/SketchObject.h>
#include "TaskSketchBasedParameters.h"
#include "ReferenceSelection.h"
#include "Utils.h"

using namespace PartDesignGui;
using namespace Gui;

/* TRANSLATOR PartDesignGui::TaskPocketParameters */

TaskPocketParameters::TaskPocketParameters(ViewProviderPocket *PocketView,QWidget *parent, bool newObj)
    : TaskSketchBasedParameters(PocketView, parent, "PartDesign_Pocket", tr("Pocket parameters"))
    , ui(new Ui_TaskPocketParameters)
    , oldLength(0)
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui->setupUi(proxy);
    ui->lineFaceName->setPlaceholderText(tr("No face selected"));
    addBlinkWidget(ui->lineFaceName);

    ui->lineFaceName->installEventFilter(this);
    ui->lineFaceName->setMouseTracking(true);

    this->groupLayout()->addWidget(proxy);
    initUI(proxy);

    // set the history path
    ui->lengthEdit->setParamGrpPath(QByteArray("User parameter:BaseApp/History/PocketLength"));
    ui->lengthEdit2->setParamGrpPath(QByteArray("User parameter:BaseApp/History/PocketLength2"));
    ui->offsetEdit->setParamGrpPath(QByteArray("User parameter:BaseApp/History/PocketOffset"));
    ui->taperAngleEdit->setParamGrpPath(QByteArray("User parameter:BaseApp/History/TaperAngle"));
    ui->taperAngleEdit2->setParamGrpPath(QByteArray("User parameter:BaseApp/History/TaperAngle2"));
    ui->innerTaperAngleEdit->setParamGrpPath(QByteArray("User parameter:BaseApp/History/InnerTaperAngle"));
    ui->innerTaperAngleEdit2->setParamGrpPath(QByteArray("User parameter:BaseApp/History/InnerTaperAngle2"));

    ui->changeMode->clear();
    ui->changeMode->insertItem(0, tr("Dimension"));
    ui->changeMode->insertItem(1, tr("Through all"));
    ui->changeMode->insertItem(2, tr("To first"));
    ui->changeMode->insertItem(3, tr("Up to face"));
    ui->changeMode->insertItem(4, tr("Two dimensions"));

    PartDesign::Pocket* pcPocket = static_cast<PartDesign::Pocket*>(vp->getObject());

    ui->checkFaceLimits->setToolTip(QApplication::translate(
                "Property", pcPocket->CheckUpToFaceLimits.getDocumentation()));

    ui->taperAngleEdit->setToolTip(QApplication::translate(
                "Property", pcPocket->TaperAngle.getDocumentation()));
    ui->taperAngleEdit2->setToolTip(QApplication::translate(
                "Property", pcPocket->TaperAngleRev.getDocumentation()));
    ui->innerTaperAngleEdit->setToolTip(QApplication::translate(
                "Property", pcPocket->TaperInnerAngle.getDocumentation()));
    ui->innerTaperAngleEdit2->setToolTip(QApplication::translate(
                "Property", pcPocket->TaperInnerAngleRev.getDocumentation()));
    ui->autoInnerTaperAngle->setToolTip(QApplication::translate(
                "Property", pcPocket->AutoTaperInnerAngle.getDocumentation()));

    // Bind input fields to properties
    ui->lengthEdit->bind(pcPocket->Length);
    ui->lengthEdit2->bind(pcPocket->Length2);
    ui->offsetEdit->bind(pcPocket->Offset);
    ui->taperAngleEdit->bind(pcPocket->TaperAngle);
    ui->taperAngleEdit2->bind(pcPocket->TaperAngleRev);
    ui->innerTaperAngleEdit->bind(pcPocket->TaperInnerAngle);
    ui->innerTaperAngleEdit2->bind(pcPocket->TaperInnerAngleRev);

    QMetaObject::connectSlotsByName(this);

    connect(ui->lengthEdit, SIGNAL(valueChanged(double)),
            this, SLOT(onLengthChanged(double)));
    connect(ui->lengthEdit2, SIGNAL(valueChanged(double)),
            this, SLOT(onLength2Changed(double)));
    connect(ui->taperAngleEdit, SIGNAL(valueChanged(double)),
            this, SLOT(onAngleChanged(double)));
    connect(ui->taperAngleEdit2, SIGNAL(valueChanged(double)),
            this, SLOT(onAngle2Changed(double)));
    connect(ui->innerTaperAngleEdit, SIGNAL(valueChanged(double)),
            this, SLOT(onInnerAngleChanged(double)));
    connect(ui->innerTaperAngleEdit2, SIGNAL(valueChanged(double)),
            this, SLOT(onInnerAngle2Changed(double)));
    connect(ui->offsetEdit, SIGNAL(valueChanged(double)),
            this, SLOT(onOffsetChanged(double)));
    connect(ui->checkBoxMidplane, SIGNAL(toggled(bool)),
            this, SLOT(onMidplaneChanged(bool)));
    connect(ui->checkBoxReversed, SIGNAL(toggled(bool)),
            this, SLOT(onReversedChanged(bool)));
    connect(ui->checkBoxUsePipe, SIGNAL(toggled(bool)),
            this, SLOT(onUsePipeChanged(bool)));
    connect(ui->checkFaceLimits, SIGNAL(toggled(bool)),
            this, SLOT(onCheckFaceLimitsChanged(bool)));
    connect(ui->changeMode, SIGNAL(currentIndexChanged(int)),
            this, SLOT(onModeChanged(int)));
    connect(ui->buttonFace, SIGNAL(clicked()),
            this, SLOT(onButtonFace()));
    connect(ui->lineFaceName, SIGNAL(textEdited(QString)),
            this, SLOT(onFaceName(QString)));

    QObject::connect(ui->autoInnerTaperAngle, &QCheckBox::toggled, [this](bool checked) {
        PartDesign::Pocket* pcPocket = static_cast<PartDesign::Pocket*>(vp->getObject());
        pcPocket->AutoTaperInnerAngle.setValue(checked);
        ui->innerTaperAngleEdit->setDisabled(checked);
        ui->innerTaperAngleEdit2->setDisabled(checked);
        recomputeFeature();
    });

    // Due to signals attached after changes took took into effect we should update the UI now.
    refresh();

    // if it is a newly created object use the last value of the history
    // TODO: newObj doesn't supplied normally by any caller (2015-07-24, Fat-Zer)
    if(newObj){
        ui->lengthEdit->setToLastUsedValue();
        ui->lengthEdit->selectNumber();
        ui->lengthEdit2->setToLastUsedValue();
        ui->lengthEdit2->selectNumber();
        ui->offsetEdit->setToLastUsedValue();
        ui->offsetEdit->selectNumber();
        ui->taperAngleEdit->setToLastUsedValue();
        ui->taperAngleEdit->selectNumber();
        ui->taperAngleEdit2->setToLastUsedValue();
        ui->taperAngleEdit2->selectNumber();
        ui->innerTaperAngleEdit->setToLastUsedValue();
        ui->innerTaperAngleEdit->selectNumber();
        ui->innerTaperAngleEdit2->setToLastUsedValue();
        ui->innerTaperAngleEdit2->selectNumber();
    }
}

bool TaskPocketParameters::eventFilter(QObject *o, QEvent *ev)
{
    switch(ev->type()) {
    case QEvent::Leave:
        Gui::Selection().rmvPreselect();
        break;
    case QEvent::Enter:
        if (vp && o == ui->lineFaceName) {
            auto pocket = static_cast<PartDesign::Pocket*>(vp->getObject());
            auto obj = pocket->UpToFace.getValue();
            if (obj) {
                const auto &subs = pocket->UpToFace.getSubValues(true);
                PartDesignGui::highlightObjectOnTop(
                        App::SubObjectT(obj, subs.size()?subs[0].c_str():""));
            }
        }
        break;
    default:
        break;
    }
    return false;
}

void TaskPocketParameters::refresh()
{
    if (!vp || !vp->getObject())
        return;

    // Get the feature data
    PartDesign::Pocket* pcPocket = static_cast<PartDesign::Pocket*>(vp->getObject());
    Base::Quantity l = pcPocket->Length.getQuantityValue();
    Base::Quantity l2 = pcPocket->Length2.getQuantityValue();
    Base::Quantity off = pcPocket->Offset.getQuantityValue();
    bool midplane = pcPocket->Midplane.getValue();
    bool reversed = pcPocket->Reversed.getValue();
    int index = pcPocket->Type.getValue(); // must extract value here, clear() kills it!
    double angle = pcPocket->TaperAngle.getValue();
    double angle2 = pcPocket->TaperAngleRev.getValue();
    double innerAngle = pcPocket->TaperInnerAngle.getValue();
    double innerAngle2 = pcPocket->TaperInnerAngleRev.getValue();

    // Temporarily prevent unnecessary feature recomputes
    for (QWidget* child : proxy->findChildren<QWidget*>())
        child->blockSignals(true);

    // Fill data into dialog elements
    ui->lengthEdit->setValue(l);
    ui->lengthEdit2->setValue(l2);
    ui->offsetEdit->setValue(off);
    ui->checkBoxMidplane->setChecked(midplane);
    ui->checkBoxReversed->setChecked(reversed);
    ui->checkBoxUsePipe->setChecked(pcPocket->UsePipeForDraft.getValue());
    ui->taperAngleEdit->setValue(angle);
    ui->taperAngleEdit2->setValue(angle2);
    ui->innerTaperAngleEdit->setValue(innerAngle);
    ui->innerTaperAngleEdit2->setValue(innerAngle2);

    // Set object labels
    App::DocumentObject* obj = pcPocket->UpToFace.getValue();
    std::vector<std::string> subStrings = pcPocket->UpToFace.getSubValues(false);
    if (obj && (subStrings.empty() || subStrings.front().empty())) {
        ui->lineFaceName->setText(QString::fromUtf8(obj->Label.getValue()));
        ui->lineFaceName->setProperty("FeatureName", QByteArray(obj->getNameInDocument()));
        ui->lineFaceName->setProperty("FaceName", QVariant());
    }
    else if (obj) {
        ui->lineFaceName->setText(QStringLiteral("%1:%2")
                                  .arg(QString::fromUtf8(obj->Label.getValue()))
                                  .arg(QString::fromUtf8(subStrings.front().c_str())));
        ui->lineFaceName->setProperty("FeatureName", QByteArray(obj->getNameInDocument()));
        ui->lineFaceName->setProperty("FaceName", QByteArray(subStrings.front().c_str()));

    }
    else {
        ui->lineFaceName->clear();
        ui->lineFaceName->setProperty("FeatureName", QVariant());
        ui->lineFaceName->setProperty("FaceName", QVariant());
    }

    ui->changeMode->setCurrentIndex(index);

    ui->checkFaceLimits->setChecked(pcPocket->CheckUpToFaceLimits.getValue());

    ui->autoInnerTaperAngle->setChecked(pcPocket->AutoTaperInnerAngle.getValue());

    // Temporarily prevent unnecessary feature recomputes
    for (QWidget* child : proxy->findChildren<QWidget*>())
        child->blockSignals(false);

    updateUI(index);
    TaskSketchBasedParameters::refresh();
}


void TaskPocketParameters::updateUI(int index)
{
    // disable/hide everything unless we are sure we don't need it
    bool isLengthEditVisable  = false;
    bool isLengthEdit2Visable = false;    
    bool isOffsetEditVisable  = false;
    bool isOffsetEditEnabled  = true;
    bool isMidplateEnabled    = false;
    bool isReversedEnabled    = false;
    bool isFaceEditEnabled    = false;

    // dimension
    if (index == 0) {
        isLengthEditVisable = true;
        ui->lengthEdit->selectNumber();
        // Make sure that the spin box has the focus to get key events
        // Calling setFocus() directly doesn't work because the spin box is not
        // yet visible.
        QMetaObject::invokeMethod(ui->lengthEdit, "setFocus", Qt::QueuedConnection);
        isMidplateEnabled = true;
        // Reverse only makes sense if Midplane is not true
        isReversedEnabled = !ui->checkBoxMidplane->isChecked();
    }
    // through all
    else if (index == 1) {
        isOffsetEditVisable = true;
        isOffsetEditEnabled = false; // offset may have some meaning for through all but it doesn't work
        isMidplateEnabled = true;
        isReversedEnabled = !ui->checkBoxMidplane->isChecked();
    }
    // up to first
    else if (index == 2) {
        isOffsetEditVisable = true;
        isReversedEnabled = true;       // Will change the direction it seeks for its first face?
            // It may work not quite as expected but useful if sketch oriented upside-down.
            // (may happen in bodies)
            // FIXME: Fix probably lies somewhere in IF block on line 125 of FeaturePocket.cpp
    }
    // up to face
    else if (index == 3) {
        isOffsetEditVisable = true;
        isReversedEnabled = true;
        isFaceEditEnabled    = true;
        // Go into reference selection mode if no face has been selected yet
        if (ui->lineFaceName->property("FeatureName").isNull())
            onButtonFace(true);
        ui->lineFaceName->show();
        ui->buttonFace->show();
    }
    // two dimensions
    else {
        isLengthEditVisable = true;
        isLengthEdit2Visable = true;
        isReversedEnabled = true;
    }    

    if (index != 3) {
        ui->lineFaceName->hide();
        ui->buttonFace->hide();
    }

    ui->checkFaceLimits->setVisible(index == 2 || index == 3);

    ui->lengthEdit->setVisible( isLengthEditVisable );
    ui->lengthEdit->setEnabled( isLengthEditVisable );
    ui->labelLength->setVisible( isLengthEditVisable );

    ui->lengthEdit2->setVisible( isLengthEdit2Visable );
    ui->lengthEdit2->setEnabled( isLengthEdit2Visable );
    ui->labelLength2->setVisible( isLengthEdit2Visable );

    ui->offsetEdit->setVisible( isOffsetEditVisable );
    ui->offsetEdit->setEnabled( isOffsetEditVisable && isOffsetEditEnabled );
    ui->labelOffset->setVisible( isOffsetEditVisable );

    ui->checkBoxMidplane->setEnabled( isMidplateEnabled );

    ui->checkBoxReversed->setEnabled( isReversedEnabled );

    ui->buttonFace->setEnabled( isFaceEditEnabled );
    ui->lineFaceName->setEnabled( isFaceEditEnabled );
    if (!isFaceEditEnabled) {
        onButtonFace(false);
    }

    bool angleVisible = index == 0 || index == 4;
    ui->taperAngleEdit->setVisible( angleVisible );
    ui->taperAngleEdit->setEnabled( angleVisible );
    ui->labelTaperAngle->setVisible( angleVisible );
    ui->taperAngleEdit2->setVisible( angleVisible );
    ui->taperAngleEdit2->setEnabled( angleVisible );
    ui->labelTaperAngle2->setVisible( angleVisible );
    ui->innerTaperAngleEdit->setVisible( angleVisible );
    ui->innerTaperAngleEdit->setEnabled( angleVisible );
    ui->labelInnerTaperAngle->setVisible( angleVisible );
    ui->innerTaperAngleEdit2->setVisible( angleVisible );
    ui->innerTaperAngleEdit2->setEnabled( angleVisible );
    ui->labelInnerTaperAngle2->setVisible( angleVisible );
}

void TaskPocketParameters::_onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if (msg.Type == Gui::SelectionChanges::AddSelection) {
        if (getSelectionMode() == SelectionMode::refAdd) {
            QSignalBlocker guard(ui->lineFaceName);
            QString refText = onSelectUpToFace(msg);
            if (refText.length() > 0) {
                ui->lineFaceName->setText(refText);
                QStringList list(refText.split(QLatin1Char(':')));
                ui->lineFaceName->setProperty("FeatureName", list[0].toUtf8());
                ui->lineFaceName->setProperty("FaceName", list.size()>1 ? list[1].toLatin1() : QByteArray());
                // Turn off reference selection mode
                onButtonFace(false);
            } else {
                ui->lineFaceName->clear();
                ui->lineFaceName->setProperty("FeatureName", QVariant());
                ui->lineFaceName->setProperty("FaceName", QVariant());
            }
        }
    } else if (msg.Type == Gui::SelectionChanges::ClrSelection) {
        if (getSelectionMode() == SelectionMode::refAdd) {
            QSignalBlocker guard(ui->lineFaceName);
            ui->lineFaceName->clear();
            ui->lineFaceName->setProperty("FeatureName", QVariant());
            ui->lineFaceName->setProperty("FaceName", QVariant());
        }
    }
}

void TaskPocketParameters::onLengthChanged(double len)
{
    PartDesign::Pocket* pcPocket = static_cast<PartDesign::Pocket*>(vp->getObject());
    pcPocket->Length.setValue(len);
    recomputeFeature();
}

void TaskPocketParameters::onLength2Changed(double len)
{
    PartDesign::Pocket* pcPocket = static_cast<PartDesign::Pocket*>(vp->getObject());
    pcPocket->Length2.setValue(len);
    recomputeFeature();
}

void TaskPocketParameters::onAngleChanged(double angle)
{
    PartDesign::Pocket* pcPocket = static_cast<PartDesign::Pocket*>(vp->getObject());
    pcPocket->TaperAngle.setValue(angle);
    recomputeFeature();
}

void TaskPocketParameters::onAngle2Changed(double angle)
{
    PartDesign::Pocket* pcPocket = static_cast<PartDesign::Pocket*>(vp->getObject());
    pcPocket->TaperAngleRev.setValue(angle);
    recomputeFeature();
}

void TaskPocketParameters::onInnerAngleChanged(double angle)
{
    PartDesign::Pocket* pcPocket = static_cast<PartDesign::Pocket*>(vp->getObject());
    pcPocket->TaperInnerAngle.setValue(angle);
    recomputeFeature();
}

void TaskPocketParameters::onInnerAngle2Changed(double angle)
{
    PartDesign::Pocket* pcPocket = static_cast<PartDesign::Pocket*>(vp->getObject());
    pcPocket->TaperInnerAngleRev.setValue(angle);
    recomputeFeature();
}

void TaskPocketParameters::onOffsetChanged(double len)
{
    PartDesign::Pocket* pcPocket = static_cast<PartDesign::Pocket*>(vp->getObject());
    pcPocket->Offset.setValue(len);
    recomputeFeature();
}

void TaskPocketParameters::onMidplaneChanged(bool on)
{
    PartDesign::Pocket* pcPocket = static_cast<PartDesign::Pocket*>(vp->getObject());
    pcPocket->Midplane.setValue(on);
    ui->checkBoxReversed->setEnabled(!on);
    recomputeFeature();
}

void TaskPocketParameters::onUsePipeChanged(bool on)
{
    PartDesign::Pocket* pcPocket = static_cast<PartDesign::Pocket*>(vp->getObject());
    pcPocket->UsePipeForDraft.setValue(on);
    ui->checkBoxReversed->setEnabled(!on);
    recomputeFeature();
}

void TaskPocketParameters::onCheckFaceLimitsChanged(bool on)
{
    PartDesign::Pocket* pcPocket = static_cast<PartDesign::Pocket*>(vp->getObject());
    pcPocket->CheckUpToFaceLimits.setValue(on);
    recomputeFeature();
}

void TaskPocketParameters::onReversedChanged(bool on)
{
    PartDesign::Pocket* pcPocket = static_cast<PartDesign::Pocket*>(vp->getObject());
    pcPocket->Reversed.setValue(on);
    recomputeFeature();
}

void TaskPocketParameters::onModeChanged(int index)
{
    PartDesign::Pocket* pcPocket = static_cast<PartDesign::Pocket*>(vp->getObject());

    switch (index) {
        case 0:
            // Why? See below for "UpToFace"
            if (oldLength < Precision::Confusion())
                oldLength = 5.0;
            pcPocket->Length.setValue(oldLength);
            ui->lengthEdit->setValue(oldLength);
            pcPocket->Type.setValue("Length");
            break;
        case 1:
            oldLength = pcPocket->Length.getValue();
            pcPocket->Type.setValue("ThroughAll");
            break;
        case 2:
            oldLength = pcPocket->Length.getValue();
            pcPocket->Type.setValue("UpToFirst");
            break;
        case 3: {
            // Because of the code at the beginning of Pocket::execute() which is used to detect
            // broken legacy parts, we must set the length to zero here!
            oldLength = pcPocket->Length.getValue();
            pcPocket->Type.setValue("UpToFace");
            QSignalBlocker blocker(ui->lengthEdit);
            pcPocket->Length.setValue(0.0);
            ui->lengthEdit->setValue(0.0);
            break;
        }
        default: 
            oldLength = pcPocket->Length.getValue();
            pcPocket->Type.setValue("TwoLengths");
    }

    updateUI(index);
    if (index != 3 || pcPocket->UpToFace.getValue())
        recomputeFeature();
}

void TaskPocketParameters::onButtonFace(const bool pressed)
{
    if (!pressed) {
        exitSelectionMode();
        return;
    }
    TaskSketchBasedParameters::onSelectReference(ui->buttonFace);
}

void TaskPocketParameters::onSelectionModeChanged(SelectionMode)
{
    if (getSelectionMode() == SelectionMode::refAdd) {
        ui->buttonFace->setChecked(true);
    } else {
        ui->buttonFace->setChecked(false);
    }
}

void TaskPocketParameters::onFaceName(const QString& text)
{
    if (text.isEmpty()) {
        // if user cleared the text field then also clear the properties
        ui->lineFaceName->setProperty("FeatureName", QVariant());
        ui->lineFaceName->setProperty("FaceName", QVariant());
    }
    else {
        // expect that the label of an object is used
        QStringList parts = text.split(QChar::fromLatin1(':'));
        QString label = parts[0];
        QVariant name = objectNameByLabel(label, ui->lineFaceName->property("FeatureName"));
        if (name.isValid()) {
            parts[0] = name.toString();
            QString uptoface = parts.join(QStringLiteral(":"));
            ui->lineFaceName->setProperty("FeatureName", name);
            ui->lineFaceName->setProperty("FaceName", setUpToFace(uptoface));
        }
        else {
            ui->lineFaceName->setProperty("FeatureName", QVariant());
            ui->lineFaceName->setProperty("FaceName", QVariant());
        }
    }
}

double TaskPocketParameters::getLength(void) const
{
    return ui->lengthEdit->value().getValue();
}

double TaskPocketParameters::getLength2(void) const
{
    return ui->lengthEdit2->value().getValue();
}

double TaskPocketParameters::getOffset(void) const
{
    return ui->offsetEdit->value().getValue();
}

bool   TaskPocketParameters::getReversed(void) const
{
    return ui->checkBoxReversed->isChecked();
}

bool   TaskPocketParameters::getMidplane(void) const
{
    return ui->checkBoxMidplane->isChecked();
}

int TaskPocketParameters::getMode(void) const
{
    return ui->changeMode->currentIndex();
}

QString TaskPocketParameters::getFaceName(void) const
{
    // 'Up to face' mode
    if (getMode() == 3) {
        QVariant featureName = ui->lineFaceName->property("FeatureName");
        if (featureName.isValid()) {
            QString faceName = ui->lineFaceName->property("FaceName").toString();
            return getFaceReference(featureName.toString(), faceName);
        }
    }
    return QStringLiteral("None");
}

TaskPocketParameters::~TaskPocketParameters()
{
}

void TaskPocketParameters::changeEvent(QEvent *e)
{
    TaskBox::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
        ui->lengthEdit->blockSignals(true);
        ui->lengthEdit2->blockSignals(true);
        ui->offsetEdit->blockSignals(true);
        ui->lineFaceName->blockSignals(true);
        ui->changeMode->blockSignals(true);
        int index = ui->changeMode->currentIndex();
        ui->retranslateUi(proxy);
        ui->changeMode->clear();
        ui->changeMode->addItem(tr("Dimension"));
        ui->changeMode->addItem(tr("Through all"));
        ui->changeMode->addItem(tr("To first"));
        ui->changeMode->addItem(tr("Up to face"));
        ui->changeMode->addItem(tr("Two dimensions"));
        ui->changeMode->setCurrentIndex(index);

        ui->lineFaceName->setPlaceholderText(tr("No face selected"));
        addBlinkWidget(ui->lineFaceName);
        
        ui->lengthEdit->blockSignals(false);
        ui->lengthEdit2->blockSignals(false);
        ui->offsetEdit->blockSignals(false);
        ui->lineFaceName->blockSignals(false);
        ui->changeMode->blockSignals(false);
    }
}

void TaskPocketParameters::saveHistory(void)
{
    // save the user values to history
    ui->lengthEdit->pushToHistory();
    ui->lengthEdit2->pushToHistory();
    ui->offsetEdit->pushToHistory();
    ui->taperAngleEdit->pushToHistory();
    ui->taperAngleEdit2->pushToHistory();
    ui->innerTaperAngleEdit->pushToHistory();
    ui->innerTaperAngleEdit2->pushToHistory();
}

void TaskPocketParameters::apply()
{
    auto obj = vp->getObject();

    ui->lengthEdit->apply();
    ui->lengthEdit2->apply();

    FCMD_OBJ_CMD(obj,"Type = " << getMode());
    QString facename = getFaceName();
    FCMD_OBJ_CMD(obj,"UpToFace = " << facename.toUtf8().data());
    FCMD_OBJ_CMD(obj,"Reversed = " << (getReversed()?1:0));
    FCMD_OBJ_CMD(obj,"Midplane = " << (getMidplane()?1:0));
    FCMD_OBJ_CMD(obj,"Offset = " << getOffset());
}

//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgPocketParameters::TaskDlgPocketParameters(ViewProviderPocket *PocketView)
    : TaskDlgSketchBasedParameters(PocketView)
{
    assert(vp);
    Content.push_back ( new TaskPocketParameters(PocketView ) );
}

#include "moc_TaskPocketParameters.cpp"
