/***************************************************************************
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>              *
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
# include <deque>
# include <sstream>
# include <QByteArray>
# include <QContextMenuEvent>
# include <QHeaderView>
# include <QInputDialog>
# include <QMessageBox>
# include <QMenu>
# include <QTreeWidget>
# include <QDesktopWidget>
# include <QTimer>
#endif

#include <iomanip>
#include <boost/regex.hpp>
#include <boost/algorithm/string/predicate.hpp>

#include <App/Application.h>
#include <Base/Exception.h>
#include <Base/Parameter.h>
#include <Base/Tools.h>

#include "DlgParameterImp.h"
#include "ui_DlgParameter.h"
#include "Action.h"
#include "Application.h"
#include "BitmapFactory.h"
#include "Command.h"
#include "DlgParameterFind.h"
#include "DlgInputDialogImp.h"
#include "FileDialog.h"
#include "MainWindow.h"
#include "SpinBox.h"
#include "PrefWidgets.h"
#include "ViewParams.h"


namespace bp = boost::placeholders;
using namespace Gui::Dialog;

static QByteArray _LastParameterSet;

/* TRANSLATOR Gui::Dialog::DlgParameterImp */

/**
 *  Constructs a DlgParameterImp which is a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
DlgParameterImp::DlgParameterImp( QWidget* parent,  Qt::WindowFlags fl )
  : QWidget(parent, fl|Qt::WindowMinMaxButtonsHint|Qt::WindowCloseButtonHint)
  , ui(new Ui_DlgParameter)
  , widgetStates(new PrefWidgetStates(this))
{
    ui->setupUi(this);
    setupConnections();

    ui->checkSort->setVisible(false); // for testing

    widgetStates->addSplitter(ui->splitter3);

    setAttribute(Qt::WA_DeleteOnClose);

    QStringList groupLabels;
    groupLabels << tr( "Group" );
    paramGroup = new ParameterGroup(this, ui->splitter3);
    paramGroup->setHeaderLabels(groupLabels);
    paramGroup->setRootIsDecorated(true);
    paramGroup->setSortingEnabled(true);
    paramGroup->sortByColumn(0, Qt::AscendingOrder);
    paramGroup->header()->setProperty("showSortIndicator", QVariant(true));

    QStringList valueLabels;
    valueLabels << tr( "Name" ) << tr( "Type" ) << tr( "Value" );
    paramValue = new ParameterValue(this, ui->splitter3);
    paramValue->setHeaderLabels(valueLabels);
    paramValue->setRootIsDecorated(false);
    paramValue->setSelectionMode(QTreeView::ExtendedSelection);
    paramValue->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    paramValue->setSortingEnabled(true);
    paramValue->sortByColumn(0, Qt::AscendingOrder);
    paramValue->header()->setProperty("showSortIndicator", QVariant(true));

    connect(paramGroup, &QTreeWidget::itemChanged,
            this, &DlgParameterImp::onGroupItemChanged);
    connect(paramValue, &QTreeWidget::itemChanged,
            this, &DlgParameterImp::onValueItemChanged);

    QSizePolicy policy = paramValue->sizePolicy();
    policy.setHorizontalStretch(3);
    paramValue->setSizePolicy(policy);

#if 0 // This is needed for Qt's lupdate
    qApp->translate( "Gui::Dialog::DlgParameterImp", "System parameter" );
    qApp->translate( "Gui::Dialog::DlgParameterImp", "User parameter" );
#endif

    connect(ui->parameterSet, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &DlgParameterImp::onChangeParameterSet);
    connect(paramGroup, &QTreeWidget::currentItemChanged,
            this, &DlgParameterImp::onGroupSelected);
    onGroupSelected(paramGroup->currentItem());

    // setup for function onFindGroupTextChanged:
    // store the current font properties because
    // we don't know what style sheet the user uses for FC
    defaultFont = paramGroup->font();
    boldFont = defaultFont;
    boldFont.setBold(true);

    ui->findGroupLE->setPlaceholderText(tr("Search Group"));

    populate();

    QObject::connect(&actMerge, &QAction::triggered, [this]() {
        doImportOrMerge(curParamManager, true);
    });

    QMenu *menu = new QMenu(this);
    ui->btnMerge->setMenu(menu);
    QObject::connect(menu, &QMenu::aboutToShow, [this, menu]() {
        menu->clear();
        actMerge.setText(tr("From file..."));
        menu->addAction(&actMerge);
        bool first = true;
        for (auto &v : monitors) {
            if (v.first == curParamManager || v.second.changes.empty())
                continue;
            for (int i=0, c=ui->parameterSet->count(); i<c; ++i) {
                auto name = ui->parameterSet->itemData(i).toByteArray();
                if (App::GetApplication().GetParameterSet(name) == v.first) {
                    if (first) {
                        first = false;
                        menu->addSeparator();
                    }
                    auto action = menu->addAction(ui->parameterSet->itemText(i));
                    ParameterManager *manager = v.first;
                    QObject::connect(action, &QAction::triggered, [this, manager]() {
                        try {
                            auto h = copyParameters(manager);
                            if (h)
                                h->insertTo((ParameterManager*)curParamManager);
                        } catch(Base::Exception &e) {
                            e.ReportException();
                        }
                        onChangeParameterSet(ui->parameterSet->currentIndex());
                    });
                }
            }
        }
    });
}

/**
 *  Destroys the object and frees any allocated resources
 */
DlgParameterImp::~DlgParameterImp()
{
    saveState();
    // no need to delete child widgets, Qt does it all for us
    delete ui;
    paramValue->_owner = nullptr;
    paramGroup->_owner = nullptr;
}

void DlgParameterImp::setupConnections()
{
    connect(ui->buttonFind, &QPushButton::clicked,
            this, &DlgParameterImp::onButtonFindClicked);
    connect(ui->findGroupLE, &QLineEdit::textChanged,
            this, &DlgParameterImp::onFindGroupTextChanged);
    connect(ui->buttonSaveToDisk, &QPushButton::clicked,
            this, &DlgParameterImp::onButtonSaveToDiskClicked);
    connect(ui->btnExport, &QPushButton::clicked,
            this, &DlgParameterImp::onButtonExportClicked);
    connect(ui->btnImport, &QPushButton::clicked,
            this, &DlgParameterImp::onButtonImportClicked);
    connect(ui->btnAdd, &QPushButton::clicked,
            this, &DlgParameterImp::onButtonAddClicked);
    connect(ui->btnRename, &QPushButton::clicked,
            this, &DlgParameterImp::onButtonRenameClicked);
    connect(ui->btnRemove, &QPushButton::clicked,
            this, &DlgParameterImp::onButtonRemoveClicked);
    connect(ui->btnToolTip, &QPushButton::clicked,
            this, &DlgParameterImp::onButtonToolTipClicked);
    connect(ui->btnRefresh, &QPushButton::clicked,
            this, &DlgParameterImp::onButtonRefreshClicked);
    connect(ui->btnRestart, &QPushButton::clicked,
            this, &DlgParameterImp::onButtonRestartClicked);
    connect(ui->btnReset, &QPushButton::clicked,
            this, &DlgParameterImp::onButtonResetClicked);
    connect(ui->closeButton, &QPushButton::clicked,
            this, &DlgParameterImp::onCloseButtonClicked);
    connect(ui->btnCopy, &QPushButton::clicked,
            this, &DlgParameterImp::onButtonCopyClicked);
    connect(ui->checkSort, &QCheckBox::toggled,
            this, &DlgParameterImp::onCheckSortToggled);
    connect(ui->checkBoxMonitor, &QCheckBox::toggled,
            this, &DlgParameterImp::onCheckBoxMonitorToggled);
    connect(ui->checkBoxPreset, &QCheckBox::toggled,
            this, &DlgParameterImp::onCheckBoxPresetToggled);
}

void DlgParameterImp::onButtonFindClicked()
{
    if (finder.isNull())
        finder = new DlgParameterFind(this);
    finder->show();
}

void DlgParameterImp::onButtonCopyClicked()
{
    std::string name = QStringLiteral("%1(copy)").arg(
            ui->parameterSet->currentText()).toUtf8().constData();
    auto manager = App::GetApplication().AddParameterSet(
            name, curParamManager->GetSerializeFileName());
    std::string tooltip = curParamManager->GetASCII("ToolTip");
    if (auto h = copyParameters(curParamManager)) {
        h->SetASCII("Name", name);
        h->SetASCII("ToolTip", tooltip);
        h->copyTo(manager.get());
    }

    onButtonRefreshClicked();
    int idx = ui->parameterSet->findData(QByteArray(name.c_str()));
    if (idx >= 0)
        ui->parameterSet->setCurrentIndex(idx);
}

