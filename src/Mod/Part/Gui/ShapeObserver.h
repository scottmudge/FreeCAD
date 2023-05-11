/****************************************************************************
 *   Copyright (c) 2023 Zheng Lei (realthunder) <realthunder.dev@gmail.com> *
 *                                                                          *
 *   This file is part of the FreeCAD CAx development system.               *
 *                                                                          *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Library General Public            *
 *   License as published by the Free Software Foundation; either           *
 *   version 2 of the License, or (at your option) any later version.       *
 *                                                                          *
 *   This library  is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Library General Public License for more details.                   *
 *                                                                          *
 *   You should have received a copy of the GNU Library General Public      *
 *   License along with this library; see the file COPYING.LIB. If not,     *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,          *
 *   Suite 330, Boston, MA  02111-1307, USA                                 *
 *                                                                          *
 ****************************************************************************/

#ifndef PART_SHAPE_OBSERVER_H
#define PART_SHAPE_OBSERVER_H

#include <App/DocumentObserver.h>
#include <Base/BoundBox.h>
#include <Gui/Selection.h>
#include <Mod/Part/PartGlobal.h>
#include <Mod/Part/App/TopoShape.h>

namespace PartGui {

class PartGuiExport ShapeObserver: public Gui::SelectionObserver
{
public:
    ShapeObserver();
    static ShapeObserver *instance();

    bool hasShape();
    const std::vector<Part::TopoShape> &getWholeShapes();
    const std::vector<Part::TopoShape> &getShapeElements();
    const Base::BoundBox3d &getBoundBox();
    bool hasNonSolid();

    void onSelectionChanged(const Gui::SelectionChanges& msg) override;

private:
    bool _hasShape = false;
    bool _checked = false;
    bool _shapeChecked = false;
    bool _hasNoneSolid = false;
    bool _solidsChecked = false;
    bool _elementChecked = false;
    bool _hasBoundBox = false;
    std::vector<Part::TopoShape> _shapes;
    std::set<App::SubObjectT> _sels;
    std::vector<Part::TopoShape> _shapeElements;
    Base::BoundBox3d _boundBox;
};

} // namespace PartGui

#endif // PART_SHAPE_OBSERVER_H
