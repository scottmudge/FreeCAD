/***************************************************************************
 *   Copyright (c) 2004 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <QLocale>
# include <QStyleFactory>
# include <QTextStream>
# include <QTimer>
#endif

#include "DlgGeneralImp.h"
#include "ui_DlgGeneral.h"
#include "Action.h"
#include "Application.h"
#include "DockWindowManager.h"
#include "MainWindow.h"
#include "PrefWidgets.h"
#include "PythonConsole.h"
#include "TreeParams.h"
#include "ViewParams.h"
#include "OverlayWidgets.h"
#include "Language/Translator.h"

using namespace Gui;
using namespace Gui::Dialog;

/* TRANSLATOR Gui::Dialog::DlgGeneralImp */

/**
 *  Constructs a DlgGeneralImp which is a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
DlgGeneralImp::DlgGeneralImp( QWidget* parent )
  : PreferencePage(parent)
  , ui(new Ui_DlgGeneral)
{
    ui->setupUi(this);

    // fills the combo box with all available workbenches
    // sorted by their menu text
    QStringList work = Application::Instance->workbenches();
    QMap<QString, QString> menuText;
    for (QStringList::Iterator it = work.begin(); it != work.end(); ++it) {
        QString text = Application::Instance->workbenchMenuText(*it);
        menuText[text] = *it;
    }

    ui->toolbarArea->addItem(tr("Top"), QByteArray("Top"));
    ui->toolbarArea->addItem(tr("Left"), QByteArray("Left"));
    ui->toolbarArea->addItem(tr("Right"), QByteArray("Right"));
    ui->toolbarArea->addItem(tr("Bottom"), QByteArray("Bottom"));

    ui->globalToolbarArea->addItem(tr("Top"), QByteArray("Top"));
    ui->globalToolbarArea->addItem(tr("Left"), QByteArray("Left"));
    ui->globalToolbarArea->addItem(tr("Right"), QByteArray("Right"));
    ui->globalToolbarArea->addItem(tr("Bottom"), QByteArray("Bottom"));

    {   // add special workbench to selection
        QPixmap px = Application::Instance->workbenchIcon(QString::fromLatin1("NoneWorkbench"));
        QString key = QString::fromLatin1("<last>");
        QString value = QString::fromLatin1("$LastModule");
        if (px.isNull())
            ui->AutoloadModuleCombo->addItem(key, QVariant(value));
        else
            ui->AutoloadModuleCombo->addItem(px, key, QVariant(value));
    }

    for (QMap<QString, QString>::Iterator it = menuText.begin(); it != menuText.end(); ++it) {
        QPixmap px = Application::Instance->workbenchIcon(it.value());
        if (px.isNull())
            ui->AutoloadModuleCombo->addItem(it.key(), QVariant(it.value()));
        else
            ui->AutoloadModuleCombo->addItem(px, it.key(), QVariant(it.value()));
    }

    ui->treeIconSize->setValue(TreeParams::IconSize());
    ui->treeFontSize->setValue(TreeParams::FontSize());
    ui->treeItemSpacing->setValue(TreeParams::ItemSpacing());
    ui->CmdHistorySize->setValue(ViewParams::getCommandHistorySize());
    ui->checkPopUpWindow->setChecked(ViewParams::getCheckWidgetPlacementOnRestore());
}

/**
 *  Destroys the object and frees any allocated resources
 */
DlgGeneralImp::~DlgGeneralImp()
{
}

/** Sets the size of the recent file list from the user parameters.
 * @see RecentFilesAction
 * @see StdCmdRecentFiles
 */
void DlgGeneralImp::setRecentFileSize()
{
    RecentFilesAction *recent = getMainWindow()->findChild<RecentFilesAction *>(QLatin1String("recentFiles"));
    if (recent) {
        ParameterGrp::handle hGrp = WindowParameter::getDefaultParameter()->GetGroup("RecentFiles");
        recent->resizeList(hGrp->GetInt("RecentFiles", 4));
    }
}