void DlgParameterImp::onFindGroupTextChanged(const QString &SearchStr)
{
    // search for group tree items and highlight found results

    QTreeWidgetItem* ExpandItem;

    // at first reset all items to the default font and expand state
    if (!foundList.empty()) {
        for (QTreeWidgetItem* item : qAsConst(foundList)) {
            item->setFont(0, defaultFont);
            item->setForeground(0, defaultColor);
            ExpandItem = item;
            // a group can be nested down to several levels
            // do not collapse if the search string is empty
            while (!SearchStr.isEmpty()) {
                if (!ExpandItem->parent())
                    break;
                else {
                    ExpandItem->setExpanded(false);
                    ExpandItem = ExpandItem->parent();
                }
            }
        }
    }
    // expand the top level entries to get the initial tree state
    for (int i = 0; i < paramGroup->topLevelItemCount(); ++i) {
        paramGroup->topLevelItem(i)->setExpanded(true);
    }

    // don't perform a search if the string is empty
    if (SearchStr.isEmpty())
        return;

    // search the tree widget
    foundList = paramGroup->findItems(SearchStr, Qt::MatchContains | Qt::MatchRecursive);
    if (!foundList.empty()) {
        // reset background style sheet
        if (!ui->findGroupLE->styleSheet().isEmpty())
            ui->findGroupLE->setStyleSheet(QString());
        for (QTreeWidgetItem* item : qAsConst(foundList)) {
            item->setFont(0, boldFont);
            item->setForeground(0, Qt::red);
            // expand its parent to see the item
            // a group can be nested down to several levels
            ExpandItem = item;
            while (true) {
                if (!ExpandItem->parent())
                    break;
                else {
                    ExpandItem->setExpanded(true);
                    ExpandItem = ExpandItem->parent();
                }
            }
            // if there is only one item found, scroll to it
            if (foundList.size() == 1) {
                paramGroup->scrollToItem(foundList[0], QAbstractItemView::PositionAtCenter);
            }
        }
    }
    else {
        // Set red background to indicate no matching
        QString styleSheet = QStringLiteral(
            " QLineEdit {\n"
            "     background-color: rgb(221,144,161);\n"
            " }\n"
        );
        ui->findGroupLE->setStyleSheet(styleSheet);
    }
}

/**
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void DlgParameterImp::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
        paramGroup->headerItem()->setText( 0, tr( "Group" ) );
        paramValue->headerItem()->setText( 0, tr( "Name" ) );
        paramValue->headerItem()->setText( 1, tr( "Type" ) );
        paramValue->headerItem()->setText( 2, tr( "Value" ) );
    } else {
        QWidget::changeEvent(e);
    }
}

void DlgParameterImp::onCheckSortToggled(bool on)
{
    paramGroup->setSortingEnabled(on);
    paramGroup->sortByColumn(0, Qt::AscendingOrder);
    paramGroup->header()->setProperty("showSortIndicator", QVariant(on));

    paramValue->setSortingEnabled(on);
    paramValue->sortByColumn(0, Qt::AscendingOrder);
    paramValue->header()->setProperty("showSortIndicator", QVariant(on));
}

void DlgParameterImp::onCloseButtonClicked()
{
    close();
}

void DlgParameterImp::removeState()
{
    ParameterGrp::handle hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp/Preferences/ParameterEditor/LastGroup");
    int index = ui->parameterSet->currentIndex();
    if (index >= 0)
        hGrp->RemoveASCII(ui->parameterSet->itemData(index).toByteArray().constData());
    lastIndex = -1;
}

void DlgParameterImp::saveState()
{
    ParameterGrp::handle hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp/Preferences/ParameterEditor");
    QTreeWidgetItem* current = paramGroup->currentItem();
    if (current && lastIndex >= 0) {
        QStringList paths;
        paths << current->text(0);
        QTreeWidgetItem* parent = current->parent();
        while (parent) {
            paths.push_front(parent->text(0));
            parent = parent->parent();
        }

        QString path = paths.join(QStringLiteral("."));
        hGrp->GetGroup("LastGroup")->SetASCII(
                ui->parameterSet->itemData(lastIndex).toByteArray(),
                (const char*)path.toUtf8());
        _LastParameterSet = ui->parameterSet->itemData(lastIndex).toByteArray();
    }
    lastIndex = ui->parameterSet->currentIndex();
}

void DlgParameterImp::onGroupSelected( QTreeWidgetItem * item )
{
    if ( item && item->type() == QTreeWidgetItem::UserType + 1 )
    {
        QSignalBlocker block(paramValue);

        bool sortingEnabled = paramValue->isSortingEnabled();
        paramValue->clear();
        Base::Reference<ParameterGrp> _hcGrp = static_cast<ParameterGroupItem*>(item)->_hcGrp;
        static_cast<ParameterValue*>(paramValue)->setCurrentGroup( _hcGrp );

        // filling up Text nodes
        std::vector<std::pair<std::string,std::string> > mcTextMap = _hcGrp->GetASCIIMap();
        for(const auto & It2 : mcTextMap)
        {
            (void)new ParameterText(paramValue,QString::fromUtf8(It2.first.c_str()),
                It2.second.c_str(), _hcGrp);
        }

        // filling up Int nodes
        std::vector<std::pair<std::string,long> > mcIntMap = _hcGrp->GetIntMap();
        for(const auto & It3 : mcIntMap)
        {
            (void)new ParameterInt(paramValue,QString::fromUtf8(It3.first.c_str()),It3.second, _hcGrp);
        }

        // filling up Float nodes
        std::vector<std::pair<std::string,double> > mcFloatMap = _hcGrp->GetFloatMap();
        for(const auto & It4 : mcFloatMap)
        {
            (void)new ParameterFloat(paramValue,QString::fromUtf8(It4.first.c_str()),It4.second, _hcGrp);
        }

        // filling up bool nodes
        std::vector<std::pair<std::string,bool> > mcBoolMap = _hcGrp->GetBoolMap();
        for(const auto & It5 : mcBoolMap)
        {
            (void)new ParameterBool(paramValue,QString::fromUtf8(It5.first.c_str()),It5.second, _hcGrp);
        }

        // filling up UInt nodes
        std::vector<std::pair<std::string,unsigned long> > mcUIntMap = _hcGrp->GetUnsignedMap();
        for(const auto & It6 : mcUIntMap)
        {
            (void)new ParameterUInt(paramValue,QString::fromUtf8(It6.first.c_str()),It6.second, _hcGrp);
        }
        paramValue->setSortingEnabled(sortingEnabled);

        auto it = monitors.find(curParamManager);
        if (it != monitors.end()) {
            auto &changes = it->second.changes;
            auto iter = changes.find(paramValue->currentGroup());
            for (QTreeWidgetItemIterator it(paramValue); *it; ++it) {
                auto item = static_cast<ParameterValueItem*>(*it);
                if (iter != changes.end()
                        && (iter->second.empty() || iter->second.count(item->getKey())))
                    item->setCheckState(0, Qt::Checked);
                else
                    item->setCheckState(0, Qt::Unchecked);
            }
        }
    }
    else
        paramValue->clear();
}

/** Switches the type of parameters with name @a config. */
void DlgParameterImp::activateParameterSet(const char* config)
{
    int index = ui->parameterSet->findData(QByteArray(config));
    if (index != -1) {
        ui->parameterSet->setCurrentIndex(index);
        onChangeParameterSet(index);
    }
}

bool DlgParameterImp::isReadOnly(ParameterManager* mgr)
{
    if (!mgr)
        mgr = curParamManager;
    return boost::starts_with(mgr->GetSerializeFileName(), App::GetApplication().getResourceDir());
}

/** Switches the type of parameters either to user or system parameters. */
void DlgParameterImp::onChangeParameterSet(int itemPos)
{
    QByteArray paramName = ui->parameterSet->itemData(itemPos).toByteArray();
    ParameterManager* rcParMngr = App::GetApplication().GetParameterSet(paramName);
    if (!rcParMngr)
        return;

    saveState();

    bool readonly =  isReadOnly(rcParMngr);
    bool editable = !readonly && rcParMngr != &App::GetApplication().GetUserParameter()
            && rcParMngr != &App::GetApplication().GetSystemParameter();
    ui->parameterSet->setEditable(false);
    ui->btnRename->setEnabled(editable);
    ui->btnRemove->setEnabled(editable);
    ui->checkBoxPreset->setEnabled(editable);
    ui->btnToolTip->setEnabled(editable);

    ui->btnMerge->setDisabled(readonly);
    ui->btnImport->setDisabled(readonly);
    ui->btnReset->setDisabled(readonly);
    
    paramGroup->subGrpAct->setDisabled(readonly);
    paramGroup->removeAct->setDisabled(readonly);
    paramGroup->renameAct->setDisabled(readonly);
    paramGroup->exportAct->setDisabled(readonly);
    paramGroup->importAct->setDisabled(readonly);
    paramGroup->mergeAct->setDisabled(readonly);

    rcParMngr->CheckDocument();
    ui->buttonSaveToDisk->setEnabled(rcParMngr->HasSerializer() && !readonly);

    QSignalBlocker block(paramGroup);
    QSignalBlocker block2(paramValue);

    // remove all labels
    paramGroup->clear();
    paramValue->clear();

    // root labels
    std::vector<Base::Reference<ParameterGrp> > grps = rcParMngr->GetGroups();
    for (const auto & grp : grps) {
        auto item = new ParameterGroupItem(paramGroup, grp);
        paramGroup->expandItem(item);
        item->setIcon(0, QApplication::style()->standardPixmap(QStyle::SP_ComputerIcon));
    }

    // get the path of the last selected group in the editor
    ParameterGrp::handle hGrp = App::GetApplication().GetUserParameter().GetGroup(
            "BaseApp/Preferences/ParameterEditor/LastGroup");
    QString path = QString::fromUtf8(hGrp->GetASCII(paramName).c_str());
#if QT_VERSION >= QT_VERSION_CHECK(5,15,0)
    QStringList paths = path.split(QStringLiteral("."), Qt::SkipEmptyParts);
#else
    QStringList paths = path.split(QStringLiteral("."), QString::SkipEmptyParts);
#endif

    QTreeWidgetItem* parent = nullptr;
    for (int index=0; index < paramGroup->topLevelItemCount() && !paths.empty(); index++) {
        QTreeWidgetItem* child = paramGroup->topLevelItem(index);
        if (child->text(0) == paths.front()) {
            paths.pop_front();
            parent = child;
        }
    }

    while (parent && !paths.empty()) {
        parent->setExpanded(true);
        QTreeWidgetItem* item = parent;
        parent = nullptr;
        for (int index=0; index < item->childCount(); index++) {
            QTreeWidgetItem* child = item->child(index);
            if (child->text(0) == paths.front()) {
                paths.pop_front();
                parent = child;
                break;
            }
        }
    }

    if (parent)
        paramGroup->setCurrentItem(parent);
    else if (paramGroup->topLevelItemCount() > 0)
        paramGroup->setCurrentItem(paramGroup->topLevelItem(0));

    curParamManager = rcParMngr;
    {
        QSignalBlocker blocker(ui->checkBoxPreset);
        ui->checkBoxPreset->setChecked(curParamManager->GetBool("Preset", true));
    }

    if (auto item = paramGroup->currentItem())
        onGroupSelected(item);

    {
        QSignalBlocker block(ui->checkBoxMonitor);
        ui->checkBoxMonitor->setChecked(monitors.count(rcParMngr) > 0);
        onCheckBoxMonitorToggled(ui->checkBoxMonitor->isChecked());
    }
}

