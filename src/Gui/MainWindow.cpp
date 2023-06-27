/***************************************************************************
 *   Copyright (c) 2005 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <QActionGroup>
# include <QApplication>
# include <QByteArray>
# include <QCheckBox>
# include <QClipboard>
# include <QCloseEvent>
# include <QContextMenuEvent>
# include <QDesktopServices>
# if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
#   include <QDesktopWidget>
# endif
# include <QDockWidget>
# include <QFontMetrics>
# include <QKeySequence>
# include <QLabel>
# include <QMdiSubWindow>
# include <QMenu>
# include <QMessageBox>
# include <QMimeData>
# include <QPainter>
# include <QRegularExpression>
# include <QRegularExpressionMatch>
# include <QScreen>
# include <QSettings>
# include <QSignalMapper>
# include <QStatusBar>
# include <QThread>
# include <QTimer>
# include <QToolBar>
# include <QUrlQuery>
# include <QWhatsThis>
# include <QHBoxLayout>
# include <QPushButton>
#endif

#if defined(Q_OS_WIN)
# include <QtPlatformHeaders/QWindowsWindowFunctions>
#endif

#include <QWidgetAction>

#include <boost/algorithm/string/predicate.hpp>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/DocumentObjectGroup.h>
#include <Base/ConsoleObserver.h>
#include <Base/Parameter.h>
#include <Base/Exception.h>
#include <Base/FileInfo.h>
#include <Base/Interpreter.h>
#include <Base/Stream.h>
#include <Base/Tools.h>
#include <Base/UnitsApi.h>
#include <DAGView/DAGView.h>
#include <TaskView/TaskView.h>

#include "MainWindow.h"
#include "Action.h"
#include "Assistant.h"
#include "BitmapFactory.h"
#include "ComboView.h"
#include "Command.h"
#include "DockWindowManager.h"
#include "DownloadManager.h"
#include "FileDialog.h"
#include "MenuManager.h"
#include "OverlayWidgets.h"
#include "NotificationArea.h"
#include "ProgressBar.h"
#include "PropertyView.h"
#include "PythonConsole.h"
#include "ReportView.h"
#include "SelectionView.h"
#include "Splashscreen.h"
#include "ToolBarManager.h"
#include "ToolBoxManager.h"
#include "Tree.h"
#include "WaitCursor.h"
#include "Widgets.h"
#include "WidgetFactory.h"
#include "WorkbenchManager.h"
#include "Workbench.h"
#include "MergeDocuments.h"
#include "ViewProviderExtern.h"
#include "ViewParams.h"

#include "SpaceballEvent.h"
#include "View3DInventor.h"
#include "View3DInventorViewer.h"
#include "DlgObjectSelection.h"
#include "Tools.h"
#include <App/Color.h>

FC_LOG_LEVEL_INIT("MainWindow",false,true,true)

#if defined(Q_OS_WIN32)
#define slots
#endif

using namespace Gui;
using namespace Gui::DockWnd;
using namespace std;


MainWindow* MainWindow::instance = nullptr;

namespace Gui {

/**
 * The CustomMessageEvent class is used to send messages as events in the methods
 * Error(), Warning() and Message() of the StatusBarObserver class to the main window
 * to display them on the status bar instead of printing them directly to the status bar.
 *
 * This makes the usage of StatusBarObserver thread-safe.
 * @author Werner Mayer
 */
class CustomMessageEvent : public QEvent
{
public:
    CustomMessageEvent(int t, const QString& s, int timeout=0)
      : QEvent(QEvent::User), _type(t), msg(s), _timeout(timeout)
    { }
    ~CustomMessageEvent() override
    { }
    int type() const
    { return _type; }
    const QString& message() const
    { return msg; }
    int timeout() const
    { return _timeout; }
private:
    int _type;
    QString msg;
    int _timeout;
};

/**
 * The DimensionWidget class is aiming at providing a widget used in the status bar that will:
 *  - Allow application to display dimension information such as the viewportsize
 *  - Provide a popup menu allowing user to change the used unit schema (and update if changed elsewhere)
 */
class DimensionWidget : public QPushButton, WindowParameter
{
    Q_OBJECT

public:
    explicit DimensionWidget(QWidget* parent): QPushButton(parent), WindowParameter("Units")
    {
        setFlat(true);
        setText(qApp->translate("Gui::MainWindow", "Dimension"));
        setMinimumWidth(120);

        //create the action buttons
        auto* menu = new QMenu(this);
        auto* actionGrp = new QActionGroup(menu);
        int num = static_cast<int>(Base::UnitSystem::NumUnitSystemTypes);
        for (int i = 0; i < num; i++) {
            QAction* action = menu->addAction(QStringLiteral("UnitSchema%1").arg(i));
            actionGrp->addAction(action);
            action->setCheckable(true);
            action->setData(i);
        }
        QObject::connect(actionGrp, &QActionGroup::triggered, this, [this](QAction* action) {
            int userSchema = action->data().toInt();
            // Set and save the Unit System
            Base::UnitsApi::setSchema(static_cast<Base::UnitSystem>(userSchema));
            getWindowParameter()->SetInt("UserSchema", userSchema);
            // Update the application to show the unit change
            Gui::Application::Instance->onUpdate();
        } );
        setMenu(menu);
        retranslateUi();
        unitChanged();
        getWindowParameter()->Attach(this);
    }

    ~DimensionWidget()
    {
        getWindowParameter()->Detach(this);
    }

    void OnChange(Base::Subject<const char*> &rCaller, const char * sReason) override
    {
        Q_UNUSED(rCaller)
        if (strcmp(sReason, "UserSchema") == 0) {
            unitChanged();
        }
    }

    void changeEvent(QEvent *event) override
    {
        if (event->type() == QEvent::LanguageChange) {
            retranslateUi();
        }
        else {
            QPushButton::changeEvent(event);
        }
    }

private:
    void unitChanged(void)
    {
        int userSchema = getWindowParameter()->GetInt("UserSchema", 0);
        auto actions = menu()->actions();
        if(Q_UNLIKELY(userSchema < 0 || userSchema >= actions.size())) {
            userSchema = 0;
        }
        actions[userSchema]->setChecked(true);
    }

    void retranslateUi() {
        auto actions = menu()->actions();
        int maxSchema = static_cast<int>(Base::UnitSystem::NumUnitSystemTypes);
        assert(actions.size() <= maxSchema);
        for(int i = 0; i < maxSchema ; i++)
        {
            actions[i]->setText(Base::UnitsApi::getDescription(static_cast<Base::UnitSystem>(i)));
        }
    }
};

// -------------------------------------
// Pimpl class
struct MainWindowP
{
    QPushButton* sizeLabel;
    QLabel* actionLabel;
    QTimer* actionTimer;
    QTimer* statusTimer;
    QTimer* activityTimer;
    QTimer* visibleTimer;
    QTimer saveStateTimer;
    QTimer restoreStateTimer;
    QMdiArea* mdiArea;
    QPointer<MDIView> activeView;
    QSignalMapper* windowMapper;
    QSplashScreen* splashscreen;
    StatusBarObserver* status;
    bool whatsthis;
    QString whatstext;
    Assistant* assistant;
    int currentStatusType = 100;
    QString currentStatusMessage;
    int actionUpdateDelay = 0;
    QMap<QString, QPointer<UrlHandler> > urlHandler;
    std::string hiddenDockWindows;
    int screen = -1;
    boost::signals2::scoped_connection connParam;
    ParameterGrp::handle hGrp;
    bool _restoring = false;
    bool _closingAll = false;
    QTime _showNormal;

    QString overrideIcons;
    bool hasOverrideIcons = false;
    QString overrideExtraIcons;
    bool hasOverrideExtraIcons = false;

    void restoreWindowState(const QByteArray &);
    void applyOverrideIcons(const QStringList &icons);
};

class MDITabbar : public QTabBar
{
public:
    explicit MDITabbar( QWidget * parent = nullptr ) : QTabBar(parent)
    {
        menu = new QMenu(this);
        setDrawBase(false);
        setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
    }

    ~MDITabbar() override
    {
        delete menu;
    }

protected:
    void contextMenuEvent ( QContextMenuEvent * e ) override
    {
        menu->clear();
        CommandManager& cMgr = Application::Instance->commandManager();
        if (tabRect(currentIndex()).contains(e->pos()))
            cMgr.getCommandByName("Std_CloseActiveWindow")->addTo(menu);
        cMgr.getCommandByName("Std_CloseAllWindows")->addTo(menu);
        menu->addSeparator();
        cMgr.getCommandByName("Std_CascadeWindows")->addTo(menu);
        cMgr.getCommandByName("Std_TileWindows")->addTo(menu);
        menu->addSeparator();
        cMgr.getCommandByName("Std_Windows")->addTo(menu);
        menu->popup(e->globalPos());
    }

private:
    QMenu* menu;
};

#if defined(Q_OS_WIN32)
class MainWindowTabBar : public QTabBar
{
public:
    MainWindowTabBar(QWidget *parent) : QTabBar(parent)
    {
        setExpanding(false);
    }
protected:
    bool event(QEvent *e)
    {
        // show the tooltip if tab is too small to fit label
        if (e->type() != QEvent::ToolTip)
            return QTabBar::event(e);
        QSize size = this->size();
        QSize hint = sizeHint();
        if (shape() == QTabBar::RoundedWest || shape() == QTabBar::RoundedEast) {
            size.transpose();
            hint.transpose();
        }
        if (size.width() < hint.width())
            return QTabBar::event(e);
        e->accept();
        return true;
    }
    void tabInserted (int index)
    {
        // get all dock windows
        QList<QDockWidget*> dw = getMainWindow()->findChildren<QDockWidget*>();
        for (QList<QDockWidget*>::iterator it = dw.begin(); it != dw.end(); ++it) {
            // compare tab text and window title to get the right dock window
            if (this->tabText(index) == (*it)->windowTitle()) {
                QWidget* dock = (*it)->widget();
                if (dock) {
                    QIcon icon = dock->windowIcon();
                    if (!icon.isNull())
                        setTabIcon(index, icon);
                }
                break;
            }
        }
    }
};
#endif

} // namespace Gui

/* TRANSLATOR Gui::MainWindow */