void DlgGeneralImp::saveSettings()
{
    int index = ui->AutoloadModuleCombo->currentIndex();
    QVariant data = ui->AutoloadModuleCombo->itemData(index);
    QString startWbName = data.toString();
    App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/General")->
                          SetASCII("AutoloadModule", startWbName.toLatin1());

    ui->RecentFiles->onSave();
    ui->SplashScreen->onSave();
    ui->PythonWordWrap->onSave();

    setRecentFileSize();

    ParameterGrp::handle hGrp = WindowParameter::getDefaultParameter()->GetGroup("General");
    QString lang = QLocale::languageToString(QLocale().language());
    QByteArray language = hGrp->GetASCII("Language", (const char*)lang.toLatin1()).c_str();
    QByteArray current = ui->Languages->itemData(ui->Languages->currentIndex()).toByteArray();
    if (current != language) {
        hGrp->SetASCII("Language", current.constData());
        // Translator::instance()->activateLanguage(current.constData());
    }

    QVariant size = ui->toolbarIconSize->itemData(ui->toolbarIconSize->currentIndex());
    int pixel = size.toInt();
    hGrp->SetInt("ToolbarIconSize", pixel);
    // getMainWindow()->setIconSize(QSize(pixel,pixel));

    hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/DockWindows");
    bool treeView=false, propertyView=false, comboView=true;
    switch(ui->treeMode->currentIndex()) {
    case 1:
        treeView = true;
        propertyView = true;
        comboView = false;
        break;
    case 2:
        treeView = true;
        comboView = true;
        propertyView = true;
        break;
    }

    if(propertyView != hGrp->GetGroup("PropertyView")->GetBool("Enabled",false)
            || treeView != hGrp->GetGroup("TreeView")->GetBool("Enabled",false)
            || comboView != hGrp->GetGroup("ComboView")->GetBool("Enabled",true))
    {
        hGrp->GetGroup("ComboView")->SetBool("Enabled",comboView);
        hGrp->GetGroup("TreeView")->SetBool("Enabled",treeView);
        hGrp->GetGroup("PropertyView")->SetBool("Enabled",propertyView);
        // getMainWindow()->initDockWindows(true);
    }

    hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/MainWindow");
    hGrp->SetBool("TiledBackground", ui->tiledBackground->isChecked());

    QVariant sheet = ui->StyleSheets->itemData(ui->StyleSheets->currentIndex());
    hGrp->SetASCII("StyleSheet", (const char*)sheet.toByteArray());

    // Most of the UI settings is now actively monitored by class AppStyleMonitor
    //
    // Application::Instance->setStyleSheet(sheet.toString(), ui->tiledBackground->isChecked());

    sheet = ui->OverlayStyleSheets->itemData(ui->OverlayStyleSheets->currentIndex());
    hGrp->SetASCII("OverlayActiveStyleSheet", (const char*)sheet.toByteArray());

    sheet = ui->MenuStyleSheets->itemData(ui->MenuStyleSheets->currentIndex());
    hGrp->SetASCII("MenuStyleSheet", (const char*)sheet.toByteArray());

    hGrp->SetASCII("DefaultToolBarArea",
            ui->toolbarArea->itemData(ui->toolbarArea->currentIndex()).toByteArray());
    hGrp->SetASCII("GlobalToolBarArea",
            ui->globalToolbarArea->itemData(ui->globalToolbarArea->currentIndex()).toByteArray());

    TreeParams::setIconSize(ui->treeIconSize->value());
    TreeParams::setFontSize(ui->treeFontSize->value());
    TreeParams::setItemSpacing(ui->treeItemSpacing->value());
    ViewParams::setCommandHistorySize(ui->CmdHistorySize->value());
    ViewParams::setCheckWidgetPlacementOnRestore(ui->checkPopUpWindow->isChecked());
    ViewParams::setDefaultFontSize(ui->appFontSize->value());
}