void DlgParameterImp::onCheckBoxMonitorToggled(bool checked)
{
    if (!checked) {
        monitors.erase(curParamManager);
        for (QTreeWidgetItemIterator it(paramGroup); *it; ++it)
            (*it)->setData(0, Qt::CheckStateRole, QVariant());
        for (QTreeWidgetItemIterator it(paramValue); *it; ++it)
            (*it)->setData(0, Qt::CheckStateRole, QVariant());
        return;
    }
    auto &monitor = monitors[curParamManager];
    if (!monitor.handle) {
        monitor.handle = curParamManager;
        monitor.conn = curParamManager->signalParamChanged.connect(
                boost::bind(&DlgParameterImp::slotParamChanged,
                this, bp::_1, bp::_2, bp::_3, bp::_4));
    }
    auto &changes = monitor.changes;
    QSignalBlocker block(paramGroup);
    for (QTreeWidgetItemIterator it(paramGroup); *it; ++it) {
        auto item = static_cast<ParameterGroupItem*>(*it);
        if (!changes.count(item->_hcGrp))
            item->setCheckState(0, Qt::Unchecked);
        else if (checkGroupItemState(item, Qt::Checked))
            setGroupItemState(item, Qt::Checked);
        else
            setGroupItemState(item, Qt::PartiallyChecked);
        if (item->checkState(0) != Qt::Unchecked) {
            for (QTreeWidgetItem *t = item; t; t = t->parent())
                t->setExpanded(true);
        }
    }
    auto iter = changes.find(paramValue->currentGroup());
    if (iter != changes.end()) {
        QSignalBlocker block(paramValue);
        auto &keys = iter->second;
        for (QTreeWidgetItemIterator it(paramValue); *it; ++it) {
            auto item = static_cast<ParameterValueItem*>(*it);
            item->setCheckState(0, 
                    keys.empty()||keys.count(item->getKey()) ? Qt::Checked : Qt::Unchecked);
        }
    } else {
        QSignalBlocker block(paramValue);
        for (QTreeWidgetItemIterator it(paramValue); *it; ++it)
            (*it)->setCheckState(0, Qt::Unchecked);
    }
}

void DlgParameterImp::onButtonRestartClicked()
{
    QTimer::singleShot(100, [](){Application::Instance->restart();});
}