MainWindow::MainWindow(QWidget * parent, Qt::WindowFlags f)
  : QMainWindow( parent, f/*WDestructiveClose*/ )
{
    d = new MainWindowP;
    d->splashscreen = nullptr;
    d->activeView = nullptr;
    d->whatsthis = false;
    d->assistant = new Assistant();

    // global access
    instance = this;

    d->connParam = App::GetApplication().GetUserParameter().signalParamChanged.connect(
        [this](ParameterGrp *Param, ParameterGrp::ParamType, const char *Name, const char *) {
            if (Param != d->hGrp || !Name)
                return;
            if (boost::equals(Name, "StatusBar")) {
                if(auto sb = getMainWindow()->statusBar())
                    sb->setVisible(d->hGrp->GetBool("StatusBar", sb->isVisible()));
            }
            else if (boost::equals(Name, "MainWindowState")) {
                OverlayManager::instance()->reload(OverlayManager::ReloadPause);
                d->restoreStateTimer.start(100);
            }
        });

    d->hGrp = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/MainWindow");
    d->saveStateTimer.setSingleShot(true);
    connect(&d->saveStateTimer, &QTimer::timeout, [this](){this->saveWindowSettings();});

    d->restoreStateTimer.setSingleShot(true);
    connect(&d->restoreStateTimer, &QTimer::timeout, [this](){
        d->restoreWindowState(QByteArray::fromBase64(d->hGrp->GetASCII("MainWindowState").c_str()));
        ToolBarManager::getInstance()->restoreState();
        OverlayManager::instance()->reload(OverlayManager::ReloadResume);
    });

    // support for grouped dragging of dockwidgets
    // https://woboq.com/blog/qdockwidget-changes-in-56.html
    setDockOptions(dockOptions() | QMainWindow::GroupedDragging);

    // Create the layout containing the workspace and a tab bar
    d->mdiArea = new QMdiArea();
    // Movable tabs
    d->mdiArea->setTabsMovable(true);
    d->mdiArea->setTabPosition(QTabWidget::South);
    d->mdiArea->setViewMode(QMdiArea::TabbedView);
    auto tab = d->mdiArea->findChild<QTabBar*>();
    if (tab) {
        tab->setTabsClosable(true);
        // The tabs might be very wide
        tab->setExpanding(false);
        tab->setObjectName(QString::fromLatin1("mdiAreaTabBar"));
    }
    d->mdiArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    d->mdiArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    d->mdiArea->setOption(QMdiArea::DontMaximizeSubWindowOnActivation, false);
    d->mdiArea->setActivationOrder(QMdiArea::ActivationHistoryOrder);
    d->mdiArea->setBackground(QBrush(QColor(160,160,160)));
    setCentralWidget(d->mdiArea);

    statusBar()->setObjectName(QStringLiteral("statusBar"));
    connect(statusBar(), &QStatusBar::messageChanged, this, &MainWindow::statusMessageChanged);

    // labels and progressbar
    d->status = new StatusBarObserver();
    d->actionLabel = new QLabel(statusBar());
    d->actionLabel->setObjectName(QStringLiteral("SB_ActionLabel"));
    d->actionLabel->setWindowTitle(tr("Message label"));
    // d->actionLabel->setMinimumWidth(120);

    d->sizeLabel = new DimensionWidget(statusBar());

    statusBar()->addWidget(d->actionLabel, 1);
    QProgressBar* progressBar = Gui::SequencerBar::instance()->getProgressBar(statusBar());
    progressBar->setWindowTitle(tr("Progress bar"));
    progressBar->setObjectName(QStringLiteral("SB_ProgressBar"));
    statusBar()->addPermanentWidget(progressBar, 0);
    statusBar()->addPermanentWidget(d->sizeLabel, 0);

    auto hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/NotificationArea");

    auto notificationAreaEnabled = hGrp->GetBool("NotificationAreaEnabled", true);

    if(notificationAreaEnabled) {
        NotificationArea* notificationArea = new NotificationArea(statusBar());
        notificationArea->setObjectName(QString::fromLatin1("notificationArea"));
        notificationArea->setStyleSheet(QStringLiteral("text-align:left;"));
        statusBar()->addPermanentWidget(notificationArea);
    }
    // clears the action label
    d->actionTimer = new QTimer( this );
    d->actionTimer->setObjectName(QStringLiteral("actionTimer"));
    connect(d->actionTimer, &QTimer::timeout, d->actionLabel, &QLabel::clear);

    // clear status type
    d->statusTimer = new QTimer( this );
    d->statusTimer->setObjectName(QStringLiteral("statusTimer"));
    connect(d->statusTimer, &QTimer::timeout, this, &MainWindow::clearStatus);

    // update gui timer
    d->activityTimer = new QTimer(this);
    d->activityTimer->setObjectName(QStringLiteral("activityTimer"));
    connect(d->activityTimer, &QTimer::timeout, this, &MainWindow::_updateActions);
    d->activityTimer->setSingleShot(false);
    d->activityTimer->start(150);

    // update view-sensitive commands when clipboard has changed
    QClipboard *clipbd = QApplication::clipboard();
    connect(clipbd, &QClipboard::dataChanged, this, &MainWindow::updateEditorActions);

    d->windowMapper = new QSignalMapper(this);

    // connection between workspace, window menu and tab bar
#if QT_VERSION < QT_VERSION_CHECK(5,15,0)
    connect(d->windowMapper, qOverload<QWidget*>(&QSignalMapper::mapped),
            this, &MainWindow::onSetActiveSubWindow);
#elif QT_VERSION < QT_VERSION_CHECK(6,0,0)
    connect(d->windowMapper, &QSignalMapper::mappedWidget,
            this, &MainWindow::onSetActiveSubWindow);
#else
    connect(d->windowMapper, &QSignalMapper::mappedObject,
            this, [=](QObject* object) {
        onSetActiveSubWindow(qobject_cast<QWidget*>(object));
    });
#endif
    connect(d->mdiArea, &QMdiArea::subWindowActivated,
            this, &MainWindow::onWindowActivated);

    setupDockWindows();

    // accept drops on the window, get handled in dropEvent, dragEnterEvent
    setAcceptDrops(true);

    // setup font substitutions for NaviCube
    // Helvetica usually gives good enough results on mac & linux
    // in rare cases Helvetica matches a bad font on linux
    // Nimbus Sans Narrow and Open Sans Condensed added as fallback
    // Bahnschrift is a condensed font available on windows versions since 2017
    // Arial added as fallback for older version
    auto substitutions = QStringLiteral("Bahnschrift,Helvetica,Nimbus Sans Narrow,Open Sans Condensed,Arial,Sans");
    auto family = QStringLiteral("FreeCAD NaviCube");
    QFont::insertSubstitutions(family, substitutions.split(QLatin1Char(',')));

    statusBar()->showMessage(tr("Ready"), 2001);
}

MainWindow::~MainWindow()
{
    delete d->status;
    delete d;
    instance = nullptr;
}

MainWindow* MainWindow::getInstance()
{
    // MainWindow has a public constructor
    return instance;
}

// Helper function to update dock widget according to the user parameter
// settings, e.g. register/unregister, enable/disable, show/hide.
template<class T>
static inline void _updateDockWidget(const char *name,
                                    bool enabled,
                                    bool show,
                                    Qt::DockWidgetArea pos,
                                    T callback)
{
    auto pDockMgr = DockWindowManager::instance();
    auto widget = pDockMgr->findRegisteredDockWindow(name);
    if (!enabled) {
        if(widget) {
            pDockMgr->removeDockWindow(widget);
            pDockMgr->unregisterDockWindow(name);
            widget->deleteLater();
        }
        return;
    }
    // Use callback to perform specific update for each type of dock widget
    widget = callback(widget);
    if(!widget)
        return;
    DockWindowManager::instance()->registerDockWindow(name, widget);
    if(show) {
        auto dock = pDockMgr->addDockWindow(
                widget->objectName().toUtf8().constData(), widget, pos);
        if(dock) {
            if(!dock->toggleViewAction()->isChecked())
                dock->toggleViewAction()->activate(QAction::Trigger);
            OverlayManager::instance()->refresh(dock);
        }
    }
}

void MainWindow::initDockWindows(bool show)
{
    bool treeView = false;

    if (d->hiddenDockWindows.find("Std_TreeView") == std::string::npos) {
        //work through parameter.
        ParameterGrp::handle group = App::GetApplication().GetUserParameter().
                GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("DockWindows")->GetGroup("TreeView");
        bool enabled = group->GetBool("Enabled", true);
        if (enabled != group->GetBool("Enabled", false)) {
            enabled = App::GetApplication().GetUserParameter().GetGroup("BaseApp")
                            ->GetGroup("MainWindow")->GetGroup("DockWindows")->GetBool("Std_TreeView",false);
        }
        group->SetBool("Enabled", enabled); //ensure entry exists.
        treeView = enabled;
        _updateDockWidget("Std_TreeView", enabled, show, Qt::RightDockWidgetArea,
            [](QWidget *widget) {
                if(widget)
                    return widget;
                TreeDockWidget* tree = new TreeDockWidget(0,getMainWindow());
                tree->setObjectName(QStringLiteral(QT_TRANSLATE_NOOP("QDockWidget","Tree view")));
                tree->setMinimumWidth(210);
                widget = tree;
                return widget;
            });
    }

    // Property view
    if (d->hiddenDockWindows.find("Std_PropertyView") == std::string::npos) {
        //work through parameter.
        ParameterGrp::handle group = App::GetApplication().GetUserParameter().
                GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("DockWindows")->GetGroup("PropertyView");
        bool enabled = treeView || group->GetBool("Enabled", false);
        if (!treeView && enabled != group->GetBool("Enabled", true)) {
            enabled = App::GetApplication().GetUserParameter().GetGroup("BaseApp")
                            ->GetGroup("MainWindow")->GetGroup("DockWindows")->GetBool("Std_PropertyView",false);
        }
        group->SetBool("Enabled", enabled); //ensure entry exists.
        _updateDockWidget("Std_PropertyView", enabled, show, Qt::RightDockWidgetArea,
            [](QWidget *widget) {
                if(widget)
                    return widget;
                PropertyDockView* pcPropView = new PropertyDockView(0, getMainWindow());
                pcPropView->setObjectName(QStringLiteral(QT_TRANSLATE_NOOP("QDockWidget","Property view")));
                pcPropView->setMinimumWidth(210);
                widget = pcPropView;
                return widget;
            });
    }

    // Combo view
    if (d->hiddenDockWindows.find("Std_ComboView") == std::string::npos) {
        bool enable = !treeView;
        if (!enable) {
            ParameterGrp::handle group = App::GetApplication().GetUserParameter().
                    GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("DockWindows")->GetGroup("ComboView");
            enable = group->GetBool("Enabled", true);
        }
        _updateDockWidget("Std_ComboView", true, show, Qt::LeftDockWidgetArea,
            [enable](QWidget *widget) {
                auto pcComboView = qobject_cast<ComboView*>(widget);
                if(widget) {
                    if(pcComboView)
                        pcComboView->setShowModel(enable);
                    return widget;
                }
                pcComboView = new ComboView(enable, 0, getMainWindow());
                pcComboView->setObjectName(QStringLiteral(QT_TRANSLATE_NOOP("QDockWidget","Combo View")));
                pcComboView->setMinimumWidth(150);
                widget = pcComboView;
                return widget;
            });
    }

    //Task List (task watcher).
    if (d->hiddenDockWindows.find("Std_TaskWatcher") == std::string::npos) {
        //work through parameter.
        ParameterGrp::handle group = App::GetApplication().GetUserParameter().
              GetGroup("BaseApp/Preferences/DockWindows/TaskWatcher");
        bool enabled = group->GetBool("Enabled", false);
        group->SetBool("Enabled", enabled); //ensure entry exists.
        _updateDockWidget("Std_TaskWatcher", enabled, show, Qt::RightDockWidgetArea,
            [](QWidget *widget) {
                if(widget)
                    return widget;
                widget = new TaskView::TaskView(getMainWindow());
                widget->setObjectName(QStringLiteral(QT_TRANSLATE_NOOP("QDockWidget","Task List")));
                return widget;
            });
    }

    //Dag View.
    if (d->hiddenDockWindows.find("Std_DAGView") == std::string::npos) {
        //work through parameter.
        // old group name
        ParameterGrp::handle deprecateGroup = App::GetApplication().GetUserParameter().
              GetGroup("BaseApp")->GetGroup("Preferences");
        bool enabled = false;
        if (deprecateGroup->HasGroup("DAGView")) {
            deprecateGroup = deprecateGroup->GetGroup("DAGView");
            enabled = deprecateGroup->GetBool("Enabled", enabled);
        }
        // new group name
        ParameterGrp::handle group = App::GetApplication().GetUserParameter().
              GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("DockWindows")->GetGroup("DAGView");
        enabled = group->GetBool("Enabled", enabled);
        group->SetBool("Enabled", enabled); //ensure entry exists.

        _updateDockWidget("Std_DAGView", enabled, show, Qt::RightDockWidgetArea,
            [](QWidget *widget) {
                if(widget)
                    return widget;
                DAG::DockWindow *dagDockWindow = new DAG::DockWindow(nullptr, getMainWindow());
                dagDockWindow->setObjectName(QStringLiteral(QT_TRANSLATE_NOOP("QDockWidget","DAG View")));
                widget = dagDockWindow;
                return widget;
            });
    }
}

void MainWindow::setupDockWindows()
{
    // Report view must be created before PythonConsole!
    setupReportView();
    setupPythonConsole();
    setupSelectionView();

    initDockWindows(false);
}

bool MainWindow::setupSelectionView()
{
    // Selection view
    if (d->hiddenDockWindows.find("Std_SelectionView") == std::string::npos) {
        auto pcSelectionView = new SelectionView(nullptr, this);
        pcSelectionView->setObjectName
            (QStringLiteral(QT_TRANSLATE_NOOP("QDockWidget","Selection view")));
        pcSelectionView->setMinimumWidth(210);

        DockWindowManager* pDockMgr = DockWindowManager::instance();
        pDockMgr->registerDockWindow("Std_SelectionView", pcSelectionView);
        return true;
    }

    return false;
}

