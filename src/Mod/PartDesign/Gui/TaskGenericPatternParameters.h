/***************************************************************************
 *   Copyright (c) 2020 Zheng Lei (realthunder)<realthunder.dev@gmail.com> *
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

#ifndef GUI_TASKVIEW_TaskGenericPatternParameters_H
#define GUI_TASKVIEW_TaskGenericPatternParameters_H

#include <memory>
#include <App/DocumentObject.h>
#include <Gui/TaskView/TaskView.h>
#include <Gui/Selection.h>
#include <Gui/TaskView/TaskDialog.h>

#include "TaskTransformedParameters.h"
#include "ViewProviderGenericPattern.h"

class QTimer;
class Ui_TaskGenericPatternParameters;

namespace App {
class Property;
}

namespace Gui {
class ViewProvider;
}

namespace PartDesignGui {

class TaskMultiTransformParameters;

class TaskGenericPatternParameters : public TaskTransformedParameters
{
    Q_OBJECT

public:
    /// Constructor for task with ViewProvider
    TaskGenericPatternParameters(ViewProviderTransformed *TransformedView, QWidget *parent = nullptr);
    /// Constructor for task with parent task (MultiTransform mode)
    TaskGenericPatternParameters(TaskMultiTransformParameters *parentTask, QLayout *layout);
    ~TaskGenericPatternParameters() override;

    void apply() override;

private Q_SLOTS:
    void onChangedExpression();
    void onUpdateView(bool) override;

protected:
    void changeEvent(QEvent *e) override;

private:
    void setupUI();
    void updateUI() override;

private:
    Ui_TaskGenericPatternParameters* ui;
    std::shared_ptr<App::Expression> expr;
};


/// simulation dialog for the TaskView
class TaskDlgGenericPatternParameters : public TaskDlgTransformedParameters
{
    Q_OBJECT

public:
    TaskDlgGenericPatternParameters(ViewProviderGenericPattern *GenericPatternView);
    ~TaskDlgGenericPatternParameters() override
    {
    }
};

} //namespace PartDesignGui

#endif // GUI_TASKVIEW_TASKAPPERANCE_H
