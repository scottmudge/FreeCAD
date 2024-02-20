/***************************************************************************
 *   Copyright (C) 2015 Alexander Golubev (Fat-Zer) <fatzer2@gmail.com>    *
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
#include <QApplication>
#include <QMessageBox>
#include <QCheckBox>
#endif

#include <Base/Tools.h>
#include <Gui/Application.h>
#include <Gui/CommandT.h>
#include <Gui/MainWindow.h>
#include <Gui/BitmapFactory.h>
#include <Gui/PrefWidgets.h>
#include <Gui/WaitCursor.h>
#include <Mod/Part/Gui/PartParams.h>
#include <Mod/PartDesign/App/FeatureAddSub.h>
#include <Mod/PartDesign/App/Body.h>

#include "TaskFeatureParameters.h"
#include "TaskSketchBasedParameters.h"

using namespace PartDesignGui;
using namespace Gui;

/*********************************************************************
 *                      Task Feature Parameters                      *
 *********************************************************************/

TaskFeatureParameters::TaskFeatureParameters(PartDesignGui::ViewProvider *vp, QWidget *parent,
                                                     const std::string& pixmapname, const QString& parname)
    : TaskBox(Gui::BitmapFactory().pixmap(pixmapname.c_str()),parname,true, parent),
      vp(vp), blockUpdate(false)
{
    Gui::Document* doc = vp->getDocument();
    this->attachDocument(doc);
    App::GetApplication().getActiveTransaction(&transactionID);
}

TaskFeatureParameters::TaskFeatureParameters(PartDesignGui::ViewProvider *vp, QWidget *parent,
                                             const QString& parname)
    : TaskBox(vp->getIcon().pixmap(64), parname,true, parent),
      vp(vp), blockUpdate(false)
{
    Gui::Document* doc = vp->getDocument();
    this->attachDocument(doc);
    App::GetApplication().getActiveTransaction(&transactionID);
}

void TaskFeatureParameters::slotDeletedObject(const Gui::ViewProviderDocumentObject& Obj)
{
    if (this->vp == &Obj)
        this->vp = nullptr;
}

void TaskFeatureParameters::slotDeleteDocument(const Gui::Document& Doc)
{
    if (this->vp && this->vp->getDocument() == &Doc)
        this->vp = nullptr;
}

void TaskFeatureParameters::slotUndoDocument(const Gui::Document &)
{
    _refresh();
    refresh();
}

void TaskFeatureParameters::slotRedoDocument(const Gui::Document &)
{
    _refresh();
    refresh();
}

void TaskFeatureParameters::_refresh()
{
    if (!this->vp)
        return;

    auto feat = Base::freecad_dynamic_cast<PartDesign::Feature>(this->vp->getObject());
    if (feat) {
        if (checkBoxNewSolid) {
            QSignalBlocker guard(checkBoxNewSolid);
            checkBoxNewSolid->setChecked(feat->NewSolid.getValue());
        } else if (comboAddSubType) {
            QSignalBlocker guard(comboAddSubType);
            if (feat->NewSolid.getValue())
                comboAddSubType->setCurrentIndex(0);
            else {
                switch(static_cast<PartDesign::FeatureAddSub*>(feat)->getAddSubType()) {
                case PartDesign::FeatureAddSub::Subtractive:
                    comboAddSubType->setCurrentIndex(2);
                    break;
                case PartDesign::FeatureAddSub::Intersecting:
                    comboAddSubType->setCurrentIndex(3);
                    break;
                default:
                    comboAddSubType->setCurrentIndex(1);
                }
            }
        }
    }
}

void TaskFeatureParameters::onUpdateView(bool on)
{
    blockUpdate = !on;
    recomputeFeature();
}

void TaskFeatureParameters::addBlinkWidget(QWidget *widget, const QString &txt)
{
    QString text;
    if (auto *w = qobject_cast<QLineEdit*>(widget))
        text = w->placeholderText();
    else if (auto *w = qobject_cast<QLabel*>(widget))
        text = w->text();
    else if (auto *w = qobject_cast<QAbstractButton*>(widget))
        text = w->text();
    else
        return;
    auto &info = blinkWidgets[widget];
    info.widget = widget;
    info.text = text;
    info.altText = txt;
    if (blinkTimerId == 0)
        blinkTimerId = startTimer(500);
}

