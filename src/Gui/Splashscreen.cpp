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
# include <cstdlib>
# include <QApplication>
# include <QClipboard>
# include <QFile>
# include <QDesktopWidget>
# include <QLocale>
# include <QMutex>
# include <QProcessEnvironment>
# include <QRegularExpression>
# include <QRegularExpressionMatch>
# include <QScreen>
# include <QSysInfo>
# include <QTextBrowser>
# include <QTextStream>
# include <QWaitCondition>
# include <QLabel>
# include <QFileInfo>
# include <QDir>
# include <Inventor/C/basic.h>
#endif

#include <QMovie>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string.hpp>

#include <App/Application.h>
#include <App/Metadata.h>
#include <Base/Console.h>
#include <CXX/WrapPython.h>

#include <boost/filesystem.hpp>
#include <LibraryVersions.h>
#include <zlib.h>

#include "Splashscreen.h"
#include "ui_AboutApplication.h"
#include "MainWindow.h"


using namespace Gui;
using namespace Gui::Dialog;
namespace fs = boost::filesystem;

static inline int parseAlignment(const std::string &text)
{
    std::vector<std::string> alignment_keys;
    boost::split(alignment_keys, text, boost::is_any_of("|"));

    int align=0;
    for (const auto& align_key : alignment_keys) {
        if (boost::starts_with(align_key, "VCenter"))
            align |= Qt::AlignVCenter;
        else if (boost::starts_with(align_key, "Top"))
            align |= Qt::AlignTop;
        else if (boost::starts_with(align_key, "Bottom"))
            align |= Qt::AlignBottom;

        if (boost::starts_with(align_key, "HCenter"))
            align |= Qt::AlignHCenter;
        else if (boost::starts_with(align_key, "Right"))
            align |= Qt::AlignRight;
        else if (boost::starts_with(align_key, "Left"))
            align |= Qt::AlignLeft;
    }
    return align;
}

namespace Gui {
/** Displays all messages at startup inside the splash screen.
 * \author Werner Mayer
 */
class SplashObserver : public Base::ILogger
{
public:
    SplashObserver(const SplashObserver&) = delete;
    SplashObserver(SplashObserver&&) = delete;
    SplashObserver& operator= (const SplashObserver&) = delete;
    SplashObserver& operator= (SplashObserver&&) = delete;

    explicit SplashObserver(QSplashScreen* splasher=nullptr)
      : splash(splasher)
      , alignment(Qt::AlignBottom|Qt::AlignLeft)
      , textColor(Qt::black)
    {
        Base::Console().AttachObserver(this);

        // allow to customize text position and color
        const std::map<std::string,std::string>& cfg = App::Application::Config();
        auto al = cfg.find("SplashAlignment");
        if (al != cfg.end()) {
            if (int align = parseAlignment(al->second))
                alignment = align;
        }

        // choose text color
        auto tc = cfg.find("SplashTextColor");
        if (tc != cfg.end()) {
            QColor col; col.setNamedColor(QString::fromUtf8(tc->second.c_str()));
            if (col.isValid()) {
                textColor = col;
            }
        }
    }
    ~SplashObserver() override
    {
        Base::Console().DetachObserver(this);
    }
    const char* Name() override
    {
        return "SplashObserver";
    }
    void SendLog(const std::string& msg, Base::LogStyle level) override
    {
#ifdef FC_DEBUG
        Log(msg.c_str());
        Q_UNUSED(level)
#else
        if (level == Base::LogStyle::Log) {
            Log(msg.c_str());
        }
#endif
    }
    void Log (const char * text)
    {
        QString msg(QString::fromUtf8(text));
        QRegularExpression rx;
        // ignore 'Init:' and 'Mod:' prefixes
        rx.setPattern(QStringLiteral("^\\s*(Init:|Mod:)\\s*"));
        auto match = rx.match(msg);
        if (match.hasMatch()) {
            msg = msg.mid(match.capturedLength());
        }
        else {
            // ignore activation of commands
            rx.setPattern(QStringLiteral("^\\s*(\\+App::|Create|CmdC:|CmdG:|Act:)\\s*"));
            match = rx.match(msg);
            if (match.hasMatch() && match.capturedStart() == 0)
                return;
        }

        // Strip out initial newlines.
        msg = msg.replace(QStringLiteral("\n"), QString());
        // Add one to buffer from bottom alignment, and shift to right.
        msg.prepend(QStringLiteral("     "));
        msg.append(QStringLiteral("\n"));

        splash->showMessage(msg, alignment, textColor);
        QMutex mutex;
        QMutexLocker ml(&mutex);
        QWaitCondition().wait(&mutex, 50);
    }

private:
    QSplashScreen* splash;
    int alignment;
    QColor textColor;
};
} // namespace Gui

