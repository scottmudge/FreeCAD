/***************************************************************************
 *   Copyright (c) 2013 Jan Rheinländer                                    *
 *                                   <jrheinlaender@users.sourceforge.net> *
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
# include <QAction>
# include <QKeyEvent>
# include <QMessageBox>
#endif

#include "ui_TaskBooleanParameters.h"
#include "TaskBooleanParameters.h"
#include "Utils.h"
#include <Base/Tools.h>
#include <App/Application.h>
#include <App/DocumentObserver.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/MainWindow.h>
#include <Gui/WaitCursor.h>
#include <Mod/PartDesign/App/FeatureBoolean.h>
#include <Mod/PartDesign/App/ShapeBinder.h>
#include <Mod/PartDesign/App/Body.h>
#include <Mod/PartDesign/App/FeatureBoolean.h>

#include "ui_TaskBooleanParameters.h"
#include "TaskBooleanParameters.h"


using namespace PartDesignGui;
using namespace Gui;

/* TRANSLATOR PartDesignGui::TaskBooleanParameters */

TaskBooleanParameters::TaskBooleanParameters(ViewProviderBoolean *BooleanView,QWidget *parent)
    : TaskBox(Gui::BitmapFactory().pixmap("PartDesign_Boolean"), tr("Boolean parameters"), true, parent)
    , ui(new Ui_TaskBooleanParameters)
    , BooleanView(BooleanView)
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    // remembers the initial transaction ID
    App::GetApplication().getActiveTransaction(&transactionID);

    connect(ui->buttonAdd, SIGNAL(clicked(bool)),
            this, SLOT(onButtonAdd()));
    connect(ui->buttonRemove, SIGNAL(clicked(bool)),
            this, SLOT(onButtonRemove()));
    connect(ui->comboType, SIGNAL(currentIndexChanged(int)),
            this, SLOT(onTypeChanged(int)));
    connect(ui->checkBoxNewSolid, SIGNAL(toggled(bool)),
            this, SLOT(onNewSolidChanged(bool)));

    this->groupLayout()->addWidget(proxy);
    
#if QT_VERSION >= 0x040200
    ui->listWidgetBodies->setMouseTracking(true); // needed for itemEntered() to work
#endif
    ui->listWidgetBodies->setDragDropMode(QListWidget::InternalMove);
    connect(ui->listWidgetBodies->model(), SIGNAL(rowsMoved(QModelIndex,int,int,QModelIndex,int)),
            this, SLOT(onItemMoved()));
    connect(ui->listWidgetBodies, SIGNAL(itemEntered(QListWidgetItem*)), 
            this, SLOT(preselect(QListWidgetItem*)));
    ui->listWidgetBodies->installEventFilter(this); // for leaveEvent

    connect(ui->listWidgetBodies, SIGNAL(itemSelectionChanged()), 
            this, SLOT(onItemSelection()));

    undoConn = App::GetApplication().signalUndo.connect(boost::bind(&TaskBooleanParameters::populate,this));
    redoConn = App::GetApplication().signalRedo.connect(boost::bind(&TaskBooleanParameters::populate,this));

    populate();

    // Create context menu
    QAction* action = new QAction(tr("Remove"), this);
    action->setShortcut(QKeySequence::Delete);
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
    // display shortcut behind the context menu entry
    action->setShortcutVisibleInContextMenu(true);
#endif
    ui->listWidgetBodies->addAction(action);
    connect(action, SIGNAL(triggered()), this, SLOT(onButtonRemove()));
    ui->listWidgetBodies->setContextMenuPolicy(Qt::ActionsContextMenu);

    auto hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/PartDesign");
    ui->checkboxDeleteOnRemove->setChecked(hGrp->GetBool("BooleanDeleteOnRemove",true));

    connect(ui->checkboxDeleteOnRemove, SIGNAL(toggled(bool)), 
            this, SLOT(onDeleteOnRemove(bool)));
}

void TaskBooleanParameters::onDeleteOnRemove(bool checked) {
    auto hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/PartDesign");
    hGrp->SetBool("BooleanDeleteOnRemove",checked);
}

