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
# include <QMessageBox>
# include <QListWidget>
# include <QAction>
# include <QCheckBox>
# include <QSplitter>
# include <QHeaderView>
# include <QHBoxLayout>
# include <QGridLayout>
# include <TopoDS_Shape.hxx>
# include <TopoDS_Face.hxx>
# include <TopoDS.hxx>
# include <BRepAdaptor_Surface.hxx>
#endif

#include <boost_bind_bind.hpp>
#include <boost/algorithm/string/predicate.hpp>

#include <Base/Console.h>
#include <Base/Tools.h>
#include <App/Application.h>
#include <App/Document.h>
#include <App/Origin.h>
#include <App/OriginFeature.h>
#include <App/MappedElement.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/BitmapFactory.h>
#include <Gui/ViewProvider.h>
#include <Gui/WaitCursor.h>
#include <Gui/Selection.h>
#include <Gui/Command.h>
#include <Gui/ViewParams.h>
#include <Gui/DlgPropertyLink.h>
#include <Gui/Placement.h>

#include <Mod/Part/Gui/PartParams.h>
#include <Mod/Part/App/SubShapeBinder.h>
#include <Mod/PartDesign/App/FeatureTransformed.h>
#include <Mod/PartDesign/App/Body.h>
#include <Mod/PartDesign/App/FeatureAddSub.h>

#include "ReferenceSelection.h"
#include "TaskMultiTransformParameters.h"
#include "Utils.h"

#include "TaskTransformedParameters.h"

namespace bp = boost::placeholders;

FC_LOG_LEVEL_INIT("PartDesign",true,true)

using namespace PartDesignGui;
using namespace Gui;
using namespace Gui::Dialog;

/* TRANSLATOR PartDesignGui::TaskTransformedParameters */

TaskTransformedParameters::TaskTransformedParameters(ViewProviderTransformed *TransformedView, QWidget *parent)
    : TaskBox(Gui::BitmapFactory().pixmap((std::string("PartDesign_") + TransformedView->featureName).c_str()),
              QString::fromUtf8((TransformedView->featureName + " parameters").c_str()), true, parent)
    , proxy(nullptr)
    , TransformedView(TransformedView)
    , parentTask(nullptr)
    , insideMultiTransform(false)
    , blockUpdate(false)
{
    selectionMode = none;

    if (TransformedView) {
        Gui::Document* doc = TransformedView->getDocument();
        this->attachDocument(doc);
    }

    onTopEnabled = Gui::ViewParams::getShowSelectionOnTop();
    if(!onTopEnabled)
        Gui::ViewParams::setShowSelectionOnTop(true);
    // remember initial transaction ID
    App::GetApplication().getActiveTransaction(&transactionID);
}

TaskTransformedParameters::TaskTransformedParameters(TaskMultiTransformParameters *parentTask)
    : TaskBox(QPixmap(), tr(""), true, parentTask),
      proxy(nullptr),
      TransformedView(nullptr),
      parentTask(parentTask),
      insideMultiTransform(true),
      blockUpdate(false)
{
    selectionMode = none;
}

TaskTransformedParameters::~TaskTransformedParameters()
{
    // make sure to remove selection gate in all cases
    Gui::Selection().rmvSelectionGate();

    if(!onTopEnabled)
        Gui::ViewParams::setShowSelectionOnTop(false);
}

void TaskTransformedParameters::slotDeletedObject(const Gui::ViewProviderDocumentObject& Obj)
{
    if (TransformedView == &Obj)
        TransformedView = nullptr;
}

void TaskTransformedParameters::slotUndoDocument(const Gui::Document& Doc)
{
    if (TransformedView && TransformedView->getDocument() == &Doc)
        refresh();
}

void TaskTransformedParameters::slotRedoDocument(const Gui::Document& Doc)
{
    if (TransformedView && TransformedView->getDocument() == &Doc)
        refresh();
}

bool TaskTransformedParameters::isViewUpdated() const
{
    return (blockUpdate == false);
}

int TaskTransformedParameters::getUpdateViewTimeout() const
{
    return 500;
}

void TaskTransformedParameters::onUpdateViewTimer()
{
    if (!blockUpdate) {
        setupTransaction();
        recomputeFeature();
    }
}

