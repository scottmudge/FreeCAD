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
# include <QColorDialog>
# include <QDebug>
# include <QDesktopServices>
# include <QDialogButtonBox>
# include <QDrag>
# include <QEventLoop>
# include <QHBoxLayout>
# include <QKeyEvent>
# include <QMessageBox>
# include <QMimeData>
# include <QPainter>
# include <QPlainTextEdit>
# include <QStylePainter>
# include <QTextEdit>
# include <QTextBlock>
# include <QTimer>
# include <QToolTip>
# include <QToolBar>
# include <QUrl>
# include <QScreen>
# if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
#   include <QDesktopWidget>
# endif
#endif

#include <QCache>
#include <QMovie>

#include <Base/Tools.h>
#include <Base/Exception.h>
#include <Base/Interpreter.h>
#include <Base/Console.h>
#include <App/ExpressionParser.h>
#include <App/Material.h>

#include "Widgets.h"
#include "Action.h"
#include "Application.h"
#include "BitmapFactory.h"
#include "Command.h"
#include "DlgExpressionInput.h"
#include "PrefWidgets.h"
#include "QuantitySpinBox_p.h"
#include "Tools.h"
#include "MainWindow.h"
#include "ViewParams.h"
#include "ui_DlgTreeWidget.h"

FC_LOG_LEVEL_INIT("Gui", true, true, true);

using namespace Gui;
using namespace App;
using namespace Base;

/**
 * Constructs an empty command view with parent \a parent.
 */
CommandIconView::CommandIconView ( QWidget * parent )
  : QListWidget(parent)
{
    connect(this, &QListWidget::currentItemChanged,
            this, &CommandIconView::onSelectionChanged);
}

/**
 * Destroys the icon view and deletes all items.
 */
CommandIconView::~CommandIconView ()
{
}

/**
 * Stores the name of the selected commands for drag and drop.
 */
void CommandIconView::startDrag (Qt::DropActions supportedActions)
{
    Q_UNUSED(supportedActions);
    QList<QListWidgetItem*> items = selectedItems();
    QByteArray itemData;
    QDataStream dataStream(&itemData, QIODevice::WriteOnly);

    QPixmap pixmap;
    dataStream << items.count();
    for (QList<QListWidgetItem*>::Iterator it = items.begin(); it != items.end(); ++it) {
        if (it == items.begin())
            pixmap = ((*it)->data(Qt::UserRole)).value<QPixmap>();
        dataStream << (*it)->text();
    }

    auto mimeData = new QMimeData;
    mimeData->setData(QStringLiteral("text/x-action-items"), itemData);

    auto drag = new QDrag(this);
    drag->setMimeData(mimeData);
    drag->setHotSpot(QPoint(pixmap.width()/2, pixmap.height()/2));
    drag->setPixmap(pixmap);
    drag->exec(Qt::MoveAction);
}

/**
 * This slot is called when a new item becomes current. \a item is the new current item
 * (or 0 if no item is now current). This slot emits the emitSelectionChanged()
 * signal for its part.
 */
void CommandIconView::onSelectionChanged(QListWidgetItem * item, QListWidgetItem *)
{
    if (item)
        Q_EMIT emitSelectionChanged(item->toolTip());
}

// ------------------------------------------------------------------------------

/* TRANSLATOR Gui::ActionSelector */

ActionSelector::ActionSelector(QWidget* parent)
  : QWidget(parent)
{
    addButton = new QPushButton(this);
    addButton->setObjectName(QStringLiteral("addButton"));
    addButton->setMinimumSize(QSize(30, 30));
    addButton->setIcon(BitmapFactory().pixmap("button_right"));
    gridLayout = new QGridLayout(this);
    gridLayout->addWidget(addButton, 1, 1, 1, 1);

    spacerItem = new QSpacerItem(33, 57, QSizePolicy::Minimum, QSizePolicy::Expanding);
    gridLayout->addItem(spacerItem, 5, 1, 1, 1);
    spacerItem1 = new QSpacerItem(33, 58, QSizePolicy::Minimum, QSizePolicy::Expanding);
    gridLayout->addItem(spacerItem1, 0, 1, 1, 1);

    removeButton = new QPushButton(this);
    removeButton->setObjectName(QStringLiteral("removeButton"));
    removeButton->setMinimumSize(QSize(30, 30));
    removeButton->setIcon(BitmapFactory().pixmap("button_left"));
    removeButton->setAutoDefault(true);
    removeButton->setDefault(false);

    gridLayout->addWidget(removeButton, 2, 1, 1, 1);

    upButton = new QPushButton(this);
    upButton->setObjectName(QStringLiteral("upButton"));
    upButton->setMinimumSize(QSize(30, 30));
    upButton->setIcon(BitmapFactory().pixmap("button_up"));

    gridLayout->addWidget(upButton, 3, 1, 1, 1);

    downButton = new QPushButton(this);
    downButton->setObjectName(QStringLiteral("downButton"));
    downButton->setMinimumSize(QSize(30, 30));
    downButton->setIcon(BitmapFactory().pixmap("button_down"));
    downButton->setAutoDefault(true);

    gridLayout->addWidget(downButton, 4, 1, 1, 1);

    vboxLayout = new QVBoxLayout();
    vboxLayout->setContentsMargins(0, 0, 0, 0);
    labelAvailable = new QLabel(this);
    vboxLayout->addWidget(labelAvailable);

    availableWidget = new QTreeWidget(this);
    availableWidget->setObjectName(QStringLiteral("availableTreeWidget"));
    availableWidget->setRootIsDecorated(false);
    availableWidget->setHeaderLabels(QStringList() << QString());
    availableWidget->header()->hide();
    vboxLayout->addWidget(availableWidget);

    gridLayout->addLayout(vboxLayout, 0, 0, 6, 1);

    vboxLayout1 = new QVBoxLayout();
    vboxLayout1->setContentsMargins(0, 0, 0, 0);
    labelSelected = new QLabel(this);
    vboxLayout1->addWidget(labelSelected);

    selectedWidget = new QTreeWidget(this);
    selectedWidget->setObjectName(QStringLiteral("selectedTreeWidget"));
    selectedWidget->setRootIsDecorated(false);
    selectedWidget->setHeaderLabels(QStringList() << QString());
    selectedWidget->header()->hide();
    vboxLayout1->addWidget(selectedWidget);

    gridLayout->addLayout(vboxLayout1, 0, 2, 6, 1);

    addButton->setText(QString());
    removeButton->setText(QString());
    upButton->setText(QString());
    downButton->setText(QString());

    connect(addButton, &QPushButton::clicked, this, &ActionSelector::onAddButtonClicked);
    connect(removeButton, &QPushButton::clicked, this, &ActionSelector::onRemoveButtonClicked);
    connect(upButton, &QPushButton::clicked, this, &ActionSelector::onUpButtonClicked);
    connect(downButton, &QPushButton::clicked, this, &ActionSelector::onDownButtonClicked);

    connect(availableWidget, &QTreeWidget::itemDoubleClicked, this, &ActionSelector::onItemDoubleClicked);
    connect(availableWidget, &QTreeWidget::currentItemChanged, this, &ActionSelector::onCurrentItemChanged);
    connect(selectedWidget, &QTreeWidget::itemDoubleClicked, this, &ActionSelector::onItemDoubleClicked);
    connect(selectedWidget, &QTreeWidget::currentItemChanged, this, &ActionSelector::onCurrentItemChanged);

    retranslateUi();
    setButtonsEnabled();
}

ActionSelector::~ActionSelector()
{
}

void ActionSelector::setSelectedLabel(const QString& label)
{
    labelSelected->setText(label);
}

QString ActionSelector::selectedLabel() const
{
    return labelSelected->text();
}

void ActionSelector::setAvailableLabel(const QString& label)
{
    labelAvailable->setText(label);
}

QString ActionSelector::availableLabel() const
{
    return labelAvailable->text();
}

void ActionSelector::retranslateUi()
{
    labelAvailable->setText(QApplication::translate("Gui::ActionSelector", "Available:"));
    labelSelected->setText(QApplication::translate("Gui::ActionSelector", "Selected:"));
    addButton->setToolTip(QApplication::translate("Gui::ActionSelector", "Add"));
    removeButton->setToolTip(QApplication::translate("Gui::ActionSelector", "Remove"));
    upButton->setToolTip(QApplication::translate("Gui::ActionSelector", "Move up"));
    downButton->setToolTip(QApplication::translate("Gui::ActionSelector", "Move down"));
}

