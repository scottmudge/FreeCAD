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

#ifndef _PreComp_
# include <QLabel>
# include <QCheckBox>
# include <QSpinBox>
# include <QApplication>
# include <QGridLayout>
# include <QVBoxLayout>
# include <QHBoxLayout>
# include <QGroupBox>
# include <QComboBox>
# include <QTimer>
#endif

#include <QPropertyAnimation>

#include "DlgSettingsUI.h"
#include "ViewParams.h"
#include "ExprParams.h"
#include "TreeParams.h"
#include "Widgets.h"

using namespace Gui::Dialog;

/* TRANSLATOR Gui::Dialog::DlgSettingsUI */

class DlgSettingsUI::Private
{
public:
#define FC_GENERAL_PARAMS \
    FC_UI_SPINBOX(ViewParams, TextCursorWidth, "Text cursor width", 1, 100, 1) \

#define FC_TREEVIEW_PARAMS \
    FC_UI_BRUSH(TreeParams, ItemBackground, "Item background color") \
    FC_UI_SPINBOX(TreeParams, ItemBackgroundPadding, "Item background padding", 0, 100, 1) \
    FC_UI_CHECKBOX(TreeParams, ResizableColumn,"Resizable columns") \
    FC_UI_CHECKBOX(TreeParams, CheckBoxesSelection,"Show item checkbox") \
    FC_UI_CHECKBOX(TreeParams, HideColumn, "Hide extra column") \
    FC_UI_CHECKBOX(TreeParams, ShowGenericAuxNames, "Show generic auxillary group names") \
    FC_UI_CHECKBOX(TreeParams, HideScrollBar,"Hide scroll bar") \
    FC_UI_CHECKBOX(TreeParams, HideHeaderView,"Hide header") \

#define FC_EXPRESSION_PARAMS \
    FC_UI_CHECKBOX(ExprParams, AutoHideEditorIcon, "Auto hide editor icon") \
    FC_UI_LINEEDIT(ExprParams, EditorTrigger, "Editor trigger shortcut") \
    FC_UI_CHECKBOX(ExprParams, NoSystemBackground, "In place editing") \
    FC_UI_CHECKBOX(ExprParams, EditDialogBGAlpha, "Background opacity") \

#define FC_PIEMENU_PARAMS \
    FC_UI_SPINBOX(ViewParams, PieMenuIconSize, "Icon size", 0, 64, 1) \
    FC_UI_SPINBOX(ViewParams, PieMenuRadius, "Radius", 10, 500, 10) \
    FC_UI_SPINBOX(ViewParams, PieMenuTriggerRadius, "Trigger radius", 10, 500, 10) \
    FC_UI_SPINBOX(ViewParams, PieMenuCenterRadius, "Center radius", 0, 250, 1) \
    FC_UI_SPINBOX(ViewParams, PieMenuFontSize, "Font size", 0, 32, 1) \
    FC_UI_SPINBOX(ViewParams, PieMenuTriggerDelay, "Trigger delay (ms)", 0, 10000, 100) \
    FC_UI_CHECKBOX(ViewParams, PieMenuTriggerAction, "Trigger action") \
    FC_UI_SPINBOX(ViewParams, PieMenuAnimationDuration, "Animation duration (ms)", 0, 5000, 100) \
    FC_UI_COMBOBOX(ViewParams, PieMenuAnimationCurve, "Animation curve type") \
    FC_UI_CHECKBOX(ViewParams, PieMenuPopup, "Show pie menu as popup") \

#define FC_OVERLAY_PARAMS \
    FC_UI_CHECKBOX(ViewParams, DockOverlayHideTabBar,"Hide tab bar") \
    FC_UI_CHECKBOX(ViewParams, DockOverlayHidePropertyViewScrollBar,"Hide property view scroll bar") \
    FC_UI_CHECKBOX(ViewParams, DockOverlayAutoView, "Auto hide in non 3D view") \
    FC_UI_CHECKBOX(ViewParams, DockOverlayAutoMouseThrough, "Auto mouse pass through") \
    FC_UI_CHECKBOX(ViewParams, DockOverlayWheelPassThrough, "Auto mouse wheel pass through") \
    FC_UI_SPINBOX(ViewParams, DockOverlayWheelDelay, "Delay mouse wheel pass through (ms)", 0, 99999, 1) \
    FC_UI_SPINBOX(ViewParams, DockOverlayAlphaRadius, "Alpha test radius", 1, 100, 1) \
    FC_UI_CHECKBOX(ViewParams, DockOverlayCheckNaviCube, "Check Navigation Cube") \
    FC_UI_SPINBOX(ViewParams, DockOverlayHintTriggerSize, "Hint trigger size", 1, 100, 1) \
    FC_UI_SPINBOX(ViewParams, DockOverlayHintSize, "Hint width", 1, 100, 1) \
    FC_UI_CHECKBOX(ViewParams, DockOverlayHintTabBar, "Hint show tab bar") \
    FC_UI_SPINBOX(ViewParams, DockOverlayHintDelay, "Hint delay (ms)", 0, 1000, 100) \
    FC_UI_SPINBOX(ViewParams, DockOverlaySplitterHandleTimeout, "Splitter auto hide delay (ms)", 0, 99999, 100) \
    FC_UI_CHECKBOX(ViewParams, DockOverlayActivateOnHover,"Activate on hover") \
    FC_UI_SPINBOX(ViewParams, DockOverlayDelay, "Layout delay (ms)", 0, 5000, 100) \
    FC_UI_SPINBOX(ViewParams, DockOverlayAnimationDuration, "Animation duration (ms)", 0, 5000, 100) \
    FC_UI_COMBOBOX(ViewParams, DockOverlayAnimationCurve, "Animation curve type") \

#define FC_UI_PARAMS \
    FC_GENERAL_PARAMS \
    FC_TREEVIEW_PARAMS \
    FC_EXPRESSION_PARAMS \
    FC_PIEMENU_PARAMS \
    FC_OVERLAY_PARAMS

#define FC_UI_CHECKBOX(_params, _name, _label) \
    FC_UI_PARAM(_params, _name, _label, QCheckBox, isChecked, setChecked)

#define FC_UI_BRUSH(_params, _name, _label) \
    FC_UI_PARAM(_params, _name, _label, Gui::TransparentColorButton, packedColor, setPackedColor)

#define FC_UI_LINEEDIT(_params, _name, _label) \
    FC_UI_PARAM(_params, _name, _label, Gui::AccelLineEdit, latinText, setLatinText)

#define FC_UI_COMBOBOX(_params, _name, _label) \
    FC_UI_PARAM(_params, _name, _label, QComboBox, currentIndex, setCurrentIndex)

#define FC_UI_SPINBOX(_params, _name, _label, _min, _max, _step) \
    FC_UI_PARAM(_params, _name, _label, QSpinBox, value, setValue)

#undef FC_UI_PARAM
#define FC_UI_PARAM(_params, _name, _label, _type, _getter, _setter) \
    _type * _name = nullptr;\
    QLabel *label##_name = nullptr;

