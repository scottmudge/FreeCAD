/***************************************************************************
 *   Copyright (c) 2012 Jan Rheinländer                                    *
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
# include <QApplication>
# include <QAction>
# include <QKeyEvent>
# include <QPushButton>
# include <QCheckBox>
#endif

#include <boost/algorithm/string/predicate.hpp>
#include "TaskDressUpParameters.h"
#include "Utils.h"
#include <Base/Tools.h>
#include <App/Application.h>
#include <App/Document.h>
#include <App/MappedElement.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/BitmapFactory.h>
#include <Gui/ViewProvider.h>
#include <Gui/WaitCursor.h>
#include <Base/Console.h>
#include <Gui/Selection.h>
#include <Gui/Command.h>
#include <Gui/MainWindow.h>
#include <Gui/ViewParams.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>
#include <Mod/PartDesign/App/Body.h>
#include <Mod/PartDesign/App/FeatureDressUp.h>
#include <Mod/PartDesign/Gui/ReferenceSelection.h>

FC_LOG_LEVEL_INIT("PartDesign",true,true)

using namespace PartDesignGui;
using namespace Gui;

/* TRANSLATOR PartDesignGui::TaskDressUpParameters */

TaskDressUpParameters::TaskDressUpParameters(ViewProviderDressUp *DressUpView, bool selectEdges, bool selectFaces, QWidget *parent)
    : TaskBox(Gui::BitmapFactory().pixmap((std::string("PartDesign_") + DressUpView->featureName()).c_str()),
              QString::fromUtf8((DressUpView->featureName() + " parameters").c_str()),
              true,
              parent)
    , proxy(0)
    , DressUpView(DressUpView)
    , allowFaces(selectFaces)
    , allowEdges(selectEdges)
{
    // remember initial transaction ID
    App::GetApplication().getActiveTransaction(&transactionID);

    selectionMode = none;

    onTopEnabled = Gui::ViewParams::getShowSelectionOnTop();
    if(!onTopEnabled)
        Gui::ViewParams::setShowSelectionOnTop(true);

    connUndo = App::GetApplication().signalUndo.connect(boost::bind(&TaskDressUpParameters::refresh, this));
    connRedo = App::GetApplication().signalRedo.connect(boost::bind(&TaskDressUpParameters::refresh, this));

    connDelete = Gui::Application::Instance->signalDeletedObject.connect(
        [this](const Gui::ViewProvider &Obj) {
            if(this->DressUpView == &Obj)
                this->DressUpView = nullptr;
        });

    connDeleteDoc = Gui::Application::Instance->signalDeleteDocument.connect(
        [this](const Gui::Document &Doc) {
            if(this->DressUpView && this->DressUpView->getDocument() == &Doc)
                this->DressUpView = nullptr;
        });

    timer = new QTimer(this);
    timer->setSingleShot(true);
    connect(timer, SIGNAL(timeout()), this, SLOT(onTimer()));
}

TaskDressUpParameters::~TaskDressUpParameters()
{
    // make sure to remove selection gate in all cases
    Gui::Selection().rmvSelectionGate();

    if(!onTopEnabled)
        Gui::ViewParams::setShowSelectionOnTop(false);

    clearButtons(none);
    exitSelectionMode();
}

QTreeWidgetItem *TaskDressUpParameters::getCurrentItem() const
{
    if(!DressUpView)
        return nullptr;
    QTreeWidgetItem *current = nullptr;
    auto sels = treeWidget->selectedItems();
    if (sels.size())
        current = sels[sels.size()-1];
    else {
        current = treeWidget->currentItem();
        if (!current)
            current = treeWidget->itemAt(QPoint(0,0));
    }
    return current;
}

void TaskDressUpParameters::setupTransaction() {
    if(!DressUpView)
        return;

    int tid = 0;
    const char *name = App::GetApplication().getActiveTransaction(&tid);
    if(tid && tid == transactionID)
        return;

    std::string n("Edit ");
    n += DressUpView->getObject()->getNameInDocument();
    if(!name || n!=name)
        tid = App::GetApplication().setActiveTransaction(n.c_str());

    if (!transactionID)
        transactionID = tid;
}