void TaskTransformedParameters::kickUpdateViewTimer() const
{
    if (updateViewTimer) {
        PartDesign::Transformed* pcTransformed = getObject();
        int interval = PartGui::PartParams::getEditRecomputeWait();
        if (pcTransformed && pcTransformed->isRecomputePaused())
            interval /= 3;
        updateViewTimer->start(interval);
    } else if (parentTask)
        parentTask->kickUpdateViewTimer();
}

void TaskTransformedParameters::originalSelectionChanged()
{
    std::vector<App::DocumentObject*> objs;
    std::vector<std::string> subs;
    for(auto &link : linkEditor->currentLinks()) {
        objs.push_back(link.getObject());
        subs.push_back(link.getSubName());
    }

    PartDesign::Transformed* pcTransformed = getObject();
    setupTransaction();
    pcTransformed->OriginalSubs.setValues(std::move(objs),std::move(subs));
    recomputeFeature();
}

void TaskTransformedParameters::setupTransaction()
{
    auto obj = getObject();
    if (!obj)
        return;

    int tid = 0;
    const char *name = App::GetApplication().getActiveTransaction(&tid);
    if(tid && tid == transactionID)
        return;

    std::string n("Edit ");
    n += getObject()->getNameInDocument();
    if(!name || n!=name)
        tid = App::GetApplication().setActiveTransaction(n.c_str());

    if(!transactionID)
        transactionID = tid;
}