// ------------------------------------------------------------------------------

namespace {

class GifLabel: public QLabel
{
public:
    GifLabel(QWidget *parent)
        :QLabel(parent)
    {
        parent->installEventFilter(this);
    }

    bool eventFilter(QObject *o, QEvent *e)
    {
        if (e->type() == QEvent::Resize) {
            auto parentSize = static_cast<QResizeEvent*>(e)->size();
            if (auto label = qobject_cast<QLabel*>(o)) {
                QSize imageSize;
                if (auto pixmap = label->pixmap())
                    imageSize = pixmap->size();
                else if (auto movie = label->movie())
                    imageSize = movie->frameRect().size();
                if (!label->hasScaledContents()
                        && imageSize.height()
                        && imageSize.width()
                        && parentSize.height())
                {
                    double ar = double(imageSize.width()) / imageSize.height();
                    double arParent = double(parentSize.width()) / parentSize.height();
                    if (arParent > ar) {
                        imageSize.setWidth(imageSize.width() * parentSize.height() / imageSize.height());
                        imageSize.setHeight(parentSize.height());
                    } else {
                        imageSize.setHeight(imageSize.height() * parentSize.width() / imageSize.width());
                        imageSize.setWidth(parentSize.width());
                    }

                    QPoint pos;
                    auto align = label->alignment();
                    if (align & Qt::AlignRight)
                        pos.setX(parentSize.width() - imageSize.width());
                    else if (align & Qt::AlignHCenter)
                        pos.setX((parentSize.width() - imageSize.width())/2);
                    if (align & Qt::AlignBottom)
                        pos.setY(parentSize.height() - imageSize.height());
                    else if (align & Qt::AlignVCenter)
                        pos.setY((parentSize.height() - imageSize.height())/2);
                    move(pos);
                    parentSize = imageSize;
                }
            }
            resize(parentSize);
        }
        return QLabel::eventFilter(o, e);
    }
};

QLabel *loadSplashGif(QWidget *parent, const char *key, const char *alignmentKey)
{
    const auto& cfg = App::GetApplication().Config();
    auto itGif = cfg.find(key);
    if (itGif == cfg.end())
        return nullptr;
    QString path = QString::fromUtf8(itGif->second.c_str());
    if (QDir(path).isRelative()) {
        QString home = QString::fromStdString(App::GetApplication().getHomePath());
        path = QFileInfo(QDir(home), path).absoluteFilePath();
    }
    auto movie =  new QMovie(parent);
    movie->setFileName(path);
    if (!movie->isValid()) {
        delete movie;
        return nullptr;
    }
    auto gifLabel = new GifLabel(parent);
    gifLabel->setMovie(movie);
    movie->start();
    auto itAlignment = cfg.find(alignmentKey);
    int align = 0;
    if (itAlignment != cfg.end())
        align = parseAlignment(itAlignment->second);
    if (align)
        gifLabel->setAlignment(Qt::AlignmentFlag(align));
    else
        gifLabel->setAlignment(Qt::AlignTop | Qt::AlignRight);
    gifLabel->setAttribute(Qt::WA_TranslucentBackground, true);
    return gifLabel;
}
} // anonymous namespace

/**
 * Constructs a splash screen that will display the pixmap.
 */
SplashScreen::SplashScreen(  const QPixmap & pixmap , Qt::WindowFlags f )
    : QSplashScreen(pixmap, f)
{
    // write the messages to splasher
    messages = new SplashObserver(this);
    loadSplashGif(this, "SplashGif", "SplashGifAlignment");
}

/** Destruction. */
SplashScreen::~SplashScreen()
{
    delete messages;
}