void TaskDressUpParameters::setup(QLabel *label, QTreeWidget *widget, QCheckBox *_btnAdd, bool touched)
{
    if(!DressUpView)
        return;
    auto* pcDressUp = static_cast<PartDesign::DressUp*>(DressUpView->getObject());
    if(!pcDressUp || !pcDressUp->Base.getValue())
        return;

    PartDesignGui::addTaskCheckBox(proxy);

    // Remember the initial transaction ID
    App::GetApplication().getActiveTransaction(&transactionID);

    messageLabel = label;
    messageLabel->hide();
    messageLabel->setWordWrap(true);

    btnAdd = _btnAdd;
    connect(btnAdd, SIGNAL(toggled(bool)), this, SLOT(onButtonRefAdd(bool)));

    treeWidget = widget;
    treeWidget->setRootIsDecorated(false);
    treeWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    treeWidget->setMouseTracking(true);
    treeWidget->installEventFilter(this);

    connect(treeWidget, SIGNAL(itemSelectionChanged()),
        this, SLOT(onItemSelectionChanged()));
    connect(treeWidget, SIGNAL(itemEntered(QTreeWidgetItem*,int)),
        this, SLOT(onItemEntered(QTreeWidgetItem*,int)));

    if(!deleteAction) {
        // Create context menu
        deleteAction = new QAction(tr("Remove"), this);
        deleteAction->setShortcut(QKeySequence::Delete);
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
        // display shortcut behind the context menu entry
        deleteAction->setShortcutVisibleInContextMenu(true);
#endif
        widget->addAction(deleteAction);
        connect(deleteAction, SIGNAL(triggered()), this, SLOT(onRefDeleted()));
        widget->setContextMenuPolicy(Qt::ActionsContextMenu);
    }

    if(populate() || touched) {
        setupTransaction();
        recompute();
    }
}

void TaskDressUpParameters::refresh()
{
    populate(true);
}

bool TaskDressUpParameters::populate(bool refresh)
{
    if(!treeWidget || !DressUpView)
        return false;

    auto* pcDressUp = static_cast<PartDesign::DressUp*>(DressUpView->getObject());
    if(!pcDressUp || !pcDressUp->Base.getValue())
        return false;

    Base::StateLocker guard(busy);
    if (!refresh)
        setupTransaction();
    return PartDesignGui::populateGeometryReferences(treeWidget, pcDressUp->Base, refresh);
}

bool TaskDressUpParameters::showOnTop(bool enable,
        std::vector<App::SubObjectT> &&objs)
{
    if(onTopObjs.size()) {
        auto doc = Gui::Application::Instance->getDocument(
                onTopObjs.front().getDocumentName().c_str());
        if(doc) {
            auto view = Base::freecad_dynamic_cast<Gui::View3DInventor>(doc->getActiveView());
            if(view) {
                auto viewer = view->getViewer();
                for(auto &obj : onTopObjs) {
                    viewer->checkGroupOnTop(Gui::SelectionChanges(SelectionChanges::RmvSelection,
                                obj.getDocumentName().c_str(),
                                obj.getObjectName().c_str(),
                                obj.getSubName().c_str()), true);
                }
            }
        }
    }
    onTopObjs.clear();
    if(!enable)
        return true;

    if(objs.empty())
        objs.push_back(getInEdit());

    auto doc = Gui::Application::Instance->getDocument(
            objs.front().getDocumentName().c_str());
    if(!doc)
        return false;
    auto view = Base::freecad_dynamic_cast<Gui::View3DInventor>(doc->getActiveView());
    if(!view)
        return false;
    auto viewer = view->getViewer();
    for(auto &obj : objs) {
        viewer->checkGroupOnTop(Gui::SelectionChanges(SelectionChanges::AddSelection,
                    obj.getDocumentName().c_str(),
                    obj.getObjectName().c_str(),
                    obj.getSubName().c_str()), true);
    }
    onTopObjs = std::move(objs);
    return true;
}

void TaskDressUpParameters::clearButtons(const selectionModes notThis)
{
    if (notThis != refToggle && btnAdd) {
        QSignalBlocker blocker(btnAdd);
        btnAdd->setChecked(false);
        showOnTop(false);
    }
    if(DressUpView)
        DressUpView->highlightReferences(false);
}