void TaskFeatureParameters::BlinkInfo::setText(const QString &text)
{
    if (auto *w = qobject_cast<QLineEdit*>(widget))
        w->setPlaceholderText(text);
    else if (auto *w = qobject_cast<QLabel*>(widget))
        w->setText(text);
    else if (auto *w = qobject_cast<QAbstractButton*>(widget)) {
        int width = w->width();
        w->setText(text);
        w->setMinimumWidth(std::max(width, w->sizeHint().width()));
    }
}

void TaskFeatureParameters::removeBlinkWidget(QWidget *widget)
{
    auto it = blinkWidgets.find(widget);
    if (it != blinkWidgets.end()) {
        it->second.setText(it->second.text);
        blinkWidgets.erase(it);
    }
}

void TaskFeatureParameters::timerEvent(QTimerEvent *ev)
{
    if (ev->timerId() == blinkTimerId) {
        for (auto &v : blinkWidgets)
            v.second.setText(blink ? v.second.altText : v.second.text);
        blink = !blink;
    }
}

void TaskFeatureParameters::recomputeFeature(bool delay)
{
    if (blockUpdate || !vp || !vp->getObject())
        return;

    if (delay && updateViewTimer) {
        int interval = PartGui::PartParams::getEditRecomputeWait();
        auto feat = Base::freecad_dynamic_cast<PartDesign::FeatureAddSub>(vp->getObject());
        if (feat && feat->isRecomputePaused())
            interval /= 3;
        updateViewTimer->start(interval);
    } else {
        Gui::WaitCursor cursor;
        setupTransaction();
        App::DocumentObject* obj = vp->getObject ();
        obj->getDocument()->recomputeFeature ( obj );
    }
}

void TaskFeatureParameters::onNewSolidChanged()
{
    if (vp) {
        PartDesign::Feature* pc =
            Base::freecad_dynamic_cast<PartDesign::Feature>(vp->getObject());
        if (pc) {
            if (checkBoxNewSolid)
                pc->NewSolid.setValue(checkBoxNewSolid->isChecked());
            else if (comboAddSubType) {
                pc->NewSolid.setValue(comboAddSubType->currentIndex() == 0);
                auto feat = static_cast<PartDesign::FeatureAddSub*>(pc);
                if (pc->NewSolid.getValue())
                    feat->AddSubType.setValue("Additive");
                else {
                    switch(comboAddSubType->currentIndex()) {
                    case 2:
                        feat->AddSubType.setValue("Subtractive");
                        break;
                    case 3:
                        feat->AddSubType.setValue("Intersecting");
                        break;
                    default:
                        feat->AddSubType.setValue("Additive");
                        break;
                    }
                }
            }
            recomputeFeature();
        }
    }
}

void TaskFeatureParameters::saveHistory()
{
    if (checkBoxUpdateView)
        checkBoxUpdateView->onSave();
}

const char *TaskFeatureParameters::updateViewParameter() const
{
    return "User parameter:BaseApp/History/UpdateView";
}

void TaskFeatureParameters::addUpdateViewCheckBox(QBoxLayout *boxLayout)
{
    updateViewTimer = new QTimer(this);
    updateViewTimer->setSingleShot(true);
    connect(updateViewTimer, SIGNAL(timeout()), this, SLOT(onUpdateViewTimer()));

    checkBoxUpdateView = new Gui::PrefCheckBox(this);
    checkBoxUpdateView->setText(tr("Update view"));
    checkBoxUpdateView->setChecked(true);
    connect(checkBoxUpdateView, SIGNAL(toggled(bool)), this, SLOT(onUpdateView(bool)));
    boxLayout->addWidget(checkBoxUpdateView);
    checkBoxUpdateView->setParamGrpPath(QByteArray(updateViewParameter()));
    checkBoxUpdateView->onRestore();
}

void TaskFeatureParameters::onUpdateViewTimer()
{
    recomputeFeature(false);
}

void TaskFeatureParameters::setupTransaction()
{
    if (!vp || !vp->getObject())
        return;

    auto obj = vp->getObject();
    if (!obj)
        return;

    int tid = 0;
    const char *name = App::GetApplication().getActiveTransaction(&tid);
    if(tid && tid == transactionID)
        return;

    std::string n("Edit ");
    n += obj->getNameInDocument();
    if(!name || n!=name)
        tid = App::GetApplication().setActiveTransaction(n.c_str());

    if(!transactionID)
        transactionID = tid;
}