/**
 * Draws the contents of the splash screen using painter \a painter. The default
 * implementation draws the message passed by message().
 */
void SplashScreen::drawContents ( QPainter * painter )
{
    QSplashScreen::drawContents(painter);
}

// ------------------------------------------------------------------------------

AboutDialogFactory* AboutDialogFactory::factory = nullptr;

AboutDialogFactory::~AboutDialogFactory()
{
}

QDialog *AboutDialogFactory::create(QWidget *parent) const
{
#ifdef _USE_3DCONNEXION_SDK
    return new AboutDialog(true, parent);
#else
    return new AboutDialog(false, parent);
#endif
}

const AboutDialogFactory *AboutDialogFactory::defaultFactory()
{
    static const AboutDialogFactory this_factory;
    if (factory)
        return factory;
    return &this_factory;
}

void AboutDialogFactory::setDefaultFactory(AboutDialogFactory *f)
{
    if (factory != f)
        delete factory;
    factory = f;
}

// ------------------------------------------------------------------------------

/* TRANSLATOR Gui::Dialog::AboutDialog */

/**
 *  Constructs an AboutDialog which is a child of 'parent', with the
 *  name 'name' and widget flags set to 'WStyle_Customize|WStyle_NoBorder|WType_Modal'
 *
 *  The dialog will be modal.
 */
AboutDialog::AboutDialog(bool showLic, QWidget* parent)
  : QDialog(parent), ui(new Ui_AboutApplication)
{
    Q_UNUSED(showLic);

    setModal(true);
    ui->setupUi(this);
    // remove the automatic help button in dialog title since we don't use it
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);

    layout()->setSizeConstraint(QLayout::SetFixedSize);
    QRect rect = QApplication::desktop()->availableGeometry(getMainWindow());

    loadSplashGif(ui->labelSplashPicture, "SplashGif", "SplashGifAlignment");

    // See if we have a custom About screen image set
    QPixmap image = getMainWindow()->aboutImage();

    // Fallback to the splashscreen image
    if (image.isNull()) {
        image = getMainWindow()->splashImage();
    }

    // Make sure the image is not too big
    int denom = 2;
    if (image.height() > rect.height()/denom || image.width() > rect.width()/denom) {
        float scale = static_cast<float>(image.width()) / static_cast<float>(image.height());
        int width = std::min(image.width(), rect.width()/denom);
        int height = std::min(image.height(), rect.height()/denom);
        height = std::min(height, static_cast<int>(width / scale));
        width = static_cast<int>(scale * height);

        image = image.scaled(width, height,
                    Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }
    ui->labelSplashPicture->setPixmap(image);
    ui->tabWidget->setCurrentIndex(0); // always start on the About tab

    setupLabels();
    showCredits();
    showLicenseInformation();
    showLibraryInformation();
    showCollectionInformation();
    showOrHideImage(rect);
}

/**
 *  Destroys the object and frees any allocated resources
 */
AboutDialog::~AboutDialog()
{
    // no need to delete child widgets, Qt does it all for us
    delete ui;
}

void AboutDialog::showOrHideImage(const QRect& rect)
{
    adjustSize();
    if (height() > rect.height()) {
        ui->labelSplashPicture->hide();
    }
}