void DlgParameterImp::onButtonResetClicked()
{
    QString msg;
    if (curParamManager == &App::GetApplication().GetUserParameter())
        msg = tr("The application will restart after resetting the configuration.\n\n"
                 "Do you want to make a backup?");
    else
        msg = tr("Do you want to make a backup before resetting the configuration?\n");
    auto res = QMessageBox::question(this, tr("Reset"), msg,
                QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
    if (res == QMessageBox::Cancel)
        return;

    if (res == QMessageBox::Yes) {
        if (!doExport(curParamManager))
            return;
    }

    if (curParamManager == &App::GetApplication().GetUserParameter()) {
        QTimer::singleShot(100, [](){Application::Instance->restart(true);});
    } else {
        try {
            curParamManager->Clear(true);
            monitors.erase(curParamManager);
            onChangeParameterSet(ui->parameterSet->currentIndex());
        } catch (Base::Exception &e) {
            e.ReportException();
            QMessageBox::critical(this, tr("Reset"), tr("Failed to reset configuration."));
            return;
        }
    }
}

void DlgParameterImp::onButtonImportClicked()
{
    if (QMessageBox::warning(this,
                tr("Import parameter from file"),
                tr("This will overwrite the current settings.\nDo you want to continue?"),
                QMessageBox::Yes|QMessageBox::No) != QMessageBox::Yes)
        return;
    doImportOrMerge(curParamManager, false);
}

void DlgParameterImp::onButtonExportClicked()
{
    auto h = copyParameters(curParamManager);
    if (!h)
        h = curParamManager;
    doExport(h);
}

ParameterGrp::handle DlgParameterImp::copyParameters(ParameterManager *manager)
{
    auto it = monitors.find(manager);
    if (it == monitors.end())
        return nullptr;
    auto hTmp = ParameterManager::Create();
    ParameterGrp::handle res(hTmp.get());
    hTmp->CreateDocument();
    for (auto &v : it->second.changes) {
        auto hSrc = v.first;
        if (hSrc->Manager() != manager)
            continue;
        auto hGrp = hTmp->GetGroup(hSrc->GetPath().c_str());

        auto setValue = [hSrc, hGrp](ParameterGrp::ParamType type, const char *name) {
            if (type == ParameterGrp::ParamType::FCBool) {
                auto value = hSrc->GetBool(name, false);
                if (value == hSrc->GetBool(name, true)) // to make sure the key exist
                    hGrp->SetBool(name, value);
            }
            else if (type == ParameterGrp::ParamType::FCInt) {
                auto value = hSrc->GetInt(name, 0);
                if (value == hSrc->GetInt(name, 1)) // to make sure the key exist
                    hGrp->SetInt(name, value);
            }
            else if (type == ParameterGrp::ParamType::FCUInt) {
                auto value = hSrc->GetUnsigned(name, 0);
                if (value == hSrc->GetUnsigned(name, 1)) // to make sure the key exist
                    hGrp->SetUnsigned(name, value);
            }
            else if (type == ParameterGrp::ParamType::FCFloat) {
                auto value = hSrc->GetFloat(name, 0.f);
                if (value == hSrc->GetFloat(name, 1.f)) // to make sure the key exist
                    hGrp->SetFloat(name, value);
            }
            else if (type == ParameterGrp::ParamType::FCText) {
                auto value = hSrc->GetASCII(name, "");
                if (value == hSrc->GetASCII(name, "0")) // to make sure the key exist
                    hGrp->SetASCII(name, value);
            }
        };

        if (v.second.empty()) {
            hSrc->copyTo(hGrp);
        } else {
            for (auto &key : v.second)
                setValue(key.type, key.name.constData());
        }
    }
    return res;
}

void DlgParameterImp::onButtonAddClicked()
{
    QString file = FileDialog::getOpenFileName( this,
            tr("Add Configuration"), QString(),
            QStringLiteral("XML (*.FCParam);;%1 (*.*)").arg(tr("All files")));
    if (file.isEmpty())
        return;
    std::string name = QFileInfo(file).completeBaseName().toUtf8().constData();
    App::GetApplication().AddParameterSet(name, file.toUtf8().constData());
    ui->parameterSet->addItem(QString::fromUtf8(name.c_str()), QByteArray(name.c_str()));
    ui->parameterSet->setCurrentIndex(ui->parameterSet->count()-1);
}

void DlgParameterImp::onButtonRenameClicked()
{
    if (!ui->parameterSet->isEditable()) {
        ui->parameterSet->setEditable(true);
        ui->parameterSet->setFocus();
        ui->parameterSet->lineEdit()->selectAll();
        connect(ui->parameterSet->lineEdit(), &QLineEdit::editingFinished,
                this, &DlgParameterImp::onParameterSetNameChanged);
    }
}

void DlgParameterImp::onButtonRefreshClicked()
{
    saveState();
    QSignalBlocker guard(paramGroup);
    QSignalBlocker guard2(paramValue);
    App::GetApplication().RefreshParameterSet();
    populate();
}

void DlgParameterImp::populate()
{
    QSignalBlocker guard(paramGroup);
    QSignalBlocker guard2(paramValue);
    QSignalBlocker guard3(ui->parameterSet);

    monitors.clear();
    paramValue->clear();
    paramGroup->clear();
    ui->parameterSet->clear();

    int pos = -1;
    const auto& rcList = App::GetApplication().GetParameterSetList();
    for (auto it= rcList.begin();it!=rcList.end();++it) {
        // for now ignore system parameters because they are nowhere used
        if (it->second == &App::GetApplication().GetSystemParameter())
            continue;
        if (it->second == &App::GetApplication().GetUserParameter()) {
            ui->parameterSet->insertItem(0, tr("<Application Settings>"),
                        QVariant(QByteArray(it->first.c_str())));
            continue;
        }
        QString name = QString::fromUtf8(
                it->second->GetASCII("Name", it->first.c_str()).c_str());
        QString tooltip = QString::fromUtf8(it->second->GetASCII("ToolTip").c_str());
        QString t = QString::fromUtf8(it->second->GetSerializeFileName().c_str());
        if (t.size()) {
            if (tooltip.size())
                tooltip += QStringLiteral("\n\n");
            tooltip += t;
        }
        
        int idx = ui->parameterSet->count();
        if (!isReadOnly(it->second)) {
            if (pos < 0)
                pos = idx;
        } else if (pos >= 0)
            idx = pos++;
            
        ui->parameterSet->insertItem(idx, name, QVariant(QByteArray(it->first.c_str())));
        ui->parameterSet->setItemData(idx, tooltip, Qt::ToolTipRole);
    }
    ParameterGrp::handle hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp/Preferences/ParameterEditor");
    int index = ui->parameterSet->findData(_LastParameterSet);
    if (index < 0) {
        _LastParameterSet = "User parameter";
        index = ui->parameterSet->findData(_LastParameterSet);
        if (index < 0)
            index = 0;
    }
    ui->parameterSet->setCurrentIndex(index);
    onChangeParameterSet(index);
    if (!hasDefaultColor && paramGroup->topLevelItemCount()) {
        hasDefaultColor = true;
        defaultColor = paramGroup->topLevelItem(0)->foreground(0);
    }
}

void DlgParameterImp::onButtonRemoveClicked()
{
    int index = ui->parameterSet->currentIndex();
    if (index < 0)
        return;
    auto hManager = App::GetApplication().GetParameterSet(
            ui->parameterSet->itemData(index).toByteArray());
    if (hManager == &App::GetApplication().GetUserParameter()
            || hManager == &App::GetApplication().GetSystemParameter())
        return;
    removeState();
    App::GetApplication().RemoveParameterSet(
            ui->parameterSet->itemData(index).toByteArray());
    QSignalBlocker block(ui->parameterSet);
    ui->parameterSet->removeItem(index);
    if (index == ui->parameterSet->count())
        --index;
    ui->parameterSet->setCurrentIndex(index);
    onChangeParameterSet(index);
}

void DlgParameterImp::onButtonToolTipClicked()
{
    int index = ui->parameterSet->currentIndex();
    if (index < 0)
        return;
    auto hManager = App::GetApplication().GetParameterSet(
            ui->parameterSet->itemData(index).toByteArray());
    if (hManager == &App::GetApplication().GetUserParameter()
            || hManager == &App::GetApplication().GetSystemParameter())
        return;

    bool ok = false;
    QString tooltip = QInputDialog::getMultiLineText(
            this, ui->parameterSet->currentText(), tr("Tool tip:"),
            QString::fromUtf8(hManager->GetASCII("ToolTip").c_str()), &ok);
    if (ok) {
        hManager->SetASCII("ToolTip", tooltip.toUtf8().constData());
        QString t = QString::fromUtf8(hManager->GetSerializeFileName().c_str());
        if (t.size())
            tooltip += QStringLiteral("\n\n");
        tooltip += t;
        ui->parameterSet->setItemData(index, tooltip, Qt::ToolTipRole);
    }
}

void DlgParameterImp::onCheckBoxPresetToggled(bool checked)
{
    int index = ui->parameterSet->currentIndex();
    if (index < 0)
        return;
    curParamManager->SetBool("Preset", checked);
}

void DlgParameterImp::onParameterSetNameChanged()
{
    int index = ui->parameterSet->currentIndex();
    ParameterManager* parmgr = App::GetApplication().GetParameterSet(
            ui->parameterSet->itemData(index).toByteArray());
    QString text = ui->parameterSet->currentText();
    ui->parameterSet->setEditable(false);
    parmgr->SetASCII("Name", text.toUtf8().constData());
    ui->parameterSet->setItemText(index, text);
}

static inline bool hasCheckedItem(QTreeWidget *widget)
{
    QTreeWidgetItemIterator it(widget, QTreeWidgetItemIterator::Checked);
    return (*it) != nullptr;
}

static inline bool hasUncheckedItem(QTreeWidget *widget)
{
    QTreeWidgetItemIterator it(widget, QTreeWidgetItemIterator::NotChecked);
    return (*it) != nullptr;
}

void DlgParameterImp::setGroupItemState(ParameterGroupItem *gitem, Qt::CheckState state) {
    if (gitem->checkState(0) != state) {
        QSignalBlocker block(paramGroup);
        gitem->setCheckState(0, state);
        updateGroupItemCheckState(gitem);
    }
}

void DlgParameterImp::setValueItemState(ParameterValueItem *vitem, Qt::CheckState state) {
    if (vitem->checkState(0) != state) {
        QSignalBlocker block(paramValue);
        vitem->setCheckState(0, state);
        onValueItemChanged(vitem, 0);
    }
}

bool DlgParameterImp::checkGroupItemState(ParameterGroupItem *gitem, Qt::CheckState state) {
    for (int i=0, c=gitem->childCount(); i<c; ++i) {
        if (gitem->child(i)->checkState(0) != state)
            return false;
    }
    auto names = gitem->_hcGrp->GetParameterNames();
    if (names.empty())
        return true;
    auto &changes = monitors[curParamManager].changes;
    auto it = changes.find(gitem->_hcGrp);
    if (state == Qt::Checked) {
        if (it == changes.end())
            return false;
        auto &keys = it->second;
        if (keys.size() == names.size())
            keys.clear();
        else if (keys.size())
            return false;
    } else if (state == Qt::Unchecked) {
        if (it != changes.end())
            return false;
    }
    return true;
}

static inline bool hasItemState(QTreeWidgetItem *item, Qt::CheckState state) {
    for (int i=0, c=item->childCount(); i<c; ++i) {
        if (item->child(i)->checkState(0) == state)
            return true;
    }
    return false;
}

void DlgParameterImp::onValueItemChanged(QTreeWidgetItem *item, int col)
{
    if (col!=0 || !ui->checkBoxMonitor->isChecked())
        return;

    auto state = item->checkState(0);
    auto gitem = paramGroup->findItem(paramValue->currentGroup());
    if (!gitem)
        return;

    auto vitem = static_cast<ParameterValueItem*>(item);
    QSignalBlocker block(paramValue);
    QSignalBlocker block2(paramGroup);

    auto &changes = monitors[curParamManager].changes;
    auto &keys = changes[paramValue->currentGroup()];
    if (state == Qt::Unchecked) {
        if (keys.empty()) {
            for (QTreeWidgetItemIterator it(paramValue, QTreeWidgetItemIterator::Checked); *it; ++it) {
                auto vi = static_cast<ParameterValueItem*>(*it);
                keys.insert(vi->getKey());
            }
            if (keys.empty())
                changes.erase(paramValue->currentGroup());
        } else {
            keys.erase(vitem->getKey());
            if (keys.empty())
                changes.erase(paramValue->currentGroup());
        }
        if (hasCheckedItem(paramValue) || hasItemState(gitem, Qt::Checked))
            return;
        gitem->setCheckState(0, Qt::Unchecked);
    } else if (hasUncheckedItem(paramValue)) {
        keys.insert(vitem->getKey());
        if (gitem->checkState(0) == Qt::PartiallyChecked)
            return;
        gitem->setCheckState(0, Qt::PartiallyChecked);
    } else {
        keys.clear();
        if (!hasItemState(gitem, Qt::Unchecked))
            gitem->setCheckState(0, Qt::Checked);
        else
            return;
    }
    updateGroupItemCheckState(gitem);
}

void DlgParameterImp::onGroupItemChanged(QTreeWidgetItem *item, int col)
{
    if (col!=0 || !ui->checkBoxMonitor->isChecked())
        return;

    auto state = item->checkState(0);
    if (state == Qt::PartiallyChecked)
        return;
    auto gitem = static_cast<ParameterGroupItem*>(item);
    QSignalBlocker block(paramValue);
    if (paramValue->currentGroup() == gitem->_hcGrp) {
        for (QTreeWidgetItemIterator it(paramValue); *it; ++it)
            (*it)->setCheckState(0, state);
    }
    QSignalBlocker block2(paramGroup);
    auto &changes = monitors[curParamManager].changes;
    if (state == Qt::Checked)
        changes[gitem->_hcGrp].clear();
    else
        changes.erase(gitem->_hcGrp);

    static std::deque<QTreeWidgetItem*> stack;
    stack.clear();
    stack.push_back(item);
    while(stack.size()) {
        auto curItem = stack.front();
        stack.pop_front();
        for (int i=0, c=curItem->childCount(); i<c; ++i) {
            auto child = curItem->child(i);
            stack.push_back(child);
            if (child->checkState(0) == state)
                continue;
            auto gitem = static_cast<ParameterGroupItem*>(child);
            gitem->setCheckState(0, state);
            if (state == Qt::Checked)
                changes[gitem->_hcGrp].clear();
            else
                changes.erase(gitem->_hcGrp);
            if (paramValue->currentGroup() == gitem->_hcGrp) {
                for (QTreeWidgetItemIterator it(paramValue); *it; ++it)
                    (*it)->setCheckState(0, state);
            }
        }
    }
    updateGroupItemCheckState(item);
}

void DlgParameterImp::updateGroupItemCheckState(QTreeWidgetItem *gitem)
{
    QSignalBlocker block(paramGroup);
    auto state = gitem->checkState(0);
    QTreeWidgetItem *item = nullptr;
    if (state == Qt::Checked) {
        for (item = gitem->parent(); item; item = item->parent()) {
            auto s = item->checkState(0);
            if (s == Qt::Checked)
                return;
            if (checkGroupItemState(static_cast<ParameterGroupItem*>(item), Qt::Checked))
                item->setCheckState(0, Qt::Checked);
            else
                break;
        }
    } else if (state == Qt::Unchecked) {
        for (item = gitem->parent(); item; item = item->parent()) {
            if (item->checkState(0) == Qt::Unchecked)
                return;
            if (checkGroupItemState(static_cast<ParameterGroupItem*>(item), Qt::Unchecked))
                item->setCheckState(0, Qt::Unchecked);
            else
                break;
        }
    } else
        item = gitem->parent();

    for (; item; item = item->parent()) {
        if (item->checkState(0) == Qt::PartiallyChecked)
            break;
        item->setCheckState(0, Qt::PartiallyChecked);
    }
}

void DlgParameterImp::clearGroupItem(ParameterGroupItem *item,
        std::unordered_map<ParameterGrp*, std::set<ParamKey>> &changes)
{
    changes.erase(item->_hcGrp);
    for (int i=0, c=item->childCount(); i<c; ++i) {
        auto child = static_cast<ParameterGroupItem*>(item->child(i));
        if (child->_hcGrp == paramValue->currentGroup()) {
            QSignalBlocker block(paramValue);
            paramValue->clear();
        }
        clearGroupItem(child, changes);
    }

}

void DlgParameterImp::slotParamChanged(ParameterGrp *Param,
                                       ParameterGrp::ParamType Type,
                                       const char *Name, 
                                       const char *Value)
{
    if (!Param || importing)
        return;
    if (Name && !Name[0])
        return;

    auto it = monitors.find(Param->Manager());
    if (it == monitors.end())
        return;

    auto &changes = it->second.changes;

    QSignalBlocker block(paramGroup);
    QSignalBlocker block2(paramValue);

    if (Type == ParameterGrp::ParamType::FCGroup) {
        if (!Name) {
            // This means the group itself is deleted. But currently we can
            // only sync for value changes. So just erase any current changes
            // under this group for now.
            auto item = paramGroup->findItem(Param, false);
            if (item) {
                auto curItem = paramGroup->currentItem();
                clearGroupItem(item, changes); 
                delete item;
                if (paramGroup->currentItem() != curItem)
                    onGroupSelected(paramGroup->currentItem());
            }
        } else if (Name == Value) {
            // This means new group
            auto item = paramGroup->findItem(Param, true);
            if (item)
                paramGroup->scrollToItem(item);
        } else if (Value) {
            // This means group rename. Value is the old name. Name is the new.
            auto item = paramGroup->findItem(Param, false);
            if (item) {
                item->setText(0,QString::fromUtf8(Name));
                // changes with empty keys means all parameters under the group
                // are considered as changed (i.e. will be exported)
                changes[Param].clear();
                paramGroup->scrollToItem(item);
            }
        }
        return;
    }

    auto iter = changes.find(Param);
    ParamKey key(Type, Name);
    if (Value) {
        auto groupItem = paramGroup->findItem(Param, true);
        // Means either value changed or new value
        if (iter == changes.end())
            changes[Param].insert(key);
        else if (iter->second.size())
            iter->second.insert(key);

        if (groupItem) {
            if (checkGroupItemState(groupItem, Qt::Checked))
                setGroupItemState(groupItem, Qt::Checked);
            else
                setGroupItemState(groupItem, Qt::PartiallyChecked);
            paramGroup->scrollToItem(groupItem);
        }
    } else {
        auto groupItem = paramGroup->findItem(Param, false);
        // Means parameter removed
        if (iter != changes.end() && iter->second.size()) {
            iter->second.erase(key);
            if (iter->second.size()) {
                if (groupItem && !checkGroupItemState(groupItem, Qt::Unchecked))
                    setGroupItemState(groupItem, Qt::PartiallyChecked);
            } else {
                changes.erase(iter);
                if (groupItem) {
                    if(checkGroupItemState(groupItem, Qt::Unchecked))
                        setGroupItemState(groupItem, Qt::Unchecked);
                    else
                        setGroupItemState(groupItem, Qt::PartiallyChecked);
                }
            }
        }
    }
    
    if (paramValue->currentGroup() != Param)
        return;

    auto item = paramValue->findItem(key);
    if (!Value)
        delete item;
    else if (item) {
        if (Type == ParameterGrp::ParamType::FCBool) {
            if (boost::equals(Value, "1"))
                Value = "true";
            else
                Value = "false";
        }
        item->setText(2, QString::fromUtf8(Value));
        setValueItemState(item, Qt::Checked);
        paramValue->scrollToItem(item);
    } else if (Name) {
        if (Type == ParameterGrp::ParamType::FCBool)
            item = new ParameterBool(paramValue,
                                     QString::fromUtf8(Name),
                                     Value[0] == '1',
                                     Param);
        else if (Type == ParameterGrp::ParamType::FCInt)
            item = new ParameterInt(paramValue,
                                    QString::fromUtf8(Name),
                                    atol(Value),
                                    Param);
        else if (Type == ParameterGrp::ParamType::FCUInt)
            item = new ParameterUInt(paramValue,
                                    QString::fromUtf8(Name),
                                    strtoul(Value, 0, 10),
                                    Param);
        else if (Type == ParameterGrp::ParamType::FCFloat)
            item = new ParameterUInt(paramValue,
                                    QString::fromUtf8(Name),
                                    atof(Value),
                                    Param);
        else if (Type == ParameterGrp::ParamType::FCText)
            item = new ParameterText(paramValue,
                                    QString::fromUtf8(Name),
                                    Value,
                                    Param);
        if (item) {
            setValueItemState(item, Qt::Checked);
            paramValue->scrollToItem(item);
        }
    }
}

void DlgParameterImp::onButtonSaveToDiskClicked()
{
    int index = ui->parameterSet->currentIndex();
    ParameterManager* parmgr = App::GetApplication().GetParameterSet(ui->parameterSet->itemData(index).toByteArray());
    if (!parmgr)
        return;

    parmgr->SaveDocument();
}

namespace Gui {
bool validateInput(QWidget* parent, const QString& input)
{
    if (input.isEmpty())
        return false;
    static boost::regex e("\\w[\\w.]*");
    if(!boost::regex_match(input.toUtf8().constData(),e)) {
        QMessageBox::warning(parent, DlgParameterImp::tr("Invalid input"),
                DlgParameterImp::tr("Invalid key name '%1'").arg(input));
        return false;
    }
    return true;
}
}

// --------------------------------------------------------------------

/* TRANSLATOR Gui::Dialog::ParameterGroup */

ParameterGroup::ParameterGroup( DlgParameterImp *owner, QWidget * parent )
  : QTreeWidget(parent), _owner(owner)
{
    menuEdit = new QMenu(this);
    expandAct = menuEdit->addAction(QString(), this, &ParameterGroup::onToggleSelectedItem);
    menuEdit->addSeparator();
    subGrpAct = menuEdit->addAction(QString(), this, &ParameterGroup::onCreateSubgroup);
    removeAct = menuEdit->addAction(QString(), this, &ParameterGroup::onDeleteSelectedItem);
    renameAct = menuEdit->addAction(QString(), this, &ParameterGroup::onRenameSelectedItem);
    menuEdit->addSeparator();
    exportAct = menuEdit->addAction(QString(), this, &ParameterGroup::onExportToFile);
    importAct = menuEdit->addAction(QString(), this, &ParameterGroup::onImportFromFile);
    mergeAct = menuEdit->addAction(QString(), this, &ParameterGroup::onMergeFromFile);
    menuEdit->setDefaultAction(expandAct);
    applyLanguage();
}

ParameterGroup::~ParameterGroup()
{
    clear();
}

void ParameterGroup::clear()
{
    QSignalBlocker blocker(this);
    itemMap.clear();
    QTreeWidget::clear();
}

ParameterGroupItem *ParameterGroup::findItem(ParameterGrp *hGrp, bool create)
{
    auto it = itemMap.find(hGrp);
    if (it != itemMap.end())
        return it->second;
    if (!create || !hGrp || !_owner || hGrp->Manager() != _owner->curParamManager)
        return nullptr;

    if (hGrp->Parent() == _owner->curParamManager)
        return new ParameterGroupItem(this, hGrp);

    auto parent = findItem(hGrp->Parent(), true);
    if (parent) {
        parent->setExpanded(true);
        // ParameterGroupItem will populate its children groups in its
        // constructor, so we need to search the item again.
        it = itemMap.find(hGrp);
        if (it != itemMap.end())
            return it->second;
        return new ParameterGroupItem(parent, hGrp);
    }
    return nullptr;
}

void ParameterGroup::contextMenuEvent ( QContextMenuEvent* event )
{
    QTreeWidgetItem* item = currentItem();
    if (item && item->isSelected())
    {
        expandAct->setEnabled(item->childCount() > 0);

        if (!_owner->isReadOnly()) {
            // do not allow to import parameters from a non-empty parameter group
            importAct->setEnabled(item->childCount() == 0);
        }

        if (item->isExpanded())
            expandAct->setText( tr("Collapse") );
        else
        expandAct->setText( tr("Expand") );
        menuEdit->popup(event->globalPos());
    }
}

void ParameterGroup::keyPressEvent (QKeyEvent* event)
{
    switch ( tolower(event->key()) )
    {
    case Qt::Key_Delete:
        {
            onDeleteSelectedItem();
        }   break;
    default:
            QTreeWidget::keyPressEvent(event);
  }
}

void ParameterGroup::onDeleteSelectedItem()
{
    if (_owner->isReadOnly())
        return;
    QTreeWidgetItem* sel = currentItem();
    if (sel && sel->isSelected())
    {
        if ( QMessageBox::question(this, tr("Remove group"), tr("Do you really want to remove this parameter group?"),
                               QMessageBox::Yes | QMessageBox::No, QMessageBox::No) ==
                               QMessageBox::Yes )
        {
            auto item = static_cast<ParameterGroupItem*>(sel);
            ParameterGrp::handle hParent;
            if (!sel->parent())
                hParent = _owner->curParamManager;
            else {
                auto parent = static_cast<ParameterGroupItem*>(sel->parent());
                hParent = parent->_hcGrp;
            }
            auto hGrp = item->_hcGrp;
            delete item;
            hParent->RemoveGrp(hGrp->GetGroupName());
        }
    }
}

void ParameterGroup::onToggleSelectedItem()
{
    QTreeWidgetItem* sel = currentItem();
    if (sel && sel->isSelected())
    {
        if (sel->isExpanded())
            sel->setExpanded(false);
        else if (sel->childCount() > 0)
            sel->setExpanded(true);
    }
}

void ParameterGroup::onCreateSubgroup()
{
    bool ok;
    QString name = QInputDialog::getText(this, QObject::tr("New sub-group"), QObject::tr("Enter the name:"),
                                         QLineEdit::Normal, QString(), &ok, Qt::MSWindowsFixedSizeDialogHint);

    if (ok && Gui::validateInput(this, name))
    {
        QTreeWidgetItem* item = currentItem();
        if (item && item->isSelected())
        {
            auto para = static_cast<ParameterGroupItem*>(item);
            Base::Reference<ParameterGrp> hGrp = para->_hcGrp;

            if ( hGrp->HasGroup( name.toUtf8() ) )
            {
                QMessageBox::critical( this, tr("Existing sub-group"),
                    tr("The sub-group '%1' already exists.").arg( name ) );
                return;
            }

            hGrp = hGrp->GetGroup( name.toUtf8() );
            (void)new ParameterGroupItem(para,hGrp);
            expandItem(para);
        }
    }
}

void ParameterGroup::onExportToFile()
{
    QTreeWidgetItem* item = currentItem();
    if (_owner && item && item->isSelected())
        _owner->doExport(static_cast<ParameterGroupItem*>(item)->_hcGrp);
}

bool DlgParameterImp::doExport(ParameterGrp *hGrp)
{
    QString file = FileDialog::getSaveFileName( this, tr("Export parameter to file"),
        QString(), QStringLiteral("XML (*.FCParam)"));
    if ( !file.isEmpty() ) {
        try {
            hGrp->exportTo( file.toUtf8() );
            return true;
        } catch (Base::Exception &e) {
            e.ReportException();
            QMessageBox::critical(this, tr("Export Failed"),
                    tr("Failed to export to '%1'.").arg( file ));
        }
        
    }
    return false;
}

void ParameterGroup::onImportFromFile()
{
    QTreeWidgetItem* item = currentItem();
    if (_owner && item && item->isSelected())
        _owner->doImportOrMerge(static_cast<ParameterGroupItem*>(item)->_hcGrp, false);
}

void DlgParameterImp::doImportOrMerge(ParameterGrp *hGrp, bool merge)
{
    QString file = FileDialog::getOpenFileName( this,
            merge ? tr("Merge parameter from file") : tr("Import parameter from file"),
            QString(), QStringLiteral("XML (*.FCParam);;%1 (*.*)").arg(tr("All files")));
    if ( file.isEmpty() )
        return;

    QSignalBlocker block(paramValue);
    QSignalBlocker block2(paramGroup);

    // save name and tooltip of the current configuration
    std::string name = curParamManager->GetASCII("Name");
    std::string tooltip = curParamManager->GetASCII("ToolTip");

    auto iter = monitors.find(curParamManager);
    if (paramValue->currentGroup() == hGrp)
        paramValue->clear();

    auto item = paramGroup->findItem(hGrp);
    if (item) {
        if (iter != monitors.end())
            clearGroupItem(item, iter->second.changes);

        // remove the items and internal parameter values
        QList<QTreeWidgetItem*> childs = item->takeChildren();
        for (QList<QTreeWidgetItem*>::iterator it = childs.begin(); it != childs.end(); ++it)
        {
            if (iter != monitors.end())
                iter->second.changes.erase(static_cast<ParameterGroupItem*>(*it)->_hcGrp);
            delete *it;
        }
    } else if (hGrp == curParamManager) {
        paramGroup->clear();
        paramValue->clear();
        if (iter != monitors.end())
            iter->second.changes.clear();
    }

    if (hGrp == &App::GetApplication().GetUserParameter()) {
        if (auto action = PresetsAction::instance())
            action->push(merge ? tr("Merge") : tr("Import"));
    }

    try
    {
        Base::StateLocker guard(importing);
        if (merge)
            hGrp->insert( file.toUtf8() );
        else
            hGrp->importFrom( file.toUtf8() );
    }
    catch( const Base::Exception& e)
    {
        e.ReportException();
        QMessageBox::critical(this, tr("Import Failed"),tr("Reading from '%1' failed.").arg( file ));
    }

    // restore name and tooltip
    if (name.empty())
        curParamManager->RemoveASCII("Name");
    else
        curParamManager->SetASCII("Name", name);
    if (tooltip.empty())
        curParamManager->RemoveASCII("ToolTip");
    else
        curParamManager->SetASCII("ToolTip", tooltip);

    if (item) {
        for (auto &grp : hGrp->GetGroups())
            new ParameterGroupItem(item, grp);
        if (item)
            item->setExpanded(true);
        if (item && paramGroup->currentItem() == item)
            onGroupSelected(item);
    } else
        onChangeParameterSet(ui->parameterSet->currentIndex());
}

void ParameterGroup::onMergeFromFile()
{
    QTreeWidgetItem* item = currentItem();
    if (_owner && item && item->isSelected())
        _owner->doImportOrMerge(static_cast<ParameterGroupItem*>(item)->_hcGrp, true);
}

void ParameterGroup::onRenameSelectedItem()
{
    QTreeWidgetItem* sel = currentItem();
    if (sel && sel->isSelected())
    {
        editItem(sel, 0);
    }
}

void ParameterGroup::applyLanguage()
{
    expandAct->setText(tr("Expand"));
    subGrpAct->setText(tr("Add sub-group"));
    removeAct->setText(tr("Remove group"));
    renameAct->setText(tr("Rename group"));
    exportAct->setText(tr("Export parameter"));
    mergeAct->setText(tr("Merge parameter"));
    importAct->setText(tr("Import parameter"));
    exportAct->setToolTip(tr("Export the current parameter group"));
    importAct->setToolTip(tr("Import saved parameter into the current group.\n"
                            "Only an empty group is allowed to accept import."));
    mergeAct->setToolTip(tr("Merge the saved parameter into the current group"));
}

void ParameterGroup::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        applyLanguage();
    } else {
        QTreeWidget::changeEvent(e);
    }
}

