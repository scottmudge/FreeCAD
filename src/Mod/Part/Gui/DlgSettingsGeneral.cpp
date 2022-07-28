/***************************************************************************
 *   Copyright (c) 2007 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <QButtonGroup>
# include <QRegExp>
# include <QRegExpValidator>
# include <Interface_Static.hxx>
#endif

#include <boost/algorithm/string/predicate.hpp>
#include <Base/Parameter.h>
#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/DocumentObserver.h>
#include "DlgSettingsGeneral.h"
#include "ui_DlgSettingsGeneral.h"
#include "ui_DlgImportExportIges.h"
#include "ui_DlgImportExportStep.h"

using namespace PartGui;

DlgSettingsGeneral::DlgSettingsGeneral(QWidget* parent)
  : PreferencePage(parent), ui(new Ui_DlgSettingsGeneral)
{
    ui->setupUi(this);
    QObject::connect(ui->btnAuxGroup, &QPushButton::pressed, [this]() {
        bool checked = ui->checkBoxAuxGroup->isChecked();
        const char *typeName = "PartDesign::AuxGroup";
        try {
            Base::Type::importModule(typeName);
            auto type = Base::Type::fromName(typeName);

            std::vector<std::pair<App::DocumentObjectT, const char*>> objs;

            auto checkName = [&objs](App::DocumentObject *obj, const char *name) {
                if (boost::starts_with(obj->getNameInDocument(), name)) {
                    objs.emplace_back(obj, name);
                    return true;
                }
                return false;
            };

            for (auto doc : App::GetApplication().getDocuments()) {
                if (doc->testStatus(App::Document::PartialDoc))
                    continue;
                for (auto obj : doc->getObjects()) {
                    if (!obj->isDerivedFrom(type)) continue;
                    if (!checkName(obj, "Sketches") && !checkName(obj, "Datums"))
                        checkName(obj, "Misc");
                }
            }

            if (checked) {
                // About to change back to unique indexed label (i.e. the
                // group's internal name). There may be a small chance of label
                // conflict here where a group uses an indexed label that
                // happens to be another group's internal name. So we first
                // change all groups to some temporary name, and later reset
                // their label to their internal name together.
                std::string tmp;
                for (auto &v : objs) {
                    if (auto obj = v.first.getObject()) {
                        tmp = std::string(v.second) + "_";
                        obj->Label.setValue(tmp);
                    }
                }
                for (auto &v : objs) {
                    if (auto obj = v.first.getObject())
                        obj->Label.setValue(obj->getNameInDocument());
                }
            } else {
                for (auto &v : objs) {
                    if (auto obj = v.first.getObject())
                        obj->Label.setValue(v.second);
                }
            }
        } catch (Base::Exception &e) {
            e.ReportException();
            return;
        }
    });
}

/**
 *  Destroys the object and frees any allocated resources
 */
DlgSettingsGeneral::~DlgSettingsGeneral()
{
    // no need to delete child widgets, Qt does it all for us
}

void DlgSettingsGeneral::saveSettings()
{
    ui->checkBooleanCheck->onSave();
    ui->checkBooleanRefine->onSave();
    ui->checkSketchBaseRefine->onSave();
    ui->checkLinearize->onSave();
    ui->checkSingleSolid->onSave();
    ui->checkObjectNaming->onSave();
    ui->comboBoxCommandOverride->onSave();
    ui->comboBoxWrapFeature->onSave();
    ui->checkAutoGroupSolids->onSave();
    ui->checkBoxAuxGroup->onSave();
    ui->checkSplitEllipsoid->onSave();
    ui->checkAutoAuxiliaryGrouping->onSave();
    ui->checkAutoHideOrigins->onSave();
}

void DlgSettingsGeneral::loadSettings()
{
    ui->checkBooleanCheck->onRestore();
    ui->checkBooleanRefine->onRestore();
    ui->checkSketchBaseRefine->onRestore();
    ui->checkLinearize->onRestore();
    ui->checkSingleSolid->onRestore();
    ui->checkObjectNaming->onRestore();
    ui->comboBoxCommandOverride->onRestore();
    ui->comboBoxWrapFeature->onRestore();
    ui->checkAutoGroupSolids->onRestore();
    ui->checkBoxAuxGroup->onRestore();
    ui->checkSplitEllipsoid->onRestore();
    ui->checkAutoAuxiliaryGrouping->onRestore();
    ui->checkAutoHideOrigins->onRestore();
}

/**
 * Sets the strings of the subwidgets using the current language.
 */
void DlgSettingsGeneral::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    else {
        QWidget::changeEvent(e);
    }
}

// ----------------------------------------------------------------------------

