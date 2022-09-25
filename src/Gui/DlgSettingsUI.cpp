/****************************************************************************
 *   Copyright (c) 2020 Zheng Lei (realthunder) <realthunder.dev@gmail.com> *
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

#include <QPropertyAnimation>
#include <QTimer>

/*[[[cog
import DlgSettingsUI
DlgSettingsUI.define()
]]]*/

// Auto generated code (Tools/params_utils.py:467)
#ifndef _PreComp_
#   include <QApplication>
#   include <QLabel>
#   include <QGroupBox>
#   include <QGridLayout>
#   include <QVBoxLayout>
#   include <QHBoxLayout>
#endif
#include <Gui/ViewParams.h>
#include <Gui/TreeParams.h>
#include <Gui/ExprParams.h>
// Auto generated code (Tools/params_utils.py:485)
#include "Gui/DlgSettingsUI.h"
using namespace Gui::Dialog;
/* TRANSLATOR Gui::Dialog::DlgSettingsUI */

// Auto generated code (Tools/params_utils.py:492)
DlgSettingsUI::DlgSettingsUI(QWidget* parent)
    : PreferencePage( parent )
{

    auto layout = new QVBoxLayout(this);


    // Auto generated code (Tools/params_utils.py:342)
    groupGeneral = new QGroupBox(this);
    layout->addWidget(groupGeneral);
    auto layoutHorizGeneral = new QHBoxLayout(groupGeneral);
    auto layoutGeneral = new QGridLayout();
    layoutHorizGeneral->addLayout(layoutGeneral);
    layoutHorizGeneral->addStretch();

    // Auto generated code (Tools/params_utils.py:353)
    labelTextCursorWidth = new QLabel(this);
    layoutGeneral->addWidget(labelTextCursorWidth, 0, 0);
    TextCursorWidth = new Gui::PrefSpinBox(this);
    layoutGeneral->addWidget(TextCursorWidth, 0, 1);
    TextCursorWidth->setValue(Gui::ViewParams::defaultTextCursorWidth());
    TextCursorWidth->setEntryName("TextCursorWidth");
    TextCursorWidth->setParamGrpPath("View");
    // Auto generated code (Tools/params_utils.py:948)
    TextCursorWidth->setMinimum(1);
    TextCursorWidth->setMaximum(100);
    TextCursorWidth->setSingleStep(1);


    // Auto generated code (Tools/params_utils.py:342)
    groupTreeview = new QGroupBox(this);
    layout->addWidget(groupTreeview);
    auto layoutHorizTreeview = new QHBoxLayout(groupTreeview);
    auto layoutTreeview = new QGridLayout();
    layoutHorizTreeview->addLayout(layoutTreeview);
    layoutHorizTreeview->addStretch();

    // Auto generated code (Tools/params_utils.py:353)
    labelItemBackground = new QLabel(this);
    layoutTreeview->addWidget(labelItemBackground, 0, 0);
    ItemBackground = new Gui::PrefColorButton(this);
    layoutTreeview->addWidget(ItemBackground, 0, 1);
    ItemBackground->setPackedColor(Gui::TreeParams::defaultItemBackground());
    ItemBackground->setEntryName("ItemBackground");
    ItemBackground->setParamGrpPath("TreeView");
    ItemBackground->setAllowTransparency(true);

    // Auto generated code (Tools/params_utils.py:353)
    labelItemBackgroundPadding = new QLabel(this);
    layoutTreeview->addWidget(labelItemBackgroundPadding, 1, 0);
    ItemBackgroundPadding = new Gui::PrefSpinBox(this);
    layoutTreeview->addWidget(ItemBackgroundPadding, 1, 1);
    ItemBackgroundPadding->setValue(Gui::TreeParams::defaultItemBackgroundPadding());
    ItemBackgroundPadding->setEntryName("ItemBackgroundPadding");
    ItemBackgroundPadding->setParamGrpPath("TreeView");
    // Auto generated code (Tools/params_utils.py:948)
    ItemBackgroundPadding->setMinimum(0);
    ItemBackgroundPadding->setMaximum(100);
    ItemBackgroundPadding->setSingleStep(1);

    // Auto generated code (Tools/params_utils.py:353)
    ResizableColumn = new Gui::PrefCheckBox(this);
    layoutTreeview->addWidget(ResizableColumn, 2, 0);
    ResizableColumn->setChecked(Gui::TreeParams::defaultResizableColumn());
    ResizableColumn->setEntryName("ResizableColumn");
    ResizableColumn->setParamGrpPath("TreeView");

    // Auto generated code (Tools/params_utils.py:353)
    CheckBoxesSelection = new Gui::PrefCheckBox(this);
    layoutTreeview->addWidget(CheckBoxesSelection, 3, 0);
    CheckBoxesSelection->setChecked(Gui::TreeParams::defaultCheckBoxesSelection());
    CheckBoxesSelection->setEntryName("CheckBoxesSelection");
    CheckBoxesSelection->setParamGrpPath("TreeView");

    // Auto generated code (Tools/params_utils.py:353)
    HideColumn = new Gui::PrefCheckBox(this);
    layoutTreeview->addWidget(HideColumn, 4, 0);
    HideColumn->setChecked(Gui::TreeParams::defaultHideColumn());
    HideColumn->setEntryName("HideColumn");
    HideColumn->setParamGrpPath("TreeView");

    // Auto generated code (Tools/params_utils.py:353)
    HideScrollBar = new Gui::PrefCheckBox(this);
    layoutTreeview->addWidget(HideScrollBar, 5, 0);
    HideScrollBar->setChecked(Gui::TreeParams::defaultHideScrollBar());
    HideScrollBar->setEntryName("HideScrollBar");
    HideScrollBar->setParamGrpPath("TreeView");

    // Auto generated code (Tools/params_utils.py:353)
    HideHeaderView = new Gui::PrefCheckBox(this);
    layoutTreeview->addWidget(HideHeaderView, 6, 0);
    HideHeaderView->setChecked(Gui::TreeParams::defaultHideHeaderView());
    HideHeaderView->setEntryName("HideHeaderView");
    HideHeaderView->setParamGrpPath("TreeView");

    // Auto generated code (Tools/params_utils.py:353)
    TreeToolTipIcon = new Gui::PrefCheckBox(this);
    layoutTreeview->addWidget(TreeToolTipIcon, 7, 0);
    TreeToolTipIcon->setChecked(Gui::TreeParams::defaultTreeToolTipIcon());
    TreeToolTipIcon->setEntryName("TreeToolTipIcon");
    TreeToolTipIcon->setParamGrpPath("TreeView");


    // Auto generated code (Tools/params_utils.py:342)
    groupExpression = new QGroupBox(this);
    layout->addWidget(groupExpression);
    auto layoutHorizExpression = new QHBoxLayout(groupExpression);
    auto layoutExpression = new QGridLayout();
    layoutHorizExpression->addLayout(layoutExpression);
    layoutHorizExpression->addStretch();

    // Auto generated code (Tools/params_utils.py:353)
    AutoHideEditorIcon = new Gui::PrefCheckBox(this);
    layoutExpression->addWidget(AutoHideEditorIcon, 0, 0);
    AutoHideEditorIcon->setChecked(Gui::ExprParams::defaultAutoHideEditorIcon());
    AutoHideEditorIcon->setEntryName("AutoHideEditorIcon");
    AutoHideEditorIcon->setParamGrpPath("Expression");

    // Auto generated code (Tools/params_utils.py:353)
    labelEditorTrigger = new QLabel(this);
    layoutExpression->addWidget(labelEditorTrigger, 1, 0);
    EditorTrigger = new Gui::PrefAccelLineEdit(this);
    layoutExpression->addWidget(EditorTrigger, 1, 1);
    EditorTrigger->setDisplayText(Gui::ExprParams::defaultEditorTrigger());
    EditorTrigger->setEntryName("EditorTrigger");
    EditorTrigger->setParamGrpPath("Expression");

    // Auto generated code (Tools/params_utils.py:353)
    NoSystemBackground = new Gui::PrefCheckBox(this);
    layoutExpression->addWidget(NoSystemBackground, 2, 0);
    NoSystemBackground->setChecked(Gui::ExprParams::defaultNoSystemBackground());
    NoSystemBackground->setEntryName("NoSystemBackground");
    NoSystemBackground->setParamGrpPath("Expression");

    // Auto generated code (Tools/params_utils.py:353)
    labelEditDialogBGAlpha = new QLabel(this);
    layoutExpression->addWidget(labelEditDialogBGAlpha, 3, 0);
    EditDialogBGAlpha = new Gui::PrefSpinBox(this);
    layoutExpression->addWidget(EditDialogBGAlpha, 3, 1);
    EditDialogBGAlpha->setValue(Gui::ExprParams::defaultEditDialogBGAlpha());
    EditDialogBGAlpha->setEntryName("EditDialogBGAlpha");
    EditDialogBGAlpha->setParamGrpPath("Expression");
    // Auto generated code (Tools/params_utils.py:948)
    EditDialogBGAlpha->setMinimum(0);
    EditDialogBGAlpha->setMaximum(255);
    EditDialogBGAlpha->setSingleStep(1);


    // Auto generated code (Tools/params_utils.py:342)
    groupPiemenu = new QGroupBox(this);
    layout->addWidget(groupPiemenu);
    auto layoutHorizPiemenu = new QHBoxLayout(groupPiemenu);
    auto layoutPiemenu = new QGridLayout();
    layoutHorizPiemenu->addLayout(layoutPiemenu);
    layoutHorizPiemenu->addStretch();

    // Auto generated code (Tools/params_utils.py:353)
    labelPieMenuIconSize = new QLabel(this);
    layoutPiemenu->addWidget(labelPieMenuIconSize, 0, 0);
    PieMenuIconSize = new Gui::PrefSpinBox(this);
    layoutPiemenu->addWidget(PieMenuIconSize, 0, 1);
    PieMenuIconSize->setValue(Gui::ViewParams::defaultPieMenuIconSize());
    PieMenuIconSize->setEntryName("PieMenuIconSize");
    PieMenuIconSize->setParamGrpPath("View");
    // Auto generated code (Tools/params_utils.py:948)
    PieMenuIconSize->setMinimum(0);
    PieMenuIconSize->setMaximum(64);
    PieMenuIconSize->setSingleStep(1);

    // Auto generated code (Tools/params_utils.py:353)
    labelPieMenuRadius = new QLabel(this);
    layoutPiemenu->addWidget(labelPieMenuRadius, 1, 0);
    PieMenuRadius = new Gui::PrefSpinBox(this);
    layoutPiemenu->addWidget(PieMenuRadius, 1, 1);
    PieMenuRadius->setValue(Gui::ViewParams::defaultPieMenuRadius());
    PieMenuRadius->setEntryName("PieMenuRadius");
    PieMenuRadius->setParamGrpPath("View");
    // Auto generated code (Tools/params_utils.py:948)
    PieMenuRadius->setMinimum(10);
    PieMenuRadius->setMaximum(500);
    PieMenuRadius->setSingleStep(10);

    // Auto generated code (Tools/params_utils.py:353)
    labelPieMenuTriggerRadius = new QLabel(this);
    layoutPiemenu->addWidget(labelPieMenuTriggerRadius, 2, 0);
    PieMenuTriggerRadius = new Gui::PrefSpinBox(this);
    layoutPiemenu->addWidget(PieMenuTriggerRadius, 2, 1);
    PieMenuTriggerRadius->setValue(Gui::ViewParams::defaultPieMenuTriggerRadius());
    PieMenuTriggerRadius->setEntryName("PieMenuTriggerRadius");
    PieMenuTriggerRadius->setParamGrpPath("View");
    // Auto generated code (Tools/params_utils.py:948)
    PieMenuTriggerRadius->setMinimum(10);
    PieMenuTriggerRadius->setMaximum(500);
    PieMenuTriggerRadius->setSingleStep(10);

    // Auto generated code (Tools/params_utils.py:353)
    labelPieMenuCenterRadius = new QLabel(this);
    layoutPiemenu->addWidget(labelPieMenuCenterRadius, 3, 0);
    PieMenuCenterRadius = new Gui::PrefSpinBox(this);
    layoutPiemenu->addWidget(PieMenuCenterRadius, 3, 1);
    PieMenuCenterRadius->setValue(Gui::ViewParams::defaultPieMenuCenterRadius());
    PieMenuCenterRadius->setEntryName("PieMenuCenterRadius");
    PieMenuCenterRadius->setParamGrpPath("View");
    // Auto generated code (Tools/params_utils.py:948)
    PieMenuCenterRadius->setMinimum(0);
    PieMenuCenterRadius->setMaximum(250);
    PieMenuCenterRadius->setSingleStep(1);

    // Auto generated code (Tools/params_utils.py:353)
    labelPieMenuFontSize = new QLabel(this);
    layoutPiemenu->addWidget(labelPieMenuFontSize, 4, 0);
    PieMenuFontSize = new Gui::PrefSpinBox(this);
    layoutPiemenu->addWidget(PieMenuFontSize, 4, 1);
    PieMenuFontSize->setValue(Gui::ViewParams::defaultPieMenuFontSize());
    PieMenuFontSize->setEntryName("PieMenuFontSize");
    PieMenuFontSize->setParamGrpPath("View");
    // Auto generated code (Tools/params_utils.py:948)
    PieMenuFontSize->setMinimum(0);
    PieMenuFontSize->setMaximum(32);
    PieMenuFontSize->setSingleStep(1);

    // Auto generated code (Tools/params_utils.py:353)
    labelPieMenuTriggerDelay = new QLabel(this);
    layoutPiemenu->addWidget(labelPieMenuTriggerDelay, 5, 0);
    PieMenuTriggerDelay = new Gui::PrefSpinBox(this);
    layoutPiemenu->addWidget(PieMenuTriggerDelay, 5, 1);
    PieMenuTriggerDelay->setValue(Gui::ViewParams::defaultPieMenuTriggerDelay());
    PieMenuTriggerDelay->setEntryName("PieMenuTriggerDelay");
    PieMenuTriggerDelay->setParamGrpPath("View");
    // Auto generated code (Tools/params_utils.py:948)
    PieMenuTriggerDelay->setMinimum(0);
    PieMenuTriggerDelay->setMaximum(10000);
    PieMenuTriggerDelay->setSingleStep(100);

    // Auto generated code (Tools/params_utils.py:353)
    PieMenuTriggerAction = new Gui::PrefCheckBox(this);
    layoutPiemenu->addWidget(PieMenuTriggerAction, 6, 0);
    PieMenuTriggerAction->setChecked(Gui::ViewParams::defaultPieMenuTriggerAction());
    PieMenuTriggerAction->setEntryName("PieMenuTriggerAction");
    PieMenuTriggerAction->setParamGrpPath("View");

    // Auto generated code (Tools/params_utils.py:353)
    labelPieMenuAnimationDuration = new QLabel(this);
    layoutPiemenu->addWidget(labelPieMenuAnimationDuration, 7, 0);
    PieMenuAnimationDuration = new Gui::PrefSpinBox(this);
    layoutPiemenu->addWidget(PieMenuAnimationDuration, 7, 1);
    PieMenuAnimationDuration->setValue(Gui::ViewParams::defaultPieMenuAnimationDuration());
    PieMenuAnimationDuration->setEntryName("PieMenuAnimationDuration");
    PieMenuAnimationDuration->setParamGrpPath("View");
    // Auto generated code (Tools/params_utils.py:948)
    PieMenuAnimationDuration->setMinimum(0);
    PieMenuAnimationDuration->setMaximum(5000);
    PieMenuAnimationDuration->setSingleStep(100);

    // Auto generated code (Tools/params_utils.py:353)
    labelPieMenuAnimationCurve = new QLabel(this);
    layoutPiemenu->addWidget(labelPieMenuAnimationCurve, 8, 0);
    PieMenuAnimationCurve = new Gui::PrefComboBox(this);
    layoutPiemenu->addWidget(PieMenuAnimationCurve, 8, 1);
    PieMenuAnimationCurve->setEntryName("PieMenuAnimationCurve");
    PieMenuAnimationCurve->setParamGrpPath("View");
    // Auto generated code (Gui/ViewParams.py:126)
    for (const auto &item : ViewParams::AnimationCurveTypes)
        PieMenuAnimationCurve->addItem(item);
    PieMenuAnimationCurve->setCurrentIndex(Gui::ViewParams::defaultPieMenuAnimationCurve());

    // Auto generated code (Tools/params_utils.py:353)
    PieMenuPopup = new Gui::PrefCheckBox(this);
    layoutPiemenu->addWidget(PieMenuPopup, 9, 0);
    PieMenuPopup->setChecked(Gui::ViewParams::defaultPieMenuPopup());
    PieMenuPopup->setEntryName("PieMenuPopup");
    PieMenuPopup->setParamGrpPath("View");


    // Auto generated code (Tools/params_utils.py:342)
    groupOverlay = new QGroupBox(this);
    layout->addWidget(groupOverlay);
    auto layoutHorizOverlay = new QHBoxLayout(groupOverlay);
    auto layoutOverlay = new QGridLayout();
    layoutHorizOverlay->addLayout(layoutOverlay);
    layoutHorizOverlay->addStretch();

    // Auto generated code (Tools/params_utils.py:353)
    DockOverlayHideTabBar = new Gui::PrefCheckBox(this);
    layoutOverlay->addWidget(DockOverlayHideTabBar, 0, 0);
    DockOverlayHideTabBar->setChecked(Gui::ViewParams::defaultDockOverlayHideTabBar());
    DockOverlayHideTabBar->setEntryName("DockOverlayHideTabBar");
    DockOverlayHideTabBar->setParamGrpPath("View");

    // Auto generated code (Tools/params_utils.py:353)
    DockOverlayHidePropertyViewScrollBar = new Gui::PrefCheckBox(this);
    layoutOverlay->addWidget(DockOverlayHidePropertyViewScrollBar, 1, 0);
    DockOverlayHidePropertyViewScrollBar->setChecked(Gui::ViewParams::defaultDockOverlayHidePropertyViewScrollBar());
    DockOverlayHidePropertyViewScrollBar->setEntryName("DockOverlayHidePropertyViewScrollBar");
    DockOverlayHidePropertyViewScrollBar->setParamGrpPath("View");

    // Auto generated code (Tools/params_utils.py:353)
    DockOverlayAutoView = new Gui::PrefCheckBox(this);
    layoutOverlay->addWidget(DockOverlayAutoView, 2, 0);
    DockOverlayAutoView->setChecked(Gui::ViewParams::defaultDockOverlayAutoView());
    DockOverlayAutoView->setEntryName("DockOverlayAutoView");
    DockOverlayAutoView->setParamGrpPath("View");

    // Auto generated code (Tools/params_utils.py:353)
    DockOverlayAutoMouseThrough = new Gui::PrefCheckBox(this);
    layoutOverlay->addWidget(DockOverlayAutoMouseThrough, 3, 0);
    DockOverlayAutoMouseThrough->setChecked(Gui::ViewParams::defaultDockOverlayAutoMouseThrough());
    DockOverlayAutoMouseThrough->setEntryName("DockOverlayAutoMouseThrough");
    DockOverlayAutoMouseThrough->setParamGrpPath("View");

    // Auto generated code (Tools/params_utils.py:353)
    DockOverlayWheelPassThrough = new Gui::PrefCheckBox(this);
    layoutOverlay->addWidget(DockOverlayWheelPassThrough, 4, 0);
    DockOverlayWheelPassThrough->setChecked(Gui::ViewParams::defaultDockOverlayWheelPassThrough());
    DockOverlayWheelPassThrough->setEntryName("DockOverlayWheelPassThrough");
    DockOverlayWheelPassThrough->setParamGrpPath("View");

    // Auto generated code (Tools/params_utils.py:353)
    labelDockOverlayWheelDelay = new QLabel(this);
    layoutOverlay->addWidget(labelDockOverlayWheelDelay, 5, 0);
    DockOverlayWheelDelay = new Gui::PrefSpinBox(this);
    layoutOverlay->addWidget(DockOverlayWheelDelay, 5, 1);
    DockOverlayWheelDelay->setValue(Gui::ViewParams::defaultDockOverlayWheelDelay());
    DockOverlayWheelDelay->setEntryName("DockOverlayWheelDelay");
    DockOverlayWheelDelay->setParamGrpPath("View");
    // Auto generated code (Tools/params_utils.py:948)
    DockOverlayWheelDelay->setMinimum(0);
    DockOverlayWheelDelay->setMaximum(99999);
    DockOverlayWheelDelay->setSingleStep(1);

    // Auto generated code (Tools/params_utils.py:353)
    labelDockOverlayAlphaRadius = new QLabel(this);
    layoutOverlay->addWidget(labelDockOverlayAlphaRadius, 6, 0);
    DockOverlayAlphaRadius = new Gui::PrefSpinBox(this);
    layoutOverlay->addWidget(DockOverlayAlphaRadius, 6, 1);
    DockOverlayAlphaRadius->setValue(Gui::ViewParams::defaultDockOverlayAlphaRadius());
    DockOverlayAlphaRadius->setEntryName("DockOverlayAlphaRadius");
    DockOverlayAlphaRadius->setParamGrpPath("View");
    // Auto generated code (Tools/params_utils.py:948)
    DockOverlayAlphaRadius->setMinimum(1);
    DockOverlayAlphaRadius->setMaximum(100);
    DockOverlayAlphaRadius->setSingleStep(1);

    // Auto generated code (Tools/params_utils.py:353)
    DockOverlayCheckNaviCube = new Gui::PrefCheckBox(this);
    layoutOverlay->addWidget(DockOverlayCheckNaviCube, 7, 0);
    DockOverlayCheckNaviCube->setChecked(Gui::ViewParams::defaultDockOverlayCheckNaviCube());
    DockOverlayCheckNaviCube->setEntryName("DockOverlayCheckNaviCube");
    DockOverlayCheckNaviCube->setParamGrpPath("View");

    // Auto generated code (Tools/params_utils.py:353)
    labelDockOverlayHintTriggerSize = new QLabel(this);
    layoutOverlay->addWidget(labelDockOverlayHintTriggerSize, 8, 0);
    DockOverlayHintTriggerSize = new Gui::PrefSpinBox(this);
    layoutOverlay->addWidget(DockOverlayHintTriggerSize, 8, 1);
    DockOverlayHintTriggerSize->setValue(Gui::ViewParams::defaultDockOverlayHintTriggerSize());
    DockOverlayHintTriggerSize->setEntryName("DockOverlayHintTriggerSize");
    DockOverlayHintTriggerSize->setParamGrpPath("View");
    // Auto generated code (Tools/params_utils.py:948)
    DockOverlayHintTriggerSize->setMinimum(1);
    DockOverlayHintTriggerSize->setMaximum(100);
    DockOverlayHintTriggerSize->setSingleStep(1);

    // Auto generated code (Tools/params_utils.py:353)
    labelDockOverlayHintSize = new QLabel(this);
    layoutOverlay->addWidget(labelDockOverlayHintSize, 9, 0);
    DockOverlayHintSize = new Gui::PrefSpinBox(this);
    layoutOverlay->addWidget(DockOverlayHintSize, 9, 1);
    DockOverlayHintSize->setValue(Gui::ViewParams::defaultDockOverlayHintSize());
    DockOverlayHintSize->setEntryName("DockOverlayHintSize");
    DockOverlayHintSize->setParamGrpPath("View");
    // Auto generated code (Tools/params_utils.py:948)
    DockOverlayHintSize->setMinimum(1);
    DockOverlayHintSize->setMaximum(100);
    DockOverlayHintSize->setSingleStep(1);

    // Auto generated code (Tools/params_utils.py:353)
    DockOverlayHintTabBar = new Gui::PrefCheckBox(this);
    layoutOverlay->addWidget(DockOverlayHintTabBar, 10, 0);
    DockOverlayHintTabBar->setChecked(Gui::ViewParams::defaultDockOverlayHintTabBar());
    DockOverlayHintTabBar->setEntryName("DockOverlayHintTabBar");
    DockOverlayHintTabBar->setParamGrpPath("View");

    // Auto generated code (Tools/params_utils.py:353)
    labelDockOverlayHintDelay = new QLabel(this);
    layoutOverlay->addWidget(labelDockOverlayHintDelay, 11, 0);
    DockOverlayHintDelay = new Gui::PrefSpinBox(this);
    layoutOverlay->addWidget(DockOverlayHintDelay, 11, 1);
    DockOverlayHintDelay->setValue(Gui::ViewParams::defaultDockOverlayHintDelay());
    DockOverlayHintDelay->setEntryName("DockOverlayHintDelay");
    DockOverlayHintDelay->setParamGrpPath("View");
    // Auto generated code (Tools/params_utils.py:948)
    DockOverlayHintDelay->setMinimum(0);
    DockOverlayHintDelay->setMaximum(1000);
    DockOverlayHintDelay->setSingleStep(100);

    // Auto generated code (Tools/params_utils.py:353)
    labelDockOverlaySplitterHandleTimeout = new QLabel(this);
    layoutOverlay->addWidget(labelDockOverlaySplitterHandleTimeout, 12, 0);
    DockOverlaySplitterHandleTimeout = new Gui::PrefSpinBox(this);
    layoutOverlay->addWidget(DockOverlaySplitterHandleTimeout, 12, 1);
    DockOverlaySplitterHandleTimeout->setValue(Gui::ViewParams::defaultDockOverlaySplitterHandleTimeout());
    DockOverlaySplitterHandleTimeout->setEntryName("DockOverlaySplitterHandleTimeout");
    DockOverlaySplitterHandleTimeout->setParamGrpPath("View");
    // Auto generated code (Tools/params_utils.py:948)
    DockOverlaySplitterHandleTimeout->setMinimum(0);
    DockOverlaySplitterHandleTimeout->setMaximum(99999);
    DockOverlaySplitterHandleTimeout->setSingleStep(100);

    // Auto generated code (Tools/params_utils.py:353)
    DockOverlayActivateOnHover = new Gui::PrefCheckBox(this);
    layoutOverlay->addWidget(DockOverlayActivateOnHover, 13, 0);
    DockOverlayActivateOnHover->setChecked(Gui::ViewParams::defaultDockOverlayActivateOnHover());
    DockOverlayActivateOnHover->setEntryName("DockOverlayActivateOnHover");
    DockOverlayActivateOnHover->setParamGrpPath("View");

    // Auto generated code (Tools/params_utils.py:353)
    labelDockOverlayDelay = new QLabel(this);
    layoutOverlay->addWidget(labelDockOverlayDelay, 14, 0);
    DockOverlayDelay = new Gui::PrefSpinBox(this);
    layoutOverlay->addWidget(DockOverlayDelay, 14, 1);
    DockOverlayDelay->setValue(Gui::ViewParams::defaultDockOverlayDelay());
    DockOverlayDelay->setEntryName("DockOverlayDelay");
    DockOverlayDelay->setParamGrpPath("View");
    // Auto generated code (Tools/params_utils.py:948)
    DockOverlayDelay->setMinimum(0);
    DockOverlayDelay->setMaximum(5000);
    DockOverlayDelay->setSingleStep(100);

    // Auto generated code (Tools/params_utils.py:353)
    labelDockOverlayAnimationDuration = new QLabel(this);
    layoutOverlay->addWidget(labelDockOverlayAnimationDuration, 15, 0);
    DockOverlayAnimationDuration = new Gui::PrefSpinBox(this);
    layoutOverlay->addWidget(DockOverlayAnimationDuration, 15, 1);
    DockOverlayAnimationDuration->setValue(Gui::ViewParams::defaultDockOverlayAnimationDuration());
    DockOverlayAnimationDuration->setEntryName("DockOverlayAnimationDuration");
    DockOverlayAnimationDuration->setParamGrpPath("View");
    // Auto generated code (Tools/params_utils.py:948)
    DockOverlayAnimationDuration->setMinimum(0);
    DockOverlayAnimationDuration->setMaximum(5000);
    DockOverlayAnimationDuration->setSingleStep(100);

    // Auto generated code (Tools/params_utils.py:353)
    labelDockOverlayAnimationCurve = new QLabel(this);
    layoutOverlay->addWidget(labelDockOverlayAnimationCurve, 16, 0);
    DockOverlayAnimationCurve = new Gui::PrefComboBox(this);
    layoutOverlay->addWidget(DockOverlayAnimationCurve, 16, 1);
    DockOverlayAnimationCurve->setEntryName("DockOverlayAnimationCurve");
    DockOverlayAnimationCurve->setParamGrpPath("View");
    // Auto generated code (Gui/ViewParams.py:126)
    for (const auto &item : ViewParams::AnimationCurveTypes)
        DockOverlayAnimationCurve->addItem(item);
    DockOverlayAnimationCurve->setCurrentIndex(Gui::ViewParams::defaultDockOverlayAnimationCurve());
    layout->addItem(new QSpacerItem(40, 20, QSizePolicy::Fixed, QSizePolicy::Expanding));
    retranslateUi();
    // Auto generated code (Tools/params_utils.py:499)
    init();
}