void ActionSelector::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
    QWidget::changeEvent(event);
}

void ActionSelector::keyPressEvent(QKeyEvent* event)
{
    if ((event->modifiers() & Qt::ControlModifier)) {
        switch (event->key())
        {
        case Qt::Key_Right:
            onAddButtonClicked();
            break;
        case Qt::Key_Left:
            onRemoveButtonClicked();
            break;
        case Qt::Key_Up:
            onUpButtonClicked();
            break;
        case Qt::Key_Down:
            onDownButtonClicked();
            break;
        default:
            event->ignore();
            return;
        }
    }
}

void ActionSelector::setButtonsEnabled()
{
    addButton->setEnabled(availableWidget->indexOfTopLevelItem(availableWidget->currentItem()) > -1);
    removeButton->setEnabled(selectedWidget->indexOfTopLevelItem(selectedWidget->currentItem()) > -1);
    upButton->setEnabled(selectedWidget->indexOfTopLevelItem(selectedWidget->currentItem()) > 0);
    downButton->setEnabled(selectedWidget->indexOfTopLevelItem(selectedWidget->currentItem()) > -1 &&
                           selectedWidget->indexOfTopLevelItem(selectedWidget->currentItem()) < selectedWidget->topLevelItemCount() - 1);
}

void ActionSelector::onCurrentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)
{
    setButtonsEnabled();
}

void ActionSelector::onItemDoubleClicked(QTreeWidgetItem * item, int column)
{
    Q_UNUSED(column);
    QTreeWidget* treeWidget = item->treeWidget();
    if (treeWidget == availableWidget) {
        int index = availableWidget->indexOfTopLevelItem(item);
        item = availableWidget->takeTopLevelItem(index);
        availableWidget->setCurrentItem(nullptr);
        selectedWidget->addTopLevelItem(item);
        selectedWidget->setCurrentItem(item);
    }
    else if (treeWidget == selectedWidget) {
        int index = selectedWidget->indexOfTopLevelItem(item);
        item = selectedWidget->takeTopLevelItem(index);
        selectedWidget->setCurrentItem(nullptr);
        availableWidget->addTopLevelItem(item);
        availableWidget->setCurrentItem(item);
    }
}

void ActionSelector::onAddButtonClicked()
{
    QTreeWidgetItem* item = availableWidget->currentItem();
    if (item) {
        int index = availableWidget->indexOfTopLevelItem(item);
        item = availableWidget->takeTopLevelItem(index);
        availableWidget->setCurrentItem(nullptr);
        selectedWidget->addTopLevelItem(item);
        selectedWidget->setCurrentItem(item);
    }
}

void ActionSelector::onRemoveButtonClicked()
{
    QTreeWidgetItem* item = selectedWidget->currentItem();
    if (item) {
        int index = selectedWidget->indexOfTopLevelItem(item);
        item = selectedWidget->takeTopLevelItem(index);
        selectedWidget->setCurrentItem(nullptr);
        availableWidget->addTopLevelItem(item);
        availableWidget->setCurrentItem(item);
    }
}

void ActionSelector::onUpButtonClicked()
{
    QTreeWidgetItem* item = selectedWidget->currentItem();
    if (item && item->isSelected()) {
        int index = selectedWidget->indexOfTopLevelItem(item);
        if (index > 0) {
            selectedWidget->takeTopLevelItem(index);
            selectedWidget->insertTopLevelItem(index-1, item);
            selectedWidget->setCurrentItem(item);
        }
    }
}

void ActionSelector::onDownButtonClicked()
{
    QTreeWidgetItem* item = selectedWidget->currentItem();
    if (item && item->isSelected()) {
        int index = selectedWidget->indexOfTopLevelItem(item);
        if (index < selectedWidget->topLevelItemCount()-1) {
            selectedWidget->takeTopLevelItem(index);
            selectedWidget->insertTopLevelItem(index+1, item);
            selectedWidget->setCurrentItem(item);
        }
    }
}

// ------------------------------------------------------------------------------

/* TRANSLATOR Gui::AccelLineEdit */

/**
 * Constructs a line edit with no text.
 * The \a parent argument is sent to the QLineEdit constructor.
 */
AccelLineEdit::AccelLineEdit ( QWidget * parent )
  : QLineEdit(parent)
{
    setupPlaceHolderText();
    keyPressedCount = 0;
    LineEditStyle::setup(this);
}

bool AccelLineEdit::isNone() const
{
    return text().isEmpty();
}

void AccelLineEdit::changeEvent(QEvent *ev)
{
    switch(ev->type()) {
    case QEvent::ReadOnlyChange:
    case QEvent::LanguageChange:
    case QEvent::EnabledChange:
        setupPlaceHolderText();
        break;
    default:
        break;
    }
    QLineEdit::changeEvent(ev);
}

void AccelLineEdit::setupPlaceHolderText()
{
    if (isReadOnly() || !isEnabled()) {
        setPlaceholderText(tr("None"));
        setClearButtonEnabled(false);
    } else {
        setPlaceholderText(tr("Press a keyboard shortcut"));
        setClearButtonEnabled(true);
    }
}

/**
 * Checks which keys are pressed and show it as text.
 */
void AccelLineEdit::keyPressEvent (QKeyEvent * e)
{
    if (isReadOnly()) {
        QLineEdit::keyPressEvent(e);
        return;
    }

    QString txtLine = text();

    int key = e->key();
    Qt::KeyboardModifiers state = e->modifiers();

    // Backspace clears the shortcut if text is present, else sets Backspace as shortcut.
    // If a modifier is pressed without any other key, return.
    // AltGr is not a modifier but doesn't have a QString representation.
    switch(key) {
    case Qt::Key_Backspace:
    case Qt::Key_Delete:
        if (state == Qt::NoModifier) {
            keyPressedCount = 0;
            if (isNone()) {
                QKeySequence ks(key);
                setText(ks.toString(QKeySequence::NativeText));
            }
            else {
                clear();
            }
        }
    case Qt::Key_Control:
    case Qt::Key_Shift:
    case Qt::Key_Alt:
    case Qt::Key_Meta:
    case Qt::Key_AltGr:
        return;
    default:
        break;
    }

    if (txtLine.isEmpty()) {
        // Text maybe cleared by QLineEdit's built in clear button
        keyPressedCount = 0;
    } else {
        // 4 keys are allowed for QShortcut
        switch (keyPressedCount) {
        case 4:
            keyPressedCount = 0;
            txtLine.clear();
            break;
        case 0:
            txtLine.clear();
            break;
        default:
            txtLine += QStringLiteral(",");
            break;
        }
    }

    // Handles modifiers applying a mask.
    if ((state & Qt::ControlModifier) == Qt::ControlModifier) {
        QKeySequence ks(Qt::CTRL);
        txtLine += ks.toString(QKeySequence::NativeText);
    }
    if ((state & Qt::AltModifier) == Qt::AltModifier) {
        QKeySequence ks(Qt::ALT);
        txtLine += ks.toString(QKeySequence::NativeText);
    }
    if ((state & Qt::ShiftModifier) == Qt::ShiftModifier) {
        QKeySequence ks(Qt::SHIFT);
        txtLine += ks.toString(QKeySequence::NativeText);
    }
    if ((state & Qt::MetaModifier) == Qt::MetaModifier) {
        QKeySequence ks(Qt::META);
        txtLine += ks.toString(QKeySequence::NativeText);
    }

    // Handles normal keys
    QKeySequence ks(key);
    txtLine += ks.toString(QKeySequence::NativeText);

    setText(txtLine);
    keyPressedCount++;
}

// ------------------------------------------------------------------------------

/* TRANSLATOR Gui::ModifierLineEdit */

/**
 * Constructs a line edit with no text.
 * The \a parent argument is sent to the QLineEdit constructor.
 */
ModifierLineEdit::ModifierLineEdit (QWidget * parent )
  : QLineEdit(parent)
{
    setPlaceholderText(tr("Press modifier keys"));
    LineEditStyle::setup(this);
}

/**
 * Checks which modifiers are pressed and show it as text.
 */
