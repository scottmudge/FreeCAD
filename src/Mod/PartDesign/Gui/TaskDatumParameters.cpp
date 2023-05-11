/***************************************************************************
 *   Copyright (c) 2013 Jan Rheinländer                                    *
 *                                   <jrheinlaender@users.sourceforge.net> *
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
# include <QMessageBox>
# include <Standard_Failure.hxx>
#endif

#include <App/DocumentObject.h>
#include <App/Origin.h>
#include <App/Part.h>
#include <Gui/MainWindow.h>
#include <Gui/ViewProvider.h>
#include <Gui/Selection.h>
#include <Mod/Part/App/DatumFeature.h>
#include <Mod/PartDesign/App/Body.h>

#include <ui_DlgReference.h>

#include "TaskDatumParameters.h"
#include "ReferenceSelection.h"
#include "TaskFeaturePick.h"
#include "Utils.h"

using namespace PartDesignGui;
using namespace Attacher;

/* TRANSLATOR PartDesignGui::TaskDatumParameters */

TaskDatumParameters::TaskDatumParameters(ViewProviderDatum *ViewProvider,QWidget *parent)
    : PartGui::TaskAttacher(ViewProvider, parent, QStringLiteral("PartDesign_") + ViewProvider->datumType,
              ViewProvider->datumText + tr(" parameters"))
{
    Gui::Selection().addSelectionGate(new NoDependentsSelection(ViewProvider->getObject()));
    ViewProvider->setPickable(false);
}

TaskDatumParameters::~TaskDatumParameters()
{
    if(this->ViewProvider && this->ViewProvider->isDerivedFrom(ViewProviderDatum::getClassTypeId()))
        static_cast<ViewProviderDatum*>(this->ViewProvider)->setPickable(true);
    Gui::Selection().rmvSelectionGate();
}


//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgDatumParameters::TaskDlgDatumParameters(ViewProviderDatum *ViewProvider)
    : TaskDlgAttacher(ViewProvider, false)
{
    assert(ViewProvider);
    parameter  = new TaskDatumParameters(ViewProvider);
    Content.push_back(parameter);
    if (parameter->getPartner())
        Content.push_back(parameter->getPartner());
}

TaskDlgDatumParameters::~TaskDlgDatumParameters()
{

}

bool TaskDlgDatumParameters::reject() {

    return PartGui::TaskDlgAttacher::reject();
}


bool TaskDlgDatumParameters::accept() {

    //see if we are able to assign a mode
    if (parameter->getActiveMapMode() == mmDeactivated) {
        QMessageBox msg;
        msg.setWindowTitle(tr("Incompatible reference set"));
        msg.setText(tr("There is no attachment mode that fits the current set"
        " of references. If you choose to continue, the feature will remain where"
        " it is now, and will not be moved as the references change."
        " Continue?"));
        msg.addButton(QMessageBox::Yes);
        auto btNo =  msg.addButton(QMessageBox::No);
        msg.setDefaultButton(btNo);
        msg.setIcon(QMessageBox::Warning);
        msg.exec();
        if (msg.buttonRole(msg.clickedButton()) == QMessageBox::NoRole)
            return false;
    }

    if (!PartGui::TaskDlgAttacher::accept())
        return false;

    return true;
}

#include "moc_TaskDatumParameters.cpp"
