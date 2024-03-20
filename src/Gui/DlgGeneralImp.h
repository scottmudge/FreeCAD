// SPDX-License-Identifier: LGPL-2.1-or-later

 /****************************************************************************
  *   Copyright (c) 2004 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef GUI_DIALOG_DLGGENERALIMP_H
#define GUI_DIALOG_DLGGENERALIMP_H

#include "PropertyPage.h"
#include <memory>
#include <string>

class QTabWidget;
class QComboBox;

namespace Gui {

class PrefComboBox;

namespace Dialog {
class Ui_DlgGeneral;

/** This class implements the settings for the application.
 *  You can change window style, size of pixmaps, size of recent file list and so on
 *  \author Werner Mayer
 */
class DlgGeneralImp : public PreferencePage
{
    Q_OBJECT

public:
    explicit DlgGeneralImp( QWidget* parent = nullptr );
    ~DlgGeneralImp() override;

    static void attachObserver();

    void saveSettings() override;
    void loadSettings() override;

    static void populateStylesheets(const char *key,
                                    const char *path,
                                    PrefComboBox *combo,
                                    const char *def,
                                    QStringList filter = QStringList());
    void setupToolBarIconSize();

protected:
    void updateLanguage();
    void changeEvent(QEvent *event) override;

private:
    void setRecentFileSize();

public Q_SLOTS:
    void onUnitSystemIndexChanged(int index);

private:
    std::unique_ptr<Ui_DlgGeneral> ui;
};

} // namespace Dialog
} // namespace Gui

#endif // GUI_DIALOG_DLGGENERALIMP_H