void AboutDialog::setupLabels()
{
    //fonts are rendered smaller on Mac so point size can't be the same for all platforms
    int fontSize = 8;
#ifdef Q_OS_MAC
    fontSize = 11;
#endif
    //avoid overriding user set style sheet
    if (qApp->styleSheet().isEmpty()) {
        setStyleSheet(QStringLiteral("Gui--Dialog--AboutDialog QLabel {font-size: %1pt;}").arg(fontSize));
    }

    QString exeName = qApp->applicationName();
    std::map<std::string, std::string>& config = App::Application::Config();
    std::map<std::string,std::string>::iterator it;
    QString banner  = QString::fromUtf8(config["CopyrightInfo"].c_str());
    banner = banner.left( banner.indexOf(QLatin1Char('\n')) );
    QString major  = QString::fromUtf8(config["BuildVersionMajor"].c_str());
    QString minor  = QString::fromUtf8(config["BuildVersionMinor"].c_str());
    QString point  = QString::fromUtf8(config["BuildVersionPoint"].c_str());
    QString build  = QString::fromUtf8(config["BuildRevision"].c_str());
    QString disda  = QString::fromUtf8(config["BuildRevisionDate"].c_str());
    QString mturl  = QString::fromUtf8(config["MaintainerUrl"].c_str());

    // we use replace() to keep label formatting, so a label with text "<b>Unknown</b>"
    // gets replaced to "<b>FreeCAD</b>", for example

    QString author = ui->labelAuthor->text();
    author.replace(QStringLiteral("Unknown Application"), exeName);
    author.replace(QStringLiteral("(c) Unknown Author"), banner);
    ui->labelAuthor->setText(author);
    ui->labelAuthor->setUrl(mturl);

    if (qApp->styleSheet().isEmpty()) {
        ui->labelAuthor->setStyleSheet(QStringLiteral("Gui--UrlLabel {color: #0000FF;text-decoration: underline;font-weight: 600;}"));
    }

    QString version = ui->labelBuildVersion->text();
    version.replace(QStringLiteral("Unknown"), QStringLiteral("%1.%2.%3").arg(major, minor, point));
    ui->labelBuildVersion->setText(version);

    QString revision = ui->labelBuildRevision->text();
    revision.replace(QStringLiteral("Unknown"), build);
    ui->labelBuildRevision->setText(revision);

    QString date = ui->labelBuildDate->text();
    date.replace(QStringLiteral("Unknown"), disda);
    ui->labelBuildDate->setText(date);

    QString os = ui->labelBuildOS->text();
    os.replace(QStringLiteral("Unknown"), QSysInfo::prettyProductName());
    ui->labelBuildOS->setText(os);

    QString platform = ui->labelBuildPlatform->text();
    platform.replace(QStringLiteral("Unknown"),
        QStringLiteral("%1-bit").arg(QSysInfo::WordSize));
    ui->labelBuildPlatform->setText(platform);

    // branch name
    it = config.find("BuildRevisionBranch");
    if (it != config.end()) {
        QString branch = ui->labelBuildBranch->text();
        branch.replace(QStringLiteral("Unknown"), QString::fromUtf8(it->second.c_str()));
        ui->labelBuildBranch->setText(branch);
    }
    else {
        ui->labelBranch->hide();
        ui->labelBuildBranch->hide();
    }

    // hash id
    it = config.find("BuildRevisionHash");
    if (it != config.end()) {
        QString hash = ui->labelBuildHash->text();
        hash.replace(QStringLiteral("Unknown"), QString::fromUtf8(it->second.c_str()).left(7)); // Use the 7-char abbreviated hash
        ui->labelBuildHash->setText(hash);
        if (auto url_itr = config.find("BuildRepositoryURL"); url_itr != config.end()) {
            auto url = QString::fromStdString(url_itr->second);

            if (int space = url.indexOf(QChar::fromLatin1(' ')); space != -1)
                url = url.left(space); // Strip off the branch information to get just the repo

            if (url == QString::fromUtf8("Unknown"))
                url = QString::fromUtf8("https://github.com/FreeCAD/FreeCAD"); // Just take a guess

            // This may only create valid URLs for Github, but some other hosts use the same format so give it a shot...
            auto https = url.replace(QString::fromUtf8("git://"), QString::fromUtf8("https://"));
            https.replace(QString::fromUtf8(".git"), QString::fromUtf8(""));
            ui->labelBuildHash->setUrl(https + QString::fromUtf8("/commit/") + QString::fromStdString(it->second));
        }
    }
    else {
        ui->labelHash->hide();
        ui->labelBuildHash->hide();
    }
}

class AboutDialog::LibraryInfo {
public:
    QString name;
    QString href;
    QString url;
    QString version;
};