void TaskDressUpParameters::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if(!treeWidget || !DressUpView)
        return;

    bool addSel = false;
    switch(msg.Type) {
    case Gui::SelectionChanges::ClrSelection: {
        showMessage();
        Base::StateLocker guard(busy);
        treeWidget->selectionModel()->clearSelection();
        break;
    }
    case Gui::SelectionChanges::RmvSelection:
        break;
    case Gui::SelectionChanges::AddSelection:
        addSel = true;
        break;
    default:
        return;
    }

    App::DocumentObject* base = this->getBase();
    auto selObj = msg.Object.getObject();
    if(!base || !selObj)
        return;

    auto* pcDressUp = static_cast<PartDesign::DressUp*>(DressUpView->getObject());
    if(selObj == pcDressUp && addSel) {
        // The dress up feature itself is selected, trace the selected element
        // back to its base
        auto history = pcDressUp->getElementHistory(pcDressUp,msg.pSubName);
        const char *element = 0;
        std::string tmp;
        for(auto &hist : history) {
            if(hist.obj != base)
                continue;

            char type = pcDressUp->Shape.getShape().elementType(hist.element);
            if ((!allowFaces && (type == 'F' || type == 'W')) || (!allowEdges && type == 'E'))
                continue;
            if(element) {
                showMessage("Ambiguous selection");
                return;
            }
            element = hist.element.toPrefixedString(tmp);
        }
        if(element) {
            std::vector<App::SubObjectT> sels;
            sels.emplace_back(base, element);

            // base element found, check if it is already in the widget
            auto items = treeWidget->findItems(
                    QString::fromUtf8(sels.back().getOldElementName().c_str()), Qt::MatchExactly);

            if(items.size()) {
                if(selectionMode != refToggle) {
                    // element found, but we are not toggling, select and exit
                    Base::StateLocker guard(busy);
                    for(auto item : items)
                        item->setSelected(true);
                    return;
                }

                // We are toggling, so delete the found item, and call syncItems() below
                if(onTopObjs.empty()) {
                    for(auto item : items)
                        delete item;
                    sels.clear();
                }
            }

            if(msg.pOriginalMsg) {
                // We about dress up the current selected element, meaning that
                // this original element will be gone soon. So remove it from
                // the selection to avoid warning.
                Gui::Selection().rmvSelection(msg.pOriginalMsg->pDocName,
                                              msg.pOriginalMsg->pObjectName,
                                              msg.pOriginalMsg->pSubName);
            }

            if(onTopObjs.empty()) {
                // If no on top display at the moment, just sync the reference
                // selection.
                syncItems(sels);
            } else {
                // If there is on top display, replace the current selection
                // with the base element selection, and let it trigger an
                // subsequent onSelectionChanged() event, which will be handled
                // by code below.
                std::string sub;
                auto obj = getInEdit(sub);
                if(obj) {
                    sub += element;
                    Gui::Selection().addSelection(obj->getDocument()->getName(),
                            obj->getNameInDocument(), sub.c_str());
                }
            }
        }
        return;
    }

    showMessage();

    if(!base || base != selObj)
        return;

    bool check = true;
    auto items = treeWidget->findItems(QString::fromUtf8(msg.pSubName), Qt::MatchExactly);
    if(items.size()) {
        Base::StateLocker guard(busy);
        if(selectionMode == refToggle && addSel) {
            check = false;
            for(auto item : items)
                delete item;
        } else {
            for(auto item : items) {
                if(!item->isSelected())
                    item->setSelected(addSel);
            }
        }
    }

    if(addSel) {
        if(check)
            syncItems(Gui::Selection().getSelectionT());
        else
            syncItems();
    }
}