void DlgGeneralImp::loadSettings()
{
    std::string start = App::Application::Config()["StartWorkbench"];
    start = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/General")->
                                  GetASCII("AutoloadModule", start.c_str());
    QString startWbName = QLatin1String(start.c_str());
    ui->AutoloadModuleCombo->setCurrentIndex(ui->AutoloadModuleCombo->findData(startWbName));

    ui->RecentFiles->onRestore();
    ui->SplashScreen->onRestore();
    ui->PythonWordWrap->onRestore();

    // search for the language files
    ParameterGrp::handle hGrp = WindowParameter::getDefaultParameter()->GetGroup("General");
    QString langToStr = QLocale::languageToString(QLocale().language());
    QByteArray language = hGrp->GetASCII("Language", langToStr.toLatin1()).c_str();

    int index = 1;
    TStringMap list = Translator::instance()->supportedLocales();
    ui->Languages->clear();
    ui->Languages->addItem(QString::fromLatin1("English"), QByteArray("English"));
    for (TStringMap::iterator it = list.begin(); it != list.end(); ++it, index++) {
        QByteArray lang = it->first.c_str();
        QString langname = QString::fromLatin1(lang.constData());

#if QT_VERSION >= 0x040800
        QLocale locale(QString::fromLatin1(it->second.c_str()));
        QString native = locale.nativeLanguageName();
        if (!native.isEmpty()) {
            if (native[0].isLetter())
                native[0] = native[0].toUpper();
            langname = native;
        }
#endif

        ui->Languages->addItem(langname, lang);
        if (language == lang) {
            ui->Languages->setCurrentIndex(index);
        }
    }

    QAbstractItemModel* model = ui->Languages->model();
    if (model)
        model->sort(0);

    int current = getMainWindow()->iconSize().width();
    ui->toolbarIconSize->addItem(tr("Small (%1px)").arg(16), QVariant((int)16));
    ui->toolbarIconSize->addItem(tr("Medium (%1px)").arg(24), QVariant((int)24));
    ui->toolbarIconSize->addItem(tr("Large (%1px)").arg(32), QVariant((int)32));
    ui->toolbarIconSize->addItem(tr("Extra large (%1px)").arg(48), QVariant((int)48));
    index = ui->toolbarIconSize->findData(QVariant(current));
    if (index < 0) {
        ui->toolbarIconSize->addItem(tr("Custom (%1px)").arg(current), QVariant((int)current));
        index = ui->toolbarIconSize->findData(QVariant(current));
    }
    ui->toolbarIconSize->setCurrentIndex(index);

    ui->treeMode->addItem(tr("Combo View"));
    ui->treeMode->addItem(tr("TreeView and PropertyView"));
    ui->treeMode->addItem(tr("Both"));

    hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/DockWindows");
    bool propertyView = hGrp->GetGroup("PropertyView")->GetBool("Enabled",false);
    bool treeView = hGrp->GetGroup("TreeView")->GetBool("Enabled",false);
    bool comboView = hGrp->GetGroup("ComboView")->GetBool("Enabled",true);
    index = 0;
    if(propertyView || treeView) {
        index = comboView?2:1;
    }
    ui->treeMode->setCurrentIndex(index);

    hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/MainWindow");
    ui->tiledBackground->setChecked(hGrp->GetBool("TiledBackground", false));

    auto populateStylesheets = 
    [hGrp](const char *key, const char *path, QComboBox *combo, const char *def) {
        // List all .qss/.css files
        QMap<QString, QString> cssFiles;
        QDir dir;
        QStringList filter;
        filter << QString::fromLatin1("*.qss");
        filter << QString::fromLatin1("*.css");
        QFileInfoList fileNames;

        // read from user, resource and built-in directory
        QStringList qssPaths = QDir::searchPaths(QString::fromLatin1(path));
        for (QStringList::iterator it = qssPaths.begin(); it != qssPaths.end(); ++it) {
            dir.setPath(*it);
            fileNames = dir.entryInfoList(filter, QDir::Files, QDir::Name);
            for (QFileInfoList::iterator jt = fileNames.begin(); jt != fileNames.end(); ++jt) {
                if (cssFiles.find(jt->baseName()) == cssFiles.end()) {
                    cssFiles[jt->baseName()] = jt->fileName();
                }
            }
        }

        // now add all unique items
        combo->addItem(tr(def), QString::fromLatin1(""));
        for (QMap<QString, QString>::iterator it = cssFiles.begin(); it != cssFiles.end(); ++it) {
            combo->addItem(it.key(), it.value());
        }

        QString selectedStyleSheet = QString::fromLatin1(hGrp->GetASCII(key).c_str());
        int index = combo->findData(selectedStyleSheet);

        // might be an absolute path name
        if (index < 0 && !selectedStyleSheet.isEmpty()) {
            QFileInfo fi(selectedStyleSheet);
            if (fi.isAbsolute()) {
                QString path = fi.absolutePath();
                if (qssPaths.indexOf(path) >= 0) {
                    selectedStyleSheet = fi.fileName();
                }
                else {
                    selectedStyleSheet = fi.absoluteFilePath();
                    combo->addItem(fi.baseName(), selectedStyleSheet);
                }

                index = combo->findData(selectedStyleSheet);
            }
        }

        if (index > -1) combo->setCurrentIndex(index);
    };

    populateStylesheets("StyleSheet", "qss", ui->StyleSheets, "No style sheet");
    populateStylesheets("OverlayActiveStyleSheet", "overlay", ui->OverlayStyleSheets, "Auto");
    populateStylesheets("MenuStyleSheet", "qssm", ui->MenuStyleSheets, "Auto");

    int area = ui->toolbarArea->findData(
            QByteArray(hGrp->GetASCII("DefaultToolBarArea", "Top").c_str()));
    if (area < 0)
        area = 0;
    ui->toolbarArea->setCurrentIndex(area);

    area = ui->globalToolbarArea->findData(
            QByteArray(hGrp->GetASCII("GlobalToolBarArea", "Top").c_str()));
    if (area < 0)
        area = 0;
    ui->globalToolbarArea->setCurrentIndex(area);

    ui->treeIconSize->setValue(TreeParams::IconSize());
    ui->treeFontSize->setValue(TreeParams::FontSize());
    ui->treeItemSpacing->setValue(TreeParams::ItemSpacing());
    ui->CmdHistorySize->setValue(ViewParams::getCommandHistorySize());
    ui->checkPopUpWindow->setChecked(ViewParams::getCheckWidgetPlacementOnRestore());
    ui->appFontSize->setValue(ViewParams::getDefaultFontSize());
}