void AboutDialog::showCredits()
{
    auto creditsFileURL = QStringLiteral(":/doc/CONTRIBUTORS");
    QFile creditsFile(creditsFileURL);

    if (!creditsFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }

    auto tab_credits = new QWidget();
    tab_credits->setObjectName(QStringLiteral("tab_credits"));
    ui->tabWidget->addTab(tab_credits, tr("Credits"));
    auto hlayout = new QVBoxLayout(tab_credits);
    auto textField = new QTextBrowser(tab_credits);
    textField->setOpenExternalLinks(false);
    textField->setOpenLinks(false);
    hlayout->addWidget(textField);

    QString creditsHTML = QStringLiteral("<html><body><h1>");
    //: Header for the Credits tab of the About screen
    creditsHTML += tr("Credits");
    creditsHTML += QStringLiteral("</h1><p>");
    creditsHTML += tr("FreeCAD would not be possible without the contributions of");
    creditsHTML += QStringLiteral(":</p><h2>"); 
    //: Header for the list of individual people in the Credits list.
    creditsHTML += tr("Individuals");
    creditsHTML += QStringLiteral("</h2><ul>");

    QTextStream stream(&creditsFile);
#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
    stream.setCodec("UTF-8");
#endif
    QString line;
    while (stream.readLineInto(&line)) {
        if (!line.isEmpty()) {
            if (line == QStringLiteral("Firms")) {
                creditsHTML += QStringLiteral("</ul><h2>");
                //: Header for the list of companies/organizations in the Credits list.
                creditsHTML += tr("Organizations");
                creditsHTML += QStringLiteral("</h2><ul>");
            }
            else {
                creditsHTML += QStringLiteral("<li>") + line + QStringLiteral("</li>");
            }
        }
    }
    creditsHTML += QStringLiteral("</ul></body></html>");
    textField->setHtml(creditsHTML);
}

void AboutDialog::showLicenseInformation()
{
    QString licenseFileURL = QStringLiteral("%1/LICENSE.html")
        .arg(QString::fromUtf8(App::Application::getHelpDir().c_str()));
    QFile licenseFile(licenseFileURL);

    if (licenseFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString licenseHTML = QString::fromUtf8(licenseFile.readAll());
        const auto placeholder = QString::fromUtf8("<!--PLACEHOLDER_FOR_ADDITIONAL_LICENSE_INFORMATION-->");
        licenseHTML.replace(placeholder, getAdditionalLicenseInformation());

        ui->tabWidget->removeTab(1); // Hide the license placeholder widget

        auto tab_license = new QWidget();
        tab_license->setObjectName(QStringLiteral("tab_license"));
        ui->tabWidget->addTab(tab_license, tr("License"));
        auto hlayout = new QVBoxLayout(tab_license);
        auto textField = new QTextBrowser(tab_license);
        textField->setOpenExternalLinks(true);
        textField->setOpenLinks(true);
        hlayout->addWidget(textField);

        textField->setHtml(licenseHTML);
    }
    else {
        QString info(QStringLiteral("SUCH DAMAGES.<hr/>"));
        info += getAdditionalLicenseInformation();
        QString lictext = ui->textBrowserLicense->toHtml();
        lictext.replace(QStringLiteral("SUCH DAMAGES.<hr/>"), info);
        ui->textBrowserLicense->setHtml(lictext);
    }
}

QString AboutDialog::getAdditionalLicenseInformation() const
{
    // Any additional piece of text to be added after the main license text goes below.
    // Please set title in <h2> tags, license text in <p> tags
    // and add an <hr/> tag at the end to nicely separate license blocks
    QString info;
#ifdef _USE_3DCONNEXION_SDK
    info += QString::fromUtf8(
        "<h2>3D Mouse Support</h2>"
        "<p>Development tools and related technology provided under license from 3Dconnexion.<br/>"
        "Copyright &#169; 1992&ndash;2012 3Dconnexion. All rights reserved.</p>"
        "<hr/>"
    );
#endif
    return info;
}