void ModifierLineEdit::keyPressEvent (QKeyEvent * e)
{
    int key = e->key();
    Qt::KeyboardModifiers state = e->modifiers();

    switch (key) {
    case Qt::Key_Backspace:
    case Qt::Key_Delete:
        clear();
        return;
    case Qt::Key_Control:
    case Qt::Key_Shift:
    case Qt::Key_Alt:
    case Qt::Key_Meta:
        break;
    default:
        return;
    }

    clear();
    QString txtLine;

    // Handles modifiers applying a mask.
    if ((state & Qt::ControlModifier) == Qt::ControlModifier) {
        QKeySequence ks(Qt::CTRL);
        txtLine += ks.toString(QKeySequence::NativeText);
    }
    if ((state & Qt::AltModifier) == Qt::AltModifier) {
        QKeySequence ks(Qt::ALT);
        txtLine += ks.toString(QKeySequence::NativeText);
    }
    if ((state & Qt::ShiftModifier) == Qt::ShiftModifier) {
        QKeySequence ks(Qt::SHIFT);
        txtLine += ks.toString(QKeySequence::NativeText);
    }
    if ((state & Qt::MetaModifier) == Qt::MetaModifier) {
        QKeySequence ks(Qt::META);
        txtLine += ks.toString(QKeySequence::NativeText);
    }

    setText(txtLine);
}

// ------------------------------------------------------------------------------

ClearLineEdit::ClearLineEdit (QWidget * parent)
  : QLineEdit(parent)
{
    clearAction = this->addAction(QIcon(QStringLiteral(":/icons/edit-cleartext.svg")),
                                        QLineEdit::TrailingPosition);
    connect(clearAction, &QAction::triggered, this, &ClearLineEdit::clear);
    connect(this, &QLineEdit::textChanged, this, &ClearLineEdit::updateClearButton);

    LineEditStyle::setup(this);
}

void ClearLineEdit::resizeEvent(QResizeEvent *e)
{
    QLineEdit::resizeEvent(e);
}

void ClearLineEdit::updateClearButton(const QString& text)
{
    clearAction->setVisible(!text.isEmpty());
}

// ------------------------------------------------------------------------------

/* TRANSLATOR Gui::CheckListDialog */

/**
 *  Constructs a CheckListDialog which is a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
CheckListDialog::CheckListDialog( QWidget* parent, Qt::WindowFlags fl )
    : QDialog( parent, fl )
    , ui(new Ui_DlgTreeWidget)
{
    ui->setupUi(this);
}

/**
 *  Destroys the object and frees any allocated resources
 */
CheckListDialog::~CheckListDialog()
{
    // no need to delete child widgets, Qt does it all for us
}

/**
 * Sets the items to the dialog's list view. By default all items are checkable..
 */
void CheckListDialog::setCheckableItems( const QStringList& items )
{
    for (const auto & it : items) {
        auto item = new QTreeWidgetItem(ui->treeWidget);
        item->setText(0, it);
        item->setCheckState(0, Qt::Unchecked);
    }
}

/**
 * Sets the items to the dialog's list view. If the boolean type of a CheckListItem
 * is set to false the item is not checkable any more.
 */
void CheckListDialog::setCheckableItems( const QList<CheckListItem>& items )
{
    for (const auto & it : items) {
        auto item = new QTreeWidgetItem(ui->treeWidget);
        item->setText(0, it.first);
        item->setCheckState(0, ( it.second ? Qt::Checked : Qt::Unchecked));
    }
}

/**
 * Returns a list of the check items.
 */
QStringList CheckListDialog::getCheckedItems() const
{
    return checked;
}

/**
 * Collects all checked items to be able to return them by call \ref getCheckedItems().
 */
void CheckListDialog::accept ()
{
    QTreeWidgetItemIterator it(ui->treeWidget, QTreeWidgetItemIterator::Checked);
    while (*it) {
        checked.push_back((*it)->text(0));
        ++it;
    }

    QDialog::accept();
}

// ------------------------------------------------------------------------------

namespace Gui {
struct ColorButtonP
{
    QColor old, col;
    QPointer<QColorDialog> cd;
    bool allowChange;
    bool autoChange;
    bool drawFrame;
    bool allowTransparency;
    bool modal;
    bool dirty;
    bool allowAlpha;

    ColorButtonP()
        : cd(nullptr)
        , allowChange(true)
        , autoChange(false)
        , drawFrame(true)
        , allowTransparency(false)
        , modal(true)
        , dirty(true)
        , allowAlpha(false)
    {
    }
};
}

/**
 * Constructs a colored button called \a name with parent \a parent.
 */
ColorButton::ColorButton(QWidget* parent)
    : QPushButton(parent)
{
    d = new ColorButtonP();
    d->col = palette().color(QPalette::Active,QPalette::Midlight);
    connect(this, &ColorButton::clicked, this, &ColorButton::onChooseColor);

    int e = style()->pixelMetric(QStyle::PM_ButtonIconSize);
    setIconSize(QSize(2*e, e));
}

/**
 * Destroys the button.
 */
ColorButton::~ColorButton()
{
    delete d;
}

/**
 * Sets the color \a c to the button.
 */
void ColorButton::setColor(const QColor& c)
{
    d->col = c;
    d->dirty = true;
    update();
}

/**
 * Returns the current color of the button.
 */
QColor ColorButton::color() const
{
    return d->col;
}

/**
 * Sets the packed color \a c to the button.
 */
void ColorButton::setPackedColor(uint32_t c)
{
    App::Color color;
    color.setPackedValue(c);
    d->col.setRedF(color.r);
    d->col.setGreenF(color.g);
    d->col.setBlueF(color.b);
    d->col.setAlphaF(color.a);
    d->dirty = true;
    update();
}

/**
 * Returns the current packed color of the button.
 */
uint32_t ColorButton::packedColor() const
{
    App::Color color(d->col.redF(), d->col.greenF(), d->col.blueF(), d->col.alphaF());
    return color.getPackedValue();
}

void ColorButton::setAllowChangeColor(bool ok)
{
    d->allowChange = ok;
}

bool ColorButton::allowChangeColor() const
{
    return d->allowChange;
}

void ColorButton::setDrawFrame(bool ok)
{
    d->drawFrame = ok;
}

bool ColorButton::drawFrame() const
{
    return d->drawFrame;
}

void Gui::ColorButton::setAllowTransparency(bool allow)
{
    d->allowTransparency = allow;
    if (d->cd)
        d->cd->setOption(QColorDialog::ColorDialogOption::ShowAlphaChannel, allow);
}

bool Gui::ColorButton::allowTransparency() const
{
    if (d->cd)
        return d->cd->testOption(QColorDialog::ColorDialogOption::ShowAlphaChannel);
    else
        return d->allowTransparency;
}

void ColorButton::setModal(bool b)
{
    d->modal = b;
}

bool ColorButton::isModal() const
{
    return d->modal;
}

void ColorButton::setAutoChangeColor(bool on)
{
    d->autoChange = on;
}

bool ColorButton::autoChangeColor() const
{
    return d->autoChange;
}

/**
 * Draws the button label.
 */
void ColorButton::paintEvent (QPaintEvent * e)
{
    if (d->dirty) {
        QSize isize = iconSize();
        QPixmap pix(isize);
        pix.fill(palette().button().color());

        QPainter p(&pix);

        int w = pix.width();
        int h = pix.height();
        p.setPen(QPen(Qt::gray));

        QColor c = d->col;
        c.setAlpha(255);
        if (d->drawFrame) {
            p.setBrush(c);
            p.drawRect(2, 2, w - 5, h - 5);
        }
        else {
            p.fillRect(0, 0, w, h, QBrush(c));
        }
        if (allowTransparency() && d->col.alpha() != 255) {
            p.setPen(QPen(Qt::white, 2));
            p.setBrush(QBrush());
            p.drawRect(4, 4, w-8, h-8);
        }

        setIcon(QIcon(pix));

        d->dirty = false;
    }

    QPushButton::paintEvent(e);
}

void ColorButton::showModeless()
{
    if (d->cd.isNull()) {
        d->old = d->col;

        QColorDialog* dlg = new QColorDialog(d->col, this);
        dlg->setAttribute(Qt::WA_DeleteOnClose);
        if (DialogOptions::dontUseNativeColorDialog())
            dlg->setOptions(QColorDialog::DontUseNativeDialog);
        dlg->setOption(QColorDialog::ColorDialogOption::ShowAlphaChannel, d->allowTransparency);
        dlg->setCurrentColor(d->old);
        connect(dlg, &QColorDialog::rejected, this, &ColorButton::onRejected);
        connect(dlg, &QColorDialog::currentColorChanged, this, &ColorButton::onColorChosen);
        d->cd = dlg;
    }
    d->cd->show();
}

