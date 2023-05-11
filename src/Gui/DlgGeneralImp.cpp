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
#include "Command.h"
#include "DockWindowManager.h"
#include "MainWindow.h"
#include "PrefWidgets.h"
#include "PythonConsole.h"
#include "TreeParams.h"
#include "ViewParams.h"
#include "OverlayWidgets.h"
#include "Language/Translator.h"
#include "Gui/PreferencePackManager.h"
#include "DlgPreferencesImp.h"

#include "DlgCreateNewPreferencePackImp.h"

// Only needed until PreferencePacks can be managed from the AddonManager:
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;


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

    {   // add special workbench to selection
        QPixmap px = Application::Instance->workbenchIcon(QStringLiteral("NoneWorkbench"));
        QString key = QStringLiteral("<last>");
        QString value = QStringLiteral("$LastModule");
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
    RecentFilesAction *recent = getMainWindow()->findChild<RecentFilesAction *>(QStringLiteral("recentFiles"));
    if (recent) {
        ParameterGrp::handle hGrp = WindowParameter::getDefaultParameter()->GetGroup("RecentFiles");
        recent->resizeList(hGrp->GetInt("RecentFiles", 4));
    }
}

static void saveTreeMode(int value)
{
    auto hGrp = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/DockWindows");
    bool treeView=false, propertyView=false, comboView=true;
    switch(value) {
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
    }
}

void DlgGeneralImp::saveSettings()
{
    int index = ui->AutoloadModuleCombo->currentIndex();
    QVariant data = ui->AutoloadModuleCombo->itemData(index);
    QString startWbName = data.toString();
    App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/General")->
                          SetASCII("AutoloadModule", startWbName.toUtf8());

    setRecentFileSize();

    ui->AutoApply->onSave();
    ui->SaveParameter->onSave();
    ui->tiledBackground->onSave();
    ui->Languages->onSave();
    ui->toolbarArea->onSave();
    ui->globalToolbarArea->onSave();
    ui->treeIconSize->onSave();
    ui->treeFontSize->onSave();
    ui->treeItemSpacing->onSave();
    ui->appFontSize->onSave();
    ui->RecentFiles->onSave();
    ui->SubstituteDecimal->onSave();
    ui->UseLocaleFormatting->onSave();
    ui->EnableCursorBlinking->onSave();
    ui->SplashScreen->onSave();
    ui->PythonWordWrap->onSave();
    ui->CmdHistorySize->onSave();
    ui->checkPopUpWindow->onSave();
    ui->toolbarIconSize->onSave();
    ui->workbenchTabIconSize->onSave();
    ui->StyleSheets->onSave();
    ui->IconSets->onSave();
    ui->OverlayStyleSheets->onSave();
    ui->MenuStyleSheets->onSave();
    ui->checkboxTaskList->onSave();
    ui->toolTipIconSize->onSave();
    saveTreeMode(ui->treeMode->currentIndex());
}

void DlgGeneralImp::populateStylesheets(const char *key,
                                        const char *path,
                                        PrefComboBox *combo,
                                        const char *def,
                                        QStringList filter) {
    auto hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/MainWindow");
    // List all .qss/.css files
    QMap<QString, QString> cssFiles;
    QDir dir;
    if (filter.isEmpty()) {
        filter << QStringLiteral("*.qss");
        filter << QStringLiteral("*.css");
    }
    QFileInfoList fileNames;

    // read from user, resource and built-in directory
    QStringList qssPaths = QDir::searchPaths(QString::fromUtf8(path));
    for (QStringList::iterator it = qssPaths.begin(); it != qssPaths.end(); ++it) {
        dir.setPath(*it);
        fileNames = dir.entryInfoList(filter, QDir::Files, QDir::Name);
        for (QFileInfoList::iterator jt = fileNames.begin(); jt != fileNames.end(); ++jt) {
            if (cssFiles.find(jt->baseName()) == cssFiles.end()) {
                cssFiles[jt->baseName()] = jt->fileName();
            }
        }
    }

    combo->clear();

    // now add all unique items
    combo->addItem(tr(def), QStringLiteral(""));
    for (QMap<QString, QString>::iterator it = cssFiles.begin(); it != cssFiles.end(); ++it) {
        combo->addItem(it.key(), it.value());
    }

    QString selectedStyleSheet = QString::fromUtf8(hGrp->GetASCII(key).c_str());
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
        }
    }

    combo->setCurrentIndex(index);
    combo->onRestore();
}

void DlgGeneralImp::setupToolBarIconSize(QComboBox *comboBox)
{
    int current = getMainWindow()->iconSize().width();
    int idx = 1;
    if (comboBox->count() != 0) {
        idx = comboBox->currentIndex();
        if (comboBox->count() > 4)
            current = comboBox->itemData(4).toInt();
    }
    QSignalBlocker blocker(comboBox);
    comboBox->clear();
    comboBox->addItem(tr("Small (%1px)").arg(16), QVariant((int)16));
    comboBox->addItem(tr("Medium (%1px)").arg(24), QVariant((int)24));
    comboBox->addItem(tr("Large (%1px)").arg(32), QVariant((int)32));
    comboBox->addItem(tr("Extra large (%1px)").arg(48), QVariant((int)48));
    if (comboBox->findData(QVariant(current)) < 0)
        comboBox->addItem(tr("Custom (%1px)").arg(current), QVariant((int)current));
    comboBox->setCurrentIndex(idx);
}