void TaskTransformedParameters::setupUI() {
    if(!TransformedView || !proxy)
        return;

    updateViewTimer = new QTimer(this);
    updateViewTimer->setSingleShot(true);
    connect(updateViewTimer, SIGNAL(timeout()), this, SLOT(onUpdateViewTimer()));
    
    // remembers the initial transaction ID
    App::GetApplication().getActiveTransaction(&transactionID);

    connMessage = TransformedView->signalDiagnosis.connect(
            boost::bind(&TaskTransformedParameters::slotDiagnosis, this,bp::_1));
    labelMessage = new QLabel(this);
    labelMessage->hide();
    labelMessage->setWordWrap(true);

    linkEditor = new DlgPropertyLink(this, DlgPropertyLink::NoButton
                                          |DlgPropertyLink::NoSearchBox
                                          |DlgPropertyLink::NoTypeFilter
                                          |DlgPropertyLink::NoSubObject
                                          |DlgPropertyLink::AllowSubElement);
    auto treeWidget = linkEditor->treeWidget();
    if(treeWidget && treeWidget->header()) {
        treeWidget->header()->setToolTip(
                tr("Select one or more objects as the base for transformation.\n"
                   "Or Leave it unselected to transform the previous feature.\n\n"
                   "Click item in 'Object' column to make selection in both the\n"
                   "feature list and 3D view.\n\n"
                   "Click item in 'Element' column to make selection only in 3D\n"
                   "view without changing the feature list.\n\n"
                   "Element (Face) selection is only effecitive for features with\n"
                   "multiple solids."));
    }

    linkEditor->setElementFilter([](const App::SubObjectT &sobj, std::string &element) {
        if(!boost::starts_with(element, "Face")) {
            element.clear();
        } else {
            auto feature = Base::freecad_dynamic_cast<PartDesign::Feature>(sobj.getSubObject());
            if(!feature || feature->Shape.getShape().countSubShapes("Solid") <= 1)
                element.clear();
        }
        return false;
    });
    linkEditor->setMinimumHeight(150);

    checkBoxSubTransform = new QCheckBox(this);
    checkBoxSubTransform->setText(tr("Transform sub-feature"));
    checkBoxSubTransform->setToolTip(tr("Check this option to transform individual sub-features,\n"
                                        "or else, transform the entire history up till the selected base."));
    checkBoxSubTransform->setChecked(getObject()->SubTransform.getValue());
    connect(checkBoxSubTransform, SIGNAL(toggled(bool)), this, SLOT(onChangedSubTransform(bool)));

    checkBoxParallel = new QCheckBox(this);
    checkBoxParallel->setText(tr("Operate in parallel"));
    checkBoxParallel->setToolTip(
            tr("Check this option to perform boolean operation on pattern in\n"
               "parallel. Note that this may fail if the pattern shape contains\n"
               "overlap. Uncheck this option to perform operation in sequence."));
    checkBoxParallel->setChecked(getObject()->SubTransform.getValue());
    connect(checkBoxParallel, SIGNAL(toggled(bool)), this, SLOT(onChangedParallelTransform(bool)));

    checkBoxNewSolid = new QCheckBox(this);
    checkBoxNewSolid->setText(tr("New shape"));
    checkBoxNewSolid->setToolTip(tr("Make a new shape using the resulting pattern shape"));
    checkBoxNewSolid->setChecked(getObject()->NewSolid.getValue());
    connect(checkBoxNewSolid, SIGNAL(toggled(bool)), this, SLOT(onChangedNewSolid(bool)));

    auto layout = qobject_cast<QBoxLayout*>(proxy->layout());
    assert(layout);

    auto grid = PartDesignGui::addTaskCheckBox(proxy);
    grid->addWidget(checkBoxParallel, 1, 1);
    grid->addWidget(checkBoxNewSolid, 2, 0);
    grid->addWidget(checkBoxSubTransform, 2, 1);

    splitter = new QSplitter(Qt::Vertical, this);
    splitter->addWidget(labelMessage);
    splitter->addWidget(linkEditor);
    splitter->addWidget(proxy);
    proxy->setMinimumHeight(proxy->minimumHeight() + defaultMinimumHeight);

    this->groupLayout()->addWidget(splitter);

    auto editDoc = Gui::Application::Instance->editDocument();
    if(editDoc) {
        ViewProviderDocumentObject *editVp = nullptr;
        std::string subname;
        editDoc->getInEdit(&editVp,&subname);
        if(editVp) {
            auto sobjs = editVp->getObject()->getSubObjectList(subname.c_str());
            while(sobjs.size()) {
                if(Base::freecad_dynamic_cast<PartDesign::Body>(sobjs.back()))
                    break;
                sobjs.pop_back();
            }
            if(sobjs.size()) {
                std::ostringstream ss;
                for(size_t i=1;i<sobjs.size();++i)
                    ss << sobjs[i]->getNameInDocument() << ".";
                linkEditor->setContext(App::SubObjectT(sobjs.front(), ss.str().c_str()));
            }
        }
    }

    PartDesign::Transformed* pcTransformed = getObject();
    auto body = PartDesign::Body::findBodyOf(pcTransformed);
    if(body) {
        std::vector<App::DocumentObjectT> objs;
        for(auto child : body->Group.getValues()) {
            if (child == pcTransformed)
                continue;
            if(child->isDerivedFrom(PartDesign::Transformed::getClassTypeId())) {
                if (static_cast<PartDesign::Transformed*>(child)->getBaseObject(true))
                    objs.emplace_back(child);
                continue;
            }
            if(child->isDerivedFrom(PartDesign::Feature::getClassTypeId())
                    || child->isDerivedFrom(Part::SubShapeBinder::getClassTypeId())
                    || child->isDerivedFrom(Part::Part2DObject::getClassTypeId()))
                objs.emplace_back(child);
        }
        linkEditor->setInitObjects(std::move(objs));
    }

    auto values = pcTransformed->OriginalSubs.getValues();
    auto itValue = values.begin();
    const auto &shadows = pcTransformed->OriginalSubs.getShadowSubs();
    auto itShadow = shadows.begin();
    PartDesign::Feature *feat = 0;
    auto subs = pcTransformed->OriginalSubs.getSubValues(false);
    bool touched = false;
    for(auto &sub : subs) {
        auto obj = *itValue++;
        const auto &shadow = *itShadow++;
        if(feat!=obj)
            feat = Base::freecad_dynamic_cast<PartDesign::Feature>(obj);
        if(feat && shadow.first.size()) {
            try {
                feat->Shape.getShape().getSubShape(shadow.first.c_str());
            }catch(...) {
                auto names = Part::Feature::getRelatedElements(obj,shadow.first.c_str());
                if(names.size()) {
                    auto &mapped = names.front();
                    FC_WARN("guess element reference: " << shadow.first << " -> " << mapped.name);
                    sub.clear();
                    mapped.index.toString(sub);
                    touched = true;
                } else {
                    sub = shadow.first; // use new style name for future guessing
                }
            }
        }
    }

    if(touched) {
        setupTransaction();
        pcTransformed->OriginalSubs.setValues(values,subs);
        recomputeFeature();
    }

    linkEditor->init(App::DocumentObjectT(&pcTransformed->OriginalSubs),false);

    QObject::connect(linkEditor, SIGNAL(linkChanged()), this, SLOT(originalSelectionChanged()));
}