bool MainWindow::setupReportView()
{
    // Report view
    if (d->hiddenDockWindows.find("Std_ReportView") == std::string::npos) {
        auto pcReport = new ReportOutput(this);
        pcReport->setWindowIcon(BitmapFactory().pixmap("MacroEditor"));
        pcReport->setObjectName
            (QStringLiteral(QT_TRANSLATE_NOOP("QDockWidget","Report view")));

        DockWindowManager* pDockMgr = DockWindowManager::instance();
        pDockMgr->registerDockWindow("Std_ReportView", pcReport);

        auto rvObserver = new ReportOutputObserver(pcReport);
        qApp->installEventFilter(rvObserver);
        return true;
    }

    return false;
}

bool MainWindow::setupPythonConsole()
{
    // Python console
    if (d->hiddenDockWindows.find("Std_PythonView") == std::string::npos) {
        auto pcPython = new PythonConsole(this);
        pcPython->setWindowIcon(Gui::BitmapFactory().iconFromTheme("applications-python"));
        pcPython->setObjectName
            (QStringLiteral(QT_TRANSLATE_NOOP("QDockWidget","Python console")));

        DockWindowManager* pDockMgr = DockWindowManager::instance();
        pDockMgr->registerDockWindow("Std_PythonView", pcPython);
        return true;
    }

    return false;
}

namespace {

enum MenuType {
    ToolBarMenu,
    DockWindowMenu,
};

void populateMenu(QMenu *menu, MenuType type, bool popup,
                  const QString &shortcutPrefix = QString())
{
    auto mw = getMainWindow();
    QMap<QString, QAction*> actions;
    QString tooltip, ltooltip;
    QMenu *hiddenMenu = nullptr;
    QMenu *lockMenu = nullptr;
    menu->setToolTipsVisible(true);

    if (ViewParams::getEnableMenuBarCheckBox())
        popup = true;

    switch(type) {
    case ToolBarMenu: {
        tooltip = QObject::tr("Toggles this toolbar");
        ltooltip = QObject::tr("Lock this toolbar");
        lockMenu = new QMenu(QObject::tr("Lock toolbars"), menu);
        lockMenu->setToolTipsVisible(true);

        auto cb = [](bool checked) {ToolBarManager::getInstance()->setDefaultMovable(!checked);};
        QAction *lockAction = nullptr;
        if (popup) {
            QCheckBox *checkbox;
            lockAction = Action::addCheckBox(lockMenu, QObject::tr("Default"),
                    QObject::tr("Default locking of all toolbars"), QIcon(),
                    !ToolBarManager::getInstance()->isDefaultMovable(), &checkbox);
            QObject::connect(lockAction, &QAction::toggled, cb);
            QObject::connect(checkbox, &QCheckBox::toggled, cb);
        } else {
            lockAction = lockMenu->addAction(QObject::tr("Default"));
            QObject::connect(lockAction, &QAction::toggled, cb);
            lockAction->setToolTip(QObject::tr("Default locking of all toolbars"));
            lockAction->setCheckable(true);
            lockAction->setChecked(!ToolBarManager::getInstance()->isDefaultMovable());
        }
        lockMenu->addSeparator();

        bool relocate = false;
        QRect rectMain(mw->mapToGlobal(QPoint(0,0)), QSize(mw->width()-20, mw->height()-20));

        for (auto toolbar : mw->findChildren<QToolBar*>()) {
            auto action = toolbar->toggleViewAction();
            auto parent = toolbar->parentWidget();

            relocate = relocate || (toolbar->isFloating()
                    && !rectMain.contains(toolbar->mapToGlobal(QPoint(0,0))));

            if (parent == mw || parent == mw->statusBar()) {
                // Some misbehaved code may force the toolbar to be visible
                // while hiding its action, which causes the user to be unable
                // to switch it off. We'll just include those actions anyway.
                if ((action->isVisible() || toolbar->isVisible()) && action->text().size()) {
                    action->setVisible(true);
                    actions[action->text()] = action;
                    auto cb = [toolbar](bool checked) {toolbar->setMovable(!checked);};
                    if (!popup) {
                        auto act = lockMenu->addAction(action->text());
                        act->setToolTip(ltooltip);
                        act->setIcon(action->icon());
                        act->setCheckable(true);
                        act->setChecked(!toolbar->isMovable());
                        QObject::connect(act, &QAction::toggled, cb);
                        continue;
                    }
                    QCheckBox *checkbox;
                    auto wa = Action::addCheckBox(lockMenu, action->text(),
                            ltooltip, action->icon(), !toolbar->isMovable(), &checkbox);
                    QObject::connect(wa, &QWidgetAction::toggled, cb);
                    QObject::connect(lockAction, &QAction::toggled, [checkbox](bool checked){
                        QSignalBlocker block(checkbox);
                        checkbox->setChecked(checked);
                    });
                }
#ifdef FC_DEBUG
                else {
                    actions[action->text().size() ? action->text() : toolbar->objectName()] = action;
                    if (!hiddenMenu) {
                        hiddenMenu = new QMenu(QObject::tr("Other toolbars"), menu);
                        hiddenMenu->setToolTipsVisible(true);
                    }
                }
#endif
            }
        }

        if (relocate) {
            // At least for MacOS, it is possible to move the toolbar outside
            // the screen making it impossible to move back
            menu->addAction(QObject::tr("Relocate outlier toolbars"), [rectMain]() {
                for (auto tb : getMainWindow()->findChildren<QToolBar*>()) {
                    if (!tb->isFloating())
                        continue;
                    QPoint pos = tb->mapToGlobal(QPoint(0,0));
                    if (rectMain.contains(pos))
                        continue;
                    tb->move(QPoint(std::min(std::max(pos.x(), rectMain.left()), rectMain.right()),
                                    std::min(std::max(pos.y(), rectMain.top()), rectMain.bottom())));
                }
            });
            menu->addSeparator();
        }
        break;
    }
    case DockWindowMenu:
        tooltip = QObject::tr("Toggles this dockable window");
        for(auto dock : mw->findChildren<QDockWidget*>()) {
            auto action = dock->toggleViewAction();
            actions[action->text()] = action;
            if (!hiddenMenu && !action->isVisible()) {
                hiddenMenu = new QMenu(QObject::tr("Other dockables"), menu);
                hiddenMenu->setToolTipsVisible(true);
            }
        }
    }

    auto addCheckBox = [&](QMenu *m, const QString &title, QAction *action) {
        QCheckBox *checkbox;
        auto wa = Action::addCheckBox(
                m, title, tooltip, action->icon(), action->isChecked(), &checkbox);
        QObject::connect(checkbox, SIGNAL(toggled(bool)), action, SIGNAL(triggered(bool)));
        QObject::connect(wa, SIGNAL(triggered(bool)), action, SIGNAL(triggered(bool)));
        return wa;
    };

    if (!popup || shortcutPrefix.isEmpty()) {
        for(auto it=actions.begin(); it!=actions.end(); ++it) {
            auto action = *it;
            auto m = hiddenMenu && !action->isVisible() ? hiddenMenu : menu;
            if (!popup)
                m->addAction(action);
            else
                addCheckBox(m, it.key(), action);
        }
    }
    else {
        // Auto assign shortcuts. Try to reuse old one so that the shortcut won't
        // jump around due to the fact that the actions are sorted by their names.
        std::map<int, std::pair<QString, QAction*>> shortcutMap;
        for(auto it=actions.begin(); it!=actions.end();) {
            auto action = *it;
            if (hiddenMenu && !action->isVisible()) {
                addCheckBox(hiddenMenu, it.key(), action);
                it = actions.erase(it);
                continue;
            }
            QString shortcut = action->shortcut().toString();
            if (shortcut.startsWith(shortcutPrefix)) {
                int idx = shortcut.right(shortcut.size()-shortcutPrefix.size()).toInt();
                if (!shortcutMap.emplace(idx, std::make_pair(it.key(), action)).second) {
                    action->setShortcut(QString());
                } else {
                    it = actions.erase(it);
                    continue;
                }
            }
            ++it;
        }
        int i = 1;
        for(auto it=actions.begin(); it!=actions.end(); ++it) {
            for (; shortcutMap.count(i); ++i);
            shortcutMap.emplace(i,std::make_pair(it.key(), it.value()));
        }

        for (const auto &v : shortcutMap) {
            int idx = v.first;
            QString title = v.second.first;
            QAction *action = v.second.second;
            QString shortcut;
            if (idx <= 9) {
                shortcut = QStringLiteral("%1%2").arg(shortcutPrefix).arg(idx);
                title += QStringLiteral("  (%1)").arg(shortcut);
            }
            auto wa = addCheckBox(menu, title, action);
            wa->setShortcut(shortcut);
            action->setShortcut(shortcut);
        }
    }

    if (hiddenMenu)
        menu->addMenu(hiddenMenu);
    if (lockMenu)
        menu->addMenu(lockMenu);
}

} // anonymous namespace

QMenu* MainWindow::createPopupMenu ()
{
    QMenu *menu = new QMenu(this);
    populateMenu(menu, DockWindowMenu, true);
    menu->addSeparator();
    populateMenu(menu, ToolBarMenu, true);
    menu->addSeparator();

    Workbench* wb = WorkbenchManager::instance()->active();
    if (wb) {
        MenuItem item;
        wb->createMainWindowPopupMenu(&item);
        if (item.hasItems()) {
            menu->addSeparator();
            QList<MenuItem*> items = item.getItems();
            for (const auto & item : items) {
                if (item->command() == "Separator") {
                    menu->addSeparator();
                }
                else {
                    Command* cmd = Application::Instance->commandManager().getCommandByName(item->command().c_str());
                    if (cmd) cmd->addTo(menu);
                }
            }
        }
    }

    return menu;
}

void MainWindow::tile()
{
    d->mdiArea->tileSubWindows();
}

void MainWindow::cascade()
{
    d->mdiArea->cascadeSubWindows();
}

void MainWindow::closeActiveWindow ()
{
    d->mdiArea->closeActiveSubWindow();
}

int MainWindow::confirmSave(const char *docName, QWidget *parent, bool addCheckbox) {
    QMessageBox box(parent?parent:this);
    box.setIcon(QMessageBox::Question);
    box.setWindowTitle(QObject::tr("Unsaved document"));
    if(docName)
        box.setText(QObject::tr("Do you want to save your changes to document '%1' before closing?")
                    .arg(QString::fromUtf8(docName)));
    else
        box.setText(QObject::tr("Do you want to save your changes to document before closing?"));

    box.setInformativeText(QObject::tr("If you don't save, your changes will be lost."));
    box.setStandardButtons(QMessageBox::Discard | QMessageBox::Cancel | QMessageBox::Save);
    box.setDefaultButton(QMessageBox::Save);
    box.setEscapeButton(QMessageBox::Cancel);

    QCheckBox checkBox(QObject::tr("Apply answer to all"));
    ParameterGrp::handle hGrp;
    if(addCheckbox) {
         hGrp = App::GetApplication().GetUserParameter().
            GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("General");
        checkBox.setChecked(hGrp->GetBool("ConfirmAll",false));
        checkBox.blockSignals(true);
        box.addButton(&checkBox, QMessageBox::ResetRole);
    }

    // add shortcuts
    QAbstractButton* saveBtn = box.button(QMessageBox::Save);
    if (saveBtn->shortcut().isEmpty()) {
        QString text = saveBtn->text();
        text.prepend(QLatin1Char('&'));
        saveBtn->setShortcut(QKeySequence::mnemonic(text));
    }

    QAbstractButton* discardBtn = box.button(QMessageBox::Discard);
    if (discardBtn->shortcut().isEmpty()) {
        QString text = discardBtn->text();
        text.prepend(QLatin1Char('&'));
        discardBtn->setShortcut(QKeySequence::mnemonic(text));
    }

    int res = ConfirmSaveResult::Cancel;
    box.adjustSize(); // Silence warnings from Qt on Windows
    switch (box.exec())
    {
    case QMessageBox::Save:
        res = checkBox.isChecked()?ConfirmSaveResult::SaveAll:ConfirmSaveResult::Save;
        break;
    case QMessageBox::Discard:
        res = checkBox.isChecked()?ConfirmSaveResult::DiscardAll:ConfirmSaveResult::Discard;
        break;
    }
    if(addCheckbox && res)
        hGrp->SetBool("ConfirmAll",checkBox.isChecked());
    return res;
}

bool MainWindow::isClosingAll() const
{
    return d->_closingAll;
}