void AboutDialog::showLibraryInformation()
{
    auto tab_library = new QWidget();
    tab_library->setObjectName(QStringLiteral("tab_library"));
    ui->tabWidget->addTab(tab_library, tr("Libraries"));
    auto hlayout = new QVBoxLayout(tab_library);
    auto textField = new QTextBrowser(tab_library);
    textField->setOpenExternalLinks(false);
    textField->setOpenLinks(false);
    hlayout->addWidget(textField);

    QList<LibraryInfo> libInfo;
    QString baseurl = QString::fromUtf8("file:///%1/ThirdPartyLibraries.html")
            .arg(QString::fromUtf8(App::Application::getHelpDir().c_str()));

    // Boost
    libInfo << LibraryInfo {
        QStringLiteral("Boost"),
        baseurl + QStringLiteral("#_TocBoost"),
        QStringLiteral("https://www.boost.org"),
        QStringLiteral(BOOST_LIB_VERSION)
    };

    // Coin3D
    libInfo << LibraryInfo {
        QStringLiteral("Coin3D"),
        baseurl + QStringLiteral("#_TocCoin3D"),
        QStringLiteral("https://coin3d.github.io"),
        QStringLiteral(COIN_VERSION)
    };

    // Eigen3
    libInfo << LibraryInfo {
        QStringLiteral("Eigen"),
        baseurl + QStringLiteral("#_TocEigen"),
        QStringLiteral("https://eigen.tuxfamily.org"),
        QString::fromUtf8(fcEigen3Version)
    };

    // FreeType
    libInfo << LibraryInfo {
        QStringLiteral("FreeType"),
        baseurl + QStringLiteral("#_TocFreeType"),
        QStringLiteral("https://freetype.org"),
        QString::fromUtf8(fcFreetypeVersion)
    };

    // KDL
    libInfo << LibraryInfo {
        QStringLiteral("KDL"),
        baseurl + QStringLiteral("#_TocKDL"),
        QStringLiteral("https://www.orocos.org/kdl"),
        QString()
    };

    // libarea
    libInfo << LibraryInfo {
        QStringLiteral("libarea"),
        baseurl + QStringLiteral("#_TocLibArea"),
        QStringLiteral("https://github.com/danielfalck/libarea"),
        QString()
    };

    // OCCT
#if defined(HAVE_OCC_VERSION)
    libInfo << LibraryInfo {
        QStringLiteral("Open CASCADE Technology"),
        baseurl + QStringLiteral("#_TocOCCT"),
        QStringLiteral("https://www.opencascade.com/open-cascade-technology/"),
        QStringLiteral(OCC_VERSION_STRING_EXT)
    };
#endif

    // pcl
    libInfo << LibraryInfo {
        QStringLiteral("Point Cloud Library"),
        baseurl + QStringLiteral("#_TocPcl"),
        QStringLiteral("https://www.pointclouds.org"),
        QString::fromUtf8(fcPclVersion)
    };

    // PyCXX
    libInfo << LibraryInfo {
        QStringLiteral("PyCXX"),
        baseurl + QStringLiteral("#_TocPyCXX"),
        QStringLiteral("http://cxx.sourceforge.net"),
        QString::fromUtf8(fcPycxxVersion)
    };

    // Python
    libInfo << LibraryInfo {
        QStringLiteral("Python"),
        baseurl + QStringLiteral("#_TocPython"),
        QStringLiteral("https://www.python.org"),
        QStringLiteral(PY_VERSION)
    };

    // PySide
    libInfo << LibraryInfo {
        QStringLiteral("Qt for Python (PySide)"),
        baseurl + QStringLiteral("#_TocPySide"),
        QStringLiteral("https://wiki.qt.io/Qt_for_Python"),
        QString::fromUtf8(fcPysideVersion)
    };

    // Qt
    libInfo << LibraryInfo {
        QStringLiteral("Qt"),
        baseurl + QStringLiteral("#_TocQt"),
        QStringLiteral("https://www.qt.io"),
        QStringLiteral(QT_VERSION_STR)
    };

    // Salome SMESH
    libInfo << LibraryInfo {
        QStringLiteral("Salome SMESH"),
        baseurl + QStringLiteral("#_TocSalomeSMESH"),
        QStringLiteral("https://salome-platform.org"),
#ifdef SMESH_VERSION_STR
        QStringLiteral(SMESH_VERSION_STR)
#else
        QString()
#endif
    };

    // Shiboken
    libInfo << LibraryInfo {
        QStringLiteral("Qt for Python (Shiboken)"),
        baseurl + QStringLiteral("#_TocPySide"),
        QStringLiteral("https://wiki.qt.io/Qt_for_Python"),
        QString::fromUtf8(fcShibokenVersion)
    };

    // vtk
    libInfo << LibraryInfo {
        QStringLiteral("vtk"),
        baseurl + QStringLiteral("#_TocVtk"),
        QStringLiteral("https://www.vtk.org"),
        QString::fromUtf8(fcVtkVersion)
    };

    // Xerces-C
    libInfo << LibraryInfo {
        QStringLiteral("Xerces-C"),
        baseurl + QStringLiteral("#_TocXercesC"),
        QStringLiteral("https://xerces.apache.org/xerces-c"),
        QString::fromUtf8(fcXercescVersion)
    };

    // Zipios++
    libInfo << LibraryInfo {
        QStringLiteral("Zipios++"),
        baseurl + QStringLiteral("#_TocZipios"),
        QStringLiteral("http://zipios.sourceforge.net"),
        QString()
    };

    // zlib
    libInfo << LibraryInfo {
        QStringLiteral("zlib"),
        baseurl + QStringLiteral("#_TocZlib"),
        QStringLiteral("https://zlib.net"),
        QStringLiteral(ZLIB_VERSION)
    };


    QString msg = tr("This software uses open source components whose copyright and other "
                     "proprietary rights belong to their respective owners:");
    QString html;
    QTextStream out(&html);
    out << "<html><head/><body style=\" font-size:8.25pt; font-weight:400; font-style:normal;\">"
        << "<p>" << msg << "<br/></p>\n<ul>\n";
    for (QList<LibraryInfo>::iterator it = libInfo.begin(); it != libInfo.end(); ++it) {
        out << "<li><p>" << it->name << " " << it->version << "</p>"
               "<p><a href=\"" << it->href << "\">" << it->url
            << "</a><br/></p></li>\n";
    }
    out << "</ul>\n</body>\n</html>";
    textField->setHtml(html);

    connect(textField, &QTextBrowser::anchorClicked, this, &AboutDialog::linkActivated);
}

