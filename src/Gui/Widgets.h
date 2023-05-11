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


#ifndef GUI_WIDGETS_H
#define GUI_WIDGETS_H

#include <memory>
#include <FCGlobal.h>

#include <QBasicTimer>
#include <QButtonGroup>
#include <QDialog>
#include <QElapsedTimer>
#include <QLabel>
#include <QLineEdit>
#include <QModelIndex>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QPointer>
#include <QProxyStyle>
#include <QPushButton>
#include <QTimer>
#include <QToolButton>


#include <Base/Parameter.h>
#include "ExpressionBinding.h"

class QGridLayout;
class QVBoxLayout;
class QTreeWidget;
class QTreeWidgetItem;
class QSpacerItem;

class QTextEdit;
class QHelpEvent;

namespace Gui {
class PrefCheckBox;
class CommandViewItemPrivate;

/**
 * This class allows to drag one or more items which correspond to a Command object.
 * The dragged items can be dropped onto the @ref Gui::PythonConsole.
 * @see CommandViewItem, Command
 * @author Werner Mayer
 */
class CommandIconView : public QListWidget
{
  Q_OBJECT

public:
  CommandIconView (QWidget * parent = nullptr);
  ~CommandIconView () override;

protected:
  void startDrag ( Qt::DropActions supportedActions ) override;

protected Q_SLOTS:
  void onSelectionChanged( QListWidgetItem * item, QListWidgetItem * );

Q_SIGNALS:
  /** Emits this signal if selection has changed. */
  void emitSelectionChanged( const QString& );
};

// ------------------------------------------------------------------------------

class GuiExport ActionSelector : public QWidget
{
    Q_OBJECT

public:
    ActionSelector(QWidget* parent=nullptr);
    ~ActionSelector() override;

    QTreeWidget* availableTreeWidget() const
    { return availableWidget; }
    QTreeWidget* selectedTreeWidget() const
    { return selectedWidget; }
    void setSelectedLabel(const QString&);
    QString selectedLabel() const;
    void setAvailableLabel(const QString&);
    QString availableLabel() const;

private:
    void keyPressEvent(QKeyEvent *) override;
    void changeEvent(QEvent*) override;
    void retranslateUi();
    void setButtonsEnabled();

private Q_SLOTS:
    void onAddButtonClicked();
    void onRemoveButtonClicked();
    void onUpButtonClicked();
    void onDownButtonClicked();
    void onCurrentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*);
    void onItemDoubleClicked(QTreeWidgetItem * item, int column);

private:
    QGridLayout *gridLayout;
    QVBoxLayout *vboxLayout;
    QVBoxLayout *vboxLayout1;
    QPushButton *addButton;
    QPushButton *removeButton;
    QPushButton *upButton;
    QPushButton *downButton;
    QLabel      *labelAvailable;
    QLabel      *labelSelected;
    QTreeWidget *availableWidget;
    QTreeWidget *selectedWidget;
    QSpacerItem *spacerItem;
    QSpacerItem *spacerItem1;
};

// ------------------------------------------------------------------------------

/**
 * The AccelLineEdit class provides a lineedit to specify shortcuts.
 * \author Werner Mayer
 */
class GuiExport AccelLineEdit : public QLineEdit
{
  Q_OBJECT

public:
    AccelLineEdit(QWidget * parent=nullptr);
    bool isNone() const;

    std::string displayText() const {
        return std::string(text().toUtf8().constData());
    }

    void setDisplayText(const std::string &txt) {
        setText(QString::fromUtf8(txt.c_str()));
    }

protected:
    void keyPressEvent(QKeyEvent * e) override;
    void changeEvent(QEvent *) override;
    void setupPlaceHolderText();

private:
    int keyPressedCount;
};

// ------------------------------------------------------------------------------
// Style for controlling text cursor width of a QLineEdit
//
// Modified from https://stackoverflow.com/a/54933105

class LineEditStyle: public QProxyStyle
{
    Q_OBJECT
    Q_PROPERTY(int cursorWidth READ cursorWidth WRITE setCursorWidth)

public:
    LineEditStyle(QStyle *style);

    int cursorWidth() const{
        if(cursor_width < 0)
            return baseStyle()->pixelMetric(PM_TextCursorWidth);
        return pixelMetric(PM_TextCursorWidth);
    }

    void setCursorWidth(int cursorWidth){
        cursor_width = cursorWidth;
    }

    int pixelMetric(QStyle::PixelMetric metric,
                    const QStyleOption *option = nullptr,
                    const QWidget *widget = nullptr) const override
    {
        if(metric == PM_TextCursorWidth)
            if(cursor_width > 0)
                return  cursor_width;
        return  QProxyStyle::pixelMetric(metric, option, widget);
    }