bool MainWindow::closeAllDocuments (bool close)
{
    Base::StateLocker guard(d->_closingAll);
    auto docs = App::GetApplication().getDocuments();
    try {
        docs = App::Document::getDependentDocuments(docs, true);
    }
    catch(Base::Exception &e) {
        e.ReportException();
    }

    bool checkModify = true;
    bool saveAll = false;
    int failedSaves = 0;

    for (auto doc : docs) {
        auto gdoc = Application::Instance->getDocument(doc);
        if (!gdoc)
            continue;
        if (!gdoc->canClose(false))
            return false;
        if (!gdoc->isModified()
                || doc->testStatus(App::Document::PartialDoc)
                || doc->testStatus(App::Document::TempDoc))
            continue;
        bool save = saveAll;
        if (!save && checkModify) {
            int res = confirmSave(doc->Label.getStrValue().c_str(), this, docs.size()>1);
            switch (res)
            {
            case ConfirmSaveResult::Cancel:
                return false;
            case ConfirmSaveResult::SaveAll:
                saveAll = true;
                /* FALLTHRU */
            case ConfirmSaveResult::Save:
                save = true;
                break;
            case ConfirmSaveResult::DiscardAll:
                checkModify = false;
            }
        }

        if (save && !gdoc->save())
            failedSaves++;
    }

    if (failedSaves > 0) {
        int ret = QMessageBox::question(
            getMainWindow(),
            QObject::tr("%1 Document(s) not saved").arg(QString::number(failedSaves)),
            QObject::tr("Some documents could not be saved. Do you want to cancel closing?"),
            QMessageBox::Discard | QMessageBox::Cancel,
            QMessageBox::Discard);
        if (ret == QMessageBox::Cancel)
            return false;
    }

    if (close)
        App::GetApplication().closeAllDocuments();

    return true;
}

void MainWindow::activateNextWindow ()
{
    auto tab = d->mdiArea->findChild<QTabBar*>();
    if (tab && tab->count() > 0) {
        int index = (tab->currentIndex() + 1) % tab->count();
        tab->setCurrentIndex(index);
    }
}

void MainWindow::activatePreviousWindow ()
{
    auto tab = d->mdiArea->findChild<QTabBar*>();
    if (tab && tab->count() > 0) {
        int index = (tab->currentIndex() + tab->count() - 1) % tab->count();
        tab->setCurrentIndex(index);
    }
}

void MainWindow::activateWorkbench(const QString& name)
{
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/View");
    bool saveWB = hGrp->GetBool("SaveWBbyTab", false);
    QMdiSubWindow* subWin = d->mdiArea->activeSubWindow();
    if (subWin && saveWB) {
        QString currWb = subWin->property("ownWB").toString();
        if (currWb.isEmpty() || currWb != name) {
            subWin->setProperty("ownWB", name);
        }
    }
    // emit this signal
    Q_EMIT workbenchActivated(name);
    updateActions(true);
}

void MainWindow::whatsThis()
{
    QWhatsThis::enterWhatsThisMode();
}

void MainWindow::showDocumentation(const QString& help)
{
    Base::PyGILStateLocker lock;
    PyObject* module = PyImport_ImportModule("Help");
    if (module) {
        Py_DECREF(module);
        Gui::Command::addModule(Gui::Command::Gui,"Help");
        Gui::Command::doCommand(Gui::Command::Gui,"Help.show(\"%s\")", help.toStdString().c_str());
    }
    else {
        PyErr_Clear();
        QUrl url(help);
        if (url.scheme().isEmpty()) {
            QMessageBox msgBox(getMainWindow());
            msgBox.setWindowTitle(tr("Help addon needed!"));
            msgBox.setText(tr("The Help system of %1 is now handled by the \"Help\" addon. "
               "It can easily be installed via the Addons Manager").arg(QString(qApp->applicationName())));
            QAbstractButton* pButtonAddonMgr = msgBox.addButton(tr("Open Addon Manager"), QMessageBox::YesRole);
            msgBox.addButton(QMessageBox::Ok);
            msgBox.exec();
            if (msgBox.clickedButton() == pButtonAddonMgr) {
                ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Addons");
                hGrp->SetASCII("SelectedAddon", "Help");
                Gui::Command::doCommand(Gui::Command::Gui,"Gui.runCommand('Std_AddonMgr',0)");
            }
        }
        else {
            QDesktopServices::openUrl(url);
        }
    }
}

bool MainWindow::event(QEvent *e)
{
    if (e->type() == QEvent::WindowStateChange) {
        if (isMaximized()
                && d->_showNormal.isValid()
                && d->_showNormal > QTime::currentTime())
        {
            QMetaObject::invokeMethod(this, "showNormal", Qt::QueuedConnection);
        }
        d->_showNormal = QTime();
    }
    if (e->type() == QEvent::EnterWhatsThisMode) {
        // Unfortunately, for top-level widgets such as menus or dialogs we
        // won't be notified when the user clicks the link in the hypertext of
        // the what's this text. Thus, we have to install the main window to
        // the application to observe what happens in eventFilter().
        d->whatstext.clear();
        if (!d->whatsthis) {
            d-> whatsthis = true;
            qApp->installEventFilter(this);
        }
    }
    else if (e->type() == QEvent::LeaveWhatsThisMode) {
        // Here we can't do anything because this event is sent
        // before the WhatThisClicked event is sent. Thus, we handle
        // this in eventFilter().
    }
    else if (e->type() == QEvent::WhatsThisClicked) {
        auto wt = static_cast<QWhatsThisClickedEvent*>(e);
        showDocumentation(wt->href());
    }
    else if (e->type() == QEvent::ApplicationWindowIconChange) {
        // if application icon changes apply it to the main window and the "About..." dialog
        this->setWindowIcon(QApplication::windowIcon());
        Command* about = Application::Instance->commandManager().getCommandByName("Std_About");
        if (about) {
            Action* action = about->getAction();
            if (action) action->setIcon(QApplication::windowIcon());
        }
    }
    else if (e->type() == Spaceball::ButtonEvent::ButtonEventType) {
        auto buttonEvent = dynamic_cast<Spaceball::ButtonEvent *>(e);
        if (!buttonEvent)
            return true;
        buttonEvent->setHandled(true);
        //only going to respond to button press events.
        if (buttonEvent->buttonStatus() != Spaceball::BUTTON_PRESSED)
            return true;
        ParameterGrp::handle group = App::GetApplication().GetUserParameter().GetGroup("BaseApp")->
                GetGroup("Spaceball")->GetGroup("Buttons");
        QByteArray groupName(QVariant(buttonEvent->buttonNumber()).toByteArray());
        if (group->HasGroup(groupName.data())) {
            ParameterGrp::handle commandGroup = group->GetGroup(groupName.data());
            std::string commandName(commandGroup->GetASCII("Command"));
            if (commandName.empty())
                return true;
            else
                Application::Instance->commandManager().runCommandByName(commandName.c_str());
        }
        else
            return true;
    }
    else if (e->type() == Spaceball::MotionEvent::MotionEventType) {
        auto motionEvent = dynamic_cast<Spaceball::MotionEvent *>(e);
        if (!motionEvent)
            return true;
        motionEvent->setHandled(true);
        Gui::Document *doc = Application::Instance->activeDocument();
        if (!doc)
            return true;
        auto temp = dynamic_cast<View3DInventor *>(doc->getActiveView());
        if (!temp)
            return true;
        View3DInventorViewer *view = temp->getViewer();
        if (view) {
            Spaceball::MotionEvent anotherEvent(*motionEvent);
            qApp->sendEvent(view, &anotherEvent);
        }
        return true;
    }else if(e->type() == QEvent::StatusTip) {
        // make sure warning and error message don't get blocked by tooltips
        if(std::abs(d->currentStatusType) <= MainWindow::Wrn)
            return true;
    }
    return QMainWindow::event(e);
}

bool MainWindow::eventFilter(QObject* o, QEvent* e)
{
    if (o != this) {
        if (e->type() == QEvent::WindowStateChange) {
            // notify all mdi views when the active view receives a show normal, show minimized
            // or show maximized event
            auto view = qobject_cast<MDIView*>(o);
            if (view) { // emit this signal
                Qt::WindowStates oldstate = static_cast<QWindowStateChangeEvent*>(e)->oldState();
                Qt::WindowStates newstate = view->windowState();
                if (oldstate != newstate)
                    Q_EMIT windowStateChanged(view);
            }
        }

        // We don't want to show the bubble help for the what's this text but want to
        // start the help viewer with the according key word.
        // Thus, we have to observe WhatThis events if called for a widget, use its text and
        // must avoid to make the bubble widget visible.
        if (e->type() == QEvent::WhatsThis) {
            if (!o->isWidgetType())
                return false;
            // clicked on a widget in what's this mode
            auto w = static_cast<QWidget *>(o);
            d->whatstext = w->whatsThis();
        }
        if (e->type() == QEvent::WhatsThisClicked) {
            // if the widget is a top-level window
            if (o->isWidgetType() && qobject_cast<QWidget*>(o)->isWindow()) {
                // re-direct to the widget
                QApplication::sendEvent(this, e);
            }
        }
        // special treatment for menus because they directly call QWhatsThis::showText()
        // whereby we must be informed for which action the help should be shown
        if (o->inherits("QMenu") && QWhatsThis::inWhatsThisMode()) {
            bool whatthis = false;
            if (e->type() == QEvent::KeyPress) {
                auto ke = static_cast<QKeyEvent*>(e);
                if (ke->key() == Qt::Key_Return || ke->key() == Qt::Key_Enter || ke->key() == Qt::Key_F1)
                    whatthis = true;
            }
            else if (e->type() == QEvent::MouseButtonRelease)
                whatthis = true;
            else if (e->type() == QEvent::EnterWhatsThisMode)
                whatthis = true;
            if (whatthis) {
                QAction* cur = static_cast<QMenu*>(o)->activeAction();
                if (cur) {
                    // get the help text for later usage
                    QString s = cur->whatsThis();
                    if (s.isEmpty())
                        s = static_cast<QMenu*>(o)->whatsThis();
                    d->whatstext = s;
                }
            }
        }
        if (o->inherits("QWhatsThat") && e->type() == QEvent::Show) {
            // the bubble help should become visible which we avoid by marking the widget
            // that it is out of range. Instead of, we show the help viewer
            if (!d->whatstext.isEmpty()) {
                QWhatsThisClickedEvent e(d->whatstext);
                QApplication::sendEvent(this, &e);
            }
            static_cast<QWidget *>(o)->setAttribute(Qt::WA_OutsideWSRange);
            o->deleteLater();
            return true;
        }
        if (o->inherits("QWhatsThat") && e->type() == QEvent::Hide) {
            // leave what's this mode
            if (d->whatsthis) {
                d->whatsthis = false;
                d->whatstext.clear();
                qApp->removeEventFilter(this);
            }
        }
    }

    return QMainWindow::eventFilter(o, e);
}

void MainWindow::addWindow(MDIView* view)
{
    // make workspace parent of view
    bool isempty = d->mdiArea->subWindowList().isEmpty();
    auto child = qobject_cast<QMdiSubWindow*>(view->parentWidget());
    if(!child) {
        child = new QMdiSubWindow(d->mdiArea->viewport());
        child->setAttribute(Qt::WA_DeleteOnClose);
        child->setWidget(view);
        child->setWindowIcon(view->windowIcon());
        QMenu* menu = child->systemMenu();

        // See StdCmdCloseActiveWindow (#0002631)
        QList<QAction*> acts = menu->actions();
        for (auto & act : acts) {
            if (act->shortcut() == QKeySequence(QKeySequence::Close)) {
                act->setShortcuts(QList<QKeySequence>());
                break;
            }
        }

        QAction* action = menu->addAction(tr("Close All"));
        connect(action, &QAction::triggered, d->mdiArea, &QMdiArea::closeAllSubWindows);
        d->mdiArea->addSubWindow(child);
    }

    connect(view, &MDIView::message, this, &MainWindow::showMessage);
    connect(this, &MainWindow::windowStateChanged, view, &MDIView::windowStateChanged);

    // listen to the incoming events of the view
    view->installEventFilter(this);

    // show the very first window in maximized mode
    if (isempty)
        view->showMaximized();
    else
        view->show();
}

/**
 * Removes the instance of Gui::MDiView from the main window and sends am event
 * to the parent widget, a QMdiSubWindow to delete itself.
 * If you want to avoid that the Gui::MDIView instance gets destructed too you
 * must reparent it afterwards, e.g. set parent to NULL.
 */