void ColorButton::showModal()
{
    QColor currentColor = d->col;
    QColorDialog* dlg = new QColorDialog(d->col, this);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    if (DialogOptions::dontUseNativeColorDialog())
        dlg->setOptions(QColorDialog::DontUseNativeDialog);
    dlg->setOption(QColorDialog::ColorDialogOption::ShowAlphaChannel, d->allowTransparency);

    if (d->autoChange) {
        connect(dlg, &QColorDialog::currentColorChanged, this, &ColorButton::onColorChosen);
    }

    dlg->setCurrentColor(currentColor);
    dlg->adjustSize();

    connect(dlg, &QColorDialog::finished, this, [&](int result) {
        if (result == QDialog::Accepted) {
            QColor c = dlg->selectedColor();
            if (c.isValid()) {
                setColor(c);
                Q_EMIT changed();
            }
        }
        else if (d->autoChange) {
            setColor(currentColor);
            Q_EMIT changed();
        }
    });

    dlg->exec();
}

/**
 * Opens a QColorDialog to set a new color.
 */
void ColorButton::onChooseColor()
{
    if (!d->allowChange)
        return;
    if (d->modal) {
        showModal();
    }
    else {
        showModeless();
    }
}

void ColorButton::onColorChosen(const QColor& c)
{
    setColor(c);
    Q_EMIT changed();
}

void ColorButton::onRejected()
{
    setColor(d->old);
    Q_EMIT changed();
}

// ------------------------------------------------------------------------------

TransparentColorButton::TransparentColorButton(QWidget *parent)
    :ColorButton(parent)
{
    setAllowTransparency(true);
}

// ------------------------------------------------------------------------------

UrlLabel::UrlLabel(QWidget* parent, Qt::WindowFlags f)
    : QLabel(parent, f)
    , _url (QStringLiteral("http://localhost"))
    , _launchExternal(true)
{
    setToolTip(this->_url);    
    setCursor(Qt::PointingHandCursor);
    if (qApp->styleSheet().isEmpty())
        setStyleSheet(QStringLiteral("Gui--UrlLabel {color: #0000FF;text-decoration: underline;}"));
}

UrlLabel::~UrlLabel()
{
}

void Gui::UrlLabel::setLaunchExternal(bool l)
{
    _launchExternal = l;
}

void UrlLabel::mouseReleaseEvent(QMouseEvent*)
{
    if (_launchExternal)
        QDesktopServices::openUrl(this->_url);
    else
        // Someone else will deal with it...
        Q_EMIT linkClicked(_url);
}

QString UrlLabel::url() const
{
    return this->_url;
}

bool Gui::UrlLabel::launchExternal() const
{
    return _launchExternal;
}

void UrlLabel::setUrl(const QString& u)
{
    this->_url = u;
    setToolTip(this->_url);
}

// --------------------------------------------------------------------

StatefulLabel::StatefulLabel(QWidget* parent)
    : QLabel(parent)
    , _overridePreference(false)
{
    // Always attach to the parameter group that stores the main FreeCAD stylesheet
    _stylesheetGroup = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/General");
    _stylesheetGroup->Attach(this);
}

StatefulLabel::~StatefulLabel()
{
    if (_parameterGroup.isValid())
        _parameterGroup->Detach(this);
    _stylesheetGroup->Detach(this);
}

void StatefulLabel::setDefaultStyle(const QString& defaultStyle)
{
    _defaultStyle = defaultStyle;
}

void StatefulLabel::setParameterGroup(const std::string& groupName)
{
    if (_parameterGroup.isValid())
        _parameterGroup->Detach(this);
        
    // Attach to the Parametergroup so we know when it changes
    _parameterGroup = App::GetApplication().GetParameterGroupByPath(groupName.c_str());    
    if (_parameterGroup.isValid())
        _parameterGroup->Attach(this);
}

void StatefulLabel::registerState(const QString& state, const QString& styleCSS,
    const std::string& preferenceName)
{
    _availableStates[state] = { styleCSS, preferenceName };
}

void StatefulLabel::registerState(const QString& state, const QColor& color,
    const std::string& preferenceName)
{
    QString css;
    if (color.isValid())
        css = QString::fromUtf8("Gui--StatefulLabel{ color : rgba(%1,%2,%3,%4) ;}").arg(color.red()).arg(color.green()).arg(color.blue()).arg(color.alpha());
    _availableStates[state] = { css, preferenceName };
}

void StatefulLabel::registerState(const QString& state, const QColor& fg, const QColor& bg,
    const std::string& preferenceName)
{
    QString colorEntries;
    if (fg.isValid())
        colorEntries.append(QString::fromUtf8("color : rgba(%1,%2,%3,%4);").arg(fg.red()).arg(fg.green()).arg(fg.blue()).arg(fg.alpha()));
    if (bg.isValid())
        colorEntries.append(QString::fromUtf8("background-color : rgba(%1,%2,%3,%4);").arg(bg.red()).arg(bg.green()).arg(bg.blue()).arg(bg.alpha()));
    QString css = QString::fromUtf8("Gui--StatefulLabel{ %1 }").arg(colorEntries);
    _availableStates[state] = { css, preferenceName };
}

/** Observes the parameter group and clears the cache if it changes */
void StatefulLabel::OnChange(Base::Subject<const char*>& rCaller, const char* rcReason)
{
    Q_UNUSED(rCaller);
    auto changedItem = std::string(rcReason);
    if (changedItem == "StyleSheet") {
        _styleCache.clear();
    }
    else {
        for (const auto& state : _availableStates) {
            if (state.second.preferenceString == changedItem) {
                _styleCache.erase(_styleCache.find(state.first));
            }
        }
    }
}

void StatefulLabel::setOverridePreference(bool overridePreference)
{
    _overridePreference = overridePreference;
}

void StatefulLabel::setState(QString state)
{
    _state = state;
    this->ensurePolished();

    // If the stylesheet insists, ignore all other logic and let it do its thing. This
    // property is *only* set by the stylesheet.
    if (_overridePreference)
        return;

    // Check the cache first:
    if (auto style = _styleCache.find(_state); style != _styleCache.end()) {
        auto test = style->second.toStdString();
        this->setStyleSheet(style->second);
        return;
    }

    if (auto entry = _availableStates.find(state); entry != _availableStates.end()) {
        // Order of precedence: first, check if the user has set this in their preferences:
        if (!entry->second.preferenceString.empty()) {
            // First, try to see if it's just stored a color (as an unsigned int):
            auto availableColorPrefs = _parameterGroup->GetUnsignedMap();
            std::string lookingForGroup = entry->second.preferenceString;
            for (const auto &unsignedEntry : availableColorPrefs) {
                std::string foundGroup = unsignedEntry.first;
                if (unsignedEntry.first == entry->second.preferenceString) {
                    // Convert the stored Uint into usable color data:
                    unsigned int col = unsignedEntry.second;
                    QColor qcolor(App::Color::fromPackedRGB<QColor>(col));
                    this->setStyleSheet(QString::fromUtf8("Gui--StatefulLabel{ color : rgba(%1,%2,%3,%4) ;}").arg(qcolor.red()).arg(qcolor.green()).arg(qcolor.blue()).arg(qcolor.alpha()));
                    _styleCache[state] = this->styleSheet();
                    return;
                }
            }

            // If not, try to see if there's an entire style string set as ASCII:
            auto availableStringPrefs = _parameterGroup->GetASCIIMap();
            for (const auto& stringEntry : availableStringPrefs) {
                if (stringEntry.first == entry->second.preferenceString) {
                    QString css = QString::fromUtf8("Gui--StatefulLabel{ %1 }").arg(QString::fromStdString(stringEntry.second));
                    this->setStyleSheet(css);
                    _styleCache[state] = this->styleSheet();
                    return;
                }
            }
        }

        // If there is no preferences entry for this label, allow the stylesheet to set it, and only set to the default
        // formatting if there is no stylesheet entry
        if (qApp->styleSheet().isEmpty()) {
            this->setStyleSheet(entry->second.defaultCSS);
            _styleCache[state] = this->styleSheet();
            return;
        }
        // else the stylesheet sets our appearance: make sure it recalculates the appearance:
        this->setStyleSheet(QString());
        this->setStyle(qApp->style());
        this->style()->unpolish(this);
        this->style()->polish(this);
    }
    else {
        if (styleSheet().isEmpty()) {
            this->setStyleSheet(_defaultStyle);
            _styleCache[state] = this->styleSheet();
        }
    }
}

// --------------------------------------------------------------------

/* TRANSLATOR Gui::LabelButton */

/**
 * Constructs a file chooser called \a name with the parent \a parent.
 */
