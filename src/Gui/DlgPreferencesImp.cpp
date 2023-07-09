// SPDX-License-Identifier: LGPL-2.1-or-later

 /****************************************************************************
  *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>               *
  *   Copyright (c) 2023 FreeCAD Project Association                         *
  *                                                                          *
  *   This file is part of FreeCAD.                                          *
  *                                                                          *
  *   FreeCAD is free software: you can redistribute it and/or modify it     *
  *   under the terms of the GNU Lesser General Public License as            *
  *   published by the Free Software Foundation, either version 2.1 of the   *
  *   License, or (at your option) any later version.                        *
  *                                                                          *
  *   FreeCAD is distributed in the hope that it will be useful, but         *
  *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
  *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
  *   Lesser General Public License for more details.                        *
  *                                                                          *
  *   You should have received a copy of the GNU Lesser General Public       *
  *   License along with FreeCAD. If not, see                                *
  *   <https://www.gnu.org/licenses/>.                                       *
  *                                                                          *
  ***************************************************************************/

#include "PreCompiled.h"
#ifndef _PreComp_
# include <algorithm>
# include <cstring>
# include <QAbstractButton>
# include <QApplication>
# include <QDebug>
# include <QGenericReturnArgument>
# include <QMessageBox>
# include <QScreen>
# include <QScrollArea>
# include <QScrollBar>
# include <QMoveEvent>
# include <QComboBox>
# include <QSpinBox>
# include <QTimer>
# include <QProcess>
#endif

#include <App/Application.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Tools.h>

#include "DlgPreferencesImp.h"
#include "ui_DlgPreferences.h"

#include "Action.h"
#include "BitmapFactory.h"
#include "MainWindow.h"
#include "PrefWidgets.h"
#include "PropertyPage.h"
#include "Tools.h"
#include "ViewParams.h"
#include "WidgetFactory.h"
#include "Widgets.h"

using namespace Gui::Dialog;

const int DlgPreferencesImp::GroupNameRole = Qt::UserRole;

/* TRANSLATOR Gui::Dialog::DlgPreferencesImp */

std::list<DlgPreferencesImp::TGroupPages> DlgPreferencesImp::_pages;
std::map<std::string, DlgPreferencesImp::Group> DlgPreferencesImp::_groupMap;

DlgPreferencesImp* DlgPreferencesImp::_activeDialog = nullptr;

/**
 *  Constructs a DlgPreferencesImp which is a child of 'parent', with
 *  widget flags set to 'fl'
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
DlgPreferencesImp::DlgPreferencesImp(QWidget* parent, Qt::WindowFlags fl)
    : QDialog(parent, fl), ui(new Ui_DlgPreferences),
      invalidParameter(false), restartRequired(false)
{
    ui->setupUi(this);

    widgetStates.reset(new Gui::PrefWidgetStates(this));

    // remove unused help button
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    connect(ui->buttonBox, &QDialogButtonBox::clicked,
            this, &DlgPreferencesImp::onButtonBoxClicked);
    connect(ui->buttonBox,  &QDialogButtonBox::helpRequested,
            getMainWindow(), &MainWindow::whatsThis);
    connect(ui->listBox, &QListWidget::currentItemChanged,
            this, &DlgPreferencesImp::changeGroup);

    setupPages();

    // Maintain a static pointer to the current active dialog (if there is one) so that
    // if the static page manipulation functions are called while the dialog is showing
    // it can update its content.
    DlgPreferencesImp::_activeDialog = this;

    auto manager = ParameterManager::Create();
    manager->CreateDocument();
    hBackup = manager.get();
    App::GetApplication().GetUserParameter().copyTo(hBackup);
    connParam = App::GetApplication().GetUserParameter().signalParamChanged.connect(
        [this](ParameterGrp*, ParameterGrp::ParamType, const char*, const char*) {
            this->paramTouched = true;
        });
}

/**
 *  Destroys the object and frees any allocated resources.
 */
DlgPreferencesImp::~DlgPreferencesImp()
{
    if (DlgPreferencesImp::_activeDialog == this) {
        DlgPreferencesImp::_activeDialog = nullptr;
    }
}