void MainWindow::removeWindow(Gui::MDIView* view, bool close)
{
    // free all connections
    disconnect(view, &MDIView::message, this, &MainWindow::showMessage);
    disconnect(this, &MainWindow::windowStateChanged, view, &MDIView::windowStateChanged);

    view->removeEventFilter(this);

    // check if the focus widget is a child of the view
    QWidget* foc = this->focusWidget();
    if (foc) {
        QWidget* par = foc->parentWidget();
        while (par) {
            if (par == view) {
                foc->clearFocus();
                break;
            }
            par = par->parentWidget();
        }
    }

    QWidget* parent = view->parentWidget();

    // The call of 'd->mdiArea->removeSubWindow(parent)' causes the QMdiSubWindow
    // to lose its parent and thus the notification in QMdiSubWindow::closeEvent
    // of other mdi windows to get maximized if this window is maximized will fail.
    // However, we must let it here otherwise deleting MDI child views directly can
    // cause other problems.
    //
    // The above mentioned problem can be fixed by setParent(0) which triggers a
    // ChildRemoved event being handled properly inside QMidArea::viewportEvent()
    //
    auto subwindow = qobject_cast<QMdiSubWindow*>(parent);
    if(subwindow && d->mdiArea->subWindowList().contains(subwindow)) {
        subwindow->setParent(nullptr);

        assert(!d->mdiArea->subWindowList().contains(subwindow));
        // d->mdiArea->removeSubWindow(parent);
    }

    if(close)
        parent->deleteLater();
    updateActions();
}

void MainWindow::tabChanged(MDIView* view)
{
    Q_UNUSED(view);
    updateActions();
}

void MainWindow::tabCloseRequested(int index)
{
    auto tab = d->mdiArea->findChild<QTabBar*>();
    if (index < 0 || index >= tab->count())
        return;

    const QList<QMdiSubWindow *> subWindows = d->mdiArea->subWindowList();
    Q_ASSERT(index < subWindows.size());

    QMdiSubWindow *subWindow = d->mdiArea->subWindowList().at(index);
    Q_ASSERT(subWindow);
    subWindow->close();
    updateActions();
}

void MainWindow::onSetActiveSubWindow(QWidget *window)
{
    if (!window)
        return;
    d->mdiArea->setActiveSubWindow(qobject_cast<QMdiSubWindow *>(window));
    updateActions();
}

void MainWindow::setActiveWindow(MDIView* view)
{
    if (!view || d->activeView == view)
        return;
    onSetActiveSubWindow(view->parentWidget());
    d->activeView = view;
    Application::Instance->viewActivated(view);
    updateActions();
}

void MainWindow::onWindowActivated(QMdiSubWindow* w)
{
    if (!w)
        return;
    auto view = dynamic_cast<MDIView*>(w->widget());
    if(view == d->activeView)
        return;

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/View");
    bool saveWB = hGrp->GetBool("SaveWBbyTab", false);
    if (saveWB) {
        QString currWb = w->property("ownWB").toString();
        if (! currWb.isEmpty()) {
            this->activateWorkbench(currWb);
        }
        else {
            w->setProperty("ownWB", QString::fromStdString(WorkbenchManager::instance()->active()->name()));
        }
    }

    // Even if windowActivated() signal is emitted mdi doesn't need to be a top-level window.
    // This happens e.g. if two windows are top-level and one of them gets docked again.
    // QWorkspace emits the signal then even though the other window is in front.
    // The consequence is that the docked window becomes the active window and not the undocked
    // window on top. This means that all accel events, menu and toolbar actions get redirected
    // to the (wrong) docked window.
    // But just testing whether the view is active and ignore it if not leads to other more serious problems -
    // at least under Linux. It seems to be a problem with the window manager.
    // Under Windows it seems to work though it's not really sure that it works reliably.
    // Result: So, we accept the first problem to be sure to avoid the second one.
    if ( !view /*|| !mdi->isActiveWindow()*/ ) {
        OverlayManager::instance()->refresh();
        return; // either no MDIView or no valid object or no top-level window
    }

    // set active the appropriate window (it needs not to be part of mdiIds, e.g. directly after creation)
    d->activeView = view;
    Application::Instance->viewActivated(view);
    updateActions(true);
}

void MainWindow::onWindowsMenuAboutToShow()
{
    QList<QMdiSubWindow*> windows = d->mdiArea->subWindowList(QMdiArea::CreationOrder);
    QWidget* active = d->mdiArea->activeSubWindow();

    // We search for the 'Std_WindowsMenu' command that provides the list of actions
    CommandManager& cMgr = Application::Instance->commandManager();
    Command* cmd = cMgr.getCommandByName("Std_WindowsMenu");
    QList<QAction*> actions = qobject_cast<ActionGroup*>(cmd->getAction())->actions();

    // do the connection only once
    static bool firstShow = true;
    if (firstShow) {
        firstShow = false;
        QAction* last = actions.isEmpty() ? 0 : actions.last();
        for (const auto & action : actions) {
            if (action == last)
                break; // this is a separator
            connect(action, &QAction::triggered, d->windowMapper, qOverload<>(&QSignalMapper::map));
        }
    }

    int numWindows = std::min<int>(actions.count()-1, windows.count());
    for (int index = 0; index < numWindows; index++) {
        QWidget* child = windows.at(index);
        QAction* action = actions.at(index);
        QString text;
        QString title = child->windowTitle();
        int lastIndex = title.lastIndexOf(QStringLiteral("[*]"));
        if (lastIndex > 0) {
            title = title.left(lastIndex);
            if (child->isWindowModified())
                title = QStringLiteral("%1*").arg(title);
        }
        if (index < 9)
            text = QStringLiteral("&%1 %2").arg(index+1).arg(title);
        else
            text = QStringLiteral("%1 %2").arg(index+1).arg(title);
        action->setText(text);
        action->setVisible(true);
        action->setChecked(child == active);
        d->windowMapper->setMapping(action, child);
    }

    // if less windows than actions
    for (int index = numWindows; index < actions.count(); index++)
        actions[index]->setVisible(false);
    // show the separator
    if (numWindows > 0)
        actions.last()->setVisible(true);
}

void MainWindow::onToolBarMenuAboutToShow()
{
    auto menu = static_cast<QMenu*>(sender());
    menu->clear();
    populateMenu(menu, ToolBarMenu, false);
}

void MainWindow::onDockWindowMenuAboutToShow()
{
    auto menu = static_cast<QMenu*>(sender());
    menu->clear();
    QString shortcutPrefix = QString::fromUtf8(d->hGrp->GetASCII("DockableWindowShortcut", "D, D").c_str());
    shortcutPrefix = shortcutPrefix.trimmed();
    if (!shortcutPrefix.isEmpty()) {
        if (!shortcutPrefix.endsWith(QLatin1Char(',')))
            shortcutPrefix += QStringLiteral(", ");
        else
            shortcutPrefix += QStringLiteral(" ");
    }
    populateMenu(menu, DockWindowMenu, false, shortcutPrefix);
}

void MainWindow::setDockWindowMenu(QMenu* menu)
{
    connect(menu, &QMenu::aboutToShow, this, &MainWindow::onDockWindowMenuAboutToShow);
}

void MainWindow::setToolBarMenu(QMenu* menu)
{
    connect(menu, &QMenu::aboutToShow, this, &MainWindow::onToolBarMenuAboutToShow);
}

void MainWindow::setWindowsMenu(QMenu* menu)
{
    connect(menu, &QMenu::aboutToShow, this, &MainWindow::onWindowsMenuAboutToShow);
}

QList<QWidget*> MainWindow::windows(QMdiArea::WindowOrder order) const
{
    QList<QWidget*> mdis;
    QList<QMdiSubWindow*> wnds = d->mdiArea->subWindowList(order);
    for (const auto & wnd : wnds) {
        mdis << wnd->widget();
    }
    return mdis;
}

MDIView* MainWindow::activeWindow() const
{
    // each activated window notifies this main window when it is activated
    return d->activeView;
}

void MainWindow::closeEvent (QCloseEvent * e)
{
    Application::Instance->tryClose(e);
    if (e->isAccepted()) {
        // Send close event to all non-modal dialogs
        QList<QDialog*> dialogs = this->findChildren<QDialog*>();
        // It is possible that closing a dialog internally closes further dialogs. Thus,
        // we have to check the pointer before.
        QVector< QPointer<QDialog> > dialogs_ptr;
        for (const auto & dialog : dialogs) {
            dialogs_ptr.append(dialog);
        }
        for (auto & it : dialogs_ptr) {
            if (!it.isNull())
                it->close();
        }
        QList<MDIView*> mdis = this->findChildren<MDIView*>();
        // Force to close any remaining (passive) MDI child views
        for (auto & mdi : mdis) {
            mdi->hide();
            mdi->deleteLater();
        }

        if (Workbench* wb = WorkbenchManager::instance()->active())
            wb->removeTaskWatcher();

        Q_EMIT  mainWindowClosed();
        d->activityTimer->stop();

        saveWindowSettings();

        // https://forum.freecad.org/viewtopic.php?f=8&t=67748
        // When the session manager jumps in it can happen that the closeEvent()
        // function is triggered twice and for the second call the main window might be
        // invisible. In this case the window settings shouldn't be saved.
        // 
        // Setting _restoring = true will prevent saveWindowSettings() to actually save the states
        d->_restoring = true;

        delete d->assistant;
        d->assistant = nullptr;

        // See createMimeDataFromSelection
        QVariant prop = this->property("x-documentobject-file");
        if (!prop.isNull()) {
            Base::FileInfo fi((const char*)prop.toByteArray());
            if (fi.exists())
                fi.deleteFile();
        }

        if (this->property("QuitOnClosed").isValid()) {
            QApplication::closeAllWindows();
            qApp->quit(); // stop the event loop
        }
    }
}

void MainWindow::showEvent(QShowEvent* e)
{
    std::clog << "Show main window" << std::endl;
    QMainWindow::showEvent(e);
}

void MainWindow::hideEvent(QHideEvent* e)
{
    std::clog << "Hide main window" << std::endl;
    QMainWindow::hideEvent(e);
}

void MainWindow::processMessages(const QList<QByteArray> & msg)
{
    // handle all the messages to open files
    try {
        WaitCursor wc;
        std::list<std::string> files;
        QByteArray action("OpenFile:");
        for (const auto & it : msg) {
            if (it.startsWith(action))
                files.emplace_back(it.mid(action.size()).constData());
        }
        files = App::Application::processFiles(files);
        for (const auto & file : files) {
            QString filename = QString::fromUtf8(file.c_str(), file.size());
            FileDialog::setWorkingDirectory(filename);
        }
    }
    catch (const Base::SystemExitException&) {
    }
}

void MainWindow::delayedStartup()
{
    // automatically run unit tests in Gui
    if (App::Application::Config()["RunMode"] == "Internal") {
        QTimer::singleShot(1000, this, []{
            try {
                Base::Interpreter().runString(
                    "import sys\n"
                    "import FreeCAD\n"
                    "import QtUnitGui\n\n"
                    "testCase = FreeCAD.ConfigGet(\"TestCase\")\n"
                    "QtUnitGui.addTest(testCase)\n"
                    "QtUnitGui.setTest(testCase)\n"
                    "result = QtUnitGui.runTest()\n"
                    "sys.stdout.flush()\n"
                    "sys.exit(0 if result else 1)");
            }
            catch (const Base::SystemExitException&) {
                throw;
            }
            catch (const Base::Exception& e) {
                e.ReportException();
            }
        });
        return;
    }

    // processing all command line files
    try {
        std::list<std::string> files = App::Application::getCmdLineFiles();
        files = App::Application::processFiles(files);
        for (const auto & file : files) {
            QString filename = QString::fromUtf8(file.c_str(), file.size());
            FileDialog::setWorkingDirectory(filename);
        }
    }
    catch (const Base::SystemExitException&) {
        throw;
    }

    const std::map<std::string,std::string>& cfg = App::Application::Config();
    auto it = cfg.find("StartHidden");
    if (it != cfg.end()) {
        QApplication::quit();
        return;
    }

    // Create new document?
    ParameterGrp::handle hGrp = WindowParameter::getDefaultParameter()->GetGroup("Document");
    if (hGrp->GetBool("CreateNewDoc", false)) {
        if (App::GetApplication().getDocuments().empty()){
            Application::Instance->commandManager().runCommandByName("Std_New");
        }
    }

    if (hGrp->GetBool("RecoveryEnabled", true)) {
        Application::Instance->checkForPreviousCrashes();
    }
}