    static void setup(QLineEdit *);
    static void setup(QTextEdit *);
    static void setup(QPlainTextEdit *);
    static void setupWidget(QWidget *w);
    static void setupObject(QObject *o);
    static void setupChildren(QObject *o);

private:
    int cursor_width;
};

// ------------------------------------------------------------------------------

/**
 * The ModifierLineEdit class provides a lineedit to specify modifiers.
 */
class GuiExport ModifierLineEdit : public QLineEdit
{
  Q_OBJECT

public:
    ModifierLineEdit(QWidget * parent=nullptr);

protected:
    void keyPressEvent(QKeyEvent * e) override;
};

// ------------------------------------------------------------------------------

/**
 * The ClearLineEdit class adds a clear button at the right side.
 * http://stackoverflow.com/questions/21232224/qlineedit-with-custom-button
 */
class GuiExport ClearLineEdit : public QLineEdit
{
  Q_OBJECT

public:
    ClearLineEdit (QWidget * parent=nullptr);

protected:
    void resizeEvent(QResizeEvent *) override;

private Q_SLOTS:
    void updateClearButton(const QString &text);

private:
    QAction *clearAction;
};

// ------------------------------------------------------------------------------

using CheckListItem = QPair<QString, bool>;
class Ui_DlgTreeWidget;

/**
 * The CheckListDialog class provides a dialog with a QListView with
 * checkable items inside.
 * \author Werner Mayer
 */
class GuiExport CheckListDialog : public QDialog
{

  Q_OBJECT

public:
  CheckListDialog( QWidget* parent = nullptr, Qt::WindowFlags fl = Qt::WindowFlags() );
  ~CheckListDialog() override;

  void setCheckableItems( const QStringList& items );
  void setCheckableItems( const QList<CheckListItem>& items );
  QStringList getCheckedItems() const;

  void accept () override;

private:
  QStringList checked;
  std::unique_ptr<Ui_DlgTreeWidget> ui;
};

// ------------------------------------------------------------------------------

/**
 *  Implementation of a color button.
 * \author Werner Mayer
 */
class GuiExport ColorButton : public QPushButton
{
    Q_OBJECT

    Q_PROPERTY( QColor color READ color WRITE setColor NOTIFY changed)
    Q_PROPERTY( bool allowChangeColor READ allowChangeColor WRITE setAllowChangeColor ) // clazy:exclude=qproperty-without-notify
    Q_PROPERTY( bool drawFrame READ drawFrame WRITE setDrawFrame ) // clazy:exclude=qproperty-without-notify
    Q_PROPERTY( bool allowTransparency READ allowTransparency WRITE setAllowTransparency) // clazy:exclude=qproperty-without-notify

public:
    ColorButton(QWidget* parent = nullptr);
    ~ColorButton() override;

    void setColor(const QColor&);
    QColor color() const;

    void setPackedColor(uint32_t);
    uint32_t packedColor() const;

    void setAllowChangeColor(bool);
    bool allowChangeColor() const;

    void setDrawFrame(bool);
    bool drawFrame() const;

    void setAllowTransparency(bool);
    bool allowTransparency() const;

    void setModal(bool);
    bool isModal() const;

    void setAutoChangeColor(bool);
    bool autoChangeColor() const;

public Q_SLOTS:
    void onChooseColor();

private Q_SLOTS:
    void onColorChosen(const QColor&);
    void onRejected();

Q_SIGNALS:
    /** Emits this signal when color has changed */
    void changed();

protected:
    void paintEvent (QPaintEvent*) override;

private:
    void showModeless();
    void showModal();

private:
    struct ColorButtonP *d;
};

class GuiExport TransparentColorButton : public ColorButton
{
public:
    TransparentColorButton(QWidget* parent = 0);
};

// ------------------------------------------------------------------------------

/**
 * A text label where a url can specified. When the user clicks on the text label the system browser
 * gets opened with the specified url.
 *
 * This can be used for e.g. in the about dialog where the url of the maintainer of an application
 * can be specified.
 * @author Werner Mayer
 */
class GuiExport UrlLabel : public QLabel
{
  Q_OBJECT
  Q_PROPERTY( QString  url    READ url   WRITE setUrl) // clazy:exclude=qproperty-without-notify
  Q_PROPERTY( bool  launchExternal    READ launchExternal   WRITE setLaunchExternal) // clazy:exclude=qproperty-without-notify

public:
  UrlLabel ( QWidget * parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags() );
  ~UrlLabel() override;

  QString url() const;
  bool launchExternal() const;
  
Q_SIGNALS:
  void linkClicked(QString url);

public Q_SLOTS:
  void setUrl( const QString &u );
  void setLaunchExternal(bool l);

protected:
  void mouseReleaseEvent ( QMouseEvent * ) override;

private:
  QString _url;
  bool _launchExternal;
};