LabelButton::LabelButton (QWidget * parent)
  : QWidget(parent)
{
    auto layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(1);

    label = new QLabel(this);
    label->setAutoFillBackground(true);
    layout->addWidget(label);

    button = new QPushButton(QStringLiteral("..."), this);
#if defined (Q_OS_MAC)
    button->setAttribute(Qt::WA_LayoutUsesWidgetRect); // layout size from QMacStyle was not correct
#endif
    layout->addWidget(button);

    connect(button, &QPushButton::clicked, this, &LabelButton::browse);
    connect(button, &QPushButton::clicked, this, &LabelButton::buttonClicked);
}

LabelButton::~LabelButton()
{
}

void LabelButton::resizeEvent(QResizeEvent* e)
{
    button->setFixedWidth(e->size().height());
    button->setFixedHeight(e->size().height());
}

QLabel *LabelButton::getLabel() const
{
    return label;
}

QPushButton *LabelButton::getButton() const
{
    return button;
}

QVariant LabelButton::value() const
{
    return _val;
}

void LabelButton::setValue(const QVariant& val)
{
    _val = val;
    showValue(_val);
    Q_EMIT valueChanged(_val);
}

void LabelButton::showValue(const QVariant& data)
{
    label->setText(data.toString());
}

void LabelButton::browse()
{
}

// ----------------------------------------------------------------------

namespace {

std::unordered_map<QLabel *, QPointer<QLabel>> _QtTipLabels;
QPointer<TipLabel> _TipLabel;
QPointer<TipLabel> _OverlayTipLabel;
struct TipIconEntry {
    QPixmap pixmap;
    QMovie animation;
};
QCache<QString, TipIconEntry> _TipIconCache(1024);
int _TipIconSize;

void hideQtToolTips()
{
    // QToolTips actually uses multiple instances of a private class QTipLabel.
    // In some rare condition (e.g. calling QToolTip::hideText() too frequent),
    // previous shown TipLabel instance is somehow abandoned, and left visible.
    // As a work around, I'll collect all shown QTipLabel instances and hide
    // them.
    decltype(_QtTipLabels) labels = std::move(_QtTipLabels);
    _QtTipLabels.clear();
    for (const auto &v : labels) {
        if (v.second)
            v.second->setVisible(false);
    }
}

QRect getScreenGeometry(const QPoint &pos, QWidget *w)
{
#if QT_VERSION < QT_VERSION_CHECK(5,14,0)
    if (QApplication::desktop()->isVirtualDesktop())
        return QApplication::screenAt(pos)->availableGeometry();
    else
        return QApplication::desktop()->availableGeometry(w);
#else
    auto screen = w->screen();
    if (auto sibling = screen->virtualSiblingAt(pos))
        return sibling->availableGeometry();
    else
        return screen->availableGeometry();
#endif
}

} // anonymous event filter