void TaskDressUpParameters::onButtonRefAdd(bool checked)
{
    if(!DressUpView)
        return;

    // We no longer use highlightReferences now, but ShowOnTopSelection
    // instead. Turning off here just to be safe.
    DressUpView->highlightReferences(false);

    if(!checked) {
        clearButtons(none);
        exitSelectionMode();
        return;
    }

    std::string subname;
    auto obj = getInEdit(subname);
    if(obj) {
        blockConnection(true);
        Base::StateLocker guard(busy);
        for(auto item : treeWidget->selectedItems()) {
            std::string sub = subname + getGeometryItemText(item).constData();
            Gui::Selection().rmvSelection(obj->getDocument()->getName(),
                    obj->getNameInDocument(), sub.c_str());
            delete item;
        }
        blockConnection(false);
        syncItems(Gui::Selection().getSelectionT());
    }
    exitSelectionMode();
    selectionMode = refToggle;
    clearButtons(refToggle);
    Gui::Selection().clearSelection();
    ReferenceSelection::Config conf;
    conf.edge = allowEdges;
    conf.plane = allowFaces;
    conf.planar = false;
    Gui::Selection().addSelectionGate(new ReferenceSelection(this->getBase(), conf));
}

void TaskDressUpParameters::onRefDeleted() {
    if(!treeWidget || !DressUpView)
        return;

    Base::StateLocker guard(busy);
    for(auto item : treeWidget->selectedItems())
        delete item;

    syncItems();
}

bool TaskDressUpParameters::syncItems(const std::vector<App::SubObjectT> &sels) {
    if(!DressUpView)
        return false;

    std::set<std::string> subset;
    std::vector<std::string> subs;
    for(int i=0, count=treeWidget->topLevelItemCount();i<count;++i) {
        std::string s = treeWidget->topLevelItem(i)->text(0).toStdString();
        if(subset.insert(s).second)
            subs.push_back(std::move(s));
    }

    auto* pcDressUp = static_cast<PartDesign::DressUp*>(DressUpView->getObject());
    App::DocumentObject* base = pcDressUp->Base.getValue();

    if(sels.size()) {
        Base::StateLocker guard(busy);
        for(auto &sel : sels) {
            if(sel.getObject() != base)
                continue;
            std::string element = sel.getOldElementName();
            if(subset.count(element))
                continue;
            if((allowEdges && boost::starts_with(element,"Edge"))
                    || (allowFaces && (boost::starts_with(element,"Face")
                                       || boost::starts_with(element, "Wire"))))
            {
                subset.insert(element);
                auto item = new QTreeWidgetItem(treeWidget);
                item->setText(0, QString::fromUtf8(element.c_str()));
                setGeometryItemText(item, element.c_str());
                item->setSelected(true);
                subs.push_back(std::move(element));
                onNewItem(item);
            }
        }
    }

    if(subs == pcDressUp->Base.getSubValues())
        return false;

    try {
        setupTransaction();
        pcDressUp->Base.setValue(base, subs);
        recompute();
    } catch (Base::Exception &e) {
        e.ReportException();
    }
    return true;
}

void TaskDressUpParameters::recompute() {
    if(DressUpView) {
        DressUpView->getObject()->recomputeFeature();
        showMessage();
    }
}

void TaskDressUpParameters::showMessage(const char *msg) {
    if(!messageLabel || !DressUpView)
        return;

    auto obj = DressUpView->getObject();
    if(!msg && obj->isError())
        msg = obj->getStatusString();
    if(!msg || !msg[0])
        messageLabel->hide();
    else {
        messageLabel->setText(QStringLiteral("<font color='red'>%1<br/>%2</font>").arg(
                tr("Recompute failed"), tr(msg)));
        messageLabel->show();
    }
}

void TaskDressUpParameters::onItemSelectionChanged()
{
    if(!treeWidget || busy)
        return;

    if(selectionMode == refToggle) {
        onRefDeleted();
        return;
    }

    std::string subname;
    auto obj = getInEdit(subname);
    if(!obj)
        return;

    std::set<QTreeWidgetItem*> itemSet;
    std::vector<std::string> subs;
    for(auto item : treeWidget->selectedItems()) {
        if (auto parent = item->parent())
            item = parent;
        if (itemSet.insert(item).second) {
            if (getGeometryItemReference(item).isEmpty())
                subs.push_back(subname + getGeometryItemText(item).constData());
        }
    }

    blockConnection(true);
    Gui::Selection().clearSelection();
    Gui::Selection().addSelections(obj->getDocument()->getName(),
            obj->getNameInDocument(), subs);
    blockConnection(false);
}

