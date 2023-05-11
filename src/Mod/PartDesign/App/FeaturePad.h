/***************************************************************************
 *   Copyright (c) 2010 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
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


#ifndef PARTDESIGN_Pad_H
#define PARTDESIGN_Pad_H

#include "FeatureExtrude.h"

namespace PartDesign
{

class PartDesignExport Pad : public FeatureExtrude
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartDesign::Pad);

public:
    Pad(const char *propertyGroupName="Pad");

    App::DocumentObjectExecReturn *execute() override;
    /// returns the type name of the view provider
    const char* getViewProviderName() const override {
        return "PartDesignGui::ViewProviderPad";
    }

protected:
    App::DocumentObjectExecReturn * _execute(bool makesface, bool fuse);

protected:
    static const char* TypeEnums[];
};

} //namespace PartDesign


#endif // PART_Pad_H