// Auto generated code (Tools/params_utils.py:504)
DlgSettingsUI::~DlgSettingsUI()
{
}

// Auto generated code (Tools/params_utils.py:510)
void DlgSettingsUI::saveSettings()
{
    // Auto generated code (Tools/params_utils.py:373)
    TextCursorWidth->onSave();
    ItemBackground->onSave();
    ItemBackgroundPadding->onSave();
    ResizableColumn->onSave();
    CheckBoxesSelection->onSave();
    HideColumn->onSave();
    HideScrollBar->onSave();
    HideHeaderView->onSave();
    TreeToolTipIcon->onSave();
    AutoHideEditorIcon->onSave();
    EditorTrigger->onSave();
    NoSystemBackground->onSave();
    EditDialogBGAlpha->onSave();
    PieMenuIconSize->onSave();
    PieMenuRadius->onSave();
    PieMenuTriggerRadius->onSave();
    PieMenuCenterRadius->onSave();
    PieMenuFontSize->onSave();
    PieMenuTriggerDelay->onSave();
    PieMenuTriggerAction->onSave();
    PieMenuAnimationDuration->onSave();
    PieMenuAnimationCurve->onSave();
    PieMenuPopup->onSave();
    DockOverlayHideTabBar->onSave();
    DockOverlayHidePropertyViewScrollBar->onSave();
    DockOverlayAutoView->onSave();
    DockOverlayAutoMouseThrough->onSave();
    DockOverlayWheelPassThrough->onSave();
    DockOverlayWheelDelay->onSave();
    DockOverlayAlphaRadius->onSave();
    DockOverlayCheckNaviCube->onSave();
    DockOverlayHintTriggerSize->onSave();
    DockOverlayHintSize->onSave();
    DockOverlayHintTabBar->onSave();
    DockOverlayHintDelay->onSave();
    DockOverlaySplitterHandleTimeout->onSave();
    DockOverlayActivateOnHover->onSave();
    DockOverlayDelay->onSave();
    DockOverlayAnimationDuration->onSave();
    DockOverlayAnimationCurve->onSave();
}