TipLabel::TipLabel(QWidget *parent, Qt::WindowFlags flags)
    : QLabel(parent, flags)
{
    textLabel = new QLabel(this);

    // setAttribute(Qt::WA_NoSystemBackground, true);
    // setAttribute(Qt::WA_TranslucentBackground, true);
    setWindowFlag(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
    setForegroundRole(QPalette::ToolTipText);
    setBackgroundRole(QPalette::ToolTipBase);
    setPalette(QToolTip::palette());
    ensurePolished();
    setMargin(1 + style()->pixelMetric(QStyle::PM_ToolTipLabelFrameWidth, 0, this));
    setFrameShape(QFrame::NoFrame);
    setAlignment(Qt::AlignRight);
    setIndent(1);
    textLabel->setMargin(margin());
    textLabel->setIndent(indent());
}

TipLabel *TipLabel::instance(QWidget *parent, const QPoint &pos, bool overlay)
{
    if (!overlay) {
        if (!_TipLabel) {
            (void)pos;
            _TipLabel = new TipLabel(
                    nullptr,
                    Qt::ToolTip | Qt::BypassGraphicsProxyWidget);
        }
        return _TipLabel;
    }

    auto findParent = [&](QWidget *widget) -> QWidget* {
        if (!widget)
            return getMainWindow();
        for (auto w=widget; w; w=w->parentWidget()) {
            if (w == getMainWindow())
                return w;
        }
        return widget;
    };

    static QPointer<QWidget> _parent;
    if (!_OverlayTipLabel) {
        _OverlayTipLabel = new TipLabel(findParent(parent));
        _OverlayTipLabel->setObjectName(QStringLiteral("fc_overlay_tiplabel"));
    } else if (_parent != parent) {
        _OverlayTipLabel->setParent(findParent(parent));
        _parent = parent;
    }
    return _OverlayTipLabel;
}

void TipLabel::hideLabel(bool hideOverlay)
{
    auto doHide = [](TipLabel *label) {
        if (label && label->isVisible()) {
            label->setVisible(false);
            if (label->movie()) {
                label->movie()->stop();
                label->setMovie(nullptr);
            }
        }
    };
    doHide(_TipLabel);
    if (hideOverlay)
        doHide(_OverlayTipLabel);
}

void TipLabel::refreshIcons()
{
    _TipIconCache.clear();
}

void TipLabel::set(const QString &text)
{
    textLabel->setWordWrap(Qt::mightBeRichText(text));
    textLabel->setText(text);
    if (_OverlayTipLabel == this
            && fontSize != ViewParams::getPreselectionToolTipFontSize()) {
        fontSize = ViewParams::getPreselectionToolTipFontSize();
        if (!fontSize)
            textLabel->setFont(QFont());
        else {
            QFont font = this->font();
            font.setPointSize(fontSize);
            textLabel->setFont(font);
        }
    }
            
    QFontMetrics fm(font());
    QSize extra(1, 0);
    // Make it look good with the default ToolTip font on Mac, which has a small descent.
    if (fm.descent() == 2 && fm.ascent() >= 11)
        ++extra.rheight();

    textLabel->resize(textLabel->sizeHint());
    QSize s(textLabel->sizeHint() + extra);
    resize(s);
}

void TipLabel::set(const QString &text, QPixmap pixmap)
{
    set(text);
    if (pixmap.isNull()) {
        setPixmap(pixmap);
        return;
    }
    int tipIconSize = ViewParams::getToolTipIconSize();
    if (pixmap.height() != tipIconSize) {
        QSize size(tipIconSize, tipIconSize);
        if (pixmap.width() && pixmap.height())
            size.setWidth(tipIconSize / pixmap.height() * pixmap.width());
        pixmap = pixmap.scaled(size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }

    this->setPixmap(pixmap);
    QSize iconSize = pixmap.size();
    iconSize.setHeight(std::max(iconSize.height(), tipIconSize));
    QSize s(this->size());
    if (s.height() < iconSize.height())
        s.setHeight(iconSize.height());
    else
        iconSize.setHeight(s.height());
    s.setWidth(s.width() + iconSize.width());
    this->resize(s);
}

void TipLabel::set(const QString &text, const QString &iconPath)
{
    set(text);
    if (iconPath.isEmpty()) {
        setMovie(nullptr);
        return;
    }
    if (_TipIconSize != ViewParams::getToolTipIconSize()) {
        _TipIconSize = ViewParams::getToolTipIconSize();
        _TipIconCache.clear();
    }
    auto entry = _TipIconCache.object(iconPath);
    if (!entry) {
        QFileInfo finfo(iconPath + QStringLiteral(".gif"));
        if (finfo.exists()) {
            entry = new TipIconEntry;
            entry->animation.setFileName(finfo.filePath());
            _TipIconCache.insert(iconPath, entry, std::min(128, entry->animation.frameCount()));
        }
        else {
            QPixmap pixmap;
            QByteArray path(iconPath.toUtf8());
            BitmapFactory().findPixmapInCache(path.constData(), pixmap);
            if (pixmap.height() != _TipIconSize) {
                QSize size(_TipIconSize, _TipIconSize);
                if (pixmap.width() && pixmap.height())
                    size.setWidth(_TipIconSize / pixmap.height() * pixmap.width());
                if (iconPath.endsWith(QStringLiteral(".svg"), Qt::CaseInsensitive))
                    pixmap = BitmapFactory().pixmapFromSvg(path.constData(), size);
                else
                    pixmap = pixmap.scaled(size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
            }
            if (!pixmap.isNull()) {
                entry = new TipIconEntry;
                entry->pixmap = pixmap;
                _TipIconCache.insert(iconPath, entry, 1);
            }
        }
    }

    QSize iconSize;
    if (entry && !entry->pixmap.isNull()) {
        this->setPixmap(entry->pixmap);
        iconSize = entry->pixmap.size();
    } else if (entry && entry->animation.isValid()) {
        this->setMovie(&entry->animation);
        entry->animation.start();
        iconSize = entry->animation.currentImage().size();
    } else {
        this->setMovie(nullptr);
        return;
    }

    iconSize.setHeight(std::max(iconSize.height(), _TipIconSize));
    QSize s(this->size());
    if (s.height() < iconSize.height())
        s.setHeight(iconSize.height());
    else
        iconSize.setHeight(s.height());
    s.setWidth(s.width() + iconSize.width());
    this->resize(s);
}

void TipLabel::paintEvent(QPaintEvent *ev)
{
    QStylePainter p(this);
    QStyleOptionFrame opt;
    opt.init(this);
    p.setOpacity(0.7); // This seems only effecitve when no stylesheet is set.
    p.drawPrimitive(QStyle::PE_PanelTipLabel, opt);
    p.end();

    QLabel::paintEvent(ev);
}

void TipLabel::resizeEvent(QResizeEvent *e)
{
    QStyleHintReturnMask frameMask;
    QStyleOption option;
    option.init(this);
    if (style()->styleHint(QStyle::SH_ToolTip_Mask, &option, this, &frameMask))
        setMask(frameMask.region);

    QLabel::resizeEvent(e);
}

// ----------------------------------------------------------------------

ToolTip* ToolTip::inst = nullptr;

ToolTip* ToolTip::instance()
{
    if (!inst)
        inst = new ToolTip();
    return inst;
}

ToolTip::ToolTip() : hidden(true)
{
    tooltipTimer.setSingleShot(true);
    hideTimer.setSingleShot(true);
    hideTimerNoOverlay.setSingleShot(true);
    connect(&tooltipTimer, &QTimer::timeout, [this]() {onShowTimer();});
    connect(&hideTimer, &QTimer::timeout, [this]() {onHideTimer(true);});
    connect(&hideTimerNoOverlay, &QTimer::timeout, [this]() {onHideTimer(false);});
    qApp->installEventFilter(this);
}

ToolTip::~ToolTip()
{
}

void ToolTip::showText(const QPoint & pos,
                       const QString & text,
                       QWidget * w,
                       bool overlay,
                       Corner corner)
{
    ToolTip* tip = instance();
    if (!text.isEmpty()) {
        if (!overlay && ViewParams::getToolTipDisable())
            return;
        tip->pos = pos;
        tip->text = text;
        tip->iconPixmap = QPixmap();
        tip->iconPath.clear();
        tip->w = w;
        tip->corner = corner;
        tip->overlay = overlay;
        tip->tooltipTimer.start(80);
        tip->displayTime.start();
        tip->hideTimer.stop();
        tip->hideTimerNoOverlay.stop();
    }
    else {
        hideText();
    }
}

void ToolTip::showText(const QPoint & pos,
                       const QString & text,
                       const QString & iconPath,
                       QWidget * w)
{
    ToolTip* tip = instance();
    if (!text.isEmpty() || !iconPath.isEmpty()) {
        if (ViewParams::getToolTipDisable())
            return;
        tip->pos = pos;
        tip->text = text;
        tip->iconPath = iconPath;
        tip->iconPixmap = QPixmap();
        tip->corner = NoCorner;
        tip->w = w;
        tip->overlay = false;
        tip->tooltipTimer.start(80);
        tip->displayTime.start();
        tip->hideTimer.stop();
        tip->hideTimerNoOverlay.stop();
    }
    else {
        hideText();
    }
}

void ToolTip::showText(const QPoint & pos,
                       const QString & text,
                       const QPixmap & pixmap,
                       QWidget * w)
{
    ToolTip* tip = instance();
    if (!text.isEmpty() || !pixmap.isNull()) {
        if (ViewParams::getToolTipDisable())
            return;
        tip->pos = pos;
        tip->text = text;
        tip->iconPath.clear();
        tip->iconPixmap = pixmap;
        tip->corner = NoCorner;
        tip->w = w;
        tip->overlay = false;
        tip->tooltipTimer.start(80);
        tip->displayTime.start();
        tip->hideTimer.stop();
        tip->hideTimerNoOverlay.stop();
    }
    else {
        hideText();
    }
}

void ToolTip::hideText(int delay, bool hideOverlay)
{
    if (delay > 0) {
        auto &timer = hideOverlay ? instance()->hideTimer : instance()->hideTimerNoOverlay;
        if (!timer.isActive()) {
            instance()->tooltipTimer.stop();
            timer.start(delay);
        }
        return;
    }
    instance()->tooltipTimer.stop();
    TipLabel::hideLabel(hideOverlay);
    hideQtToolTips();
}

void ToolTip::onShowTimer()
{
    if (ViewParams::getToolTipIconSize() > 0
            || this->overlay
            || this->corner != NoCorner) {
        auto tipLabel = TipLabel::instance(w, pos, overlay);
        QWidget *parent = tipLabel->parentWidget();
        if(this->iconPixmap.isNull())
            tipLabel->set(text, iconPath);
        else
            tipLabel->set(text, this->iconPixmap);
        if (!this->w || this->corner == NoCorner) {
            QRect screen = getScreenGeometry(pos, w);
            pos += QPoint(2, 16);
            if (pos.x() + tipLabel->width() > screen.x() + screen.width())
                pos.rx() -= 4 + tipLabel->width();
            if (pos.y() + tipLabel->height() > screen.y() + screen.height())
                pos.ry() -= 24 + tipLabel->height();
            if (pos.y() < screen.y())
                pos.setY(screen.y());
            if (pos.x() + tipLabel->width() > screen.x() + screen.width())
                pos.setX(screen.x() + screen.width() - tipLabel->width());
            if (pos.x() < screen.x())
                pos.setX(screen.x());
            if (pos.y() + tipLabel->height() > screen.y() + screen.height())
                pos.setY(screen.y() + screen.height() - tipLabel->height());
            if (parent)
                pos = parent->mapFromGlobal(pos);
            tipLabel->move(pos);
        } else if (!parent)
            return;
        else {
            pos = parent->mapFromGlobal(this->w->mapToGlobal(QPoint()));
            auto size = this->w->size();
            switch(this->corner) {
            case TopLeft:
                pos += this->pos;
                break;
            case TopRight:
                pos.setX(pos.x() + size.width() - tipLabel->width() - this->pos.x());
                pos.setY(pos.y() + this->pos.y());
                break;
            case BottomLeft:
                pos.setX(pos.x() + this->pos.x());
                pos.setY(pos.y() + size.height() - tipLabel->height() - this->pos.y());
                break;
            case BottomRight:
                pos.setX(pos.x() + size.width() - tipLabel->width() - this->pos.x());
                pos.setY(pos.y() + size.height() - tipLabel->height() - this->pos.y());
                break;
            default:
                break;
            }
            tipLabel->move(pos);
        }
        hideQtToolTips();
        if (_TipLabel && _TipLabel != tipLabel)
            _TipLabel->hide();
        else if (_OverlayTipLabel && _OverlayTipLabel != tipLabel)
            _OverlayTipLabel->hide();
        tipLabel->show();
        tipLabel->raise();
    } else {
        TipLabel::hideLabel();
        QToolTip::showText(pos, text, w);
    }
    hideTimer.stop();
    hideTimerNoOverlay.stop();
    tooltipTimer.stop();
    displayTime.restart();
}

void ToolTip::onHideTimer(bool hideOverlay)
{
    hideTimer.stop();
    hideTimerNoOverlay.stop();
    tooltipTimer.stop();
    TipLabel::hideLabel(hideOverlay);
}

bool ToolTip::checkToolTip(QWidget *w, QHelpEvent *helpEvent) {
    if (ViewParams::getToolTipDisable())
        return true;
    if (ViewParams::getToolTipIconSize() <= 0)
        return false;
    QString tooltip;
    if (auto menu = qobject_cast<QMenu*>(w)) {
        if (const QAction *action = menu->actionAt(helpEvent->pos()))
            tooltip = action->toolTip();
    }
    else if (auto tabbar = qobject_cast<QTabBar*>(w)) {
        int index = tabbar->tabAt(helpEvent->pos());
        if (index >= 0)
            tooltip = tabbar->tabToolTip(index);
    }
    else if (qobject_cast<QAbstractItemView*>(w))
        return false;
    else
        tooltip = w->toolTip();

    if (tooltip.isEmpty())
        return false;

    QString key = QStringLiteral("<img src='");
    int index = tooltip.indexOf(key);
    QString iconPath;
    if (index >= 0) {
        int end = tooltip.indexOf(QLatin1Char('\''), index + key.size());
        if (end >= 0) {
            iconPath = tooltip.mid(index+key.size(), end-index-key.size());
            iconPath = QUrl::fromPercentEncoding(iconPath.toUtf8());
            end = tooltip.indexOf(QLatin1Char('>'), end+1);
            if (end < 0)
                iconPath.clear();
            else
                tooltip = tooltip.left(index) + tooltip.right(tooltip.size() - end - 1);
        }
    }
    ToolTip::showText(helpEvent->globalPos(), tooltip, iconPath, w);
    return true;
}

bool ToolTip::eventFilter(QObject* o, QEvent*e)
{
    if (!o->isWidgetType())
        return false;
    auto w = static_cast<QWidget*>(o);
    switch(e->type()) {
    case QEvent::MouseButtonPress:
        hideText();
        break;
    case QEvent::KeyPress:
        if (static_cast<QKeyEvent*>(e)->key() == Qt::Key_Escape)
            hideText();
        break;
    case QEvent::ToolTip:
        return checkToolTip(w, static_cast<QHelpEvent*>(e));
    case QEvent::Leave:
        hideText(300, false);
        break;
    case QEvent::Timer:
        if (static_cast<QTimerEvent*>(e)->timerId() == hideTimer.timerId()
            || static_cast<QTimerEvent*>(e)->timerId() == hideTimerNoOverlay.timerId())
            break;
        // fall through
    case QEvent::Show:
    case QEvent::Hide:
        if (auto label = qobject_cast<QLabel*>(o)) {
            if (label->objectName() == QStringLiteral("qtooltip_label")) {
                // This is a trick to circumvent that the tooltip gets hidden immediately
                // after it gets visible. We just filter out all timer events to keep the
                // label visible.

                // Ignore the timer events to prevent from being closed
                if (e->type() == QEvent::Show) {
                    TipLabel::hideLabel();
                    _QtTipLabels.insert(std::make_pair(label, label));
                    this->hidden = false;
                }
                else if (e->type() == QEvent::Hide) {
                    this->hidden = true;
                }
                else if (e->type() == QEvent::Timer &&
                    !this->hidden && displayTime.elapsed() < 5000) {
                    return true;
                }
            }
        }
        break;
    default:
        break;
    }
    return false;
}

// ----------------------------------------------------------------------

StatusWidget::StatusWidget(QWidget* parent)
  : QDialog(parent, Qt::Dialog | Qt::FramelessWindowHint)
{
    label = new QLabel(this);
    label->setAlignment(Qt::AlignCenter);

    auto gridLayout = new QGridLayout(this);
    gridLayout->setSpacing(6);
    gridLayout->setContentsMargins(9, 9, 9, 9);
    gridLayout->addWidget(label, 0, 0, 1, 1);
}

StatusWidget::~StatusWidget()
{
}

void StatusWidget::setStatusText(const QString& s)
{
    label->setText(s);
}

void StatusWidget::showText(int ms)
{
    show();
    QTimer timer;
    QEventLoop loop;
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.start(ms);
    loop.exec(QEventLoop::ExcludeUserInputEvents);
    hide();
}

QSize StatusWidget::sizeHint () const
{
    return QSize(250,100);
}

void StatusWidget::showEvent(QShowEvent* event)
{
    QDialog::showEvent(event);
}

void StatusWidget::hideEvent(QHideEvent*)
{
}

// --------------------------------------------------------------------

class LineNumberArea : public QWidget
{
public:
    explicit LineNumberArea(PropertyListEditor *editor) : QWidget(editor) {
        codeEditor = editor;
    }

    QSize sizeHint() const override {
        return QSize(codeEditor->lineNumberAreaWidth(), 0);
    }

protected:
    void paintEvent(QPaintEvent *event) override {
        codeEditor->lineNumberAreaPaintEvent(event);
    }

private:
    PropertyListEditor *codeEditor;
};

PropertyListEditor::PropertyListEditor(QWidget *parent) : QPlainTextEdit(parent)
{
    LineEditStyle::setup(this);
    lineNumberArea = new LineNumberArea(this);

    connect(this, &QPlainTextEdit::blockCountChanged,
            this, &PropertyListEditor::updateLineNumberAreaWidth);
    connect(this, &QPlainTextEdit::updateRequest,
            this, &PropertyListEditor::updateLineNumberArea);
    connect(this, &QPlainTextEdit::cursorPositionChanged,
            this, &PropertyListEditor::highlightCurrentLine);

    updateLineNumberAreaWidth(0);
    highlightCurrentLine();
}

int PropertyListEditor::lineNumberAreaWidth()
{
    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10) {
        max /= 10;
        ++digits;
    }

    int space = 3 + QtTools::horizontalAdvance(fontMetrics(), QLatin1Char('9')) * digits;

    return space;
}

void PropertyListEditor::updateLineNumberAreaWidth(int /* newBlockCount */)
{
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void PropertyListEditor::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy)
        lineNumberArea->scroll(0, dy);
    else
        lineNumberArea->update(0, rect.y(), lineNumberArea->width(), rect.height());

    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth(0);
}

void PropertyListEditor::resizeEvent(QResizeEvent *e)
{
    QPlainTextEdit::resizeEvent(e);

    QRect cr = contentsRect();
    lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void PropertyListEditor::highlightCurrentLine()
{
    QList<QTextEdit::ExtraSelection> extraSelections;
    if (!isReadOnly()) {
        QTextEdit::ExtraSelection selection;

        QColor lineColor = QColor(Qt::yellow).lighter(160);

        selection.format.setBackground(lineColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }

    setExtraSelections(extraSelections);
}

void PropertyListEditor::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    QPainter painter(lineNumberArea);
    painter.fillRect(event->rect(), Qt::lightGray);

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = (int) blockBoundingGeometry(block).translated(contentOffset()).top();
    int bottom = top + (int) blockBoundingRect(block).height();

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            QString number = QString::number(blockNumber + 1);
            painter.setPen(Qt::black);
            painter.drawText(0, top, lineNumberArea->width(), fontMetrics().height(),
                             Qt::AlignRight, number);
        }

        block = block.next();
        top = bottom;
        bottom = top + (int) blockBoundingRect(block).height();
        ++blockNumber;
    }
}