    FC_UI_PARAMS

#undef FC_UI_PARAM
#define FC_UI_PARAM(_params, _name, _label, _type, _getter, _setter) do {\
        label##_name->setText(tr(_label));\
        QString tooltip = QApplication::translate(#_params, _params::doc##_name()); \
        if (!tooltip.isEmpty()) {\
            _name->setToolTip(tooltip); \
            label##_name->setToolTip(tooltip);\
        }\
    }while(0);

    void init()
    {
        FC_UI_PARAMS
    }

    QTimer timer;

    QPropertyAnimation *animator1;
    qreal t1 = 0;
    qreal a1 = 0, b1 = 0;
    QPropertyAnimation *animator2;
    qreal t2 = 0;
    qreal a2 = 0, b2 = 0;
};

DlgSettingsUI::DlgSettingsUI(QWidget* parent)
    : PreferencePage(parent)
    , ui(new Private)
{
    setWindowTitle(tr("UI"));

    ui->animator1 = new QPropertyAnimation(this, "offset1", this);
    connect(ui->animator1, SIGNAL(stateChanged(QAbstractAnimation::State, 
                                               QAbstractAnimation::State)),
            this, SLOT(onStateChanged()));
    ui->animator2 = new QPropertyAnimation(this, "offset2", this);
    connect(ui->animator2, SIGNAL(stateChanged(QAbstractAnimation::State, 
                                               QAbstractAnimation::State)),
            this, SLOT(onStateChanged()));

#undef FC_UI_PARAM
#define FC_UI_PARAM(_params, _name, _label, _type, _getter, _setter) do {\
        ui->_name = new _type(parent);\
        ui->label##_name = new QLabel(parent);\
        ui->label##_name->setMinimumHeight(25);\
        ui->label##_name->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);\
        layout->addWidget(ui->label##_name, row, 0);\
        layout->addWidget(ui->_name, row, 1);\
        ++row;\
    }while(0);

#undef FC_UI_SPINBOX
#define FC_UI_SPINBOX(_params, _name, _label, _min, _max, _step) \
    FC_UI_PARAM(_params, _name, _label, QSpinBox, value, setValue)\
    ui->_name->setMaximum(_max);\
    ui->_name->setMinimum(_min);\
    ui->_name->setSingleStep(_step);

    auto vlayout = new QVBoxLayout(this);
    int row;
    QGroupBox *group;
    QGridLayout *layout;

    group = new QGroupBox(tr("General"), this);
    vlayout->addWidget(group);
    layout = new QGridLayout();
    group->setLayout(layout);
    row = 0;
    FC_GENERAL_PARAMS;
    layout->addItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Fixed), row-1, 2);

    group = new QGroupBox(tr("Tree view"), this);
    vlayout->addWidget(group);
    layout = new QGridLayout();
    group->setLayout(layout);
    row = 0;
    FC_TREEVIEW_PARAMS;
    layout->addItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Fixed), row-1, 2);

    group = new QGroupBox(tr("Expression editor"), this);
    vlayout->addWidget(group);
    layout = new QGridLayout();
    group->setLayout(layout);
    row = 0;
    FC_EXPRESSION_PARAMS;
    ui->EditorTrigger->setFixedWidth(100);
    layout->addItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Fixed), row-1, 2);

    group = new QGroupBox(tr("Dockable Overlay"), this);
    vlayout->addWidget(group);
    layout = new QGridLayout();
    group->setLayout(layout);
    row = 0;
    FC_OVERLAY_PARAMS;
    layout->addItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Fixed), row-1, 2);

    group = new QGroupBox(tr("Pie Menu"), this);
    vlayout->addWidget(group);
    layout = new QGridLayout();
    group->setLayout(layout);
    row = 0;
    FC_PIEMENU_PARAMS;
    layout->addItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Fixed), row-1, 2);

    vlayout->addStretch();

    const char *_curveTypes[] = {
        "Linear",
        "InQuad", "OutQuad", "InOutQuad", "OutInQuad",
        "InCubic", "OutCubic", "InOutCubic", "OutInCubic",
        "InQuart", "OutQuart", "InOutQuart", "OutInQuart",
        "InQuint", "OutQuint", "InOutQuint", "OutInQuint",
        "InSine", "OutSine", "InOutSine", "OutInSine",
        "InExpo", "OutExpo", "InOutExpo", "OutInExpo",
        "InCirc", "OutCirc", "InOutCirc", "OutInCirc",
        "InElastic", "OutElastic", "InOutElastic", "OutInElastic",
        "InBack", "OutBack", "InOutBack", "OutInBack",
        "InBounce", "OutBounce", "InOutBounce", "OutInBounce", };

	QStringList curveTypes;
    for(unsigned i=0; i<sizeof(_curveTypes)/sizeof(_curveTypes[0]); ++i)
        curveTypes.append(QLatin1String(_curveTypes[i]));

    ui->DockOverlayAnimationCurve->addItems(curveTypes);
    ui->PieMenuAnimationCurve->addItems(curveTypes);

    ui->init();

