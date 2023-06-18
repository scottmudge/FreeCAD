/***************************************************************************
 *   Copyright (c) 2021 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include <App/Application.h>

#include "DlgSettingsSelection.h"
#include "ui_DlgSettingsSelection.h"
#include <App/Application.h>
#include "TreeParams.h"
#include "ViewParams.h"

using namespace Gui;
using namespace Gui::Dialog;

/* TRANSLATOR Gui::Dialog::DlgSettingsSelection */

DlgSettingsSelection::DlgSettingsSelection(QWidget* parent)
    : PreferencePage(parent)
    , ui(new Ui_DlgSettingsSelection)
{
    ui->setupUi(this);
}

DlgSettingsSelection::~DlgSettingsSelection()
{
}

void DlgSettingsSelection::saveSettings()
{
    ui->spinPickRadius->onSave();

    TreeParams::setSyncView(ui->checkBoxAutoSwitch->isChecked());
    TreeParams::setSyncSelection(ui->checkBoxAutoExpand->isChecked());
    TreeParams::setPreSelection(ui->checkBoxPreselect->isChecked());
    TreeParams::setRecordSelection(ui->checkBoxRecord->isChecked());
    TreeParams::setCheckBoxesSelection(ui->checkBoxSelectionCheckBoxes->isChecked());

    ViewParams::setShowSelectionOnTop(ui->checkBoxSelectionOnTop->isChecked());
    ViewParams::setShowPreSelectedFaceOnTop(ui->checkBoxPreSelectionOnTop->isChecked());
    ViewParams::setShowSelectionBoundingBox(ui->checkBoxShowBoundBox->isChecked());
    ViewParams::setHiddenLineSelectionOnTop(ui->checkBoxHiddenLineSelect->isChecked());
    ViewParams::setSelectElementOnTop(ui->checkBoxSelectElementOnTop->isChecked());

    ViewParams::setPreselectionToolTipCorner(ui->comboBoxToolTipCorner->currentIndex());
    ViewParams::setPreselectionToolTipOffsetX(ui->spinBoxToolTipOffsetX->value());
    ViewParams::setPreselectionToolTipOffsetY(ui->spinBoxToolTipOffsetY->value());
    ViewParams::setPreselectionToolTipFontSize(ui->spinBoxToolTipFontSize->value());
    
    ViewParams::setSelectionColorDifference(ui->spinBoxColorDiff->value());
    ViewParams::setShowSelectionBoundingBoxThreshold(ui->spinBoxSelectionBBoxThreshold->value());
}

void DlgSettingsSelection::loadSettings()
{
    ui->spinPickRadius->onRestore();

    ui->checkBoxAutoSwitch->setChecked(TreeParams::getSyncView());
    ui->checkBoxAutoExpand->setChecked(TreeParams::getSyncSelection());
    ui->checkBoxPreselect->setChecked(TreeParams::getPreSelection());
    ui->checkBoxRecord->setChecked(TreeParams::getRecordSelection());
    ui->checkBoxSelectionCheckBoxes->setChecked(TreeParams::getCheckBoxesSelection());

    ui->checkBoxSelectionOnTop->setChecked(ViewParams::getShowSelectionOnTop());
    ui->checkBoxPreSelectionOnTop->setChecked(ViewParams::getShowPreSelectedFaceOnTop());
    ui->checkBoxShowBoundBox->setChecked(ViewParams::getShowSelectionBoundingBox());
    ui->checkBoxHiddenLineSelect->setChecked(ViewParams::getHiddenLineSelectionOnTop());
    ui->checkBoxSelectElementOnTop->setChecked(ViewParams::getSelectElementOnTop());

    ui->comboBoxToolTipCorner->setCurrentIndex(ViewParams::getPreselectionToolTipCorner());
    ui->spinBoxToolTipOffsetX->setValue(ViewParams::getPreselectionToolTipOffsetX());
    ui->spinBoxToolTipOffsetY->setValue(ViewParams::getPreselectionToolTipOffsetY());
    ui->spinBoxToolTipFontSize->setValue(ViewParams::getPreselectionToolTipFontSize());

    ui->spinBoxColorDiff->setValue(ViewParams::getSelectionColorDifference());
    ui->spinBoxSelectionBBoxThreshold->setValue(ViewParams::getShowSelectionBoundingBoxThreshold());
}

void DlgSettingsSelection::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    else {
        QWidget::changeEvent(e);
    }
}

#include "moc_DlgSettingsSelection.cpp"