// Auto generated code (Tools/params_utils.py:517)
void DlgSettingsUI::loadSettings()
{
    // Auto generated code (Tools/params_utils.py:364)
    TextCursorWidth->onRestore();
    ItemBackground->onRestore();
    ItemBackgroundPadding->onRestore();
    ResizableColumn->onRestore();
    CheckBoxesSelection->onRestore();
    HideColumn->onRestore();
    HideScrollBar->onRestore();
    HideHeaderView->onRestore();
    TreeToolTipIcon->onRestore();
    AutoHideEditorIcon->onRestore();
    EditorTrigger->onRestore();
    NoSystemBackground->onRestore();
    EditDialogBGAlpha->onRestore();
    PieMenuIconSize->onRestore();
    PieMenuRadius->onRestore();
    PieMenuTriggerRadius->onRestore();
    PieMenuCenterRadius->onRestore();
    PieMenuFontSize->onRestore();
    PieMenuTriggerDelay->onRestore();
    PieMenuTriggerAction->onRestore();
    PieMenuAnimationDuration->onRestore();
    PieMenuAnimationCurve->onRestore();
    PieMenuPopup->onRestore();
    DockOverlayHideTabBar->onRestore();
    DockOverlayHidePropertyViewScrollBar->onRestore();
    DockOverlayAutoView->onRestore();
    DockOverlayAutoMouseThrough->onRestore();
    DockOverlayWheelPassThrough->onRestore();
    DockOverlayWheelDelay->onRestore();
    DockOverlayAlphaRadius->onRestore();
    DockOverlayCheckNaviCube->onRestore();
    DockOverlayHintTriggerSize->onRestore();
    DockOverlayHintSize->onRestore();
    DockOverlayHintTabBar->onRestore();
    DockOverlayHintDelay->onRestore();
    DockOverlaySplitterHandleTimeout->onRestore();
    DockOverlayActivateOnHover->onRestore();
    DockOverlayDelay->onRestore();
    DockOverlayAnimationDuration->onRestore();
    DockOverlayAnimationCurve->onRestore();
}