#undef FC_UI_PARAM
#define FC_UI_PARAM(_params, _name, _label, _type, _getter, _setter) \
    ui->_name->_setter(_params::get##_name());\

#undef FC_UI_SPINBOX
#define FC_UI_SPINBOX(_params, _name, _label, _min, _max, _step) \
    FC_UI_PARAM(_params, _name, _label, QSpinBox, value, setValue)

    FC_UI_PARAMS;

    connect(ui->DockOverlayAnimationCurve, SIGNAL(currentIndexChanged(int)),
            this, SLOT(onCurrentChanged(int)));
    connect(ui->PieMenuAnimationCurve, SIGNAL(currentIndexChanged(int)),
            this, SLOT(onCurrentChanged(int)));

    ui->timer.setSingleShot(true);
    connect(&ui->timer, SIGNAL(timeout()), this, SLOT(onTimer()));
}

/** 
 *  Destroys the object and frees any allocated resources
 */
DlgSettingsUI::~DlgSettingsUI()
{
    // no need to delete child widgets, Qt does it all for us
}

void DlgSettingsUI::onStateChanged()
{
    if (sender() == ui->animator1) {
        if (ui->animator1->state() != QAbstractAnimation::Running)
            ui->timer.start(1000);
    } else if (sender() == ui->animator2) {
        if (ui->animator2->state() != QAbstractAnimation::Running)
            ui->timer.start(1000);
    }
}