class PropertyListDialog : public QDialog
{
    int type;

public:
    PropertyListDialog(int type, QWidget* parent) : QDialog(parent),type(type)
    {
    }

    void accept() override
    {
        auto edit = this->findChild<PropertyListEditor*>();
        QStringList lines;
        if (edit) {
            QString inputText = edit->toPlainText();
            if (!inputText.isEmpty()) // let pass empty input, regardless of the type, so user can void the value
                lines = inputText.split(QStringLiteral("\n"));
        }
        if (!lines.isEmpty()) {
            if (type == 1) { // floats
                bool ok;
                int line=1;
                for (QStringList::iterator it = lines.begin(); it != lines.end(); ++it, ++line) {
                    it->toDouble(&ok);
                    if (!ok) {
                        QMessageBox::critical(this, tr("Invalid input"), tr("Input in line %1 is not a number").arg(line));
                        return;
                    }
                }
            }
            else if (type == 2) { // integers
                bool ok;
                int line=1;
                for (QStringList::iterator it = lines.begin(); it != lines.end(); ++it, ++line) {
                    it->toInt(&ok);
                    if (!ok) {
                        QMessageBox::critical(this, tr("Invalid input"), tr("Input in line %1 is not a number").arg(line));
                        return;
                    }
                }
            }
        }
        QDialog::accept();
    }
};

// --------------------------------------------------------------------

LabelEditor::LabelEditor (QWidget * parent)
  : QWidget(parent)
{
    type = String;
    auto layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(2);

    lineEdit = new QLineEdit(this);
    layout->addWidget(lineEdit);
    LineEditStyle::setup(lineEdit);

    connect(lineEdit, &QLineEdit::textChanged,
            this, &LabelEditor::validateText);

    button = new QPushButton(QStringLiteral("..."), this);
#if defined (Q_OS_MAC)
    button->setAttribute(Qt::WA_LayoutUsesWidgetRect); // layout size from QMacStyle was not correct
#endif
    layout->addWidget(button);

    connect(button, &QPushButton::clicked, this, &LabelEditor::changeText);

    setFocusProxy(lineEdit);
}

LabelEditor::~LabelEditor()
{
}

void LabelEditor::resizeEvent(QResizeEvent* e)
{
    button->setFixedWidth(e->size().height());
    button->setFixedHeight(e->size().height());
}