// --------------------------------------------------------------------

/* TRANSLATOR Gui::Dialog::ParameterValue */

ParameterValue::ParameterValue( DlgParameterImp *owner, QWidget * parent )
  : QTreeWidget(parent), _owner(owner)
{
    menuEdit = new QMenu(this);
    changeAct = menuEdit->addAction(tr("Change value"), this, qOverload<>(&ParameterValue::onChangeSelectedItem));
    menuEdit->addSeparator();
    removeAct = menuEdit->addAction(tr("Remove key"), this, &ParameterValue::onDeleteSelectedItem);
    renameAct = menuEdit->addAction(tr("Rename key"), this, &ParameterValue::onRenameSelectedItem);
    touchAct = menuEdit->addAction(tr("Touch item"), this, &ParameterValue::onTouchItem);
    menuEdit->setDefaultAction(changeAct);

    menuEdit->addSeparator();
    menuNew = menuEdit->addMenu(tr("New"));
    newStrAct = menuNew->addAction(tr("New string item"), this, &ParameterValue::onCreateTextItem);
    newFltAct = menuNew->addAction(tr("New float item"), this, &ParameterValue::onCreateFloatItem);
    newIntAct = menuNew->addAction(tr("New integer item"), this, &ParameterValue::onCreateIntItem);
    newUlgAct = menuNew->addAction(tr("New unsigned item"), this, &ParameterValue::onCreateUIntItem);
    newBlnAct = menuNew->addAction(tr("New Boolean item"), this, &ParameterValue::onCreateBoolItem);

    connect(this, &ParameterValue::itemDoubleClicked,
            this, qOverload<QTreeWidgetItem*, int>(&ParameterValue::onChangeSelectedItem));
}