void TaskTransformedParameters::slotDiagnosis(QString msg)
{
    if(labelMessage) {
        if(msg.isEmpty())
            labelMessage->hide();
        else {
            labelMessage->show();
            labelMessage->setText(msg);
        }
    }
}

void TaskTransformedParameters::refresh()
{
    if(TransformedView) {
        auto pcTransformed = static_cast<PartDesign::Transformed*>(TransformedView->getObject());
        if(linkEditor) {
            QSignalBlocker blocker(linkEditor);
            linkEditor->init(App::DocumentObjectT(&pcTransformed->OriginalSubs),false);
        }
        if(checkBoxSubTransform) {
            QSignalBlocker blocker(checkBoxSubTransform);
            checkBoxSubTransform->setChecked(getObject()->SubTransform.getValue());
        }
        if(checkBoxParallel) {
            QSignalBlocker blocker(checkBoxParallel);
            checkBoxParallel->setChecked(getObject()->ParallelTransform.getValue());
        }
        if (transformOffsetPlacement)
            transformOffsetPlacement->setPlacement(pcTransformed->TransformOffset.getValue());

    }
    updateUI();
}

void TaskTransformedParameters::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if(selectionMode == none && linkEditor)
        linkEditor->selectionChanged(msg);
}

void TaskTransformedParameters::fillAxisCombo(ComboLinks &combolinks,
                                              Part::Part2DObject* sketch)
{
    combolinks.clear();

    //add sketch axes
    if (sketch){
        combolinks.addLink(sketch, "N_Axis",tr("Normal sketch axis"));
        combolinks.addLink(sketch,"V_Axis",tr("Vertical sketch axis"));
        combolinks.addLink(sketch,"H_Axis",tr("Horizontal sketch axis"));
        for (int i=0; i < sketch->getAxisCount(); i++) {
            QString itemText = tr("Construction line %1").arg(i+1);
            std::stringstream sub;
            sub << "Axis" << i;
            combolinks.addLink(sketch,sub.str(),itemText);
        }
    }

    //add part axes
    App::DocumentObject* obj = getObject();
    PartDesign::Body * body = PartDesign::Body::findBodyOf ( obj );

    if (body) {
        try {
            App::Origin* orig = body->getOrigin();
            combolinks.addLink(orig->getX(),"",tr("Base X axis"));
            combolinks.addLink(orig->getY(),"",tr("Base Y axis"));
            combolinks.addLink(orig->getZ(),"",tr("Base Z axis"));
        } catch (const Base::Exception &ex) {
            Base::Console().Error ("%s\n", ex.what() );
        }
    }

    //add "Select reference"
    combolinks.addLink(0,std::string(),tr("Select reference..."));
}

void TaskTransformedParameters::fillPlanesCombo(ComboLinks &combolinks,
                                                Part::Part2DObject* sketch)
{
    combolinks.clear();

    //add sketch axes
    if (sketch){
        combolinks.addLink(sketch,"V_Axis",QObject::tr("Vertical sketch axis"));
        combolinks.addLink(sketch,"H_Axis",QObject::tr("Horizontal sketch axis"));
        for (int i=0; i < sketch->getAxisCount(); i++) {
            QString itemText = tr("Construction line %1").arg(i+1);
            std::stringstream sub;
            sub << "Axis" << i;
            combolinks.addLink(sketch,sub.str(),itemText);
        }
    }

    //add part baseplanes
    App::DocumentObject* obj = getObject();
    PartDesign::Body * body = PartDesign::Body::findBodyOf ( obj );

    if (body) {
        try {
            App::Origin* orig = body->getOrigin();
            combolinks.addLink(orig->getXY(),"",tr("Base XY plane"));
            combolinks.addLink(orig->getYZ(),"",tr("Base YZ plane"));
            combolinks.addLink(orig->getXZ(),"",tr("Base XZ plane"));
        } catch (const Base::Exception &ex) {
            Base::Console().Error ("%s\n", ex.what() );
        }
    }

    //add "Select reference"
    combolinks.addLink(0,std::string(),tr("Select reference..."));
}

void TaskTransformedParameters::recomputeFeature() {
    Gui::WaitCursor cursor;
    getTopTransformedView()->recomputeFeature();
}