QString LabelEditor::text() const
{
    return this->plainText;
}

void LabelEditor::setText(const QString& s)
{
    this->plainText = s;

    QString text = QStringLiteral("[%1]").arg(this->plainText);
    lineEdit->setText(text);
}

void LabelEditor::changeText()
{
    PropertyListDialog* dlg = new PropertyListDialog(static_cast<int>(type), this);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->setWindowTitle(tr("List"));

    auto hboxLayout = new QVBoxLayout(dlg);
    auto buttonBox = new QDialogButtonBox(dlg);
    buttonBox->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    auto edit = new PropertyListEditor(dlg);
    edit->setPlainText(this->plainText);

    hboxLayout->addWidget(edit);
    hboxLayout->addWidget(buttonBox);
    connect(buttonBox, &QDialogButtonBox::accepted, dlg, &PropertyListDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, dlg, &PropertyListDialog::reject);
    connect(dlg, &PropertyListDialog::accepted, this, [&] {
        QString inputText = edit->toPlainText();
        QString text = QStringLiteral("[%1]").arg(inputText);
        lineEdit->setText(text);
    });

    dlg->exec();
}

/**
 * Validates if the input of the lineedit is a valid list.
 */
void LabelEditor::validateText(const QString& text)
{
    if (text.startsWith(QStringLiteral("[")) && text.endsWith(QStringLiteral("]"))) {
        this->plainText = text.mid(1, text.size()-2);
        Q_EMIT textChanged(this->plainText);
    }
}

/**
 * Sets the browse button's text to \a txt.
 */
void LabelEditor::setButtonText(const QString& txt)
{
    button->setText(txt);
    int w1 = 2 * QtTools::horizontalAdvance(button->fontMetrics(), txt);
    int w2 = 2 * QtTools::horizontalAdvance(button->fontMetrics(), QStringLiteral(" ... "));
    button->setFixedWidth((w1 > w2 ? w1 : w2));
}

/**
 * Returns the browse button's text.
 */
QString LabelEditor::buttonText() const
{
    return button->text();
}

void LabelEditor::setInputType(InputType t)
{
    this->type = t;
}

// --------------------------------------------------------------------

ExpLineEdit::ExpLineEdit(QWidget* parent, bool expressionOnly)
    : QLineEdit(parent), autoClose(expressionOnly)
{
    makeLabel(this);

    QObject::connect(iconLabel, &ExpressionLabel::clicked, this, &ExpLineEdit::openFormulaDialog);
    if (expressionOnly)
        QMetaObject::invokeMethod(this, "openFormulaDialog", Qt::QueuedConnection, QGenericReturnArgument());
}

bool ExpLineEdit::apply(const std::string& propName) {

    if (!ExpressionBinding::apply(propName)) {
        if(!autoClose) {
            QString val = QString::fromUtf8(Base::Interpreter().strToPython(text().toUtf8()).c_str());
            Gui::Command::doCommand(Gui::Command::Doc, "%s = \"%s\"", propName.c_str(), val.constData());
        }
        return true;
    }

    return false;
}

void ExpLineEdit::bind(const ObjectIdentifier& _path) {

    ExpressionBinding::bind(_path);
}

void ExpLineEdit::setExpression(std::shared_ptr<Expression> expr)
{
    Q_ASSERT(isBound());

    try {
        ExpressionBinding::setExpression(expr);
    }
    catch (const Base::Exception &) {
        setReadOnly(true);
        QPalette p(palette());
        p.setColor(QPalette::Active, QPalette::Text, Qt::red);
        setPalette(p);
    }
}

void ExpLineEdit::onChange() {

    if (hasExpression()) {
        // Do NOT update property item text with expression evaluation result,
        // because some property has different output and input value.
        //
        // std::unique_ptr<Expression> result(getExpression()->eval());
        // if(result->isDerivedFrom(App::StringExpression::getClassTypeId()))
        //     setText(QString::fromUtf8(static_cast<App::StringExpression*>(
        //                     result.get())->getText().c_str()));
        // else
        //     setText(QString::fromUtf8(result->toString().c_str()));

        setReadOnly(true);
        QPalette p(palette());
        p.setColor(QPalette::Text, Qt::lightGray);
        setPalette(p);
    }
    else {
        setReadOnly(false);
        QPalette p(palette());
        p.setColor(QPalette::Active, QPalette::Text, defaultPalette.color(QPalette::Text));
        setPalette(p);
    }
}

void ExpLineEdit::resizeEvent(QResizeEvent * event)
{
    QLineEdit::resizeEvent(event);

    try {
        if (isBound() && hasExpression()) {
            setReadOnly(true);
            QPalette p(palette());
            p.setColor(QPalette::Text, Qt::lightGray);
            setPalette(p);
        }
        else {
            setReadOnly(false);
            QPalette p(palette());
            p.setColor(QPalette::Active, QPalette::Text, defaultPalette.color(QPalette::Text));
            setPalette(p);
        }
    }
    catch (const Base::Exception &) {
        setReadOnly(true);
        QPalette p(palette());
        p.setColor(QPalette::Active, QPalette::Text, Qt::red);
        setPalette(p);
    }
}

void ExpLineEdit::openFormulaDialog()
{
    Q_ASSERT(isBound());

    auto box = new Gui::Dialog::DlgExpressionInput(
            getPath(), getExpression(),Unit(), this);
    connect(box, &Dialog::DlgExpressionInput::finished, this, &ExpLineEdit::finishFormulaDialog);
    box->show();

    box->setExpressionInputSize(width(), height());
}

void ExpLineEdit::finishFormulaDialog()
{
    auto box = qobject_cast<Gui::Dialog::DlgExpressionInput*>(sender());
    if (!box) {
        qWarning() << "Sender is not a Gui::Dialog::DlgExpressionInput";
        return;
    }

    if (box->result() == QDialog::Accepted)
        setExpression(box->getExpression());
    else if (box->discardedFormula())
        setExpression(std::shared_ptr<Expression>());

    box->deleteLater();

    if(autoClose)
        this->deleteLater();
}

void ExpLineEdit::keyPressEvent(QKeyEvent *event)
{
    if (!hasExpression())
        QLineEdit::keyPressEvent(event);
}

// --------------------------------------------------------------------
LineEditStyle::LineEditStyle(QStyle *style)
    :QProxyStyle(style)
{
    cursor_width = ViewParams::getTextCursorWidth();
}

void LineEditStyle::setup(QLineEdit *le)
{
    if (!le) return;
    if (auto style = qobject_cast<LineEditStyle*>(le->style())) {
        if (style->cursorWidth() != ViewParams::getTextCursorWidth()) {
            style->setCursorWidth(ViewParams::getTextCursorWidth());
            if (le->hasFocus())
                le->update();
        }
    } else
        le->setStyle(new LineEditStyle(le->style()));
}

void LineEditStyle::setup(QTextEdit *editor)
{
    if (editor)
        editor->setCursorWidth(ViewParams::getTextCursorWidth());
}

void LineEditStyle::setup(QPlainTextEdit *editor)
{
    if (editor)
        editor->setCursorWidth(ViewParams::getTextCursorWidth());
}

void LineEditStyle::setupWidget(QWidget *w)
{
    if (!w) return;
    if (auto editor = qobject_cast<QLineEdit*>(w))
        setup(editor);
    else if (auto editor = qobject_cast<QTextEdit*>(w))
        setup(editor);
    else if (auto editor = qobject_cast<QPlainTextEdit*>(w))
        setup(editor);
}

void LineEditStyle::setupChildren(QObject *o)
{
    if (!o) return;
    for (auto w : o->findChildren<QWidget*>())
        setupWidget(w);
}

void LineEditStyle::setupObject(QObject *o)
{
    if (o && o->isWidgetType())
        setupWidget(static_cast<QWidget*>(o));
}

// --------------------------------------------------------------------

ButtonGroup::ButtonGroup(QObject *parent)
  : QButtonGroup(parent)
  , _exclusive(true)
{
    QButtonGroup::setExclusive(false);

    connect(this, qOverload<QAbstractButton *>(&QButtonGroup::buttonClicked),
            [=](QAbstractButton *button) {
        if (exclusive()) {
            const auto btns = buttons();
            for (auto btn : btns) {
                if (btn && btn != button && btn->isCheckable())
                    btn->setChecked(false);
            }
        }
    });
}

void ButtonGroup::setExclusive(bool on)
{
    _exclusive = on;
}

bool ButtonGroup::exclusive() const
{
    return _exclusive;
}

#include "moc_Widgets.cpp"