void TaskBooleanParameters::populate() {
    if (!BooleanView || !BooleanView->getObject())
        return;

    ui->listWidgetBodies->clear();
    PartDesign::Boolean* pcBoolean = static_cast<PartDesign::Boolean*>(BooleanView->getObject());
    std::vector<App::DocumentObject*> bodies = pcBoolean->Group.getValues();
    for (std::vector<App::DocumentObject*>::const_iterator it = bodies.begin(); it != bodies.end(); ++it) {
        QListWidgetItem* item = new QListWidgetItem(ui->listWidgetBodies);
        item->setText(QString::fromUtf8((*it)->Label.getValue()));
        item->setData(Qt::UserRole, QString::fromUtf8((*it)->getNameInDocument()));
        auto vp = Application::Instance->getViewProvider(*it);
        if(vp)
            item->setIcon(vp->getIcon());
    }
    int index = pcBoolean->Type.getValue();
    ui->comboType->setCurrentIndex(index);
}

bool TaskBooleanParameters::eventFilter(QObject *watched, QEvent *event) {
    if(watched == ui->listWidgetBodies) {
        switch (event->type()) {
        case QEvent::Leave:
            Gui::Selection().rmvPreselect();
            break;
        case QEvent::ShortcutOverride:
        case QEvent::KeyPress: {
            QKeyEvent * kevent = static_cast<QKeyEvent*>(event);
            if (kevent->modifiers() == Qt::NoModifier) {
                if (kevent->key() == Qt::Key_Delete) {
                    kevent->accept();
                    if (event->type() == QEvent::KeyPress)
                        onButtonRemove();
                }
            }
            break;
        }
        default:
            break;
        }
    }
    return false;
}

App::DocumentObject *TaskBooleanParameters::getInEdit(std::string &subname) {
    auto gdoc = Gui::Application::Instance->editDocument();
    if(!gdoc)
        return 0;

    ViewProviderDocumentObject *parent = 0;
    auto vp = gdoc->getInEdit(&parent,&subname);
    if(!parent || vp!=BooleanView)
        return 0;

    return parent->getObject();
}

void TaskBooleanParameters::onItemMoved()
{
    if (!BooleanView || !BooleanView->getObject())
        return;

    PartDesign::Boolean* pcBoolean = static_cast<PartDesign::Boolean*>(BooleanView->getObject());
    std::vector<App::DocumentObject*> group;
    for(int i=0,count=ui->listWidgetBodies->count();i<count;++i) {
        auto item = ui->listWidgetBodies->item(i);
        auto obj = pcBoolean->getDocument()->getObject(
                item->data(Qt::UserRole).toString().toUtf8().constData());
        if (obj)
            group.push_back(obj);
    }
    if (group == pcBoolean->Group.getValues())
        return;

    try {
        Gui::WaitCursor cursor;
        setupTransaction();
        Gui::Selection().clearSelection();
        pcBoolean->Group.setValues(group);
        pcBoolean->recomputeFeature(true);
    } catch (Base::Exception &e) {
        e.ReportException();
    }
    populate();
}

void TaskBooleanParameters::preselect(QListWidgetItem *item) {
    std::string subname;
    auto parent = getInEdit(subname);
    if(!parent)
        return;
    QString name = item->data(Qt::UserRole).toString();
    subname += name.toUtf8().constData();
    subname += ".";

    Gui::Selection().setPreselect(parent->getDocument()->getName(),
            parent->getNameInDocument(),subname.c_str(),0,0,0,
            Gui::SelectionChanges::MsgSource::TreeView,true);
}

void TaskBooleanParameters::onItemSelection() {
    std::string subname;
    auto parent = getInEdit(subname);
    if(!parent)
        return;

    Base::StateLocker guard(this->selecting);

    Gui::Selection().clearCompleteSelection();
    std::vector<std::string> subs;
    for(auto item : ui->listWidgetBodies->selectedItems()) {
        QString name = item->data(Qt::UserRole).toString();
        subs.emplace_back(subname + name.toUtf8().constData() + ".");
    }

    Gui::Selection().addSelections(parent->getDocument()->getName(),
            parent->getNameInDocument(),subs);
}

void TaskBooleanParameters::syncSelection() {
    if(ui->listWidgetBodies->signalsBlocked())
        return;
    std::string subname;
    auto parent = getInEdit(subname);
    if(!parent)
        return;
    QSignalBlocker blocker(ui->listWidgetBodies);
    bool hasSelection = false;
    for(int i=0,count=ui->listWidgetBodies->count();i<count;++i) {
        auto item = ui->listWidgetBodies->item(i);
        QString name = item->data(Qt::UserRole).toString();
        std::string sub = subname + name.toUtf8().constData() + ".";
        auto sobj = parent->getSubObject(sub.c_str());
        bool selected = item->isSelected();
        if(selected != Gui::Selection().isSelected(sobj))
            item->setSelected(!selected);
        if (selected)
            hasSelection = true;
    }
    ui->buttonAdd->setEnabled(!hasSelection && Gui::Selection().hasSelection());
    ui->buttonRemove->setEnabled(hasSelection);
}

