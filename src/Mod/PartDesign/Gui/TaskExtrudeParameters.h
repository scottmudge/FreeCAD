/***************************************************************************
 *   Copyright (c) 2021 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#ifndef GUI_TASKVIEW_TaskExtrudeParameters_H
#define GUI_TASKVIEW_TaskExtrudeParameters_H

#include "TaskSketchBasedParameters.h"
#include "ViewProviderSketchBased.h"


class Ui_TaskPadPocketParameters;

namespace App {
class Property;
}

namespace PartDesign {
class ProfileBased;
}

namespace PartDesignGui {


class TaskExtrudeParameters : public TaskSketchBasedParameters
{
    Q_OBJECT

    enum DirectionModes {
        Normal,
        Select,
        Custom,
        Reference
    };

public:
    TaskExtrudeParameters(ViewProviderSketchBased *SketchBasedView, QWidget *parent,
                          const std::string& pixmapname, const QString& parname);
    ~TaskExtrudeParameters() override;

    void saveHistory() override;
    void refresh() override;

    void fillDirectionCombo();
    void applyParameters(QString facename);

    virtual bool isPocket() {
        return false;
    }

    enum class Modes {
        Dimension,
        ThroughAll,
        ToLast = ThroughAll,
        ToFirst,
        ToFace,
        TwoDimensions
    };

protected:
    void onLengthChanged(double);
    void onLength2Changed(double);
    void onOffsetChanged(double);
    void onTaperChanged(double);
    void onTaper2Changed(double);
    void onInnerAngleChanged(double);
    void onInnerAngle2Changed(double);
    void onDirectionCBChanged(int);
    void onAlongSketchNormalChanged(bool);
    void onDirectionToggled(bool);
    void onXDirectionEditChanged(double);
    void onYDirectionEditChanged(double);
    void onZDirectionEditChanged(double);
    void onMidplaneChanged(bool);
    void onReversedChanged(bool);
    void onButtonFace(const bool checked = true);
    void onFaceName(const QString& text);
    void onUsePipeChanged(bool);
    void onCheckFaceLimitsChanged(bool);
    void onSelectionModeChanged(SelectionMode);

protected:
    void changeEvent(QEvent *e) override;
    bool eventFilter(QObject *o, QEvent *ev) override;
    void _onSelectionChanged(const Gui::SelectionChanges& msg) override;

    void setCheckboxes();
    void setupDialog(bool newObj, const char *historyPath);
    void readValuesFromHistory();
    App::PropertyLinkSub* propReferenceAxis;
    void getReferenceAxis(App::DocumentObject*& obj, std::vector<std::string>& sub) const;

    double getOffset(void) const;
    bool   getAlongSketchNormal(void) const;
    bool   getCustom(void) const;
    std::string getReferenceAxis(void) const;
    double getXDirection(void) const;
    double getYDirection(void) const;
    double getZDirection(void) const;
    bool   getReversed(void) const;
    bool   getMidplane(void) const;
    int    getMode(void) const;
    QString getFaceName(void) const;
    void updateDirectionEdits(void);
    void setDirectionMode(int index);
    void addAxisToCombo(App::DocumentObject* linkObj, const std::string &linkSubname, const QString &itemText);

    virtual void onModeChanged(int);
    virtual void translateTooltips();
    virtual void translateModeList(int index);
    virtual void translateFaceNamePlaceHolder();

private:
    void tryRecomputeFeature();
    void translateFaceName();
    void connectSlots();
    bool hasProfileFace(PartDesign::ProfileBased*) const;
    void selectedReferenceAxis(const Gui::SelectionChanges& msg);
    void clearFaceName();

protected:
    QWidget* proxy;
    std::unique_ptr<Ui_TaskPadPocketParameters> ui;
    bool selectionFace;
    std::vector<App::SubObjectT> axesInList;
};

} //namespace PartDesignGui

#endif // GUI_TASKVIEW_TaskExtrudeParameters_H