void MainWindow::appendRecentFile(const QString& filename)
{
    auto recent = this->findChild<RecentFilesAction *>
        (QStringLiteral("recentFiles"));
    if (recent) {
        recent->appendFile(filename);
    }
}

void MainWindow::appendRecentMacro(const QString& filename)
{
    auto recent = this->findChild<RecentMacrosAction *>
        (QStringLiteral("recentMacros"));
    if (recent) {
        recent->appendFile(filename);
    }
}

void MainWindow::updateActions(bool delay)
{
    //make it safe to call before the main window is actually created
    if (!instance)
        return;

    if (!d->activityTimer->isActive()) {
        // If for some reason updateActions() is called from a worker thread
        // we must avoid to directly call QTimer::start() because this leaves
        // the whole application in a weird state
        if (d->activityTimer->thread() != QThread::currentThread()) {
            QMetaObject::invokeMethod(d->activityTimer, "start", Qt::QueuedConnection,
                QGenericReturnArgument(), Q_ARG(int, 150));
        }
        else {
            d->activityTimer->start(150);
        }
    }
    else if (delay) {
        if (!d->actionUpdateDelay)
            d->actionUpdateDelay = 1;
    }
    else {
        d->actionUpdateDelay = -1;
    }
}

void MainWindow::_updateActions()
{
    if (isVisible() && d->actionUpdateDelay <= 0) {
        FC_LOG("update actions");
        d->activityTimer->stop();
        Application::Instance->commandManager().testActive();
    }
    d->actionUpdateDelay = 0;
}

void MainWindow::updateEditorActions()
{
    Command* cmd = nullptr;
    CommandManager& mgr = Application::Instance->commandManager();

    cmd = mgr.getCommandByName("Std_Cut");
    if (cmd) cmd->testActive();

    cmd = mgr.getCommandByName("Std_Copy");
    if (cmd) cmd->testActive();

    cmd = mgr.getCommandByName("Std_Paste");
    if (cmd) cmd->testActive();

    cmd = mgr.getCommandByName("Std_Undo");
    if (cmd) cmd->testActive();

    cmd = mgr.getCommandByName("Std_Redo");
    if (cmd) cmd->testActive();
}

void MainWindow::switchToTopLevelMode()
{
    QList<QDockWidget*> dw = this->findChildren<QDockWidget*>();
    for (auto & it : dw) {
        it->setParent(nullptr, Qt::Window);
        it->show();
    }
    QList<QWidget*> mdi = getMainWindow()->windows();
    for (auto & it : mdi) {
        it->setParent(nullptr, Qt::Window);
        it->show();
    }
}

void MainWindow::switchToDockedMode()
{
    // Search for all top-level MDI views
    QWidgetList toplevel = QApplication::topLevelWidgets();
    for (const auto & it : toplevel) {
        auto view = qobject_cast<MDIView*>(it);
        if (view)
            view->setCurrentViewMode(MDIView::Child);
    }
}

void MainWindow::loadWindowSettings()
{
    QString vendor = QString::fromUtf8(App::Application::Config()["ExeVendor"].c_str());
    QString application = QString::fromUtf8(App::Application::Config()["ExeName"].c_str());
    int major = (QT_VERSION >> 0x10) & 0xff;
    int minor = (QT_VERSION >> 0x08) & 0xff;
    QString qtver = QStringLiteral("Qt%1.%2").arg(major).arg(minor);
    QSettings config(vendor, application);

#if QT_VERSION < QT_VERSION_CHECK(5,14,0)
    QRect rect = QApplication::desktop()->availableGeometry(this);
#else
    QRect rect = this->screen()->availableGeometry();
#endif

    config.beginGroup(qtver);
    QPoint pos = config.value(QStringLiteral("Position"), this->pos()).toPoint();
    QSize size = config.value(QStringLiteral("Size"), rect.size()).toSize();
    bool max = config.value(QStringLiteral("Maximized"), false).toBool();
    bool showStatusBar = config.value(QStringLiteral("StatusBar"), true).toBool();
    QByteArray windowState = config.value(QStringLiteral("MainWindowState")).toByteArray();
    config.endGroup();

    std::string geometry = d->hGrp->GetASCII("Geometry");
    std::istringstream iss(geometry);
    int x,y,w,h;
    if (iss >> x >> y >> w >> h) {
        pos = QPoint(x,y);
        size = QSize(w,h);
    }
    max = d->hGrp->GetBool("Maximized", max);
    showStatusBar = d->hGrp->GetBool("StatusBar", showStatusBar);
    std::string wstate = d->hGrp->GetASCII("MainWindowState");
    if (wstate.size())
        windowState = QByteArray::fromBase64(wstate.c_str());

    x = std::max<int>(rect.left(), std::min<int>(rect.left()+rect.width()/2, pos.x()));
    y = std::max<int>(rect.top(), std::min<int>(rect.top()+rect.height()/2, pos.y()));
    w = std::min<int>(rect.width(), size.width());
    h = std::min<int>(rect.height(), size.height());

    this->move(x, y);
    this->resize(w, h);

    Base::StateLocker guard(d->_restoring);

    d->restoreWindowState(windowState);
    std::clog << "Main window restored" << std::endl;

    if (max)
        showMaximized();
    else {
        // It seems that X window can't really show normal if the window hasn't
        // been shown before, or may be because of some other reason. The work
        // around here is to use _showNormal to set a time interval, within
        // which is the window is maximized (by some unknown force), we change
        // it back to normal.
        d->_showNormal = QTime::currentTime().addSecs(1);
        showNormal();
    }
    statusBar()->setVisible(showStatusBar);

    ToolBarManager::getInstance()->restoreState();
    std::clog << "Toolbars restored" << std::endl;

    OverlayManager::instance()->restore();
}

bool MainWindow::isRestoringWindowState() const
{
    return d->_restoring;
}

void MainWindowP::restoreWindowState(const QByteArray &windowState)
{
    if (windowState.isEmpty())
        return;

    Base::StateLocker guard(_restoring);

    // tmp. disable the report window to suppress some bothering warnings
    if (Base::Console().IsMsgTypeEnabled("ReportOutput", Base::ConsoleSingleton::MsgType_Wrn)) {
        Base::Console().SetEnabledMsgType("ReportOutput", Base::ConsoleSingleton::MsgType_Wrn, false);
        getMainWindow()->restoreState(windowState);
        Base::Console().SetEnabledMsgType("ReportOutput", Base::ConsoleSingleton::MsgType_Wrn, true);
    } else
        getMainWindow()->restoreState(windowState);

    Base::ConnectionBlocker block(connParam);
    // as a notification for user code on window state restore
    hGrp->SetBool("WindowStateRestored", !hGrp->GetBool("WindowStateRestored", false));
}

void MainWindow::saveWindowSettings(bool canDelay)
{
    if (isRestoringWindowState())
        return;

    if (canDelay) {
        d->saveStateTimer.start(100);
        return;
    }

    QString vendor = QString::fromUtf8(App::Application::Config()["ExeVendor"].c_str());
    QString application = QString::fromUtf8(App::Application::Config()["ExeName"].c_str());
    int major = (QT_VERSION >> 0x10) & 0xff;
    int minor = (QT_VERSION >> 0x08) & 0xff;
    QString qtver = QStringLiteral("Qt%1.%2").arg(major).arg(minor);
    QSettings config(vendor, application);

#if 0
    config.beginGroup(qtver);
    config.setValue(QStringLiteral("Size"), this->size());
    config.setValue(QStringLiteral("Position"), this->pos());
    config.setValue(QStringLiteral("Maximized"), this->isMaximized());
    config.setValue(QStringLiteral("MainWindowState"), this->saveState());
    config.setValue(QStringLiteral("StatusBar"), this->statusBar()->isVisible());
    config.endGroup();
#else
    // We are migrating from saving qt main window layout state in QSettings to
    // FreeCAD parameters, for more control. The old settings is explicitly
    // remove from old QSettings conf to allow easier complete reset of
    // application state by just removing FC user.cfg file.
    config.remove(qtver);
#endif

    Base::ConnectionBlocker block(d->connParam);
    d->hGrp->SetBool("Maximized", this->isMaximized());
    d->hGrp->SetBool("StatusBar", this->statusBar()->isVisible());
    d->hGrp->SetASCII("MainWindowState", this->saveState().toBase64().constData());

    std::ostringstream ss;
    QRect rect(this->pos(), this->size());
    ss << rect.left() << " " << rect.top() << " " << rect.width() << " " << rect.height();
    d->hGrp->SetASCII("Geometry", ss.str().c_str());

    DockWindowManager::instance()->saveState();
    OverlayManager::instance()->save();
    ToolBarManager::getInstance()->saveState();
}

void MainWindow::startSplasher()
{
    // startup splasher
    // when running in verbose mode no splasher
    if (!(App::Application::Config()["Verbose"] == "Strict") &&
         (App::Application::Config()["RunMode"] == "Gui")) {
        ParameterGrp::handle hGrp = App::GetApplication().GetUserParameter().
            GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("General");
        // first search for an external image file
        if (hGrp->GetBool("ShowSplasher", true)) {
            d->splashscreen = new SplashScreen(this->splashImage());
            d->splashscreen->show();
        }
        else
            d->splashscreen = nullptr;
    }
}

void MainWindow::stopSplasher()
{
    if (d->splashscreen) {
        d->splashscreen->finish(this);
        delete d->splashscreen;
        d->splashscreen = nullptr;
    }
}

QPixmap MainWindow::aboutImage() const
{
    // See if we have a custom About screen image set
    QPixmap about_image;
    QFileInfo fi(QStringLiteral("images:about_image.png"));
    if (fi.isFile() && fi.exists())
        about_image.load(fi.filePath(), "PNG");

    std::string about_path = App::Application::Config()["AboutImage"];
    if (!about_path.empty() && about_image.isNull()) {
        QString path = QString::fromUtf8(about_path.c_str());
        if (QDir(path).isRelative()) {
            QString home = QString::fromStdString(App::Application::getHomePath());
            path = QFileInfo(QDir(home), path).absoluteFilePath();
        }
        about_image.load(path);

        // Now try the icon paths
        if (about_image.isNull()) {
            about_image = Gui::BitmapFactory().pixmap(about_path.c_str());
        }
    }

    return about_image;
}