// Auto generated code (Tools/params_utils.py:524)
void DlgSettingsUI::retranslateUi()
{
    setWindowTitle(QObject::tr("UI"));
    groupGeneral->setTitle(QObject::tr("General"));
    TextCursorWidth->setToolTip(QApplication::translate("ViewParams", Gui::ViewParams::docTextCursorWidth()));
    labelTextCursorWidth->setText(QObject::tr("Text cursor width"));
    labelTextCursorWidth->setToolTip(TextCursorWidth->toolTip());
    groupTreeview->setTitle(QObject::tr("Tree view"));
    ItemBackground->setToolTip(QApplication::translate("TreeParams", Gui::TreeParams::docItemBackground()));
    labelItemBackground->setText(QObject::tr("Item background color"));
    labelItemBackground->setToolTip(ItemBackground->toolTip());
    ItemBackgroundPadding->setToolTip(QApplication::translate("TreeParams", Gui::TreeParams::docItemBackgroundPadding()));
    labelItemBackgroundPadding->setText(QObject::tr("Item background padding"));
    labelItemBackgroundPadding->setToolTip(ItemBackgroundPadding->toolTip());
    ResizableColumn->setToolTip(QApplication::translate("TreeParams", Gui::TreeParams::docResizableColumn()));
    ResizableColumn->setText(QObject::tr("Resizable columns"));
    CheckBoxesSelection->setToolTip(QApplication::translate("TreeParams", Gui::TreeParams::docCheckBoxesSelection()));
    CheckBoxesSelection->setText(QObject::tr("Show item checkbox"));
    HideColumn->setToolTip(QApplication::translate("TreeParams", Gui::TreeParams::docHideColumn()));
    HideColumn->setText(QObject::tr("Hide extra column"));
    HideScrollBar->setToolTip(QApplication::translate("TreeParams", Gui::TreeParams::docHideScrollBar()));
    HideScrollBar->setText(QObject::tr("Hide scroll bar"));
    HideHeaderView->setToolTip(QApplication::translate("TreeParams", Gui::TreeParams::docHideHeaderView()));
    HideHeaderView->setText(QObject::tr("Hide header"));
    TreeToolTipIcon->setToolTip(QApplication::translate("TreeParams", Gui::TreeParams::docTreeToolTipIcon()));
    TreeToolTipIcon->setText(QObject::tr("Show icon in tool tip"));
    groupExpression->setTitle(QObject::tr("Expression"));
    AutoHideEditorIcon->setToolTip(QApplication::translate("ExprParams", Gui::ExprParams::docAutoHideEditorIcon()));
    AutoHideEditorIcon->setText(QObject::tr("Auto hide editor icon"));
    EditorTrigger->setToolTip(QApplication::translate("ExprParams", Gui::ExprParams::docEditorTrigger()));
    labelEditorTrigger->setText(QObject::tr("Editor trigger shortcut"));
    labelEditorTrigger->setToolTip(EditorTrigger->toolTip());
    NoSystemBackground->setToolTip(QApplication::translate("ExprParams", Gui::ExprParams::docNoSystemBackground()));
    NoSystemBackground->setText(QObject::tr("In place editing"));
    EditDialogBGAlpha->setToolTip(QApplication::translate("ExprParams", Gui::ExprParams::docEditDialogBGAlpha()));
    labelEditDialogBGAlpha->setText(QObject::tr("Background opacity"));
    labelEditDialogBGAlpha->setToolTip(EditDialogBGAlpha->toolTip());
    groupPiemenu->setTitle(QObject::tr("Pie menu"));
    PieMenuIconSize->setToolTip(QApplication::translate("ViewParams", Gui::ViewParams::docPieMenuIconSize()));
    labelPieMenuIconSize->setText(QObject::tr("Icon size"));
    labelPieMenuIconSize->setToolTip(PieMenuIconSize->toolTip());
    PieMenuRadius->setToolTip(QApplication::translate("ViewParams", Gui::ViewParams::docPieMenuRadius()));
    labelPieMenuRadius->setText(QObject::tr("Radius"));
    labelPieMenuRadius->setToolTip(PieMenuRadius->toolTip());
    PieMenuTriggerRadius->setToolTip(QApplication::translate("ViewParams", Gui::ViewParams::docPieMenuTriggerRadius()));
    labelPieMenuTriggerRadius->setText(QObject::tr("Trigger radius"));
    labelPieMenuTriggerRadius->setToolTip(PieMenuTriggerRadius->toolTip());
    PieMenuCenterRadius->setToolTip(QApplication::translate("ViewParams", Gui::ViewParams::docPieMenuCenterRadius()));
    labelPieMenuCenterRadius->setText(QObject::tr("Center radius"));
    labelPieMenuCenterRadius->setToolTip(PieMenuCenterRadius->toolTip());
    PieMenuFontSize->setToolTip(QApplication::translate("ViewParams", Gui::ViewParams::docPieMenuFontSize()));
    labelPieMenuFontSize->setText(QObject::tr("Font size"));
    labelPieMenuFontSize->setToolTip(PieMenuFontSize->toolTip());
    PieMenuTriggerDelay->setToolTip(QApplication::translate("ViewParams", Gui::ViewParams::docPieMenuTriggerDelay()));
    labelPieMenuTriggerDelay->setText(QObject::tr("Trigger delay (ms)"));
    labelPieMenuTriggerDelay->setToolTip(PieMenuTriggerDelay->toolTip());
    PieMenuTriggerAction->setToolTip(QApplication::translate("ViewParams", Gui::ViewParams::docPieMenuTriggerAction()));
    PieMenuTriggerAction->setText(QObject::tr("Trigger action"));
    PieMenuAnimationDuration->setToolTip(QApplication::translate("ViewParams", Gui::ViewParams::docPieMenuAnimationDuration()));
    labelPieMenuAnimationDuration->setText(QObject::tr("Animation duration (ms)"));
    labelPieMenuAnimationDuration->setToolTip(PieMenuAnimationDuration->toolTip());
    PieMenuAnimationCurve->setToolTip(QApplication::translate("ViewParams", Gui::ViewParams::docPieMenuAnimationCurve()));
    labelPieMenuAnimationCurve->setText(QObject::tr("Animation curve type"));
    labelPieMenuAnimationCurve->setToolTip(PieMenuAnimationCurve->toolTip());
    PieMenuPopup->setToolTip(QApplication::translate("ViewParams", Gui::ViewParams::docPieMenuPopup()));
    PieMenuPopup->setText(QObject::tr("Show pie menu as popup"));
    groupOverlay->setTitle(QObject::tr("Overlay"));
    DockOverlayHideTabBar->setToolTip(QApplication::translate("ViewParams", Gui::ViewParams::docDockOverlayHideTabBar()));
    DockOverlayHideTabBar->setText(QObject::tr("Hide tab bar"));
    DockOverlayHidePropertyViewScrollBar->setToolTip(QApplication::translate("ViewParams", Gui::ViewParams::docDockOverlayHidePropertyViewScrollBar()));
    DockOverlayHidePropertyViewScrollBar->setText(QObject::tr("Hide property view scroll bar"));
    DockOverlayAutoView->setToolTip(QApplication::translate("ViewParams", Gui::ViewParams::docDockOverlayAutoView()));
    DockOverlayAutoView->setText(QObject::tr("Auto hide in non 3D view"));
    DockOverlayAutoMouseThrough->setToolTip(QApplication::translate("ViewParams", Gui::ViewParams::docDockOverlayAutoMouseThrough()));
    DockOverlayAutoMouseThrough->setText(QObject::tr("Auto mouse pass through"));
    DockOverlayWheelPassThrough->setToolTip(QApplication::translate("ViewParams", Gui::ViewParams::docDockOverlayWheelPassThrough()));
    DockOverlayWheelPassThrough->setText(QObject::tr("Auto mouse wheel pass through"));
    DockOverlayWheelDelay->setToolTip(QApplication::translate("ViewParams", Gui::ViewParams::docDockOverlayWheelDelay()));
    labelDockOverlayWheelDelay->setText(QObject::tr("Delay mouse wheel pass through (ms)"));
    labelDockOverlayWheelDelay->setToolTip(DockOverlayWheelDelay->toolTip());
    DockOverlayAlphaRadius->setToolTip(QApplication::translate("ViewParams", Gui::ViewParams::docDockOverlayAlphaRadius()));
    labelDockOverlayAlphaRadius->setText(QObject::tr("Alpha test radius"));
    labelDockOverlayAlphaRadius->setToolTip(DockOverlayAlphaRadius->toolTip());
    DockOverlayCheckNaviCube->setToolTip(QApplication::translate("ViewParams", Gui::ViewParams::docDockOverlayCheckNaviCube()));
    DockOverlayCheckNaviCube->setText(QObject::tr("Check Navigation Cube"));
    DockOverlayHintTriggerSize->setToolTip(QApplication::translate("ViewParams", Gui::ViewParams::docDockOverlayHintTriggerSize()));
    labelDockOverlayHintTriggerSize->setText(QObject::tr("Hint trigger size"));
    labelDockOverlayHintTriggerSize->setToolTip(DockOverlayHintTriggerSize->toolTip());
    DockOverlayHintSize->setToolTip(QApplication::translate("ViewParams", Gui::ViewParams::docDockOverlayHintSize()));
    labelDockOverlayHintSize->setText(QObject::tr("Hint width"));
    labelDockOverlayHintSize->setToolTip(DockOverlayHintSize->toolTip());
    DockOverlayHintTabBar->setToolTip(QApplication::translate("ViewParams", Gui::ViewParams::docDockOverlayHintTabBar()));
    DockOverlayHintTabBar->setText(QObject::tr("Hint show tab bar"));
    DockOverlayHintDelay->setToolTip(QApplication::translate("ViewParams", Gui::ViewParams::docDockOverlayHintDelay()));
    labelDockOverlayHintDelay->setText(QObject::tr("Hint delay (ms)"));
    labelDockOverlayHintDelay->setToolTip(DockOverlayHintDelay->toolTip());
    DockOverlaySplitterHandleTimeout->setToolTip(QApplication::translate("ViewParams", Gui::ViewParams::docDockOverlaySplitterHandleTimeout()));
    labelDockOverlaySplitterHandleTimeout->setText(QObject::tr("Splitter auto hide delay (ms)"));
    labelDockOverlaySplitterHandleTimeout->setToolTip(DockOverlaySplitterHandleTimeout->toolTip());
    DockOverlayActivateOnHover->setToolTip(QApplication::translate("ViewParams", Gui::ViewParams::docDockOverlayActivateOnHover()));
    DockOverlayActivateOnHover->setText(QObject::tr("Activate on hover"));
    DockOverlayDelay->setToolTip(QApplication::translate("ViewParams", Gui::ViewParams::docDockOverlayDelay()));
    labelDockOverlayDelay->setText(QObject::tr("Layout delay (ms)"));
    labelDockOverlayDelay->setToolTip(DockOverlayDelay->toolTip());
    DockOverlayAnimationDuration->setToolTip(QApplication::translate("ViewParams", Gui::ViewParams::docDockOverlayAnimationDuration()));
    labelDockOverlayAnimationDuration->setText(QObject::tr("Animation duration (ms)"));
    labelDockOverlayAnimationDuration->setToolTip(DockOverlayAnimationDuration->toolTip());
    DockOverlayAnimationCurve->setToolTip(QApplication::translate("ViewParams", Gui::ViewParams::docDockOverlayAnimationCurve()));
    labelDockOverlayAnimationCurve->setText(QObject::tr("Animation curve type"));
    labelDockOverlayAnimationCurve->setToolTip(DockOverlayAnimationCurve->toolTip());
}