/**
 * A text label whose appearance can change based on a specified state. 
 *
 * The state is an arbitrary string exposed as a Qt Property (and thus available for selection via 
 * a stylesheet). This is intended for things like messages to the user, where a message that is an 
 * "error" might be colored differently than one that is a "warning" or a "message".
 * 
 * In order of style precedence for a given state: User preference > Stylesheet > Default
 * unless the stylesheet sets the overridePreference, in which case the stylesheet will
 * take precedence. If a stylesheet sets styles for this widgets states, it should also
 * set the "handledByStyle" property to ensure the style values are used, rather than the
 * defaults.
 * 
 * For example, the .qss might contain:
 * Gui--StatefulLabel {
 *   qproperty-overridePreference: true;
 * }
 * Gui--StatefulLabel[state="special_state"] {
 *   color: red;
 * }
 * In this case, StatefulLabels with state "special_state" will be colored red, regardless of any
 * entry in preferences. Use the "overridePreference" stylesheet option with care!
 * 
 * @author Chris Hennes
 */
class GuiExport StatefulLabel : public QLabel, public Base::Observer<const char*>
{
    Q_OBJECT
        Q_PROPERTY( bool overridePreference MEMBER _overridePreference WRITE setOverridePreference)
        Q_PROPERTY( QString state MEMBER _state WRITE setState )

public:
    StatefulLabel(QWidget* parent = nullptr);
    ~StatefulLabel() override;

    /** If an unrecognized state is set, use this style */
    void setDefaultStyle(const QString &defaultStyle);

    /** If any of the states have user preferences associated with them, this sets the parameter
        group that stores those preferences. All states must be in the same parameter group, but
        the group does not have to have entries for all of them. */
    void setParameterGroup(const std::string& groupName);

    /** Register a state and its corresponding style (optionally attached to a user preference) */
    void registerState(const QString &state, const QString &styleCSS, 
        const std::string& preferenceName = std::string());

    /** For convenience, allow simple color-only states via QColor (optionally attached to a user preference) */
    void registerState(const QString& state, const QColor& color, 
        const std::string& preferenceName = std::string());

    /** For convenience, allow simple color-only states via QColor (optionally attached to a user preference) */
    void registerState(const QString& state, const QColor& foregroundColor, const QColor& backgroundColor, 
        const std::string& preferenceName = std::string());

    /** Observes the parameter group and clears the cache if it changes */
    void OnChange(Base::Subject<const char *>& rCaller, const char* rcReason) override;

public Q_SLOTS:
    void setState(QString state);
    void setOverridePreference(bool overridePreference);

private:
    QString _state;
    bool _overridePreference;
    ParameterGrp::handle _parameterGroup;
    ParameterGrp::handle _stylesheetGroup;

    struct StateData {
        QString defaultCSS;
        std::string preferenceString;
    };
    
    std::map<QString, StateData> _availableStates;
    std::map<QString, QString> _styleCache;
    QString _defaultStyle;
};

// ----------------------------------------------------------------------

/**
 * The LabelButton class provides a label with a button on the right side.
 * @author Werner Mayer
 */
class GuiExport LabelButton : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(QVariant value  READ value  WRITE setValue NOTIFY valueChanged)

public:
    LabelButton (QWidget * parent = nullptr);
    ~LabelButton() override;

    QVariant value() const;

    QLabel *getLabel() const;
    QPushButton *getButton() const;

public Q_SLOTS:
    void setValue(const QVariant&);

protected:
    virtual void showValue(const QVariant& data);
    void resizeEvent(QResizeEvent*) override;

protected Q_SLOTS:
    virtual void browse();

Q_SIGNALS:
    void valueChanged(const QVariant &);
    void buttonClicked();

private:
    QLabel *label;
    QPushButton *button;
    QVariant _val;
};

/** Implements a transparent tool tip label that is a child widget of the main
 * window
 */
class GuiExport TipLabel: public QLabel
{
    Q_OBJECT
public:
    TipLabel(QWidget *parent, Qt::WindowFlags flags = Qt::WindowFlags());
    static TipLabel *instance(QWidget *parent, const QPoint &pos, bool overlay);
    static void hideLabel(bool hideOverlay=true);
    static void refreshIcons();
    void set(const QString &, const QString &iconPath);
    void set(const QString &, QPixmap pixmap);
    void set(const QString &);
protected:
    void paintEvent(QPaintEvent *e);
    void resizeEvent(QResizeEvent *e);
private:
    int fontSize = 0;
    QLabel *textLabel = nullptr;
};

// ----------------------------------------------------------------------

/**
 * Qt's tooltip does not work as expected with some classes, e.g. when showing
 * it in the 3d view it immediately receives a timer event to destroy itself.
 * This class is thought to circumvent this behaviour by filtering the internal
 * timer events.
 * @author Werner Mayer
 */