App::SubObjectT TaskDressUpParameters::getInEdit(App::DocumentObject *base, const char *sub) {
    std::string subname;
    auto obj = getInEdit(subname,base);
    if(obj) {
        if(sub)
            subname += sub;
        return App::SubObjectT(obj,subname.c_str());
    }
    return App::SubObjectT();
}

App::DocumentObject *TaskDressUpParameters::getInEdit(std::string &subname, App::DocumentObject *base)
{
    if(!DressUpView)
        return nullptr;

    auto* pcDressUp = static_cast<PartDesign::DressUp*>(DressUpView->getObject());
    if(!base) {
        base = pcDressUp->Base.getValue();
        if(!base)
            return nullptr;
    }
    auto editDoc = Gui::Application::Instance->editDocument();
    if(!editDoc)
        return nullptr;

    ViewProviderDocumentObject *editVp = nullptr;
    editDoc->getInEdit(&editVp,&subname);
    if(!editVp)
        return nullptr;

    auto sobjs = editVp->getObject()->getSubObjectList(subname.c_str());
    if(sobjs.empty())
        return nullptr;

    for(;;) {
        if(sobjs.back() == base)
            break;
        sobjs.pop_back();
        if(sobjs.empty())
            break;
        std::string sub(base->getNameInDocument());
        sub += ".";
        if(sobjs.back()->getSubObject(sub.c_str())) {
            sobjs.push_back(base);
            break;
        }
    }

    std::ostringstream ss;
    for(size_t i=1;i<sobjs.size();++i)
        ss << sobjs[i]->getNameInDocument() << '.';

    subname = ss.str();
    return sobjs.size()?sobjs.front():base;
}

void TaskDressUpParameters::onItemEntered(QTreeWidgetItem *, int)
{
    enteredObject = treeWidget;
    timer->start(100);
}

bool TaskDressUpParameters::getItemElement(QTreeWidgetItem *item, std::string &subname)
{
    if (auto parent = item->parent())
        item = parent;
    QByteArray _ref = getGeometryItemReference(item);
    const char *ref = Data::ComplexGeoData::isMappedElement(_ref.constData());
    if (!ref) {
        subname += getGeometryItemText(item).constData();
        return true;
    }
    PartDesign::DressUp* pcDressUp = static_cast<PartDesign::DressUp*>(DressUpView->getObject());
    auto shape = pcDressUp->DressUpShape.getShape();
    Data::IndexedName indexed;
    std::string tmp;
    bool found = false;
    for (int i=1, c=shape.countSubShapes(TopAbs_FACE); i<=c; ++i) {
        indexed = Data::IndexedName::fromConst("Face", i);
        auto name = shape.getMappedName(indexed);
        shape.traceElement(name,
            [&] (const Data::MappedName &n, int, long, long) {
                tmp.clear();
                n.toString(tmp);
                if (tmp == ref) {
                    found = true;
                    return true;
                }
                return false;
            });
        if (found)
            break;
    }
    if (!found)
        return false;

    subname.clear();
    ViewProviderDocumentObject *editVp = nullptr;
    auto editDoc = Gui::Application::Instance->editDocument();
    if (!editDoc)
        return false;
    editDoc->getInEdit(&editVp,&subname);
    subname += PartDesign::FeatureAddSub::addsubElementPrefix();
    indexed.toString(subname);
    return true;
}

void TaskDressUpParameters::onTimer() {
    if(enteredObject != treeWidget || !treeWidget)
        return;

    auto item = treeWidget->itemAt(treeWidget->viewport()->mapFromGlobal(QCursor::pos()));
    if(!item) {
        Gui::Selection().rmvPreselect();
        return;
    }
    std::string subname;
    auto obj = getInEdit(subname);
    if(obj && getItemElement(item, subname))
        Gui::Selection().setPreselect(obj->getDocument()->getName(),
                                      obj->getNameInDocument(),
                                      subname.c_str(),0,0,0,2,true);
    else
        Gui::Selection().rmvPreselect();
}