PartDesignGui::ViewProviderTransformed *TaskTransformedParameters::getTopTransformedView(bool silent) const {
    PartDesignGui::ViewProviderTransformed *rv;

    if (insideMultiTransform) {
        rv = parentTask->TransformedView;
    } else {
        rv = TransformedView;
    }

    if(!rv && !silent)
        throw Base::RuntimeError("No Transformed object");

    return rv;
}

PartDesign::Transformed *TaskTransformedParameters::getTopTransformedObject(bool silent) const {
    auto view = getTopTransformedView(silent);
    if(!view)
        return nullptr;
    App::DocumentObject *transform = view->getObject();
    assert (transform->isDerivedFrom(PartDesign::Transformed::getClassTypeId()));
    return static_cast<PartDesign::Transformed*>(transform);
}

PartDesign::Transformed *TaskTransformedParameters::getObject() const {
    if (insideMultiTransform)
        return parentTask->getSubFeature();
    else if (TransformedView)
        return static_cast<PartDesign::Transformed*>(TransformedView->getObject());
    else
        return nullptr;
}

App::DocumentObject *TaskTransformedParameters::getBaseObject() const {
    PartDesign::Feature* feature = getTopTransformedObject ();
    // NOTE: getBaseObject() throws if there is no base; shouldn't happen here.
    App::DocumentObject *base = feature->getBaseObject(true);
    if(!base) {
        auto body = feature->getFeatureBody();
        if(body)
            base = body->getPrevSolidFeature(feature);
    }
    return base;
}

App::DocumentObject* TaskTransformedParameters::getSketchObject() const {
    return getTopTransformedObject()->getSketchObject();
}

void TaskTransformedParameters::exitSelectionMode()
{
    try {
        selectionMode = none;
        Gui::Selection().rmvSelectionGate();
        Gui::Selection().clearSelection();
    } catch(Base::Exception &e) {
        e.ReportException();
    }
}

void TaskTransformedParameters::addReferenceSelectionGate(bool edge, bool face)
{
    ReferenceSelection::Config conf;
    conf.edge = edge;
    conf.plane = face;
    conf.planar = true;
    conf.whole = false;
    conf.circle = false;
    addReferenceSelectionGate(conf);
}

void TaskTransformedParameters::addReferenceSelectionGate(const ReferenceSelection::Config &conf)
{
    std::unique_ptr<Gui::SelectionFilterGate> gateRefPtr(
            new ReferenceSelection(getBaseObject(), conf));
    std::unique_ptr<Gui::SelectionFilterGate> gateDepPtr(new NoDependentsSelection(getTopTransformedObject()));
    Gui::Selection().addSelectionGate(new CombineSelectionFilterGates(gateRefPtr, gateDepPtr));
}

void TaskTransformedParameters::onChangedSubTransform(bool checked) {
    setupTransaction();
    getObject()->SubTransform.setValue(checked);
    recomputeFeature();
}

void TaskTransformedParameters::onChangedParallelTransform(bool checked) {
    setupTransaction();
    getObject()->ParallelTransform.setValue(checked);
    recomputeFeature();
}

void TaskTransformedParameters::onChangedNewSolid(bool checked) {
    setupTransaction();
    getObject()->NewSolid.setValue(checked);
    recomputeFeature();
}

void TaskTransformedParameters::onChangedOffset(const QVariant &data, bool incr, bool)
{
    setupTransaction();
    auto pla = qvariant_cast<Base::Placement>(data);
    if (incr)
        getObject()->TransformOffset.setValue(getObject()->TransformOffset.getValue() * pla);
    else
        getObject()->TransformOffset.setValue(pla);
    kickUpdateViewTimer();
}

void TaskTransformedParameters::onToggledExpansion()
{
    if (this->isGroupVisible())
        exitSelectionMode();
    else
        selectionMode = placement;
}