void TaskFeatureParameters::addOperationCombo(QBoxLayout *layout)
{
    if (!vp || !vp->getObject() || !layout)
        return;

    auto feat = Base::freecad_dynamic_cast<PartDesign::FeatureAddSub>(vp->getObject());
    if (feat && !feat->AddSubType.testStatus(App::Property::Hidden)) {
        QHBoxLayout *l = new QHBoxLayout;
        l->addWidget(new QLabel(tr("Operation:")));
        comboAddSubType = new QComboBox(this);
        comboAddSubType->addItem(tr("New shape"));
        comboAddSubType->addItem(tr("Additive"));
        comboAddSubType->addItem(tr("Subtractive"));
        comboAddSubType->addItem(tr("Intersecting"));
        l->addWidget(comboAddSubType);
        layout->insertLayout(0, l);
        connect(comboAddSubType, SIGNAL(currentIndexChanged(int)), this, SLOT(onNewSolidChanged()));
    } else {
        checkBoxNewSolid = new QCheckBox(this);
        checkBoxNewSolid->setText(tr("New shape"));
        checkBoxNewSolid->setToolTip(tr("Make a new separate solid using this feature"));
        layout->insertWidget(0, checkBoxNewSolid);
        connect(checkBoxNewSolid, SIGNAL(toggled(bool)), this, SLOT(onNewSolidChanged()));
    }
}

/*********************************************************************
 *                            Task Dialog                            *
 *********************************************************************/
TaskDlgFeatureParameters::TaskDlgFeatureParameters(PartDesignGui::ViewProvider *vp)
    : TaskDialog(),vp(vp)
{
    assert(vp);
}

TaskDlgFeatureParameters::~TaskDlgFeatureParameters()
{

}

bool TaskDlgFeatureParameters::accept() {
    if (!vp)
        return true;

    App::DocumentObject* feature = vp->getObject();

    try {
        // Iterate over parameter dialogs and apply all parameters from them
        for ( QWidget *wgt : Content ) {
            TaskFeatureParameters *param = qobject_cast<TaskFeatureParameters *> (wgt);
            if(!param)
                continue;

            param->saveHistory ();
            param->apply ();
        }
        // Make sure the feature is what we are expecting
        // Should be fine but you never know...
        if ( !feature->getTypeId().isDerivedFrom(PartDesign::Feature::getClassTypeId()) ) {
            throw Base::TypeError("Bad object processed in the feature dialog.");
        }

        Gui::cmdGuiDocument(feature, "resetEdit()");
        Gui::cmdAppDocument(feature, "recompute()");

        if (!feature->isValid()) {
            throw Base::RuntimeError(vp->getObject()->getStatusString());
        }

        // detach the task panel from the selection to avoid to handling of selection change
        std::vector<QWidget*> subwidgets = getDialogContent();
        for (auto it : subwidgets) {
            TaskSketchBasedParameters* param = qobject_cast<TaskSketchBasedParameters*>(it);
            if (param)
                param->exitSelectionMode();
        }

        Gui::Command::commitCommand();

    } catch (const Base::Exception& e) {
        // Generally the only thing that should fail is feature->isValid() others should be fine
        QString errorText = QApplication::translate(feature->getTypeId().getName(), e.what());
        QMessageBox::warning(Gui::getMainWindow(), tr("Input error"), errorText);
        return false;
    }

    return true;
}

bool TaskDlgFeatureParameters::reject()
{
    // detach the task panel from the selection to avoid handling selection change
    std::vector<QWidget*> subwidgets = getDialogContent();
    for (auto it : subwidgets) {
        TaskSketchBasedParameters* param = qobject_cast<TaskSketchBasedParameters*>(it);
        if (param)
            param->exitSelectionMode();
    }

    // roll back the done things which may delete the feature
    Gui::Command::abortCommand();
    Gui::Command::doCommand(Gui::Command::Gui,"Gui.activeDocument().resetEdit()");
    return true;
}

#include "moc_TaskFeatureParameters.cpp"
