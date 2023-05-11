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
#endif

#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/GeometryPyCXX.h>

#include <CXX/Objects.hxx>

#include <Gui/AbstractSplitViewPy.h>
#include <Gui/AbstractSplitViewPy.cpp>
#include "View3DInventorViewer.h"

using namespace Gui;

std::string AbstractSplitViewPy::representation(void) const
{
    return "<AbstractSplitView>";
}

PyObject *AbstractSplitViewPy::getCustomAttributes(const char* ) const
{
    return nullptr;
}

int AbstractSplitViewPy::setCustomAttributes(const char* /*attr*/, PyObject * /*obj*/)
{
    return 0;
}

PyObject* AbstractSplitViewPy::fitAll(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    try {
        getSplitViewPtr()->onMsg("ViewFit", nullptr);
    } PY_CATCH

    Py_Return;
}

PyObject* AbstractSplitViewPy::viewBottom(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    try {
        getSplitViewPtr()->onMsg("ViewBottom", nullptr);
    } PY_CATCH

    Py_Return;
}

PyObject* AbstractSplitViewPy::viewFront(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    try {
        getSplitViewPtr()->onMsg("ViewFront", nullptr);
    } PY_CATCH

    Py_Return;
}

PyObject* AbstractSplitViewPy::viewLeft(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    try {
        getSplitViewPtr()->onMsg("ViewLeft", nullptr);
    } PY_CATCH

    Py_Return;
}

PyObject* AbstractSplitViewPy::viewRear(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    try {
        getSplitViewPtr()->onMsg("ViewRear", nullptr);
    } PY_CATCH

    Py_Return;
}

PyObject* AbstractSplitViewPy::viewRight(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    try {
        getSplitViewPtr()->onMsg("ViewRight", nullptr);
    } PY_CATCH

    Py_Return;
}

PyObject* AbstractSplitViewPy::viewTop(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    try {
        getSplitViewPtr()->onMsg("ViewTop", nullptr);
    } PY_CATCH

    Py_Return;
}

PyObject* AbstractSplitViewPy::viewIsometric(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    try {
        getSplitViewPtr()->onMsg("ViewAxo", nullptr);
    } PY_CATCH

    Py_Return;
}

PyObject *AbstractSplitViewPy::getViewer(PyObject *args)
{
    int viewIndex;
    if (!PyArg_ParseTuple(args, "i", &viewIndex))
        return nullptr;

    try {
        Gui::View3DInventorViewer* view = getSplitViewPtr()->getViewer(viewIndex);
        if (!view)
            throw Py::IndexError("Index out of range");
        return view->getPyObject();
    } PY_CATCH
}

Py_ssize_t AbstractSplitViewPy::sequence_length(PyObject *self)
{
    if (!PyObject_TypeCheck(self, &(AbstractSplitViewPy::Type))) {
        PyErr_SetString(PyExc_TypeError, "first arg must be AbstractSplitViewPy");
        return -1;
    }
    auto view = static_cast<AbstractSplitViewPy*>(self)->getSplitViewPtr();
    return view->getSize();
}

PyObject * AbstractSplitViewPy::sequence_item (PyObject *self, Py_ssize_t index)
{
    if (!PyObject_TypeCheck(self, &(AbstractSplitViewPy::Type))) {
        PyErr_SetString(PyExc_TypeError, "first arg must be AbstractSplitViewPy");
        return nullptr;
    }
    if (index < 0 || index > 2) {
        PyErr_SetString(PyExc_IndexError, "index out of range");
        return nullptr;
    }

    auto view = static_cast<AbstractSplitViewPy*>(self)->getSplitViewPtr();
    if (index >= view->getSize() || index < 0) {
        PyErr_SetString(PyExc_IndexError, "Index out of range");
        return nullptr;
    }
    try {
        return view->getViewer(index)->getPyObject();
    } PY_CATCH
}