ParameterValue::~ParameterValue()
{
    clear();
}

void ParameterValue::clear()
{
    itemMap.clear();
    QTreeWidget::clear();
    _hcGrp = nullptr;
}

ParameterValueItem *ParameterValue::findItem(const ParamKey &key) const
{
    auto it = itemMap.find(key);
    if (it != itemMap.end())
        return it->second;
    return nullptr;
}

void ParameterValue::setCurrentGroup( const Base::Reference<ParameterGrp>& hGrp )
{
    _hcGrp = hGrp;
}

Base::Reference<ParameterGrp> ParameterValue::currentGroup() const
{
    return _hcGrp;
}

bool ParameterValue::edit ( const QModelIndex & index, EditTrigger trigger, QEvent * event )
{
    if (index.column() > 0)
        return false;
    return QTreeWidget::edit(index, trigger, event);
}

void ParameterValue::contextMenuEvent ( QContextMenuEvent* event )
{
    if (_owner->isReadOnly())
        return;
    QTreeWidgetItem* item = currentItem();
    if (item && item->isSelected()) {
        menuEdit->popup(event->globalPos());
    }
    else {
        // There is a regression in Qt 5.12.9 where it isn't checked that a sub-menu (here menuNew)
        // can be popped up without its parent menu (menuEdit) and thus causes a crash.
        // A workaround is to simply call exec() instead.
        //
        //menuNew->popup(event->globalPos());
        menuNew->exec(event->globalPos());
    }
}

void ParameterValue::keyPressEvent (QKeyEvent* event)
{
    switch ( tolower(event->key()) )
    {
    case Qt::Key_Delete:
        {
            onDeleteSelectedItem();
        }   break;
    default:
            QTreeWidget::keyPressEvent(event);
  }
}

void ParameterValue::resizeEvent(QResizeEvent* event)
{
    QHeaderView* hv = header();
    hv->setSectionResizeMode(QHeaderView::Stretch);

    QTreeWidget::resizeEvent(event);

    hv->setSectionResizeMode(QHeaderView::Interactive);
}