DlgImportExportIges::DlgImportExportIges(QWidget* parent)
  : PreferencePage(parent), ui(new Ui_DlgImportExportIges)
{
    ui->setupUi(this);
    ui->lineEditProduct->setReadOnly(true);

    bg = new QButtonGroup(this);
    bg->addButton(ui->radioButtonBRepOff, 0);
    bg->addButton(ui->radioButtonBRepOn, 1);

    QRegExp rx;
    rx.setPattern(QStringLiteral("[\\x00-\\x7F]+"));
    QRegExpValidator* companyValidator = new QRegExpValidator(ui->lineEditCompany);
    companyValidator->setRegExp(rx);
    ui->lineEditCompany->setValidator(companyValidator);
    QRegExpValidator* authorValidator = new QRegExpValidator(ui->lineEditAuthor);
    authorValidator->setRegExp(rx);
    ui->lineEditAuthor->setValidator(authorValidator);
}

/**
 *  Destroys the object and frees any allocated resources
 */
DlgImportExportIges::~DlgImportExportIges()
{
    // no need to delete child widgets, Qt does it all for us
}

void DlgImportExportIges::saveSettings()
{
    int unit = ui->comboBoxUnits->currentIndex();
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/Part")->GetGroup("IGES");
    hGrp->SetInt("Unit", unit);
    switch (unit) {
        case 1:
            Interface_Static::SetCVal("write.iges.unit","M");
            break;
        case 2:
            Interface_Static::SetCVal("write.iges.unit","INCH");
            break;
        default:
            Interface_Static::SetCVal("write.iges.unit","MM");
            break;
    }

    hGrp->SetBool("BrepMode", bg->checkedId() == 1);
    Interface_Static::SetIVal("write.iges.brep.mode", bg->checkedId());

    // Import
    hGrp->SetBool("SkipBlankEntities", ui->checkSkipBlank->isChecked());

    // header info
    hGrp->SetASCII("Company", ui->lineEditCompany->text().toUtf8());
    hGrp->SetASCII("Author", ui->lineEditAuthor->text().toUtf8());
  //hGrp->SetASCII("Product", ui->lineEditProduct->text().toUtf8());

    Interface_Static::SetCVal("write.iges.header.company", ui->lineEditCompany->text().toUtf8());
    Interface_Static::SetCVal("write.iges.header.author", ui->lineEditAuthor->text().toUtf8());
  //Interface_Static::SetCVal("write.iges.header.product", ui->lineEditProduct->text().toUtf8());
}

void DlgImportExportIges::loadSettings()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/Part")->GetGroup("IGES");
    int unit = hGrp->GetInt("Unit", 0);
    ui->comboBoxUnits->setCurrentIndex(unit);

    int value = Interface_Static::IVal("write.iges.brep.mode");
    bool brep = hGrp->GetBool("BrepMode", value > 0);
    if (brep)
        ui->radioButtonBRepOn->setChecked(true);
    else
        ui->radioButtonBRepOff->setChecked(true);

    // Import
    ui->checkSkipBlank->setChecked(hGrp->GetBool("SkipBlankEntities", true));

    // header info
    ui->lineEditCompany->setText(QString::fromStdString(hGrp->GetASCII("Company",
        Interface_Static::CVal("write.iges.header.company"))));
    ui->lineEditAuthor->setText(QString::fromStdString(hGrp->GetASCII("Author",
        Interface_Static::CVal("write.iges.header.author"))));
  //ui->lineEditProduct->setText(QString::fromStdString(hGrp->GetASCII("Product")));
    ui->lineEditProduct->setText(QString::fromUtf8(
        Interface_Static::CVal("write.iges.header.product")));
}

/**
 * Sets the strings of the subwidgets using the current language.
 */
void DlgImportExportIges::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    else {
        QWidget::changeEvent(e);
    }
}

// ----------------------------------------------------------------------------

DlgImportExportStep::DlgImportExportStep(QWidget* parent)
  : PreferencePage(parent), ui(new Ui_DlgImportExportStep)
{
    ui->setupUi(this);

    ui->comboBoxSchema->setItemData(0, QByteArray("AP203"));
    ui->comboBoxSchema->setItemData(1, QByteArray("AP214CD"));
    ui->comboBoxSchema->setItemData(2, QByteArray("AP214DIS"));
    ui->comboBoxSchema->setItemData(3, QByteArray("AP214IS"));
    ui->comboBoxSchema->setItemData(4, QByteArray("AP242DIS"));
    ui->lineEditProduct->setReadOnly(true);
    //ui->radioButtonAP203->setToolTip(tr("Configuration controlled 3D designs of mechanical parts and assemblies"));
    //ui->radioButtonAP214->setToolTip(tr("Core data for automotive mechanical design processes"));

    // https://tracker.dev.opencascade.org/view.php?id=25654
    ui->checkBoxPcurves->setToolTip(tr("This parameter indicates whether parametric curves (curves in parametric space of surface)\n"
                                       "should be written into the STEP file. This parameter can be set to off in order to minimize\n"
                                       "the size of the resulting STEP file."));

    QRegExp rx;
    rx.setPattern(QStringLiteral("[\\x00-\\x7F]+"));
    QRegExpValidator* companyValidator = new QRegExpValidator(ui->lineEditCompany);
    companyValidator->setRegExp(rx);
    ui->lineEditCompany->setValidator(companyValidator);
    QRegExpValidator* authorValidator = new QRegExpValidator(ui->lineEditAuthor);
    authorValidator->setRegExp(rx);
    ui->lineEditAuthor->setValidator(authorValidator);
}