std::vector<std::string> TaskDressUpParameters::getReferences() const
{
    if(!DressUpView) 
        return {};

    PartDesign::DressUp* pcDressUp = static_cast<PartDesign::DressUp*>(DressUpView->getObject());
    std::vector<std::string> result = pcDressUp->Base.getSubValues();
    return result;
}

Part::Feature* TaskDressUpParameters::getBase(void) const
{
    if(!DressUpView)
        throw Base::RuntimeError("No view object");

    PartDesign::DressUp* pcDressUp = static_cast<PartDesign::DressUp*>(DressUpView->getObject());
    // Unlikely but this may throw an exception in case we are started to edit an object which base feature
    // was deleted. This exception will be likely unhandled inside the dialog and pass upper, But an error
    // message inside the report view is better than a SEGFAULT.
    // Generally this situation should be prevented in ViewProviderDressUp::setEdit()
    return pcDressUp->getBaseObject();
}

void TaskDressUpParameters::exitSelectionMode()
{
    selectionMode = none;
    Gui::Selection().rmvSelectionGate();
    Gui::Selection().clearSelection();
}

bool TaskDressUpParameters::event(QEvent *e)
{
    if (e && e->type() == QEvent::ShortcutOverride) {
        QKeyEvent * kevent = static_cast<QKeyEvent*>(e);
        if (kevent->modifiers() == Qt::NoModifier) {
            if (kevent->key() == Qt::Key_Delete) {
                kevent->accept();
                return true;
            }
        }
    }
    else if (e && e->type() == QEvent::KeyPress) {
        QKeyEvent * kevent = static_cast<QKeyEvent*>(e);
        if (deleteAction && kevent->key() == Qt::Key_Delete) {
            if (deleteAction->isEnabled())
                deleteAction->trigger();
            return true;
        }
    }
    else if (e && e->type() == QEvent::Leave) {
        Gui::Selection().rmvPreselect();
    }

    return Gui::TaskView::TaskBox::event(e);
}

bool TaskDressUpParameters::eventFilter(QObject *o, QEvent *e)
{
    if(treeWidget && o == treeWidget) {
        switch(e->type()) {
        case QEvent::Leave:
            enteredObject = nullptr;
            timer->stop();
            Gui::Selection().rmvPreselect();
            break;
        case QEvent::ShortcutOverride:
        case QEvent::KeyPress: {
            QKeyEvent * kevent = static_cast<QKeyEvent*>(e);
            if (kevent->modifiers() == Qt::NoModifier) {
                if (kevent->key() == Qt::Key_Delete) {
                    kevent->accept();
                    if (e->type() == QEvent::KeyPress)
                        onRefDeleted();
                }
            }
            break;
        }
        default:
            break;
        }
    }
    return Gui::TaskView::TaskBox::eventFilter(o,e);
}

//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgDressUpParameters::TaskDlgDressUpParameters(ViewProviderDressUp *DressUpView)
    : TaskDlgFeatureParameters(DressUpView)
    , parameter(0)
{
    assert(DressUpView);
}

TaskDlgDressUpParameters::~TaskDlgDressUpParameters()
{

}

//==== calls from the TaskView ===============================================================

bool TaskDlgDressUpParameters::accept()
{
    std::vector<std::string> refs = parameter->getReferences();
    std::stringstream str;
    str << Gui::Command::getObjectCmd(vp->getObject()) << ".Base = (" 
        << Gui::Command::getObjectCmd(parameter->getBase()) << ",[";
    for (std::vector<std::string>::const_iterator it = refs.begin(); it != refs.end(); ++it)
        str << "\"" << *it << "\",";
    str << "])";
    Gui::Command::runCommand(Gui::Command::Doc,str.str().c_str());
    return TaskDlgFeatureParameters::accept();
}

bool TaskDlgDressUpParameters::reject()
{
    auto editDoc = Gui::Application::Instance->editDocument();
    if(editDoc && parameter->getTransactionID())
        editDoc->getDocument()->undo(parameter->getTransactionID());

    return TaskDlgFeatureParameters::reject();
}

#include "moc_TaskDressUpParameters.cpp"