void TaskBooleanParameters::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if (this->selecting)
        return;
    switch(msg.Type) {
    case Gui::SelectionChanges::ClrSelection: {
        QSignalBlocker blocker(ui->listWidgetBodies);
        ui->listWidgetBodies->clearSelection();
        ui->buttonAdd->setEnabled(false);
        ui->buttonRemove->setEnabled(false);
        break;
    }
    case Gui::SelectionChanges::SetSelection:
    case Gui::SelectionChanges::RmvSelection:
    case Gui::SelectionChanges::AddSelection:
        syncSelection();
        break;
    default:
        return;
    }
}

void TaskBooleanParameters::onButtonAdd()
{
    std::string subname;
    auto parent = getInEdit(subname);
    if(!parent) {
        QMessageBox::warning(this, tr("Boolean: invalid state"), 
                tr("No editing object found. Please restart thit task panel"));
        return;
    }

    PartDesign::Boolean* pcBoolean = static_cast<PartDesign::Boolean*>(BooleanView->getObject());
    auto inset = pcBoolean->getInListEx(true);
    auto sels = Gui::Selection().getCompleteSelection(Gui::ResolveMode::NoResolve);
    if(sels.empty())
        return;

    auto group = App::GroupExtension::getGroupOfObject(pcBoolean);
    if(!group)
        group = App::GeoFeatureGroupExtension::getGroupOfObject(pcBoolean);
    if(!group) {
        QMessageBox::warning(this, tr("Boolean: Invalid state"), 
                tr("Invalid boolean object"));
        return;
    }

    auto prop = Base::freecad_dynamic_cast<App::PropertyLinkList>(
            group->getPropertyByName("Group"));
    if(!prop) {
        QMessageBox::warning(this, tr("Boolean: Invalid state"), 
                tr("Invalid owner of boolean object"));
        return;
    }
    const auto &objs = prop->getValues();
    std::set<App::DocumentObject *> objset(objs.begin(),objs.end());

    std::map<App::DocumentObject*, std::vector<std::string> > supports;
    for(auto &sel : sels) {
        auto objs = sel.pObject->getSubObjectList(sel.SubName);
        if(objs.empty())
            continue;
        std::size_t idx = 0;
        for(;idx<objs.size();++idx) {
            if(!inset.count((objs[idx])))
                break;
        }
        if(idx == objs.size()) {
            QMessageBox::warning(this, tr("Boolean: Input error"), 
                    tr("Cyclic reference to ") + QString::fromUtf8(objs.back()->Label.getValue()));
            return;
        }
        std::ostringstream ss;
        for(auto i=idx;i<objs.size();++i) {
            if(objset.count(objs[i])) {
                ss.str("");
                idx = i;
                continue;
            }
            if(i!=idx)
                ss << objs[i]->getNameInDocument() << '.';
        }
        supports[objs[idx]].emplace_back(ss.str());
    }

    setupTransaction();

    auto doc = pcBoolean->getDocument();
    PartDesign::SubShapeBinder *ref = 0;
    try {
        ref = static_cast<PartDesign::SubShapeBinder*>(
                doc->addObject("PartDesign::SubShapeBinder","Reference"));
        ref->Fuse.setValue(true);
        ref->setLinks(std::move(supports),true);
    } catch (Base::Exception &e) {
        e.ReportException();
        QMessageBox::warning(this, tr("Boolean: Input error"), 
                tr("Failed to add reference: ") + QString::fromUtf8(e.what()));
        if(ref)
            doc->removeObject(ref->getNameInDocument());
        return;
    }

    try {
        Gui::WaitCursor cursor;
        pcBoolean->addObject(ref);
        pcBoolean->recomputeFeature(true);
    } catch (Base::Exception &e) {
        e.ReportException();
    }
    populate();
}

void TaskBooleanParameters::setupTransaction() {
    int tid = 0;
    const char *name = App::GetApplication().getActiveTransaction(&tid);
    if(tid && tid == transactionID)
        return;

    std::string n("Edit ");
    n += BooleanView->getObject()->getNameInDocument();
    if(!name || n != name)
        App::GetApplication().setActiveTransaction(n.c_str());

    if(!transactionID)
        transactionID = tid;
}