void DlgPreferencesImp::setupPages()
{
    // make sure that pages are ready to create
    GetWidgetFactorySupplier();
    for (const auto &group : _pages) {
        QTabWidget* groupTab = createTabForGroup(group.first);
        for (const auto &page : group.second) {
            createPageInGroup(groupTab, page);
        }
    }

    // show the first group
    ui->listBox->setCurrentRow(0);

    // Since preference pages often require scrolling up and down to access.
    // Using wheel focus in any input field may cause accidental change of
    // value while scrolling.
    for(auto child : findChildren<QWidget*>()) {
        if (child == ui->listBox)
            continue;
        if (qobject_cast<QScrollArea*>(child))
            continue;
        if (child->focusPolicy() == Qt::WheelFocus
                && !qobject_cast<QAbstractItemView*>(child))
            child->setFocusPolicy(Qt::StrongFocus);
        // It's not enough for some widget. We must use a eventFilter
        // to actively filter out wheel event if not in focus.
        if (qobject_cast<QComboBox*>(child)
                || qobject_cast<QAbstractSpinBox*>(child))
            child->installEventFilter(this);
    }
}

QString DlgPreferencesImp::longestGroupName() const
{
    std::string name;
    for (const auto &group : _pages) {
        if (group.first.size() > name.size())
            name = group.first;
    }

    return QString::fromStdString(name);
}

/**
 * Create the necessary widgets for a new group named \a groupName. Returns a 
 * pointer to the group's QTabWidget: that widget's lifetime is managed by the 
 * tabWidgetStack, do not manually deallocate.
 */