void DlgGeneralImp::loadSettings()
{
    std::string start = App::Application::Config()["StartWorkbench"];
    start = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/General")->
                                  GetASCII("AutoloadModule", start.c_str());
    QString startWbName = QString::fromUtf8(start.c_str());
    ui->AutoloadModuleCombo->setCurrentIndex(ui->AutoloadModuleCombo->findData(startWbName));

    ParameterGrp::handle hGrp = WindowParameter::getDefaultParameter()->GetGroup("General");

    // search for the language files
    QString langToStr = QLocale::languageToString(QLocale().language());
    QByteArray language = langToStr.toUtf8();

    int index = 1;
    TStringMap list = Translator::instance()->supportedLocales();
    ui->Languages->clear();
    ui->Languages->addItem(QStringLiteral("English"), QByteArray("English"));
    for (TStringMap::iterator it = list.begin(); it != list.end(); ++it, index++) {
        QByteArray lang = it->first.c_str();
        QString langname = QString::fromUtf8(lang.constData());

        QLocale locale(QString::fromUtf8(it->second.c_str()));
        QString native = locale.nativeLanguageName();
        if (!native.isEmpty()) {
            if (native[0].isLetter())
                native[0] = native[0].toUpper();
            langname = native;
        }

        ui->Languages->addItem(langname, lang);
        if (language == lang) {
            ui->Languages->setCurrentIndex(index);
        }
    }
    QAbstractItemModel* model = ui->Languages->model();
    if (model)
        model->sort(0);

    setupToolBarIconSize(ui->toolbarIconSize);
    ui->toolbarIconSize->onRestore();

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
    QObject::connect(ui->treeMode, QOverload<int>::of(&QComboBox::currentIndexChanged),
        [this](int value) { if (PrefParam::AutoSave()) saveTreeMode(value); });

    populateStylesheets("StyleSheet", "qss", ui->StyleSheets, "No style sheet");
    populateStylesheets("IconSet", "iconset", ui->IconSets, "None", QStringList(QStringLiteral("*.txt")));
    populateStylesheets("OverlayActiveStyleSheet", "overlay", ui->OverlayStyleSheets, "Auto");
    populateStylesheets("MenuStyleSheet", "qssm", ui->MenuStyleSheets, "Auto");

    ui->toolbarArea->addItem(tr("Top"), QByteArray("Top"));
    ui->toolbarArea->addItem(tr("Left"), QByteArray("Left"));
    ui->toolbarArea->addItem(tr("Right"), QByteArray("Right"));
    ui->toolbarArea->addItem(tr("Bottom"), QByteArray("Bottom"));

    ui->globalToolbarArea->addItem(tr("Top"), QByteArray("Top"));
    ui->globalToolbarArea->addItem(tr("Left"), QByteArray("Left"));
    ui->globalToolbarArea->addItem(tr("Right"), QByteArray("Right"));
    ui->globalToolbarArea->addItem(tr("Bottom"), QByteArray("Bottom"));

    ui->AutoApply->onRestore();
    QObject::connect(ui->AutoApply, &PrefCheckBox::toggled, [this](bool checked) {
        // Always auto apply for AutoApply option itself.
        PrefParam::setAutoSave(checked);
    });

    ui->SaveParameter->onRestore();
    ui->tiledBackground->onRestore();
    ui->Languages->onRestore();
    ui->toolbarArea->onRestore();
    ui->globalToolbarArea->onRestore();
    ui->treeIconSize->onRestore();
    ui->treeFontSize->onRestore();;

    ui->treeItemSpacing->setValue(TreeParams::defaultItemSpacing());
    ui->treeItemSpacing->onRestore();

    ui->checkboxTaskList->onRestore();

    ui->appFontSize->onRestore();
    ui->SubstituteDecimal->onRestore();
    ui->UseLocaleFormatting->onRestore();
    ui->EnableCursorBlinking->onRestore();
    ui->RecentFiles->onRestore();
    ui->SplashScreen->onRestore();
    ui->PythonWordWrap->onRestore();

    ui->CmdHistorySize->setValue(ViewParams::defaultCommandHistorySize());
    ui->CmdHistorySize->onRestore();

    ui->checkPopUpWindow->setChecked(ViewParams::defaultCheckWidgetPlacementOnRestore());
    ui->checkPopUpWindow->onRestore();

    ui->workbenchTabIconSize->onRestore();

    ui->toolTipIconSize->onRestore();

    updateLanguage();
}

void DlgGeneralImp::updateLanguage()
{
    ui->retranslateUi(this);
    ui->toolTipIconSize->setToolTip(QApplication::translate("ViewParams",
                ViewParams::docToolTipIconSize()));
    setupToolBarIconSize(ui->toolbarIconSize);
}