void TaskBooleanParameters::onTypeChanged(int index)
{
    PartDesign::Boolean* pcBoolean = static_cast<PartDesign::Boolean*>(BooleanView->getObject());

    try {
        setupTransaction();
        switch (index) {
            case 0: pcBoolean->Type.setValue("Fuse"); break;
            case 1: pcBoolean->Type.setValue("Cut"); break;
            case 2: pcBoolean->Type.setValue("Common"); break;
            case 3: pcBoolean->Type.setValue("Compound"); break;
            case 4: pcBoolean->Type.setValue("Section"); break;
            default: pcBoolean->Type.setValue("Fuse");
        }

        Gui::WaitCursor cursor;
        pcBoolean->recomputeFeature(true);
    } catch (Base::Exception &e) {
        e.ReportException();
    }

}

const std::vector<std::string> TaskBooleanParameters::getBodies(void) const
{
    std::vector<std::string> result;
    for (int i = 0; i < ui->listWidgetBodies->count(); i++)
        result.push_back(ui->listWidgetBodies->item(i)->data(Qt::UserRole).toString().toStdString());
    return result;
}

int TaskBooleanParameters::getType(void) const
{
    return ui->comboType->currentIndex();
}

void TaskBooleanParameters::onButtonRemove()
{
    std::set<std::string> names;
    for(auto item : ui->listWidgetBodies->selectedItems()) 
        names.insert(item->data(Qt::UserRole).toString().toStdString());

    bool remove = ui->checkboxDeleteOnRemove->isChecked();
    std::vector<std::string> removed;

    PartDesign::Boolean* pcBoolean = static_cast<PartDesign::Boolean*>(BooleanView->getObject());
    auto group = pcBoolean->Group.getValues();
    for(auto it=group.begin();it!=group.end();) {
        auto obj = *it;
        if(names.count(obj->getNameInDocument())) {
            if(remove && !obj->isDerivedFrom(PartDesign::Body::getClassTypeId()))
                removed.push_back(obj->getNameInDocument());
            it = group.erase(it);
        } else
            ++it;
    }
    Gui::Selection().clearSelection();

    try {
        Gui::WaitCursor cursor;
        setupTransaction();
        pcBoolean->Group.setValues(group);
        for(auto &name : removed) 
            pcBoolean->getDocument()->removeObject(name.c_str());
        pcBoolean->recomputeFeature(true);
    } catch (Base::Exception &e) {
        e.ReportException();
    }
    populate();
}

TaskBooleanParameters::~TaskBooleanParameters()
{
}

void TaskBooleanParameters::changeEvent(QEvent *e)
{
    TaskBox::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
        ui->comboType->blockSignals(true);
        int index = ui->comboType->currentIndex();
        ui->retranslateUi(proxy);
        ui->comboType->setCurrentIndex(index);
    }
}

void TaskBooleanParameters::onNewSolidChanged(bool on)
{
    try {
        Gui::WaitCursor cursor;
        PartDesign::Boolean* pcBoolean = static_cast<PartDesign::Boolean*>(BooleanView->getObject());
        setupTransaction();
        pcBoolean->NewSolid.setValue(on);
        populate();
        pcBoolean->recomputeFeature();
    } catch (Base::Exception &e) {
        e.ReportException();
    }
}


//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgBooleanParameters::TaskDlgBooleanParameters(ViewProviderBoolean *BooleanView)
    : TaskDlgFeatureParameters(BooleanView), BooleanView(BooleanView)
{
    assert(BooleanView);
    parameter  = new TaskBooleanParameters(BooleanView);

    Content.push_back(parameter);
}

TaskDlgBooleanParameters::~TaskDlgBooleanParameters()
{

}

//==== calls from the TaskView ===============================================================


void TaskDlgBooleanParameters::open()
{

}

void TaskDlgBooleanParameters::clicked(int)
{

}

bool TaskDlgBooleanParameters::accept()
{
    auto obj = BooleanView->getObject();
    if(!obj || !obj->getNameInDocument())
        return false;

    std::vector<std::string> bodies = parameter->getBodies();
    if (bodies.empty()) {
        QMessageBox::warning(parameter, tr("Empty body list"),
                                tr("The body list cannot be empty"));
        return false;
    }
    parameter->setupTransaction();
    Gui::Command::doCommand(Gui::Command::Gui,"Gui.activeDocument().resetEdit()");
    Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.recompute()");
    Gui::Command::commitCommand();

    return true;
}

bool TaskDlgBooleanParameters::reject()
{
    // roll back the done things
    auto editDoc = Gui::Application::Instance->editDocument();
    if(editDoc && parameter->getTransactionID())
        editDoc->getDocument()->undo(parameter->getTransactionID());

    Gui::Command::doCommand(Gui::Command::Gui,"Gui.activeDocument().resetEdit()");
    return true;
}



#include "moc_TaskBooleanParameters.cpp"