QTabWidget* DlgPreferencesImp::createTabForGroup(const std::string &groupName)
{
    QString groupNameQString = QString::fromStdString(groupName);

    std::string fileName = groupName;
    QString tooltip;
    getGroupData(groupName, fileName, tooltip);

    auto tabWidget = new QTabWidget;
    ui->tabWidgetStack->addWidget(tabWidget);
    tabWidget->setProperty("GroupName", QVariant(groupNameQString));

    auto item = new QListWidgetItem(ui->listBox);
    item->setData(GroupNameRole, QVariant(groupNameQString));
    item->setText(QObject::tr(groupNameQString.toUtf8()));
    item->setToolTip(tooltip);
    for (auto &ch : fileName) {
        if (ch == ' ')
            ch = '_';
        else
            ch = tolower(ch);
    }
    fileName = std::string("preferences-") + fileName;
    QPixmap icon = Gui::BitmapFactory().pixmapFromSvg(fileName.c_str(), QSize(48, 48));
    if (icon.isNull()) {
        icon = Gui::BitmapFactory().pixmap(fileName.c_str());
        if (icon.isNull()) {
            qWarning() << "No group icon found for " << fileName.c_str();
        }
        else if (icon.size() != QSize(48, 48)) {
            icon = icon.scaled(48, 48, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            qWarning() << "Group icon for " << fileName.c_str() << " is not of size 48x48, so it was scaled";
        }
    }
    item->setIcon(icon);
    item->setTextAlignment(Qt::AlignHCenter);
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

    return tabWidget;
}

/**
 * Create a new preference page called \a pageName on the group tab \a tabWidget.
 */
void DlgPreferencesImp::createPageInGroup(QTabWidget *tabWidget, const std::string &pageName)
{
    try {
        PreferencePage* page = WidgetFactory().createPreferencePage(pageName.c_str());
        if (page) {
            LineEditStyle::setupChildren(this);
            QScrollArea* scrollArea = new QScrollArea(this);
            scrollArea->setFrameShape(QFrame::NoFrame);
            scrollArea->setWidgetResizable(true);
            scrollArea->setWidget(page);
            tabWidget->addTab(scrollArea, page->windowTitle());
            page->loadSettings();
            page->setProperty("GroupName", tabWidget->property("GroupName"));
            page->setProperty("PageName", QVariant(QString::fromStdString(pageName)));
        }
        else {
            Base::Console().Warning("%s is not a preference page\n", pageName.c_str());
        }
    }
    catch (const Base::Exception& e) {
        Base::Console().Error("Base exception thrown for '%s'\n", pageName.c_str());
        e.ReportException();
    }
    catch (const std::exception& e) {
        Base::Console().Error("C++ exception thrown for '%s' (%s)\n", pageName.c_str(), e.what());
    }
}

void DlgPreferencesImp::changeGroup(QListWidgetItem *current, QListWidgetItem *previous)
{
    if (!current)
        current = previous;
    ui->tabWidgetStack->setCurrentIndex(ui->listBox->row(current));
}

/**
 * Adds a preference page with its class name \a className and
 * the group \a group it belongs to. To create this page it must
 * be registered in the WidgetFactory.
 * @see WidgetFactory
 * @see PrefPageProducer
 */
void DlgPreferencesImp::addPage(const std::string& className, const std::string& group)
{
    std::list<TGroupPages>::iterator groupToAddTo = _pages.end();
    for (auto it = _pages.begin(); it != _pages.end(); ++it) {
        if (it->first == group) {
            groupToAddTo = it;
            break;
        }
    }

    if (groupToAddTo != _pages.end()) {
        // The group exists: add this page to the end of the list
        groupToAddTo->second.push_back(className);
    }
    else {
        // This is a new group: create it, with its one page
        std::list<std::string> pages;
        pages.push_back(className);
        _pages.emplace_back(group, pages);
    }

    if (DlgPreferencesImp::_activeDialog) {
        // If the dialog is currently showing, tell it to insert the new page
        _activeDialog->reloadPages();
    }
}

void DlgPreferencesImp::removePage(const std::string& className, const std::string& group)
{
    for (std::list<TGroupPages>::iterator it = _pages.begin(); it != _pages.end(); ++it) {
        if (it->first == group) {
            if (className.empty()) {
                _pages.erase(it);
                return;
            }
            else {
                std::list<std::string>& p = it->second;
                for (auto jt = p.begin(); jt != p.end(); ++jt) {
                    if (*jt == className) {
                        p.erase(jt);
                        if (p.empty())
                            _pages.erase(it);
                        return;
                    }
                }
            }
        }
    }
}

/**
 * Sets a custom icon name or tool tip for a given group.
 */
void DlgPreferencesImp::setGroupData(const std::string& name, const std::string& icon, const QString& tip)
{
    Group group;
    group.iconName = icon;
    group.tooltip = tip;
    _groupMap[name] = group;
}

/**
 * Gets the icon name or tool tip for a given group. If no custom name or tool tip is given
 * they are determined from the group name.
 */
void DlgPreferencesImp::getGroupData(const std::string& group, std::string& icon, QString& tip)
{
    auto it = _groupMap.find(group);
    if (it != _groupMap.end()) {
        icon = it->second.iconName;
        tip = it->second.tooltip;
    }

    if (icon.empty())
        icon = group;

    if (tip.isEmpty())
        tip = QObject::tr(group.c_str());
}

/**
 * Activates the page at position \a index of the group with name \a group.
 */
void DlgPreferencesImp::activateGroupPage(const QString& group, int index)
{
    int ct = ui->listBox->count();
    for (int i=0; i<ct; i++) {
        QListWidgetItem* item = ui->listBox->item(i);
        if (item->data(GroupNameRole).toString() == group) {
            ui->listBox->setCurrentItem(item);
            auto tabWidget = dynamic_cast<QTabWidget*>(ui->tabWidgetStack->widget(i));
            if (tabWidget) {
                tabWidget->setCurrentIndex(index);
                break;
            }
        }
    }
}

void DlgPreferencesImp::closeEvent(QCloseEvent *e)
{
    QDialog::reject();
    e->accept();
}

void DlgPreferencesImp::reject()
{
    if (hBackup && paramTouched) {
        int res = QMessageBox::question(this, tr("Revert changes"),
                tr("Do you want to revert back to previous settings before exit?"),
                QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel);
        if (res == QMessageBox::Cancel)
            return;
        if (res == QMessageBox::Yes) {
            restartRequired = false;
            hBackup->copyTo(&App::GetApplication().GetUserParameter());
        }
    }
    restartIfRequired();
    QDialog::reject();
}

/**
 * Returns the group name \a group and position \a index of the active page.
 */
void DlgPreferencesImp::activeGroupPage(QString& group, int& index) const
{
    int row = ui->listBox->currentRow();
    auto item = ui->listBox->item(row);
    auto tabWidget = dynamic_cast<QTabWidget*>(ui->tabWidgetStack->widget(row));

    if (item && tabWidget) {
        group = item->data(GroupNameRole).toString();
        index = tabWidget->currentIndex();
    }
}

void DlgPreferencesImp::accept()
{
    this->invalidParameter = false;
    applyChanges();
    if (!this->invalidParameter) {
        QDialog::accept();
        restartIfRequired();
    }
}

void DlgPreferencesImp::onButtonBoxClicked(QAbstractButton* btn)
{
    if (ui->buttonBox->standardButton(btn) == QDialogButtonBox::Apply)
        applyChanges();
    else if (ui->buttonBox->standardButton(btn) == QDialogButtonBox::Reset)
        restoreDefaults();
}

void DlgPreferencesImp::restoreDefaults()
{
    QMessageBox box(this);
    box.setIcon(QMessageBox::Question);
    box.setWindowTitle(tr("Clear user settings"));
    box.setText(tr("Do you want to clear all your user settings?"));
    box.setInformativeText(tr("If you agree all your settings will be cleared."));
    box.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    box.setDefaultButton(QMessageBox::No);

    if (box.exec() == QMessageBox::Yes) {

        if (auto action = PresetsAction::instance())
            action->push(tr("Reset"));

        // keep this parameter
        bool saveParameter = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/General")->
                              GetBool("SaveUserParameter", true);

        ParameterManager* mgr = App::GetApplication().GetParameterSet("User parameter");
        mgr->Clear(true);

        App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/General")->
                              SetBool("SaveUserParameter", saveParameter);

        paramTouched = false;
        reject();
    }
}

/**
 * If the dialog is currently showing and the static variable _pages changed, this function 
 * will rescan that list of pages and add any that are new to the current dialog. It will not
 * remove any pages that are no longer in the list, and will not change the user's current
 * active page.
 */
void DlgPreferencesImp::reloadPages()
{
    // Make sure that pages are ready to create
    GetWidgetFactorySupplier();

    for (const auto &group : _pages) {
        QString groupName = QString::fromStdString(group.first);

        // First, does this group already exist?
        QTabWidget* tabWidget = nullptr;
        for (int tabNumber = 0; tabNumber < ui->tabWidgetStack->count(); ++tabNumber) {
            auto thisTabWidget = qobject_cast<QTabWidget*>(ui->tabWidgetStack->widget(tabNumber));
            if (thisTabWidget->property("GroupName").toString() == groupName) {
                tabWidget = thisTabWidget;
                break;
            }
        }

        // This is a new tab that wasn't there when we started this instance of the dialog: 
        if (!tabWidget) {
            tabWidget = createTabForGroup(group.first);
        }

        // Move on to the pages in the group to see if we need to add any
        for (const auto& page : group.second) {

            // Does this page already exist?
            QString pageName = QString::fromStdString(page);
            bool pageExists = false;
            for (int pageNumber = 0; pageNumber < tabWidget->count(); ++pageNumber) {
                QWidget* widget = tabWidget->widget(pageNumber);
                if (auto scrollarea = qobject_cast<QScrollArea*>(widget))
                    widget = scrollarea->widget();
                auto prefPage = qobject_cast<PreferencePage*>(widget);
                if (prefPage && prefPage->property("PageName").toString() == pageName) {
                    pageExists = true;
                    break;
                }
            }

            // This is a new page that wasn't there when we started this instance of the dialog:
            if (!pageExists) {
                createPageInGroup(tabWidget, page);
            }
        }
    }
}

void DlgPreferencesImp::applyChanges()
{
    // Checks if any of the classes that represent several pages of settings
    // (DlgSettings*.*) implement checkSettings() method.  If any of them do,
    // call it to validate if user input is correct.  If something fails (i.e.,
    // not correct), shows a messageBox and set this->invalidParameter = true to
    // cancel further operation in other methods (like in accept()).
    try {
        for (int i=0; i<ui->tabWidgetStack->count(); i++) {
            auto tabWidget = static_cast<QTabWidget*>(ui->tabWidgetStack->widget(i));
            for (int j=0; j<tabWidget->count(); j++) {
                QWidget* page = tabWidget->widget(j);
                if (auto scrollarea = qobject_cast<QScrollArea*>(page)) {
                    page = scrollarea->widget();
                    if (!page)
                        continue;
                }
                int index = page->metaObject()->indexOfMethod("checkSettings()");
                try {
                    if (index >= 0) {
                        page->qt_metacall(QMetaObject::InvokeMetaMethod, index, nullptr);
                    }
                }
                catch (const Base::Exception& e) {
                    ui->listBox->setCurrentRow(i);
                    tabWidget->setCurrentIndex(j);
                    QMessageBox::warning(this, tr("Wrong parameter"), QString::fromUtf8(e.what()));
                    throw;
                }
            }
        }
    } catch (const Base::Exception&) {
        this->invalidParameter = true;
        return;
    }

    // If everything is ok (i.e., no validation problem), call method
    // saveSettings() in every subpage (DlgSetting*) object.
    for (int i=0; i<ui->tabWidgetStack->count(); i++) {
        auto tabWidget = static_cast<QTabWidget*>(ui->tabWidgetStack->widget(i));
        for (int j=0; j<tabWidget->count(); j++) {
            QWidget* widget = tabWidget->widget(j);
            if (auto scrollarea = qobject_cast<QScrollArea*>(widget))
                widget = scrollarea->widget();
            auto page = qobject_cast<PreferencePage*>(widget);
            if (page) {
                page->saveSettings();
                restartRequired = restartRequired || page->isRestartRequired();
            }
        }
    }

    bool saveParameter = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/General")->
                          GetBool("SaveUserParameter", true);
    if (saveParameter) {
        ParameterManager* parmgr = App::GetApplication().GetParameterSet("User parameter");
        parmgr->SaveDocument(App::Application::Config()["UserParameter"].c_str());
    }
}

void DlgPreferencesImp::restartIfRequired()
{
    if (restartRequired) {
        QMessageBox* restartBox = new QMessageBox();
        restartBox->setIcon(QMessageBox::Warning);
        restartBox->setWindowTitle(tr("Restart required"));
        restartBox->setText(tr("You must restart FreeCAD for changes to take effect."));
        restartBox->setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
        restartBox->setDefaultButton(QMessageBox::Cancel);
        auto okBtn = restartBox->button(QMessageBox::Ok);
        auto cancelBtn = restartBox->button(QMessageBox::Cancel);
        okBtn->setText(tr("Restart now"));
        cancelBtn->setText(tr("Restart later"));

        int exec = restartBox->exec();

        if (exec == QMessageBox::Ok) {
            //restart FreeCAD after a delay to give time to this dialog to close
            QTimer::singleShot(1000, []() 
            {
                QStringList args = QApplication::arguments();
                args.pop_front();
                if (getMainWindow()->close())
                    QProcess::startDetached(QApplication::applicationFilePath(), args);
            });
        }
    }
}

void DlgPreferencesImp::showEvent(QShowEvent* ev)
{
    // For some reason, calling adjustSize() below overrides size restore done
    // by widgetState
    //
    // adjustSize();

    adjustListBox();
    QDialog::showEvent(ev);
}

void DlgPreferencesImp::paintEvent(QPaintEvent *ev)
{
    QDialog::paintEvent(ev);
}

void DlgPreferencesImp::adjustListBox()
{
    int width = 0;
    for (int i=0, c=ui->listBox->count(); i<c; ++i) {
        width = std::max(width,
                ui->listBox->visualItemRect(ui->listBox->item(i)).width());
    }
    width += style()->pixelMetric(QStyle::PM_ScrollBarExtent) + 10;
    ui->listBox->setFixedWidth(width);
    ui->listBox->setGridSize(QSize(width, 75));
}

bool DlgPreferencesImp::eventFilter(QObject *o, QEvent *ev)
{
    if (o->isWidgetType()) {
        auto widget = static_cast<QWidget*>(o);
        switch(ev->type()) {
        case QEvent::Wheel:
            if (!widget->hasFocus()) {
                ev->setAccepted(false);
                return true;
            }
            break;
        case QEvent::FocusIn:
            widget->setFocusPolicy(Qt::WheelFocus);
            break;
        case QEvent::FocusOut:
            widget->setFocusPolicy(Qt::StrongFocus);
            break;
        default:
            break;
        }
    }
    return QDialog::eventFilter(o, ev);
}

void DlgPreferencesImp::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
        // update the widgets' tabs
        for (int i=0; i<ui->tabWidgetStack->count(); i++) {
            auto tabWidget = static_cast<QTabWidget*>(ui->tabWidgetStack->widget(i));
            for (int j=0; j<tabWidget->count(); j++) {
                QWidget* page = tabWidget->widget(j);
                if (auto scrollarea = qobject_cast<QScrollArea*>(page))
                    page = scrollarea->widget();
                tabWidget->setTabText(j, page->windowTitle());
            }
        }
        // update the items' text
        for (int i=0; i<ui->listBox->count(); i++) {
            QListWidgetItem *item = ui->listBox->item(i);
            QByteArray group = item->data(GroupNameRole).toByteArray();
            item->setText(QObject::tr(group.constData()));
        }
        adjustListBox();
    }
    else if (e->type() == QEvent::StyleChange)
        QMetaObject::invokeMethod(this, "adjustListBox", Qt::QueuedConnection);
    else {
        QWidget::changeEvent(e);
    }
}

void DlgPreferencesImp::reload()
{
    for (int i = 0; i < ui->tabWidgetStack->count(); i++) {
        auto tabWidget = static_cast<QTabWidget*>(ui->tabWidgetStack->widget(i));
        for (int j = 0; j < tabWidget->count(); j++) {
            QWidget* widget = tabWidget->widget(j);
            if (auto scrollarea = qobject_cast<QScrollArea*>(widget))
                widget = scrollarea->widget();
            auto page = qobject_cast<PreferencePage*>(widget);
            if (page)
                page->loadSettings();
        }
    }
    applyChanges();
}

#include "moc_DlgPreferencesImp.cpp"