/**
 *  Destroys the object and frees any allocated resources
 */
DlgImportExportStep::~DlgImportExportStep()
{
    // no need to delete child widgets, Qt does it all for us
}

void DlgImportExportStep::saveSettings()
{
    int unit = ui->comboBoxUnits->currentIndex();
    Base::Reference<ParameterGrp> hPartGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/Part");

    // General
    Base::Reference<ParameterGrp> hGenGrp = hPartGrp->GetGroup("General");
    int writesurfacecurve = ui->checkBoxPcurves->isChecked() ? 1 : 0;
    hGenGrp->SetInt("WriteSurfaceCurveMode", writesurfacecurve);
    Interface_Static::SetIVal("write.surfacecurve.mode", writesurfacecurve);

    // STEP
    Base::Reference<ParameterGrp> hStepGrp = hPartGrp->GetGroup("STEP");
    hStepGrp->SetInt("Unit", unit);
    switch (unit) {
        case 1:
            Interface_Static::SetCVal("write.step.unit","M");
            break;
        case 2:
            Interface_Static::SetCVal("write.step.unit","INCH");
            break;
        default:
            Interface_Static::SetCVal("write.step.unit","MM");
            break;
    }

    // scheme
    // possible values: AP214CD (1996), AP214DIS (1998), AP214IS (2002), AP242DIS
    QByteArray schema = ui->comboBoxSchema->itemData(ui->comboBoxSchema->currentIndex()).toByteArray();
    Interface_Static::SetCVal("write.step.schema",schema);
    hStepGrp->SetASCII("Scheme", schema);

    // header info
    hStepGrp->SetASCII("Company", ui->lineEditCompany->text().toUtf8());
    hStepGrp->SetASCII("Author", ui->lineEditAuthor->text().toUtf8());
  //hStepGrp->SetASCII("Product", ui->lineEditProduct->text().toUtf8());

    // (h)STEP of Import module
    ui->checkBoxMergeCompound->onSave();
    ui->checkBoxExportHiddenObj->onSave();
    ui->checkBoxExportLegacy->onSave();
    ui->checkBoxKeepPlacement->onSave();
    ui->checkBoxImportHiddenObj->onSave();
    ui->checkBoxLegacyImporter->onSave();
    ui->checkBoxUseAppPart->onSave();
    ui->checkBoxUseBaseName->onSave();
    ui->checkBoxReduceObjects->onSave();
    ui->checkBoxShowProgress->onSave();
    ui->comboBoxImportMode->onSave();
}

void DlgImportExportStep::loadSettings()
{
    Base::Reference<ParameterGrp> hPartGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/Part");

    // General
    Base::Reference<ParameterGrp> hGenGrp = hPartGrp->GetGroup("General");
    int writesurfacecurve = Interface_Static::IVal("write.surfacecurve.mode");
    writesurfacecurve = hGenGrp->GetInt("WriteSurfaceCurveMode", writesurfacecurve);
    ui->checkBoxPcurves->setChecked(writesurfacecurve == 0 ? false : true);

    // STEP
    Base::Reference<ParameterGrp> hStepGrp = hPartGrp->GetGroup("STEP");
    int unit = hStepGrp->GetInt("Unit", 0);
    ui->comboBoxUnits->setCurrentIndex(unit);

    // scheme
    QByteArray ap(hStepGrp->GetASCII("Scheme", Interface_Static::CVal("write.step.schema")).c_str());
    int index = ui->comboBoxSchema->findData(QVariant(ap));
    if (index >= 0)
        ui->comboBoxSchema->setCurrentIndex(index);

    // header info
    ui->lineEditCompany->setText(QString::fromStdString(hStepGrp->GetASCII("Company")));
    ui->lineEditAuthor->setText(QString::fromStdString(hStepGrp->GetASCII("Author")));
    ui->lineEditProduct->setText(QString::fromUtf8(
        Interface_Static::CVal("write.step.product.name")));

    // (h)STEP of Import module
    ui->checkBoxMergeCompound->onRestore();
    ui->checkBoxExportHiddenObj->onRestore();
    ui->checkBoxExportLegacy->onRestore();
    ui->checkBoxKeepPlacement->onRestore();
    ui->checkBoxImportHiddenObj->onRestore();
    ui->checkBoxLegacyImporter->onRestore();
    ui->checkBoxUseAppPart->onRestore();
    ui->checkBoxUseBaseName->onRestore();
    ui->checkBoxReduceObjects->onRestore();
    ui->checkBoxShowProgress->onRestore();
    ui->comboBoxImportMode->onRestore();
}

/**
 * Sets the strings of the subwidgets using the current language.
 */
void DlgImportExportStep::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    else {
        QWidget::changeEvent(e);
    }
}

#include "moc_DlgSettingsGeneral.cpp"