void ParameterValue::onChangeSelectedItem(QTreeWidgetItem* item, int col)
{
    if (_owner->isReadOnly()) {
        QMessageBox::warning(_owner, tr("Read only settings"),
                tr("The current configuration settings is read only. "
                   "You can copy it and then make changes."));
        return;
    }
    if (item && item->isSelected() && col > 0)
    {
        static_cast<ParameterValueItem*>(item)->changeValue();
    }
}

void ParameterValue::onChangeSelectedItem()
{
    onChangeSelectedItem(currentItem(), 1);
}

void ParameterValue::onDeleteSelectedItem()
{
    bool monitor = _owner->monitors.count(_owner->curParamManager) != 0;
    for (auto item : selectedItems()) {
        static_cast<ParameterValueItem*>(item)->removeFromGroup();
        if (!monitor)
            delete item;
    }
}

void ParameterValue::onRenameSelectedItem()
{
    QTreeWidgetItem* sel = currentItem();
    if (sel && sel->isSelected())
    {
        editItem(sel, 0);
    }
}

void ParameterValue::onTouchItem()
{
    if (auto manager = _hcGrp->Manager()) {
        for(auto item : selectedItems()) {
            auto vitem = static_cast<ParameterValueItem*>(item);
            manager->signalParamChanged(_hcGrp, vitem->getKey().type,
                    vitem->getKey().name, vitem->text(2).toUtf8());
        }
    }
}

void ParameterValue::onCreateTextItem()
{
    bool ok;
    QString name = QInputDialog::getText(this, QObject::tr("New text item"), QObject::tr("Enter the name:"),
                                         QLineEdit::Normal, QString(), &ok, Qt::MSWindowsFixedSizeDialogHint);

    if (!ok || !Gui::validateInput(this, name))
        return;

    std::vector<std::pair<std::string,std::string> > smap = _hcGrp->GetASCIIMap();
    for (const auto & it : smap) {
        if (name == QString::fromUtf8(it.first.c_str()))
        {
            QMessageBox::critical( this, tr("Existing item"),
                tr("The item '%1' already exists.").arg( name ) );
            return;
        }
    }

    QString val = QInputDialog::getText(this, QObject::tr("New text item"), QObject::tr("Enter your text:"),
                                        QLineEdit::Normal, QString(), &ok, Qt::MSWindowsFixedSizeDialogHint);
    if ( ok && !val.isEmpty() )
    {
        ParameterValueItem *pcItem;
        pcItem = new ParameterText(this, name, val.toUtf8(), _hcGrp);
        pcItem->appendToGroup();
    }
}

void ParameterValue::onCreateIntItem()
{
    bool ok;
    QString name = QInputDialog::getText(this, QObject::tr("New integer item"), QObject::tr("Enter the name:"),
                                         QLineEdit::Normal, QString(), &ok, Qt::MSWindowsFixedSizeDialogHint);

    if (!ok || !Gui::validateInput(this, name))
        return;

    std::vector<std::pair<std::string,long> > lmap = _hcGrp->GetIntMap();
    for (const auto & it : lmap) {
        if (name == QString::fromUtf8(it.first.c_str()))
        {
            QMessageBox::critical( this, tr("Existing item"),
                tr("The item '%1' already exists.").arg( name ) );
            return;
        }
    }

    int val = QInputDialog::getInt(this, QObject::tr("New integer item"), QObject::tr("Enter your number:"),
                                   0, -2147483647, 2147483647, 1, &ok, Qt::MSWindowsFixedSizeDialogHint);

    if ( ok )
    {
        ParameterValueItem *pcItem;
        pcItem = new ParameterInt(this,name,(long)val, _hcGrp);
        pcItem->appendToGroup();
    }
}

void ParameterValue::onCreateUIntItem()
{
    bool ok;
    QString name = QInputDialog::getText(this, QObject::tr("New unsigned item"), QObject::tr("Enter the name:"),
                                         QLineEdit::Normal, QString(), &ok, Qt::MSWindowsFixedSizeDialogHint);

    if (!ok || !Gui::validateInput(this, name))
        return;

    std::vector<std::pair<std::string,unsigned long> > lmap = _hcGrp->GetUnsignedMap();
    for (const auto & it : lmap) {
        if (name == QString::fromUtf8(it.first.c_str()))
        {
            QMessageBox::critical( this, tr("Existing item"),
                tr("The item '%1' already exists.").arg( name ) );
            return;
        }
    }

    DlgInputDialogImp dlg(QObject::tr("Enter your number:"),this, true, DlgInputDialogImp::UIntBox);
    dlg.setWindowTitle(QObject::tr("New unsigned item"));
    UIntSpinBox* edit = dlg.getUIntBox();
    edit->setRange(0,UINT_MAX);
    if (dlg.exec() == QDialog::Accepted ) {
        QString value = edit->text();
        unsigned long val = value.toULong(&ok);

        if ( ok )
        {
            ParameterValueItem *pcItem;
            pcItem = new ParameterUInt(this,name, val, _hcGrp);
            pcItem->appendToGroup();
        }
    }
}

void ParameterValue::onCreateFloatItem()
{
    bool ok;
    QString name = QInputDialog::getText(this, QObject::tr("New float item"), QObject::tr("Enter the name:"),
                                         QLineEdit::Normal, QString(), &ok, Qt::MSWindowsFixedSizeDialogHint);

    if (!ok || !Gui::validateInput(this, name))
        return;

    std::vector<std::pair<std::string,double> > fmap = _hcGrp->GetFloatMap();
    for (const auto & it : fmap) {
        if (name == QString::fromUtf8(it.first.c_str()))
        {
            QMessageBox::critical( this, tr("Existing item"),
                tr("The item '%1' already exists.").arg( name ) );
            return;
        }
    }

    double val = QInputDialog::getDouble(this, QObject::tr("New float item"), QObject::tr("Enter your number:"),
                                         0, -2147483647, 2147483647, 12, &ok, Qt::MSWindowsFixedSizeDialogHint);
    if ( ok )
    {
        ParameterValueItem *pcItem;
        pcItem = new ParameterFloat(this,name,val, _hcGrp);
        pcItem->appendToGroup();
    }
}

void ParameterValue::onCreateBoolItem()
{
    bool ok;
    QString name = QInputDialog::getText(this, QObject::tr("New Boolean item"), QObject::tr("Enter the name:"),
                                         QLineEdit::Normal, QString(), &ok, Qt::MSWindowsFixedSizeDialogHint);

    if (!ok || !Gui::validateInput(this, name))
        return;

    std::vector<std::pair<std::string,bool> > bmap = _hcGrp->GetBoolMap();
    for (const auto & it : bmap) {
        if (name == QString::fromUtf8(it.first.c_str()))
        {
            QMessageBox::critical( this, tr("Existing item"),
                tr("The item '%1' already exists.").arg( name ) );
            return;
        }
    }

    QStringList list; list << QStringLiteral("true")
                           << QStringLiteral("false");
    QString val = QInputDialog::getItem (this, QObject::tr("New boolean item"), QObject::tr("Choose an item:"),
                                         list, 0, false, &ok, Qt::MSWindowsFixedSizeDialogHint);
    if ( ok )
    {
        ParameterValueItem *pcItem;
        pcItem = new ParameterBool(this,name,(val == list[0] ? true : false), _hcGrp);
        pcItem->appendToGroup();
    }
}

// ---------------------------------------------------------------------------

ParameterGroupItem::ParameterGroupItem( ParameterGroupItem * parent, const Base::Reference<ParameterGrp> &hcGrp )
    : QTreeWidgetItem( parent, QTreeWidgetItem::UserType+1 ), _hcGrp(hcGrp), _owner(parent->_owner)
{
    setFlags(flags() | Qt::ItemIsEditable);
    fillUp();
}

ParameterGroupItem::ParameterGroupItem( ParameterGroup* parent, const Base::Reference<ParameterGrp> &hcGrp)
    : QTreeWidgetItem( parent, QTreeWidgetItem::UserType+1 ), _hcGrp(hcGrp), _owner(parent)
{
    setFlags(flags() | Qt::ItemIsEditable);
    fillUp();
}

ParameterGroupItem::~ParameterGroupItem()
{
    if (_owner)
        _owner->itemMap.erase(_hcGrp);
}

void ParameterGroupItem::fillUp()
{
    if (_owner)
        _owner->itemMap[_hcGrp] = this;

    setText(0,QString::fromUtf8(_hcGrp->GetGroupName()));

    // filling up groups
    std::vector<Base::Reference<ParameterGrp> > vhcParamGrp = _hcGrp->GetGroups();
    for(const auto & It : vhcParamGrp)
        (void)new ParameterGroupItem(this,It);
}

void ParameterGroupItem::setData ( int column, int role, const QVariant & value )
{
    if (role == Qt::EditRole) {
        QString oldName = text(0);
        QString newName = value.toString();
        if (newName.isEmpty() || oldName == newName)
            return;

        if (!Gui::validateInput(treeWidget(), newName))
            return;

        // first check if there is already a group with name "newName"
        auto item = static_cast<ParameterGroupItem*>(parent());
        if ( !item )
        {
            QMessageBox::critical( treeWidget(), QObject::tr("Rename group"),
                QObject::tr("The group '%1' cannot be renamed.").arg( oldName ) );
            return;
        }
        if ( item->_hcGrp->HasGroup( newName.toUtf8() ) )
        {
            QMessageBox::critical( treeWidget(), QObject::tr("Existing group"),
                QObject::tr("The group '%1' already exists.").arg( newName ) );
            return;
        }
        else
        {
            // rename the group by adding a new group, copy the content and remove the old group
            if (!item->_hcGrp->RenameGrp(oldName.toUtf8(), newName.toUtf8()))
                return;
        }
    }

    QTreeWidgetItem::setData(column, role, value);
}