class GuiExport ToolTip : public QObject
{
public:
    enum Corner {
        NoCorner = -1, // pos is treat as global coordinate
        TopLeft = 0,
        TopRight = 1,
        BottomLeft = 2,
        BottomRight = 3,
    };
    static void showText(const QPoint & pos,
                         const QString & text,
                         QWidget * w = nullptr,
                         bool overlay = false,
                         Corner corner = NoCorner);
    static void showText(const QPoint & pos,
                         const QString & text,
                         const QString & iconPath,
                         QWidget * w = nullptr);
    static void showText(const QPoint & pos,
                         const QString & text,
                         const QPixmap & pixmap,
                         QWidget * w = nullptr);
    static void hideText(int delay=0, bool hideOverlay=true);

    static ToolTip* instance();

protected:
    ToolTip();
    ~ToolTip() override;

    void onShowTimer();
    void onHideTimer(bool hideOverlay);
    static bool checkToolTip(QWidget *w, QHelpEvent *e);

    bool eventFilter(QObject* o, QEvent*e) override;

private:
    bool hidden;
    static ToolTip* inst;
    bool overlay = false;
    QString text;
    QString iconPath;
    QPixmap iconPixmap;
    QPoint pos;
    Corner corner;
    QPointer<QWidget> w; // need guard in case widget gets destroyed
    QTimer tooltipTimer;
    QTimer hideTimer;
    QTimer hideTimerNoOverlay;
    QElapsedTimer displayTime;
};

// ----------------------------------------------------------------------

class GuiExport StatusWidget : public QDialog
{
    Q_OBJECT

public:
    StatusWidget(QWidget* parent);
    ~StatusWidget() override;
    void setStatusText(const QString&);
    QSize sizeHint () const override;
    void showText(int ms);

protected:
    void showEvent(QShowEvent*) override;
    void hideEvent(QHideEvent*) override;

private:
    QLabel* label;
};

// ----------------------------------------------------------------------

class GuiExport PropertyListEditor : public QPlainTextEdit
{
    Q_OBJECT

public:
    PropertyListEditor(QWidget *parent = nullptr);

    void lineNumberAreaPaintEvent(QPaintEvent *event);
    int lineNumberAreaWidth();

protected:
    void resizeEvent(QResizeEvent *event) override;

private Q_SLOTS:
    void updateLineNumberAreaWidth(int newBlockCount);
    void highlightCurrentLine();
    void updateLineNumberArea(const QRect &, int);

private:
    QWidget *lineNumberArea;
};

// ----------------------------------------------------------------------

class GuiExport LabelEditor : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
    Q_PROPERTY(QString buttonText READ buttonText WRITE setButtonText) // clazy:exclude=qproperty-without-notify

public:
    enum InputType {String, Float, Integer};

    LabelEditor (QWidget * parent = nullptr);
    ~LabelEditor() override;

    /**
    * Returns the text.
    */
    QString text() const;

    /**
    * Returns the button's text.
    */
    QString buttonText() const;

    /**
    * Set the input type.
    */
    void setInputType(InputType);

public Q_SLOTS:
    virtual void setText(const QString &);
    virtual void setButtonText (const QString &);
    virtual void validateText (const QString &);

Q_SIGNALS:
    void textChanged(const QString &);

private Q_SLOTS:
    void changeText();

protected:
    void resizeEvent(QResizeEvent*) override;

private:
    InputType type;
    QString plainText;
    QLineEdit *lineEdit;
    QPushButton *button;
};

/**
 * The ExpLineEdit class provides a lineedit that support expressing binding.
 * \author realthunder
 */
class GuiExport ExpLineEdit : public QLineEdit, public ExpressionBinding
{
    Q_OBJECT

public:
    ExpLineEdit ( QWidget * parent=nullptr, bool expressionOnly=false );

    void setExpression(std::shared_ptr<App::Expression> expr) override;
    void bind(const App::ObjectIdentifier &_path) override;
    bool apply(const std::string &propName) override;

    void keyPressEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private Q_SLOTS:
    void finishFormulaDialog();
    void openFormulaDialog();
    void onChange() override;

private:
    bool autoClose;
};

/*!
 * \brief The ButtonGroup class
 * Unlike Qt's QButtonGroup this class allows it that in exclusive mode
 * all buttons can be unchecked.
 */
class GuiExport ButtonGroup : public QButtonGroup
{
    Q_OBJECT

public:
    ButtonGroup(QObject *parent = nullptr);

    void setExclusive(bool on);
    bool exclusive() const;

private:
    bool _exclusive;
};

} // namespace Gui

#endif // GUI_WIDGETS_H