void DlgSettingsUI::onTimer()
{
    if (ui->animator1->state() != QAbstractAnimation::Running) {
        setOffset1(0);
        ui->a1 = ui->b1 = 0;
    }
    if (ui->animator2->state() != QAbstractAnimation::Running) {
        setOffset2(0);
        ui->a2 = ui->b2 = 0;
    }
}

qreal DlgSettingsUI::offset1() const
{
    return ui->t1;
}

void DlgSettingsUI::setOffset1(qreal t)
{
    if (t == ui->t1)
        return;
    ui->t1 = t;
    QLabel *label = ui->labelDockOverlayAnimationCurve;
    if (ui->a1 == ui->b1) {
        ui->a1 = label->x();
        QPoint pos(width(), 0);
        ui->b1 = width() - label->fontMetrics().boundingRect(label->text()).width() - 5;
    }
    label->move(ui->a1 * (1-t) + ui->b1 * t, label->y());
}

qreal DlgSettingsUI::offset2() const
{
    return ui->t2;
}

void DlgSettingsUI::setOffset2(qreal t)
{
    if (t == ui->t2)
        return;
    ui->t2 = t;
    QLabel *label = ui->labelPieMenuAnimationCurve;
    if (ui->a2 == ui->b2) {
        ui->a2 = label->x();
        QPoint pos(width(), 0);
        ui->b2 = width() - label->fontMetrics().boundingRect(label->text()).width();
    }
    label->move(ui->a2 * (1-t) + ui->b2 * t, label->y());
}

void DlgSettingsUI::onCurrentChanged(int index)
{
    auto animator = sender() == ui->DockOverlayAnimationCurve ? ui->animator1 : ui->animator2;
    animator->setStartValue(0.0);
    animator->setEndValue(1.0);
    animator->setEasingCurve((QEasingCurve::Type)index);
    animator->setDuration(animator == ui->animator1 ?
        ui->DockOverlayAnimationDuration->value()*2 : ui->PieMenuAnimationDuration->value()*2);
    animator->start();
}

void DlgSettingsUI::loadSettings()
{
    FC_UI_PARAMS;
}

void DlgSettingsUI::saveSettings()
{
#undef FC_UI_PARAM
#define FC_UI_PARAM(_params, _name, _label, _type, _getter, _setter) \
    _params::set##_name(ui->_name->_getter());\

    FC_UI_PARAMS;
}


/**
 * Sets the strings of the subwidgets using the current language.
 */
void DlgSettingsUI::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->init();
    }
    else {
        QWidget::changeEvent(e);
    }
}

#include "moc_DlgSettingsUI.cpp"

