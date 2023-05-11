/***************************************************************************
 *   Copyright (c) 2022 Uwe Stöhr <uwestoehr@lyx.org>                      *
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

#ifndef PART_EXTRUSIONHELPER_H
#define PART_EXTRUSIONHELPER_H

#include <list>
#include <vector>
#include <gp_Dir.hxx>
#include <TopoDS_Shape.hxx>

#include <Mod/Part/PartGlobal.h>
#include "TopoShape.h"


namespace Part
{

class PartExport ExtrusionHelper
{
public:
    ExtrusionHelper();

    /**
     * @brief The ExtrusionParameters struct is supposed to be filled with final
     * extrusion parameters, after resolving links, applying mode logic,
     * reversing, etc., and be passed to extrudeShape.
     */
    struct PartExport Parameters {
        gp_Dir dir;
        double lengthFwd;
        double lengthRev;
        bool solid;
        bool innertaper;
        bool usepipe;
        bool linearize;
        double taperAngleFwd; //in radians
        double taperAngleRev;
        double innerTaperAngleFwd; //in radians
        double innerTaperAngleRev;
        std::string faceMakerClass;
        Parameters();
    };

    /**
     * @brief makeDraft: creates a drafted extrusion shape out of the input 2D shape
     */
    static void makeDraft(const Parameters& params,
                          const TopoShape& shape, 
                          std::vector<TopoShape>& drafts,
                          App::StringHasherRef hasher);
};

} //namespace Part

#endif // PART_EXTRUSIONHELPER_H