QPixmap MainWindow::splashImage() const
{
    // search in the UserAppData dir as very first
    QPixmap splash_image;
    QFileInfo fi(QStringLiteral("images:splash_image.png"));
    if (fi.isFile() && fi.exists())
        splash_image.load(fi.filePath(), "PNG");

    // if no image was found try the config
    std::string splash_path = App::Application::Config()["SplashScreen"];
    if (splash_image.isNull()) {
        QString path = QString::fromUtf8(splash_path.c_str());
        if (QDir(path).isRelative()) {
            QString home = QString::fromStdString(App::Application::getHomePath());
            path = QFileInfo(QDir(home), path).absoluteFilePath();
        }

        splash_image.load(path);
    }

    // now try the icon paths
    if (splash_image.isNull()) {
        if (qApp->devicePixelRatio() > 1.0) {
            // For HiDPI screens, we have a double-resolution version of the splash image
            splash_path += "2x";
            splash_image = Gui::BitmapFactory().pixmap(splash_path.c_str());
            splash_image.setDevicePixelRatio(2.0);
        }
        else {
            splash_image = Gui::BitmapFactory().pixmap(splash_path.c_str());
        }
    }

    // include application name and version number
    std::map<std::string,std::string>::const_iterator tc = App::Application::Config().find("SplashInfoColor");
    if (tc != App::Application::Config().end()) {
        QString title = qApp->applicationName();
        QString major   = QString::fromUtf8(App::Application::Config()["BuildVersionMajor"].c_str());
        QString minor   = QString::fromUtf8(App::Application::Config()["BuildVersionMinor"].c_str());
        QString point   = QString::fromUtf8(App::Application::Config()["BuildVersionPoint"].c_str());
        QString version = QStringLiteral("%1.%2.%3").arg(major, minor, point);
        QString position, fontFamily;

        std::map<std::string,std::string>::const_iterator te = App::Application::Config().find("SplashInfoExeName");
        std::map<std::string,std::string>::const_iterator tv = App::Application::Config().find("SplashInfoVersion");
        std::map<std::string,std::string>::const_iterator tp = App::Application::Config().find("SplashInfoPosition");
        std::map<std::string,std::string>::const_iterator tf = App::Application::Config().find("SplashInfoFont");
        if (te != App::Application::Config().end())
            title = QString::fromUtf8(te->second.c_str());
        if (tv != App::Application::Config().end())
            version = QString::fromUtf8(tv->second.c_str());
        if (tp != App::Application::Config().end())
            position = QString::fromUtf8(tp->second.c_str());
        if (tf != App::Application::Config().end())
            fontFamily = QString::fromUtf8(tf->second.c_str());

        QPainter painter;
        painter.begin(&splash_image);
        if (!fontFamily.isEmpty()) {
            QFont font = painter.font();
            if (font.fromString(fontFamily))
                painter.setFont(font);
        }

        QFont fontExe = painter.font();
        fontExe.setPointSizeF(20.0);
        QFontMetrics metricExe(fontExe);
        int l = QtTools::horizontalAdvance(metricExe, title);
        int w = splash_image.width();
        int h = splash_image.height();

        QFont fontVer = painter.font();
        fontVer.setPointSizeF(12.0);
        QFontMetrics metricVer(fontVer);
        int v = QtTools::horizontalAdvance(metricVer, version);

        if (l + v + 20 > w) {
            fontExe.setPointSize(18);
            QFontMetrics metricExe(fontExe);
            l = QtTools::horizontalAdvance(metricExe, title);

            fontVer.setPointSize(10);
            QFontMetrics metricVer(fontVer);
            v = QtTools::horizontalAdvance(metricVer, version);
        }

        int x = -1, y = -1;
        QRegularExpression rx(QStringLiteral("(\\d+).(\\d+)"));
        auto match = rx.match(position);
        if (match.hasMatch()) {
            x = match.captured(1).toInt();
            y = match.captured(2).toInt();
        }
        else {
            x = w - (l + v + 10);
            y = h - 20;
        }

        QColor color;
        color.setNamedColor(QString::fromUtf8(tc->second.c_str()));
        if (color.isValid()) {
            painter.setPen(color);
            painter.setFont(fontExe);
            painter.drawText(x, y, title);
            painter.setFont(fontVer);
            painter.drawText(x + (l + 5), y, version);
            painter.end();
        }
    }

    return splash_image;
}

/**
 * Drops the event \a e and tries to open the files.
 */
void MainWindow::dropEvent (QDropEvent* e)
{
    const QMimeData* data = e->mimeData();
    if (data->hasUrls()) {
        // load the files into the active document if there is one, otherwise let create one
        loadUrls(App::GetApplication().getActiveDocument(), data->urls());
    }
    else {
        QMainWindow::dropEvent(e);
    }
}

void MainWindow::dragEnterEvent (QDragEnterEvent * e)
{
    // Here we must allow uri drafs and check them in dropEvent
    const QMimeData* data = e->mimeData();
    if (data->hasUrls()) {
        e->accept();
    }
    else {
        e->ignore();
    }
}

static QString _MimeDocObj = QStringLiteral("application/x-documentobject");
static QString _MimeDocObjX = QStringLiteral("application/x-documentobject-x");
static QString _MimeDocObjFile = QStringLiteral("application/x-documentobject-file");
static QString _MimeDocObjXFile = QStringLiteral("application/x-documentobject-x-file");

QMimeData * MainWindow::createMimeDataFromSelection () const
{
    std::vector<App::DocumentObject*> sel;
    std::set<App::DocumentObject*> objSet;
    for(auto &s : Selection().getCompleteSelection()) {
        if(s.pObject && s.pObject->getNameInDocument() && objSet.insert(s.pObject).second)
            sel.push_back(s.pObject);
    }
    if(sel.empty())
        return nullptr;

    auto all = App::Document::getDependencyList(sel);
    if (all.size() > sel.size()) {
        DlgObjectSelection dlg(sel,getMainWindow());
        if(dlg.exec()!=QDialog::Accepted)
            return nullptr;
        sel = dlg.getSelections();
        if(sel.empty())
            return nullptr;
    }

    std::vector<App::Document*> unsaved;
    bool hasXLink = App::PropertyXLink::hasXLink(sel,&unsaved);
    if(!unsaved.empty()) {
        QMessageBox::critical(getMainWindow(), tr("Unsaved document"),
            tr("The exported object contains external link. Please save the document"
                "at least once before exporting."));
        return nullptr;
    }

    unsigned int memsize=1000; // ~ for the meta-information
    for (const auto & it : sel)
        memsize += it->getMemSize();

    // if less than ~10 MB
    bool use_buffer=(memsize < 0xA00000);
    QByteArray res;
    if(use_buffer) {
        try {
            res.reserve(memsize);
        }
        catch (const std::bad_alloc &) {
            use_buffer = false;
        }
    }

    WaitCursor wc;
    QString mime;
    if (use_buffer) {
        mime = hasXLink?_MimeDocObjX:_MimeDocObj;
        Base::ByteArrayOStreambuf buf(res);
        std::ostream str(&buf);
        // need this instance to call MergeDocuments::Save()
        App::Document* doc = sel.front()->getDocument();
        MergeDocuments mimeView(doc);
        doc->exportObjects(sel, str);
    }
    else {
        mime = hasXLink?_MimeDocObjXFile:_MimeDocObjFile;
        static Base::FileInfo fi(App::Application::getTempFileName());
        Base::ofstream str(fi, std::ios::out | std::ios::binary);
        // need this instance to call MergeDocuments::Save()
        App::Document* doc = sel.front()->getDocument();
        MergeDocuments mimeView(doc);
        doc->exportObjects(sel, str);
        str.close();
        res = fi.filePath().c_str();

        // store the path name as a custom property and
        // delete this file when closing the application
        const_cast<MainWindow*>(this)->setProperty("x-documentobject-file", res);
    }

    auto mimeData = new QMimeData();
    mimeData->setData(mime,res);
    return mimeData;
}

bool MainWindow::canInsertFromMimeData (const QMimeData * source) const
{
    if (!source)
        return false;
    return source->hasUrls() ||
        source->hasFormat(_MimeDocObj) || source->hasFormat(_MimeDocObjX) ||
        source->hasFormat(_MimeDocObjFile) || source->hasFormat(_MimeDocObjXFile);
}

void MainWindow::insertFromMimeData (const QMimeData * mimeData)
{
    if (!mimeData)
        return;
    bool fromDoc = false;
    bool hasXLink = false;
    QString format;
    if(mimeData->hasFormat(_MimeDocObj))
        format = _MimeDocObj;
    else if(mimeData->hasFormat(_MimeDocObjX)) {
        format = _MimeDocObjX;
        hasXLink = true;
    }else if(mimeData->hasFormat(_MimeDocObjFile)) {
        format = _MimeDocObjFile;
        fromDoc = true;
    }else if(mimeData->hasFormat(_MimeDocObjXFile)) {
        format = _MimeDocObjXFile;
        fromDoc = true;
        hasXLink = true;
    }else {
        if (mimeData->hasUrls())
            loadUrls(App::GetApplication().getActiveDocument(), mimeData->urls());
        return;
    }

    App::Document* doc = App::GetApplication().getActiveDocument();
    if(!doc) doc = App::GetApplication().newDocument();

    if(hasXLink && !doc->isSaved()) {
        int ret = QMessageBox::question(getMainWindow(), tr("Unsaved document"),
            tr("To link to external objects, the document must be saved at least once.\n"
               "Do you want to save the document now?"),
            QMessageBox::Yes,QMessageBox::No);
        if(ret != QMessageBox::Yes || !Application::Instance->getDocument(doc)->saveAs())
            return;
    }
    if(!fromDoc) {
        QByteArray res = mimeData->data(format);

        doc->openTransaction("Paste");
        Base::ByteArrayIStreambuf buf(res);
        std::istream in(nullptr);
        in.rdbuf(&buf);
        MergeDocuments mimeView(doc);
        std::vector<App::DocumentObject*> newObj = mimeView.importObjects(in);
        auto grp = Gui::Selection().getSelection(doc->getName(), ResolveMode::OldStyleElement, /*single*/true);
        if (grp.size() == 1) {
            Gui::Document* gui = Application::Instance->getDocument(doc);
            if (gui)
                gui->addRootObjectsToGroup(newObj, grp.front().pObject);
        }
        doc->commitTransaction();
    }
    else {
        QByteArray res = mimeData->data(format);

        doc->openTransaction("Paste");
        Base::FileInfo fi((const char*)res);
        Base::ifstream str(fi, std::ios::in | std::ios::binary);
        MergeDocuments mimeView(doc);
        std::vector<App::DocumentObject*> newObj = mimeView.importObjects(str);
        str.close();
        auto grp = Gui::Selection().getSelection(doc->getName(), ResolveMode::OldStyleElement, /*single*/true);
        if (grp.size() == 1) {
            Gui::Document* gui = Application::Instance->getDocument(doc);
            if (gui)
                gui->addRootObjectsToGroup(newObj, grp.front().pObject);
        }
        doc->commitTransaction();
    }
}

void MainWindow::setUrlHandler(const QString &scheme, Gui::UrlHandler* handler)
{
    d->urlHandler[scheme] = handler;
}

void MainWindow::unsetUrlHandler(const QString &scheme)
{
    d->urlHandler.remove(scheme);
}

void MainWindow::loadUrls(App::Document* doc, const QList<QUrl>& urls)
{
    QStringList files;
    for (const auto & it : urls) {
        QMap<QString, QPointer<UrlHandler> >::iterator jt = d->urlHandler.find(it.scheme());
        if (jt != d->urlHandler.end() && !jt->isNull()) {
            // delegate the loading to the url handler
            (*jt)->openUrl(doc, it);
            continue;
        }

        QFileInfo info(it.toLocalFile());
        if (info.exists() && info.isFile()) {
            if (info.isSymLink())
                info.setFile(info.symLinkTarget());
            std::vector<std::string> module = App::GetApplication()
                .getImportModules(info.completeSuffix().toUtf8());
            if (module.empty()) {
                module = App::GetApplication()
                    .getImportModules(info.suffix().toUtf8());
            }
            if (!module.empty()) {
                // ok, we support files with this extension
                files << info.absoluteFilePath();
            }
            else {
                Base::Console().Message("No support to load file '%s'\n",
                    (const char*)info.absoluteFilePath().toUtf8());
            }
        }
        else if (it.scheme().toLower() == QStringLiteral("http")) {
            Gui::Dialog::DownloadManager* dm = Gui::Dialog::DownloadManager::getInstance();
            dm->download(dm->redirectUrl(it));
        }
        else if (it.scheme().toLower() == QStringLiteral("https")) {
            QUrl url = it;
            QUrlQuery urlq(url);
            if (urlq.hasQueryItem(QStringLiteral("sid"))) {
                urlq.removeAllQueryItems(QStringLiteral("sid"));
                url.setQuery(urlq);
                url.setScheme(QStringLiteral("http"));
            }
            Gui::Dialog::DownloadManager* dm = Gui::Dialog::DownloadManager::getInstance();
            dm->download(dm->redirectUrl(url));
        }
        else if (it.scheme().toLower() == QStringLiteral("ftp")) {
            Gui::Dialog::DownloadManager::getInstance()->download(it);
        }
    }

    QByteArray docName = doc ? QByteArray(doc->getName()) : qApp->translate("StdCmdNew","Unnamed").toUtf8();
    SelectModule::Dict dict = SelectModule::importHandler(files);
    // load the files with the associated modules
    for (SelectModule::Dict::iterator it = dict.begin(); it != dict.end(); ++it) {
        // if the passed document name doesn't exist the module should create it, if needed
        Application::Instance->importFrom(it.key().toUtf8(), docName, it.value().toUtf8());
    }
}

void MainWindow::childEvent(QChildEvent *e)
{
    if (e->type() == QEvent::ChildAdded) {
        // The parent maybe set in the object constructor, in which case we
        // can't really tell the exact type of the object (i.e. e->child()). So
        // there is no point using qobject_cast to check for QToolBar here.
        ToolBarManager::checkToolbar();
    }

    QMainWindow::childEvent(e);
}