void DlgGeneralImp::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        updateLanguage();
    }
    else {
        QWidget::changeEvent(e);
    }
}

///////////////////////////////////////////////////////////
namespace {

void applyStyleSheet(ParameterGrp *hGrp)
{
    auto sheet = hGrp->GetASCII("StyleSheet");
    bool tiledBG = hGrp->GetBool("TiledBackground", false);
    Application::Instance->setStyleSheet(QString::fromUtf8(sheet.c_str()), tiledBG);
}

void applyToolTipIconSize(ParameterGrp *)
{
    Application::Instance->commandManager().refreshIcons();
}

void applyToolbarIconSize(const ParamKey *paramKey)
{
    int pixel = paramKey->hGrp->GetInt(paramKey->key);
    if (pixel <= 0)
        pixel = 24;
    getMainWindow()->setIconSize(QSize(pixel,pixel));
}

void applyLanguage(const ParamKey *paramKey)
{
    QString lang = QLocale::languageToString(QLocale().language());
    std::string language = paramKey->hGrp->GetASCII(paramKey->key, (const char*)lang.toUtf8());
    Translator::instance()->activateLanguage(language.c_str());
}

void applyNumberLocale(ParameterGrp *hGrp)
{
    int localeFormat = hGrp->GetInt("UseLocaleFormatting", 0);
    if (localeFormat == 0) {
        Translator::instance()->setLocale(); // Defaults to system locale
    }
    else if (localeFormat == 1) {
        QString lang = QLocale::languageToString(QLocale().language());
        std::string language = hGrp->GetASCII("Language", (const char*)lang.toUtf8());
        Translator::instance()->setLocale(language.c_str());
    }
    else if (localeFormat == 2) {
        Translator::instance()->setLocale("C");
    }
}

void applyCursorBlinking(const ParamKey *paramKey)
{
    int blinkTime = paramKey->hGrp->GetBool(paramKey->key, true) ? -1 : 0;
    qApp->setCursorFlashTime(blinkTime);
}

void applyDecimalPointConversion(const ParamKey *paramKey)
{
    Translator::instance()->enableDecimalPointConversion(paramKey->hGrp->GetBool(paramKey->key, false));
}

void applyPythonWordWrap(const ParamKey *paramKey)
{
    QWidget* pc = DockWindowManager::instance()->getDockWindow("Python console");
    PythonConsole *pcPython = qobject_cast<PythonConsole*>(pc);
    if (pcPython) {
        bool pythonWordWrap = paramKey->hGrp->GetBool(paramKey->key, true);

        if (pythonWordWrap) {
            pcPython->setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
        } else {
            pcPython->setWordWrapMode(QTextOption::NoWrap);
        }
    }
}

class ApplyDockWidget: public ParamHandler {
public:
    bool onChange(const ParamKey *) override
    {
        OverlayManager::instance()->reload(OverlayManager::ReloadMode::ReloadPause);
        return true;
    }

    void onTimer() override
    {
        getMainWindow()->initDockWindows(true);
        OverlayManager::instance()->reload(OverlayManager::ReloadMode::ReloadResume);
    }
};

} // anonymous namespace

void DlgGeneralImp::attachObserver()
{
    static ParamHandlers handlers;

    handlers.addDelayedHandler("BaseApp/Preferences/MainWindow",
                               {"StyleSheet", "TiledBackground", "IconSet"},
                               applyStyleSheet);

    auto hDockWindows = App::GetApplication().GetUserParameter().GetGroup("BaseApp/Preferences/DockWindows");
    auto applyDockWidget = std::shared_ptr<ParamHandler>(new ApplyDockWidget);
    handlers.addHandler(ParamKey(hDockWindows->GetGroup("ComboView"), "Enabled"), applyDockWidget);
    handlers.addHandler(ParamKey(hDockWindows->GetGroup("TreeView"), "Enabled"), applyDockWidget);
    handlers.addHandler(ParamKey(hDockWindows->GetGroup("PropertyView"), "Enabled"), applyDockWidget);
    handlers.addHandler(ParamKey(hDockWindows->GetGroup("DAGView"), "Enabled"), applyDockWidget);
    handlers.addHandler(ParamKey(hDockWindows->GetGroup("TaskWatcher"), "Enabled"), applyDockWidget);

    handlers.addDelayedHandler("BaseApp/Preferences/View", "ToolTipIconSize", applyToolTipIconSize);

    auto hGeneral = App::GetApplication().GetUserParameter().GetGroup("BaseApp/Preferences/General");
    handlers.addDelayedHandler(hGeneral, {"Language", "UseLocaleFormatting"}, applyNumberLocale);
    handlers.addHandler(hGeneral, "Language", applyLanguage);
    handlers.addHandler(hGeneral, "SubstituteDecimal", applyDecimalPointConversion);
    handlers.addHandler(hGeneral, "EnableCursorBlinking", applyCursorBlinking);
    handlers.addHandler(hGeneral, "ToolbarIconSize", applyToolbarIconSize);
    handlers.addHandler(hGeneral, "PythonWordWrap", applyPythonWordWrap);

    ViewParams::init();
}

#include "moc_DlgGeneralImp.cpp"