// Auto generated code (Tools/params_utils.py:537)
void DlgSettingsUI::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
    QWidget::changeEvent(e);
}

// Auto generated code (Tools/params_utils.py:548)
#include "moc_DlgSettingsUI.cpp"
//[[[end]]]

// -----------------------------------------------------------------------------------
// user code start

void DlgSettingsUI::init()
{
    timer = new QTimer(this);
    timer->setSingleShot(true);

    animator1 = new QPropertyAnimation(this, "offset1", this);
    QObject::connect(animator1, &QPropertyAnimation::stateChanged, [this]() {
        if (animator1->state() != QAbstractAnimation::Running)
            timer->start(1000);
    });
    animator2 = new QPropertyAnimation(this, "offset2", this);
    QObject::connect(animator2, &QPropertyAnimation::stateChanged, [this]() {
        if (animator2->state() != QAbstractAnimation::Running)
            timer->start(1000);
    });

    QObject::connect(DockOverlayAnimationCurve, QOverload<int>::of(&QComboBox::currentIndexChanged),
                     this, &DlgSettingsUI::onCurveChange);
    QObject::connect(PieMenuAnimationCurve, QOverload<int>::of(&QComboBox::currentIndexChanged),
                     this, &DlgSettingsUI::onCurveChange);

    QObject::connect(timer, &QTimer::timeout, [=]() {
        if (animator1->state() != QAbstractAnimation::Running) {
            this->setOffset1(1);
            this->a1 = this->b1 = 0;
        }
        if (animator2->state() != QAbstractAnimation::Running) {
            this->setOffset2(0);
            this->a2 = this->b2 = 0;
        }
    });
}