void DlgGeneralImp::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    else {
        QWidget::changeEvent(e);
    }
}

///////////////////////////////////////////////////////////
namespace {

bool applyStyleSheet(bool delayTrigger, ParameterGrp *hGrp)
{
    if (!delayTrigger)
        return true;
    auto sheet = hGrp->GetASCII("StyleSheet");
    bool tiledBG = hGrp->GetBool("TiledBackground", false);
    Application::Instance->setStyleSheet(QString::fromUtf8(sheet.c_str()), tiledBG);
    return false;
}

bool applyDockWidget(bool delayTrigger, ParameterGrp *)
{
    if (!delayTrigger) {
        OverlayManager::instance()->reload(OverlayManager::ReloadPause);
        return true;
    }
    getMainWindow()->initDockWindows(true);
    OverlayManager::instance()->reload(OverlayManager::ReloadResume);
    return false;
}

bool applyToolbarIconSize(bool, ParameterGrp *hGrp)
{
    int pixel = hGrp->GetInt("ToolbarIconSize");
    if (pixel <= 0)
        pixel = 24;
    getMainWindow()->setIconSize(QSize(pixel,pixel));
    return false;
}

bool applyLanguage(bool, ParameterGrp *hGrp)
{
    QString lang = QLocale::languageToString(QLocale().language());
    std::string language = hGrp->GetASCII("Language", (const char*)lang.toLatin1());
    Translator::instance()->activateLanguage(language.c_str());
    return false;
}

bool applyPythonWordWrap(bool, ParameterGrp *hGrp)
{
    QWidget* pc = DockWindowManager::instance()->getDockWindow("Python console");
    PythonConsole *pcPython = qobject_cast<PythonConsole*>(pc);
    if (pcPython) {
        bool pythonWordWrap = hGrp->GetBool("PythonWordWrap", true);

        if (pythonWordWrap) {
            pcPython->setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
        } else {
            pcPython->setWordWrapMode(QTextOption::NoWrap);
        }
    }
    return false;
}

struct ParamKey {
    ParameterGrp::handle hGrp;
    const char *key;
    mutable bool pending = false;

    ParamKey(const char *path, const char *key)
        :hGrp(App::GetApplication().GetUserParameter().GetGroup(path))
        ,key(key)
    {}

    ParamKey(ParameterGrp *h, const char *key)
        :hGrp(h), key(key)
    {}

    bool operator < (const ParamKey &other) const {
        if (hGrp < other.hGrp)
            return true;
        if (hGrp > other.hGrp)
            return false;
        return strcmp(key, other.key) < 0;
    }
};

struct ParamHandlers {
    std::map<ParamKey, bool (*)(bool, ParameterGrp*)> handlers;
    QTimer timer;

    void attach() {
        handlers[ParamKey("BaseApp/Preferences/MainWindow", "StyleSheet")] = applyStyleSheet;

        auto hGrp = App::GetApplication().GetParameterGroupByPath(
                "User parameter:BaseApp/Preferences/DockWindows");
        handlers[ParamKey(hGrp->GetGroup("ComboView"), "Enabled")] = applyDockWidget;
        handlers[ParamKey(hGrp->GetGroup("TreeView"), "Enabled")] = applyDockWidget;
        handlers[ParamKey(hGrp->GetGroup("PropertyView"), "Enabled")] = applyDockWidget;
        handlers[ParamKey(hGrp->GetGroup("DAGView"), "Enabled")] = applyDockWidget;

        hGrp = WindowParameter::getDefaultParameter()->GetGroup("General");
        handlers[ParamKey(hGrp, "Language")] = applyLanguage;
        handlers[ParamKey(hGrp, "ToolbarIconSize")] = applyToolbarIconSize;

        handlers[ParamKey("BaseApp/Preferences/General", "PythonWordWrap")] = applyPythonWordWrap;

        App::GetApplication().GetUserParameter().signalParamChanged.connect(
            [this](ParameterGrp *Param, const char *, const char *Name, const char *) {
                if (!Param || !Name)
                    return;
                auto it =  handlers.find(ParamKey(Param, Name));
                if (it != handlers.end() && it->second(false, Param)) {
                    it->first.pending = true;
                    timer.start(100);
                }
            });
        timer.setSingleShot(true);
        QObject::connect(&timer, &QTimer::timeout, [this]() {
            for (auto &v : handlers) {
                if (v.first.pending) {
                    v.first.pending = false;
                    v.second(true, v.first.hGrp);
                }
            }
        });
    }
};

ParamHandlers _ParamHandlers;
}


void DlgGeneralImp::attachObserver()
{
    _ParamHandlers.attach();
    ViewParams::init();
}

#include "moc_DlgGeneralImp.cpp"
