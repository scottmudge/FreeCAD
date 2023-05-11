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

#include "PreCompiled.h"
#ifndef _PreComp_
# include <TopoDS_Shape.hxx>
# include <TopExp_Explorer.hxx>
#endif
#include <Mod/Part/App/PartFeature.h>
#include "ShapeObserver.h"

using namespace PartGui;

ShapeObserver::ShapeObserver()
{
}

ShapeObserver *ShapeObserver::instance()
{
    static ShapeObserver *inst = new ShapeObserver;
    return inst;
}

bool ShapeObserver::hasShape()
{
    if (_checked)
        return _hasShape;
    _checked = true;
    auto sels = Gui::Selection().getSelectionT();
    _hasShape = false;
    for (const auto &sel : sels) {
        Part::TopoShape shp = Part::Feature::getTopoShape(sel.getObject(),
                                                            sel.getSubName().c_str(),
                                                            /*needElement*/false);
        if (!shp.isNull()) {
            _hasShape = true;
            break;
        }
    }
    return _hasShape;
}

const std::vector<Part::TopoShape> &ShapeObserver::getWholeShapes()
{
    if ((_checked && !_hasShape) || _shapeChecked)
        return _shapes;
    _checked = true;
    _shapeChecked = true;
    for (const auto &sel : Gui::Selection().getSelectionT(nullptr, Gui::ResolveMode::NoResolve)) {
        if (!_sels.insert(sel.normalized(App::SubObjectT::NormalizeOption::NoElement)).second)
            continue;
        Part::TopoShape shp = Part::Feature::getTopoShape(sel.getObject(),
                                                          sel.getSubName().c_str(),
                                                          /*needElement*/false);
        if (!shp.isNull()) {
            _shapes.push_back(shp);
        }
    }
    _hasShape = !_shapes.empty();
    return _shapes;
}

const std::vector<Part::TopoShape> &ShapeObserver::getShapeElements()
{
    if ((_checked && !_hasShape) || _elementChecked)
        return _shapeElements;
    _elementChecked = true;
    for (const auto &sel : Gui::Selection().getSelectionT(nullptr, Gui::ResolveMode::NoResolve)) {
        Part::TopoShape shp = Part::Feature::getTopoShape(sel.getObject(),
                                                          sel.getSubName().c_str(),
                                                          /*needElement*/true);
        if (!shp.isNull()) {
            _shapeElements.push_back(shp);
        }
    }
    if (!_checked && !_shapeElements.empty()) {
        _checked = true;
        _hasShape = true;
    }
    return _shapeElements;
}

bool ShapeObserver::hasNonSolid()
{
    if (_solidsChecked)
        return _hasNoneSolid;

    _solidsChecked = true;
    _hasNoneSolid = false;
    TopExp_Explorer xp;
    for (const auto &shape : getWholeShapes()) {
        if (!shape.hasSubShape(TopAbs_SOLID)) {
            _hasNoneSolid = true;
            break;
        }
        xp.Init(shape.getShape(), TopAbs_SHELL, TopAbs_SOLID);
        if (xp.More()) {
            _hasNoneSolid = true;
            break;
        }
        xp.Init(shape.getShape(), TopAbs_FACE, TopAbs_SHELL);
        if (xp.More()) {
            _hasNoneSolid = true;
            break;
        }
        xp.Init(shape.getShape(), TopAbs_WIRE, TopAbs_FACE);
        if (xp.More()) {
            _hasNoneSolid = true;
            break;
        }
        xp.Init(shape.getShape(), TopAbs_EDGE, TopAbs_WIRE);
        if (xp.More()) {
            _hasNoneSolid = true;
            break;
        }
        xp.Init(shape.getShape(), TopAbs_VERTEX, TopAbs_EDGE);
        if (xp.More()) {
            _hasNoneSolid = true;
            break;
        }
    }
    return _hasNoneSolid;
}

void ShapeObserver::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    switch(msg.Type) {
    case Gui::SelectionChanges::AddSelection:
    case Gui::SelectionChanges::RmvSelection:
    case Gui::SelectionChanges::SetSelection:
    case Gui::SelectionChanges::ClrSelection:
        _hasBoundBox = false;
        _checked = false;
        _shapeChecked = false;
        _solidsChecked = false;
        _elementChecked = false;
        _shapes.clear();
        _shapeElements.clear();
        _sels.clear();
        break;
    default:
        break;
    }
}

const Base::BoundBox3d &ShapeObserver::getBoundBox()
{
    if (!_hasBoundBox) {
        _hasBoundBox = true;
        const auto &shapes = getWholeShapes();
        _boundBox = Base::BoundBox3d();
        for (const auto &shape : shapes) {
            _boundBox.Add(shape.getBoundBox());
        }
    }
    return _boundBox;
}