QVariant ParameterGroupItem::data ( int column, int role ) const
{
    if (role == Qt::DecorationRole) {
        // The root item should keep its special pixmap
        if (parent()) {
            return this->isExpanded() ?
                QApplication::style()->standardPixmap(QStyle::SP_DirOpenIcon):
                QApplication::style()->standardPixmap(QStyle::SP_DirClosedIcon);
        }
    }

    return QTreeWidgetItem::data(column, role);
}

// --------------------------------------------------------------------

ParameterValueItem::ParameterValueItem ( ParameterValue* parent,
                                         ParameterGrp::ParamType type,
                                         const QString &label,
                                         const Base::Reference<ParameterGrp> &hcGrp)
  : QTreeWidgetItem( parent ), _hcGrp(hcGrp), _key(type, label), _owner(parent)
{
    setFlags(flags() | Qt::ItemIsEditable);
    if (_owner)
        _owner->itemMap[_key] = this;
}

ParameterValueItem::~ParameterValueItem()
{
    if (_owner)
        _owner->itemMap.erase(_key);
}

void ParameterValueItem::setData ( int column, int role, const QVariant & value )
{
    if (role == Qt::EditRole) {
        QString oldName = text(0);
        QString newName = value.toString();
        if (newName.isEmpty() || oldName == newName)
            return;

        if (!Gui::validateInput(treeWidget(), newName))
            return;

        replace( oldName, newName );
    }

    QTreeWidgetItem::setData(column, role, value);
}

// --------------------------------------------------------------------

ParameterText::ParameterText ( ParameterValue * parent, QString label, const char* value, const Base::Reference<ParameterGrp> &hcGrp)
  :ParameterValueItem( parent, ParameterGrp::ParamType::FCText, label, hcGrp)
{
    setIcon(0, BitmapFactory().iconFromTheme("Param_Text") );
    setText(0, label);
    setText(1, QStringLiteral("Text"));
    setText(2, QString::fromUtf8(value));
}

ParameterText::~ParameterText()
{
}

void ParameterText::changeValue()
{
    bool ok;
    QString txt = QInputDialog::getText(treeWidget(), QObject::tr("Change value"), QObject::tr("Enter your text:"),
                                        QLineEdit::Normal, text(2), &ok, Qt::MSWindowsFixedSizeDialogHint);
    if ( ok )
    {
        setText( 2, txt );
        _hcGrp->SetASCII(text(0).toUtf8(), txt.toUtf8());
    }
}

void ParameterText::removeFromGroup ()
{
    _hcGrp->RemoveASCII(text(0).toUtf8());
}

void ParameterText::replace( const QString& oldName, const QString& newName )
{
    std::string val = _hcGrp->GetASCII(oldName.toUtf8());
    _hcGrp->RemoveASCII(oldName.toUtf8());
    _hcGrp->SetASCII(newName.toUtf8(), val.c_str());
}

void ParameterText::appendToGroup()
{
    _hcGrp->SetASCII(text(0).toUtf8(), text(2).toUtf8());
}

// --------------------------------------------------------------------

ParameterInt::ParameterInt ( ParameterValue * parent, QString label, long value, const Base::Reference<ParameterGrp> &hcGrp)
  :ParameterValueItem( parent, ParameterGrp::ParamType::FCInt, label, hcGrp)
{
    setIcon(0, BitmapFactory().iconFromTheme("Param_Int") );
    setText(0, label);
    setText(1, QStringLiteral("Integer"));
    setText(2, QStringLiteral("%1").arg(value));
}

ParameterInt::~ParameterInt()
{
}

void ParameterInt::changeValue()
{
    bool ok;
    int num = QInputDialog::getInt(treeWidget(), QObject::tr("Change value"), QObject::tr("Enter your number:"),
                                   text(2).toInt(), -2147483647, 2147483647, 1, &ok, Qt::MSWindowsFixedSizeDialogHint);
    if ( ok )
    {
        setText(2, QStringLiteral("%1").arg(num));
        _hcGrp->SetInt(text(0).toUtf8(), (long)num);
    }
}

void ParameterInt::removeFromGroup ()
{
    _hcGrp->RemoveInt(text(0).toUtf8());
}

void ParameterInt::replace( const QString& oldName, const QString& newName )
{
    long val = _hcGrp->GetInt(oldName.toUtf8());
    _hcGrp->RemoveInt(oldName.toUtf8());
    _hcGrp->SetInt(newName.toUtf8(), val);
}

void ParameterInt::appendToGroup()
{
    _hcGrp->SetInt(text(0).toUtf8(), text(2).toLong());
}

// --------------------------------------------------------------------

ParameterUInt::ParameterUInt ( ParameterValue * parent, QString label, unsigned long value, const Base::Reference<ParameterGrp> &hcGrp)
  :ParameterValueItem( parent, ParameterGrp::ParamType::FCUInt, label, hcGrp)
{
    setIcon(0, BitmapFactory().iconFromTheme("Param_UInt") );
    setText(0, label);
    setText(1, QStringLiteral("Unsigned"));
    setText(2, QStringLiteral("%1").arg(value));
}

ParameterUInt::~ParameterUInt()
{
}

void ParameterUInt::changeValue()
{
    bool ok;
    DlgInputDialogImp dlg(QObject::tr("Enter your number:"),treeWidget(), true, DlgInputDialogImp::UIntBox);
    dlg.setWindowTitle(QObject::tr("Change value"));
    UIntSpinBox* edit = dlg.getUIntBox();
    edit->setRange(0,UINT_MAX);
    edit->setValue(text(2).toULong());
    if (dlg.exec() == QDialog::Accepted)
    {
        QString value = edit->text();
        unsigned long num = value.toULong(&ok);

        if ( ok )
        {
            setText(2, QStringLiteral("%1").arg(num));
            _hcGrp->SetUnsigned(text(0).toUtf8(), (unsigned long)num);
        }
    }
}

void ParameterUInt::removeFromGroup ()
{
    _hcGrp->RemoveUnsigned(text(0).toUtf8());
}

void ParameterUInt::replace( const QString& oldName, const QString& newName )
{
    unsigned long val = _hcGrp->GetUnsigned(oldName.toUtf8());
    _hcGrp->RemoveUnsigned(oldName.toUtf8());
    _hcGrp->SetUnsigned(newName.toUtf8(), val);
}

void ParameterUInt::appendToGroup()
{
    _hcGrp->SetUnsigned(text(0).toUtf8(), text(2).toULong());
}

// --------------------------------------------------------------------

ParameterFloat::ParameterFloat ( ParameterValue * parent, QString label, double value, const Base::Reference<ParameterGrp> &hcGrp)
  :ParameterValueItem( parent, ParameterGrp::ParamType::FCFloat, label, hcGrp)
{
    setIcon(0, BitmapFactory().iconFromTheme("Param_Float") );
    setText(0, label);
    setText(1, QStringLiteral("Float"));
    setText(2, QStringLiteral("%1").arg(value));
}

ParameterFloat::~ParameterFloat()
{
}

void ParameterFloat::changeValue()
{
    bool ok;
    double num = QInputDialog::getDouble(treeWidget(), QObject::tr("Change value"), QObject::tr("Enter your number:"),
                                         text(2).toDouble(), -2147483647, 2147483647, 12, &ok, Qt::MSWindowsFixedSizeDialogHint);
    if ( ok )
    {
        setText(2, QStringLiteral("%1").arg(num));
        _hcGrp->SetFloat(text(0).toUtf8(), num);
    }
}

void ParameterFloat::removeFromGroup ()
{
    _hcGrp->RemoveFloat(text(0).toUtf8());
}

void ParameterFloat::replace( const QString& oldName, const QString& newName )
{
    double val = _hcGrp->GetFloat(oldName.toUtf8());
    _hcGrp->RemoveFloat(oldName.toUtf8());
    _hcGrp->SetFloat(newName.toUtf8(), val);
}

void ParameterFloat::appendToGroup()
{
    _hcGrp->SetFloat(text(0).toUtf8(), text(2).toDouble());
}

// --------------------------------------------------------------------

ParameterBool::ParameterBool ( ParameterValue * parent, QString label, bool value, const Base::Reference<ParameterGrp> &hcGrp)
  :ParameterValueItem( parent, ParameterGrp::ParamType::FCBool, label, hcGrp)
{
    setIcon(0, BitmapFactory().iconFromTheme("Param_Bool") );
    setText(0, label);
    setText(1, QStringLiteral("Boolean"));
    setText(2, value ? QStringLiteral("true") : QStringLiteral("false"));
}

ParameterBool::~ParameterBool()
{
}

void ParameterBool::changeValue()
{
    bool ok;
    QStringList list; list << QStringLiteral("true")
                           << QStringLiteral("false");
    int pos = (text(2) == list[0] ? 0 : 1);

    QString txt = QInputDialog::getItem (treeWidget(), QObject::tr("Change value"), QObject::tr("Choose an item:"),
                                         list, pos, false, &ok, Qt::MSWindowsFixedSizeDialogHint);
    if ( ok )
    {
        setText( 2, txt );
        _hcGrp->SetBool(text(0).toUtf8(), (txt == list[0] ? true : false) );
    }
}

void ParameterBool::removeFromGroup ()
{
    _hcGrp->RemoveBool(text(0).toUtf8());
}

void ParameterBool::replace( const QString& oldName, const QString& newName )
{
    bool val = _hcGrp->GetBool(oldName.toUtf8());
    _hcGrp->RemoveBool(oldName.toUtf8());
    _hcGrp->SetBool(newName.toUtf8(), val);
}

void ParameterBool::appendToGroup()
{
    bool val = (text(2) == QStringLiteral("true") ? true : false);
    _hcGrp->SetBool(text(0).toUtf8(), val);
}

#include "moc_DlgParameterImp.cpp"