//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgTransformedParameters::TaskDlgTransformedParameters(
        ViewProviderTransformed *TransformedView_, TaskTransformedParameters *parameter)
    : TaskDlgFeatureParameters(TransformedView_), parameter(parameter)
{
    assert(vp);

    Content.push_back(parameter);

    auto feat = Base::freecad_dynamic_cast<PartDesign::Transformed>(vp->getObject());
    if (feat) {
        auto widget = new Gui::Dialog::Placement();
        parameter->transformOffsetPlacement = widget;
        widget->showDefaultButtons(false);
        widget->bindObject(&feat->TransformOffset);
        widget->setPlacement(feat->TransformOffset.getValue());
        taskTransformOffset = new Gui::TaskView::TaskBox(QPixmap(), tr("Transform offset"), true, 0);
        taskTransformOffset->groupLayout()->addWidget(widget);

        Content.push_back(taskTransformOffset);
        taskTransformOffset->hideGroupBox();

        connect(widget, SIGNAL(placementChanged(const QVariant &, bool, bool)),
                parameter, SLOT(onChangedOffset(const QVariant &, bool, bool)));
        connect(taskTransformOffset, SIGNAL(toggledExpansion()), this, SLOT(onToggledTaskOffset()));
        connect(parameter, SIGNAL(toggledExpansion()), this, SLOT(onToggledTaskParameters()));
    }
}

//==== calls from the TaskView ===============================================================

bool TaskDlgTransformedParameters::accept()
{
    parameter->exitSelectionMode();

    // Continue (usually in virtual method accept())
    return TaskDlgFeatureParameters::accept ();
}

bool TaskDlgTransformedParameters::reject()
{
    // ensure that we are not in selection mode
    parameter->exitSelectionMode();

    auto editDoc = Gui::Application::Instance->editDocument();
    if(editDoc && parameter->getTransactionID())
        editDoc->getDocument()->undo(parameter->getTransactionID());

    return TaskDlgFeatureParameters::reject ();
}

void TaskDlgTransformedParameters::onToggledTaskOffset()
{
    if (taskTransformOffset->foldDirection() == parameter->foldDirection()) {
        parameter->showHide();
        parameter->onToggledExpansion();
    }
}

void TaskDlgTransformedParameters::onToggledTaskParameters()
{
    if (taskTransformOffset->foldDirection()>0 && parameter->foldDirection()>0)
        taskTransformOffset->showHide();
}

#include "moc_TaskTransformedParameters.cpp"


ComboLinks::ComboLinks(QComboBox &combo)
    : doc(0)
{
    this->_combo = &combo;
    _combo->clear();
}

int ComboLinks::addLink(const App::PropertyLinkSub &lnk, QString itemText)
{
    if(!_combo)
        return 0;
    _combo->addItem(itemText);
    this->linksInList.push_back(new App::PropertyLinkSub());
    App::PropertyLinkSub &newitem = *(linksInList[linksInList.size()-1]);
    newitem.Paste(lnk);
    if (newitem.getValue() && this->doc == 0)
        this->doc = newitem.getValue()->getDocument();
    return linksInList.size()-1;
}

int ComboLinks::addLink(App::DocumentObject *linkObj, std::string linkSubname, QString itemText)
{
    if(!_combo)
        return 0;
    _combo->addItem(itemText);
    this->linksInList.push_back(new App::PropertyLinkSub());
    App::PropertyLinkSub &newitem = *(linksInList[linksInList.size()-1]);
    newitem.setValue(linkObj,std::vector<std::string>(1,linkSubname));
    if (newitem.getValue() && this->doc == 0)
        this->doc = newitem.getValue()->getDocument();
    return linksInList.size()-1;
}

void ComboLinks::clear()
{
    for(size_t i = 0  ;  i < this->linksInList.size()  ;  i++){
        delete linksInList[i];
    }
    if(this->_combo)
        _combo->clear();
}

App::PropertyLinkSub &ComboLinks::getLink(int index) const
{
    if (index < 0 || index > (ssize_t) linksInList.size()-1)
        throw Base::IndexError("ComboLinks::getLink:Index out of range");
    if (linksInList[index]->getValue() && doc && !(doc->isIn(linksInList[index]->getValue())))
        throw Base::ValueError("Linked object is not in the document; it may have been deleted");
    return *(linksInList[index]);
}

App::PropertyLinkSub &ComboLinks::getCurrentLink() const
{
    assert(_combo);
    return getLink(_combo->currentIndex());
}

int ComboLinks::setCurrentLink(const App::PropertyLinkSub &lnk)
{
    for(size_t i = 0  ;  i < linksInList.size()  ;  i++) {
        App::PropertyLinkSub &it = *(linksInList[i]);
        if(lnk.getValue() == it.getValue() && lnk.getSubValues() == it.getSubValues()){
            bool wasBlocked = _combo->signalsBlocked();
            _combo->blockSignals(true);
            _combo->setCurrentIndex(i);
            _combo->blockSignals(wasBlocked);
            return i;
        }
    }
    return -1;
}