void AboutDialog::showCollectionInformation()
{
    QString doc = QString::fromUtf8(App::Application::getHelpDir().c_str());
    QString path = doc + QStringLiteral("Collection.html");
    if (!QFile::exists(path))
        return;

    auto tab_collection = new QWidget();
    tab_collection->setObjectName(QStringLiteral("tab_collection"));
    ui->tabWidget->addTab(tab_collection, tr("Collection"));
    auto hlayout = new QVBoxLayout(tab_collection);
    auto textField = new QTextBrowser(tab_collection);
    textField->setOpenExternalLinks(true);
    hlayout->addWidget(textField);
    textField->setSource(path);
}

void AboutDialog::linkActivated(const QUrl& link)
{
    auto licenseView = new LicenseView();
    licenseView->setAttribute(Qt::WA_DeleteOnClose);
    licenseView->show();
    QString title = tr("License");
    QString fragment = link.fragment();
    if (fragment.startsWith(QStringLiteral("_Toc"))) {
        QString prefix = fragment.mid(4);
        title = QStringLiteral("%1 %2").arg(prefix, title);
    }
    licenseView->setWindowTitle(title);
    getMainWindow()->addWindow(licenseView);
    licenseView->setSource(link);
}

void AboutDialog::on_copyButton_clicked()
{
    QString data;
    QTextStream str(&data);
    std::map<std::string, std::string>& config = App::Application::Config();
    std::map<std::string,std::string>::iterator it;
    QString exe = QString::fromStdString(App::Application::getExecutableName());

    QString major  = QString::fromUtf8(config["BuildVersionMajor"].c_str());
    QString minor  = QString::fromUtf8(config["BuildVersionMinor"].c_str());
    QString point  = QString::fromUtf8(config["BuildVersionPoint"].c_str());
    QString build  = QString::fromUtf8(config["BuildRevision"].c_str());

    QString deskEnv = QProcessEnvironment::systemEnvironment().value(QStringLiteral("XDG_CURRENT_DESKTOP"),QStringLiteral(""));
    QString deskSess = QProcessEnvironment::systemEnvironment().value(QStringLiteral("DESKTOP_SESSION"),QStringLiteral(""));
    QString deskInfo = QStringLiteral("");

    if (!(deskEnv == QStringLiteral("") && deskSess == QStringLiteral(""))) {
        if (deskEnv == QStringLiteral("") || deskSess == QStringLiteral("")) {
            deskInfo = QStringLiteral(" (") + deskEnv + deskSess + QStringLiteral(")");

        } else {
            deskInfo = QStringLiteral(" (") + deskEnv + QStringLiteral("/") + deskSess + QStringLiteral(")");
        }
    }

    str << "[code]\n";
    str << "OS: " << QSysInfo::prettyProductName() << deskInfo << '\n';
    str << "Word size of " << exe << ": " << QSysInfo::WordSize << "-bit\n";
    str << "Version: " << major << "." << minor << "." << point << "." << build;
    char *appimage = getenv("APPIMAGE");
    if (appimage)
        str << " AppImage";
    char* snap = getenv("SNAP_REVISION");
    if (snap)
        str << " Snap " << snap;
    str << '\n';

#if defined(_DEBUG) || defined(DEBUG)
    str << "Build type: Debug\n";
#elif defined(NDEBUG)
    str << "Build type: Release\n";
#elif defined(CMAKE_BUILD_TYPE)
    str << "Build type: " << CMAKE_BUILD_TYPE << '\n';
#else
    str << "Build type: Unknown\n";
#endif
    it = config.find("BuildRevisionBranch");
    if (it != config.end())
        str << "Branch: " << QString::fromUtf8(it->second.c_str()) << '\n';
    it = config.find("BuildRevisionHash");
    if (it != config.end())
        str << "Hash: " << it->second.c_str() << '\n';
    // report also the version numbers of the most important libraries in FreeCAD
    str << "Python " << PY_VERSION << ", ";
    str << "Qt " << QT_VERSION_STR << ", ";
    str << "Coin " << COIN_VERSION << ", ";
    str << "Vtk " << fcVtkVersion << ", ";
#if defined(HAVE_OCC_VERSION)
    str << "OCC "
        << OCC_VERSION_MAJOR << "."
        << OCC_VERSION_MINOR << "."
        << OCC_VERSION_MAINTENANCE
#ifdef OCC_VERSION_DEVELOPMENT
        << "." OCC_VERSION_DEVELOPMENT
#endif
        << '\n';
#endif
    QLocale loc;
    str << "Locale: " << loc.languageToString(loc.language()) << "/"
        << loc.countryToString(loc.country())
        << " (" << loc.name() << ")";
    if (loc != QLocale::system()) {
        loc = QLocale::system();
        str << " [ OS: " << loc.languageToString(loc.language()) << "/"
            << loc.countryToString(loc.country())
            << " (" << loc.name() << ") ]";
    }
    str << "\n";

    // Add installed module information:
    auto modDir = fs::path(App::Application::getUserAppDataDir()) / "Mod";
    bool firstMod = true;
    if (fs::exists(modDir) && fs::is_directory(modDir)) {
        for (const auto& mod : fs::directory_iterator(modDir)) {
            auto dirName = mod.path().leaf().string();
            if (dirName[0] == '.') // Ignore dot directories
                continue;
            if (firstMod) {
                firstMod = false;
                str << "Installed mods: \n";
            }
            str << "  * " << QString::fromStdString(mod.path().leaf().string());
            auto metadataFile = mod.path() / "package.xml";
            if (fs::exists(metadataFile)) {
                App::Metadata metadata(metadataFile);
                if (metadata.version() != App::Meta::Version())
                    str << QStringLiteral(" ") + QString::fromStdString(metadata.version().str());
            }
            auto disablingFile = mod.path() / "ADDON_DISABLED";
            if (fs::exists(disablingFile))
                str << " (Disabled)";
            
            str << "\n";
        }
    }

    str << "[/code]\n";
    QClipboard* cb = QApplication::clipboard();
    cb->setText(data);
}

// ----------------------------------------------------------------------------

/* TRANSLATOR Gui::LicenseView */

LicenseView::LicenseView(QWidget* parent)
    : MDIView(nullptr,parent,Qt::WindowFlags())
{
    browser = new QTextBrowser(this);
    browser->setOpenExternalLinks(true);
    browser->setOpenLinks(true);
    setCentralWidget(browser);
}

LicenseView::~LicenseView()
{
}

void LicenseView::setSource(const QUrl& url)
{
    browser->setSource(url);
}

#include "moc_Splashscreen.cpp"