QString MainWindow::overrideIcons() const
{
    return d->overrideIcons;
}

void MainWindow::setOverrideIcons(const QString &icons)
{
    d->overrideIcons = icons;
    d->hasOverrideIcons = true;
}

QString MainWindow::overrideExtraIcons() const
{
    return d->overrideExtraIcons;
}

void MainWindow::setOverrideExtraIcons(const QString &icons)
{
    d->overrideExtraIcons = icons;
    d->hasOverrideExtraIcons = true;
}

void MainWindowP::applyOverrideIcons(const QStringList &icons)
{
    for (const auto &line : icons) {
        auto s = line.left(line.size()).trimmed();
        if (s.startsWith(QStringLiteral("#")) || s.startsWith(QStringLiteral("//")))
            continue;
        auto pair = s.split(QLatin1Char(','));
        if (pair.isEmpty())
            continue;
        if (pair.size() > 3 ) {
            FC_WARN("Invalid icon override in stylesheet: " << s.toUtf8().constData());
            continue;
        }
        QString path;
        QByteArray name = pair[0].trimmed().toUtf8();
        QPixmap icon;
        if (pair.size() >= 2) {
            path = pair[1].trimmed();
            if (path.size()) {
                QSize size(64, 64);
                if (pair.size() == 3) {
                    auto sz = pair[2].trimmed().split(
                            QLatin1Char('x'),
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
                            QString::KeepEmptyParts, 
#else
                            Qt::KeepEmptyParts, 
#endif
                            Qt::CaseInsensitive);
                    if (sz.size() >= 1) {
                        size.setWidth(sz[0].toInt());
                        if (sz.size() > 1)
                            size.setHeight(sz[1].toInt());
                    }
                }
                QFileInfo fi(path);
                if (fi.exists() && fi.suffix().toLower() == QStringLiteral("svg"))
                    icon = BitmapFactory().pixmapFromSvg(path.toUtf8().constData(), size);
                else
                    BitmapFactory().loadPixmap(path, icon);
                if (icon.isNull())
                    FC_WARN("Failed to load icon for " << name.constData()
                            << " from " << path.toUtf8().constData());
            }
        }
        BitmapFactory().addPixmapToCache(name, icon, path.toUtf8().constData(), true);
    }
}

namespace {
QStringList loadIconSet(std::set<QString> &files,
                        QString content,
                        bool asFileName)
{
    QStringList lines;
    if (asFileName) {
        static QString prefix(QStringLiteral("iconset:"));

        QFileInfo finfo;
        if (QFile::exists(content)) {
            finfo.setFile(content);
        }
        else if (QFile::exists(prefix + content)) {
            finfo.setFile(prefix + content);
        }
        else {
            FC_WARN("Cannot file icon set file " << content.toUtf8().constData());
            return lines;
        }
        if (!files.insert(finfo.canonicalFilePath()).second) {
            FC_WARN("Cyclic icon set import " << content.toUtf8().constData());
            return lines;
        }
        QFile file(finfo.filePath());
        if (!file.open(QFile::ReadOnly | QFile::Text))
            return lines;

        QTextStream str(&file);
        content = str.readAll();
    }

    static QString keyword(QStringLiteral("#import "));
    lines = content.split(QRegExp(QStringLiteral("[\r\n]")),QString::SkipEmptyParts);
    for (auto it = lines.begin(); it != lines.end(); ++it) {
        auto line = it->left(it->size()).trimmed();
        if (!line.startsWith(keyword))
            continue;
        auto importFile = line.right(line.size() - keyword.size()).trimmed();
        for (const auto &s : loadIconSet(files, importFile, /*asFileName*/true))
            it = lines.insert(++it, s);
    }
    return lines;
}

QStringList loadIconSet(QString content, bool asFileName)
{
    std::set<QString> files;
    return loadIconSet(files, content, asFileName);
}
} // anonymous namespace

void MainWindow::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        d->sizeLabel->setText(tr("Dimension"));

        CommandManager& rclMan = Application::Instance->commandManager();
        std::vector<Command*> cmd = rclMan.getAllCommands();
        for (auto & it : cmd)
            it->languageChange();

        // reload current workbench to retranslate all actions and window titles
        Workbench* wb = WorkbenchManager::instance()->active();
        if (wb) wb->retranslate();
    }
    else if (e->type() == QEvent::StyleChange) {
        if (d->hasOverrideExtraIcons) {
            d->hasOverrideExtraIcons = false;
            d->applyOverrideIcons(loadIconSet(d->overrideExtraIcons, /*asFileName*/true));
        } else
            d->overrideExtraIcons.clear();

        if (d->hasOverrideIcons) {
            d->hasOverrideIcons = false;
            d->applyOverrideIcons(loadIconSet(d->overrideIcons, /*asFileName*/false));
        } else
            d->overrideIcons.clear();

        BitmapFactory().onStyleChange();
        Application::Instance->commandManager().refreshIcons();
        OverlayManager::instance()->refreshIcons();
        TipLabel::refreshIcons();
    }
    else if (e->type() == QEvent::ActivationChange) {
        if (isActiveWindow()) {
            QMdiSubWindow* mdi = d->mdiArea->currentSubWindow();
            if (mdi) {
                auto view = dynamic_cast<MDIView*>(mdi->widget());
                if (view && getMainWindow()->activeWindow() != view) {
                    d->activeView = view;
                    Application::Instance->viewActivated(view);
                }
            }
        }
    }
    else {
        QMainWindow::changeEvent(e);
    }
}

void MainWindow::clearStatus() {
    d->currentStatusType = 100;
    statusBar()->setStyleSheet(QStringLiteral("#statusBar{}"));
}

void MainWindow::statusMessageChanged(const QString &msg) {
    if (d->currentStatusMessage != msg) {
        // here probably means the status bar message is changed by QMainWindow
        // internals, e.g. for displaying tooltip and stuff. Set reset what
        // we've changed.
        d->statusTimer->stop();
        d->currentStatusMessage.clear();
        clearStatus();
    }
}

void MainWindow::showMessage(const QString& message, int timeout) {
    if(QApplication::instance()->thread() != QThread::currentThread()) {
        QApplication::postEvent(this, new CustomMessageEvent(MainWindow::Tmp,message,timeout));
        return;
    }
    d->actionLabel->setText(message.simplified());
    if(timeout == 0)
        timeout = ViewParams::getStatusMessageTimeout();
    if(timeout > 0) {
        d->actionTimer->setSingleShot(true);
        d->actionTimer->start(timeout);
    }else
        d->actionTimer->stop();
}

void MainWindow::showStatus(int type, const QString& message)
{
    if(QApplication::instance()->thread() != QThread::currentThread()) {
        QApplication::postEvent(this,
                new CustomMessageEvent(type,message));
        return;
    }

    if(d->currentStatusType < type)
        return;

    d->statusTimer->setSingleShot(true);
    // TODO: hardcode?
    int timeout = 5000;
    d->statusTimer->start(timeout);

    QFontMetrics fm(statusBar()->font());
    QString msg = fm.elidedText(message, Qt::ElideMiddle, this->d->actionLabel->width());
    switch(type) {
    case MainWindow::Err:
        statusBar()->setStyleSheet(d->status->err);
        break;
    case MainWindow::Wrn:
        statusBar()->setStyleSheet(d->status->wrn);
        break;
    case MainWindow::Pane:
        statusBar()->setStyleSheet(QStringLiteral("#statusBar{}"));
        break;
    default:
        statusBar()->setStyleSheet(d->status->msg);
        break;
    }
    d->currentStatusType = type;
    d->currentStatusMessage = msg.simplified();
    statusBar()->showMessage(d->currentStatusMessage, timeout);
}


// set text to the pane
void MainWindow::setPaneText(int i, QString text)
{
    if (i==1) {
        showStatus(MainWindow::Pane, text);
    }
    else if (i==2) {
        d->sizeLabel->setText(text);
    }
}

void MainWindow::customEvent(QEvent* e)
{
    if (e->type() == QEvent::User) {
        auto ce = static_cast<Gui::CustomMessageEvent*>(e);
        QString msg = ce->message();
        switch(ce->type()) {
        case MainWindow::Log: {
            if (msg.startsWith(QStringLiteral("#Inventor V2.1 ascii "))) {
                Gui::Document *d = Application::Instance->activeDocument();
                if (d) {
                    auto view = new ViewProviderExtern();
                    try {
                        view->setModeByString("1",msg.toUtf8().constData());
                        d->setAnnotationViewProvider("Vdbg",view);
                    }
                    catch (...) {
                        delete view;
                    }
                }
            }
            break;
        } case MainWindow::Tmp: {
            showMessage(msg, ce->timeout());
            break;
        } default:
            showStatus(ce->type(),msg);
        }
    }
    else if (e->type() == ActionStyleEvent::EventType) {
        QList<TaskView::TaskView*> tasks = findChildren<TaskView::TaskView*>();
        if (static_cast<ActionStyleEvent*>(e)->getType() == ActionStyleEvent::Clear) {
            for (auto & task : tasks) {
                task->clearActionStyle();
            }
        }
        else {
            for (auto & task : tasks) {
                task->restoreActionStyle();
            }
        }
    }
}

QMdiArea *MainWindow::getMdiArea() const
{
    return d->mdiArea;
}

// ----------------------------------------------------------

StatusBarObserver::StatusBarObserver()
  : WindowParameter("OutputWindow")
{
    msg = QStringLiteral("#statusBar{color: #000000}"); // black
    wrn = QStringLiteral("#statusBar{color: #ffaa00}"); // orange
    err = QStringLiteral("#statusBar{color: #ff0000}"); // red
    Base::Console().AttachObserver(this);
    getWindowParameter()->Attach(this);
    getWindowParameter()->NotifyAll();
}

StatusBarObserver::~StatusBarObserver()
{
    getWindowParameter()->Detach(this);
    Base::Console().DetachObserver(this);
}

void StatusBarObserver::OnChange(Base::Subject<const char*> &rCaller, const char * sReason)
{
    ParameterGrp& rclGrp = ((ParameterGrp&)rCaller);
    auto format = QStringLiteral("#statusBar{color: %1}");
    if (strcmp(sReason, "colorText") == 0) {
        unsigned long col = rclGrp.GetUnsigned( sReason );
        this->msg = format.arg(App::Color::fromPackedRGB<QColor>(col).name());
    }
    else if (strcmp(sReason, "colorWarning") == 0) {
        unsigned long col = rclGrp.GetUnsigned( sReason );
        this->wrn = format.arg(App::Color::fromPackedRGB<QColor>(col).name());
    }
    else if (strcmp(sReason, "colorError") == 0) {
        unsigned long col = rclGrp.GetUnsigned( sReason );
        this->err = format.arg(App::Color::fromPackedRGB<QColor>(col).name());
    }
    else if (strcmp(sReason, "colorCritical") == 0) {
        unsigned long col = rclGrp.GetUnsigned( sReason );
        this->critical = format.arg(QColor((col >> 24) & 0xff,(col >> 16) & 0xff,(col >> 8) & 0xff).name());
    }
}

void StatusBarObserver::SendLog(const std::string& notifiername, const std::string& msg, Base::LogStyle level)
{
    (void) notifiername;

    int messageType = -1;
    switch(level){
        case Base::LogStyle::Warning:
            messageType = MainWindow::Wrn;
            break;
        case Base::LogStyle::Message:
            messageType = MainWindow::Msg;
            break;
        case Base::LogStyle::Error:
            messageType = MainWindow::Err;
            break;
        case Base::LogStyle::Log:
            messageType = MainWindow::Log;
            break;
        case Base::LogStyle::Critical:
            messageType = MainWindow::Critical;
            break;
        default:
            break;
    }

    // Send the event to the main window to allow thread-safety. Qt will delete it when done.
    auto ev = new CustomMessageEvent(messageType, QString::fromUtf8(msg.c_str()));
    QApplication::postEvent(getMainWindow(), ev);
}

// -------------------------------------------------------------

int ActionStyleEvent::EventType = -1;

ActionStyleEvent::ActionStyleEvent(Style type)
  : QEvent(QEvent::Type(EventType)), type(type)
{
}

ActionStyleEvent::Style ActionStyleEvent::getType() const
{
    return type;
}


#include "moc_MainWindow.cpp"
#include "MainWindow.moc"