qreal DlgSettingsUI::offset1() const
{
    return this->t1;
}

void DlgSettingsUI::setOffset1(qreal t)
{
    if (t == this->t1)
        return;
    this->t1 = t;
    QLabel *label = this->labelDockOverlayAnimationCurve;
    if (this->a1 == this->b1) {
        this->a1 = label->x();
        QPoint pos(width(), 0);
        this->b1 = width() - label->fontMetrics().boundingRect(label->text()).width() - 5;
    }
    label->move(this->a1 * (1-t) + this->b1 * t, label->y());
}

qreal DlgSettingsUI::offset2() const
{
    return this->t2;
}

void DlgSettingsUI::setOffset2(qreal t)
{
    if (t == this->t2)
        return;
    this->t2 = t;
    QLabel *label = this->labelPieMenuAnimationCurve;
    if (this->a2 == this->b2) {
        this->a2 = label->x();
        QPoint pos(width(), 0);
        this->b2 = width() - label->fontMetrics().boundingRect(label->text()).width();
    }
    label->move(this->a2 * (1-t) + this->b2 * t, label->y());
}

void DlgSettingsUI::onCurveChange(int index)
{
    auto animator = sender() == DockOverlayAnimationCurve ? animator1 : animator2;
    animator->setStartValue(0.0);
    animator->setEndValue(1.0);
    animator->setEasingCurve((QEasingCurve::Type)index);
    animator->setDuration(animator == animator1 ?
            DockOverlayAnimationDuration->value()*2 : PieMenuAnimationDuration->value()*2);
    animator->start();
}

// user code end
// -----------------------------------------------------------------------------------
