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


#ifndef PARTDESIGN_EXTRUSION_H
#define PARTDESIGN_EXTRUSION_H

#include "FeaturePad.h"

namespace PartDesign
{

class PartDesignExport Extrusion : public Pad
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartDesign::Extrusion);

public:
    Extrusion();

    App::DocumentObjectExecReturn *execute() override;

    const char* getViewProviderName() const override {
        return "PartDesignGui::ViewProviderExtrusion";
    }
};

} //namespace PartDesign


#endif // PARTDESIGN_EXTRUSION_H
